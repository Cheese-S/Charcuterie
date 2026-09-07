#include <benchmark/IBenchmark.h>

#include <asset/import/Ir.h>
#include <asset/import/builder/ITemplateAssetBuilder.h>
#include <core/mlm/IRay.h>
#include <swiss/render/Bvh.h>

namespace mk::swiss::render
{

namespace
{

constexpr u32         kRngSeed = 0x9E3779B9u;
constexpr f32         kOneOver2Pow24 = 1.0f / 16777216.0f;
constexpr u32         kTraceRayCount = 1024;
constexpr const char* kBunnyPath = "../../../../../asset/bunny.glb";

// Deterministic xorshift32. A fixed seed guarantees identical geometry across
// runs and across benchmark functions, so construction and tracing share the
// same input.
class XorShift32
{
public:
    explicit XorShift32(u32 seed): state_(seed)
    {
    }

    u32 next()
    {
        u32 x = state_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state_ = x;
        return x;
    }

    f32 nextF32()
    {
        return static_cast<f32>(next() >> 8) * kOneOver2Pow24;
    }

private:
    u32 state_;
};

asset::ir::Ir makeDeterministicIr(u32 triCount)
{
    XorShift32 rng(kRngSeed);

    asset::ir::MeshPart part;
    part.positions.reserve(static_cast<usize>(triCount) * 3);
    part.indices.reserve(static_cast<usize>(triCount) * 3);

    for (u32 i = 0; i < triCount; i++)
    {
        for (u32 v = 0; v < 3; v++)
        {
            mlm::PackedVec3 p(rng.nextF32(), rng.nextF32(), rng.nextF32());
            part.positions.push(p);
            part.indices.push(static_cast<u16>(i * 3 + v));
            part.localBound.toInclude(p);
        }
    }

    asset::ir::Mesh mesh;
    mesh.parts.push(std::move(part));

    asset::ir::Ir ir;
    ir.entities.push({ .transform = mlm::mat4(), .meshIndex = 0, .children = {} });
    ir.meshes.push(std::move(mesh));

    return ir;
}

bool loadBunnyIr(asset::ir::Ir& outIr)
{
    return isOk(asset::TemplateAssetBuilder::build(fs::Path(kBunnyPath), outIr));
}

// Deterministic ray set: samples triangle centroids evenly and fires a ray one
// unit along the outward normal back at the surface.
Vector<mlm::Ray> generateRays(const asset::ir::Ir& ir, u32 count)
{
    const asset::ir::MeshPart& part = ir.meshes[0].parts[0];
    const mlm::mat4&           worldFromLocal = ir.entities[0].transform;

    const u32 triCount = static_cast<u32>(part.indices.size()) / 3;
    u32       step = triCount / count;
    if (step == 0)
    {
        step = 1;
    }

    Vector<mlm::Ray> rays;
    rays.reserve(count);

    for (u32 t = 0; t < triCount && rays.size() < count; t += step)
    {
        const mlm::PackedVec3& v0 = part.positions[part.indices[3 * t + 0]];
        const mlm::PackedVec3& v1 = part.positions[part.indices[3 * t + 1]];
        const mlm::PackedVec3& v2 = part.positions[part.indices[3 * t + 2]];

        mlm::PackedVec3 centroid = (v0 + v1 + v2) / 3.0f;
        mlm::PackedVec3 normal = mlm::normalize(mlm::cross(v1 - v0, v2 - v0));

        mlm::Ray localHit{ .o = centroid + normal, .d = normal / -1.0f };
        rays.push(mlm::transformRay(worldFromLocal, localHit));
    }

    return rays;
}

void BM_BvhConstructionSynthetic(benchmark::State& state)
{
    const u32     triCount = static_cast<u32>(state.range(0));
    asset::ir::Ir ir = makeDeterministicIr(triCount);

    for (auto _ : state)
    {
        asset::ir::Ir irCopy = ir;
        Bvh            bvh(std::move(irCopy));
        benchmark::DoNotOptimize(bvh);
    }
}
BENCHMARK(BM_BvhConstructionSynthetic)->Arg(1024)->Arg(4096)->Arg(16384)->Arg(20000);

void BM_BvhTracingSynthetic(benchmark::State& state)
{
    const u32        triCount = static_cast<u32>(state.range(0));
    asset::ir::Ir    ir = makeDeterministicIr(triCount);
    Vector<mlm::Ray> rays = generateRays(ir, kTraceRayCount);
    Bvh              bvh(std::move(ir));

    for (auto _ : state)
    {
        u32 hits = 0;
        for (const mlm::Ray& ray : rays)
        {
            if (bvh.intersect(ray, kF32Infinity))
            {
                hits++;
            }
        }
        benchmark::DoNotOptimize(hits);
    }
}
BENCHMARK(BM_BvhTracingSynthetic)->Arg(1024)->Arg(4096)->Arg(16384)->Arg(20000);

void BM_BvhConstructionBunny(benchmark::State& state)
{
    asset::ir::Ir ir;
    if (!loadBunnyIr(ir))
    {
        state.SkipWithError("Failed to load bunny.glb");
        return;
    }

    for (auto _ : state)
    {
        asset::ir::Ir irCopy = ir;
        Bvh            bvh(std::move(irCopy));
        benchmark::DoNotOptimize(bvh);
    }
}
BENCHMARK(BM_BvhConstructionBunny);

void BM_BvhTracingBunny(benchmark::State& state)
{
    asset::ir::Ir ir;
    if (!loadBunnyIr(ir))
    {
        state.SkipWithError("Failed to load bunny.glb");
        return;
    }

    Vector<mlm::Ray> rays = generateRays(ir, kTraceRayCount);
    Bvh              bvh(std::move(ir));

    for (auto _ : state)
    {
        u32 hits = 0;
        for (const mlm::Ray& ray : rays)
        {
            if (bvh.intersect(ray, kF32Infinity))
            {
                hits++;
            }
        }
        benchmark::DoNotOptimize(hits);
    }
}
BENCHMARK(BM_BvhTracingBunny);

} // namespace

} // namespace mk::swiss::render

MK_FULL_BENCHMARK_MAIN()

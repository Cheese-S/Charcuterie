#include <swiss/render/Bvh.h>
#include <algorithm>
#include <core/log/ILog.h>
#include <core/IAppContext.h>
#include <core/log/ILogCategory.h>
#include <core/mlm/IFormatter.h>

MK_ADD_AND_DEFINE_LOG_CATEGORY(Bvh, "Bvh");

namespace mk::swiss::render
{

Bvh::Bvh(asset::ir::Ir&& ir): ir_(std::move(ir))
{
    MK_ASSERT(!ir_.entities.empty());

    for (const auto& entity : ir_.entities)
    {
        if (entity.meshIndex == 0)
        {
            worldToLocal_ = mlm::mat4::inverse(entity.transform);
            break;
        }
    }

    buildPart(ir_.meshes[0].parts[0]);
    nodes_.shrinkToFit();
}

bool Bvh::intersect(const mlm::Ray& r, f32 tMax) const
{
    struct DfsEntry
    {
        const Node& node;
        usize       i;
    };

    const asset::ir::MeshPart& part = ir_.meshes[0].parts[0];

    mlm::Ray                  localR = mlm::transformRay(worldToLocal_, r);
    StackVector<DfsEntry, 64> dfsStack;

    dfsStack.push({ .node = nodes_[0], .i = 0 });

    mlm::PackedVec3 invD = mlm::PackedVec3(1.0f) / localR.d;
    u8              dirIsNeg[3] = { localR.d.x() < 0.0f, localR.d.y() < 0.0f, localR.d.z() < 0.0f };

    while (!dfsStack.empty())
    {
        DfsEntry    entry = dfsStack.pop();
        const Node& node = entry.node;

        if (!mlm::rayBoundIntersection(localR, node.localBound, tMax, invD, dirIsNeg))
        {
            continue;
        }

        if (node.triCount)
        {
            for (u32 i = 0; i < node.triCount; i++)
            {
                const u32              indexStart = tris_[node.triStart + i];
                // TODO(Cheese_S): two mem jumps. Probably not good.
                const mlm::PackedVec3& v0 = part.positions[part.indices[indexStart + 0]];
                const mlm::PackedVec3& v1 = part.positions[part.indices[indexStart + 1]];
                const mlm::PackedVec3& v2 = part.positions[part.indices[indexStart + 2]];

                if (mlm::rayTriIntersection(localR, tMax, v0, v1, v2))
                {
                    return true;
                }
            }
            continue;
        }

        dfsStack.push({ .node = nodes_[entry.i + 1], .i = entry.i + 1 });
        dfsStack.push({ .node = nodes_[node.rightChildIndex], .i = node.rightChildIndex });
    }

    return false;
}

void Bvh::buildPart(const asset::ir::MeshPart& part)
{
    MK_ASSERT((part.indices.size() % 3) == 0);

    Vector<SahTri> sahTris;
    tris_.reserve(2 * part.indices.size() / 3 - 1);
    sahTris.reserve(tris_.capacity());

    // NOLINTNEXTLINE(bugprone-too-small-loop-variable)
    for (u32 i = 0; i < part.indices.size(); i += 3)
    {
        const mlm::PackedVec3& v0 = part.positions[part.indices[i]];
        const mlm::PackedVec3& v1 = part.positions[part.indices[i + 1]];
        const mlm::PackedVec3& v2 = part.positions[part.indices[i + 2]];

        sahTris.push({
            .indexStart = i,
            .centeroid = (v0 + v1 + v2) / 3.0f,
            .bound = {},
        });

        sahTris.back().bound.toInclude(v0);
        sahTris.back().bound.toInclude(v1);
        sahTris.back().bound.toInclude(v2);
    }

    u32 triCount = part.indices.size() / 3;
    nodes_.reserve(triCount * 2 - 1);
    nodes_.push({
        .localBound = part.localBound,
        .rightChildIndex = 0,
        .triCount = 0,
        .axis = Axis::eX,
    });

    setSplitResult(nodes_[0], split(sahTris, 0, triCount), 0, triCount);

    for (SahTri& sahTri : sahTris)
    {
        tris_.push(sahTri.indexStart);
    }
}

// NOLINTBEGIN(misc-no-recursion)
std::optional<Bvh::SplitResult>
Bvh::split(Vector<SahTri>& tris, u32 parentTriStart, u16 parentTriCount)
// NOLINTEND(misc-no-recursion)
{
    if (parentTriCount == 1)
    {
        return std::nullopt;
    }

    mlm::Bound totalBound;
    mlm::Bound centeroidsBound;
    for (u32 i = 0; i < parentTriCount; i++)
    {
        centeroidsBound.toInclude(tris[parentTriStart + i].centeroid);
        totalBound.toInclude(tris[parentTriStart + i].bound);
    }

    Axis axis = chooseSplitAxis(centeroidsBound);

    if (parentTriCount == 2)
    {
        nodes_.push({
            .localBound = tris[parentTriStart].bound,
            .triStart = parentTriStart,
            .triCount = 1,
            .axis = eX,
        });

        nodes_.push({ .localBound = tris[parentTriStart + 1].bound,
                      .triStart = parentTriStart + 1,
                      .triCount = 1,
                      .axis = eX });

        return std::make_optional<SplitResult>(static_cast<u32>(nodes_.size() - 1), axis);
    }

    // Empirically found to be the best by PBRT
    SahBin bins[kNumSahBins];
    for (SahBin& bin : bins)
    {
        bin.count = 0;
    }

    for (u32 i = 0; i < parentTriCount; i++)
    {
        const SahTri& tri = tris[parentTriStart + i];
        u8            b = findBin(centeroidsBound, tri, axis);

        SahBin& bin = bins[b];

        bin.count++;
        bin.bound.toInclude(tri.bound);
    }

    // We consider the cost to be C = T_traversal + P_a * sum_(i=1)^N_a T_intersect + P_b *
    // sum_(i=1)^N_b T_intersect where P_a and P_b is the probability of traversing to left or right
    // respectively. P_a = a.surfaceArea() / total.surfaceArea()
    // We really only care about the the ratio between T_traversal and T_intersect. We set them to
    // 0.5 and 1 arbitriary

    struct SahBinAccmulation
    {
        mlm::Bound bound;
        u16        count;
    };

    static constexpr u8 kNumSahSplits = kNumSahBins - 1;
    f32                 costs[kNumSahSplits];
    SahBinAccmulation   accBelow[kNumSahSplits];
    SahBinAccmulation   accAbove[kNumSahSplits];

    for (i16 i = 0; i < kNumSahSplits; i++)
    {
        SahBin&            bin = bins[i];
        SahBinAccmulation& acc = accBelow[i];
        acc = {};

        costs[i] = 0;

        acc.count += bin.count;
        acc.bound.toInclude(bin.bound);
        if (i)
        {
            acc.bound.toInclude(accBelow[i - 1].bound);
            acc.count += accBelow[i - 1].count;
        }

        costs[i] += acc.count * acc.bound.surfaceArea();
    }

    u8  minCostBin = kU8Max;
    f32 minCost = FLT_MAX;
    for (i16 i = kNumSahSplits - 1; i >= 0; i--)
    {
        SahBin&            bin = bins[i + 1];
        SahBinAccmulation& acc = accAbove[i];
        acc = {};

        acc.count += bin.count;
        acc.bound.toInclude(bin.bound);
        if (i < kNumSahSplits - 1)
        {
            acc.bound.toInclude(accAbove[i + 1].bound);
            acc.count += accAbove[i + 1].count;
        }

        // TODO(Cheese_S): MIN COST BIN OR MIN COST SPLIT.
        costs[i] += acc.count * acc.bound.surfaceArea();
        costs[i] = costs[i] / totalBound.surfaceArea() + 0.5;
        if (costs[i] < minCost)
        {
            minCostBin = i;
            minCost = costs[i];
        }
    }

    MK_ASSERT(minCostBin != kU8Max);

    // T_intersect all tri in parent.
    if (minCost >= parentTriCount)
    {
        return std::nullopt;
    }

    MK_ASSERT(accBelow[minCostBin].count && accAbove[minCostBin].count);

    std::partition(tris.begin() + parentTriStart,
                   tris.begin() + parentTriStart + parentTriCount,
                   [&centeroidsBound, axis, minCostBin](const auto& tri)
                   { return findBin(centeroidsBound, tri, axis) <= minCostBin; });

    // Recursively split left
    usize leftChildIndex = nodes_.size();
    {
        u16 triCount = accBelow[minCostBin].count;
        u32 triStart = parentTriStart;

        nodes_.push({
            .localBound = accBelow[minCostBin].bound,
            .rightChildIndex = 0,
            .triCount = 0,
            .axis = Axis::eX,
        });

        setSplitResult(nodes_[leftChildIndex], split(tris, triStart, triCount), triStart, triCount);
    }

    // Recursively split right
    usize rightChildIndex = nodes_.size();
    {
        u16 triCount = accAbove[minCostBin].count;
        u32 triStart = parentTriStart + accBelow[minCostBin].count;

        nodes_.push({ .localBound = accAbove[minCostBin].bound,
                      .rightChildIndex = 0,
                      .triCount = 0,
                      .axis = Axis::eX });

        setSplitResult(nodes_[rightChildIndex],
                       split(tris, triStart, triCount),
                       triStart,
                       triCount);
    }
    return std::make_optional<SplitResult>(rightChildIndex, axis);
}

Bvh::Axis Bvh::chooseSplitAxis(const mlm::Bound& bound)
{
    mlm::PackedVec3 extent = bound.max() - bound.min();
    if (extent.x() >= extent.y() && extent.x() >= extent.z())
    {
        return Bvh::Axis::eX;
    }

    if (extent.y() >= extent.x() && extent.y() >= extent.z())
    {
        return Bvh::Axis::eY;
    }

    return Bvh::Axis::eZ;
}

u8 Bvh::findBin(const mlm::Bound& centeroidsBound, const SahTri& tri, Axis axis)
{
    MK_ASSERTF(mlm::allLessEqualThan(tri.centeroid, centeroidsBound.max()) &&
                   mlm::allGreaterEqualThan(tri.centeroid, centeroidsBound.min()),
               "bound must contain tri");

    f32 diff = tri.centeroid[axis] - centeroidsBound.min()[axis];
    if (diff > 0)
    {
        diff = diff / (centeroidsBound.max() - centeroidsBound.min())[axis];
    }

    u8 bin = diff * kNumSahBins;

    if (bin == kNumSahBins)
    {
        bin--;
    }

    return bin;
}

void Bvh::setSplitResult(Bvh::Node&                 node,
                         std::optional<SplitResult> result,
                         u32                        nodeTriStart,
                         u16                        nodeTriCount)
{
    if (!result)
    {
        node.triStart = nodeTriStart;
        node.triCount = nodeTriCount;
        return;
    }

    MK_ASSERT(!node.triCount);
    node.axis = result->axis;
    node.rightChildIndex = result->rightChildIndex;
}

} // namespace mk::swiss::render

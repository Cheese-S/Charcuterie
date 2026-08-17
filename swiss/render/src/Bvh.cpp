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
    MK_ASSERT(!ir_.meshes.empty());
    for (const auto& node : ir_.nodes)
    {
        if (node.meshIndex == 0)
        {
            worldToLocal_ = mlm::mat4::inverse(node.transform);
            break;
        }
    }

    buildPart(ir_.meshes[0].parts[0]);
}

bool Bvh::intersect(const mlm::Ray& r, f32 tMax) const
{
    const asset::ir::MeshPart& part = ir_.meshes[0].parts[0];

    mlm::Ray                     localR = mlm::transformRay(worldToLocal_, r);
    StackVector<const Node*, 64> dfsStack;

    // NOLINTNEXTLINE
    dfsStack.push(&nodes_[0]);

    mlm::PackedVec3 invD = mlm::PackedVec3(1.0f) / localR.d;
    u8              dirIsNeg[3] = { localR.d.x() < 0.0f, localR.d.y() < 0.0f, localR.d.z() < 0.0f };

    while (!dfsStack.empty())
    {
        const Node* node = dfsStack.pop();

        if (!mlm::rayBoundIntersection(localR, node->localBound, tMax, invD, dirIsNeg))
        {
            continue;
        }

        if (!node->left && !node->right)
        {
            for (u32 i = 0; i < node->triCount; i++)
            {
                const Tri&             tri = tris_[node->triStart + i];
                // TODO(Cheese_S): two mem jumps. Probably not good.
                const mlm::PackedVec3& v0 = part.positions[part.indices[tri.indexStart + 0]];
                const mlm::PackedVec3& v1 = part.positions[part.indices[tri.indexStart + 1]];
                const mlm::PackedVec3& v2 = part.positions[part.indices[tri.indexStart + 2]];

                if (mlm::rayTriIntersection(localR, tMax, v0, v1, v2))
                {
                    return true;
                }
            }
            continue;
        }

        dfsStack.push(node->left);
        dfsStack.push(node->right);
    }

    return false;
}

void Bvh::buildPart(const asset::ir::MeshPart& part)
{
    MK_ASSERT((part.indices.size() % 3) == 0);

    tris_.reserve(part.indices.size() / 3);

    // NOLINTNEXTLINE(bugprone-too-small-loop-variable)
    for (u32 i = 0; i < part.indices.size(); i += 3)
    {
        const mlm::PackedVec3& v0 = part.positions[part.indices[i]];
        const mlm::PackedVec3& v1 = part.positions[part.indices[i + 1]];
        const mlm::PackedVec3& v2 = part.positions[part.indices[i + 2]];

        tris_.push({
            .indexStart = i,
            .centeroid = (v0 + v1 + v2) / 3.0f,
            .bound = {},
        });

        tris_.back().bound.include(v0);
        tris_.back().bound.include(v1);
        tris_.back().bound.include(v2);
    }

    // TODO(Cheese_S): we will have TLAS and BLAS. SO u16 is not good enough.
    Bvh::Node root = { .left = nullptr,
                       .right = nullptr,
                       .localBound = part.localBound,
                       .triStart = 0,
                       .triCount = static_cast<u16>(part.indices.size() / 3) };

    nodes_.reserve(root.triCount);
    nodes_.push(root);

    split(root);
}

void Bvh::split(Bvh::Node& parent)
{
    MK_ASSERTF(parent.triCount >= 2, "Unexpected triCount: {}", parent.triCount);
    MK_ASSERT(!parent.left && !parent.right);

    if (parent.triCount == 2)
    {
        nodes_.push({ .left = nullptr,
                      .right = nullptr,
                      .localBound = tris_[parent.triStart].bound,
                      .triStart = parent.triStart,
                      .triCount = 1 });
        parent.left = &nodes_.back();
        nodes_.push({ .left = nullptr,
                      .right = nullptr,
                      .localBound = tris_[parent.triStart + 1].bound,
                      .triStart = static_cast<u16>(parent.triStart + 1),
                      .triCount = 1 });
        parent.right = &nodes_.back();
        return;
    }

    mlm::Bound totalBound;
    mlm::Bound centeroidsBound;
    for (u32 i = 0; i < parent.triCount; i++)
    {
        centeroidsBound.include(tris_[parent.triStart + i].centeroid);
        totalBound.include(tris_[parent.triStart + i].bound);
    }

    Axis axis = chooseSplitAxis(centeroidsBound);

    // Empirically found to be the best by PBRT
    SahBin bins[kNumSahBins];
    for (SahBin& bin : bins)
    {
        bin.count = 0;
    }

    for (u32 i = 0; i < parent.triCount; i++)
    {
        const Tri& tri = tris_[parent.triStart + i];
        u8         b = findBin(centeroidsBound, tri, axis);

        SahBin& bin = bins[b];

        bin.count++;
        bin.bound.include(tri.bound);
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
        acc.bound.include(bin.bound);
        if (i)
        {
            acc.bound.include(accBelow[i - 1].bound);
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
        acc.bound.include(bin.bound);
        if (i < kNumSahSplits - 1)
        {
            acc.bound.include(accAbove[i + 1].bound);
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

    // T_intersect all tri in parent = parent.triCount
    if (minCost >= parent.triCount)
    {
        return;
    }

    std::partition(tris_.begin() + parent.triStart,
                   tris_.begin() + parent.triStart + parent.triCount,
                   [&centeroidsBound, axis, minCostBin](const auto& tri)
                   { return findBin(centeroidsBound, tri, axis) <= minCostBin; });

    nodes_.push({ .left = nullptr,
                  .right = nullptr,
                  .localBound = accBelow[minCostBin].bound,
                  .triStart = parent.triStart,
                  .triCount = accBelow[minCostBin].count });

    parent.left = &nodes_.back();

    if (nodes_.back().triCount > 1)
    {
        split(nodes_.back());
    }

    nodes_.push({ .left = nullptr,
                  .right = nullptr,
                  .localBound = accAbove[minCostBin].bound,
                  .triStart = static_cast<u16>(parent.triStart + accBelow[minCostBin].count),
                  .triCount = accAbove[minCostBin].count });

    parent.right = &nodes_.back();

    if (nodes_.back().triCount > 1)
    {
        MK_LOG_DEBUG("splitting right: {}, parent.triStart: {}, minCostBin: {}",
                     nodes_.back().triStart,
                     parent.triStart,
                     accBelow[minCostBin].count);
        split(nodes_.back());
    }
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

u8 Bvh::findBin(const mlm::Bound& centeroidsBound, const Tri& tri, Axis axis)
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

} // namespace mk::swiss::render

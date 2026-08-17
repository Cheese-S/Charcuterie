#pragma once
#include <core/mlm/IMlm.h>
#include <fmt/format.h>

template<mk::Vec3Type V>
struct fmt::formatter<V>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const V& v, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({}, {}, {})", v.x(), v.y(), v.z());
    }
};

#pragma once

#include <core/math/IMlm.h>

namespace mk::swiss::render
{
class PerspectiveCamera
{
public:
    PerspectiveCamera(const mlm::Transform& worldToCamera,
                      mlm::u16vec2          resolution,
                      f32                   fovY,
                      f32                   n,
                      f32                   f);

    mlm::Ray sampleRay(u16 pixelX, u16 pixelY);

    mlm::u16vec2 getResolution() const
    {
        return resolution_;
    };

private:
    mlm::u16vec2 resolution_;

    // TODO(Cheese_S): Probably replace this
    mlm::Transform worldToCamera_;
    mlm::Transform rasterToCamera_;
};
} // namespace mk::swiss::render

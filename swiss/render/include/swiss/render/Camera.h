#pragma once

#include <core/mlm/IRay.h>

namespace mk::swiss::render
{
class PerspectiveCamera
{
public:
    PerspectiveCamera(const mlm::mat4& worldToCamera,
                      mlm::u16vec2     resolution,
                      f32              fovY,
                      f32              n,
                      f32              f);

    mlm::Ray sampleRay(u16 pixelX, u16 pixelY);

    mlm::u16vec2 getResolution() const
    {
        return resolution_;
    };

private:
    mlm::u16vec2 resolution_;

    // TODO(Cheese_S): Probably replace this
    mlm::mat4 worldToCamera_;
    mlm::mat4 cameraToWorld_;
    mlm::mat4 rasterToCamera_;
};
} // namespace mk::swiss::render

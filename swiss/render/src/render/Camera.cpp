#include <swiss/render/Camera.h>
#include <core/IAssert.h>

namespace mk::swiss::render
{
// Raster Space
// ------------------------
// |(0, 0)
// |
// |
// |
// |                 (w, h)
//

// NDC
//     +-----------------+
//    / .               /|
//   /   .             / |
//  /     .           /  |
// +-----------------+   |
// (0,0,0)+ . . . . .| . + <- (1,1,1)
// |     .           |  /
// |    .            | /
// |   .             |/
// +-----------------+
// (Top Left) -> (0,0,0)
// (Bot Right) -> (1,1,1)

// Screen Space
//               + (0, 1)
//               |
//               |
// (-1, 0)       | (0, 0)      (1, 0)
// +-------------+--------------+
//               |
//               |
//               |
//               + (0, -1)

PerspectiveCamera::PerspectiveCamera(const mlm::mat4& worldToCamera,
                                     mlm::u16vec2     resolution,
                                     f32              fovY,
                                     f32              n,
                                     f32              f):
    resolution_(resolution), worldToCamera_(worldToCamera),
    cameraToWorld_(mlm::mat4::inverse(worldToCamera_))
{
    f32       cotHalfFovY = 1.0F / (std::tan(fovY / 2));
    // clang-format off
    mlm::mat4 cameraToScreen = mlm::mat4::scale(cotHalfFovY, cotHalfFovY, 1) * mlm::mat4(
          1, 0, 0, 0, 
          0, 1, 0, 0, 
          0, 0, f / (f - n), -(n * f) / (f - n), 
          0, 0, 1, 0
        );
    // clang-format on
    mlm::mat4 screenToCamera = mlm::mat4::inverse(cameraToScreen);

    MK_ASSERT(resolution.x() >= resolution.y());
    f32 aspect = static_cast<f32>(resolution_.x()) / resolution_.y();
    f32 screenX = 2 * aspect;
    f32 screenY = 2;

    mlm::mat4 screenToRaster = mlm::mat4::scale(resolution.x(), resolution.y(), 1) *
                               mlm::mat4::scale(1 / screenX, -1 / screenY, 1) *
                               mlm::mat4::translate(aspect, -1, 0);
    mlm::mat4 rasterToScreen = mlm::mat4::inverse(screenToRaster);
    rasterToCamera_ = screenToCamera * rasterToScreen;
}

mlm::Ray PerspectiveCamera::sampleRay(u16 pixelX, u16 pixelY)
{
    f32             rasterX = (pixelX + 0.5F);
    f32             rasterY = (pixelY + 0.5F);
    mlm::PackedVec3 pt = mlm::vec3(rasterX, rasterY, 0.0f);
    mlm::PackedVec3 o = mlm::PackedVec3(0, 0, 0);
    return mlm::transformRay(cameraToWorld_,
                             mlm::Ray{
                                 .o = o,
                                 .d = mlm::normalize(mlm::transformPoint(rasterToCamera_, pt)),
                             });
}

} // namespace mk::swiss::render

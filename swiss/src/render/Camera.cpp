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

PerspectiveCamera::PerspectiveCamera(const mlm::Transform& worldToCamera,
                                     mlm::u16vec2          resolution,
                                     f32                   fovY,
                                     f32                   n,
                                     f32 f): resolution_(resolution), worldToCamera_(worldToCamera)
{
    f32            cotHalfFovY = 1.0F / (std::tan(fovY / 2));
    // clang-format off
    mlm::Transform cameraToScreen = mlm::Transform::scale(cotHalfFovY, cotHalfFovY, 1) * mlm::Transform(
        { 1, 0, 0, 0, 
          0, 1, 0, 0, 
          0, 0, f / (f - n), -(n * f) / (f - n), 
          0, 0, 1, 0,
        });
    // clang-format on
    mlm::Transform screenToCamera = mlm::Transform::inverse(cameraToScreen);

    MK_ASSERT(resolution.x() >= resolution.y());
    f32 aspect = static_cast<f32>(resolution_.x()) / resolution_.y();
    f32 screenX = 2 * aspect;
    f32 screenY = 2;

    mlm::Transform screenToRaster = mlm::Transform::scale(resolution.x(), resolution.y(), 1) *
                                    mlm::Transform::scale(1 / screenX, -1 / screenY, 1) *
                                    mlm::Transform::translate(aspect, -1, 0);
    mlm::Transform rasterToScreen = mlm::Transform::inverse(screenToRaster);
    rasterToCamera_ = screenToCamera * rasterToScreen;
}

mlm::Ray PerspectiveCamera::sampleRay(u16 pixelX, u16 pixelY)
{
    f32 rasterX = (pixelX + 0.5F);
    f32 rasterY = (pixelY + 0.5F);

    mlm::Point pt = rasterToCamera_ * mlm::Point(rasterX, rasterY, 0.0f);
    mlm::Point o = mlm::Point(0, 0, 0);
    return mlm::Ray{
        .o = o,
        .d = mlm::Vector::normalize(mlm::Vector(pt)),
    };
}

} // namespace mk::swiss::render

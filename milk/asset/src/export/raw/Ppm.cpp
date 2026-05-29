#include <core/log/ILog.h>
#include <core/log/ILogCategory.h>
#include <core/IAppContext.h>
#include <asset/export/raw/IPpm.h>
#include <core/filesystem/IFile.h>

MK_DEFINE_DEFAULT_LOG_CATEGORY(Asset);

namespace mk::asset::exp
{

Result savePpm(const fs::Path& path, VectorView<float> data, u16 width, u16 height)
{
    MK_ASSERT(data.size() == width * height * 3);
    fs::IFileHandlePtr handle;
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(fs::openUniqueFile(path, fs::FileAccessMode::eWrite, handle),
                                      "Failed to open file ");

    StackString<1024> header;
    // p6 means it's binary
    header.fmt("P6\n{} {}\n255\n", width, height);

    Vector<u8> quantized;
    quantized.reserve(data.size());
    for (float f : data)
    {
        quantized.push(std::round(std::clamp(f, 0.F, 1.F) * 255.0F));
    }

    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(handle->write(header.begin(), header.size()),
                                      "Failed to write the ppm header.");
    MK_LOG_ERROR_AND_RETURN_IF_NOT_OK(handle->write(quantized), "Failed to write the ppm header.");

    return Result::eOk;
}
} // namespace mk::asset::exp

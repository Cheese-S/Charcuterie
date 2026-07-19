#include <algorithm>

#include <core/log/ILog.h>
#include <asset/LogCategory.h>
#include <core/IAppContext.h>
#include <asset/export/raw/IPpm.h>
#include <core/filesystem/IFileUtil.h>
#include <core/io/IBufferedWriter.h>

MK_DEFINE_DEFAULT_LOG_CATEGORY(Asset);

namespace mk::asset::exp
{

Result savePpm(const fs::Path& path, VectorView<float> data, u16 width, u16 height)
{
    MK_ASSERT(data.size() == width * height * 3);

    StackString<1024> header;
    // p6 means it's binary
    header.fmt("P6\n{} {}\n255\n", width, height);

    io::BufferedWriter writer;
    writer.reserve(header.size() + data.size());
    writer << StringView(header.data(), header.size());

    for (float f : data)
    {
        u8 value = std::round(std::clamp(f, 0.F, 1.F) * 255.0F);
        writer << value;
    }

    MK_RETURN_IF_NOT_OK(fs::util::writeBinaryFile(path, writer.getView()));
    return Result::eOk;
}
} // namespace mk::asset::exp

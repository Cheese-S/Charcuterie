#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/container/IString.h>
#include <core/container/IVector.h>
#include <core/filesystem/IFile.h>
#include <core/filesystem/IPath.h>

namespace mk::fs
{

// ─────────────────────────────────────────────
// Mock
// ─────────────────────────────────────────────

class MockFileHandle: public IFileHandle
{
public:
    MOCK_METHOD(Result, read, (Vector<u8> & dst), (override));
    MOCK_METHOD(Result, read, (String & dst), (override));
    MOCK_METHOD(Result, read, (void* dst, usize dstSize), (override));
    MOCK_METHOD(Result, write, (VectorView<u8> data), (override));
    MOCK_METHOD(Result, write, (const void* buf, usize size), (override));
    MOCK_METHOD(Result, write, (StringView str), (override));
    MOCK_METHOD(usize, size, (), (override));
};

// ─────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────

class FileHandleTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        testPath_ = Path("mk_filehandle_test_tmp.txt");
        MK_IGNORE_RET_VAL
        removeFile(testPath_);
    }

    void TearDown() override
    {
        MK_IGNORE_RET_VAL
        removeFile(testPath_);
    }

    Path testPath_ = Path("");
};

// ─────────────────────────────────────────────
// fileExist / removeFile
// ─────────────────────────────────────────────

TEST_F(FileHandleTest, FileDoesNotExistBeforeCreation)
{
    EXPECT_FALSE(fileExist(testPath_));
}

TEST_F(FileHandleTest, FileExistsAfterWriteOpen)
{
    IFileHandle* handle = nullptr;
    ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
    delete handle;
    EXPECT_TRUE(fileExist(testPath_));
}

TEST_F(FileHandleTest, RemoveFileDeletesFile)
{
    IFileHandle* handle = nullptr;
    ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
    delete handle;
    ASSERT_TRUE(fileExist(testPath_));
    EXPECT_EQ(removeFile(testPath_), Result::eOk);
    EXPECT_FALSE(fileExist(testPath_));
}

TEST_F(FileHandleTest, RemoveNonExistentFileFails)
{
    EXPECT_NE(removeFile(testPath_), Result::eOk);
}

// ─────────────────────────────────────────────
// openFile — access mode behaviour
// ─────────────────────────────────────────────

TEST_F(FileHandleTest, OpenReadFailsWhenFileAbsent)
{
    IFileHandle* handle = nullptr;
    EXPECT_NE(openFile(testPath_, FileAccessMode::eRead, &handle), Result::eOk);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(FileHandleTest, OpenWriteCreatesNewFile)
{
    IFileHandle* handle = nullptr;
    EXPECT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
    ASSERT_NE(handle, nullptr);
    delete handle;
}

TEST_F(FileHandleTest, OpenAppendCreatesNewFile)
{
    IFileHandle* handle = nullptr;
    EXPECT_EQ(openFile(testPath_, FileAccessMode::eAppend, &handle), Result::eOk);
    ASSERT_NE(handle, nullptr);
    delete handle;
}

TEST_F(FileHandleTest, OpenReadAndWriteFailsWhenFileAbsent)
{
    IFileHandle* handle = nullptr;
    EXPECT_NE(openFile(testPath_, FileAccessMode::eReadAndWrite, &handle), Result::eOk);
    EXPECT_EQ(handle, nullptr);
}

TEST_F(FileHandleTest, OpenReadAndOverwriteCreatesNewFile)
{
    IFileHandle* handle = nullptr;
    EXPECT_EQ(openFile(testPath_, FileAccessMode::eReadAndOverwrite, &handle),
              Result::eOk);
    ASSERT_NE(handle, nullptr);
    delete handle;
}

TEST_F(FileHandleTest, OpenReadAndAppendCreatesNewFile)
{
    IFileHandle* handle = nullptr;
    EXPECT_EQ(openFile(testPath_, FileAccessMode::eReadAndAppend, &handle), Result::eOk);
    ASSERT_NE(handle, nullptr);
    delete handle;
}

// ─────────────────────────────────────────────
// write / read roundtrip — StringView
// ─────────────────────────────────────────────

TEST_F(FileHandleTest, WriteAndReadStringRoundtrip)
{
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->write(StringView("hello world")), Result::eOk);
        delete handle;
    }
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eRead, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        String content;
        content.resize(11);
        EXPECT_EQ(handle->read(content), Result::eOk);
        EXPECT_TRUE(content == "hello world");
        delete handle;
    }
}

TEST_F(FileHandleTest, WriteAndReadBytesRoundtrip)
{
    const u8        kPayload[] = { 0x01, 0x02, 0x03, 0xFF };
    constexpr usize kPayloadSize = sizeof(kPayload);

    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->write(static_cast<const void*>(kPayload), kPayloadSize),
                  Result::eOk);
        delete handle;
    }
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eRead, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        Vector<u8> dst;
        dst.resize(4);
        EXPECT_EQ(handle->read(dst), Result::eOk);
        ASSERT_EQ(dst.size(), kPayloadSize);
        for (usize i = 0; i < kPayloadSize; ++i)
        {
            EXPECT_EQ(dst[static_cast<int>(i)], kPayload[i]);
        }
        delete handle;
    }
}

TEST_F(FileHandleTest, WriteAndReadVoidPtrRoundtrip)
{
    const char      kData[] = "raw buffer";
    constexpr usize kDataSize = sizeof(kData) - 1;

    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->write(static_cast<const void*>(kData), kDataSize), Result::eOk);
        delete handle;
    }
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eRead, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        char buf[64] = {};
        EXPECT_EQ(handle->read(static_cast<void*>(buf), kDataSize), Result::eOk);
        EXPECT_EQ(strncmp(buf, kData, kDataSize), 0);
        delete handle;
    }
}

// ─────────────────────────────────────────────
// size
// ─────────────────────────────────────────────

TEST_F(FileHandleTest, SizeMatchesBytesWritten)
{
    StringView content("hello");
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->write(content), Result::eOk);
        delete handle;
    }
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eRead, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->size(), content.size());
        delete handle;
    }
}

TEST_F(FileHandleTest, SizeOfEmptyFileIsZero)
{
    IFileHandle* handle = nullptr;
    ASSERT_EQ(openFile(testPath_, FileAccessMode::eWrite, &handle), Result::eOk);
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(handle->size(), 0U);
    delete handle;
}

// ─────────────────────────────────────────────
// Append mode
// ─────────────────────────────────────────────

TEST_F(FileHandleTest, AppendModeAccumulatesContent)
{
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eAppend, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->write(StringView("hello")), Result::eOk);
        delete handle;
    }
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eAppend, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        EXPECT_EQ(handle->write(StringView(" world")), Result::eOk);
        delete handle;
    }
    {
        IFileHandle* handle = nullptr;
        ASSERT_EQ(openFile(testPath_, FileAccessMode::eRead, &handle), Result::eOk);
        ASSERT_NE(handle, nullptr);
        String content;
        content.resize(11);
        EXPECT_EQ(handle->read(content), Result::eOk);
        EXPECT_TRUE(content == "hello world");
        delete handle;
    }
}

// ─────────────────────────────────────────────
// Mock — interface contract
// Disambiguate overloaded methods with ::testing::An<Type>()
// ─────────────────────────────────────────────

TEST(FileHandleMock, ReadStringDelegatesToImpl)
{
    MockFileHandle mock;
    EXPECT_CALL(mock, read(::testing::An<String&>()))
        .WillOnce(::testing::Return(Result::eOk));
    String dst;
    EXPECT_EQ(mock.read(dst), Result::eOk);
}

TEST(FileHandleMock, ReadBytesVecDelegatesToImpl)
{
    MockFileHandle mock;
    EXPECT_CALL(mock, read(::testing::An<Vector<u8>&>()))
        .WillOnce(::testing::Return(Result::eOk));
    Vector<u8> dst;
    EXPECT_EQ(mock.read(dst), Result::eOk);
}

TEST(FileHandleMock, WriteStringViewDelegatesToImpl)
{
    MockFileHandle mock;
    EXPECT_CALL(mock, write(::testing::An<StringView>()))
        .WillOnce(::testing::Return(Result::eOk));
    EXPECT_EQ(mock.write(StringView("test")), Result::eOk);
}

TEST(FileHandleMock, WriteReturnsErrorOnFailure)
{
    MockFileHandle mock;
    EXPECT_CALL(mock, write(::testing::An<StringView>()))
        .WillOnce(::testing::Return(Result::eInvalidPermission));
    EXPECT_EQ(mock.write(StringView("test")), Result::eInvalidPermission);
}

TEST(FileHandleMock, SizeReturnsExpectedValue)
{
    MockFileHandle mock;
    EXPECT_CALL(mock, size()).WillOnce(::testing::Return(42U));
    EXPECT_EQ(mock.size(), 42U);
}

} // namespace mk::fs

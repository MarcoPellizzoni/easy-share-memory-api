
#include "mshm.h"
#include "gtest/gtest.h"

#include <cstddef>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

struct TestStruct
{
    int    foo;
    double bar;
};

// ============================================================
// shmem_open / shmem_close
// ============================================================

TEST(ShmOpen, BasicOpenClose)
{
    mshm::mshm_handle handle = nullptr;

    auto ret = mshm::shmem_open(handle, "mshm_test_basic", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
    EXPECT_NE(handle, nullptr);

    ret = mshm::shmem_close(handle);
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
}

TEST(ShmOpen, EmptyName)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
    EXPECT_EQ(handle, nullptr);
}

TEST(ShmOpen, NameWithSpaces)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "invalid name", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
    EXPECT_EQ(handle, nullptr);
}

TEST(ShmOpen, NameWithSlash)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "invalid/name", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
    EXPECT_EQ(handle, nullptr);
}

TEST(ShmOpen, NameWithDot)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "invalid.name", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
    EXPECT_EQ(handle, nullptr);
}

TEST(ShmOpen, NameTooLong)
{
    mshm::mshm_handle handle = nullptr;
    std::string long_name(256, 'a');
    auto ret = mshm::shmem_open(handle, long_name.c_str(), sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
    EXPECT_EQ(handle, nullptr);
}

TEST(ShmOpen, ValidNameWithUnderscoreAndDash)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "valid_name-123", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
    EXPECT_NE(handle, nullptr);

    mshm::shmem_close(handle);
}

TEST(ShmOpen, ZeroSize)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "mshm_test_zerosize", 0);
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
    EXPECT_EQ(handle, nullptr);
}

TEST(ShmOpen, CloseNullHandle)
{
    auto ret = mshm::shmem_close(nullptr);
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_NOT_OPEN);
}

// ============================================================
// Fixture for write / read tests
// Each test gets a fresh shared memory segment.
// ============================================================

class ShmWriteRead : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto ret = mshm::shmem_open(handle, "mshm_test_rw", sizeof(TestStruct));
        ASSERT_EQ(ret.error_code, mshm::SHMEM_OK);
    }

    void TearDown() override
    {
        mshm::shmem_close(handle);
    }

    mshm::mshm_handle handle = nullptr;
};

TEST_F(ShmWriteRead, NewSegmentIsZeroInitialized)
{
    TestStruct dst{1, 1.0}; // pre-fill with non-zero
    auto ret = mshm::shmem_read(handle, &dst, sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
    EXPECT_EQ(dst.foo, 0);
    EXPECT_DOUBLE_EQ(dst.bar, 0.0);
}

TEST_F(ShmWriteRead, WriteAndReadRoundtrip)
{
    TestStruct src{42, 3.14};
    auto ret = mshm::shmem_write(handle, &src, sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);

    TestStruct dst{};
    ret = mshm::shmem_read(handle, &dst, sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
    EXPECT_EQ(dst.foo, src.foo);
    EXPECT_DOUBLE_EQ(dst.bar, src.bar);
}

TEST_F(ShmWriteRead, PartialWriteAtOffset)
{
    TestStruct src{10, 2.71};
    mshm::shmem_write(handle, &src, sizeof(TestStruct));

    // Overwrite only 'foo' (at offset 0)
    int new_foo = 99;
    auto ret = mshm::shmem_write(handle, &new_foo, sizeof(int), 0);
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);

    TestStruct dst{};
    mshm::shmem_read(handle, &dst, sizeof(TestStruct));
    EXPECT_EQ(dst.foo, 99);
    EXPECT_DOUBLE_EQ(dst.bar, 2.71);
}

TEST_F(ShmWriteRead, PartialReadAtOffset)
{
    TestStruct src{7, 1.23};
    mshm::shmem_write(handle, &src, sizeof(TestStruct));

    double bar_out = 0.0;
    auto ret = mshm::shmem_read(handle, &bar_out, sizeof(double), offsetof(TestStruct, bar));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
    EXPECT_DOUBLE_EQ(bar_out, 1.23);
}

TEST_F(ShmWriteRead, WriteExactlyFitsBuffer)
{
    std::vector<uint8_t> buf(sizeof(TestStruct), 0xAB);
    auto ret = mshm::shmem_write(handle, buf.data(), sizeof(TestStruct), 0);
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
}

TEST_F(ShmWriteRead, WriteExceedsBuffer)
{
    std::vector<uint8_t> buf(sizeof(TestStruct) + 1, 0xFF);
    auto ret = mshm::shmem_write(handle, buf.data(), buf.size());
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
}

TEST_F(ShmWriteRead, ReadExceedsBuffer)
{
    std::vector<uint8_t> buf(sizeof(TestStruct) + 1);
    auto ret = mshm::shmem_read(handle, buf.data(), buf.size());
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
}

TEST_F(ShmWriteRead, OffsetPlusSizeExceedsBuffer)
{
    // offset is valid, but offset + size overflows the segment
    int val = 1;
    auto ret = mshm::shmem_write(handle, &val, sizeof(int), sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
}

TEST_F(ShmWriteRead, WriteNullSrc)
{
    auto ret = mshm::shmem_write(handle, nullptr, sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
}

TEST_F(ShmWriteRead, ReadNullDst)
{
    auto ret = mshm::shmem_read(handle, nullptr, sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_PARAM);
}

// ============================================================
// Operations on null / invalid handles
// ============================================================

TEST(ShmNullHandle, WriteToNull)
{
    int val = 1;
    auto ret = mshm::shmem_write(nullptr, &val, sizeof(int));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_NOT_OPEN);
}

TEST(ShmNullHandle, ReadFromNull)
{
    int val = 0;
    auto ret = mshm::shmem_read(nullptr, &val, sizeof(int));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_NOT_OPEN);
}

// ============================================================
// shmem_delete
// ============================================================

TEST(ShmDelete, DeleteBehavior)
{
    auto ret = mshm::shmem_delete("mshm_test_delete");
#ifdef _WIN32
    // Windows: shared memory is reference-counted by the OS.
    // shmem_delete is intentionally a no-op and always returns SHMEM_ERR_DELETE.
    EXPECT_EQ(ret.error_code, mshm::SHMEM_ERR_DELETE);
#else
    // Linux: succeeds if the object was previously created, fails otherwise.
    // Either outcome is acceptable here; the call must not crash.
    (void)ret;
#endif
}

#ifndef _WIN32
TEST(ShmDelete, DeleteAfterOpen)
{
    mshm::mshm_handle handle = nullptr;
    mshm::shmem_open(handle, "mshm_test_deleteopen", sizeof(int));
    mshm::shmem_close(handle);

    auto ret = mshm::shmem_delete("mshm_test_deleteopen");
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
}
#endif

// ============================================================
// Two handles on the same segment (shared state)
// ============================================================

TEST(ShmMultiHandle, TwoHandlesSeeSharedData)
{
    mshm::mshm_handle h1 = nullptr, h2 = nullptr;
    const char* name = "mshm_test_multi";

    auto ret = mshm::shmem_open(h1, name, sizeof(int));
    ASSERT_EQ(ret.error_code, mshm::SHMEM_OK);

    ret = mshm::shmem_open(h2, name, sizeof(int));
    ASSERT_EQ(ret.error_code, mshm::SHMEM_OK);

    int val_w = 12345;
    ret = mshm::shmem_write(h1, &val_w, sizeof(int));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);

    int val_r = 0;
    ret = mshm::shmem_read(h2, &val_r, sizeof(int));
    EXPECT_EQ(ret.error_code, mshm::SHMEM_OK);
    EXPECT_EQ(val_r, 12345);

    mshm::shmem_close(h1);
    mshm::shmem_close(h2);
}

// ============================================================
// Concurrent read / write (mutex protection smoke test)
// ============================================================

TEST(ShmConcurrent, ConcurrentReadWrite)
{
    mshm::mshm_handle handle = nullptr;
    auto ret = mshm::shmem_open(handle, "mshm_test_concurrent", sizeof(int));
    ASSERT_EQ(ret.error_code, mshm::SHMEM_OK);

    std::atomic<bool> stop{false};

    std::thread writer([&]() {
        for (int i = 0; i < 500; ++i)
        {
            mshm::shmem_write(handle, &i, sizeof(int));
        }
        stop = true;
    });

    std::thread reader([&]() {
        int val = 0;
        while (!stop)
        {
            // Must not crash or return an error (other than SHMEM_OK)
            auto r = mshm::shmem_read(handle, &val, sizeof(int));
            EXPECT_EQ(r.error_code, mshm::SHMEM_OK);
        }
    });

    writer.join();
    reader.join();

    mshm::shmem_close(handle);
}

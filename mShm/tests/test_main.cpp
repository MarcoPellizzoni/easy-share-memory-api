
#include "mshm.h"
#include "gtest/gtest.h"


struct TestStruct
{
    int     foo;
    double  bar;
};


TEST(ShmTest, ShmOpenClose)
{
    mshm::mshm_handle handle = nullptr;

    auto ret = mshm::shmem_open(handle, "foo", sizeof(TestStruct));
    EXPECT_EQ(ret.error_code, 0);

    ret = mshm::shmem_close(handle);
    EXPECT_EQ(ret.error_code, 0);
}

// write other tests
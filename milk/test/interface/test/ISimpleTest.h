#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define MK_SIMPLE_MAIN()                        \
    int main(int argc, char** argv)             \
    {                                           \
        ::testing::InitGoogleMock(&argc, argv); \
        return RUN_ALL_TESTS();                 \
    }

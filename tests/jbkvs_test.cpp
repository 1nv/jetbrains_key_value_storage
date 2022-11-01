#include <gtest/gtest.h>
#include <jbkvs/jbkvs.h>

TEST(JBKVSTest, TestDummy)
{
    jbkvs::Storage storage(123);
    EXPECT_EQ(storage.getXxx(), 123);
    EXPECT_EQ(storage.test(), 100500);
    EXPECT_EQ(storage.getXxx(), 456);
}

#include <gtest/gtest.h>

#include <jbkvs/types/blob.h>

TEST(BlobTest, BlobWithCopiedDataCanBeCreated)
{
    uint8_t blobData[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    jbkvs::types::BlobPtr blob = jbkvs::types::Blob::create(blobData, std::size(blobData));
    memset(blobData, 0, sizeof(blobData));

    const uint8_t* data = blob->data();
    for (uint8_t i = 0; i < 8; ++i)
    {
        ASSERT_EQ(data[i], i);
    }
}

TEST(BlobTest, BlobWithMovedDataCanBeCreated)
{
    std::unique_ptr<uint8_t[]> blobData = std::unique_ptr<uint8_t[]>(new uint8_t[8]);
    for (uint8_t i = 0; i < 8; ++i)
    {
        blobData[i] = i;
    }

    jbkvs::types::BlobPtr blob = jbkvs::types::Blob::create(std::move(blobData), 8);

    ASSERT_EQ(blobData.get(), nullptr);

    const uint8_t* data = blob->data();
    for (uint8_t i = 0; i < 8; ++i)
    {
        ASSERT_EQ(data[i], i);
    }
}

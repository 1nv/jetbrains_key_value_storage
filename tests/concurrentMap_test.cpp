#include <gtest/gtest.h>

#include <string>
#include <future>

#include <jbkvs/detail/concurrentMap.h>

using namespace std::literals::string_literals;

TEST(ConcurrentMapTest, PutChangesState)
{
    jbkvs::detail::ConcurrentMap<uint32_t, std::string> map;

    uint32_t key = 123u;

    EXPECT_EQ(!!map.get(key), false);
    EXPECT_EQ(map.size(), 0);

    map.put(key, "123");

    EXPECT_EQ(!!map.get(key), true);
    EXPECT_EQ(map.size(), 1);
}

TEST(ConcurrentMapTest, GetAfterPutReturnsSameData)
{
    jbkvs::detail::ConcurrentMap<uint32_t, std::string> map;

    const uint32_t keyStr = 123u;
    const uint32_t keyRaw = 456u;
    std::string dataStr = "some_string"s;
    const char* dataRaw = "other_string";

    map.put(keyStr, dataStr);
    map.put(keyRaw, dataRaw);

    auto gotStr = map.get(keyStr);
    auto gotRaw = map.get(keyRaw);

    ASSERT_EQ(!!gotStr, true);
    ASSERT_EQ(!!gotRaw, true);

    EXPECT_EQ(*gotStr, dataStr);
    EXPECT_EQ(*gotRaw, std::string(dataRaw));
}

TEST(ConcurrentMapTest, ConcurrentPutWorksWithSeparateKeys)
{
    jbkvs::detail::ConcurrentMap<uint32_t, std::string> map;

    const size_t itemsToBeWrittenByOneThread = 1000;
    const std::string data = "dummy"s;

    std::promise<void> barrierPromise;
    std::shared_future<void> barrierFuture = barrierPromise.get_future().share();

    std::thread threads[4];
    for (size_t threadIndex = 0; threadIndex < std::size(threads); ++threadIndex)
    {
        threads[threadIndex] = std::thread([&map, itemsToBeWrittenByOneThread, &data, threadIndex, barrierFuture]()
        {
            barrierFuture.wait();

            for (uint32_t i = threadIndex * itemsToBeWrittenByOneThread; i < (threadIndex + 1) * itemsToBeWrittenByOneThread; ++i)
            {
                map.put(i, data);
            }
        });
    }

    barrierPromise.set_value();

    for (size_t threadIndex = 0; threadIndex < std::size(threads); ++threadIndex)
    {
        threads[threadIndex].join();
    }

    size_t expectedSize = itemsToBeWrittenByOneThread * std::size(threads);

    EXPECT_EQ(map.size(), expectedSize);

    for (uint32_t key = 0; key < (uint32_t)expectedSize; ++key)
    {
        auto got = map.get(key);
        EXPECT_EQ(!!got, true);
        EXPECT_EQ(*got, data);
    }
}

TEST(ConcurrentMapTest, ConcurrentPutWorksWithCollidingKeys)
{
    jbkvs::detail::ConcurrentMap<uint32_t, std::string> map;

    const size_t itemsToBeWrittenByOneThread = 1000;
    const std::string data = "dummy"s;

    std::promise<void> barrierPromise;
    std::shared_future<void> barrierFuture = barrierPromise.get_future().share();

    std::thread threads[4];
    for (size_t threadIndex = 0; threadIndex < std::size(threads); ++threadIndex)
    {
        threads[threadIndex] = std::thread([&map, itemsToBeWrittenByOneThread, &data, barrierFuture]()
        {
            barrierFuture.wait();

            for (uint32_t i = 0; i < itemsToBeWrittenByOneThread; ++i)
            {
                map.put(i, data);
            }
        });
    }

    barrierPromise.set_value();

    for (size_t threadIndex = 0; threadIndex < std::size(threads); ++threadIndex)
    {
        threads[threadIndex].join();
    }

    size_t expectedSize = itemsToBeWrittenByOneThread;

    EXPECT_EQ(map.size(), expectedSize);

    for (uint32_t key = 0; key < (uint32_t)expectedSize; ++key)
    {
        auto got = map.get(key);
        EXPECT_EQ(!!got, true);
        EXPECT_EQ(*got, data);
    }
}

TEST(ConcurrentMapTest, RemoveWorks)
{
    jbkvs::detail::ConcurrentMap<uint32_t, std::string> map;

    map.put(123u, "data1"s);
    map.put(456u, "data2"s);

    map.remove(123u);

    EXPECT_EQ(!!map.get(123u), false);
    EXPECT_EQ(!!map.get(456u), true);
    EXPECT_EQ(map.size(), 1);
}

TEST(ConcurrentMapTest, ClearWorks)
{
    jbkvs::detail::ConcurrentMap<uint32_t, std::string> map;

    map.put(123u, "data1"s);
    map.put(456u, "data2"s);

    map.clear();

    EXPECT_EQ(!!map.get(123u), false);
    EXPECT_EQ(!!map.get(456u), false);
    EXPECT_EQ(map.size(), 0);
}

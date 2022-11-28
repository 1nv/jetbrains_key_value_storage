#include <gtest/gtest.h>
#include "testUtils.h"

#include <thread>

#include <jbkvs/node.h>
#include <jbkvs/storage.h>

using namespace std::literals::string_literals;

TEST(NodeTest, EmptyNodeCanBeCreated)
{
    jbkvs::NodePtr node = jbkvs::Node::create();
}

TEST(NodeTest, AttachingTwoChildrenWithSameNameFails)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(root, "child");
    jbkvs::NodePtr child2 = jbkvs::Node::create(root, "child");

    ASSERT_EQ(!!child2, false);
    jbkvs::NodePtr child = root->getChild("child");
    ASSERT_EQ(child, child1);
}

TEST(NodeTest, SimpleNodeHierarchyCanBeCreated)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr childA = jbkvs::Node::create(root, "A");
    jbkvs::NodePtr childB = jbkvs::Node::create(root, "B");
    jbkvs::NodePtr subChildA1 = jbkvs::Node::create(childA, "1");

    EXPECT_EQ(root->getChild("A"), childA);
    EXPECT_EQ(root->getChild("B"), childB);
    EXPECT_EQ(childA->getChild("1"), subChildA1);
}

TEST(NodeTest, GetChildReturnsOnlyExistingChildren)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr childA = jbkvs::Node::create(root, "A");
    jbkvs::NodePtr childB = jbkvs::Node::create(root, "B");

    EXPECT_EQ(root->getChild("A"), childA);
    EXPECT_EQ(root->getChild("B"), childB);
    EXPECT_EQ(root->getChild("C"), jbkvs::NodePtr());
}

TEST(NodeTest, DetachRemovesNodeFromParent)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "A");
    bool detached = child->detach();

    ASSERT_EQ(detached, true);
    EXPECT_EQ(root->getChild("A"), jbkvs::NodePtr());
}

TEST(NodeTest, DetachOnRootShouldNotWork)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    bool detached = root->detach();

    ASSERT_EQ(detached, false);
}

TEST(NodeTest, GetPutRemoveSequenceWorks)
{
    jbkvs::NodePtr node = jbkvs::Node::create(jbkvs::NodePtr(), std::string());

    const uint32_t key = 123u;

    EXPECT_EQ(node->get<uint32_t>(key).has_value(), false);

    node->put(key, 456u);

    auto got = node->get<uint32_t>(key);
    ASSERT_EQ(!!got, true);
    EXPECT_EQ(*got, 456u);

    auto gotStr = node->get<std::string>(key);
    EXPECT_EQ(!!gotStr, false);

    node->put(key, "string"s);
    gotStr = node->get<std::string>(key);
    ASSERT_EQ(!!gotStr, true);
    EXPECT_EQ(*gotStr, "string"s);

    node->remove(key);
    gotStr = node->get<std::string>(key);
    ASSERT_EQ(!!gotStr, false);
}

TEST(NodeTest, MultipleValueTypesAreSupported)
{
    jbkvs::NodePtr node = jbkvs::Node::create();

    uint32_t u32 = 0xFFEEDDCC;
    uint64_t u64 = 0xFFEEDDCCBBAA9988;
    float f = 0.125f;
    double d = 0.00048828125;
    std::string s = "some long long long string"s;

    uint8_t blobData[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    jbkvs::types::BlobPtr blob = jbkvs::types::Blob::create(blobData, std::size(blobData));
    memset(blobData, 0, sizeof(blobData));

    node->put(1u, u32);
    node->put(2u, u64);
    node->put(3u, f);
    node->put(4u, d);
    node->put(5u, s);
    node->put(6u, blob);

    auto u32r = node->get<uint32_t>(1u);
    ASSERT_EQ(!!u32r, true);
    EXPECT_EQ(*u32r, u32);

    auto u64r = node->get<uint64_t>(2u);
    ASSERT_EQ(!!u64r, true);
    EXPECT_EQ(*u64r, u64);

    auto fr = node->get<float>(3u);
    ASSERT_EQ(!!fr, true);
    EXPECT_EQ(*fr, f);

    auto dr = node->get<double>(4u);
    ASSERT_EQ(!!dr, true);
    EXPECT_EQ(*dr, d);

    auto sr = node->get<std::string>(5u);
    ASSERT_EQ(!!sr, true);
    EXPECT_EQ(*sr, s);

    auto br = node->get<jbkvs::types::BlobPtr>(6u);
    ASSERT_EQ(!!br, true);
    EXPECT_EQ(*br, blob);
    for (uint8_t i = 0; i < 8; ++i)
    {
        EXPECT_EQ((*br)->data()[i], i);
    }
}

TEST(NodeTest, CreationOfMountedNodeChildShoudBeAllowed)
{
    jbkvs::NodePtr node = jbkvs::Node::create();

    jbkvs::Storage storage;
    storage.mount("/", node);

    jbkvs::NodePtr child = jbkvs::Node::create(node, "test");
    EXPECT_EQ(!!child, true);
}

TEST(NodeTest, DetachOfMountedNodeShoudBeAllowed)
{
    jbkvs::NodePtr node = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(node, "test");

    jbkvs::Storage storage;
    storage.mount("/", node);

    bool detached = child->detach();
    EXPECT_EQ(detached, true);
}

TEST(NodeTest, GetChildrenWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(root, "1");
    jbkvs::NodePtr child2 = jbkvs::Node::create(root, "2");
    jbkvs::NodePtr child3 = jbkvs::Node::create(root, "3");

    auto children = root->getChildren();
    jbkvs::NodePtr c1 = children.get("1");
    jbkvs::NodePtr c2 = children.get("2");
    jbkvs::NodePtr c3 = children.get("3");

    ASSERT_EQ(c1, child1);
    ASSERT_EQ(c2, child2);
    ASSERT_EQ(c3, child3);
}

static std::string _bigString = std::string(1024, 'a');

static void _createLargeVolumeChildren(const jbkvs::NodePtr& node, size_t depth, size_t count, size_t maxDepth)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (depth < maxDepth)
        {
            jbkvs::NodePtr child = jbkvs::Node::create(node, std::to_string(i));
            _createLargeVolumeChildren(child, depth + 1, count, maxDepth);
        }
        else
        {
            node->put(123u, _bigString);
        }
    }
}

static void _verifyLargeVolumeChildren(const jbkvs::NodePtr& node, size_t depth, size_t count, size_t maxDepth)
{
    auto children = node->getChildren();

    if (depth == maxDepth)
    {
        ASSERT_EQ(children.size(), 0);
        auto s = node->get<std::string>(123u);
        ASSERT_EQ(!!s, true);
        ASSERT_EQ(*s, _bigString);
    }
    else
    {
        ASSERT_EQ(children.size(), count);
        for (const auto& [childName, child] : children)
        {
            _verifyLargeVolumeChildren(child, depth + 1, count, maxDepth);
        }
    }
}

TEST(NodeTest, LargeVolumesAreSupported)
{
    jbkvs::NodePtr root = jbkvs::Node::create();

    _createLargeVolumeChildren(root, 0, 10, 6);
    _verifyLargeVolumeChildren(root, 0, 10, 6);
}

TEST(NodeTest, ConcurrentDetachWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "test");

    std::thread detachThreads[4];
    SimpleLatch latch(std::size(detachThreads));

    std::atomic<uint32_t> successfulDetachCounter(0);

    for (size_t i = 0; i < std::size(detachThreads); ++i)
    {
        detachThreads[i] = std::thread([&latch, &child, &successfulDetachCounter]()
        {
            latch.arrive_and_wait();

            bool detached = child->detach();
            if (detached)
            {
                ++successfulDetachCounter;
            }
        });
    }

    for (size_t i = 0; i < std::size(detachThreads); ++i)
    {
        detachThreads[i].join();
    }

    ASSERT_EQ(successfulDetachCounter.load(), 1u);
}

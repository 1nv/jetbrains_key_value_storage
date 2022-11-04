#include <gtest/gtest.h>
#include <jbkvs/node.h>

using namespace std::literals::string_literals;

TEST(NodeTest, EmptyNodeCanBeCreated)
{
    jbkvs::NodePtr node = jbkvs::Node::create();
}

TEST(NodeTest, SimpleNodeHierarchyCanBeCreated)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr childA = jbkvs::Node::create(root, "A"s);
    jbkvs::NodePtr childB = jbkvs::Node::create(root, "B"s);
    jbkvs::NodePtr subChildA1 = jbkvs::Node::create(childA, "1"s);

    EXPECT_EQ(root->getChild("A"s), childA);
    EXPECT_EQ(root->getChild("B"s), childB);
    EXPECT_EQ(childA->getChild("1"s), subChildA1);
}

TEST(NodeTest, GetChildReturnsOnlyExistingChildren)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr childA = jbkvs::Node::create(root, "A"s);
    jbkvs::NodePtr childB = jbkvs::Node::create(root, "B"s);

    EXPECT_EQ(root->getChild("A"s), childA);
    EXPECT_EQ(root->getChild("B"s), childB);
    EXPECT_EQ(root->getChild("C"s), jbkvs::NodePtr());
}

TEST(NodeTest, DetachRemovesNodeFromParent)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "A"s);
    child->detach();

    EXPECT_EQ(root->getChild("A"s), jbkvs::NodePtr());
}

TEST(NodeTest, DetachOnRootWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    root->detach();
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

    node->put(1u, u32);
    node->put(2u, u64);
    node->put(3u, f);
    node->put(4u, d);
    node->put(5u, s);

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
}

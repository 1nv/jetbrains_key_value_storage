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

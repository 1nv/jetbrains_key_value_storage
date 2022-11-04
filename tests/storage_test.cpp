#include <gtest/gtest.h>
#include <jbkvs/storage.h>

using namespace std::literals::string_literals;

TEST(StorageTest, MountWithNonSeparatorStartedPathFails)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();

    jbkvs::Storage storage;
    bool mounted;

    mounted = storage.mount(""s, volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("foo"s, volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount(" /"s, volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("foo/"s, volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("foo/bar"s, volumeRoot);
    EXPECT_EQ(mounted, false);
}

TEST(StorageTest, MountWithNullNodeFails)
{
    jbkvs::Storage storage;
    bool mounted;

    mounted = storage.mount("/"s, jbkvs::NodePtr());
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("/foo"s, jbkvs::NodePtr());
    EXPECT_EQ(mounted, false);
}

TEST(StorageTest, RootMountWorks)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();
    volumeRoot->put(123u, "data"s);

    jbkvs::Storage storage;
    bool mounted = storage.mount("/"s, volumeRoot);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageRoot = storage.getNode("/"s);
    ASSERT_EQ(!!storageRoot, true);

    auto data = storageRoot->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);
}

TEST(StorageTest, UnmountWithInvalidNodeFails)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();

    jbkvs::Storage storage;
    storage.mount("/"s, volumeRoot);

    bool unmounted;

    unmounted = storage.unmount("/"s, jbkvs::NodePtr());
    EXPECT_EQ(unmounted, false);

    jbkvs::NodePtr otherNode = jbkvs::Node::create();
    unmounted = storage.unmount("/"s, otherNode);
    EXPECT_EQ(unmounted, false);
}

TEST(StorageTest, RootUnmountWorks)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();
    volumeRoot->put(123u, "data"s);

    jbkvs::Storage storage;
    storage.mount("/"s, volumeRoot);
    bool unmounted = storage.unmount("/"s, volumeRoot);
    ASSERT_EQ(unmounted, true);

    jbkvs::StorageNodePtr storageRoot = storage.getNode("/"s);
    ASSERT_EQ(!!storageRoot, true);

    auto data = storageRoot->get<std::string>(123u);

    ASSERT_EQ(!!data, false);
}

TEST(StorageTest, UnmountWithChildOfMountedNodeFails)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();
    jbkvs::NodePtr childA = jbkvs::Node::create(volumeRoot, "foo"s);

    jbkvs::Storage storage;
    storage.mount("/"s, volumeRoot);

    bool unmounted = storage.unmount("/foo"s, childA);
    ASSERT_EQ(unmounted, false);
}

TEST(StorageTest, LastUnmountDestroysHierarchy)
{
    jbkvs::NodePtr volume1Root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(volume1Root, "foo"s);
    jbkvs::NodePtr child11 = jbkvs::Node::create(child1, "bar"s);
    jbkvs::NodePtr child111 = jbkvs::Node::create(child11, "baz"s);

    jbkvs::NodePtr volume2Root = jbkvs::Node::create();
    jbkvs::NodePtr child2 = jbkvs::Node::create(volume2Root, "bar"s);
    jbkvs::NodePtr child22 = jbkvs::Node::create(child2, "baz"s);

    jbkvs::Storage storage;
    storage.mount("/virtual/path"s, volume1Root);
    storage.mount("/virtual/path/foo"s, volume2Root);

    storage.unmount("/virtual/path"s, volume1Root);

    EXPECT_EQ(!!storage.getNode("/virtual/path/foo/bar/baz"s), true);

    storage.unmount("/virtual/path/foo"s, volume2Root);

    EXPECT_EQ(!!storage.getNode("/virtual"s), false);
}

TEST(StorageTest, MixOfMountedAndVirtualNodesWorks)
{
    jbkvs::NodePtr volume1Root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(volume1Root, "foo"s);
    jbkvs::NodePtr child11 = jbkvs::Node::create(child1, "bar"s);
    jbkvs::NodePtr child111 = jbkvs::Node::create(child11, "baz"s);

    jbkvs::NodePtr volume2Root = jbkvs::Node::create();

    jbkvs::Storage storage1;
    storage1.mount("/"s, volume1Root);
    storage1.mount("/foo/bar/baz"s, volume2Root);
    storage1.unmount("/"s, volume1Root);

    EXPECT_EQ(!!storage1.getNode("/foo/bar/baz"s), true);

    storage1.unmount("/foo/bar/baz"s, volume2Root);

    EXPECT_EQ(!!storage1.getNode("/foo"s), false);

    jbkvs::Storage storage2;
    storage2.mount("/foo/bar/baz"s, volume2Root);
    storage2.mount("/"s, volume1Root);
    storage2.unmount("/"s, volume1Root);

    EXPECT_EQ(!!storage2.getNode("/foo/bar/baz"s), true);

    storage2.unmount("/foo/bar/baz"s, volume2Root);

    EXPECT_EQ(!!storage2.getNode("/foo"s), false);
}

TEST(StorageTest, MountingAndUnmountingSameNodeWithDifferentRootPathsWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr multiMountNode = jbkvs::Node::create(root, "test"s);
    multiMountNode->put(123u, "data"s);

    jbkvs::Storage storage;
    storage.mount("/"s, root);
    storage.mount("/test"s, multiMountNode);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/test"s);
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<std::string>(123u);

    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);

    storage.unmount("/test"s, multiMountNode);

    data = storageNode->get<std::string>(123u);

    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);
}

#include <gtest/gtest.h>
#include "testUtils.h"

#include <thread>

#include <jbkvs/storage.h>

using namespace std::literals::string_literals;

TEST(StorageTest, MountWithNonSeparatorStartedPathFails)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();

    jbkvs::Storage storage;
    bool mounted;

    mounted = storage.mount("", volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("foo", volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount(" /", volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("foo/", volumeRoot);
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("foo/bar", volumeRoot);
    EXPECT_EQ(mounted, false);
}

TEST(StorageTest, MountWithNullNodeFails)
{
    jbkvs::Storage storage;
    bool mounted;

    mounted = storage.mount("/", jbkvs::NodePtr());
    EXPECT_EQ(mounted, false);

    mounted = storage.mount("/foo", jbkvs::NodePtr());
    EXPECT_EQ(mounted, false);
}

TEST(StorageTest, RootMountWorks)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();
    volumeRoot->put(123u, "data"s);

    jbkvs::Storage storage;
    bool mounted = storage.mount("/", volumeRoot);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageRoot = storage.getNode("/");
    ASSERT_EQ(!!storageRoot, true);

    auto data = storageRoot->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);
}

TEST(StorageTest, UnmountWithInvalidNodeFails)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();

    jbkvs::Storage storage;
    storage.mount("/", volumeRoot);

    bool unmounted;

    unmounted = storage.unmount("/", jbkvs::NodePtr());
    EXPECT_EQ(unmounted, false);

    jbkvs::NodePtr otherNode = jbkvs::Node::create();
    unmounted = storage.unmount("/", otherNode);
    EXPECT_EQ(unmounted, false);
}

TEST(StorageTest, RootUnmountWorks)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();
    volumeRoot->put(123u, "data"s);

    jbkvs::Storage storage;
    storage.mount("/", volumeRoot);
    bool unmounted = storage.unmount("/", volumeRoot);
    ASSERT_EQ(unmounted, true);

    jbkvs::StorageNodePtr storageRoot = storage.getNode("/");
    ASSERT_EQ(!!storageRoot, true);

    auto data = storageRoot->get<std::string>(123u);

    ASSERT_EQ(!!data, false);
}

TEST(StorageTest, UnmountWithChildOfMountedNodeFails)
{
    jbkvs::NodePtr volumeRoot = jbkvs::Node::create();
    jbkvs::NodePtr childA = jbkvs::Node::create(volumeRoot, "foo");

    jbkvs::Storage storage;
    storage.mount("/", volumeRoot);

    bool unmounted = storage.unmount("/foo", childA);
    ASSERT_EQ(unmounted, false);
}

TEST(StorageTest, LastUnmountDestroysHierarchy)
{
    jbkvs::NodePtr volume1Root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(volume1Root, "foo");
    jbkvs::NodePtr child11 = jbkvs::Node::create(child1, "bar");
    jbkvs::NodePtr child111 = jbkvs::Node::create(child11, "baz");

    jbkvs::NodePtr volume2Root = jbkvs::Node::create();
    jbkvs::NodePtr child2 = jbkvs::Node::create(volume2Root, "bar");
    jbkvs::NodePtr child22 = jbkvs::Node::create(child2, "baz");

    jbkvs::Storage storage;
    storage.mount("/virtual/path", volume1Root);
    storage.mount("/virtual/path/foo", volume2Root);

    storage.unmount("/virtual/path", volume1Root);

    EXPECT_EQ(!!storage.getNode("/virtual/path/foo/bar/baz"), true);

    storage.unmount("/virtual/path/foo", volume2Root);

    EXPECT_EQ(!!storage.getNode("/virtual"), false);
}

TEST(StorageTest, MixOfMountedAndVirtualNodesWorks)
{
    jbkvs::NodePtr volume1Root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(volume1Root, "foo");
    jbkvs::NodePtr child11 = jbkvs::Node::create(child1, "bar");
    jbkvs::NodePtr child111 = jbkvs::Node::create(child11, "baz");

    jbkvs::NodePtr volume2Root = jbkvs::Node::create();

    jbkvs::Storage storage1;
    storage1.mount("/", volume1Root);
    storage1.mount("/foo/bar/baz", volume2Root);
    storage1.unmount("/", volume1Root);

    EXPECT_EQ(!!storage1.getNode("/foo/bar/baz"), true);

    storage1.unmount("/foo/bar/baz", volume2Root);

    EXPECT_EQ(!!storage1.getNode("/foo"), false);

    jbkvs::Storage storage2;
    storage2.mount("/foo/bar/baz", volume2Root);
    storage2.mount("/", volume1Root);
    storage2.unmount("/", volume1Root);

    EXPECT_EQ(!!storage2.getNode("/foo/bar/baz"), true);

    storage2.unmount("/foo/bar/baz", volume2Root);

    EXPECT_EQ(!!storage2.getNode("/foo"), false);
}

TEST(StorageTest, MountingAndUnmountingSameNodeWithDifferentRootPathsWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr multiMountNode = jbkvs::Node::create(root, "test");
    multiMountNode->put(123u, "data"s);

    jbkvs::Storage storage;
    storage.mount("/", root);
    storage.mount("/test", multiMountNode);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/test");
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<std::string>(123u);

    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);

    storage.unmount("/test", multiMountNode);

    data = storageNode->get<std::string>(123u);

    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);
}

TEST(StorageTest, MountingAndUnmountingSameNodeTowardsDifferentPathsWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "foo");
    child->put(123u, "data"s);

    jbkvs::Storage storage;
    storage.mount("/virtual/path", root);
    bool mounted = storage.mount("/virtual/another/path", root);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageNode1 = storage.getNode("/virtual/path/foo");
    ASSERT_EQ(!!storageNode1, true);

    auto data = storageNode1->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);

    jbkvs::StorageNodePtr storageNode2 = storage.getNode("/virtual/another/path/foo");
    ASSERT_EQ(!!storageNode2, true);

    data = storageNode2->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);

    bool unmounted = storage.unmount("/virtual/path", root);
    ASSERT_EQ(unmounted, true);

    jbkvs::StorageNodePtr storageNode3 = storage.getNode("/virtual/another/path/foo");
    ASSERT_EQ(!!storageNode3, true);

    data = storageNode3->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);
}

TEST(StorageTest, MountingAndUnmountingSameNodeTowardsDifferentStoragesWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "foo");
    child->put(123u, "data"s);

    jbkvs::Storage storage1, storage2;
    storage1.mount("/virtual/path", root);
    bool mounted = storage2.mount("/", root);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageNode1 = storage1.getNode("/virtual/path/foo");
    ASSERT_EQ(!!storageNode1, true);

    auto data = storageNode1->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);

    jbkvs::StorageNodePtr storageNode2 = storage2.getNode("/foo");
    ASSERT_EQ(!!storageNode2, true);

    data = storageNode2->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);

    bool unmounted = storage1.unmount("/virtual/path", root);
    ASSERT_EQ(unmounted, true);

    jbkvs::StorageNodePtr storageNode3 = storage2.getNode("/foo");
    ASSERT_EQ(!!storageNode3, true);

    data = storageNode3->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data"s);
}

TEST(StorageTest, DataFromMultipleNodesIsMerged)
{
    jbkvs::NodePtr root1 = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(root1, "foo");
    child1->put(123u, "data1"s);

    jbkvs::NodePtr root2 = jbkvs::Node::create();
    jbkvs::NodePtr child2 = jbkvs::Node::create(root2, "foo");
    child2->put(456u, "data2"s);

    jbkvs::NodePtr root3 = jbkvs::Node::create();
    child2->put(789u, "data3"s);


    jbkvs::Storage storage;
    bool mounted;
    mounted = storage.mount("/", root1);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/", root2);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/foo", root3);
    ASSERT_EQ(mounted, true);

    root3.reset();
    root2.reset();
    root1.reset();

    jbkvs::StorageNodePtr storageNode = storage.getNode("/foo");
    ASSERT_EQ(!!storageNode, true);

    auto data1 = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data1, true);
    EXPECT_EQ(*data1, "data1"s);
    auto data2 = storageNode->get<std::string>(456u);
    ASSERT_EQ(!!data2, true);
    EXPECT_EQ(*data2, "data2"s);
    auto data3 = storageNode->get<std::string>(789u);
    ASSERT_EQ(!!data3, true);
    EXPECT_EQ(*data3, "data3"s);
}

TEST(StorageTest, CollidingDataWithTheSameTypeFromMultipleNodesIsRetrievedFromTheNewerNode)
{
    jbkvs::NodePtr root1= jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(root1, "foo");
    child1->put(123u, "data1"s);

    jbkvs::NodePtr root2 = jbkvs::Node::create();
    jbkvs::NodePtr child2 = jbkvs::Node::create(root2, "foo");
    child2->put(123u, "data2"s);

    jbkvs::NodePtr root3 = jbkvs::Node::create();
    root3->put(123u, "data3"s);


    jbkvs::Storage storage;
    bool mounted;
    mounted = storage.mount("/", root1);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/", root2);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/foo", root3);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/foo");
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data3"s);

    bool unmounted = storage.unmount("/foo", root3);
    ASSERT_EQ(unmounted, true);

    data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data2"s);
}

TEST(StorageTest, CollidingDataWithDifferentTypesFromMultipleNodesIsMerged)
{
    jbkvs::NodePtr root1 = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(root1, "foo");
    child1->put(123u, "data1"s);

    jbkvs::NodePtr root2 = jbkvs::Node::create();
    jbkvs::NodePtr child2 = jbkvs::Node::create(root2, "foo");
    child2->put(123u, 2u);

    jbkvs::NodePtr root3 = jbkvs::Node::create();
    root3->put(123u, 3.0f);


    jbkvs::Storage storage;
    bool mounted;
    mounted = storage.mount("/", root1);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/", root2);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/foo", root3);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/foo");
    ASSERT_EQ(!!storageNode, true);

    auto data1 = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data1, true);
    EXPECT_EQ(*data1, "data1"s);

    auto data2 = storageNode->get<uint32_t>(123u);
    ASSERT_EQ(!!data2, true);
    EXPECT_EQ(*data2, 2u);

    auto data3 = storageNode->get<float>(123u);
    ASSERT_EQ(!!data3, true);
    EXPECT_EQ(*data3, 3.0f);
}

TEST(StorageTest, DoubleMountingAndUnmountingSameNodePrioritizesLastMountedNode)
{
    jbkvs::NodePtr root1 = jbkvs::Node::create();
    root1->put(123u, "data1"s);

    jbkvs::NodePtr root2 = jbkvs::Node::create();
    root2->put(123u, "data2"s);

    jbkvs::Storage storage;
    bool mounted;
    mounted = storage.mount("/", root1);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/", root2);
    ASSERT_EQ(mounted, true);
    mounted = storage.mount("/", root1);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/");
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data1"s);

    bool unmounted;

    unmounted = storage.unmount("/", root1);
    ASSERT_EQ(unmounted, true);

    data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data2"s);

    unmounted = storage.unmount("/", root2);
    ASSERT_EQ(unmounted, true);

    data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data1"s);

    unmounted = storage.unmount("/", root1);
    ASSERT_EQ(unmounted, true);
}

TEST(StorageTest, PathCanEndWithSeparator)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    root->put(123u, "data1"s);

    jbkvs::Storage storage;
    bool mounted;
    mounted = storage.mount("/path/", root);
    ASSERT_EQ(mounted, true);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/path/");
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data1"s);

    storageNode = storage.getNode("/path");
    ASSERT_EQ(!!storageNode, true);

    data = storageNode->get<std::string>(123u);
    ASSERT_EQ(!!data, true);
    EXPECT_EQ(*data, "data1"s);
}

TEST(StorageTest, CreationOfMountedNodeChildAddsStorageNodeChild)
{
    jbkvs::NodePtr node = jbkvs::Node::create();

    jbkvs::Storage storage;
    storage.mount("/", node);

    jbkvs::NodePtr child = jbkvs::Node::create(node, "test");
    ASSERT_EQ(!!child, true);
    child->put(123u, 1u);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/test");
    ASSERT_EQ(!!storageNode, true);
    auto data = storageNode->get<uint32_t>(123u);
    ASSERT_EQ(!!data, true);
    ASSERT_EQ(*data, 1u);
}

TEST(StorageTest, DetachOfMountedNodeRemovesStorageNodeChild)
{
    jbkvs::NodePtr node = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(node, "test");

    jbkvs::Storage storage;
    storage.mount("/", node);

    bool detached = child->detach();
    ASSERT_EQ(detached, true);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/test");
    ASSERT_EQ(!!storageNode, false);
}

TEST(StorageTest, CreationOfMountedNodeChildKeepsMergePriority)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr separate = jbkvs::Node::create();
    separate->put(123u, "separate"s);

    jbkvs::Storage storageWithRootPriority;
    storageWithRootPriority.mount("/foo", separate);
    storageWithRootPriority.mount("/", root);

    jbkvs::Storage storageWithSeparatePriority;
    storageWithSeparatePriority.mount("/", root);
    storageWithSeparatePriority.mount("/foo", separate);

    jbkvs::StorageNodePtr rootPriorityNode = storageWithRootPriority.getNode("/foo");
    jbkvs::StorageNodePtr separatePriorityNode = storageWithSeparatePriority.getNode("/foo");

    std::string data;
    data = rootPriorityNode->get<std::string>(123u).value();
    ASSERT_EQ(data, "separate"s);
    data = separatePriorityNode->get<std::string>(123u).value();
    ASSERT_EQ(data, "separate"s);

    jbkvs::NodePtr rootChild = jbkvs::Node::create(root, "foo");
    rootChild->put(123u, "rootChild"s);

    data = rootPriorityNode->get<std::string>(123u).value();
    ASSERT_EQ(data, "rootChild"s);
    data = separatePriorityNode->get<std::string>(123u).value();
    ASSERT_EQ(data, "separate"s);
}

TEST(StorageTest, DetachOfMountedNodeUpdatesAllAffectedStorageNodes)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "test");
    child->put(123u, 1u);

    jbkvs::Storage storage1, storage2;
    storage1.mount("/", root);
    storage1.mount("/test", root);
    storage2.mount("/", root);
    storage1.mount("/xxx", root);

    bool detached = child->detach();
    ASSERT_EQ(detached, true);

    jbkvs::StorageNodePtr storageNode;

    storageNode = storage1.getNode("/xxx/test");
    ASSERT_EQ(!!storageNode, false);

    storageNode = storage2.getNode("/test");
    ASSERT_EQ(!!storageNode, false);

    storageNode = storage1.getNode("/test/test");
    ASSERT_EQ(!!storageNode, false);

    storageNode = storage1.getNode("/test");
    ASSERT_EQ(!!storageNode, true);
    auto data = storageNode->get<uint32_t>(123u);
    ASSERT_EQ(!!data, false);
}

TEST(StorageTest, DetachAndRecreationOfMountedNodeWithSameNameWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child = jbkvs::Node::create(root, "test");
    child->put(123u, 1u);

    jbkvs::Storage storage;
    storage.mount("/", root);

    bool detached = child->detach();
    ASSERT_EQ(detached, true);

    child = jbkvs::Node::create(root, "test");
    child->put(123u, 2u);

    jbkvs::StorageNodePtr storageNode = storage.getNode("/test");
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<uint32_t>(123u);
    ASSERT_EQ(!!data, true);
    ASSERT_EQ(*data, 2u);
}

TEST(StorageTest, DetachOfMountedTreeRemovesEmptyStorageNodes)
{
    jbkvs::NodePtr root1 = jbkvs::Node::create();
    jbkvs::NodePtr child11 = jbkvs::Node::create(root1, "foo");
    jbkvs::NodePtr child12 = jbkvs::Node::create(child11, "bar");
    jbkvs::NodePtr child13 = jbkvs::Node::create(child12, "baz");
    child11->put(123u, 1u);

    jbkvs::NodePtr root2 = jbkvs::Node::create();
    jbkvs::NodePtr child21 = jbkvs::Node::create(root2, "foo");
    child21->put(123u, 2u);

    jbkvs::Storage storage;
    storage.mount("/root", root1);
    storage.mount("/root", root2);

    bool detached = child11->detach();
    ASSERT_EQ(detached, true);

    jbkvs::StorageNodePtr storageNode;

    storageNode = storage.getNode("/root/foo/bar");
    ASSERT_EQ(!!storageNode, false);

    storageNode = storage.getNode("/root/foo");
    ASSERT_EQ(!!storageNode, true);

    auto data = storageNode->get<uint32_t>(123u);
    ASSERT_EQ(!!data, true);
    ASSERT_EQ(*data, 2u);
}

TEST(StorageTest, ConcurrentOperationsWork)
{
    jbkvs::NodePtr root = jbkvs::Node::create();

    jbkvs::Storage storage;
    storage.mount("/", root);

    SimpleLatch latch(3);

    std::thread readerThread = std::thread([&latch, &storage]()
    {
        latch.arrive_and_wait();

        for (size_t i = 0; i < 1000000; ++i)
        {
            jbkvs::StorageNodePtr storageNode1 = storage.getNode("/test");
            jbkvs::StorageNodePtr storageNode2 = storage.getNode("/test/test");
            jbkvs::StorageNodePtr storageNode3 = storage.getNode("/test/test/test");
        }
    });

    std::thread mounterThread = std::thread([&latch, &root, &storage]()
    {
        latch.arrive_and_wait();

        for (size_t i = 0; i < 100; ++i)
        {
            storage.mount("/test", root);
            storage.mount("/test/test", root);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            storage.unmount("/test", root);
            storage.unmount("/test/test", root);
        }
    });

    std::thread attacherThread = std::thread([&latch, &root]()
    {
        latch.arrive_and_wait();

        for (size_t i = 0; i < 100; ++i)
        {
            jbkvs::NodePtr child = jbkvs::Node::create(root, "test");
            child->put(123u, 1u);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            child->detach();
        }
    });

    readerThread.join();
    mounterThread.join();
    attacherThread.join();

    jbkvs::StorageNodePtr storageNode = storage.getNode("/test");
    ASSERT_EQ(!!storageNode, false);
}

TEST(StorageTest, DestructionOfStorageWithMountedNodesWorks)
{
    jbkvs::NodePtr root = jbkvs::Node::create();
    jbkvs::NodePtr child1 = jbkvs::Node::create(root, "child1");
    jbkvs::NodePtr child2 = jbkvs::Node::create(root, "child2");
    jbkvs::NodePtr subChild1 = jbkvs::Node::create(child1, "subChild1");
    jbkvs::NodePtr subChild2 = jbkvs::Node::create(child2, "subChild2");

    {
        jbkvs::Storage storage;
        storage.mount("/", root);
        storage.mount("/test", subChild2);
    }

    bool detached;
    detached = child1->detach();
    ASSERT_EQ(detached, true);
    detached = child2->detach();
    ASSERT_EQ(detached, true);
}

TEST(StorageTest, GetMountPointsWorks)
{
    jbkvs::NodePtr root1 = jbkvs::Node::create();
    jbkvs::NodePtr root2 = jbkvs::Node::create();

    jbkvs::Storage storage;
    storage.mount("/", root1);
    storage.mount("/", root2);
    storage.mount("/", root1);
    storage.mount("/foo", root2);
    storage.mount("/bar", root1);
    storage.unmount("/foo", root2);

    auto mountPoints = storage.getMountPoints();
    ASSERT_EQ(mountPoints.size(), 4);

    EXPECT_EQ(mountPoints[0].path, "/"s);
    EXPECT_EQ(mountPoints[0].node, root1);
    EXPECT_EQ(mountPoints[1].path, "/"s);
    EXPECT_EQ(mountPoints[1].node, root2);
    EXPECT_EQ(mountPoints[2].path, "/"s);
    EXPECT_EQ(mountPoints[2].node, root1);
    EXPECT_EQ(mountPoints[3].path, "/bar"s);
    EXPECT_EQ(mountPoints[3].node, root1);
}

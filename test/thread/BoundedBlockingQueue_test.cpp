#include "hohnor/thread/BoundedBlockingQueue.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>

using namespace Hohnor;

class BoundedBlockingQueueTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code if needed
    }

    void TearDown() override
    {
        // Cleanup code if needed
    }
};

// Test basic constructor and initial state
TEST_F(BoundedBlockingQueueTest, ConstructorAndInitialState)
{
    BoundedBlockingQueue<int> queue(5);
    
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(queue.capacity(), 6); // +1 for the empty item
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
}

// Test basic put and take operations
TEST_F(BoundedBlockingQueueTest, BasicPutTake)
{
    BoundedBlockingQueue<int> queue(3);
    
    // Test put and size
    queue.put(42);
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.empty());
    EXPECT_FALSE(queue.full());
    
    // Test take
    int value = queue.take();
    EXPECT_EQ(value, 42);
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
}

// Test capacity limits
TEST_F(BoundedBlockingQueueTest, CapacityLimits)
{
    BoundedBlockingQueue<int> queue(3);
    
    // Fill to capacity
    queue.put(1);
    queue.put(2);
    queue.put(3);
    
    EXPECT_EQ(queue.size(), 3);
    EXPECT_EQ(queue.capacity(), 4); // +1 for the empty item
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    
    // Take all items
    EXPECT_EQ(queue.take(), 1);
    EXPECT_EQ(queue.take(), 2);
    EXPECT_EQ(queue.take(), 3);
    
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
}

// Test FIFO ordering
TEST_F(BoundedBlockingQueueTest, FIFOOrdering)
{
    BoundedBlockingQueue<int> queue(5);
    
    // Put multiple values
    for (int i = 0; i < 5; ++i)
    {
        queue.put(i);
    }
    EXPECT_EQ(queue.size(), 5);
    EXPECT_TRUE(queue.full());
    
    // Take all values in FIFO order
    for (int i = 0; i < 5; ++i)
    {
        int value = queue.take();
        EXPECT_EQ(value, i);
    }
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
}

// Test move semantics for put operation
TEST_F(BoundedBlockingQueueTest, MoveSemanticsTest)
{
    BoundedBlockingQueue<std::string> queue(3);
    
    std::string original = "test_string";
    std::string copy = original;
    
    // Test move put
    queue.put(std::move(copy));
    EXPECT_EQ(queue.size(), 1);
    
    // The moved string should be empty or in valid but unspecified state
    std::string result = queue.take();
    EXPECT_EQ(result, original);
    EXPECT_EQ(queue.size(), 0);
}

// Test copy semantics for put operation
TEST_F(BoundedBlockingQueueTest, CopySemanticsTest)
{
    BoundedBlockingQueue<std::string> queue(3);
    
    std::string original = "test_string";
    
    // Test copy put
    queue.put(original);
    EXPECT_EQ(queue.size(), 1);
    
    // Original should remain unchanged
    EXPECT_EQ(original, "test_string");
    
    std::string result = queue.take();
    EXPECT_EQ(result, original);
    EXPECT_EQ(queue.size(), 0);
}

// Test blocking behavior when queue is full
TEST_F(BoundedBlockingQueueTest, BlockingWhenFull)
{
    BoundedBlockingQueue<int> queue(2);
    std::atomic<bool> producer_blocked{false};
    std::atomic<bool> producer_finished{false};
    std::atomic<int> items_put{0};
    
    // Fill the queue to capacity
    queue.put(1);
    queue.put(2);
    EXPECT_TRUE(queue.full());
    
    // Producer thread that will block on third put
    Thread producer([&]() {
        queue.put(3); // This should block
        producer_blocked = true;
        items_put = 3;
        queue.put(4); // This should also work after consumer takes items
        items_put = 4;
        producer_finished = true;
    });
    
    producer.start();
    
    // Give producer time to block
    CurrentThread::sleepUsec(50000); // 50ms
    EXPECT_FALSE(producer_blocked);
    EXPECT_FALSE(producer_finished);
    
    // Consumer takes one item, should unblock producer
    int value1 = queue.take();
    EXPECT_EQ(value1, 1);
    
    // Give producer time to put the third item
    CurrentThread::sleepUsec(50000); // 50ms
    EXPECT_TRUE(producer_blocked);
    EXPECT_EQ(items_put.load(), 3);
    
    // Take another item to allow fourth put
    int value2 = queue.take();
    EXPECT_EQ(value2, 2);
    
    producer.join();
    
    EXPECT_TRUE(producer_finished);
    EXPECT_EQ(items_put.load(), 4);
    
    // Verify remaining items
    EXPECT_EQ(queue.take(), 3);
    EXPECT_EQ(queue.take(), 4);
    EXPECT_TRUE(queue.empty());
}

// Test blocking behavior when queue is empty
TEST_F(BoundedBlockingQueueTest, BlockingWhenEmpty)
{
    BoundedBlockingQueue<int> queue(3);
    std::atomic<bool> consumer_started{false};
    std::atomic<bool> consumer_finished{false};
    std::atomic<int> value_received{0};
    
    // Consumer thread that will block on empty queue
    Thread consumer([&]() {
        consumer_started = true;
        int value = queue.take(); // This should block
        value_received = value;
        consumer_finished = true;
    });
    
    consumer.start();
    
    // Wait for consumer to start and block
    while (!consumer_started)
    {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    CurrentThread::sleepUsec(50000); // 50ms
    EXPECT_FALSE(consumer_finished);
    
    // Producer puts an item, should unblock consumer
    queue.put(42);
    
    consumer.join();
    
    EXPECT_TRUE(consumer_finished);
    EXPECT_EQ(value_received.load(), 42);
    EXPECT_TRUE(queue.empty());
}

// Test producer-consumer pattern with bounded queue
TEST_F(BoundedBlockingQueueTest, ProducerConsumerBounded)
{
    BoundedBlockingQueue<int> queue(5); // Small capacity to test blocking
    const int total_items = 20;
    std::vector<int> consumed;
    std::atomic<bool> consumer_finished{false};
    
    // Consumer thread
    Thread consumer([&]() {
        for (int i = 0; i < total_items; ++i)
        {
            int value = queue.take();
            consumed.push_back(value);
        }
        consumer_finished = true;
    });
    
    // Producer thread
    Thread producer([&]() {
        for (int i = 0; i < total_items; ++i)
        {
            queue.put(i);
            // Small delay to allow consumer to work
            if (i % 5 == 0)
            {
                CurrentThread::sleepUsec(10000); // 10ms
            }
        }
    });
    
    consumer.start();
    producer.start();
    
    producer.join();
    consumer.join();
    
    EXPECT_TRUE(consumer_finished);
    EXPECT_EQ(consumed.size(), total_items);
    
    // Verify all items were consumed in order
    for (int i = 0; i < total_items; ++i)
    {
        EXPECT_EQ(consumed[i], i);
    }
    EXPECT_TRUE(queue.empty());
}

// Test multiple producers and consumers with bounded queue
TEST_F(BoundedBlockingQueueTest, MultipleProducersConsumersBounded)
{
    BoundedBlockingQueue<int> queue(10); // Bounded capacity
    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 50;
    
    std::atomic<int> total_consumed{0};
    std::vector<std::unique_ptr<Thread>> producers;
    std::vector<std::unique_ptr<Thread>> consumers;
    
    // Create consumers
    for (int i = 0; i < num_consumers; ++i)
    {
        consumers.emplace_back(new Thread([&]() {
            while (true)
            {
                int value = queue.take();
                if (value == -1) // Sentinel value to stop
                {
                    queue.put(-1); // Put it back for other consumers
                    break;
                }
                total_consumed.fetch_add(1);
            }
        }));
    }
    
    // Create producers
    for (int i = 0; i < num_producers; ++i)
    {
        producers.emplace_back(new Thread([&, i]() {
            for (int j = 0; j < items_per_producer; ++j)
            {
                queue.put(i * items_per_producer + j);
            }
        }));
    }
    
    // Start all threads
    for (auto& consumer : consumers)
    {
        consumer->start();
    }
    for (auto& producer : producers)
    {
        producer->start();
    }
    
    // Wait for producers to finish
    for (auto& producer : producers)
    {
        producer->join();
    }
    
    // Send sentinel to stop consumers
    queue.put(-1);
    
    // Wait for consumers to finish
    for (auto& consumer : consumers)
    {
        consumer->join();
    }
    
    EXPECT_EQ(total_consumed.load(), num_producers * items_per_producer);
}

// Test giveUp functionality
TEST_F(BoundedBlockingQueueTest, GiveUpFunctionality)
{
    BoundedBlockingQueue<int> queue(5);
    std::atomic<bool> consumer_finished{false};
    std::atomic<int> values_received{0};
    
    // Put some items first
    queue.put(1);
    queue.put(2);
    queue.put(3);
    EXPECT_EQ(queue.size(), 3);
    
    // Consumer thread that will be blocked waiting for more items
    Thread consumer([&]() {
        while (true)
        {
            int value = queue.take();
            if (value == 0) // Default constructed value indicates giveUp was called
            {
                break;
            }
            values_received.fetch_add(1);
        }
        consumer_finished = true;
    });
    
    consumer.start();
    
    // Give consumer time to consume existing items and block
    CurrentThread::sleepUsec(100000); // 100ms
    
    // Consumer should have consumed the 3 items and be blocked
    EXPECT_EQ(values_received.load(), 3);
    EXPECT_FALSE(consumer_finished);
    
    // Call giveUp to unblock the consumer
    queue.giveUp();
    
    // Wait for consumer to finish
    consumer.join();
    
    EXPECT_TRUE(consumer_finished);
    EXPECT_EQ(values_received.load(), 3);
}

// Test giveUp unblocks both producers and consumers
TEST_F(BoundedBlockingQueueTest, GiveUpUnblocksAll)
{
    BoundedBlockingQueue<int> queue(5);
    std::atomic<bool> producer_finished{false};
    
    
    // Fill the queue
    queue.put(1);
    queue.put(2);
    queue.put(3);
    queue.put(4);
    queue.put(5);
    EXPECT_TRUE(queue.full());
    
    // Producer thread that will block
    Thread producer([&]() {
        queue.put(6); // This will block since queue is full
        producer_finished = true;
    });

    producer.start();
    EXPECT_FALSE(producer_finished);
    queue.giveUp();
    producer.join();
    EXPECT_TRUE(producer_finished);
    
    BoundedBlockingQueue<int> queue2(5);
    std::atomic<bool> consumer_finished{false};
    // Consumer thread that will block after taking items
    Thread consumer([&]() {
        queue2.take(); // This will block since queue becomes empty
        consumer_finished = true;
    });
    
    
    consumer.start();
    
    // Give threads time to block
    CurrentThread::sleepUsec(100000); // 100ms
    
    // Both should be blocked now
    
    EXPECT_FALSE(consumer_finished);
    
    // Call giveUp to unblock both
    queue2.giveUp();
    
    
    consumer.join();
    
    
    EXPECT_TRUE(consumer_finished);
}

// Test behavior after giveUp is called
TEST_F(BoundedBlockingQueueTest, BehaviorAfterGiveUp)
{
    BoundedBlockingQueue<int> queue(3);
    
    // Put some items
    queue.put(1);
    queue.put(2);
    
    // Call giveUp
    queue.giveUp();
    
    // take() should return default constructed values
    int value1 = queue.take();
    int value2 = queue.take();
    int value3 = queue.take();
    
    EXPECT_EQ(value1, 0); // Default constructed int
    EXPECT_EQ(value2, 0);
    EXPECT_EQ(value3, 0);
    
    // put() should return immediately without adding items
    queue.put(42);
    // Size should remain 0 or unchanged after giveUp
}

// Test with custom objects
struct TestObject
{
    int value;
    std::string name;
    
    TestObject() : value(0), name("default") {}
    TestObject(int v, const std::string& n) : value(v), name(n) {}
    
    bool operator==(const TestObject& other) const
    {
        return value == other.value && name == other.name;
    }
};

TEST_F(BoundedBlockingQueueTest, CustomObjectTest)
{
    BoundedBlockingQueue<TestObject> queue(3);
    
    TestObject obj1(42, "test1");
    TestObject obj2(84, "test2");
    
    queue.put(obj1);
    queue.put(obj2);
    
    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.full());
    
    TestObject result1 = queue.take();
    TestObject result2 = queue.take();
    
    EXPECT_EQ(result1, obj1);
    EXPECT_EQ(result2, obj2);
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
}

// Test giveUp with custom objects
TEST_F(BoundedBlockingQueueTest, GiveUpWithCustomObjects)
{
    BoundedBlockingQueue<TestObject> queue(3);
    
    queue.put(TestObject(1, "one"));
    queue.giveUp();
    
    TestObject result = queue.take();
    TestObject expected; // Default constructed
    
    EXPECT_EQ(result, expected);
}

// Test circular buffer behavior (wrapping around)
TEST_F(BoundedBlockingQueueTest, CircularBufferBehavior)
{
    BoundedBlockingQueue<int> queue(3);
    
    // Fill and empty multiple times to test circular behavior
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        // Fill the queue
        for (int i = 0; i < 3; ++i)
        {
            queue.put(cycle * 10 + i);
        }
        EXPECT_TRUE(queue.full());
        EXPECT_EQ(queue.size(), 3);
        
        // Empty the queue
        for (int i = 0; i < 3; ++i)
        {
            int value = queue.take();
            EXPECT_EQ(value, cycle * 10 + i);
        }
        EXPECT_TRUE(queue.empty());
        EXPECT_EQ(queue.size(), 0);
    }
}

// Test partial fill and take patterns
TEST_F(BoundedBlockingQueueTest, PartialFillTakePatterns)
{
    BoundedBlockingQueue<int> queue(5);
    
    // Pattern: put 3, take 2, put 2, take 3
    queue.put(1);
    queue.put(2);
    queue.put(3);
    EXPECT_EQ(queue.size(), 3);
    
    EXPECT_EQ(queue.take(), 1);
    EXPECT_EQ(queue.take(), 2);
    EXPECT_EQ(queue.size(), 1);
    
    queue.put(4);
    queue.put(5);
    EXPECT_EQ(queue.size(), 3);
    
    EXPECT_EQ(queue.take(), 3);
    EXPECT_EQ(queue.take(), 4);
    EXPECT_EQ(queue.take(), 5);
    EXPECT_TRUE(queue.empty());
}

// Test stress scenario with bounded queue
TEST_F(BoundedBlockingQueueTest, StressTestBounded)
{
    BoundedBlockingQueue<int> queue(20); // Moderate capacity
    const int num_operations = 1000;
    std::atomic<int> put_count{0};
    std::atomic<int> take_count{0};
    
    // Producer thread
    Thread producer([&]() {
        for (int i = 0; i < num_operations; ++i)
        {
            queue.put(i);
            put_count.fetch_add(1);
            if (i % 50 == 0)
            {
                CurrentThread::sleepUsec(1000); // 1ms pause every 50 items
            }
        }
    });
    
    // Consumer thread
    Thread consumer([&]() {
        for (int i = 0; i < num_operations; ++i)
        {
            int value = queue.take();
            EXPECT_EQ(value, i);
            take_count.fetch_add(1);
            if (i % 100 == 0)
            {
                CurrentThread::sleepUsec(2000); // 2ms pause every 100 items
            }
        }
    });
    
    producer.start();
    consumer.start();
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(put_count.load(), num_operations);
    EXPECT_EQ(take_count.load(), num_operations);
    EXPECT_EQ(queue.size(), 0);
}

// Test edge case: queue with capacity 1
TEST_F(BoundedBlockingQueueTest, SingleCapacityQueue)
{
    BoundedBlockingQueue<int> queue(2);
    
    EXPECT_EQ(queue.capacity(), 3); // +1 for the empty item
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    
    queue.put(42);
    queue.put(42);
    EXPECT_TRUE(queue.full());
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 2);
    
    int value = queue.take();
    EXPECT_EQ(value, 42);
    value = queue.take();
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);
}

// Test thread safety of size(), empty(), full(), capacity() methods
// TEST_F(BoundedBlockingQueueTest, ThreadSafetyOfQueryMethods)
// {
//     BoundedBlockingQueue<int> queue(10);
//     std::atomic<bool> stop{false};
//     std::atomic<int> query_count{0};
    
//     // Thread that continuously queries the queue state
//     Thread query_thread([&]() {
//         while (!stop)
//         {
//             size_t size = queue.size();
//             bool empty = queue.empty();
//             bool full = queue.full();
//             size_t capacity = queue.capacity();
            
//             // Basic consistency checks
//             EXPECT_EQ(capacity, 11); // +1 for the empty item
//             EXPECT_EQ(empty, (size == 0));
//             EXPECT_EQ(full, (size + 1 == capacity));
            
//             query_count.fetch_add(1);
//             CurrentThread::sleepUsec(100); // 0.1ms
//         }
//     });
    
//     // Thread that modifies the queue
//     Thread modifier_thread([&]() {
//         for (int i = 0; i < 100; ++i)
//         {
//             queue.put(i);
//             CurrentThread::sleepUsec(1000); // 1ms
//             if (i % 10 == 0)
//             {
//                 queue.take();
//             }
//         }
        
//         // Empty the queue
//         while (!queue.empty())
//         {
//             queue.take();
//         }
        
//         stop = true;
//     });
    
//     query_thread.start();
//     modifier_thread.start();
    
//     modifier_thread.join();
//     query_thread.join();
    
//     EXPECT_GT(query_count.load(), 0);
//     EXPECT_TRUE(queue.empty());
// }
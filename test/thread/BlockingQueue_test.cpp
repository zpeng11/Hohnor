#include "hohnor/thread/BlockingQueue.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>

using namespace Hohnor;

class BlockingQueueTest : public ::testing::Test
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

// Test basic put and take operations
TEST_F(BlockingQueueTest, BasicPutTake)
{
    BlockingQueue<int> queue;
    
    // Test initial state
    EXPECT_EQ(queue.size(), 0);
    
    // Test put and take
    queue.put(42);
    EXPECT_EQ(queue.size(), 1);
    
    int value = queue.take();
    EXPECT_EQ(value, 42);
    EXPECT_EQ(queue.size(), 0);
}

// Test multiple put and take operations
TEST_F(BlockingQueueTest, MultiplePutTake)
{
    BlockingQueue<int> queue;
    
    // Put multiple values
    for (int i = 0; i < 10; ++i)
    {
        queue.put(i);
    }
    EXPECT_EQ(queue.size(), 10);
    
    // Take all values in order
    for (int i = 0; i < 10; ++i)
    {
        int value = queue.take();
        EXPECT_EQ(value, i);
    }
    EXPECT_EQ(queue.size(), 0);
}

// Test move semantics for put operation
TEST_F(BlockingQueueTest, MoveSemanticsTest)
{
    BlockingQueue<std::string> queue;
    
    std::string original = "test_string";
    std::string copy = original;
    
    // Test move put
    queue.put(std::move(copy));
    EXPECT_EQ(queue.size(), 1);
    
    // The moved string should be empty or in valid but unspecified state
    // We can't guarantee what state it's in, so we just check the queue works
    std::string result = queue.take();
    EXPECT_EQ(result, original);
    EXPECT_EQ(queue.size(), 0);
}

// Test copy semantics for put operation
TEST_F(BlockingQueueTest, CopySemanticsTest)
{
    BlockingQueue<std::string> queue;
    
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

// Test blocking behavior with producer-consumer pattern
TEST_F(BlockingQueueTest, ProducerConsumerSingleThread)
{
    BlockingQueue<int> queue;
    std::vector<int> consumed;
    std::atomic<bool> consumer_started{false};
    std::atomic<bool> consumer_finished{false};
    
    // Consumer thread
    Thread consumer([&]() {
        consumer_started = true;
        for (int i = 0; i < 5; ++i)
        {
            int value = queue.take();
            consumed.push_back(value);
        }
        consumer_finished = true;
    });
    
    consumer.start();
    
    // Wait for consumer to start
    while (!consumer_started)
    {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Give consumer a moment to block on empty queue
    CurrentThread::sleepUsec(10000); // 10ms
    EXPECT_FALSE(consumer_finished);
    
    // Producer - put items with delays
    for (int i = 0; i < 5; ++i)
    {
        queue.put(i);
        CurrentThread::sleepUsec(5000); // 5ms between puts
    }
    
    consumer.join();
    
    EXPECT_TRUE(consumer_finished);
    EXPECT_EQ(consumed.size(), 5);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(consumed[i], i);
    }
}

// Test multiple producers and consumers
TEST_F(BlockingQueueTest, MultipleProducersConsumers)
{
    BlockingQueue<int> queue;
    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 100;
    
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

// Test size() method accuracy under concurrent access
TEST_F(BlockingQueueTest, SizeAccuracy)
{
    BlockingQueue<int> queue;
    
    // Test size with sequential operations
    EXPECT_EQ(queue.size(), 0);
    
    queue.put(1);
    EXPECT_EQ(queue.size(), 1);
    
    queue.put(2);
    EXPECT_EQ(queue.size(), 2);
    
    queue.take();
    EXPECT_EQ(queue.size(), 1);
    
    queue.take();
    EXPECT_EQ(queue.size(), 0);
}

// Test giveUp functionality
TEST_F(BlockingQueueTest, GiveUpFunctionality)
{
    BlockingQueue<int> queue;
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
    CurrentThread::sleepUsec(50000); // 50ms
    
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

// Test behavior after giveUp is called
TEST_F(BlockingQueueTest, BehaviorAfterGiveUp)
{
    BlockingQueue<int> queue;
    
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

TEST_F(BlockingQueueTest, CustomObjectTest)
{
    BlockingQueue<TestObject> queue;
    
    TestObject obj1(42, "test1");
    TestObject obj2(84, "test2");
    
    queue.put(obj1);
    queue.put(obj2);
    
    EXPECT_EQ(queue.size(), 2);
    
    TestObject result1 = queue.take();
    TestObject result2 = queue.take();
    
    EXPECT_EQ(result1, obj1);
    EXPECT_EQ(result2, obj2);
    EXPECT_EQ(queue.size(), 0);
}

// Test giveUp with custom objects
TEST_F(BlockingQueueTest, GiveUpWithCustomObjects)
{
    BlockingQueue<TestObject> queue;
    
    queue.put(TestObject(1, "one"));
    queue.giveUp();
    
    TestObject result = queue.take();
    TestObject expected; // Default constructed
    
    EXPECT_EQ(result, expected);
}

// Test queue behavior under stress
TEST_F(BlockingQueueTest, StressTest)
{
    BlockingQueue<int> queue;
    const int num_operations = 1000;
    std::atomic<int> put_count{0};
    std::atomic<int> take_count{0};
    
    // Producer thread
    Thread producer([&]() {
        for (int i = 0; i < num_operations; ++i)
        {
            queue.put(i);
            put_count.fetch_add(1);
            if (i % 100 == 0)
            {
                CurrentThread::sleepUsec(1000); // 1ms pause every 100 items
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
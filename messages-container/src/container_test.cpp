#include <messages-container/lock_free_container.hpp>
#include <message.hpp>


#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <iostream>
#include <cassert>

namespace
{

constexpr size_t INITIAL_CAPACITY = 8191;
constexpr size_t NUM_OPERATIONS   = 100000;
constexpr size_t NUM_KEYS         = 1000;
constexpr auto TIMEOUT_MS         = 1000;

thread_local std::mt19937_64 rng(std::random_device{}());

std::uniform_int_distribution<uint64_t> key_dist(0, UINT64_MAX);
std::uniform_int_distribution<uint16_t> dist_size(1, 1024);
std::uniform_int_distribution<uint8_t> dist_type(0, 255);
std::uniform_int_distribution<uint64_t> dist_data(0, UINT64_MAX);

Message generate_random_message(uint64_t id)
{
    return Message{
        dist_size(rng),  // MessageSize
        dist_type(rng),  // MessageType
        id,              // MessageId
        dist_data(rng)   // MessageData
    };
}

int stress_test(HashMap<Message, INITIAL_CAPACITY>& map, std::atomic<bool>& running)
{
    std::vector<Message> local_messages;
    local_messages.reserve(NUM_KEYS);

    for (uint64_t i = 0; i < NUM_KEYS; i++)
    {
        auto key = key_dist(rng);
        local_messages.push_back(generate_random_message(key));
        map.insert(local_messages.back());
    }

    while (running.load())
    {
        for (auto message : local_messages)
        {
            auto key = message.MessageId;
            int op = key % 3;

            switch (op)
            {
            case 0:
            {  // Insert
                auto local_key = key_dist(rng);
                Message msg = generate_random_message(local_key);
                map.insert(msg);
                break;
            }
            case 1:
            {  // Find
                Message found_msg;
                map.find(key, found_msg);
                break;
            }
            case 2:
            {  // Remove
                map.remove(key);
                break;
            }
            }
        }
    }

    // Cleanup phase
    for (auto message : local_messages)
    {
        auto key = message.MessageId;
        map.remove(key);
        Message found_msg;
        bool found = map.find(key, found_msg);
        assert(!found);  // Should be removed
    }

    return 0;
}

void concurrent_operations_test(HashMap<Message, INITIAL_CAPACITY>& map, size_t num_threads)
{
    std::atomic<bool> running(true);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(stress_test, std::ref(map), std::ref(running));
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    running.store(false);

    for (auto& t : threads)
    {
        t.join();
    }

    // Verify final state
    assert(map.size() == 0);  // All keys should be removed
}

void basic_concurrent_test(HashMap<Message, INITIAL_CAPACITY>& map, size_t num_threads)
{
    std::vector<std::thread> threads;
    std::vector<uint64_t> keys;
    threads.reserve(num_threads);
    keys.reserve(num_threads); // dangerous, but we know the exact number of inserts and prevent reallocation

    // Concurrent inserts
    for (size_t i = 0; i < num_threads; ++i)
    {
        auto test_key          = key_dist(rng);
        Message test_msg       = generate_random_message(test_key);
        keys.push_back(test_msg.MessageId);

        threads.emplace_back(
            [test_msg, &map, &keys]()
            {
                bool inserted = map.insert(test_msg);
                if (inserted)
                {
                    Message found_msg;
                    bool found = map.find(test_msg.MessageId, found_msg);
                    assert(found);
                }
                else {
                    std::cerr << "Failed to insert message with ID: " << test_msg.MessageId << "\n";
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    map.debug();

    std::cout << "Messages inserted, map size: " << map.size() << "\n";

     // Should have exactly one successful insert
     Message found_msg;
     bool found = map.find(keys[3], found_msg);
     assert(found);

    threads.clear();

    // Concurrent removes
    for (size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(
            [&keys, &map, i]()
            {
                map.remove(keys[i]);
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }


    std::cout << "Verifying removal key: " << keys[1] << "\n";
    map.debug();
    found = map.find(keys[1], found_msg);
    assert(!found);

    std::cout << "Current Map Size: " << map.size() << "\n";
    assert(map.size() == 0);
}

}  // namespace

int main()
{
    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
    {
        num_threads = 4;
    }
    std::cout << "Running tests with " << num_threads << " threads\n";

    HashMap<Message, INITIAL_CAPACITY> map;

    std::cout << "Running basic concurrency test...\n";
    basic_concurrent_test(map, num_threads);

    std::cout << "\n\nRunning stress test...\n";
    concurrent_operations_test(map, num_threads);

    std::cout << "All tests passed!\n";


    return 0;
}

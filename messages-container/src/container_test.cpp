#include <messages-container/lock_free_container.hpp>

#include <iostream>

/*
struct Message {
    uint16_t MessageSize;
    uint8_t MessageType;
    uint64_t MessageId;
    uint64_t MessageData;
};
*/

int main()
{
    LockFreeMessageMap messageMap(8191);

    Message msg1{10, 1, 1001, 12345};
    Message msg2{12, 2, 1002, 67890};
    Message msg3{14, 3, 1001, 99999};  // Duplicate MessageId

    bool inserted1 = messageMap.insert(msg1);
    bool inserted2 = messageMap.insert(msg2);
    bool inserted3 = messageMap.insert(msg3);  // Should be dropped

    std::cout << "Message 1 inserted: " << (inserted1 ? "Yes" : "No") << std::endl;
    std::cout << "Message 2 inserted: " << (inserted2 ? "Yes" : "No") << std::endl;
    std::cout << "Message 3 inserted: " << (inserted3 ? "Yes" : "No") << std::endl;

    Message foundMsg;
    bool found = messageMap.find(1001, foundMsg);
    if (found)
    {
        std::cout << "Found Message: " << foundMsg.MessageData << std::endl;
    }
    else
    {
        std::cout << "Message not found!" << std::endl;
    }

    messageMap.remove(1001);

    found = messageMap.find(1001, foundMsg);
    if (!found)
    {
        std::cout << "Message 1001 removed successfully!" << std::endl;
    }

    return 0;
}

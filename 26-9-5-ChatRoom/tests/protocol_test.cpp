#include "chat/protocol.h"

#include <cassert>
#include <iostream>

int main() {
    std::string error;
    const auto chat = chat::parseMessage("CHAT|alice|hello", error);
    assert(chat.has_value());
    assert(chat->type == chat::MessageType::Chat);
    assert(chat->nickname == "alice");
    assert(chat->content == "hello");
    assert(chat::serializeMessage(*chat) == "CHAT|alice|hello");

    assert(!chat::parseMessage("UNKNOWN|alice|hello", error).has_value());
    assert(!chat::parseMessage("CHAT|alice|", error).has_value());
    assert(!chat::parseMessage("JOIN|alice|unexpected", error).has_value());
    assert(!chat::parseMessage("CHAT|alice|line\nfeed", error).has_value());
    assert(!chat::parseMessage("CHAT|alice|pipe|not-allowed", error).has_value());

    std::cout << "protocol tests passed\n";
    return 0;
}

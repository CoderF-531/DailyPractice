#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace chat {

constexpr std::size_t kMaxNicknameBytes = 20;
constexpr std::size_t kMaxContentBytes = 512;
constexpr std::size_t kMaxDatagramBytes = 1024;

enum class MessageType { Join, Chat, Leave, System };

struct Message {
    MessageType type;
    std::string nickname;
    std::string content;
};

std::optional<Message> parseMessage(std::string_view wire, std::string& error);
std::string serializeMessage(const Message& message);
std::string messageTypeName(MessageType type);

}  // namespace chat

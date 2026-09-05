#include "chat/protocol.h"

#include <algorithm>

namespace chat {
namespace {

bool isControlCharacter(unsigned char value) {
    return value < 0x20 || value == 0x7F;
}

bool hasControlCharacter(std::string_view value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return isControlCharacter(character);
    });
}

std::optional<MessageType> parseType(std::string_view value) {
    if (value == "JOIN") {
        return MessageType::Join;
    }
    if (value == "CHAT") {
        return MessageType::Chat;
    }
    if (value == "LEAVE") {
        return MessageType::Leave;
    }
    if (value == "SYSTEM") {
        return MessageType::System;
    }
    return std::nullopt;
}

}  // namespace

std::optional<Message> parseMessage(std::string_view wire, std::string& error) {
    if (wire.empty() || wire.size() > kMaxDatagramBytes) {
        error = "message length is invalid";
        return std::nullopt;
    }

    const std::size_t firstSeparator = wire.find('|');
    const std::size_t secondSeparator = wire.find('|', firstSeparator == std::string_view::npos ? 0 : firstSeparator + 1);
    if (firstSeparator == std::string_view::npos || secondSeparator == std::string_view::npos) {
        error = "message must contain type, nickname and content fields";
        return std::nullopt;
    }

    const std::optional<MessageType> type = parseType(wire.substr(0, firstSeparator));
    if (!type.has_value()) {
        error = "unknown message type";
        return std::nullopt;
    }

    const std::string_view nickname = wire.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1);
    const std::string_view content = wire.substr(secondSeparator + 1);
    if (nickname.empty() || nickname.size() > kMaxNicknameBytes || nickname.find('|') != std::string_view::npos ||
        hasControlCharacter(nickname)) {
        error = "nickname is invalid";
        return std::nullopt;
    }
    if (content.size() > kMaxContentBytes || content.find('|') != std::string_view::npos || hasControlCharacter(content)) {
        error = "content is invalid";
        return std::nullopt;
    }
    if ((*type == MessageType::Join || *type == MessageType::Leave) && !content.empty()) {
        error = "JOIN and LEAVE messages cannot contain content";
        return std::nullopt;
    }
    if ((*type == MessageType::Chat || *type == MessageType::System) && content.empty()) {
        error = "CHAT and SYSTEM messages require content";
        return std::nullopt;
    }

    return Message{*type, std::string(nickname), std::string(content)};
}

std::string serializeMessage(const Message& message) {
    return messageTypeName(message.type) + "|" + message.nickname + "|" + message.content;
}

std::string messageTypeName(MessageType type) {
    switch (type) {
        case MessageType::Join:
            return "JOIN";
        case MessageType::Chat:
            return "CHAT";
        case MessageType::Leave:
            return "LEAVE";
        case MessageType::System:
            return "SYSTEM";
    }
    return "UNKNOWN";
}

}  // namespace chat

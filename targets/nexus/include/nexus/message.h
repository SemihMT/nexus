#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>
#include <cstring>

namespace nxs
{
    // MessageType is a user-defined enum class that should use uint32_t as its underlying type, to ensure a set size
    template <typename MessageType>
    struct Header
    {
        // Test for MessageType being an enum class and having uint32_t as its underlying type
        static_assert(std::is_enum_v<MessageType>, "MessageType must be an enum class.");
        static_assert(std::is_same_v<std::underlying_type_t<MessageType>, uint32_t>,
                      "MessageType must have uint32_t as its underlying type.");


        size_t Size() const { return sizeof(size) + sizeof(owner) + sizeof(type); }
        // The size of the message body in bytes NOT INCLUDING the header
        uint32_t size{};
        // The ID of the message sender
        uint32_t owner{};
        // The type of the message being sent (user-defined)
        MessageType type{};
    };

    template <typename MessageType>
    struct Message
    {
        // Test for MessageType being an enum class and having uint32_t as its underlying type
        static_assert(std::is_enum_v<MessageType>, "MessageType must be an enum class.");
        static_assert(std::is_same_v<std::underlying_type_t<MessageType>, uint32_t>,
                      "MessageType must have uint32_t as its underlying type.");

        Message(MessageType type, uint32_t owner = 0) : m_Header{0, owner, type} {}

        size_t Size() const { return m_Header.Size() + m_Data.size(); }
        size_t BodySize() const { return m_Data.size(); }
        
        // Overloaded << operator to allow for easy message creation
        template <typename T>
        friend Message<MessageType> &operator<<(Message<MessageType> &msg, const T &data)
        {
            static_assert(std::is_standard_layout_v<T>, "Data must be standard layout.");
            const size_t size = msg.m_Data.size();

            msg.m_Data.resize(size + sizeof(T));
            std::memcpy(msg.m_Data.data() + size, &data, sizeof(T));
            msg.m_Header.size = msg.m_Data.size();
            return msg;
        }
        // Overloaded >> operator to allow for easy message reading
        template <typename T>
        friend Message<MessageType> &operator>>(Message<MessageType> &msg, T &data)
        {
            static_assert(std::is_standard_layout_v<T>, "Data must be standard layout.");
            const size_t size = msg.m_Data.size() - sizeof(T);
            std::memcpy(&data, msg.m_Data.data() + size, sizeof(T));
            msg.m_Data.resize(size);
            msg.m_Header.size = msg.m_Data.size();
            return msg;
        }
        // Overloaded << operator to allow for easy message creation
        friend Message<MessageType> &operator<<(Message<MessageType> &msg, const std::string &data)
        {
            const size_t size = msg.m_Data.size();
            // Resize the data vector to fit the string size (size_t) + the actual string size
            msg.m_Data.resize(size + data.size() + sizeof(size_t) );
            // Copy the string to the end of the data vector
            std::memcpy(msg.m_Data.data() + size, data.data(), data.size());
            // Copy the size of the string to the end of the data vector
            size_t dataSize = data.size();
            std::memcpy(msg.m_Data.data() + size + data.size(), &dataSize, sizeof(size_t));
            // Update the header size
            msg.m_Header.size = msg.m_Data.size();
            return msg;
        }
        // Overloaded >> operator to allow for easy message reading
        friend Message<MessageType> &operator>>(Message<MessageType> &msg, std::string &data)
        {
           // Read the size of the string
            size_t dataSize;
            std::memcpy(&dataSize, msg.m_Data.data() + msg.m_Data.size() - sizeof(size_t), sizeof(size_t));
            // Resize the string to fit the data
            data.resize(dataSize);
            // Copy the data to the string
            std::memcpy(data.data(), msg.m_Data.data() + msg.m_Data.size() - sizeof(size_t) - dataSize, dataSize);
            // Resize the data vector to remove the string
            msg.m_Data.resize(msg.m_Data.size() - dataSize - sizeof(size_t));
            // Update the header size
            msg.m_Header.size = msg.m_Data.size();
            return msg;
        }

        // Overloaded ostream operator to allow for easy message debugging
        friend std::ostream &operator<<(std::ostream &os, const Message &msg)
        {
            os << "Header: {size: " << msg.m_Header.size << ", owner: " << static_cast<unsigned>(msg.m_Header.owner) << ", type: " << static_cast<uint32_t>(msg.m_Header.type) << "}\n";
            // Print the data
            os << "Data: {";
            if (!msg.m_Data.empty())
            {
                size_t i = 0;
                while (i < msg.m_Data.size())
                {
                    os << std::hex << "0x"; // Display in hexadecimal format for better readability
                    os << static_cast<uint32_t>(static_cast<unsigned char>(msg.m_Data[i]));
                    if (++i < msg.m_Data.size())
                        os << ", ";
                }
            }
            else
            {
                os << "empty";
            }
            os << "}";

            // Reset the stream formatting to its original state
            os << std::dec;

            return os;
        }
        Header<MessageType> m_Header{};
        std::vector<std::byte> m_Data;
    };

} // namespace nxs
#endif // MESSAGE_H
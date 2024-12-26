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

        Header() = default;
        // The size of the message body in bytes NOT INCLUDING the header
        uint32_t size{};
        // The ID of the message sender
        uint8_t owner{};
        // The type of the message being sent (user-defined)
        MessageType type{};
    };

    template <typename MessageType>
    class Message
    {
        // Test for MessageType being an enum class and having uint32_t as its underlying type
        static_assert(std::is_enum_v<MessageType>, "MessageType must be an enum class.");
        static_assert(std::is_same_v<std::underlying_type_t<MessageType>, uint32_t>,
                      "MessageType must have uint32_t as its underlying type.");

    public:
        Message(MessageType type, uint8_t owner = 0) : m_Header{0, owner, type} {}

        // Overloaded << operator to allow for easy message creation
        template <typename T>
        Message &operator<<(const T &data)
        {
            static_assert(std::is_standard_layout_v<T>, "Data must be standard layout.");
            const size_t size = m_Data.size();
            std::cout << "Data size: " << sizeof(T) << std::endl;

            m_Data.resize(size + sizeof(T));
            std::memcpy(m_Data.data() + size, &data, sizeof(T));
            m_Header.size = m_Data.size();
            return *this;
        }
        // Overloaded >> operator to allow for easy message reading
        template <typename T>
        Message &operator>>(T &data)
        {
            static_assert(std::is_standard_layout_v<T>, "Data must be standard layout.");
            const size_t size = m_Data.size() - sizeof(T);
            std::memcpy(&data, m_Data.data() + size, sizeof(T));
            m_Data.resize(size);
            m_Header.size = m_Data.size();
            return *this;
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

    private:
        Header<MessageType> m_Header{};
        std::vector<std::byte> m_Data;
    };

} // namespace nxs
#endif // MESSAGE_H
#pragma once

#include <boost/crc.hpp>
#include <concepts>
#include <cstdint>
#include <deque>
#include <span>
#include <string>

/**
 * A concept describing an object that can receive and process data packets parsed by the PacketParser.
 *
 * Required to have one member function named handle_command_N per every known packet type.
 */
template <typename Handler>
concept CommandHandlerConcept = requires(Handler h, std::string str_data, uint8_t u8_data, uint16_t u16_data) {
    { h.handle_command_1(std::move(str_data)) } -> std::same_as<void>;
    { h.handle_command_2(u8_data) } -> std::same_as<void>;
    { h.handle_command_3(u16_data, u8_data) } -> std::same_as<void>;
};

/**
 * A functor object that can handle blocks of binary data received from the network client.
 *
 * Can be used as a handler object for tcp_server::TcpServer. A separate instance must be created for every client
 * connection by a factory object or function.
 *
 * @tparam CommandHandler the next handler type that will process parsed data. See the concept for the concrete
 * function signatures.
 */
template <CommandHandlerConcept CommandHandler>
class PacketParser
{
    enum class ParserState
    {
        header, // Looking for a packet start
        data, // Packet start matched - need to determine the data length and CRC position
        crc, // Need to check the CRC
        handle, // Packet is valid and ready - parse it and send to the handler
        fail // Packet is invalid - continue looking for a header
    };

    // Just a few constants for a simple parsing mechanism. This approach is error-prone as the constants are scattered
    // over the parser implementation. Some of these constants and associated methods can be represented as separate
    // data access objects if the code grows.
    static constexpr char header[] = "CMD";
    static constexpr unsigned int header_length = std::char_traits<char>::length(header);
    static constexpr unsigned int cmd_id_pos = header_length;
    static constexpr unsigned int cmd_id_length = 2;
    static constexpr unsigned int data_pos = cmd_id_pos + cmd_id_length;
    static constexpr unsigned int crc_length = 2;
    static constexpr unsigned int min_packet_length = header_length + cmd_id_length + crc_length;
    // current protocol technically allows zero-length data, but currently every command uses at least one byte
    static constexpr unsigned int min_data_length = 1;

    CommandHandler& handler_;
    std::deque<char> buffer_{};
    ParserState state_ = ParserState::header;
    int cmd_id_ = 0;
    int data_length_ = min_data_length;

    // Utility function to read big endian uint16 from the buffer.
    [[nodiscard]] uint16_t read_uint16_(const int buffer_position) const
    {
        const auto high = static_cast<unsigned char>(buffer_[buffer_position]);
        const auto low = static_cast<unsigned char>(buffer_[buffer_position + 1]);
        return high << 8 | low;
    }

    // Check if the buffer head contains the proper header value.
    ParserState find_header_()
    {
        // Buffer is guaranteed to be longer than header length by the parse() while loop condition
        for (unsigned int i = 0; i < header_length; i++)
        {
            if (buffer_[i] != header[i])
            {
                // Buffer head does not match the required header format.
                // Drop the first byte and try again if possible.
                buffer_.pop_front();
                return ParserState::header;
            }
        }
        // Header matched - proceed to cmd id check and data size estimation.
        return ParserState::data;
    }

    // Check the cmd id and estimate remaining data length.
    ParserState parse_cmd_id_()
    {
        cmd_id_ = read_uint16_(cmd_id_pos);
        switch (cmd_id_)
        {
        case 1:
            data_length_ = 1 + static_cast<unsigned char>(buffer_[data_pos]);
            break;
        case 2:
            data_length_ = 1;
            break;
        case 3:
            data_length_ = 3;
            break;
        default:
            // Unknown cmd id - proceed to failed packet handling.
            return ParserState::fail;
        }
        // Data length estimated - check the CRC if available.
        return ParserState::crc;
    }

    // Parse the received data and call an appropriate handler_ method.
    ParserState handle_command_()
    {
        switch (cmd_id_)
        {
        case 1: // length_u8 char[length]
            {
                const auto data_start = buffer_.cbegin() + data_pos;
                auto data_1 = std::string(data_start + 1, data_start + data_length_);
                handler_.handle_command_1(std::move(data_1));
                break;
            }
        case 2: // data_u8
            {
                auto data_2 = static_cast<uint8_t>(buffer_[data_pos]);
                handler_.handle_command_2(data_2);
                break;
            }
        case 3: // data_u16 data_u8
            {
                auto data_3_1 = read_uint16_(data_pos);
                auto data_3_2 = static_cast<uint8_t>(buffer_[data_pos + 2]);
                handler_.handle_command_3(data_3_1, data_3_2);
                break;
            }
        default:
            return ParserState::fail; // Defensive coding. Unreachable due to the check in parse_cmd_id_.
        }
        // Command was successfully parsed and handled. Fully remove it from the buffer.
        buffer_.erase(buffer_.begin(), buffer_.begin() + min_packet_length + data_length_);
        // And reset the estimated data length to the minimal value in case we receive the smallest packet next time.
        data_length_ = min_data_length;
        // And start looking for the next header.
        return ParserState::header;
    }

    // Compute the crc over command id and the data, compare it against the packet's crc bytes.
    ParserState check_crc_()
    {
        boost::crc_16_type crc;
        const unsigned int crc_pos = data_pos + data_length_;
        // Unfortunately, the dequeue is not continuous, so we have to compute the crc one byte at a time.
        for (unsigned int i = cmd_id_pos; i < crc_pos; i++)
        {
            crc.process_byte(buffer_[i]);
        }
        const int expected = read_uint16_(crc_pos);
        return crc.checksum() == expected ? ParserState::handle : ParserState::fail;
    }

    // Handle an invalid command id or a broken crc.
    ParserState fail_packet_()
    {
        // This can only happen if we have successfully found the header string.
        // So now we can fully skip the header bytes, but not the command id as it might be the start of another header.
        buffer_.erase(buffer_.begin(), buffer_.begin() + header_length);
        data_length_ = min_data_length; // prepare to read the smallest possible packet.
        return ParserState::header;
    }

    // Simple state machine step function.
    ParserState state_machine_step_()
    {
        switch (state_)
        {
        case ParserState::header:
            return find_header_();
        case ParserState::data:
            return parse_cmd_id_();
        case ParserState::crc:
            return check_crc_();
        case ParserState::handle:
            return handle_command_();
        case ParserState::fail:
            return fail_packet_();
        default:
            return ParserState::fail;
        }
    }

public:
    /**
     * Creates a new parser instance.
     *
     * @param handler the next handler object that will process parsed data. See the concept for the concrete
     * function signatures. May be shared between multiple parser instances.
     */
    explicit PacketParser(CommandHandler& handler) : handler_(handler) {}

    /**
     * This function is responsible for reconstructing the packets from (potentially fragmented) sequences of bytes,
     * parsing the commands and passing the resulting data to the handler object.
     *
     * @param packet a sequence of bytes received from the network client.
     */
    void operator()(std::span<const char> packet)
    {
        // Copy the received data to the internal buffer to handle short reads and invalid messages.
        buffer_.insert(buffer_.end(), packet.begin(), packet.end());
        // ReSharper disable once CppDFALoopConditionNotUpdated
        // Buffer size and data length are indirectly modified inside the loop.
        // Make sure that the buffer always contains at least header+cmd_id+(data for that cmd)+crc bytes.
        while (buffer_.size() >= min_packet_length + data_length_)
        {
            state_ = state_machine_step_();
        }
    }
};

#pragma once

#include <boost/crc.hpp>
#include <cstdint>
#include <deque>
#include <span>
#include <string>

template <typename PacketHandler>
class PacketParser
{
    enum class ParserState
    {
        header,
        data,
        crc,
        handle,
        fail
    };

    static constexpr char header[] = "CMD";
    static constexpr unsigned int header_length = std::char_traits<char>::length(header);
    static constexpr unsigned int cmd_id_pos = header_length;
    static constexpr unsigned int cmd_id_length = 2;
    static constexpr unsigned int data_pos = cmd_id_pos + cmd_id_length;
    static constexpr unsigned int crc_length = 2;
    static constexpr unsigned int min_packet_length = header_length + cmd_id_length + crc_length;
    static constexpr unsigned int min_data_length = 1;

    PacketHandler& handler_;
    std::deque<char> buffer_{};
    ParserState state_ = ParserState::header;
    int cmd_id_ = 0;
    int data_length_ = min_data_length;

    [[nodiscard]] uint16_t read_uint16_(const int buffer_position) const
    {
        const auto high = static_cast<unsigned char>(buffer_[buffer_position]);
        const auto low = static_cast<unsigned char>(buffer_[buffer_position + 1]);
        return high << 8 | low;
    }

    ParserState find_header_()
    {
        // buffer is guaranteed to be longer than header length by the parse() while loop condition
        for (unsigned int i = 0; i < header_length; i++)
        {
            if (buffer_[i] != header[i])
            {
                buffer_.pop_front();
                return ParserState::header;
            }
        }
        return ParserState::data;
    }

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
            return ParserState::fail;
        }
        return ParserState::crc;
    }

    ParserState handle_packet_()
    {
        switch (cmd_id_)
        {
        case 1:
            {
                const auto data_start = buffer_.cbegin() + data_pos;
                auto data_1 = std::string(data_start + 1, data_start + data_length_);
                handler_.handle_packet_1(data_1);
                break;
            }
        case 2:
            {
                auto data_2 = static_cast<uint8_t>(buffer_[data_pos]);
                handler_.handle_packet_2(data_2);
                break;
            }
        case 3:
            {
                auto data_3_1 = read_uint16_(data_pos);
                auto data_3_2 = static_cast<uint8_t>(buffer_[data_pos + 2]);
                handler_.handle_packet_3(data_3_1, data_3_2);
                break;
            }
        default:
            return ParserState::fail;
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + min_packet_length + data_length_);
        data_length_ = min_data_length;
        return ParserState::header;
    }

    ParserState check_crc_()
    {
        boost::crc_16_type crc;
        for (unsigned int i = cmd_id_pos; i < cmd_id_pos + cmd_id_length + data_length_; i++)
        {
            crc.process_byte(buffer_[i]);
        }
        const int expected = read_uint16_(data_pos + data_length_);
        return crc.checksum() == expected ? ParserState::handle : ParserState::fail;
    }

    ParserState fail_packet_()
    {
        buffer_.erase(buffer_.begin(), buffer_.begin() + header_length);
        data_length_ = min_data_length;
        return ParserState::header;
    }

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
            return handle_packet_();
        case ParserState::fail:
            return fail_packet_();
        default:
            return ParserState::fail;
        }
    }

public:
    explicit PacketParser(PacketHandler& handler) : handler_(handler) {}

    void operator()(std::span<const char> packet)
    {
        buffer_.insert(buffer_.end(), packet.begin(), packet.end());
        // ReSharper disable once CppDFALoopConditionNotUpdated
        // Buffer size and data length are indirectly modified inside the loop
        while (buffer_.size() >= min_packet_length + data_length_)
        {
            state_ = state_machine_step_();
        }
    }
};

#pragma once

#include <boost/crc.hpp>
#include <deque>
#include <ranges>
#include <span>
#include <string>

template <typename PacketHandler>
class PacketParser
{
private:
    enum class ParserState
    {
        header,
        data,
        crc,
        handle,
        fail
    };

    static constexpr char header[] = "CMD";
    static constexpr int header_length = std::char_traits<char>::length(header);
    static constexpr int cmd_id_pos = header_length;
    static constexpr int cmd_id_length = 2;
    static constexpr int data_pos = cmd_id_pos + cmd_id_length;
    static constexpr int crc_length = 2;
    static constexpr int min_packet_length = header_length + cmd_id_length + crc_length;
    static constexpr int min_data_length = 1;

    std::deque<char> buffer_;
    PacketHandler handler_;
    ParserState state_ = ParserState::header;
    int cmd_id_ = 0;
    int data_length_ = min_data_length;

    [[nodiscard]] int readUint16_(const int buffer_position) const
    {
        const auto high = static_cast<unsigned char>(buffer_[buffer_position]);
        const auto low = static_cast<unsigned char>(buffer_[buffer_position + 1]);
        return high << 8 | low;
    }

    void findHeader_()
    {
        // buffer is guaranteed to be longer than header length by the parse while loop condition
        for (int i = 0; i < header_length; i++)
        {
            if (buffer_[i] != header[i])
            {
                buffer_.pop_front();
                return;
            }
        }
        state_ = ParserState::data;
    }

    void parseCmdId_()
    {
        cmd_id_ = readUint16_(header_length);
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
            state_ = ParserState::fail;
            return;
        }
        state_ = ParserState::crc;
    }

    void checkCrc()
    {
        boost::crc_16_type crc;
        for (int i = cmd_id_pos; i < cmd_id_pos + cmd_id_length + data_length_; i++)
        {
            crc.process_byte(buffer_[i]);
        }
        const int expected = readUint16_(data_pos + data_length_);
        state_ = crc.checksum() == expected ? ParserState::handle : ParserState::fail;
    }

    void handlePacket()
    {
        switch (cmd_id_)
        {
        case 1:
            {
                auto data_start = buffer_.cbegin() + data_pos;
                auto data_1 = std::string(data_start + 1, data_start + data_length_);
                handler_.handle_packet_1(data_1);
                break;
            }
        case 2:
            {
                auto data_2 = static_cast<unsigned char>(buffer_[data_pos]);
                handler_.handle_packet_2(data_2);
                break;
            }
        case 3:
            {
                auto data_3_1 = readUint16_(data_pos);
                auto data_3_2 = static_cast<unsigned char>(buffer_[data_pos + 2]);
                handler_.handle_packet_3(data_3_1, data_3_2);
                break;
            }
        default:
            state_ = ParserState::fail;
            return;
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + min_packet_length + data_length_);
        data_length_ = min_data_length;
        state_ = ParserState::header;
    }

    void failPacket()
    {
        buffer_.erase(buffer_.begin(), buffer_.begin() + header_length);
        data_length_ = min_data_length;
        state_ = ParserState::header;
    }

public:
    void operator()(std::span<const char> packet)
    {
        buffer_.insert(buffer_.end(), packet.begin(), packet.end());

        while (buffer_.size() >= static_cast<unsigned int>(min_packet_length + data_length_))
        {
            switch (state_)
            {
            case ParserState::header:
                findHeader_();
                break;
            case ParserState::data:
                parseCmdId_();
                break;
            case ParserState::crc:
                checkCrc();
                break;
            case ParserState::handle:
                handlePacket();
                break;
            case ParserState::fail:
                failPacket();
                break;
            }
        }
    }
};

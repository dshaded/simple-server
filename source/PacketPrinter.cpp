#include "../include/PacketPrinter.hpp"

#include <format>

PacketPrinter::PacketPrinter(std::ostream& stream) : stream_{stream} {}

void PacketPrinter::handle_packet_1(const std::string& data_1) const
{
    stream_ << std::format("{:#06x} {}\n", 1, data_1);
}

void PacketPrinter::handle_packet_2(uint8_t data_2) const
{
    stream_ << std::format("{:#06x} {:#x}\n", 2, data_2);
}

void PacketPrinter::handle_packet_3(uint16_t data_3_1, uint8_t data_3_2) const
{
    stream_ << std::format("{:#06X} {:#x} {:#x}\n", 3, data_3_1, data_3_2);
}

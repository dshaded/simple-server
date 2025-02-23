#pragma once
#include <cstdint>
#include <iostream>
#include <string>

class PacketPrinter
{
    std::ostream &stream_;

public:
    explicit PacketPrinter(std::ostream &stream);
    void handle_packet_1(const std::string &data_1) const;
    void handle_packet_2(uint8_t data_2) const;
    void handle_packet_3(uint16_t data_3_1, uint8_t data_3_2) const;
};

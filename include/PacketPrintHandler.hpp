#pragma once
#include <string>
#include <iostream>

class PacketPrintHandler {
public:
    void handle_packet_1(std::string& data_1) { std::cout <<  data_1 << "\r\n"; };
    void handle_packet_2(const int data_2)  { std::cout <<  data_2 << "\r\n"; };
    void handle_packet_3(const int data_3_1, const int data_3_2)  { std::cout <<  data_3_1 << data_3_2 << "\r\n"; };
};

#pragma once
#include <cstdint>
#include <string>

/**
 * A simple command handler that prints the received command id and data to the provided stream.
 *
 * See PacketParser and CommandHandlerConcept for more details.
 */
class CommandPrinter
{
    std::ostream &stream_;

public:
    explicit CommandPrinter(std::ostream &stream);
    void handle_command_1(const std::string &data_1) const;
    void handle_command_2(uint8_t data_2) const;
    void handle_command_3(uint16_t data_3_1, uint8_t data_3_2) const;
};

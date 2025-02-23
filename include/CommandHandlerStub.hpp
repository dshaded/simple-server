#pragma once
#include <cstdint>
#include <string>
#include <vector>

/**
 * A stub command handler implementation. Mostly useful for testing purposes.
 *
 * See PacketParser and CommandHandlerConcept for more details.
 */
struct CommandHandlerStub
{
    std::vector<int> call_sequence;
    std::vector<std::string> cmd_1;
    std::vector<uint8_t> cmd_2;
    std::vector<std::pair<uint16_t, uint8_t>> cmd_3;

    void clear()
    {
        call_sequence.clear();
        cmd_1.clear();
        cmd_2.clear();
        cmd_3.clear();
    }

    void handle_command_1(std::string &&data_1)
    {
        call_sequence.push_back(1);
        cmd_1.push_back(std::move(data_1));
    }

    void handle_command_2(uint8_t data_2)
    {
        call_sequence.push_back(2);
        cmd_2.push_back(data_2);
    }

    void handle_command_3(uint16_t data_3_1, uint8_t data_3_2)
    {
        call_sequence.push_back(3);
        cmd_3.emplace_back(data_3_1, data_3_2);
    }
};

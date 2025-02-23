#include <CommandHandlerStub.hpp>
#include <PacketParser.hpp>
#include <boost/crc.hpp>
#include <catch2/catch_test_macros.hpp>
#include <span>

auto compute_packet_crc(std::span<char> packet)
{
    if (packet.size() > 5)
    {
        boost::crc_16_type crc;
        // compute checksum for every byte excluding header and crc
        crc.process_bytes(packet.data() + 3, packet.size() - 5);
        auto cs = crc.checksum();
        packet[packet.size() - 1] = cs &  0xff;
        packet[packet.size() - 2] = cs >> 8;
    }
}


TEST_CASE("PacketParser")
{
    auto stub = CommandHandlerStub{};
    auto parser = PacketParser<CommandHandlerStub>{stub};

    char test_data[] = "CMD\x00\x01\x05""ABCDE_";
    compute_packet_crc(test_data);

    parser(test_data);

    REQUIRE(stub.call_sequence == std::vector {1});
    REQUIRE(stub.cmd_1 == std::vector<std::string> {"ABCDE"});
    REQUIRE(stub.cmd_2.empty());
    REQUIRE(stub.cmd_3.empty());
}

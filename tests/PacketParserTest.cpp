#include <CommandHandlerStub.hpp>
#include <PacketParser.hpp>
#include <boost/crc.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <sstream>
#include <string>

using namespace std::string_literals;

static std::string make_packet(const std::string &data)
{
    boost::crc_16_type crc;
    crc.process_bytes(data.data(), data.length());
    const auto cs = crc.checksum();
    const unsigned char low = cs & 0xff;
    const unsigned char high = cs >> 8;
    return "CMD"s + data + static_cast<char>(high) + static_cast<char>(low);
}

using vi = std::vector<int>;
using vs = std::vector<std::string>;
using v8 = std::vector<uint8_t>;
using vp = std::vector<std::pair<uint16_t, uint8_t>>;

TEST_CASE("PacketParser")
{
    auto stub = CommandHandlerStub{};
    auto parser = PacketParser<CommandHandlerStub>{stub};
    auto data = std::ostringstream{};

    SECTION("Cmd 1 valid values")
    {
        data << make_packet("\x00\x01\x00"s);
        data << make_packet("\x00\x01\x01\x00"s);
        data << make_packet("\x00\x01\x{05}ABCDE"s);
        auto max_length_string = std::string(255, 'X');
        data << make_packet("\x00\x01\xff"s + max_length_string);

        parser(data.str());

        CHECK(stub.call_sequence == vi{1, 1, 1, 1});
        CHECK(stub.cmd_1 == vs{""s, "\0"s, "ABCDE"s, max_length_string});
    }

    SECTION("Cmd 2 valid values")
    {
        data << make_packet("\x00\x02\x00"s);
        data << make_packet("\x00\x02\xac"s);
        data << make_packet("\x00\x02\xff"s);

        parser(data.str());

        CHECK(stub.call_sequence == vi{2, 2, 2});
        CHECK(stub.cmd_2 == v8{0x00, 0xac, 0xff});
    }

    SECTION("Cmd 3 valid values")
    {
        data << make_packet("\x00\x03\x00\x00\x00"s);
        data << make_packet("\x00\x03\x45\x67\x89"s);
        data << make_packet("\x00\x03\xff\xff\xff"s);

        parser(data.str());

        CHECK(stub.call_sequence == vi{3, 3, 3});
        CHECK(stub.cmd_3 == vp{{0x00, 0x00}, {0x4567, 0x89}, {0xffff, 0xff}});
    }

    SECTION("Invalid cmd id")
    {
        data << make_packet("\x00\x00"s);
        data << make_packet("\x00\x04"s);
        data << make_packet("\x00\xa2\x12"s);
        data << make_packet("\xcd\x02\x34"s);
        data << make_packet("\xff\xff\x34\x56"s);

        parser(data.str());

        CHECK(stub.call_sequence.empty());
    }

    SECTION("Invalid prefix")
    {
        auto valid_packet = make_packet("\x00\x02\x00"s);
        data << valid_packet.replace(0, 3, "cMD");
        data << valid_packet.replace(0, 3, "CmD");
        data << valid_packet.replace(0, 3, "CMd");
        data << valid_packet.replace(0, 3, "CMD");

        parser(data.str());

        CHECK(stub.call_sequence == vi{2});
        CHECK(stub.cmd_2 == v8{0x00});
    }

    SECTION("Invalid crc")
    {
        auto valid_packet = make_packet("\x00\x01\x{05}ABCDE"s);
        data << valid_packet;
        data << valid_packet.replace(valid_packet.size() - 2, 2, "  ");

        parser(data.str());

        CHECK(stub.call_sequence == vi{1});
        CHECK(stub.cmd_1 == vs{"ABCDE"});
    }

    SECTION("Partial reads")
    {
        for (int i = 0; i < 100; i++)
        {
            parser(std::array<char, 0>{});
        }
        for (char c : make_packet("\x00\x01\x{05}ABCDE"s))
        {
            CHECK(stub.call_sequence.empty());
            parser(std::array{c});
        }

        CHECK(stub.call_sequence == vi{1});
        CHECK(stub.cmd_1 == vs{"ABCDE"});
    }

    SECTION("Mixed commands with invalid data")
    {
        auto packet_1 = make_packet("\x00\x01\x{03}QWE"s);
        auto packet_2 = make_packet("\x00\x02\x12"s);
        auto packet_3 = make_packet("\x00\x03\x34\x56\x78"s);
        data << "CMDgarbageCCCCMCCCCMDDMCMDCC0123"s;
        data << packet_1;
        data << packet_2;
        data << "CMD\x00\x02\x03__CMDCMDC"s;
        data << packet_3;
        data << "\x00\x00\x00\x00\x00"s;
        data << packet_2;
        data << "CMDCM"s;
        data << packet_1;
        data << "CMD\x00\x03\x00\x00\x00\x01"s;

        parser(data.str());

        CHECK(stub.call_sequence == vi{1, 2, 3, 2, 1});
        CHECK(stub.cmd_1 == vs{"QWE", "QWE"});
        CHECK(stub.cmd_2 == v8{0x12, 0x12});
        CHECK(stub.cmd_3 == vp{{0x3456, 0x78}});
    }

    SECTION("Nested command")
    {
        auto nested = make_packet("\x00\x01\x{03}QWE"s);
        char nested_len = static_cast<char>(nested.size());
        auto outer = make_packet("\x00\x01"s + nested_len + nested);

        parser(outer);

        CHECK(stub.call_sequence == vi{1});
        CHECK(stub.cmd_1 == vs{nested});
    }
}

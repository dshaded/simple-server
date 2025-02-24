#include <CommandPrinter.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <sstream>

using namespace std::string_literals;

TEST_CASE("CommandPrinter")
{
    auto output = std::ostringstream{};
    auto printer = CommandPrinter{output};

    SECTION("Cmd 1")
    {
        printer.handle_command_1(""s);
        CHECK(output.str() == "0x0001 \n"s);
        output.str("");

        printer.handle_command_1("hello"s);
        CHECK(output.str() == "0x0001 hello\n"s);
        output.str("");

        printer.handle_command_1("\x00\x01\x02\r\n"s);
        CHECK(output.str() == "0x0001 \x00\x01\x02\r\n\n"s);
        output.str("");
    }

    SECTION("Cmd 2")
    {
        printer.handle_command_2(0x00);
        CHECK(output.str() == "0x0002 0x0\n"s);
        output.str("");

        printer.handle_command_2(0x08);
        CHECK(output.str() == "0x0002 0x8\n"s);
        output.str("");

        printer.handle_command_2(0x1e);
        CHECK(output.str() == "0x0002 0x1e\n"s);
        output.str("");

        printer.handle_command_2(0xff);
        CHECK(output.str() == "0x0002 0xff\n"s);
        output.str("");
    }

    SECTION("Cmd 3")
    {
        printer.handle_command_3(0x00, 0x00);
        CHECK(output.str() == "0x0003 0x0 0x0\n"s);
        output.str("");

        printer.handle_command_3(0x0001, 0x01);
        CHECK(output.str() == "0x0003 0x1 0x1\n"s);
        output.str("");

        printer.handle_command_3(0x00e0, 0xf0);
        CHECK(output.str() == "0x0003 0xe0 0xf0\n"s);
        output.str("");

        printer.handle_command_3(0x0a12, 0xab);
        CHECK(output.str() == "0x0003 0xa12 0xab\n"s);
        output.str("");

        printer.handle_command_3(0xffff, 0xff);
        CHECK(output.str() == "0x0003 0xffff 0xff\n"s);
        output.str("");
    }
}

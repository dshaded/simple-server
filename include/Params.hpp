#pragma once

struct Params
{
    Params(int argc, char* argv[]);
    int port{};
    bool no_run{};
    bool invalid{};
};

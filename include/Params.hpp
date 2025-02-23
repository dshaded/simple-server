#pragma once

/**
 * A simple object for parsing the command line options.
 */
struct Params
{
    /**
     * Parses the provided options into member variables.
     * Prints help message or parameter validation errors if necessary.
     *
     * @param argc from main function
     * @param argv from main function
     */
    Params(int argc, char* argv[]);
    // Requested server port. 0 by default.
    int port{};
    // true if the program should terminate after parsing the command line options.
    bool no_run{};
    // true if parameter validation error was encountered.
    bool invalid{};
};

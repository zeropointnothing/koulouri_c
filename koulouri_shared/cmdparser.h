#pragma once
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <variant>
#include <vector>

enum ArgType {
    SWITCH, // Toggle. Presence acts as a value.
    VALUE, // Contains a value.
    APPEND // All instances should be grouped. Acts like `VALUE` otherwise.
};

struct Argument {
    const char* cswitch; // Shorthand switch: '-e'
    const char* cswitch_long; // Longhand switch: '--example'
    ArgType type = ArgType::SWITCH; // The type of value this should hold
};


struct ArgResult {
    Argument arg; // The original `Argument` this result represents.
    std::variant<bool, char*, int> value; // The resulting value.
};

class ParseResult {
    public:
    ParseResult(std::vector<ArgResult> results) : results(results) {};

    /**
     * Attempt to fetch the ArgResult struct(s) that represents a given argument
     * if it exists and was supplied by the user.
     *
     * Note, while all results will return a vector, only 'APPEND' types will actually
     * use it. For most, simply calling .at(0) will suffice.
     */
    std::vector<ArgResult> get(const char* name); // TODO: Find better way to go about this. std::variant?

    private:
    const std::vector<ArgResult> results;
};

class CmdParser {
public:
    /**
     * Register an argument to the CmdParser.
     *
     * All arguments must be registered this way to be picked up by `parse_args`.
     *
     * Note, that this function will call `std::exit` if any arguments supplied to it are invalid.
    */
    void register_argument(Argument argument);

    /**
     * Attempt to parse all of the arguments within the 'argv' parameter.
     *
     * Returns a 'ParseResult' object which can be used to dynamically get the value of an
     * argument if it exists and was supplied.
     */
    ParseResult parse_args(int argc, char* argv[]);

private:
    /**
     * Exit wrapper. Used to (somewhat) cleanly kill the program during runtime without
     * resulting in a core dump.
     *
     * Prints in annoying red.
     */
    [[noreturn]] void fatal_error(const char* msg) {
        std::cerr << "\033[1;31mCmdParser [FATAL]: " << msg << "\033[0m" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    /**
     * Check if an `Argument` struct is considered 'valid.'
     */
    bool valid_argument(Argument arg) {
        return ((arg.cswitch && strcmp(arg.cswitch, "")!=0) &&
            (arg.cswitch_long && strcmp(arg.cswitch_long, "")!=0)
        );
    }
    std::vector<Argument> registered;
};

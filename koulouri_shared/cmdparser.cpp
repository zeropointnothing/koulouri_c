#include "cmdparser.h"
#include <cstring>
#include <iostream>
#include <optional>
#include <vector>

void CmdParser::register_argument(Argument argument) {
    if (!valid_argument(argument)) {
        std::cout << strcmp(argument.cswitch, "") << std::endl;
        fatal_error("!! Arguments cannot be constructed without either a cswitch or cswitch_long!");
    }

    CmdParser::registered.push_back(argument);
}

std::optional<ArgResult> ParseResult::get(const char* name) {
    for (const ArgResult &res : results) {
        if (strcmp(res.arg.cswitch, name)==0 || strcmp(res.arg.cswitch_long, name)==0) {
            return res;
        }
    }

    return std::nullopt;
}

ParseResult CmdParser::parse_args(int argc, char* argv[]) {
    std::vector<ArgResult> final;
    for (Argument &arg : registered) {
        for (int i=1; i<argc; i++) {
            if (strcmp(argv[i], arg.cswitch)==0 || strcmp(argv[i], arg.cswitch_long)==0) {
                if (arg.type == ArgType::SWITCH) {
                    final.push_back({arg, true});
                } else if (arg.type == ArgType::VALUE) {
                    final.push_back({arg, argv[i+1]});
                }
            }
        }
    }

    return ParseResult(final);
}

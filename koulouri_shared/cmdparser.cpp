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

std::vector<ArgResult> ParseResult::get(const char* name) {
    std::vector<ArgResult> out;
    std::optional<ArgResult> last_result; // for non 'APPEND' types, get the last one

    for (const ArgResult &res : results) {
        if (strcmp(res.arg.cswitch, name)==0 || strcmp(res.arg.cswitch_long, name)==0) {
            if (res.arg.type != ArgType::APPEND) {
                last_result = res;
            } else {
                out.push_back(res);
            }
        }
    }

    if (last_result.has_value()) {
        out.push_back(last_result.value());
    }

    return out;
}

ParseResult CmdParser::parse_args(int argc, char* argv[]) {
    std::vector<ArgResult> final;
    for (Argument &arg : registered) {
        for (int i=1; i<argc; i++) {
            if (strcmp(argv[i], arg.cswitch)==0 || strcmp(argv[i], arg.cswitch_long)==0) {
                // std::cout << arg.cswitch_long << std::endl;
                if (arg.type == ArgType::SWITCH) {
                    final.push_back({arg, true});
                } else if (arg.type == ArgType::VALUE) {
                    final.push_back({arg, argv[i+1]});
                } else if (arg.type == ArgType::APPEND) {
                    final.push_back({arg, argv[i+1]}); // functionally the same as VALUE
                }
            }
        }
    }

    return ParseResult(final);
}

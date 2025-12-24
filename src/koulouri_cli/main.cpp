#include <iostream>
#include <stdexcept>
#include <thread>
#include <variant>
#include "libkoulouri/logger.h"
#include "libkoulouri/player.h"
#include "koulouri_shared/alsasilencer.h"
#include "koulouri_shared/cmdparser.h"

int main(int argc, char* argv[]) {
    Logger::setOutput(&std::cerr);

    const char* file = nullptr; // be safe!!!
    int volume = 70;

    CmdParser cmd;
    // cmd.register_argument({"", "", ArgType::SWITCH});
    cmd.register_argument({"-h", "--help", ArgType::SWITCH});
    cmd.register_argument({"-p", "--play", ArgType::VALUE});
    cmd.register_argument({"-d", "--debug", ArgType::SWITCH});
    cmd.register_argument({"-v", "--volume", ArgType::VALUE});

    ParseResult parsed = cmd.parse_args(argc, argv);

    if (auto lst = parsed.get("--help"); !lst.empty()) {
        ArgResult &res = lst.at(0);

        if (auto val = std::get_if<bool>(&res.value)) {
            std::cout << res.arg.cswitch_long << " | " << *val << std::endl;
        }
    }

    if (auto lst = parsed.get("--play"); !lst.empty()) {
        ArgResult &res = lst.at(0);

        if (auto val = std::get_if<char*>(&res.value)) { // pass a reference to the value contained
            std::cout << "PLAY: " << *val << std::endl;
            file = *val; // we get a pointer back from get_if
        }
    }

    if (auto lst = parsed.get("--volume"); !lst.empty()) {
        ArgResult &res = lst.at(0);

        if (auto val = std::get_if<char*>(&res.value)) {
            std::cout << *val << std::endl;
            try {
                volume = std::stoi(*val);
            } catch (std::invalid_argument e) {
                std::cerr << "Bad argument! : " << e.what() << " - '" << *val << "' is not a valid integer!" << std::endl;
            }
        }
    }

    if (auto lst = parsed.get("--debug"); !lst.empty()) {
        // no point in getting 'res', since we do nothing with the value...

        Logger::setVerbosity(Logger::Level::DEBUG);
        Logger::g_log("cli", Logger::Level::DEBUG, "DEBUG logging is ON!");
    } else {
        Logger::setVerbosity(Logger::Level::INFO);
    }

    if (!getenv("KOULOURI_ALLOW_ALSAWHINE")) {
        AlsaSilencer::supressAlsa();
    }

    if (file != nullptr) {
        AudioPlayer player;
        PlayerActionResult result = player.load(file, true);

        if (result.result == PlayerActionEnum::PASS) {
            player.setVolume(volume);
            PlayerActionResult play = player.play();

            while (!player.isCompleted()) {
                std::string out;
                std::cin >> out;
                if (out == "quit") {
                    player.stop();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            std::cout << "Failed to open file '" << file << "'" << ": " << result.getFriendly() << std::endl;
        }
    }

    // for (int i = 1; i < argc; ++i) {
    //     std::string arg = argv[i];
    //     std::cout << arg << std::endl;
    // }

    // auto st = ServerThread();
    // st.run();
    //
    //
    // std::cin.get();
    //
    // auto cdam = Daemon(Daemon::Mode::Client);
    //
    // Logger::g_log("koulouri_cli", Logger::Level::DEBUG, "Ready!");
    //
    //
    // cdam.send("CONN\n\n");
    // std::string response = cdam.receive().value_or("NONE");
    // std::cout << "server responded: " << escape_control_chars(response) << std::endl;
    // cdam.send("PLAY\n/home/exii/Music/STARSET/Starset - SILOS (pre-order)/Starset - SILOS - 03 SILOS.flac\n\n");
    // cdam.send("STAT\n\n");
    // cdam.send("SEEK\n1414320\n\n");
    //
    // // std::cout << sdam.receive().value_or("none") << std::endl;
    //
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    //
    // st.stop();

    return 0;
}

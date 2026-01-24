#include <atomic>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include <signal.h>
#include "libkoulouri/logger.h"
#include "libkoulouri/metahandler.h"
#include "libkoulouri/player.h"
#include "koulouri_shared/alsasilencer.h"
#include "koulouri_shared/cmdparser.h"

std::atomic<bool> running = true;

void handleSignal(int signal) {
    std::cout << "Recieved quit signal!" << std::endl;
    running.store(false);
    // exit(0); // Exit the program
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSignal);

    Logger::setOutput(&std::cerr);

    int volume = 70;

    CmdParser cmd;
    std::deque<std::string> queue;
    size_t queueIndex = 0;

    // cmd.register_argument({"", "", ArgType::SWITCH});
    cmd.register_argument({"-h", "--help", ArgType::SWITCH});
    cmd.register_argument({"-p", "--play", ArgType::APPEND});
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
        for (ArgResult &res : lst) {
            if (auto val = std::get_if<char*>(&res.value)) {
                queue.push_back(*val);
            }
        }

        // if (auto val = std::get_if<char*>(&res.value)) { // pass a reference to the value contained
        //     std::cout << "PLAY: " << *val << std::endl;
        //     file = *val; // we get a pointer back from get_if
        // }
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

    if (queue.size() > 0) {
        AudioPlayer player;

        while (running.load()) {
            if (queueIndex >= queue.size()) {
                Logger::g_log("cli", Logger::Level::INFO, "Reached end of queue!");
                break;
            }

            const std::string &file = queue.at(queueIndex);

            PlayerActionResult result = player.load(file, true);

            if (result.result == PlayerActionEnum::PASS) {
                player.setVolume(volume);
                PlayerActionResult play = player.play();
                const size_t maximumPos = player.getMaxPos(); // this doesn't change - get it once!

                while (!player.isCompleted() && running.load()) {
                    const size_t currentPos = player.getPos();

                    // TODO: find an efficent way to round up to 2nd decimal!

                    const std::string status = std::to_string(player.posToSeconds(player.getPos())) +
                    "(" + std::to_string(currentPos) + ") " +
                    std::to_string((static_cast<double>(currentPos) / maximumPos) * 100) + "% | " +
                    std::to_string(player.getVolume()) + "% ";
                    std::cout << "\r" << status;
                    std::cout.flush();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                std::cout << std::endl;
            } else {
                std::cout << "Failed to open file '" << file << "'" << ": " << result.getFriendly() << std::endl;
            }

            player.stop();
            queueIndex += 1;
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

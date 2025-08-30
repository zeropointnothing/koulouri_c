#include "qt_gui/qtmainwindow.h"
#include "ui_qtmainwindow.h"
#include "libkoulouri/player.h"
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <qmessagebox.h>
#include <random>
#include <sstream>
#include <QtConcurrent/QtConcurrent>

// using namespace QtGui;

std::string generateRandomString(size_t length) {
    const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[distribution(generator)];
    }
    return randomString;
}

QtMainWindow::QtMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QtMainWindow)
    , player(AudioPlayer())
    , PATH(getenv("KOULOURI_PLAYFILE") ? : "SET KOULOURI_PLAYFILE!!")
    , logger(Logger("frontend"))
{
    ui->setupUi(this);
    initializePlaybackUI();
}

QtMainWindow::~QtMainWindow()
{
    delete ui;
}


void QtMainWindow::setPlaybackState(PlaybackState state) {
    currentState = state;
    emit playbackStateUpdated(state);
};

void QtMainWindow::updateProgressBar() {
    int position = (static_cast<double>(player.getPos()) / player.getMaxPos()*100);

    if (position == 100) {
        stopPlayback();
        return;
    }

    // std::cout << std::to_string(position) << std::endl;
    ui->progressBar->setValue(position);
}

/**
 * @brief Sets up C++ <-> QT bindings to enable UI functionality.
 *
 * Most, if not all connections to UI elements should be handled here for clarity.
 */
void QtMainWindow::initializePlaybackUI() {
    logger.log(Logger::Level::INFO, "Getting ready...");

    progressUpdateTimer = new QTimer(this);
    connect(progressUpdateTimer, &QTimer::timeout, this, &QtMainWindow::updateProgressBar);

    connect(ui->startButton, &QPushButton::clicked, this, &QtMainWindow::startPlayback);
    connect(ui->playpauseButton, &QPushButton::clicked, this, &QtMainWindow::togglePlayback);
    connect(ui->stopButton, &QPushButton::clicked, this, &QtMainWindow::stopPlayback);

    connect(ui->volumeSlider, &QSlider::valueChanged, this, [this](){ player.setVolume(ui->volumeSlider->value()); });

    // connect(ui->songList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item){
    //     QWidget* widget = ui->songList->itemWidget(item);
    //     auto* songWidget = qobject_cast<SongItemWidget*>(widget);
    //     if (songWidget) {
    //         std::cout << "Song widget double clicked!" << std::endl;
    //     }
    // });

    // QPushButton test = QPushButton("hello, world!");
    // std::vector<Track> tracks = mhandler.loadTrackFromDirectory("/home/exii/Music");
    mhandler.populateMetaCache("/home/exii/Music", &mcache);

    // for (const auto &pair : mcache.getCache()){
    //         QListWidgetItem *item = new QListWidgetItem(ui->songList);
    //         SongItemWidget* songWidget = new SongItemWidget(this, generateRandomString(5).c_str());

    //         songWidget->setArtist(replaceBlank(pair.second.artist.c_str(), "..."));
    //         songWidget->setAlbum(replaceBlank(pair.second.album.c_str(), "..."));
    //         songWidget->setTitle(replaceBlank(pair.second.title.c_str(), "..."));

    //         item->setSizeHint(songWidget->sizeHint());
    //         ui->songList->addItem(item);
    //         ui->songList->setItemWidget(item, songWidget);
    // }

    // Connect player to UI via state system.
    connect(this, &QtMainWindow::playbackStateUpdated, this, [this](PlaybackState state) {
        switch (state) {
        case PlaybackState::Idle: // Base state - nothing to do here
        {break;}
        case PlaybackState::Paused: // Player has entered paused state - at time of writing, nothing to do
        {break;}

        case PlaybackState::Loading: // initial stage, loads file into memory
        {
            ui->progressBar->setRange(0,0);
            ui->playpauseButton->setEnabled(false);

            // Prevent GUI lockup during FFmpeg conversion/IO.
            // all further Qt calls should be made by the main thread with invokeMethod.
            QtConcurrent::run([this] {
                if (!player.isLoaded()) {
                    PlayerActionResult result = player.load(PATH, hasseen_conversionMessage);
                    if (result.result == PlayerActionEnum::NOTSUPPORTED) {
                        logger.log(Logger::Level::DEBUG, "prompting conversion...");
                        QMetaObject::invokeMethod(this, [this]{setPlaybackState(PlaybackState::Prompting_conversion);});
                        return;
                    } else if (result.result != PlayerActionEnum::PASS) {
                        QMetaObject::invokeMethod(this, [result, this] {
                            QMessageBox msg;
                            std::string error = "Koulouri was unable to open the file '" +PATH + "' ("+result.getFriendly()+")";
                            msg.critical(nullptr, "Failed to open file!", error.c_str());

                            setPlaybackState(PlaybackState::Aborted);
                        });
                        return;
                    } else {
                        QMetaObject::invokeMethod(this, [this]{setPlaybackState(PlaybackState::Ready);});
                        return;
                    }
                }
            });
            break;
        }

        case PlaybackState::Prompting_conversion: // ask the user if they would like to enable FFmpeg conversion
        {
            QMessageBox msg;
            std::string info = "Koulouri couldn't open this file, as it's in an unsupported format.\n"
                               "If FFmpeg is installed on your system, the file can be converted into a format Koulouri does support!\n"
                               "\nWould you like to enable conversions? Please note that this rarely can cause audio quality issues, especially if your file has an odd format."
                               "\n(Your response will be remembered for the next unsupported file!)";
            msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Help);
            msg.setDefaultButton(QMessageBox::Yes);
            msg.setText(info.c_str());
            msg.setWindowTitle("Failed to open file!");
            msg.setIcon(QMessageBox::Information);
            int ret = msg.exec();

            if (ret == QMessageBox::No || ret == QMessageBox::Cancel) {
                setPlaybackState(PlaybackState::Aborted);
                return; // user aborted - we shouldn't continue!
            } else if (ret == QMessageBox::Help) {
                QDesktopServices::openUrl(QUrl("https://github.com/zeropointnothing/koulouri_c"));
                setPlaybackState(PlaybackState::Aborted);
                return; // user is being redirected, we should assume they dont want Koulouri doing things without them.
            }
            hasseen_conversionMessage = true;
            setPlaybackState(PlaybackState::Loading);
            // result = player.load(PATH, true);
            break;
        }

        case PlaybackState::Ready: // final stage before playing - attempt to call .play
        {
            player.setVolume(ui->volumeSlider->value());
            // player.setPos(player.getMaxPos()-800000);
            PlayerActionResult result = player.play();

            if (result.result != PlayerActionEnum::PASS) {
                QMessageBox msg;
                std::string error = "Koulouri was unable start playback... ("+result.getFriendly()+")";
                msg.critical(0, "Failed to start playback!", error.c_str());

                setPlaybackState(PlaybackState::Aborted);
                return;
            }

            setPlaybackState(PlaybackState::Playing);
            break;
        }
        case PlaybackState::Playing: // audio has begun playback - update UI to reflect this
        {
            ui->startButton->setDisabled(true);
            ui->stopButton->setDisabled(false);
            ui->playpauseButton->setDisabled(false);
            ui->songLabel->setText(("Now playing: " + PATH).c_str());

            ui->progressBar->setRange(0,100);
            progressUpdateTimer->start(100);
            break;
        }

        case PlaybackState::Aborted: // reset state - does not always signify a failure, and may just be a natural stop
        {
            player.stop();
            ui->progressBar->setRange(0,100);
            ui->progressBar->setValue(0);
            ui->startButton->setDisabled(false);
            ui->stopButton->setDisabled(true);
            ui->playpauseButton->setDisabled(true);

            if (progressUpdateTimer->isActive()) {
                progressUpdateTimer->stop();
            }
            setPlaybackState(PlaybackState::Idle);
        }

        }
    });
}

void QtMainWindow::stopPlayback() {
    setPlaybackState(PlaybackState::Aborted); // let our switch/case handle cleanup
}

void QtMainWindow::togglePlayback() {

    // can't switch states of nothing is playing.
    if (!player.isLoaded()) {
        return;
    }

    bool playing = player.isPlaying();
    playing ? player.pause() : player.resume();
    std::stringstream ss;
    ss << "Changed state from '" << (playing ? "Playing" : "Paused") << "' to '" << (!playing ? "Playing" : "Paused") << "'!";
    logger.log(Logger::Level::DEBUG, ss.str());
}

void QtMainWindow::startPlayback() {
    if (currentState == PlaybackState::Idle) {
        setPlaybackState(PlaybackState::Loading);
    } else {
        std::cerr << "cannot switch state to 'Loading', as currentState is not 'Idle'!" << std::endl;;
    }
}

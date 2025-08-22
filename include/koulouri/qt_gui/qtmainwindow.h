#ifndef QtMainWindow_H
#define QtMainWindow_H

#include "libkoulouri/metahandler.h"
#include "libkoulouri/player.h"
#include <QMainWindow>
#include <qtimer.h>

enum class PlaybackState {
    Idle, // Waiting for user input (startup)
    Loading, // Loading data into internal buffers
    Prompting_conversion,
    Ready, // Data loaded, waiting to be played
    Playing, // Data loaded and is being played to user
    Paused, // Data loaded, but is currently paused
    Aborted // General flow break, regardless of failure - should reset to Idle
};

QT_BEGIN_NAMESPACE
namespace Ui {
    class QtMainWindow;
}
QT_END_NAMESPACE

class QtMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtMainWindow(QWidget *parent = nullptr);
    AudioPlayer player;
    MetaHandler mhandler;
    MetaCache mcache;
    const std::string PATH;
    bool hasseen_conversionMessage = false;

    void initializePlaybackUI();
    void updateProgressBar();
    QTimer *progressUpdateTimer;

    ~QtMainWindow();

    signals:
        void playbackStateUpdated(PlaybackState state);

private slots:
    void startPlayback();
    void stopPlayback();
    void togglePlayback();

private:
    void setPlaybackState(PlaybackState state);
    PlaybackState currentState = PlaybackState::Idle;

    Ui::QtMainWindow *ui;
};

#endif // QtMainWindow_H

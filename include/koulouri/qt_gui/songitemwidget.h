#ifndef SONGITEMWIDGET_H
#define SONGITEMWIDGET_H

#include <QWidget>

namespace Ui {
class SongListItem;
}

class SongItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SongItemWidget(QWidget *parent = nullptr, QString trackId = "");
    ~SongItemWidget();

    void setTitle(const QString &value);
    void setArtist(const QString &value);
    void setAlbum(const QString &value);


    void initializeUI();

private:
    Ui::SongListItem *ui;

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
};

#endif // SONGITEMWIDGET_H

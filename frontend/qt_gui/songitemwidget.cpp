#include "../../include/songitemwidget.h"
#include "ui_songitemwidget.h"
#include <iostream>

SongItemWidget::SongItemWidget(QWidget *parent, QString trackId)
    : QWidget(parent)
    , ui(new Ui::SongListItem)
{
    ui->setupUi(this);
    setProperty("trackId", trackId);
    initializeUI();
}

SongItemWidget::~SongItemWidget()
{
    delete ui;
}

void SongItemWidget::initializeUI() {
    ui->buttonContainer->setVisible(false);

    connect(ui->atnButton, &QPushButton::clicked, this, [this]() {
        std::cout << "AddToNext clicked! " << this->property("trackId").toString().toStdString() << std::endl;
    });
    connect(ui->atqButton, &QPushButton::clicked, this, [this]() {
        std::cout << "AddToQueue clicked! " << this->property("trackId").toString().toStdString() << std::endl;
    });
    connect(ui->infButton, &QPushButton::clicked, this, [this]() {
        std::cout << "Info clicked! " << this->property("trackId").toString().toStdString() << std::endl;
    });
}

void SongItemWidget::setTitle(const QString &value) {
    ui->songTitleLabel->setText(value);
};
void SongItemWidget::setArtist(const QString &value) {
    ui->songArtistLabel->setText(value);
};
void SongItemWidget::setAlbum(const QString &value) {
    ui->songAlbumLabel->setText(value);
};


void SongItemWidget::enterEvent(QEvent* event) {
    // std::cout << "enter" << std::endl;
    ui->buttonContainer->setVisible(true);
}

void SongItemWidget::leaveEvent(QEvent* event) {
    // std::cout << "leave" << std::endl;
    ui->buttonContainer->setVisible(false);
}

#include <stdint.h>

#include <QtEndian>

#include "viewer.h"
#include "ui_viewer.h"

Viewer::Viewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Viewer),
    enabled(false)
{
    ui->setupUi(this);
    on_timeoutSlider_valueChanged(ui->timeoutSlider->value());
}

Viewer::~Viewer()
{
    delete ui;
}

void Viewer::message(QString message)
{
    ui->statusLabel->setText(message);
    ui->statusLabel->setStyleSheet("color: black; font-weight: normal;");
}

void Viewer::error(const char *message, int error)
{
    ui->statusLabel->setText(QString("%1 (%2)").arg(message).arg(error));
    ui->statusLabel->setStyleSheet("color: red; font-weight: bold;");
}

void Viewer::on_scanButton_clicked()
{
    int timeout = ui->timeoutSlider->value();

    if (timeout == ui->timeoutSlider->maximum())
        timeout = -1;
    else
        timeout *= 1000;

    Fingerprint *fingerprint;
    try {
        fingerprint = scanner->getFingerprint(timeout);
    } catch (ScannerTimeout &e) {
        message("Timeout...");
        return;
    } catch (ScannerException &e) {
        error(e.what(), e.code());
        return;
    }

    QString status;
    scene.clear();
    ui->templateText->clear();

    if (fingerprint->image) {
        QImage *image = fingerprint->image->getImage();
        pixmap = QPixmap::fromImage(*image);
        scene.addPixmap(pixmap);
        scene.setSceneRect(pixmap.rect());
        scene.addRect(pixmap.rect(), QPen(Qt::blue), QBrush());
        message(QString("Obtained %1x%2 pixels large image").arg(image->width()).arg(image->height()));
        delete image;
    }

    if (fingerprint->minutiaeRecord) {
        ui->templateText->setPlainText(fingerprint->minutiaeRecord->getRecord());

        if (fingerprint->image) {
            for (std::list<FingerprintMinutia>::iterator m = fingerprint->minutiaeRecord->minutiae.begin(); m != fingerprint->minutiaeRecord->minutiae.end(); m++) {
                scene.addRect(m->x - 1, m->y - 1, 3, 3, QPen(Qt::red), QBrush());
                scene.addRect(m->x - 2, m->y - 2, 5, 5, QPen(Qt::red), QBrush());
            }
        }
    }

    ui->imageView->setScene(&scene);

    delete fingerprint;
}

void Viewer::on_onOffButton_clicked()
{
    if (!enabled) {
        try {
            scanner = new Scanner();
            message(QString("Turned on scanner '%1'").arg(scanner->getName()));
            enabled = true;
        } catch (ScannerException &e) {
            error(e.what(), e.code());
        }
    } else {
        delete scanner;
        ui->templateText->clear();
        scene.clear();
        message("Turned off scanner");
        enabled = false;
    }

    ui->scanButton->setEnabled(enabled);
    ui->timeoutSlider->setEnabled(enabled);
    ui->onOffButton->setText(enabled ? "Turn off" : "Turn on");
}

void Viewer::on_timeoutSlider_valueChanged(int value)
{
    QString timeout;

    if (value == ui->timeoutSlider->maximum())
        timeout = QString("infinite");
    else if (value == 0)
        timeout = QString("none");
    else if (value == 1)
        timeout = QString("1 second");
    else
        timeout = QString("%1 seconds").arg(value);

    ui->timeoutLabel->setText(QString("Timeout: %1").arg(timeout));
}

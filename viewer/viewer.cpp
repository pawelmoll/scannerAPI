#include <stdint.h>

#include <QtConcurrent>
#include <QException>
#include <QFuture>
#include <QGraphicsRectItem>

#include "viewer.h"
#include "ui_viewer.h"

Viewer::Viewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Viewer),
    enabled(false)
{
    ui->setupUi(this);
    connect(&scanner_watcher, SIGNAL(finished()), this, SLOT(scannerFinished()));
    on_timeoutSlider_valueChanged(ui->timeoutSlider->value());

    ScannersList &scanners_list = ScannersList::getScannersList();
    for (std::vector<const char *>::iterator name = scanners_list.begin();
         name != scanners_list.end(); name++) {
        ui->scannersList->addItem(QString(*name));
    }
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
    if (error)
        ui->statusLabel->setText(QString("%1 (%2)").arg(message).arg(error));
    else
        ui->statusLabel->setText(QString(message));
    ui->statusLabel->setStyleSheet("color: red; font-weight: bold;");
}

class ViewerScannerException : public QException
{
    const char *message;
    int error;

public:
    void raise() const { throw *this; }
    ViewerScannerException *clone() const { return new ViewerScannerException(*this); }

    int code() { return error; }
    const char* what() { return message; }

    ViewerScannerException(const char *message, int error = 0) : message(message), error(error) {}
};

Fingerprint *Viewer::scanStart(int timeout)
{
    Fingerprint *fingerprint = NULL;
    try {
        fingerprint = scanner->getFingerprint(timeout);
    } catch (ScannerException &e) {
        throw ViewerScannerException(e.what(), e.code());
    }

    return fingerprint;
}

void Viewer::scannerFinished()
{
    Fingerprint *fingerprint = NULL;

    try {
        fingerprint = scanner_watcher.result();
    } catch (ViewerScannerException &e) {
        error(e.what(), e.code());
    }

    if (fingerprint && fingerprint->image) {
        QImage *image = fingerprint->image->getImage();
        pixmap = QPixmap::fromImage(*image);
        scene.addPixmap(pixmap);
        scene.setSceneRect(pixmap.rect());
        scene.addRect(pixmap.rect(), QPen(Qt::blue), QBrush());
        message(QString("Obtained %1x%2 pixels large image").arg(image->width()).arg(image->height()));
        delete image;
    }

    if (fingerprint && fingerprint->minutiaeRecord) {
        ui->templateText->setPlainText(fingerprint->minutiaeRecord->getRecord());

        if (fingerprint->image) {
            for (std::list<FingerprintMinutia>::iterator m = fingerprint->minutiaeRecord->minutiae.begin();
                 m != fingerprint->minutiaeRecord->minutiae.end(); m++) {
                QString label = QString("Minutia %1").arg(std::distance(fingerprint->minutiaeRecord->minutiae.begin(), m));
                QGraphicsRectItem *rect = scene.addRect(m->x - 1, m->y - 1, 3, 3, QPen(Qt::red), QBrush());
                rect->setToolTip(label);
                rect = scene.addRect(m->x - 2, m->y - 2, 5, 5, QPen(Qt::red), QBrush());
                rect->setToolTip(label);
            }
        }
    }

    ui->imageView->setScene(&scene);

    delete fingerprint;

    ui->scanButton->setEnabled(true);
    ui->onOffButton->setEnabled(true);
    ui->timeoutSlider->setEnabled(true);
}

void Viewer::on_scanButton_clicked()
{
    int timeout = ui->timeoutSlider->value();

    if (timeout == ui->timeoutSlider->maximum())
        timeout = -1;
    else
        timeout *= 1000;

    scene.clear();
    ui->templateText->clear();

    ui->onOffButton->setEnabled(false);
    ui->timeoutSlider->setEnabled(false);
    ui->scanButton->setEnabled(false);
    QFuture<Fingerprint *> future = QtConcurrent::run(this, &Viewer::scanStart, timeout);
    scanner_watcher.setFuture(future);
    message("Scanning...");
}

void Viewer::on_onOffButton_clicked()
{
    if (!enabled) {
        try {
            ScannersList &scanners_list = ScannersList::getScannersList();
            scanner = new Scanner(scanners_list[ui->scannersList->currentIndex()]);
            message(QString("Turned on scanner '%1'").arg(scanner->getName()));
            enabled = true;
        } catch (ScannerException &e) {
            error(e.what(), e.code());
        }
    } else {
        message(QString("Turned off scanner '%1'").arg(scanner->getName()));
        delete scanner;
        ui->templateText->clear();
        scene.clear();
        enabled = false;
    }

    ui->scannersList->setEnabled(!enabled);
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

#include <iostream>
#include <stdint.h>

#include <QtConcurrent>
#include <QException>
#include <QFileDialog>
#include <QFuture>
#include <QGraphicsRectItem>

#include "viewer.h"
#include "ui_viewer.h"

Viewer::Viewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Viewer),
    enabled(false),
    highlight(NULL),
    fingerprint(NULL)
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
    delete fingerprint;
}

void Viewer::highlightMinutia(int view, int minutia)
{
    if (highlight) {
            scene.removeItem(highlight);
            highlight = NULL;
    }
    if (view == 0 && minutia >= 0) {
        std::list<FingerprintMinutia>::iterator m = fingerprint->minutiaeRecord->minutiae.begin();
        std::advance(m, minutia);
        highlight = scene.addRect(m->x - 12, m->y - 12, 23, 23, QPen(QColor(0, 0xff, 0, 0xb0), 4), QBrush());
    }
}

void Viewer::scrollToMinutia(int view, int minutia)
{
    ui->templateText->scrollToAnchor(QString("view%1minutia%2").arg(view).arg(minutia));
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

class MinutiaGraphicsItem : public QGraphicsItem
{
private:
    Viewer *viewer;
    int view, minutia;
    int x, y, angle;

public:
    MinutiaGraphicsItem(Viewer *viewer, int view, int minutia, int x, int y, int angle) :
        viewer(viewer), view(view), minutia(minutia), x(x), y(y), angle(angle) {}
    QRectF boundingRect() const
    {
        return QRectF(x - 10, y - 10, 20, 20);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    {
         QPen pen(QColor(0xff, 0, 0, 0xb0), 2);
         painter->setPen(pen);

         painter->drawEllipse(x - 3, y - 3, 6, 6);

         QLineF line;
         line.setP1(QPointF(x, y));
         line.setAngle(1.0 * angle / 100000);
         line.setLength(10);
         painter->drawLine(line);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        viewer->highlightMinutia(view, minutia);
        viewer->scrollToMinutia(view, minutia);
        QGraphicsItem::mousePressEvent(event);
    }
};

void Viewer::scannerFinished()
{
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

        ui->saveImageButton->setEnabled(true);
    }

    if (fingerprint && fingerprint->minutiaeRecord) {
        ui->templateText->setHtml(fingerprint->minutiaeRecord->getHtml());

        if (fingerprint->image) {
            for (std::list<FingerprintMinutia>::iterator m = fingerprint->minutiaeRecord->minutiae.begin();
                 m != fingerprint->minutiaeRecord->minutiae.end(); m++) {
                int index = std::distance(fingerprint->minutiaeRecord->minutiae.begin(), m);
                MinutiaGraphicsItem *minutae = new MinutiaGraphicsItem(this, 0, index, m->x, m->y, m->angle);
                minutae->setToolTip(QString("Minutia %1").arg(index));
                scene.addItem(minutae);
            }
        }

        ui->saveFMRButton->setEnabled(true);
    }

    ui->imageView->setScene(&scene);

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
    ui->saveImageButton->setEnabled(false);
    ui->saveFMRButton->setEnabled(false);

    ui->onOffButton->setEnabled(false);
    ui->timeoutSlider->setEnabled(false);
    ui->scanButton->setEnabled(false);

    delete fingerprint;
    fingerprint = NULL;

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
        ui->saveImageButton->setEnabled(false);
        ui->saveFMRButton->setEnabled(false);
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

void Viewer::on_saveFMRButton_clicked()
{
    QString filter;
    QString name = QFileDialog::getSaveFileName(this, tr("Save FMR as..."), "", tr("Fingerprint Minutiae Records (*.fmr);;All Files (*)"), &filter);
    if (!name.isEmpty()) {
        if (filter.contains("fmr") && !name.endsWith(".fmr", Qt::CaseInsensitive))
            name += ".fmr";
        QFile file(name);
        file.open(QIODevice::WriteOnly);
        file.write((const char *)fingerprint->minutiaeRecord->getBinary(), fingerprint->minutiaeRecord->getSize());
        file.close();
        message(QString("%1 saved").arg(name));
    }
}

void Viewer::on_saveImageButton_clicked()
{
    QString filter;
    QString name = QFileDialog::getSaveFileName(this, tr("Save FMR as..."), "", tr("PNG Images (*.png);;JPEG Images (*.jpg);;BMP Images (*.bmp);;All Files (*)"), &filter);
    if (!name.isEmpty()) {
        if (filter.contains("png") && !name.endsWith(".png", Qt::CaseInsensitive))
            name += ".png";
        else if (filter.contains("jpg") && !name.endsWith(".jpg", Qt::CaseInsensitive) && !name.endsWith(".jpeg", Qt::CaseInsensitive))
            name += ".jpg";
        else if (filter.contains("bmp") && !name.endsWith(".bmp", Qt::CaseInsensitive))
            name += ".bmp";
        QFile file(name);
        file.open(QIODevice::WriteOnly);
        pixmap.save(&file);
        file.close();
        message(QString("%1 saved").arg(name));
    }
}

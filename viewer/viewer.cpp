#include "viewer.h"
#include "ui_viewer.h"
#include "../scanner.h"

Viewer::Viewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Viewer)
{
    ui->setupUi(this);

    for (int i = 0; i < 256; i++)
        grayScale.append(qRgb(i, i, i));
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

QString Viewer::showImage(void)
{
    int size = scanner_get_image(NULL, 0);

    if (size < 0)
        return QString("Zero sized image");

    unsigned char *buffer = (unsigned char *)malloc(size);
    if (!buffer)
        return QString("No memory for image");

    size = scanner_get_image((void *)buffer, size);
    if (size < 0)
        return QString("Failed to obtain the image (%1)").arg(size);

    scene.clear();

    switch (caps.image_format) {
    case scanner_caps::scanner_image_gray_8bit:
        image = QImage(buffer, caps.image_width, caps.image_height, QImage::Format_Indexed8);
        image.setColorTable(grayScale);

        break;
    default:
        return QString("Obtained unknown image format (%1)").arg(caps.image_format);
    }

    pixmap = QPixmap::fromImage(image);
    scene.addPixmap(pixmap);
    scene.setSceneRect(pixmap.rect());
    scene.addRect(pixmap.rect(), QPen(Qt::blue), QBrush());
    ui->imageView->setScene(&scene);

    free(buffer);

    return QString("Obtained %1 bytes big image, %2x%3 pixels").arg(size).arg(caps.image_width).arg(caps.image_height);
}

QString Viewer::showTemplate(void)
{
    int size = scanner_get_iso_template(NULL, 0);

    if (size < 0)
        return QString("Zero sized template");

    unsigned char *buffer = (unsigned char *)malloc(size);
    if (!buffer)
        return QString("No memory for template");

    size = scanner_get_iso_template((void *)buffer, size);
    if (size < 0)
        return QString("Failed to obtain the template (%1)").arg(size);

    QString hex;
    for (int i = 0; i < size; i++)
        hex.append(QString::number(buffer[i], 16).rightJustified(2, '0')).append(' ');

    ui->templateText->setPlainText(hex);

    free(buffer);

    return QString("Obtained %1 bytes big template").arg(size);
}

void Viewer::on_scanButton_clicked()
{
    int err = scanner_scan(ui->timeoutSlider->value() * 1000);

    if (err == -1) {
        message("Timeout...");
        return;
    } else if (err) {
        error("Failed to scan", err);
        return;
    }

    ui->templateText->clear();

    QString status;

    if (caps.image) {
        status = showImage();
        status.append(" / ");
    }

    if (caps.iso_template)
        status.append(showTemplate());

    message(status);
}

void Viewer::on_onOffButton_clicked()
{
    if (!enabled) {
        int err = scanner_on();
        if (err) {
            error("Failed to turn on the scanner", err);
            return;
        }
        err = scanner_get_caps(&caps);
        if (err) {
            error("Failed to obtaing caps", err);
            return;
        }
        message(QString("Turned on scanner '%1'").arg(caps.name));
        enabled = true;
    } else {
        scanner_off();
        ui->templateText->clear();
        scene.clear();
        message("Turned off scanner");
        enabled = false;
    }

    ui->scanButton->setEnabled(enabled);
    ui->timeoutSlider->setEnabled(enabled);
    ui->onOffButton->setText(enabled ? "Turn off" : "Turn on");
}

void Viewer::on_timeoutSlider_sliderMoved(int position)
{
    QString timeout;

    if (position == ui->timeoutSlider->maximum())
        timeout = QString("infinite");
    else if (position == 0)
        timeout = QString("none");
    else if (position == 1)
        timeout = QString("1 second");
    else
        timeout = QString("%1 seconds").arg(position);

    ui->timeoutLabel->setText(QString("Timeout: %1").arg(timeout));
}

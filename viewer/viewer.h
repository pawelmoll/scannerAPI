#ifndef VIEWER_H
#define VIEWER_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "../scanner.h"

namespace Ui {
class Viewer;
}

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit Viewer(QWidget *parent = 0);
    ~Viewer();

private slots:
    void on_scanButton_clicked();

    void on_onOffButton_clicked();

    void on_timeoutSlider_sliderMoved(int position);

private:
    Ui::Viewer *ui;

    bool enabled;
    struct scanner_caps caps;

    void message(QString message);
    void error(const char *message, int error);

    QImage image;
    QGraphicsScene scene;
    QPixmap pixmap;
    QVector<QRgb> grayScale;
    QVector<QRgb> grayScaleInverted;

    QString showImage(void);
    QString showTemplate(void);
    void drawMinutiae(void *buffer);
};

#endif // VIEWER_H

#ifndef VIEWER_H
#define VIEWER_H

#include <QFutureWatcher>
#include <QMainWindow>
#include <QGraphicsScene>
#include "scanner.h"

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

    void on_timeoutSlider_valueChanged(int position);

    void scannerFinished();

private:
    Ui::Viewer *ui;

    bool enabled;
    QGraphicsScene scene;
    QPixmap pixmap;

    void message(QString message);
    void error(const char *message, int error);

    Fingerprint *scanStart(int timeout);
    QFutureWatcher<Fingerprint *> scanner_watcher;
    Scanner *scanner;
};

#endif // VIEWER_H

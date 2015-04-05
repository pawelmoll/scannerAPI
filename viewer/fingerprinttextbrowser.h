#ifndef VIEWERTEXTBROWSER_H
#define VIEWERTEXTBROWSER_H

#include <QTextBrowser>

class FingerprintTextBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    FingerprintTextBrowser(QWidget *widget) : QTextBrowser(widget) {}

    bool event(QEvent *e);
    void setSource(const QUrl &url);
};

#endif // VIEWERTEXTBROWSER_H

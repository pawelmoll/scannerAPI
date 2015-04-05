#include <iostream>

#include <QHelpEvent>
#include <QEvent>
#include <QUrl>
#include <QDesktopServices>
#include <QTextStream>

#include "fingerprinttextbrowser.h"
#include "viewer.h"

bool FingerprintTextBrowser::event(QEvent *event)
{
    Viewer *viewer = (Viewer *)window();
    switch (event->type()) {
    default:
        break;
    case QEvent::Leave:
    case QEvent::Enter:
        viewer->highlightMinutia(-1, -1);
        break;
    }
    return QTextBrowser::event(event);
}

void FingerprintTextBrowser::setSource(const QUrl &url)
{
    Viewer *viewer = (Viewer *)window();
    if (url.scheme() == "minutia") {
        viewer->highlightMinutia(url.port(), url.fileName().toInt());
    } else {
        QDesktopServices::openUrl(url);
    }
}

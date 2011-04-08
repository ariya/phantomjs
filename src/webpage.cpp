#include <iostream>
#include <QApplication>

#include "webpage.h"

// public:
WebPage::WebPage(QObject *parent)
    : QWebPage(parent)
{
    m_userAgent = QWebPage::userAgentForUrl(QUrl());
    connect(this->currentFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(handleFrameUrlChanged(QUrl)));
    connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(handleLinkClicked(QUrl)));
}

// public slots:
bool WebPage::shouldInterruptJavaScript()
{
    QApplication::processEvents(QEventLoop::AllEvents, 42);
    return false;
}

// private slots:
void WebPage::handleFrameUrlChanged(const QUrl &url) {
    qDebug() << "URL Changed: " << qPrintable(url.toString());
}

void WebPage::handleLinkClicked(const QUrl &url) {
    qDebug() << "URL Clicked: " << qPrintable(url.toString());
}

// protected:
void WebPage::javaScriptAlert(QWebFrame *originatingFrame, const QString &msg)
{
    Q_UNUSED(originatingFrame);
    std::cout << "JavaScript alert: " << qPrintable(msg) << std::endl;
}

void WebPage::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
    if (!sourceID.isEmpty())
        std::cout << qPrintable(sourceID) << ":" << lineNumber << " ";
    std::cout << qPrintable(message) << std::endl;
}

QString WebPage::userAgentForUrl(const QUrl &url) const
{
    Q_UNUSED(url);
    return m_userAgent;
}

QString WebPage::chooseFile(QWebFrame *parentFrame, const QString &suggestedFile)
{
    Q_UNUSED(parentFrame);
    Q_UNUSED(suggestedFile);
    if (m_allowedFiles.contains(m_nextFileTag))
        return m_allowedFiles.value(m_nextFileTag);
    return QString();
}

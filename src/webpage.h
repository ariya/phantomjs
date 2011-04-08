#include <QtWebKit>

class WebPage: public QWebPage
{
    Q_OBJECT
public:
    WebPage(QObject *parent = 0);

public slots:
    bool shouldInterruptJavaScript();

private slots:
    void handleFrameUrlChanged(const QUrl &url);
    void handleLinkClicked(const QUrl &url);

protected:
    void javaScriptAlert(QWebFrame *originatingFrame, const QString &msg);
    void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID);
    QString userAgentForUrl(const QUrl &url) const;
    QString chooseFile(QWebFrame * parentFrame, const QString & suggestedFile);

private:
    QString m_userAgent;
    QMap<QString, QString> m_allowedFiles;
    QString m_nextFileTag;
    friend class Phantom;
};

#include <QtGui>

#include "webpage.h"
#include "csconverter.h"
#include "networkaccessmanager.h"

class Phantom: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList args READ args)
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(QString loadStatus READ loadStatus)
    Q_PROPERTY(QString state READ state WRITE setState)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent)
    Q_PROPERTY(QVariantMap version READ version)
    Q_PROPERTY(QVariantMap viewportSize READ viewportSize WRITE setViewportSize)
    Q_PROPERTY(QVariantMap paperSize READ paperSize WRITE setPaperSize)
    Q_PROPERTY(QVariantMap clipRect READ clipRect WRITE setClipRect)

public:
    Phantom(QObject *parent = 0);

    QStringList args() const;

    QString content() const;
    void setContent(const QString &content);

    bool execute();
    int returnValue() const;

    QString loadStatus() const;

    void setState(const QString &value);
    QString state() const;

    void setUserAgent(const QString &ua);
    QString userAgent() const;

    QVariantMap version() const;

    void setViewportSize(const QVariantMap &size);
    QVariantMap viewportSize() const;

    void setClipRect(const QVariantMap &size);
    QVariantMap clipRect() const;

    void setPaperSize(const QVariantMap &size);
    QVariantMap paperSize() const;

public slots:
    void exit(int code = 0);
    void open(const QString &address);
    void setFormInputFile(QWebElement el, const QString &fileTag);
    void simulateMouseClick(const QString &selector);
    bool loadJs(const QString &jsFilePath);
    void includeJs(const QString &jsFilePath, const QString &callback = "undefined");
    bool render(const QString &fileName);
    void sleep(int ms);

private slots:
    void inject();
    void finish(bool);
    bool renderPdf(const QString &fileName);

private:
    static qreal stringToPointSize(const QString &string);

    QString m_scriptFile;
    QStringList m_args;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_loadStatus;
    WebPage m_page;
    int m_returnValue;
    QString m_script;
    QString m_state;
    CSConverter *m_converter;
    QVariantMap m_paperSize; // For PDF output via render()
    QRect m_clipRect;
    NetworkAccessManager *m_netAccessMan;
};

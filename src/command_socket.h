#ifndef COMMAND_SOCKET_H
#define COMMAND_SOCKET_H

#include <QObject>
#include <QWebFrame>
#include <QLocalServer.h>
#include <QLocalSocket.h>
#include "phantom.h"
#include "terminal.h"
#include "webpage.h"
#include "consts.h"

class CommandSocket: public QObject
{
    Q_OBJECT

public:
    CommandSocket(QWebFrame*, Phantom* const, QString);
    void exit();

private slots:
    void handleConnection();
    void readCommand();
    void loadFinished(const QString &status);

private:
    void handlePostCommand(QByteArray);
    void handleGetCommand(QByteArray);
    QWebFrame *m_webframe;
    Phantom *m_parentPhantom;
    QLocalServer *m_server;
    bool m_looping;
    Terminal *m_terminal;
    QLocalSocket *m_clientConnection;
    WebPage *m_page;
    QVariantMap m_settings;
};


#endif // COMMAND_SOCKET_H

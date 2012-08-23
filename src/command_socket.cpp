#include "command_socket.h"

#include <stdio.h>
#include <QByteArray.h>
#include <QNetworkCookieJar.h>

CommandSocket::CommandSocket(QWebFrame *webframe, Phantom *parent)
    : QObject(parent)
{
    m_webframe = webframe;
    m_parentPhantom = parent;
    m_terminal = Terminal::instance();

    m_server = new QLocalServer(this);
    if (!m_server->listen("/tmp/phantomjs.socket")) {
        m_terminal->cerr("Not able to start the server", true);
        exit();
    }
    connect(m_server, SIGNAL(newConnection()), this, SLOT(handleConnection()));

    m_page = new WebPage(this);
    connect(m_page, SIGNAL(loadFinished(const QString)), this, SLOT(loadFinished(const QString)));

    m_terminal->cout("Command server ready", true);
}

void CommandSocket::handleConnection() {
    m_clientConnection = m_server->nextPendingConnection();
    m_clientConnection->write("hello there\r\n");

    connect(m_clientConnection, SIGNAL(readyRead()), this, SLOT(readCommand()));
}


void CommandSocket::readCommand() {
    bool shutdown = false;
    if (m_clientConnection->isValid()) {
        if (m_clientConnection->canReadLine()) {
            QByteArray message(m_clientConnection->readLine());
            message = message.simplified();

            if (message.startsWith("load ")) {
                QByteArray url = message.replace("load ", "").simplified();
                m_page->openUrl(url, "get", m_settings);
            } else if (message.startsWith("render ")) {
                QByteArray path = message.replace("render ", "").simplified();
                bool success = m_page->render(path);
                if (success) {
                    m_clientConnection->write("+ ok\r\n");
                } else {
                    m_clientConnection->write("no dice\r\n");
                }

            } else if (message.startsWith("show cookies for ")) {
                QByteArray address = message.replace("show cookies for ", "").simplified();
                QUrl url = QUrl::fromEncoded(address);
                QNetworkCookieJar *cookie_jar = m_page->cookieJar();
                QList<QNetworkCookie> cookies = cookie_jar->cookiesForUrl(url);

                QString resp;
                QString cookie = cookies.first().toRawForm();
                QTextStream(&resp) << "+ " << cookies.count() << "cookies: " << cookie << "\r\n";
                //QTextStream(&resp) << "+ " << cookies.count() << " cookies\r\n";
                m_clientConnection->write(resp.toAscii());

            } else if (message == "set images on") {
                m_settings[PAGE_SETTINGS_LOAD_IMAGES] = true;
                m_clientConnection->write("+ ok\r\n");
            } else if (message == "set images off") {
                m_settings[PAGE_SETTINGS_LOAD_IMAGES] = false;
                m_clientConnection->write("+ ok\r\n");
            } else if (message == "set js on") {
                m_settings[PAGE_SETTINGS_JS_ENABLED] = true;
                m_clientConnection->write("+ ok\r\n");
            } else if (message == "set js off") {
                m_settings[PAGE_SETTINGS_JS_ENABLED] = false;
                m_clientConnection->write("+ ok\r\n");

            } else if (message == "show settings") {
                m_clientConnection->write("+ settings:\r\n");
                if (m_settings[PAGE_SETTINGS_LOAD_IMAGES].toBool()) {
                    m_clientConnection->write("  images on\r\n");
                } else {
                    m_clientConnection->write("  images off\r\n");
                }
                if (m_settings[PAGE_SETTINGS_JS_ENABLED].toBool()) {
                    m_clientConnection->write("  js on\r\n");
                } else {
                    m_clientConnection->write("  js off\r\n");
                }
                m_clientConnection->write(".\r\n");



            } else if (message == "shutdown") {
                m_clientConnection->write("shutting down\r\n");
                m_clientConnection->flush();
                shutdown = true;
            } else if (message == "quit") {
                m_clientConnection->write("bye bye\r\n");
                m_clientConnection->close();
            } else {
                QString resp;
                QTextStream(&resp) << "! unrecognised command: " << message << "\r\n";
                m_clientConnection->write(resp.toAscii());
            }
        }
    }

    if (shutdown) {
        exit();
    }
}

void CommandSocket::loadFinished(const QString &status)
{
    if (status == "success") {
        m_clientConnection->write("+ page loaded ok\r\n");
    } else if (status == "fail") {
        m_clientConnection->write("- failed to load page\r\n");
    } else {
        QString resp;
        QTextStream(&resp) << "! unknown status: " << status << "\r\n";
        m_clientConnection->write(resp.toAscii());
    }
}

void CommandSocket::exit()
{
    m_server->close();
    m_parentPhantom->exit();
}

#include "command_socket.h"

#include <QByteArray.h>
#include <QNetworkCookieJar.h>
#include <QDateTime.h>

CommandSocket::CommandSocket(QWebFrame *webframe, Phantom *parent, QString socketFileName)
    : QObject(parent)
{
    m_webframe = webframe;
    m_parentPhantom = parent;
    m_terminal = Terminal::instance();

    m_server = new QLocalServer(this);
    if (!m_server->listen(socketFileName)) {
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

void CommandSocket::handlePostCommand(QByteArray message){
    // post command: post <url> <body>
    // eg. post http://www.google.com name=jakub+oboza
    QStringList msg = QString(message.simplified()).split(" ", QString::SkipEmptyParts);
    QByteArray url = msg[1].toUtf8();
    QMap<QString, QVariant> params;
    params["operation"] = "post";
    if(msg.size() == 3){
        params["data"] = msg[2];
    }
    else{
        params["data"] = "";
    }
    m_page->openUrl(url, params, m_settings);  
}

void CommandSocket::handleGetCommand(QByteArray message){
    QByteArray url = message.replace("get ", "").simplified();
    m_page->openUrl(url, "get", m_settings);
}

void CommandSocket::readCommand() {
    bool shutdown = false;
    if (m_clientConnection->isValid()) {
        if (m_clientConnection->canReadLine()) {
            QByteArray message(m_clientConnection->readLine());
            message = message.simplified();

            if (message.startsWith("get ")) {
                this->handleGetCommand(message);
            }
            else if(message.startsWith("post ")){
                this->handlePostCommand(message);
            } else if (message.startsWith("render ")) {
                QByteArray path = message.replace("render ", "").simplified();
                bool success = m_page->render(path);
                if (success) {
                    m_clientConnection->write("+ ok\r\n");
                } else {
                    m_clientConnection->write("- unable to render image\r\n");
                }

            } else if (message.startsWith("show cookies for ")) {
                QByteArray address = message.replace("show cookies for ", "").simplified();
                QUrl url = QUrl::fromEncoded(address);
                QNetworkCookieJar *cookie_jar = m_page->cookieJar();
                QList<QNetworkCookie> cookies = cookie_jar->cookiesForUrl(url);

                QString resp;
                QTextStream(&resp) << "+ " << cookies.count() << " cookies: \r\n";

                for (int i = 0; i < cookies.size(); i++) {
                    QString cookie = cookies.at(i).toRawForm();
                    QTextStream(&resp) << cookie << "\r\n";
                }

                QTextStream(&resp) << ".\r\n";
                m_clientConnection->write(resp.toAscii());


            } else if (message.startsWith("set cookies for ")) {
                QByteArray address = message.replace("show cookies for ", "").simplified();

                QVariantList cookies;
                QByteArray last_line;

                bool still_working = true;
                do {
                    m_clientConnection->waitForReadyRead();
                    last_line = m_clientConnection->readLine();
                    if (last_line.simplified() == ".") {
                        still_working = false;
                    } else {
                        QList<QNetworkCookie> new_cookies = QNetworkCookie::parseCookies(last_line);
                        for (int i = 0; i < new_cookies.size(); i++) {
                            QNetworkCookie c = new_cookies.at(i);
                            QVariantMap cookie;
                            cookie["domain"] = c.domain();
                            cookie["name"] = c.name();
                            cookie["value"] = c.value();
                            cookie["path"] = c.path();
                            cookie["expires"] = c.expirationDate();
                            cookie["httponly"] = c.isHttpOnly();
                            cookie["secure"] = c.isSecure();

                            cookies << cookie;
                        }
                    }
                } while (still_working);

                QUrl url = QUrl::fromEncoded(address);
                m_page->setCookies(cookies);
                m_clientConnection->write("+ ok\r\n");


            } else if (message == "show content") {
              QString content = m_page->content();
              m_clientConnection->write("+ content:\r\n");
              m_clientConnection->write(content.toAscii());
              if (! content.endsWith("\n")) {
                m_clientConnection->write("\r\n");
              }
              m_clientConnection->write(".\r\n");


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

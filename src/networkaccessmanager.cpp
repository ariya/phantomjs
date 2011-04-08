#include <QDebug>
#include <QNetworkRequest>

#include "networkaccessmanager.h"

// public:
NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

// protected:
QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
{
    switch(op) {
    case QNetworkAccessManager::HeadOperation: {
        qDebug() << "HTTP/1.1 HEAD - URL:" << req.url();
        break;
    }
    case QNetworkAccessManager::GetOperation: {
        qDebug() << "HTTP/1.1 GET - URL:" << req.url();
        break;
    }
    case QNetworkAccessManager::PutOperation: {
        qDebug() << "HTTP/1.1 PUT - URL:" << req.url();
        break;
    }
    case QNetworkAccessManager::PostOperation: {
        qDebug() << "HTTP/1.1 POST - URL:" << req.url();
        break;
    }
    case QNetworkAccessManager::DeleteOperation: {
        qDebug() << "HTTP/1.1 DELETE - URL:" << req.url();
        break;
    }
    case QNetworkAccessManager::CustomOperation: {
        qDebug() << "HTTP/1.1 CUSTOM - URL:" << req.url();
        break;
    }
    default: {
        qWarning() << "Unexpected HTTP Operation Type";
        break;
    }
    }

    // Pass duty to the superclass - Nothing special to do here (yet?)
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}

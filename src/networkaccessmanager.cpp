#include <QDebug>
#include <QNetworkRequest>
#include <QList>
#include <QNetworkReply>

#include "networkaccessmanager.h"

// public:
NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(handleFinished(QNetworkReply*)));
}

// protected:
QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
{
    switch(op) {
    case QNetworkAccessManager::HeadOperation: {
        qDebug() << "HTTP/1.1 HEAD Request";
        break;
    }
    case QNetworkAccessManager::GetOperation: {
        qDebug() << "HTTP/1.1 GET Request";
        break;
    }
    case QNetworkAccessManager::PutOperation: {
        qDebug() << "HTTP/1.1 PUT Request";
        break;
    }
    case QNetworkAccessManager::PostOperation: {
        qDebug() << "HTTP/1.1 POST Request";
        break;
    }
    case QNetworkAccessManager::DeleteOperation: {
        qDebug() << "HTTP/1.1 DELETE Request";
        break;
    }
    case QNetworkAccessManager::CustomOperation: {
        qDebug() << "HTTP/1.1 CUSTOM Request";
        break;
    }
    default: {
        qWarning() << "Unexpected HTTP Operation Type";
        break;
    }
    }
    qDebug() << "URL" << req.url();

    // Pass duty to the superclass - Nothing special to do here (yet?)
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}

// private slots:
void NetworkAccessManager::handleFinished(QNetworkReply *reply)
{
    qDebug() << "HTTP/1.1 Response";
    QList<QNetworkReply::RawHeaderPair> headerPairs = reply->rawHeaderPairs();
    foreach ( QNetworkReply::RawHeaderPair pair, headerPairs ) {
        qDebug() << pair.first << "=" << pair.second;
    }
}

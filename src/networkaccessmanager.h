#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    NetworkAccessManager(QObject *parent = 0);

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData = 0);

private slots:
    void handleFinished(QNetworkReply *reply);
};

#endif // NETWORKACCESSMANAGER_H

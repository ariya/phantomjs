/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QtMIMETypeSniffer_h
#define QtMIMETypeSniffer_h

#include "MIMESniffing.h"
#include <QObject>

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

class QtMIMETypeSniffer : public QObject {
    Q_OBJECT
public:
    QtMIMETypeSniffer(QNetworkReply*, const QString& advertisedMimeType, bool isSupportedImageType);
    QString mimeType() const { return m_mimeType; }
    bool isFinished() const { return m_isFinished; }

signals:
    void finished();

private slots:
    void trySniffing();

private:
    bool sniff();

    QNetworkReply* m_reply;
    QString m_mimeType;
    MIMESniffer m_sniffer;
    bool m_isFinished;
};

#endif // QtMIMETypeSniffer_h

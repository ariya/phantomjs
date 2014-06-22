/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
QNetworkAccessManager *manager = new QNetworkAccessManager(this);
connect(manager, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(replyFinished(QNetworkReply*)));

manager->get(QNetworkRequest(QUrl("http://qt-project.org")));
//! [0]


//! [1]
QNetworkRequest request;
request.setUrl(QUrl("http://qt-project.org"));
request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

QNetworkReply *reply = manager->get(request);
connect(reply, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
        this, SLOT(slotError(QNetworkReply::NetworkError)));
connect(reply, SIGNAL(sslErrors(QList<QSslError>)),
        this, SLOT(slotSslErrors(QList<QSslError>)));
//! [1]

//! [2]
QNetworkConfigurationManager manager;
networkAccessManager->setConfiguration(manager.defaultConfiguration());
//! [2]

//! [3]
networkAccessManager->setConfiguration(QNetworkConfiguration());
//! [3]

//! [4]
networkAccessManager->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
//! [4]

//! [5]
networkAccessManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
//! [5]


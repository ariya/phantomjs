/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2013 Cisco Systems, Inc. All rights reserved.

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


#include <QDebug>
#include <QNetworkReply>
#include <QtTest>
#include <qwebframe.h>
#include <qwebpage.h>
#include <qwebsecurityorigin.h>
#include <qwebview.h>

class tst_QWebSecurityOrigin : public QObject {
    Q_OBJECT
public:
    tst_QWebSecurityOrigin();
    virtual ~tst_QWebSecurityOrigin();

private slots:
    void init();
    void cleanup();
    void whiteList_data();
    void whiteList();
private:
    QWebView* m_view;
    QWebPage* m_page;
};

tst_QWebSecurityOrigin::tst_QWebSecurityOrigin()
{
}

tst_QWebSecurityOrigin::~tst_QWebSecurityOrigin()
{
}

void tst_QWebSecurityOrigin::init()
{
    m_view = new QWebView();
    m_page = m_view->page();
}

void tst_QWebSecurityOrigin::cleanup()
{
    delete m_view;
}

void tst_QWebSecurityOrigin::whiteList_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("scheme");
    QTest::addColumn<QString>("host");
    QTest::addColumn<bool>("includeSubDomains");
    QTest::addColumn<QString>("testUrl");
    QTest::addColumn<bool>("successBeforeAdd");
    QTest::addColumn<bool>("successAfterAdd");
    QTest::addColumn<bool>("successAfterRemove");

    QTest::newRow("scheme") << "http://www.source.com" << "https" << "www.target.com" << false << "https://www.target.com/other" << false << true << false;
    QTest::newRow("schemeFail") << "http://www.source.com" << "https" << "www.target.com" << false << "http://www.target.com/other" << false << false << false;
    QTest::newRow("schemeSubDom") << "http://www.source.com" << "https" << "target.com" << true << "https://www.target.com/other" << false << true << false;
    QTest::newRow("schemeSubDomFail") << "http://www.source.com" << "https" << "target.com" << true << "https://wwwtarget.com/other" << false << false << false;
    QTest::newRow("schemeSubDomFail") << "http://www.source.com" << "https" << "target.com" << true << "http://www.target.com/other" << false << false << false;
    QTest::newRow("host") << "http://www.source.com" << "http" << "www.target.com" << false << "http://www.target.com/target" << false << true << false;
    QTest::newRow("hostFail") << "http://www.source.com" << "http" << "www.target.com" << false << "http://www.newtarget.com" << false << false << false;
    QTest::newRow("hostSubDom") << "http://www.source.com" << "http" << "target.com" << true << "http://www.new.target.com" << false << true << false;
    QTest::newRow("hostSubDomFail") << "http://www.source.com" << "http" << "target.com" << false << "http://www.new.target.com" << false << false << false;
    QTest::newRow("hostSubDomFailCountry") << "http://www.source.com" << "http" << "target.com" << true << "http://www.target.com.tw" << false << false << false;
}

class CannedResponseNetworkReply: public QNetworkReply {
    Q_OBJECT
public:
    CannedResponseNetworkReply(QObject* parent, QNetworkAccessManager::Operation op, const QNetworkRequest& req, const QBuffer& body): QNetworkReply(parent)
    {
        setRequest(req);
        setUrl(req.url());
        setOperation(op);
        m_buffer.setData(body.data());
        connect(&m_buffer, SIGNAL(readyRead()), SLOT(emitReadyRead()));
        connect(&m_buffer, SIGNAL(readChannelFinished()), SLOT(emitReadChannelFinished()));
        connect(&m_buffer, SIGNAL(readChannelFinished()), SLOT(emitReadChannelFinished()));
        m_buffer.open(QIODevice::ReadOnly);
        open(QIODevice::ReadOnly);
        QTimer::singleShot(10, this, SLOT(update()));
    }
protected:
    qint64 readData(char * data, qint64 maxSize)
    {
        qint64 result = m_buffer.read(data, maxSize);
        if (!m_buffer.bytesAvailable())
            QTimer::singleShot(10, this, SLOT(emitReadChannelFinished()));
        return result;
    }

    virtual qint64 bytesAvailable() const
    {
        return m_buffer.bytesAvailable();
    }

private slots:
    void emitReadyRead()
    {
        emit readyRead();
    }

    void emitReadChannelFinished()
    {
        emit readChannelFinished();
        emit finished();
    }
    void abort() { };

    void update()
    {
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
        setHeader(QNetworkRequest::ContentLengthHeader, m_buffer.size());
        setError(QNetworkReply::NoError, "");
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QString("Ok").toLatin1());
        emit metaDataChanged();
        emit readyRead();
    }
public:
    QBuffer m_buffer;
};

class CannedResponseNetworkAccessManager: public QNetworkAccessManager {
    Q_OBJECT
protected:
    virtual QNetworkReply* createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData = 0)
    {
        return new CannedResponseNetworkReply(this, op, req, *m_buffer);
    };
public:
    QBuffer* m_buffer;
};

static const char cannedResponse[] = "Test";

void tst_QWebSecurityOrigin::whiteList()
{
    QFETCH(QString, source);
    QFETCH(QString, scheme);
    QFETCH(QString, host);
    QFETCH(bool, includeSubDomains);
    QFETCH(QString, testUrl);
    QFETCH(bool, successBeforeAdd);
    QFETCH(bool, successAfterAdd);
    QFETCH(bool, successAfterRemove);

    QWebSecurityOrigin* origin = new QWebSecurityOrigin(source);
    CannedResponseNetworkAccessManager manager;
    QBuffer buffer;
    QWebSecurityOrigin::SubdomainSetting subdomainSetting = includeSubDomains ? QWebSecurityOrigin::AllowSubdomains : QWebSecurityOrigin::DisallowSubdomains;
    buffer.setData(cannedResponse, sizeof(cannedResponse)-1);
    manager.m_buffer = &buffer;
    QFile testPageFile(":/resources/test.html");
    QVERIFY(testPageFile.open(QIODevice::ReadOnly));
    uchar* testPage = testPageFile.map(0, testPageFile.size());
    QVERIFY(testPage);
    m_view->setHtml(QString((const char*)testPage), QUrl(source));
    m_view->page()->setNetworkAccessManager(&manager);
    QString testJS="runTest(\"" + testUrl + "\")";
    QCOMPARE(m_view->page()->mainFrame()->evaluateJavaScript(testJS), QVariant(successBeforeAdd));
    origin->addAccessWhitelistEntry(scheme, host, subdomainSetting);
    QCOMPARE(m_view->page()->mainFrame()->evaluateJavaScript(testJS), QVariant(successAfterAdd));
    origin->removeAccessWhitelistEntry(scheme, host, subdomainSetting);
    QCOMPARE(m_view->page()->mainFrame()->evaluateJavaScript(testJS), QVariant(successAfterRemove));
    m_view->page()->setNetworkAccessManager(0);
}

QTEST_MAIN(tst_QWebSecurityOrigin)
#include "tst_qwebsecurityorigin.moc"


/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkreplynsurlconnectionimpl_p.h"

#include "QtCore/qdatetime.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

// Network reply implementation using NSUrlConnection.
//
// Class/object structure:
//
// QNetworkReplyNSURLConnectionImpl
//  |- QNetworkReplyNSURLConnectionImplPrivate
//      |- (bytes read)
//      |- (QIODevice and CFStream for async POST data transfer)
//      |- NSURLConnection
//      |- QtNSURLConnectionDelegate <NSURLConnectionDataDelegate>
//          |- NSURLResponse/NSHTTPURLResponse
//          |- (response data)
//
// The main entry point is the QNetworkReplyNSURLConnectionImpl constructor, which
// receives a network request from QNetworkAccessManager. The constructor
// creates a NSURLRequest and initiates a NSURLConnection with a QtNSURLConnectionDelegate.
// The delegate callbacks are then called asynchronously as the request completes.
//

@class QtNSURLConnectionDelegate;
class QNetworkReplyNSURLConnectionImplPrivate: public QNetworkReplyPrivate
{
public:
    QNetworkReplyNSURLConnectionImplPrivate();
    virtual ~QNetworkReplyNSURLConnectionImplPrivate();

    Q_DECLARE_PUBLIC(QNetworkReplyNSURLConnectionImpl)
    NSURLConnection * urlConnection;
    QtNSURLConnectionDelegate *urlConnectionDelegate;
    qint64 bytesRead;

    // Sequental outgiong data streaming
    QIODevice *outgoingData;
    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;
    CFIndex transferBufferSize;

    // Forwarding functions to the public class.
    void setFinished();
    void setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);
    void setRawHeader(const QByteArray &headerName, const QByteArray &value);
    void setError(QNetworkReply::NetworkError errorCode, const QString &errorString);
};

@interface QtNSURLConnectionDelegate : NSObject
{
    NSURLResponse *response;
    NSMutableData *responseData;
    QNetworkReplyNSURLConnectionImplPrivate * replyprivate;
}

- (id)initWithQNetworkReplyNSURLConnectionImplPrivate:(QNetworkReplyNSURLConnectionImplPrivate *)a_replyPrivate ;
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_3_0)
- (void)connection:(NSURLConnection *)connection willSendRequestForAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge;
#endif
- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError*)error;
- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse*)response;
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData*)data;
- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten
      totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite;
- (NSCachedURLResponse*)connection:(NSURLConnection*)connection willCacheResponse:(NSCachedURLResponse*)cachedResponse;
- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)request redirectResponse:(NSURLResponse*)redirectResponse;
- (void)connectionDidFinishLoading:(NSURLConnection*)connection;
- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection*)connection;

@end

QNetworkReplyNSURLConnectionImplPrivate::QNetworkReplyNSURLConnectionImplPrivate()
    : QNetworkReplyPrivate()
    , urlConnection(0)
    , urlConnectionDelegate(0)
    , bytesRead(0)
    , readStream(0)
    , writeStream(0)
    , transferBufferSize(4096)
{
}

QNetworkReplyNSURLConnectionImplPrivate::~QNetworkReplyNSURLConnectionImplPrivate()
{
    [urlConnection cancel];
    [urlConnection release];
    [urlConnectionDelegate release];
    if (readStream)
        CFRelease(readStream);
    if (writeStream)
        CFRelease(writeStream);
}

void QNetworkReplyNSURLConnectionImplPrivate::setFinished()
{
    q_func()->setFinished(true);
}

void QNetworkReplyNSURLConnectionImplPrivate::setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value)
{
    q_func()->setHeader(header, value);
}

void QNetworkReplyNSURLConnectionImplPrivate::setRawHeader(const QByteArray &headerName, const QByteArray &value)
{
    q_func()->setRawHeader(headerName, value);
}

void QNetworkReplyNSURLConnectionImplPrivate::setError(QNetworkReply::NetworkError errorCode, const QString &errorString)
{
    q_func()->setError(errorCode, errorString);
}

void QNetworkReplyNSURLConnectionImpl::readyReadOutgoingData()
{
    Q_D(QNetworkReplyNSURLConnectionImpl);
    int bytesRead = 0;
    do {
        char data[d->transferBufferSize];
        bytesRead = d->outgoingData->read(data, d->transferBufferSize);
        if (bytesRead <= 0)
            break;
        CFIndex bytesWritten = CFWriteStreamWrite(d->writeStream, reinterpret_cast<unsigned char *>(data), bytesRead);
        if (bytesWritten != bytesRead) {
            CFErrorRef err = CFWriteStreamCopyError(d->writeStream);
            qWarning() << "QNetworkReplyNSURLConnectionImpl: CFWriteStreamWrite error"
                       << (err ? QString::number(CFErrorGetCode(err)) : QStringLiteral(""));
        }
    } while (bytesRead > 0);

    if (d->outgoingData->atEnd())
        CFWriteStreamClose(d->writeStream);
}

@interface QtNSURLConnectionDelegate ()

@property (nonatomic, retain) NSURLResponse* response;
@property (nonatomic, retain) NSMutableData* responseData;

@end

@implementation QtNSURLConnectionDelegate

@synthesize response;
@synthesize responseData;

- (id)initWithQNetworkReplyNSURLConnectionImplPrivate:(QNetworkReplyNSURLConnectionImplPrivate *)a_replyPrivate
{
    if (self = [super init])
        replyprivate = a_replyPrivate;
    return self;
}

- (void)dealloc
{
    [response release];
    [responseData release];
    [super dealloc];
}

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_7, __IPHONE_3_0)
- (void)connection:(NSURLConnection *)connection willSendRequestForAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    Q_UNUSED(connection)
    Q_UNUSED(challenge)

    if ([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        SecTrustRef serverTrust = challenge.protectionSpace.serverTrust;
        SecTrustResultType resultType;
        SecTrustEvaluate(serverTrust, &resultType);
        if (resultType == kSecTrustResultUnspecified) {
            // All good
            [challenge.sender performDefaultHandlingForAuthenticationChallenge:challenge];
        } else if (resultType == kSecTrustResultRecoverableTrustFailure) {
            // Certificate verification error, ask user
            // ### TODO actually ask user
            // (test site: https://testssl-expire.disig.sk/index.en.html)
            qWarning()  << "QNetworkReplyNSURLConnection: Certificate verification error handlig is"
                        << "not implemented. Connection will time out.";
        } else {
            // other error, which the default handler will handle
            [challenge.sender performDefaultHandlingForAuthenticationChallenge:challenge];
        }
    }

    [challenge.sender performDefaultHandlingForAuthenticationChallenge:challenge];
}
#endif

- (void)connection:(NSURLConnection*)connection didFailWithError:(NSError*)error
{
    Q_UNUSED(connection)

    QNetworkReply::NetworkError qtError = QNetworkReply::UnknownNetworkError;
    if ([error domain] == NSURLErrorDomain) {
        switch ([error code]) {
        case NSURLErrorTimedOut: qtError = QNetworkReply::TimeoutError; break;
        case NSURLErrorUnsupportedURL: qtError = QNetworkReply::ProtocolUnknownError; break;
        case NSURLErrorCannotFindHost: qtError = QNetworkReply::HostNotFoundError; break;
        case NSURLErrorCannotConnectToHost: qtError = QNetworkReply::ConnectionRefusedError; break;
        case NSURLErrorNetworkConnectionLost: qtError = QNetworkReply::NetworkSessionFailedError; break;
        case NSURLErrorDNSLookupFailed: qtError = QNetworkReply::HostNotFoundError; break;
        case NSURLErrorNotConnectedToInternet: qtError = QNetworkReply::NetworkSessionFailedError; break;
        case NSURLErrorUserAuthenticationRequired: qtError = QNetworkReply::AuthenticationRequiredError; break;
        default: break;
        }
    }

    replyprivate->setError(qtError, QString::fromNSString([error localizedDescription]));
}

- (void)connection:(NSURLConnection*)connection didReceiveResponse:(NSURLResponse*)aResponse
{
    Q_UNUSED(connection)
        self.response = aResponse;
    self.responseData = [NSMutableData data];

    // copy headers
    if ([aResponse isKindOfClass:[NSHTTPURLResponse class]]) {
        NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse*)aResponse;
        NSDictionary *headers = [httpResponse allHeaderFields];
        for (NSString *key in headers) {
            NSString *value = [headers objectForKey:key];
            replyprivate->setRawHeader(QString::fromNSString(key).toUtf8(), QString::fromNSString(value).toUtf8());
        }
    } else {
        if ([aResponse expectedContentLength] != NSURLResponseUnknownLength)
            replyprivate->setHeader(QNetworkRequest::ContentLengthHeader, [aResponse expectedContentLength]);
    }

    QMetaObject::invokeMethod(replyprivate->q_func(), "metaDataChanged", Qt::QueuedConnection);
}

- (void)connection:(NSURLConnection*)connection didReceiveData:(NSData*)data
{
     Q_UNUSED(connection)
         [responseData appendData:data];

    if ([response expectedContentLength] != NSURLResponseUnknownLength) {
        QMetaObject::invokeMethod(replyprivate->q_func(), "downloadProgress", Qt::QueuedConnection,
            Q_ARG(qint64, qint64([responseData length])),
            Q_ARG(qint64, qint64([response expectedContentLength])));
    }

    QMetaObject::invokeMethod(replyprivate->q_func(), "readyRead", Qt::QueuedConnection);
}

- (void)connection:(NSURLConnection*)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten
  totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite
{
    Q_UNUSED(connection)
    Q_UNUSED(bytesWritten)
    QMetaObject::invokeMethod(replyprivate->q_func(), "uploadProgress", Qt::QueuedConnection,
        Q_ARG(qint64, qint64(totalBytesWritten)),
        Q_ARG(qint64, qint64(totalBytesExpectedToWrite)));
}

- (NSCachedURLResponse*)connection:(NSURLConnection*)connection willCacheResponse:(NSCachedURLResponse*)cachedResponse
{
    Q_UNUSED(connection)
    return cachedResponse;
}

- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)request redirectResponse:(NSURLResponse*)redirectResponse
{
    Q_UNUSED(connection)
    Q_UNUSED(redirectResponse)
        return request;
}

- (void)connectionDidFinishLoading:(NSURLConnection*)connection
{
    Q_UNUSED(connection)
        replyprivate->setFinished();
    QMetaObject::invokeMethod(replyprivate->q_func(), "finished", Qt::QueuedConnection);
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection*)connection
{
    Q_UNUSED(connection)
    return YES;
}

@end

QNetworkReplyNSURLConnectionImpl::~QNetworkReplyNSURLConnectionImpl()
{
}

QNetworkReplyNSURLConnectionImpl::QNetworkReplyNSURLConnectionImpl(QObject *parent,
    const QNetworkRequest &request, const QNetworkAccessManager::Operation operation, QIODevice* outgoingData)
    : QNetworkReply(*new QNetworkReplyNSURLConnectionImplPrivate(), parent)
{
    setRequest(request);
    setUrl(request.url());
    setOperation(operation);
    QNetworkReply::open(QIODevice::ReadOnly);

    QNetworkReplyNSURLConnectionImplPrivate *d = (QNetworkReplyNSURLConnectionImplPrivate*) d_func();

    QUrl url = request.url();
    if (url.host() == QLatin1String("localhost"))
        url.setHost(QString());

    if (url.path().isEmpty())
        url.setPath(QLatin1String("/"));
    setUrl(url);

    // Create a NSMutableURLRequest from QNetworkRequest
    NSMutableURLRequest *nsRequest = [NSMutableURLRequest requestWithURL:request.url().toNSURL()
                                             cachePolicy:NSURLRequestUseProtocolCachePolicy
                                             timeoutInterval:60.0];
    // copy headers
    foreach (const QByteArray &header, request.rawHeaderList()) {
        QByteArray headerValue = request.rawHeader(header);
        [nsRequest addValue:QString::fromUtf8(headerValue).toNSString()
                 forHTTPHeaderField:QString::fromUtf8(header).toNSString()];
    }

    if (operation == QNetworkAccessManager::GetOperation)
        [nsRequest setHTTPMethod:@"GET"];
    else if (operation == QNetworkAccessManager::PostOperation)
        [nsRequest setHTTPMethod:@"POST"];
    else if (operation == QNetworkAccessManager::PutOperation)
        [nsRequest setHTTPMethod:@"PUT"];
    else if (operation == QNetworkAccessManager::DeleteOperation)
        [nsRequest setHTTPMethod:@"DELETE"];
    else
        qWarning() << "QNetworkReplyNSURLConnection: Unsupported netork operation" << operation;

    if (outgoingData) {
        d->outgoingData = outgoingData;
        if (outgoingData->isSequential()) {
            // set up streaming from outgoingData iodevice to request
            CFStreamCreateBoundPair(kCFAllocatorDefault, &d->readStream, &d->writeStream, d->transferBufferSize);
            CFWriteStreamOpen(d->writeStream);
            [nsRequest setHTTPBodyStream:reinterpret_cast<NSInputStream *>(d->readStream)];
            connect(outgoingData, SIGNAL(readyRead()), this, SLOT(readyReadOutgoingData()));
            readyReadOutgoingData();
        } else {
            // move all data at once
            QByteArray data = outgoingData->readAll();
            [nsRequest setHTTPBody:[NSData dataWithBytes:data.constData() length:data.length()]];
        }
    }

    // Create connection
    d->urlConnectionDelegate = [[QtNSURLConnectionDelegate alloc] initWithQNetworkReplyNSURLConnectionImplPrivate:d];
    d->urlConnection = [[NSURLConnection alloc] initWithRequest:nsRequest delegate:d->urlConnectionDelegate];
    if (!d->urlConnection) {
        // ### what type of error is an initWithRequest fail?
        setError(QNetworkReply::ProtocolUnknownError, QStringLiteral("QNetworkReplyNSURLConnection internal error"));
    }
}

void QNetworkReplyNSURLConnectionImpl::close()
{
    // No-op? Network ops should continue (especially POSTs)
    QNetworkReply::close();
}

void QNetworkReplyNSURLConnectionImpl::abort()
{
    Q_D(QNetworkReplyNSURLConnectionImpl);
    [d->urlConnection cancel];
    QNetworkReply::close();
}

qint64 QNetworkReplyNSURLConnectionImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyNSURLConnectionImpl);
    qint64 available = QNetworkReply::bytesAvailable() +
            [[d->urlConnectionDelegate responseData] length] -
            d->bytesRead;

    return available;
}

bool QNetworkReplyNSURLConnectionImpl::isSequential() const
{
    return true;
}

qint64 QNetworkReplyNSURLConnectionImpl::size() const
{
    Q_D(const QNetworkReplyNSURLConnectionImpl);
    return [[d->urlConnectionDelegate responseData] length];
}

/*!
    \internal
*/
qint64 QNetworkReplyNSURLConnectionImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyNSURLConnectionImpl);
    qint64 dataSize = [[d->urlConnectionDelegate responseData] length];
    qint64 canRead = qMin(maxlen, dataSize - d->bytesRead);
    const char *sourceBase = static_cast<const char *>([[d->urlConnectionDelegate responseData] bytes]);
    memcpy(data,  sourceBase + d->bytesRead, canRead);
    d->bytesRead += canRead;
    return canRead;
}

QT_END_NAMESPACE

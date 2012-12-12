/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include "qapplication.h"
#include "qbitmap.h"
#include "qdatetime.h"
#include "qbuffer.h"
#include "qwidget.h"
#include "qevent.h"
#include "private/qcore_symbian_p.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include "txtclipboard.h"
#endif
#include "txtetext.h"
#include <QtDebug>

// Symbian's clipboard
#include <baclipb.h>
QT_BEGIN_NAMESPACE

const TUid KQtCbDataStream = {0x2001B2DD};
const TInt KPlainTextBegin = 0;

class QClipboardData
{
public:
    QClipboardData();
   ~QClipboardData();

    void setSource(QMimeData* s)
    {
        if (s == src)
            return;
        delete src;
        src = s;
    }
    QMimeData* source()
    { return src; }
    bool connected()
    { return connection; }
    void clear();

private:
    QMimeData* src;
    bool connection;
};

QClipboardData::QClipboardData():src(0),connection(true)
{
    clear();
}

QClipboardData::~QClipboardData()
{
    connection = false;
    delete src;
}

void QClipboardData::clear()
{
    QMimeData* newSrc = new QMimeData;
    delete src;
    src = newSrc;
}

static QClipboardData *internalCbData = 0;

static void cleanupClipboardData()
{
    delete internalCbData;
    internalCbData = 0;
}

static QClipboardData *clipboardData()
{
    if (internalCbData == 0) {
        internalCbData = new QClipboardData;
        if (internalCbData)
        {
            if (!internalCbData->connected())
            {
                delete internalCbData;
                internalCbData = 0;
            }
            else
            {
                qAddPostRoutine(cleanupClipboardData);
            }
        }
    }
    return internalCbData;
}

void writeToStreamLX(const QMimeData* aData, RWriteStream& aStream)
{
    // This function both leaves and throws exceptions. There must be no destructor
    // dependencies between cleanup styles, and no cleanup stack dependencies on stacked objects.
    QStringList headers = aData->formats();
    aStream << TCardinality(headers.count());
    for (QStringList::const_iterator iter= headers.constBegin();iter != headers.constEnd();iter++)
    {
        HBufC* stringData = TPtrC(reinterpret_cast<const TUint16*>((*iter).utf16())).AllocLC();
        QByteArray ba = aData->data((*iter));
        // mime type
        aStream << TCardinality(stringData->Size());
        aStream << *(stringData);
        // mime data
        aStream << TCardinality(ba.size());
        aStream.WriteL(reinterpret_cast<const uchar*>(ba.constData()),ba.size());
        CleanupStack::PopAndDestroy(stringData);
    }
}

void writeToSymbianStoreLX(const QMimeData* aData, CClipboard* clipboard)
{
    // This function both leaves and throws exceptions. There must be no destructor
    // dependencies between cleanup styles, and no cleanup stack dependencies on stacked objects.
    if (aData->hasText()) {
        CPlainText* text = CPlainText::NewL();
        CleanupStack::PushL(text);

        TPtrC textPtr(qt_QString2TPtrC(aData->text()));
        text->InsertL(KPlainTextBegin, textPtr);
        text->CopyToStoreL(clipboard->Store(), clipboard->StreamDictionary(),
                           KPlainTextBegin, textPtr.Length());
        CleanupStack::PopAndDestroy(text);
    }
}

void readSymbianStoreLX(QMimeData* aData, CClipboard* clipboard)
{
    // This function both leaves and throws exceptions. There must be no destructor
    // dependencies between cleanup styles, and no cleanup stack dependencies on stacked objects.
    CPlainText* text = CPlainText::NewL();
    CleanupStack::PushL(text);
    TInt dataLength = text->PasteFromStoreL(clipboard->Store(), clipboard->StreamDictionary(),
                                            KPlainTextBegin);
    if (dataLength == 0) {
        User::Leave(KErrNotFound);
    }
    HBufC* hBuf = HBufC::NewL(dataLength);
    TPtr buf = hBuf->Des();
    text->Extract(buf, KPlainTextBegin, dataLength);

    QString string = qt_TDesC2QString(buf);
    CleanupStack::PopAndDestroy(text);

    aData->clear();
    aData->setText(string);
}

void readFromStreamLX(QMimeData* aData,RReadStream& aStream)
{
    // This function both leaves and throws exceptions. There must be no destructor
    // dependencies between cleanup styles, and no cleanup stack dependencies on stacked objects.
    TCardinality mimeTypeCount;
    aStream >> mimeTypeCount;
    if (mimeTypeCount > 0)
        aData->clear();
    for (int i = 0; i< mimeTypeCount;i++)
    {
        // mime type
        TCardinality mimeTypeSize;
        aStream >> mimeTypeSize;
        HBufC* mimeTypeBuf = HBufC::NewLC(aStream,mimeTypeSize);
        QString mimeType = QString(reinterpret_cast<const QChar *>(mimeTypeBuf->Des().Ptr()),
                                   mimeTypeBuf->Length());
        CleanupStack::PopAndDestroy(mimeTypeBuf);
        // mime data
        TCardinality dataSize;
        aStream >> dataSize;
        QByteArray ba;
        ba.reserve(dataSize);
        aStream.ReadL(reinterpret_cast<uchar*>(ba.data_ptr()->data),dataSize);
        ba.data_ptr()->size = dataSize;
        aData->setData(mimeType,ba);
    }
}


/*****************************************************************************
  QClipboard member functions
 *****************************************************************************/

void QClipboard::clear(Mode mode)
{
    setText(QString(), mode);
}
const QMimeData* QClipboard::mimeData(Mode mode) const
{
    if (mode != Clipboard) return 0;
    QClipboardData *d = clipboardData();
    bool dataExists(false);
    if (d)
    {
        TRAPD(err,{
            RFs fs = qt_s60GetRFs();
            CClipboard* cb = CClipboard::NewForReadingLC(fs);
            Q_ASSERT(cb);
            //stream for qt
            RStoreReadStream stream;
            TStreamId stid = (cb->StreamDictionary()).At(KQtCbDataStream);
            if (stid != 0) {
                stream.OpenLC(cb->Store(),stid);
                QT_TRYCATCH_LEAVING(readFromStreamLX(d->source(),stream));
                CleanupStack::PopAndDestroy(&stream);
                dataExists = true;
            }
            else {
                //symbian clipboard
                RStoreReadStream symbianStream;
                TStreamId symbianStId = (cb->StreamDictionary()).At(KClipboardUidTypePlainText);
                if (symbianStId != 0) {
                    symbianStream.OpenLC(cb->Store(), symbianStId);
                    QT_TRYCATCH_LEAVING(readSymbianStoreLX(d->source(), cb));
                    CleanupStack::PopAndDestroy(&symbianStream);
                    dataExists = true;
                }
            }
            CleanupStack::PopAndDestroy(cb);
        });
        if (err != KErrNone){
            qDebug()<< "clipboard is empty/err: " << err;
        }

        if (dataExists) {
            return d->source();
        }
    }
    return 0;
}


void QClipboard::setMimeData(QMimeData* src, Mode mode)
{
    if (mode != Clipboard) return;
    QClipboardData *d = clipboardData();
    if (d)
    {
        TRAPD(err,{
            RFs fs = qt_s60GetRFs();
            CClipboard* cb = CClipboard::NewForWritingLC(fs);
            //stream for qt
            RStoreWriteStream  stream;
            TStreamId stid = stream.CreateLC(cb->Store());
            QT_TRYCATCH_LEAVING(writeToStreamLX(src,stream));
            d->setSource(src);
            stream.CommitL();
            (cb->StreamDictionary()).AssignL(KQtCbDataStream,stid);
            cb->CommitL();

            //stream for symbian
            RStoreWriteStream symbianStream;
            TStreamId symbianStId = symbianStream.CreateLC(cb->Store());
            QT_TRYCATCH_LEAVING(writeToSymbianStoreLX(src, cb));
            (cb->StreamDictionary()).AssignL(KClipboardUidTypePlainText, symbianStId);
            cb->CommitL();
            CleanupStack::PopAndDestroy(3,cb);
        });
        if (err != KErrNone){
            qDebug()<< "clipboard write err :" << err;
        }
    }
    emitChanged(QClipboard::Clipboard);
}

bool QClipboard::supportsMode(Mode mode) const
{
    return (mode == Clipboard);
}

bool QClipboard::ownsMode(Mode mode) const
{
    if (mode == Clipboard)
        qWarning("QClipboard::ownsClipboard: UNIMPLEMENTED!");
    return false;
}

bool QClipboard::event(QEvent * /* e */)
{
    return true;
}

void QClipboard::connectNotify( const char * )
{
}

void QClipboard::ownerDestroyed()
{
}
QT_END_NAMESPACE
#endif // QT_NO_CLIPBOARD

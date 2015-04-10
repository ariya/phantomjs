/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qiosclipboard.h"

#include <QtPlatformSupport/private/qmacmime_p.h>
#include <QtCore/QMimeData>
#include <QtGui/QGuiApplication>

@interface UIPasteboard (QUIPasteboard)
    + (UIPasteboard *)pasteboardWithQClipboardMode:(QClipboard::Mode)mode;
@end

@implementation UIPasteboard (QUIPasteboard)
+ (UIPasteboard *)pasteboardWithQClipboardMode:(QClipboard::Mode)mode
{
    NSString *name = (mode == QClipboard::Clipboard) ? UIPasteboardNameGeneral : UIPasteboardNameFind;
    return [UIPasteboard pasteboardWithName:name create:NO];
}
@end

// --------------------------------------------------------------------

@interface QUIClipboard : NSObject
{
@public
    QIOSClipboard *m_qiosClipboard;
    NSInteger m_changeCountClipboard;
    NSInteger m_changeCountFindBuffer;
}
@end

@implementation QUIClipboard

-(id)initWithQIOSClipboard:(QIOSClipboard *)qiosClipboard
{
    self = [super init];
    if (self) {
        m_qiosClipboard = qiosClipboard;
        m_changeCountClipboard = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::Clipboard].changeCount;
        m_changeCountFindBuffer = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::FindBuffer].changeCount;

        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(updatePasteboardChanged:)
            name:UIPasteboardChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(updatePasteboardChanged:)
            name:UIPasteboardRemovedNotification object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(updatePasteboardChanged:)
            name:UIApplicationDidBecomeActiveNotification
            object:nil];
    }
    return self;
}

-(void)dealloc
{
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIPasteboardChangedNotification object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIPasteboardRemovedNotification object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:UIApplicationDidBecomeActiveNotification
        object:nil];
    [super dealloc];
}

- (void)updatePasteboardChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    NSInteger changeCountClipboard = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::Clipboard].changeCount;
    NSInteger changeCountFindBuffer = [UIPasteboard pasteboardWithQClipboardMode:QClipboard::FindBuffer].changeCount;

    if (m_changeCountClipboard != changeCountClipboard) {
        m_changeCountClipboard = changeCountClipboard;
        m_qiosClipboard->emitChanged(QClipboard::Clipboard);
    }

    if (m_changeCountFindBuffer != changeCountFindBuffer) {
        m_changeCountFindBuffer = changeCountFindBuffer;
        m_qiosClipboard->emitChanged(QClipboard::FindBuffer);
    }
}

@end

// --------------------------------------------------------------------

QT_BEGIN_NAMESPACE

class QIOSMimeData : public QMimeData {
public:
    QIOSMimeData(QClipboard::Mode mode) : QMimeData(), m_mode(mode) { }
    ~QIOSMimeData() { }

    QStringList formats() const Q_DECL_OVERRIDE;
    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const Q_DECL_OVERRIDE;

private:
    const QClipboard::Mode m_mode;
};

QStringList QIOSMimeData::formats() const
{
    QStringList foundMimeTypes;
    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:m_mode];
    NSArray *pasteboardTypes = [pb pasteboardTypes];

    for (NSUInteger i = 0; i < [pasteboardTypes count]; ++i) {
        QString uti = QString::fromNSString([pasteboardTypes objectAtIndex:i]);
        QString mimeType = QMacInternalPasteboardMime::flavorToMime(QMacInternalPasteboardMime::MIME_ALL, uti);
        if (!mimeType.isEmpty() && !foundMimeTypes.contains(mimeType))
            foundMimeTypes << mimeType;
    }

    return foundMimeTypes;
}

QVariant QIOSMimeData::retrieveData(const QString &mimeType, QVariant::Type) const
{
    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:m_mode];
    NSArray *pasteboardTypes = [pb pasteboardTypes];

    foreach (QMacInternalPasteboardMime *converter,
             QMacInternalPasteboardMime::all(QMacInternalPasteboardMime::MIME_ALL)) {
        if (!converter->canConvert(mimeType, converter->flavorFor(mimeType)))
            continue;

        for (NSUInteger i = 0; i < [pasteboardTypes count]; ++i) {
            NSString *availableUtiNSString = [pasteboardTypes objectAtIndex:i];
            QString availableUti = QString::fromNSString(availableUtiNSString);
            if (!converter->canConvert(mimeType, availableUti))
                continue;

            NSData *nsData = [pb dataForPasteboardType:availableUtiNSString];
            QList<QByteArray> dataList;
            dataList << QByteArray(reinterpret_cast<const char *>([nsData bytes]), [nsData length]);
            return converter->convertToMime(mimeType, dataList, availableUti);
        }
    }

    return QVariant();
}

// --------------------------------------------------------------------

QIOSClipboard::QIOSClipboard()
    : m_clipboard([[QUIClipboard alloc] initWithQIOSClipboard:this])
{
}

QMimeData *QIOSClipboard::mimeData(QClipboard::Mode mode)
{
    Q_ASSERT(supportsMode(mode));
    return new QIOSMimeData(mode);
}

void QIOSClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    Q_ASSERT(supportsMode(mode));

    UIPasteboard *pb = [UIPasteboard pasteboardWithQClipboardMode:mode];
    NSMutableDictionary *pbItem = [NSMutableDictionary dictionaryWithCapacity:mimeData->formats().size()];

    foreach (const QString &mimeType, mimeData->formats()) {
        foreach (QMacInternalPasteboardMime *converter,
                 QMacInternalPasteboardMime::all(QMacInternalPasteboardMime::MIME_ALL)) {
            QString uti = converter->flavorFor(mimeType);
            if (uti.isEmpty() || !converter->canConvert(mimeType, uti))
                continue;

            QByteArray byteArray = converter->convertFromMime(mimeType, mimeData->data(mimeType), uti).first();
            NSData *nsData = [NSData dataWithBytes:byteArray.constData() length:byteArray.size()];
            [pbItem setValue:nsData forKey:uti.toNSString()];
            break;
        }
    }

    pb.items = [NSArray arrayWithObject:pbItem];
}

bool QIOSClipboard::supportsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard || mode == QClipboard::FindBuffer);
}

bool QIOSClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

QT_END_NAMESPACE

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

#include "qcocoaclipboard.h"
#include "qmacclipboard.h"

QT_BEGIN_NAMESPACE

QCocoaClipboard::QCocoaClipboard()
    :m_clipboard(new QMacPasteboard(kPasteboardClipboard, QMacInternalPasteboardMime::MIME_CLIP))
    ,m_find(new QMacPasteboard(kPasteboardFind, QMacInternalPasteboardMime::MIME_CLIP))
{

}

QMimeData *QCocoaClipboard::mimeData(QClipboard::Mode mode)
{
    if (QMacPasteboard *pasteBoard = pasteboardForMode(mode)) {
        pasteBoard->sync();
        return pasteBoard->mimeData();
    }
    return 0;
}

void QCocoaClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (QMacPasteboard *pasteBoard = pasteboardForMode(mode)) {
        if (data == 0) {
            pasteBoard->clear();
        }

        pasteBoard->sync();
        pasteBoard->setMimeData(data);
        emitChanged(mode);
    }
}

bool QCocoaClipboard::supportsMode(QClipboard::Mode mode) const
{
    return (mode == QClipboard::Clipboard || mode == QClipboard::FindBuffer);
}

bool QCocoaClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

QMacPasteboard *QCocoaClipboard::pasteboardForMode(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Clipboard)
        return m_clipboard.data();
    else if (mode == QClipboard::FindBuffer)
        return m_find.data();
    else
        return 0;
}

QT_END_NAMESPACE

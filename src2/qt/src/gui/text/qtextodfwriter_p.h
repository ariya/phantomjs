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

#ifndef QTEXTODFWRITER_H
#define QTEXTODFWRITER_H
#ifndef QT_NO_TEXTODFWRITER

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QXmlStreamWriter>
#include <QtCore/qset.h>
#include <QtCore/qstack.h>

#include "qtextdocument_p.h"
#include "qtextdocumentwriter.h"

QT_BEGIN_NAMESPACE

class QTextDocumentPrivate;
class QTextCursor;
class QTextBlock;
class QIODevice;
class QXmlStreamWriter;
class QTextOdfWriterPrivate;
class QTextBlockFormat;
class QTextCharFormat;
class QTextListFormat;
class QTextFrameFormat;
class QTextTableCellFormat;
class QTextFrame;
class QTextFragment;
class QOutputStrategy;

class Q_AUTOTEST_EXPORT QTextOdfWriter {
public:
    QTextOdfWriter(const QTextDocument &document, QIODevice *device);
    bool writeAll();

    void setCodec(QTextCodec *codec) { m_codec = codec; }
    void setCreateArchive(bool on) { m_createArchive = on; }
    bool createArchive() const { return m_createArchive; }

    void writeBlock(QXmlStreamWriter &writer, const QTextBlock &block);
    void writeFormats(QXmlStreamWriter &writer, QSet<int> formatIds) const;
    void writeBlockFormat(QXmlStreamWriter &writer, QTextBlockFormat format, int formatIndex) const;
    void writeCharacterFormat(QXmlStreamWriter &writer, QTextCharFormat format, int formatIndex) const;
    void writeListFormat(QXmlStreamWriter &writer, QTextListFormat format, int formatIndex) const;
    void writeFrameFormat(QXmlStreamWriter &writer, QTextFrameFormat format, int formatIndex) const;
    void writeTableCellFormat(QXmlStreamWriter &writer, QTextTableCellFormat format, int formatIndex) const;
    void writeFrame(QXmlStreamWriter &writer, const QTextFrame *frame);
    void writeInlineCharacter(QXmlStreamWriter &writer, const QTextFragment &fragment) const;

    const QString officeNS, textNS, styleNS, foNS, tableNS, drawNS, xlinkNS, svgNS;
private:
    const QTextDocument *m_document;
    QIODevice *m_device;

    QOutputStrategy *m_strategy;
    QTextCodec *m_codec;
    bool m_createArchive;

    QStack<QTextList *> m_listStack;
};

QT_END_NAMESPACE

#endif // QT_NO_TEXTODFWRITER
#endif // QTEXTODFWRITER_H

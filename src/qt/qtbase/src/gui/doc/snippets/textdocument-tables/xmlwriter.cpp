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

#include <QtGui>

#include "xmlwriter.h"

QDomDocument *XmlWriter::toXml()
{
    QDomImplementation implementation;
    QDomDocumentType docType = implementation.createDocumentType(
        "scribe-document", "scribe", "qt.nokia.com/scribe");

    document = new QDomDocument(docType);

    // ### This processing instruction is required to ensure that any kind
    // of encoding is given when the document is written.
    QDomProcessingInstruction process = document->createProcessingInstruction(
        "xml", "version=\"1.0\" encoding=\"utf-8\"");
    document->appendChild(process);

    QDomElement documentElement = document->createElement("document");
    document->appendChild(documentElement);

    QTextFrame *root = textDocument->rootFrame();

    if (root)
        processFrame(documentElement, root);

    return document;
}

void XmlWriter::processBlock(QDomElement &parent, const QTextBlock &block)
{
    QDomElement blockElement = document->createElement("block");
    blockElement.setAttribute("position", block.position());
    blockElement.setAttribute("length", block.length());
    parent.appendChild(blockElement);

    QTextBlock::iterator it;
    for (it = block.begin(); !(it.atEnd()); ++it) {
        QTextFragment fragment = it.fragment();

        if (fragment.isValid()) {
            QDomElement fragmentElement = document->createElement("fragment");
            blockElement.appendChild(fragmentElement);

            fragmentElement.setAttribute("length", fragment.length());
            QDomText fragmentText = document->createTextNode(fragment.text());

            fragmentElement.appendChild(fragmentText);
        }
    }
}

void XmlWriter::processFrame(QDomElement &parent, QTextFrame *frame)
{
    QDomElement frameElement = document->createElement("frame");
    frameElement.setAttribute("begin", frame->firstPosition());
    frameElement.setAttribute("end", frame->lastPosition());
    parent.appendChild(frameElement);

//! [0]
    QDomElement frameElement = ...

    QTextFrame::iterator it;
    for (it = frame->begin(); !(it.atEnd()); ++it) {

        QTextFrame *childFrame = it.currentFrame();
        QTextBlock childBlock = it.currentBlock();

        if (childFrame) {
            QTextTable *childTable = qobject_cast<QTextTable*>(childFrame);

            if (childTable)
                processTable(frameElement, childTable);
            else
                processFrame(frameElement, childFrame);

        } else if (childBlock.isValid())
//! [0] //! [1]
            processBlock(frameElement, childBlock);
    }
//! [1]
}

void XmlWriter::processTable(QDomElement &parent, QTextTable *table)
{
    QDomElement element = document->createElement("table");

    for (int row = 0; row < table->rows(); ++row) {
        for (int column = 0; column < table->columns(); ++column) {
            QTextTableCell cell = table->cellAt(row, column);
            processTableCell(element, cell);
        }
    }
    parent.appendChild(element);
}

void XmlWriter::processTableCell(QDomElement &parent, const QTextTableCell &cell)
{
    QDomElement element = document->createElement("cell");
    element.setAttribute("row", cell.row());
    element.setAttribute("column", cell.column());

    QTextFrame::iterator it;
    for (it = cell.begin(); !(it.atEnd()); ++it) {

        QTextFrame *childFrame = it.currentFrame();
        QTextBlock childBlock = it.currentBlock();

        if (childFrame)
            processFrame(element, childFrame);
        else if (childBlock.isValid())
            processBlock(element, childBlock);
    }
    parent.appendChild(element);
}

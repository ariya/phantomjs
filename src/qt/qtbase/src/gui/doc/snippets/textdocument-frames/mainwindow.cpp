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

#include "mainwindow.h"
#include "xmlwriter.h"

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    QAction *saveAction = fileMenu->addAction(tr("&Save..."));
    saveAction->setShortcut(tr("Ctrl+S"));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    menuBar()->addMenu(fileMenu);
    editor = new QTextEdit();

    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextFrame *mainFrame = cursor.currentFrame();

    QTextCharFormat plainCharFormat;
    QTextCharFormat boldCharFormat;
    boldCharFormat.setFontWeight(QFont::Bold);
/*  main frame
//! [0]
    QTextFrame *mainFrame = cursor.currentFrame();
    cursor.insertText(...);
//! [0]
*/
    cursor.insertText("Text documents are represented by the "
                      "QTextDocument class, rather than by QString objects. "
                      "Each QTextDocument object contains information about "
                      "the document's internal representation, its structure, "
                      "and keeps track of modifications to provide undo/redo "
                      "facilities. This approach allows features such as the "
                      "layout management to be delegated to specialized "
                      "classes, but also provides a focus for the framework.",
                      plainCharFormat);

//! [1]
    QTextFrameFormat frameFormat;
    frameFormat.setMargin(32);
    frameFormat.setPadding(8);
    frameFormat.setBorder(4);
//! [1]
    cursor.insertFrame(frameFormat);

/*  insert frame
//! [2]
    cursor.insertFrame(frameFormat);
    cursor.insertText(...);
//! [2]
*/
    cursor.insertText("Documents are either converted from external sources "
                      "or created from scratch using Qt. The creation process "
                      "can done by an editor widget, such as QTextEdit, or by "
                      "explicit calls to the Scribe API.", boldCharFormat);

    cursor = mainFrame->lastCursorPosition();
/*  last cursor
//! [3]
    cursor = mainFrame->lastCursorPosition();
    cursor.insertText(...);
//! [3]
*/
    cursor.insertText("There are two complementary ways to visualize the "
                      "contents of a document: as a linear buffer that is "
                      "used by editors to modify the contents, and as an "
                      "object hierarchy containing structural information "
                      "that is useful to layout engines. In the hierarchical "
                      "model, the objects generally correspond to visual "
                      "elements such as frames, tables, and lists. At a lower "
                      "level, these elements describe properties such as the "
                      "style of text used and its alignment. The linear "
                      "representation of the document is used for editing and "
                      "manipulation of the document's contents.",
                      plainCharFormat);


    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Frames"));
}

void MainWindow::saveFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save document as:"), "", tr("XML (*.xml)"));

    if (!fileName.isEmpty()) {
        if (writeXml(fileName))
            setWindowTitle(fileName);
        else
            QMessageBox::warning(this, tr("Warning"),
                tr("Failed to save the document."), QMessageBox::Cancel,
                QMessageBox::NoButton);
    }
}
bool MainWindow::writeXml(const QString &fileName)
{
    XmlWriter documentWriter(editor->document());

    QDomDocument *domDocument = documentWriter.toXml();
    QFile file(fileName);

    if (file.open(QFile::WriteOnly)) {
        QTextStream textStream(&file);
        textStream.setCodec(QTextCodec::codecForName("UTF-8"));

        textStream << domDocument->toString(1).toUtf8();
        file.close();
        return true;
    }
    else
        return false;
}

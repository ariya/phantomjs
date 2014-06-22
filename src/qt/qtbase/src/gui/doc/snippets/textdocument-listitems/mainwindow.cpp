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

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    fileMenu->addAction(tr("E&xit"), this, SLOT(close()),
        QKeySequence(tr("Ctrl+Q", "File|Exit")));

    QMenu *actionsMenu = new QMenu(tr("&Actions"));
    actionsMenu->addAction(tr("&Highlight List Items"),
                        this, SLOT(highlightListItems()));
    actionsMenu->addAction(tr("&Show Current List"), this, SLOT(showList()));

    QMenu *insertMenu = new QMenu(tr("&Insert"));

    insertMenu->addAction(tr("&List"), this, SLOT(insertList()),
        QKeySequence(tr("Ctrl+L", "Insert|List")));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(insertMenu);
    menuBar()->addMenu(actionsMenu);

    editor = new QTextEdit(this);
    document = new QTextDocument(this);
    editor->setDocument(document);

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document List Items"));
}

void MainWindow::highlightListItems()
{
    QTextCursor cursor = editor->textCursor();
    QTextList *list = cursor.currentList();

    if (!list)
        return;

    cursor.beginEditBlock();
//! [0]
    for (int index = 0; index < list->count(); ++index) {
        QTextBlock listItem = list->item(index);
//! [0]
        QTextBlockFormat newBlockFormat = listItem.blockFormat();
        newBlockFormat.setBackground(Qt::lightGray);
        QTextCursor itemCursor = cursor;
        itemCursor.setPosition(listItem.position());
        //itemCursor.movePosition(QTextCursor::StartOfBlock);
        itemCursor.movePosition(QTextCursor::EndOfBlock,
                                QTextCursor::KeepAnchor);
        itemCursor.setBlockFormat(newBlockFormat);
        /*
//! [1]
        processListItem(listItem);
//! [1]
        */
//! [2]
    }
//! [2]
    cursor.endEditBlock();
}

void MainWindow::showList()
{
    QTextCursor cursor = editor->textCursor();
    QTextFrame *frame = cursor.currentFrame();

    if (!frame)
        return;

    QTreeWidget *treeWidget = new QTreeWidget;
    treeWidget->setColumnCount(1);
    QStringList headerLabels;
    headerLabels << tr("Lists");
    treeWidget->setHeaderLabels(headerLabels);

    QTreeWidgetItem *parentItem = 0;
    QTreeWidgetItem *item;
    QTreeWidgetItem *lastItem = 0;
    parentItems.clear();
    previousItems.clear();

//! [3]
    QTextFrame::iterator it;
    for (it = frame->begin(); !(it.atEnd()); ++it) {

        QTextBlock block = it.currentBlock();

        if (block.isValid()) {

            QTextList *list = block.textList();

            if (list) {
                int index = list->itemNumber(block);
//! [3]
                if (index == 0) {
                    parentItems.append(parentItem);
                    previousItems.append(lastItem);
                    listStructures.append(list);
                    parentItem = lastItem;
                    lastItem = 0;

                    if (parentItem != 0)
                        item = new QTreeWidgetItem(parentItem, lastItem);
                    else
                        item = new QTreeWidgetItem(treeWidget, lastItem);

                } else {

                    while (parentItem != 0 && listStructures.last() != list) {
                        listStructures.pop_back();
                        parentItem = parentItems.takeLast();
                        lastItem = previousItems.takeLast();
                    }
                    if (parentItem != 0)
                        item = new QTreeWidgetItem(parentItem, lastItem);
                    else
                        item = new QTreeWidgetItem(treeWidget, lastItem);
                }
                item->setText(0, block.text());
                lastItem = item;
                /*
//! [4]
                processListItem(list, index);
//! [4]
                */
//! [5]
            }
//! [5] //! [6]
        }
//! [6] //! [7]
    }
//! [7]

    treeWidget->setWindowTitle(tr("List Contents"));
    treeWidget->show();
}

void MainWindow::insertList()
{
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();

    QTextList *list = cursor.currentList();
    QTextListFormat listFormat;
    if (list)
        listFormat = list->format();

    listFormat.setStyle(QTextListFormat::ListDisc);
    listFormat.setIndent(listFormat.indent() + 1);
    cursor.insertList(listFormat);

    cursor.endEditBlock();
}

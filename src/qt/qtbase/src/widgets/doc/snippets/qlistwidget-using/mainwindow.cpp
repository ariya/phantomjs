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

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    QMenu *itemsMenu = new QMenu(tr("&Items"));

    insertAction = itemsMenu->addAction(tr("&Insert Item"));
    removeAction = itemsMenu->addAction(tr("&Remove Item"));
    QAction *ascendingAction = itemsMenu->addAction(tr("Sort in &Ascending Order"));
    QAction *descendingAction = itemsMenu->addAction(tr("Sort in &Descending Order"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(itemsMenu);

/*  For convenient quoting:
//! [0]
    QListWidget *listWidget = new QListWidget(this);
//! [0]
*/
    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(ascendingAction, SIGNAL(triggered()), this, SLOT(sortAscending()));
    connect(descendingAction, SIGNAL(triggered()), this, SLOT(sortDescending()));
    connect(insertAction, SIGNAL(triggered()), this, SLOT(insertItem()));
    connect(removeAction, SIGNAL(triggered()), this, SLOT(removeItem()));
    connect(listWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(updateMenus(QListWidgetItem*)));

    setupListItems();
    updateMenus(listWidget->currentItem());

    setCentralWidget(listWidget);
    setWindowTitle(tr("List Widget"));
}

void MainWindow::setupListItems()
{
//! [1]
    new QListWidgetItem(tr("Oak"), listWidget);
    new QListWidgetItem(tr("Fir"), listWidget);
    new QListWidgetItem(tr("Pine"), listWidget);
//! [1]
    new QListWidgetItem(tr("Birch"), listWidget);
//! [2]
    new QListWidgetItem(tr("Hazel"), listWidget);
//! [2]
    new QListWidgetItem(tr("Redwood"), listWidget);
//! [3]
    new QListWidgetItem(tr("Sycamore"), listWidget);
    new QListWidgetItem(tr("Chestnut"), listWidget);
    new QListWidgetItem(tr("Mahogany"), listWidget);
//! [3]
}

void MainWindow::sortAscending()
{
//! [4]
    listWidget->sortItems(Qt::AscendingOrder);
//! [4]
}

void MainWindow::sortDescending()
{
//! [5]
    listWidget->sortItems(Qt::DescendingOrder);
//! [5]
}

void MainWindow::insertItem()
{
    if (!listWidget->currentItem())
        return;

    QString itemText = QInputDialog::getText(this, tr("Insert Item"),
        tr("Input text for the new item:"));

    if (itemText.isNull())
        return;

//! [6]
    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(itemText);
//! [6]
    int row = listWidget->row(listWidget->currentItem());
//! [7]
    listWidget->insertItem(row, newItem);
//! [7]

    QString toolTipText = tr("Tooltip:") + itemText;
    QString statusTipText = tr("Status tip:") + itemText;
    QString whatsThisText = tr("What's This?:") + itemText;
//! [8]
    newItem->setToolTip(toolTipText);
    newItem->setStatusTip(toolTipText);
    newItem->setWhatsThis(whatsThisText);
//! [8]
}

void MainWindow::removeItem()
{
    listWidget->takeItem(listWidget->row(listWidget->currentItem()));
}

void MainWindow::updateMenus(QListWidgetItem *current)
{
    insertAction->setEnabled(current != 0);
    removeAction->setEnabled(current != 0);
}

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
#include "math.h"

#include "mainwindow.h"

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    QMenu *itemsMenu = new QMenu(tr("&Items"));

    QAction *sumItemsAction = itemsMenu->addAction(tr("&Sum Items"));
    QAction *averageItemsAction = itemsMenu->addAction(tr("&Average Items"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(itemsMenu);

//! [0]
    tableWidget = new QTableWidget(12, 3, this);
//! [0]
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

//! [1]
    QTableWidgetItem *valuesHeaderItem = new QTableWidgetItem(tr("Values"));
    tableWidget->setHorizontalHeaderItem(0, valuesHeaderItem);
//! [1]
    valuesHeaderItem->setTextAlignment(Qt::AlignVCenter);
    QTableWidgetItem *squaresHeaderItem = new QTableWidgetItem(tr("Squares"));
    squaresHeaderItem->setIcon(QIcon(QPixmap(":/Images/squared.png")));
    squaresHeaderItem->setTextAlignment(Qt::AlignVCenter);
//! [2]
    QTableWidgetItem *cubesHeaderItem = new QTableWidgetItem(tr("Cubes"));
    cubesHeaderItem->setIcon(QIcon(QPixmap(":/Images/cubed.png")));
    cubesHeaderItem->setTextAlignment(Qt::AlignVCenter);
//! [2]
    tableWidget->setHorizontalHeaderItem(1, squaresHeaderItem);
    tableWidget->setHorizontalHeaderItem(2, cubesHeaderItem);

    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(sumItemsAction, SIGNAL(triggered()), this, SLOT(sumItems()));
    connect(averageItemsAction, SIGNAL(triggered()), this, SLOT(averageItems()));

    setupTableItems();

    setCentralWidget(tableWidget);
    setWindowTitle(tr("Table Widget"));
}

void MainWindow::setupTableItems()
{
    for (int row = 0; row < tableWidget->rowCount()-1; ++row) {
        for (int column = 0; column < tableWidget->columnCount(); ++column) {
//! [3]
    QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(
        pow(row, column+1)));
    tableWidget->setItem(row, column, newItem);
//! [3]
        }
    }
    for (int column = 0; column < tableWidget->columnCount(); ++column) {
        QTableWidgetItem *newItem = new QTableWidgetItem;
        newItem->setFlags(Qt::ItemIsEnabled);
        tableWidget->setItem(tableWidget->rowCount()-1, column, newItem);
    }
}

void MainWindow::averageItems()
{
    QList<QTableWidgetItem *> selected = tableWidget->selectedItems();
    QTableWidgetItem *item;
    int number = 0;
    double total = 0;

    foreach (item, selected) {
        bool ok;
        double value = item->text().toDouble(&ok);

        if (ok && !item->text().isEmpty()) {
            total += value;
            number++;
        }
    }
    if (number > 0)
        tableWidget->currentItem()->setText(QString::number(total/number));
}

void MainWindow::sumItems()
{
//! [4]
    QList<QTableWidgetItem *> selected = tableWidget->selectedItems();
    QTableWidgetItem *item;
    int number = 0;
    double total = 0;

    foreach (item, selected) {
        bool ok;
        double value = item->text().toDouble(&ok);

        if (ok && !item->text().isEmpty()) {
            total += value;
            number++;
        }
    }
//! [4]
    if (number > 0)
        tableWidget->currentItem()->setText(QString::number(total));
}

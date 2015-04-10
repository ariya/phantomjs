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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Dock Widgets");

    setupDockWindow();
    setupContents();
    setupMenus();

    textBrowser = new QTextBrowser(this);

    connect(headingList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(updateText(QListWidgetItem*)));

    updateText(headingList->item(0));
    headingList->setCurrentRow(0);
    setCentralWidget(textBrowser);
}

void MainWindow::setupContents()
{
    QFile titlesFile(":/Resources/titles.txt");
    titlesFile.open(QFile::ReadOnly);
    int chapter = 0;

    do {
        QString line = titlesFile.readLine().trimmed();
        QStringList parts = line.split("\t", QString::SkipEmptyParts);
        if (parts.size() != 2)
            break;

        QString chapterTitle = parts[0];
        QString fileName = parts[1];

        QFile chapterFile(fileName);

        chapterFile.open(QFile::ReadOnly);
        QListWidgetItem *item = new QListWidgetItem(chapterTitle, headingList);
        item->setData(Qt::DisplayRole, chapterTitle);
        item->setData(Qt::UserRole, chapterFile.readAll());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        chapterFile.close();

        chapter++;
    } while (titlesFile.isOpen());

    titlesFile.close();
}

void MainWindow::setupDockWindow()
{
//! [0]
    contentsWindow = new QDockWidget(tr("Table of Contents"), this);
    contentsWindow->setAllowedAreas(Qt::LeftDockWidgetArea
                                  | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, contentsWindow);

    headingList = new QListWidget(contentsWindow);
    contentsWindow->setWidget(headingList);
//! [0]
}

void MainWindow::setupMenus()
{
    QAction *exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAct);
}

void MainWindow::updateText(QListWidgetItem *item)
{
    QString text = item->data(Qt::UserRole).toString();
    textBrowser->setHtml(text);
}

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

    QMenu *showMenu = new QMenu(tr("&Show"));

    QAction *showTableAction = showMenu->addAction(tr("&Table"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(showMenu);

    editor = new QTextEdit();

//! [0] //! [1]
    QTextCursor cursor(editor->textCursor());
//! [0]
    cursor.movePosition(QTextCursor::Start);
//! [1]

    int rows = 11;
    int columns = 4;

//! [2]
    QTextTableFormat tableFormat;
    tableFormat.setBackground(QColor("#e0e0e0"));
    QVector<QTextLength> constraints;
    constraints << QTextLength(QTextLength::PercentageLength, 16);
    constraints << QTextLength(QTextLength::PercentageLength, 28);
    constraints << QTextLength(QTextLength::PercentageLength, 28);
    constraints << QTextLength(QTextLength::PercentageLength, 28);
    tableFormat.setColumnWidthConstraints(constraints);
//! [3]
    QTextTable *table = cursor.insertTable(rows, columns, tableFormat);
//! [2] //! [3]

    int column;
    int row;
    QTextTableCell cell;
    QTextCursor cellCursor;

    QTextCharFormat charFormat;
    charFormat.setForeground(Qt::black);

//! [4]
    cell = table->cellAt(0, 0);
    cellCursor = cell.firstCursorPosition();
    cellCursor.insertText(tr("Week"), charFormat);
//! [4]

//! [5]
    for (column = 1; column < columns; ++column) {
        cell = table->cellAt(0, column);
        cellCursor = cell.firstCursorPosition();
        cellCursor.insertText(tr("Team %1").arg(column), charFormat);
    }

    for (row = 1; row < rows; ++row) {
        cell = table->cellAt(row, 0);
        cellCursor = cell.firstCursorPosition();
        cellCursor.insertText(tr("%1").arg(row), charFormat);

        for (column = 1; column < columns; ++column) {
            if ((row-1) % 3 == column-1) {
//! [5] //! [6]
                cell = table->cellAt(row, column);
                QTextCursor cellCursor = cell.firstCursorPosition();
                cellCursor.insertText(tr("On duty"), charFormat);
            }
//! [6] //! [7]
        }
//! [7] //! [8]
    }
//! [8]

    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(showTableAction, SIGNAL(triggered()), this, SLOT(showTable()));

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Tables"));
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

void MainWindow::showTable()
{
    QTextCursor cursor = editor->textCursor();
    QTextTable *table = cursor.currentTable();

    if (!table)
        return;

    QTableWidget *tableWidget = new QTableWidget(table->rows(), table->columns());

//! [9]
    for (int row = 0; row < table->rows(); ++row) {
        for (int column = 0; column < table->columns(); ++column) {
            QTextTableCell tableCell = table->cellAt(row, column);
//! [9]
            QTextFrame::iterator it;
            QString text;
            for (it = tableCell.begin(); !(it.atEnd()); ++it) {
                QTextBlock childBlock = it.currentBlock();
                if (childBlock.isValid())
                    text += childBlock.text();
            }
            QTableWidgetItem *newItem = new QTableWidgetItem(text);
            tableWidget->setItem(row, column, newItem);
            /*
//! [10]
            processTableCell(tableCell);
//! [10]
            */
//! [11]
        }
//! [11] //! [12]
    }
//! [12]

    tableWidget->setWindowTitle(tr("Table Contents"));
    tableWidget->show();
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

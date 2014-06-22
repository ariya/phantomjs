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

    QMenu *insertMenu = new QMenu(tr("&Insert"));

    QAction *calendarAction = insertMenu->addAction(tr("&Calendar"));
    calendarAction->setShortcut(tr("Ctrl+I"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(insertMenu);

    editor = new QTextEdit(this);

    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(calendarAction, SIGNAL(triggered()), this, SLOT(insertCalendar()));

    setCentralWidget(editor);
    setWindowTitle(tr("Text Document Writer"));
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

void MainWindow::insertCalendar()
{
    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextCharFormat format(cursor.charFormat());
    format.setFontFamily("Courier");

    QTextCharFormat boldFormat = format;
    boldFormat.setFontWeight(QFont::Bold);

    cursor.insertBlock();
    cursor.insertText(" ", boldFormat);

    QDate date = QDate::currentDate();
    int year = date.year(), month = date.month();

    for (int weekDay = 1; weekDay <= 7; ++weekDay) {
        cursor.insertText(QString("%1 ").arg(QDate::shortDayName(weekDay), 3),
            boldFormat);
    }

    cursor.insertBlock();
    cursor.insertText(" ", format);

    for (int column = 1; column < QDate(year, month, 1).dayOfWeek(); ++column) {
        cursor.insertText("    ", format);
    }

    for (int day = 1; day <= date.daysInMonth(); ++day) {
        int weekDay = QDate(year, month, day).dayOfWeek();

        if (QDate(year, month, day) == date)
            cursor.insertText(QString("%1 ").arg(day, 3), boldFormat);
        else
            cursor.insertText(QString("%1 ").arg(day, 3), format);

        if (weekDay == 7) {
            cursor.insertBlock();
            cursor.insertText(" ", format);
        }
    }
}

bool MainWindow::writeXml(const QString &fileName)
{
    XmlWriter documentWriter(editor->document());

    QDomDocument *domDocument = documentWriter.toXml();
    QFile file(fileName);

    if (file.open(QFile::WriteOnly)) {
        QTextStream textStream(&file);

        textStream << domDocument->toByteArray(1);
        return true;
    }
    else
        return false;
}

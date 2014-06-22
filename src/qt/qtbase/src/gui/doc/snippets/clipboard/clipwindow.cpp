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

#include "clipwindow.h"

ClipWindow::ClipWindow(QWidget *parent)
    : QMainWindow(parent)
{
    clipboard = QApplication::clipboard();

    QWidget *centralWidget = new QWidget(this);
    QWidget *currentItem = new QWidget(centralWidget);
    QLabel *mimeTypeLabel = new QLabel(tr("MIME types:"), currentItem);
    mimeTypeCombo = new QComboBox(currentItem);
    QLabel *dataLabel = new QLabel(tr("Data:"), currentItem);
    dataInfoLabel = new QLabel("", currentItem);

    previousItems = new QListWidget(centralWidget);

//! [0]
    connect(clipboard, SIGNAL(dataChanged()), this, SLOT(updateClipboard()));
//! [0]
    connect(mimeTypeCombo, SIGNAL(activated(QString)),
            this, SLOT(updateData(QString)));

    QVBoxLayout *currentLayout = new QVBoxLayout(currentItem);
    currentLayout->addWidget(mimeTypeLabel);
    currentLayout->addWidget(mimeTypeCombo);
    currentLayout->addWidget(dataLabel);
    currentLayout->addWidget(dataInfoLabel);
    currentLayout->addStretch(1);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(currentItem, 1);
    mainLayout->addWidget(previousItems);

    setCentralWidget(centralWidget);
    setWindowTitle(tr("Clipboard"));
}

//! [1]
void ClipWindow::updateClipboard()
{
    QStringList formats = clipboard->mimeData()->formats();
    QByteArray data = clipboard->mimeData()->data(format);
//! [1]

    mimeTypeCombo->clear();
    mimeTypeCombo->insertStringList(formats);

    int size = clipboard->mimeData()->data(formats[0]).size();
    QListWidgetItem *newItem = new QListWidgetItem(previousItems);
    newItem->setText(tr("%1 (%2 bytes)").arg(formats[0]).arg(size));

    updateData(formats[0]);
//! [2]
}
//! [2]

void ClipWindow::updateData(const QString &format)
{
    QByteArray data = clipboard->mimeData()->data(format);
    dataInfoLabel->setText(tr("%1 bytes").arg(data.size()));
}

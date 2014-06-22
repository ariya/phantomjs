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

/*
  main.cpp

  A simple example of how to access items from an existing model.
*/

#include <QtGui>

/*!
    Create a default directory model and, using the index-based interface to
    the model and some QLabel widgets, populate the window's layout with the
    names of objects in the directory.

    Note that we only want to read the filenames in the highest level of the
    directory, so we supply a default (invalid) QModelIndex to the model in
    order to indicate that we want top-level items.
*/

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(window);
    QLabel *title = new QLabel("Some items from the directory model", window);
    title->setBackgroundRole(QPalette::Base);
    title->setMargin(8);
    layout->addWidget(title);

//! [0]
    QFileSystemModel *model = new QFileSystemModel;
    QModelIndex parentIndex = model->index(QDir::currentPath());
    int numRows = model->rowCount(parentIndex);
//! [0]

//! [1]
    for (int row = 0; row < numRows; ++row) {
        QModelIndex index = model->index(row, 0, parentIndex);
//! [1]

//! [2]
        QString text = model->data(index, Qt::DisplayRole).toString();
        // Display the text in a widget.
//! [2]

        QLabel *label = new QLabel(text, window);
        layout->addWidget(label);
//! [3]
    }
//! [3]

    window->setWindowTitle("A simple model example");
    window->show();
    return app.exec();
}

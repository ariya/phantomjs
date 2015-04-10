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

//! [0]
class ZipEngineHandler : public QAbstractFileEngineHandler
{
public:
    QAbstractFileEngine *create(const QString &fileName) const;
};

QAbstractFileEngine *ZipEngineHandler::create(const QString &fileName) const
{
    // ZipEngineHandler returns a ZipEngine for all .zip files
    return fileName.toLower().endsWith(".zip") ? new ZipEngine(fileName) : 0;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ZipEngineHandler engine;

    MainWindow window;
    window.show();

    return app.exec();
}
//! [0]


//! [1]
QAbstractSocketEngine *ZipEngineHandler::create(const QString &fileName) const
{
    // ZipEngineHandler returns a ZipEngine for all .zip files
    return fileName.toLower().endsWith(".zip") ? new ZipEngine(fileName) : 0;
}
//! [1]


//! [2]
QAbstractFileEngineIterator *
CustomFileEngine::beginEntryList(QDir::Filters filters, const QStringList &filterNames)
{
    return new CustomFileEngineIterator(filters, filterNames);
}
//! [2]


//! [3]
class CustomIterator : public QAbstractFileEngineIterator
{
public:
    CustomIterator(const QStringList &nameFilters, QDir::Filters filters)
        : QAbstractFileEngineIterator(nameFilters, filters), index(0)
    {
        // In a real iterator, these entries are fetched from the
        // file system based on the value of path().
        entries << "entry1" << "entry2" << "entry3";
    }

    bool hasNext() const
    {
        return index < entries.size() - 1;
    }

    QString next()
    {
       if (!hasNext())
           return QString();
       ++index;
       return currentFilePath();
    }

    QString currentFileName()
    {
        return entries.at(index);
    }

private:
    QStringList entries;
    int index;
};
//! [3]

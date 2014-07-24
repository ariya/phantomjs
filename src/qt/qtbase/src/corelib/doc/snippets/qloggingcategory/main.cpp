/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
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

#include <QCoreApplication>
#include <QLoggingCategory>

//![1]
// in a header
Q_DECLARE_LOGGING_CATEGORY(QT_DRIVER_USB)

// in one source file
Q_LOGGING_CATEGORY(QT_DRIVER_USB, "qt.driver.usb")
//![1]


// Completely made up example, inspired by en.wikipedia.org/wiki/USB :)
struct UsbEntry {
    int id;
    int classtype;
};

QDebug operator<<(QDebug &dbg, const UsbEntry &entry)
{
    dbg.nospace() << "" << entry.id << " (" << entry.classtype << ")";
    return dbg.space();
}

QList<UsbEntry> usbEntries() {
    QList<UsbEntry> entries;
    return entries;
}

//![20]
void myCategoryFilter(QLoggingCategory *);
//![20]

//![21]
QLoggingCategory::CategoryFilter oldCategoryFilter;

void myCategoryFilter(QLoggingCategory *category)
{
    // configure qt.driver.usb category here, otherwise forward to to default filter.
    if (qstrcmp(category->categoryName(), "qt.driver.usb") == 0)
        category->setEnabled(QtDebugMsg, true);
    else
        oldCategoryFilter(category);
}
//![21]

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//![2]
    QLoggingCategory::setFilterRules(QStringLiteral("qt.driver.usb.debug=true"));
//![2]

//![22]

// ...
oldCategoryFilter = QLoggingCategory::installFilter(myCategoryFilter);
//![22]

//![3]
    qSetMessagePattern("%{category} %{message}");
//![3]

//![4]
    // usbEntries() will only be called if QT_DRIVER_USB category is enabled
    qCDebug(QT_DRIVER_USB) << "devices: " << usbEntries();
//![4]

    {
//![10]
    QLoggingCategory category("qt.driver.usb");
    qCDebug(category) << "a debug message";
//![10]
    }

    {
//![11]
    QLoggingCategory category("qt.driver.usb");
    qCWarning(category) << "a warning message";
//![11]
    }

    {
//![12]
    QLoggingCategory category("qt.driver.usb");
    qCCritical(category) << "a critical message";
//![12]
    }

    {
//![13]
    QLoggingCategory category("qt.driver.usb");
    qCDebug(category, "a debug message logged into category %s", category.categoryName());
//![13]
    }

    {
//![14]
    QLoggingCategory category("qt.driver.usb");
    qCWarning(category, "a warning message logged into category %s", category.categoryName());
//![14]
    }

    {
//![15]
    QLoggingCategory category("qt.driver.usb");
    qCCritical(category, "a critical message logged into category %s", category.categoryName());
//![15]
    }

    return 0;
}


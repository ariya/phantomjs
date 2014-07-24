/*
    Copyright (C) 2014 Digia Plc. and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef STORAGE_LEVELDB_UTIL_QT_LOGGER_H_
#define STORAGE_LEVELDB_UTIL_QT_LOGGER_H_

#include "leveldb/env.h"

#include <QDateTime>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QThread>

namespace leveldb {

class QtLogger : public Logger {
private:
    QFile file;
    QTextStream log;

public:
    QtLogger(QString fname) : file(fname), log(&file)
    {
        file.open(QIODevice::WriteOnly | QIODevice::Append);
    }
    virtual ~QtLogger() {
        log.flush();
        file.close();
    }

    virtual void Logv(const char* format, va_list ap) {
        const uint64_t thread_id = (qintptr)QThread::currentThreadId();
        QString formated;
        formated = formated.vsprintf(format, ap);

        log << QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate) << " " << thread_id << " " << formated << '\n';
    }
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_QT_LOGGER_H_

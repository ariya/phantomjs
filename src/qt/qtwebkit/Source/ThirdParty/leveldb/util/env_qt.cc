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

#include "leveldb/env.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "util/logging.h"
#include "util/qt_logger.h"

#include <string.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QThread>

//#define QTENV_DEBUG

#ifdef QTENV_DEBUG
#include <QDebug>
#endif

typedef void (*UserFunctionPointer)(void*);

Q_DECLARE_METATYPE(UserFunctionPointer)

namespace leveldb {

namespace {


static Status IOError(const QString& fname, const QString& errorMessage)
{
#ifdef QTENV_DEBUG
    qDebug() << Q_FUNC_INFO << fname << errorMessage;
#endif
    return Status::IOError(fname.toStdString(), errorMessage.toStdString());
}

class QtSequentialFile: public SequentialFile {
public:
    QFile m_file;

    QtSequentialFile(const std::string& fname) : m_file(QString::fromStdString(fname))
    { }
    virtual ~QtSequentialFile() { m_file.close(); }

    virtual Status Read(size_t n, Slice* result, char* scratch) {
        Status s;
        size_t r = m_file.read(scratch, n);
        *result = Slice(scratch, r);
        if (r < n) {
            if (m_file.atEnd()) {
                // We leave status as ok if we hit the end of the file
            } else {
                // A partial read with an error: return a non-ok status
                s = IOError(m_file.fileName(), m_file.errorString());
            }
        }
        return s;
      }

    virtual Status Skip(uint64_t n) {
        if (!m_file.seek(n + m_file.pos())) {
            return IOError(m_file.fileName(), m_file.errorString());
        }
        return Status::OK();
    }
};

class QtRandomAccessFile: public RandomAccessFile {
public:
    mutable QFile m_file;

    QtRandomAccessFile(const std::string& fname) : m_file(QString::fromStdString(fname))
    { }
    virtual ~QtRandomAccessFile() { m_file.close(); }

    virtual Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const
    {
        if (!m_file.seek(offset))
            return IOError(m_file.fileName(), m_file.errorString());

        qint64 r = m_file.peek(scratch, n);
        *result = Slice(scratch, (r < 0) ? 0 : r);
        if (r < 0) {
            // An error: return a non-ok status
            return IOError(m_file.fileName(), m_file.errorString());
        }
        return Status::OK();
    }
};

class QtWritableFile: public WritableFile {
public:
    QFile m_file;

    QtWritableFile(const std::string& fname) : m_file(QString::fromStdString(fname))
    { }
    virtual ~QtWritableFile() { m_file.close(); }

    virtual Status Append(const Slice& data)
    {
        m_file.unsetError();
        qint64 r = m_file.write(data.data(), data.size());
        if (r < 0) {
            return IOError(m_file.fileName(), m_file.errorString());
        }
        return Status::OK();
    }
    virtual Status Close()
    {
        m_file.unsetError();
        if (!m_file.isOpen())
            return IOError(m_file.fileName(), QObject::tr("File not open"));
        m_file.close();
        return Status::OK();
    }
    virtual Status Flush()
    {
        m_file.unsetError();
        if (!m_file.flush())
            return IOError(m_file.fileName(), m_file.errorString());
        return Status::OK();
    }

    virtual Status Sync()
    {
        return Flush();
    }
};

class QtFileLock : public FileLock {
public:
    QLockFile m_file;
    QString m_fileName;

    QtFileLock(const std::string& fname)
        : m_file(QString::fromStdString(fname))
        , m_fileName(QString::fromStdString(fname))
    { }
};

class BGProcessor : public QObject {
    Q_OBJECT
public:
    BGProcessor()
    {
    }

public slots:
    void slot_run(UserFunctionPointer user_function, void* arg)
    {
        user_function(arg);
    }
};

class BGThread : public QThread {
    Q_OBJECT
public:
    BGThread() : m_processor(0), m_started(0)
    {
    }

    virtual void run() {
        m_mutex.lock();
        m_processor = new BGProcessor();
        QObject::connect(this, SIGNAL(schedule(UserFunctionPointer,void*)), m_processor, SLOT(slot_run(UserFunctionPointer,void*)), Qt::QueuedConnection);
        m_started.store(1);
        m_startCond.wakeAll();
        m_mutex.unlock();
        exec();
        delete m_processor;
    }

    void waitUntilStarted() {
        if (m_started)
            return;
        m_mutex.lock();
        if (!m_started)
            m_startCond.wait(&m_mutex);
        m_mutex.unlock();
    }
signals:
    void schedule(UserFunctionPointer user_function, void* arg);

private:
    BGProcessor* m_processor;
    QMutex m_mutex;
    QWaitCondition m_startCond;
    QAtomicInt m_started;
};

class QtEnv : public Env {
public:
    QtEnv() {
        bgthread.start();
    }

    virtual ~QtEnv() {
        qFatal("Destroying Env::Default()\n");
    }

    virtual Status NewSequentialFile(const std::string& fname, SequentialFile** result)
    {
        QtSequentialFile* file = new QtSequentialFile(fname);
        if (!file->m_file.open(QIODevice::ReadOnly)) {
            *result = 0;
            Status status = IOError(file->m_file.fileName(), file->m_file.errorString());
            delete file;
            return status;
        }
        *result = file;
        return Status::OK();
    }

    virtual Status NewRandomAccessFile(const std::string& fname, RandomAccessFile** result)
    {
        QtRandomAccessFile* file = new QtRandomAccessFile(fname);
        if (!file->m_file.open(QIODevice::ReadOnly)) {
            *result = 0;
            Status status = IOError(file->m_file.fileName(), file->m_file.errorString());
            delete file;
            return status;
        }
        *result = file;
        return Status::OK();
    }

    virtual Status NewWritableFile(const std::string& fname, WritableFile** result)
    {
        QtWritableFile* file = new QtWritableFile(fname);
        if (!file->m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            *result = 0;
            Status status = IOError(file->m_file.fileName(), file->m_file.errorString());
            delete file;
            return status;
        }
        *result = file;
        return Status::OK();
    }

    virtual bool FileExists(const std::string& fname)
    {
        return QFile::exists(QString::fromStdString(fname));
    }

    virtual Status GetChildren(const std::string& dir, std::vector<std::string>* result)
    {
        result->clear();
        QDir qdir(QString::fromStdString(dir));
        foreach (const QString& filename, qdir.entryList()) {
            result->push_back(filename.toStdString());
        }
        return Status::OK();
    }

    virtual Status DeleteFile(const std::string& fname)
    {
        QFile file(QString::fromStdString(fname));
        if (!file.remove())
            return IOError(file.fileName(), file.errorString());
        return Status::OK();
    }

    virtual Status CreateDir(const std::string& name)
    {
        QDir dir;
        if (!dir.mkdir(QString::fromStdString(name)))
            return IOError(QString::fromStdString(name), QObject::tr("Could not create path"));
        return Status::OK();
    }

    virtual Status DeleteDir(const std::string& name)
    {
        QDir dir;
        if (!dir.rmdir(QString::fromStdString(name)))
            return IOError(QString::fromStdString(name), QObject::tr("Could not delete path"));
        return Status::OK();
    }

    virtual Status GetFileSize(const std::string& fname, uint64_t* size)
    {
        QFile file(QString::fromStdString(fname));
        qint64 sz = file.size();
        if (!sz)
            return IOError(file.fileName(), file.errorString());

        *size = sz;
        return Status::OK();
    }

    virtual Status RenameFile(const std::string& src, const std::string& target)
    {
        QFile file(QString::fromStdString(src));
        // POSIX rename overwrites destination, QFile::rename does not.
        if (file.exists() && QFile::exists(QString::fromStdString(target)))
            QFile::remove(QString::fromStdString(target));
        if (!file.rename(QString::fromStdString(target)))
            return IOError(file.fileName(), file.errorString());
        return Status::OK();
    }

    virtual Status LockFile(const std::string& fname, FileLock** lock)
    {
        *lock = 0;
        QtFileLock* my_lock = new QtFileLock(fname);
        if (!my_lock->m_file.tryLock(500)) {
            if (my_lock->m_file.staleLockTime() > 30000) {
                // Handle stale lock files not locked by Qt.
                my_lock->m_file.setStaleLockTime(30000);
                return LockFile(fname, lock);
            }
            QString errorString;
            switch (my_lock->m_file.error()) {
            case QLockFile::NoError:
                errorString = QObject::tr("No error");
                break;
            case QLockFile::LockFailedError:
                errorString = QObject::tr("Lock failed error");
                break;
            case QLockFile::PermissionError:
                errorString = QObject::tr("Permission error");
                break;
            case QLockFile::UnknownError:
                errorString = QObject::tr("Unknown error");
                break;
            }
            delete my_lock;
            return IOError(QString::fromStdString(fname), errorString);
        }
        *lock = my_lock;
        return Status::OK();
    }

    virtual Status UnlockFile(FileLock* lock)
    {
        QtFileLock* my_lock = reinterpret_cast<QtFileLock*>(lock);
        my_lock->m_file.unlock();
        delete my_lock;
        return Status::OK();
    }

    virtual void Schedule(void (*function)(void*), void* arg)
    {
        bgthread.waitUntilStarted();
        emit bgthread.schedule(function, arg);
    }

    virtual void StartThread(void (*function)(void* arg), void* arg);

    virtual Status GetTestDirectory(std::string* result) {
        QByteArray env = qgetenv("TEST_TMPDIR");
        if (!env.isEmpty() && env[0] != '\0') {
            *result = QString::fromLocal8Bit(env).toStdString();
        } else {
            QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            *result = path.toStdString();
        }
        // Directory may already exist
        CreateDir(*result);
        return Status::OK();
    }

    virtual Status NewLogger(const std::string& fname, Logger** result)
    {
        *result = new QtLogger(QString::fromStdString(fname));
        return Status::OK();
    }

    virtual uint64_t NowMicros()
    {
        return QDateTime::currentMSecsSinceEpoch() * 1000;
    }

    virtual void SleepForMicroseconds(int micros)
    {
        QThread::usleep(micros);
    }
private:
    BGThread bgthread;
};

class StartThreadState : public QThread {
public:
    StartThreadState(UserFunctionPointer user_function, void* arg)
        : user_function(user_function)
        , arg(arg)
    { }
    virtual void run()
    {
        user_function(arg);
    }
private:
    UserFunctionPointer user_function;
    void* arg;
};

void QtEnv::StartThread(void (*function)(void*), void* arg)
{
    QThread* thread = new StartThreadState(function, arg);
    QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

}  // namespace

static port::OnceType once = LEVELDB_ONCE_INIT;
static Env* default_env;
static void InitDefaultEnv()
{
    qRegisterMetaType<UserFunctionPointer>();
    default_env = new QtEnv;
}

Env* Env::Default() {
    port::InitOnce (&once, InitDefaultEnv);
    return default_env;
}

}  // namespace leveldb

#include "env_qt.moc"

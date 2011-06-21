#include "filesystem.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>

// public:
FileSystem::FileSystem(QObject *parent) :
    QObject(parent)
{
}

// public slots:
bool FileSystem::exists(const QString &path) const
{
    return QFile::exists(path);
}

bool FileSystem::isDir(const QString &path) const
{
    return QFileInfo(path).isDir();
}

bool FileSystem::isFile(const QString &path) const
{
    return QFileInfo(path).isFile();
}

bool FileSystem::mkDir(const QString &path) const
{
    return QDir().mkdir(path);
}

QStringList FileSystem::list(const QString &path) const
{
    return QDir(path).entryList();
}

QString FileSystem::workDir() const
{
    return QDir::currentPath();
}

QString FileSystem::separator() const
{
    return QDir::separator();
}

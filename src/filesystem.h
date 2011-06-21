#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QObject>
#include <QStringList>

class FileSystem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString workDir READ workDir)
    Q_PROPERTY(QString separator READ separator)

public:
    FileSystem(QObject *parent = 0);

public slots:
    bool exists(const QString &path) const;
    bool isDir(const QString &path) const;
    bool isFile(const QString &path) const;
    bool mkDir(const QString &path) const;
    QStringList list(const QString &path) const;
    QString workDir() const;
    QString separator() const;
};

#endif // FILESYSTEM_H

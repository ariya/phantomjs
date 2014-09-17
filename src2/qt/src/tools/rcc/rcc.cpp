/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rcc.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QLocale>
#include <QtCore/QStack>

#include <QtXml/QDomDocument>

QT_BEGIN_NAMESPACE

enum {
    CONSTANT_USENAMESPACE = 1,
    CONSTANT_COMPRESSLEVEL_DEFAULT = -1,
    CONSTANT_COMPRESSTHRESHOLD_DEFAULT = 70
};


#define writeString(s) write(s, sizeof(s))

void RCCResourceLibrary::write(const char *str, int len)
{
    --len; // trailing \0 on string literals...
    int n = m_out.size();
    m_out.resize(n + len);
    memcpy(m_out.data() + n, str, len);
}

void RCCResourceLibrary::writeByteArray(const QByteArray &other)
{
    m_out.append(other);
}

static inline QString msgOpenReadFailed(const QString &fname, const QString &why)
{
    return QString::fromUtf8("Unable to open %1 for reading: %2\n").arg(fname).arg(why);
}


///////////////////////////////////////////////////////////
//
// RCCFileInfo
//
///////////////////////////////////////////////////////////

class RCCFileInfo
{
public:
    enum Flags
    {
        NoFlags = 0x00,
        Compressed = 0x01,
        Directory = 0x02
    };

    RCCFileInfo(const QString &name = QString(), const QFileInfo &fileInfo = QFileInfo(),
                QLocale::Language language = QLocale::C,
                QLocale::Country country = QLocale::AnyCountry,
                uint flags = NoFlags,
                int compressLevel = CONSTANT_COMPRESSLEVEL_DEFAULT,
                int compressThreshold = CONSTANT_COMPRESSTHRESHOLD_DEFAULT);
    ~RCCFileInfo();

    QString resourceName() const;

public:
    qint64 writeDataBlob(RCCResourceLibrary &lib, qint64 offset, QString *errorMessage);
    qint64 writeDataName(RCCResourceLibrary &, qint64 offset);
    void writeDataInfo(RCCResourceLibrary &lib);

    int m_flags;
    QString m_name;
    QLocale::Language m_language;
    QLocale::Country m_country;
    QFileInfo m_fileInfo;
    RCCFileInfo *m_parent;
    QHash<QString, RCCFileInfo*> m_children;
    int m_compressLevel;
    int m_compressThreshold;

    qint64 m_nameOffset;
    qint64 m_dataOffset;
    qint64 m_childOffset;
};

RCCFileInfo::RCCFileInfo(const QString &name, const QFileInfo &fileInfo,
    QLocale::Language language, QLocale::Country country, uint flags,
    int compressLevel, int compressThreshold)
{
    m_name = name;
    m_fileInfo = fileInfo;
    m_language = language;
    m_country = country;
    m_flags = flags;
    m_parent = 0;
    m_nameOffset = 0;
    m_dataOffset = 0;
    m_childOffset = 0;
    m_compressLevel = compressLevel;
    m_compressThreshold = compressThreshold;
}

RCCFileInfo::~RCCFileInfo()
{
    qDeleteAll(m_children);
}

QString RCCFileInfo::resourceName() const
{
    QString resource = m_name;
    for (RCCFileInfo *p = m_parent; p; p = p->m_parent)
        resource = resource.prepend(p->m_name + QLatin1Char('/'));
    return QLatin1Char(':') + resource;
}

void RCCFileInfo::writeDataInfo(RCCResourceLibrary &lib)
{
    const bool text = (lib.m_format == RCCResourceLibrary::C_Code);
    //some info
    if (text) {
        if (m_language != QLocale::C) {
            lib.writeString("  // ");
            lib.writeByteArray(resourceName().toLocal8Bit());
            lib.writeString(" [");
            lib.writeByteArray(QByteArray::number(m_country));
            lib.writeString("::");
            lib.writeByteArray(QByteArray::number(m_language));
            lib.writeString("[\n  ");
        } else {
            lib.writeString("  // ");
            lib.writeByteArray(resourceName().toLocal8Bit());
            lib.writeString("\n  ");
        }
    }

    //pointer data
    if (m_flags & RCCFileInfo::Directory) {
        // name offset
        lib.writeNumber4(m_nameOffset);

        // flags
        lib.writeNumber2(m_flags);

        // child count
        lib.writeNumber4(m_children.size());

        // first child offset
        lib.writeNumber4(m_childOffset);
    } else {
        // name offset
        lib.writeNumber4(m_nameOffset);

        // flags
        lib.writeNumber2(m_flags);

        // locale
        lib.writeNumber2(m_country);
        lib.writeNumber2(m_language);

        //data offset
        lib.writeNumber4(m_dataOffset);
    }
    if (text)
        lib.writeChar('\n');
}

qint64 RCCFileInfo::writeDataBlob(RCCResourceLibrary &lib, qint64 offset,
    QString *errorMessage)
{
    const bool text = (lib.m_format == RCCResourceLibrary::C_Code);

    //capture the offset
    m_dataOffset = offset;

    //find the data to be written
    QFile file(m_fileInfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        *errorMessage = msgOpenReadFailed(m_fileInfo.absoluteFilePath(), file.errorString());
        return 0;
    }
    QByteArray data = file.readAll();

#ifndef QT_NO_COMPRESS
    // Check if compression is useful for this file
    if (m_compressLevel != 0 && data.size() != 0) {
        QByteArray compressed =
            qCompress(reinterpret_cast<uchar *>(data.data()), data.size(), m_compressLevel);

        int compressRatio = int(100.0 * (data.size() - compressed.size()) / data.size());
        if (compressRatio >= m_compressThreshold) {
            data = compressed;
            m_flags |= Compressed;
        }
    }
#endif // QT_NO_COMPRESS

    // some info
    if (text) {
        lib.writeString("  // ");
        lib.writeByteArray(m_fileInfo.absoluteFilePath().toLocal8Bit());
        lib.writeString("\n  ");
    }

    // write the length

    lib.writeNumber4(data.size());
    if (text)
        lib.writeString("\n  ");
    offset += 4;

    // write the payload
    const char *p = data.constData();
    if (text) {
        for (int i = data.size(), j = 0; --i >= 0; --j) {
            lib.writeHex(*p++);
            if (j == 0) {
                lib.writeString("\n  ");
                j = 16;
            }
        }
    } else {
        for (int i = data.size(); --i >= 0; )
           lib.writeChar(*p++);
    }
    offset += data.size();

    // done
    if (text)
        lib.writeString("\n  ");
    return offset;
}

qint64 RCCFileInfo::writeDataName(RCCResourceLibrary &lib, qint64 offset)
{
    const bool text = (lib.m_format == RCCResourceLibrary::C_Code);

    // capture the offset
    m_nameOffset = offset;

    // some info
    if (text) {
        lib.writeString("  // ");
        lib.writeByteArray(m_name.toLocal8Bit());
        lib.writeString("\n  ");
    }

    // write the length
    lib.writeNumber2(m_name.length());
    if (text)
        lib.writeString("\n  ");
    offset += 2;

    // write the hash
    lib.writeNumber4(qHash(m_name));
    if (text)
        lib.writeString("\n  ");
    offset += 4;

    // write the m_name
    const QChar *unicode = m_name.unicode();
    for (int i = 0; i < m_name.length(); ++i) {
        lib.writeNumber2(unicode[i].unicode());
        if (text && i % 16 == 0)
            lib.writeString("\n  ");
    }
    offset += m_name.length()*2;

    // done
    if (text)
        lib.writeString("\n  ");
    return offset;
}


///////////////////////////////////////////////////////////
//
// RCCResourceLibrary
//
///////////////////////////////////////////////////////////

RCCResourceLibrary::Strings::Strings() :
   TAG_RCC(QLatin1String("RCC")),
   TAG_RESOURCE(QLatin1String("qresource")),
   TAG_FILE(QLatin1String("file")),
   ATTRIBUTE_LANG(QLatin1String("lang")),
   ATTRIBUTE_PREFIX(QLatin1String("prefix")),
   ATTRIBUTE_ALIAS(QLatin1String("alias")),
   ATTRIBUTE_THRESHOLD(QLatin1String("threshold")),
   ATTRIBUTE_COMPRESS(QLatin1String("compress"))
{
}

RCCResourceLibrary::RCCResourceLibrary()
  : m_root(0),
    m_format(C_Code),
    m_verbose(false),
    m_compressLevel(CONSTANT_COMPRESSLEVEL_DEFAULT),
    m_compressThreshold(CONSTANT_COMPRESSTHRESHOLD_DEFAULT),
    m_treeOffset(0),
    m_namesOffset(0),
    m_dataOffset(0),
    m_useNameSpace(CONSTANT_USENAMESPACE),
    m_errorDevice(0)
{
    m_out.reserve(30 * 1000 * 1000);
}

RCCResourceLibrary::~RCCResourceLibrary()
{
    delete m_root;
}

bool RCCResourceLibrary::interpretResourceFile(QIODevice *inputDevice,
    const QString &fname, QString currentPath, bool ignoreErrors)
{
    Q_ASSERT(m_errorDevice);
    const QChar slash = QLatin1Char('/');
    if (!currentPath.isEmpty() && !currentPath.endsWith(slash))
        currentPath += slash;

    QDomDocument document;
    {
        QString errorMsg;
        int errorLine = 0;
        int errorColumn = 0;
        if (!document.setContent(inputDevice, &errorMsg, &errorLine, &errorColumn)) {
            if (ignoreErrors)
                return true;
            const QString msg = QString::fromUtf8("RCC Parse Error: '%1' Line: %2 Column: %3 [%4]\n").arg(fname).arg(errorLine).arg(errorColumn).arg(errorMsg);
            m_errorDevice->write(msg.toUtf8());
            return false;
        }
    }

    QDomElement domRoot = document.firstChildElement(m_strings.TAG_RCC).toElement();
    if (!domRoot.isNull() && domRoot.tagName() == m_strings.TAG_RCC) {
        for (QDomNode node = domRoot.firstChild(); !node.isNull(); node = node.nextSibling()) {
            if (!node.isElement())
                continue;

            QDomElement child = node.toElement();
            if (!child.isNull() && child.tagName() == m_strings.TAG_RESOURCE) {
                QLocale::Language language = QLocale::c().language();
                QLocale::Country country = QLocale::c().country();

                if (child.hasAttribute(m_strings.ATTRIBUTE_LANG)) {
                    QString attribute = child.attribute(m_strings.ATTRIBUTE_LANG);
                    QLocale lang = QLocale(attribute);
                    language = lang.language();
                    if (2 == attribute.length()) {
                        // Language only
                        country = QLocale::AnyCountry;
                    } else {
                        country = lang.country();
                    }
                }

                QString prefix;
                if (child.hasAttribute(m_strings.ATTRIBUTE_PREFIX))
                    prefix = child.attribute(m_strings.ATTRIBUTE_PREFIX);
                if (!prefix.startsWith(slash))
                    prefix.prepend(slash);
                if (!prefix.endsWith(slash))
                    prefix += slash;

                for (QDomNode res = child.firstChild(); !res.isNull(); res = res.nextSibling()) {
                    if (res.isElement() && res.toElement().tagName() == m_strings.TAG_FILE) {

                        QString fileName(res.firstChild().toText().data());
                        if (fileName.isEmpty()) {
                            const QString msg = QString::fromUtf8("RCC: Warning: Null node in XML of '%1'\n").arg(fname);
                            m_errorDevice->write(msg.toUtf8());
                        }
                        QString alias;
                        if (res.toElement().hasAttribute(m_strings.ATTRIBUTE_ALIAS))
                            alias = res.toElement().attribute(m_strings.ATTRIBUTE_ALIAS);
                        else
                            alias = fileName;

                        int compressLevel = m_compressLevel;
                        if (res.toElement().hasAttribute(m_strings.ATTRIBUTE_COMPRESS))
                            compressLevel = res.toElement().attribute(m_strings.ATTRIBUTE_COMPRESS).toInt();
                        int compressThreshold = m_compressThreshold;
                        if (res.toElement().hasAttribute(m_strings.ATTRIBUTE_THRESHOLD))
                            compressThreshold = res.toElement().attribute(m_strings.ATTRIBUTE_THRESHOLD).toInt();

                        // Special case for -no-compress. Overrides all other settings.
                        if (m_compressLevel == -2)
                            compressLevel = 0;

                        alias = QDir::cleanPath(alias);
                        while (alias.startsWith(QLatin1String("../")))
                            alias.remove(0, 3);
                        alias = QDir::cleanPath(m_resourceRoot) + prefix + alias;

                        QString absFileName = fileName;
                        if (QDir::isRelativePath(absFileName))
                            absFileName.prepend(currentPath);
                        QFileInfo file(absFileName);
                        if (!file.exists()) {
                            m_failedResources.push_back(absFileName);
                            const QString msg = QString::fromUtf8("RCC: Error in '%1': Cannot find file '%2'\n").arg(fname).arg(fileName);
                            m_errorDevice->write(msg.toUtf8());
                            if (ignoreErrors)
                                continue;
                            else
                                return false;
                        } else if (file.isFile()) {
                            const bool arc =
                                addFile(alias,
                                        RCCFileInfo(alias.section(slash, -1),
                                                    file,
                                                    language,
                                                    country,
                                                    RCCFileInfo::NoFlags,
                                                    compressLevel,
                                                    compressThreshold)
                                        );
                            if (!arc)
                                m_failedResources.push_back(absFileName);
                        } else {
                            QDir dir;
                            if (file.isDir()) {
                                dir.setPath(file.filePath());
                            } else {
                                dir.setPath(file.path());
                                dir.setNameFilters(QStringList(file.fileName()));
                                if (alias.endsWith(file.fileName()))
                                    alias = alias.left(alias.length()-file.fileName().length());
                            }
                            if (!alias.endsWith(slash))
                                alias += slash;
                            QDirIterator it(dir, QDirIterator::FollowSymlinks|QDirIterator::Subdirectories);
                            while (it.hasNext()) {
                                it.next();
                                QFileInfo child(it.fileInfo());
                                if (child.fileName() != QLatin1String(".") && child.fileName() != QLatin1String("..")) {
                                    const bool arc =
                                        addFile(alias + child.fileName(),
                                                RCCFileInfo(child.fileName(),
                                                            child,
                                                            language,
                                                            country,
                                                            RCCFileInfo::NoFlags,
                                                            compressLevel,
                                                            compressThreshold)
                                                );
                                    if (!arc)
                                        m_failedResources.push_back(child.fileName());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (m_root == 0) {
        const QString msg = QString::fromUtf8("RCC: Warning: No resources in '%1'.\n").arg(fname);
        m_errorDevice->write(msg.toUtf8());
        if (!ignoreErrors && m_format == Binary) {
            // create dummy entry, otherwise loading qith QResource will crash
            m_root = new RCCFileInfo(QString(), QFileInfo(),
                    QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);
        }
    }

    return true;
}

bool RCCResourceLibrary::addFile(const QString &alias, const RCCFileInfo &file)
{
    Q_ASSERT(m_errorDevice);
    if (file.m_fileInfo.size() > 0xffffffff) {
        const QString msg = QString::fromUtf8("File too big: %1\n").arg(file.m_fileInfo.absoluteFilePath());
        m_errorDevice->write(msg.toUtf8());
        return false;
    }
    if (!m_root)
        m_root = new RCCFileInfo(QString(), QFileInfo(), QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);

    RCCFileInfo *parent = m_root;
    const QStringList nodes = alias.split(QLatin1Char('/'));
    for (int i = 1; i < nodes.size()-1; ++i) {
        const QString node = nodes.at(i);
        if (node.isEmpty())
            continue;
        if (!parent->m_children.contains(node)) {
            RCCFileInfo *s = new RCCFileInfo(node, QFileInfo(), QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);
            s->m_parent = parent;
            parent->m_children.insert(node, s);
            parent = s;
        } else {
            parent = parent->m_children[node];
        }
    }

    const QString filename = nodes.at(nodes.size()-1);
    RCCFileInfo *s = new RCCFileInfo(file);
    s->m_parent = parent;
    if (parent->m_children.contains(filename)) {
        foreach (const QString &fileName, m_fileNames)
            qWarning("%s: Warning: potential duplicate alias detected: '%s'",
                     qPrintable(fileName), qPrintable(filename));
        }
    parent->m_children.insertMulti(filename, s);
    return true;
}

void RCCResourceLibrary::reset()
{
     if (m_root) {
        delete m_root;
        m_root = 0;
    }
    m_errorDevice = 0;
    m_failedResources.clear();
}


bool RCCResourceLibrary::readFiles(bool ignoreErrors, QIODevice &errorDevice)
{
    reset();
    m_errorDevice = &errorDevice;
    //read in data
    if (m_verbose) {
        const QString msg = QString::fromUtf8("Processing %1 files [%2]\n")
            .arg(m_fileNames.size()).arg(static_cast<int>(ignoreErrors));
        m_errorDevice->write(msg.toUtf8());
    }
    for (int i = 0; i < m_fileNames.size(); ++i) {
        QFile fileIn;
        QString fname = m_fileNames.at(i);
        QString pwd;
        if (fname == QLatin1String("-")) {
            fname = QLatin1String("(stdin)");
            pwd = QDir::currentPath();
            fileIn.setFileName(fname);
            if (!fileIn.open(stdin, QIODevice::ReadOnly)) {
                m_errorDevice->write(msgOpenReadFailed(fname, fileIn.errorString()).toUtf8());
                return false;
            }
        } else {
            pwd = QFileInfo(fname).path();
            fileIn.setFileName(fname);
            if (!fileIn.open(QIODevice::ReadOnly)) {
                m_errorDevice->write(msgOpenReadFailed(fname, fileIn.errorString()).toUtf8());
                return false;
            }
        }
        if (m_verbose) {
            const QString msg = QString::fromUtf8("Interpreting %1\n").arg(fname);
            m_errorDevice->write(msg.toUtf8());
        }

        if (!interpretResourceFile(&fileIn, fname, pwd, ignoreErrors))
            return false;
    }
    return true;
}

QStringList RCCResourceLibrary::dataFiles() const
{
    QStringList ret;
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return ret;
    pending.push(m_root);
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            ret.append(child->m_fileInfo.filePath());
        }
    }
    return ret;
}

// Determine map of resource identifier (':/newPrefix/images/p1.png') to file via recursion
static void resourceDataFileMapRecursion(const RCCFileInfo *m_root, const QString &path, RCCResourceLibrary::ResourceDataFileMap &m)
{
    typedef QHash<QString, RCCFileInfo*>::const_iterator ChildConstIterator;
    const QChar slash = QLatin1Char('/');
    const ChildConstIterator cend = m_root->m_children.constEnd();
    for (ChildConstIterator it = m_root->m_children.constBegin(); it != cend; ++it) {
        const RCCFileInfo *child = it.value();
        QString childName = path;
        childName += slash;
        childName += child->m_name;
        if (child->m_flags & RCCFileInfo::Directory) {
            resourceDataFileMapRecursion(child, childName, m);
        } else {
            m.insert(childName, child->m_fileInfo.filePath());
        }
    }
}

RCCResourceLibrary::ResourceDataFileMap RCCResourceLibrary::resourceDataFileMap() const
{
    ResourceDataFileMap rc;
    if (m_root)
        resourceDataFileMapRecursion(m_root, QString(QLatin1Char(':')),  rc);
    return rc;
}

bool RCCResourceLibrary::output(QIODevice &outDevice, QIODevice &errorDevice)
{
    m_errorDevice = &errorDevice;
    //write out
    if (m_verbose)
        m_errorDevice->write("Outputting code\n");
    if (!writeHeader()) {
        m_errorDevice->write("Could not write header\n");
        return false;
    }
    if (m_root) {
        if (!writeDataBlobs()) {
            m_errorDevice->write("Could not write data blobs.\n");
            return false;
        }
        if (!writeDataNames()) {
            m_errorDevice->write("Could not write file names\n");
            return false;
        }
        if (!writeDataStructure()) {
            m_errorDevice->write("Could not write data tree\n");
            return false;
        }
    }
    if (!writeInitializer()) {
        m_errorDevice->write("Could not write footer\n");
        return false;
    }
    outDevice.write(m_out, m_out.size());
    return true;
}

void RCCResourceLibrary::writeHex(quint8 tmp)
{
    const char * const digits = "0123456789abcdef";
    writeChar('0');
    writeChar('x');
    if (tmp < 16) {
        writeChar(digits[tmp]);
    } else {
        writeChar(digits[tmp >> 4]);
        writeChar(digits[tmp & 0xf]);
    }
    writeChar(',');
}

void RCCResourceLibrary::writeNumber2(quint16 number)
{
    if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 8);
        writeHex(number);
    }
}

void RCCResourceLibrary::writeNumber4(quint32 number)
{
    if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 24);
        writeChar(number >> 16);
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 24);
        writeHex(number >> 16);
        writeHex(number >> 8);
        writeHex(number);
    }
}

bool RCCResourceLibrary::writeHeader()
{
    if (m_format == C_Code) {
        writeString("/****************************************************************************\n");
        writeString("** Resource object code\n");
        writeString("**\n");
        writeString("** Created: ");
        writeByteArray(QDateTime::currentDateTime().toString().toLatin1());
        writeString("\n**      by: The Resource Compiler for Qt version ");
        writeByteArray(QT_VERSION_STR);
        writeString("\n**\n");
        writeString("** WARNING! All changes made in this file will be lost!\n");
        writeString( "*****************************************************************************/\n\n");
        writeString("#include <QtCore/qglobal.h>\n\n");
    } else if (m_format == Binary) {
        writeString("qres");
        writeNumber4(0);
        writeNumber4(0);
        writeNumber4(0);
        writeNumber4(0);
    }
    return true;
}

bool RCCResourceLibrary::writeDataBlobs()
{
    Q_ASSERT(m_errorDevice);
    if (m_format == C_Code)
        writeString("static const unsigned char qt_resource_data[] = {\n");
    else if (m_format == Binary)
        m_dataOffset = m_out.size();
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    pending.push(m_root);
    qint64 offset = 0;
    QString errorMessage;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            else {
                offset = child->writeDataBlob(*this, offset, &errorMessage);
                if (offset == 0)
                    m_errorDevice->write(errorMessage.toUtf8());
            }
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");
    return true;
}

bool RCCResourceLibrary::writeDataNames()
{
    if (m_format == C_Code)
        writeString("static const unsigned char qt_resource_name[] = {\n");
    else if (m_format == Binary)
        m_namesOffset = m_out.size();

    QHash<QString, int> names;
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    pending.push(m_root);
    qint64 offset = 0;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            if (names.contains(child->m_name)) {
                child->m_nameOffset = names.value(child->m_name);
            } else {
                names.insert(child->m_name, offset);
                offset = child->writeDataName(*this, offset);
            }
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");
    return true;
}

static bool qt_rcc_compare_hash(const RCCFileInfo *left, const RCCFileInfo *right)
{
    return qHash(left->m_name) < qHash(right->m_name);
}

bool RCCResourceLibrary::writeDataStructure()
{
    if (m_format == C_Code)
        writeString("static const unsigned char qt_resource_struct[] = {\n");
    else if (m_format == Binary)
        m_treeOffset = m_out.size();
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    //calculate the child offsets (flat)
    pending.push(m_root);
    int offset = 1;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        file->m_childOffset = offset;

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> m_children = file->m_children.values();
        qSort(m_children.begin(), m_children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for (int i = 0; i < m_children.size(); ++i) {
            RCCFileInfo *child = m_children.at(i);
            ++offset;
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }

    //write out the structure (ie iterate again!)
    pending.push(m_root);
    m_root->writeDataInfo(*this);
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> m_children = file->m_children.values();
        qSort(m_children.begin(), m_children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for (int i = 0; i < m_children.size(); ++i) {
            RCCFileInfo *child = m_children.at(i);
            child->writeDataInfo(*this);
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");

    return true;
}

void RCCResourceLibrary::writeMangleNamespaceFunction(const QByteArray &name)
{
    if (m_useNameSpace) {
        writeString("QT_MANGLE_NAMESPACE(");
        writeByteArray(name);
        writeChar(')');
    } else {
        writeByteArray(name);
    }
}

void RCCResourceLibrary::writeAddNamespaceFunction(const QByteArray &name)
{
    if (m_useNameSpace) {
        writeString("QT_PREPEND_NAMESPACE(");
        writeByteArray(name);
        writeChar(')');
    } else {
        writeByteArray(name);
    }
}

bool RCCResourceLibrary::writeInitializer()
{
    if (m_format == C_Code) {
        //write("\nQT_BEGIN_NAMESPACE\n");
        QString initName = m_initName;
        if (!initName.isEmpty()) {
            initName.prepend(QLatin1Char('_'));
            initName.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_]")), QLatin1String("_"));
        }

        //init
        if (m_useNameSpace)
            writeString("QT_BEGIN_NAMESPACE\n\n");
        if (m_root) {
            writeString("extern Q_CORE_EXPORT bool qRegisterResourceData\n    "
                "(int, const unsigned char *, "
                "const unsigned char *, const unsigned char *);\n\n");
            writeString("extern Q_CORE_EXPORT bool qUnregisterResourceData\n    "
                "(int, const unsigned char *, "
                "const unsigned char *, const unsigned char *);\n\n");
        }
        if (m_useNameSpace)
            writeString("QT_END_NAMESPACE\n\n\n");
        QString initResources = QLatin1String("qInitResources");
        initResources += initName;
        writeString("int ");
        writeMangleNamespaceFunction(initResources.toLatin1());
        writeString("()\n{\n");

        if (m_root) {
            writeString("    ");
            writeAddNamespaceFunction("qRegisterResourceData");
            writeString("\n        (0x01, qt_resource_struct, "
                       "qt_resource_name, qt_resource_data);\n");
        }
        writeString("    return 1;\n");
        writeString("}\n\n");
        writeString("Q_CONSTRUCTOR_FUNCTION(");
        writeMangleNamespaceFunction(initResources.toLatin1());
        writeString(")\n\n");

        //cleanup
        QString cleanResources = QLatin1String("qCleanupResources");
        cleanResources += initName;
        writeString("int ");
        writeMangleNamespaceFunction(cleanResources.toLatin1());
        writeString("()\n{\n");
        if (m_root) {
            writeString("    ");
            writeAddNamespaceFunction("qUnregisterResourceData");
            writeString("\n       (0x01, qt_resource_struct, "
                      "qt_resource_name, qt_resource_data);\n");
        }
        writeString("    return 1;\n");
        writeString("}\n\n");
        writeString("Q_DESTRUCTOR_FUNCTION(");
        writeMangleNamespaceFunction(cleanResources.toLatin1());
        writeString(")\n\n");
    } else if (m_format == Binary) {
        int i = 4;
        char *p = m_out.data();
        p[i++] = 0; // 0x01
        p[i++] = 0;
        p[i++] = 0;
        p[i++] = 1;

        p[i++] = (m_treeOffset >> 24) & 0xff;
        p[i++] = (m_treeOffset >> 16) & 0xff;
        p[i++] = (m_treeOffset >>  8) & 0xff;
        p[i++] = (m_treeOffset >>  0) & 0xff;

        p[i++] = (m_dataOffset >> 24) & 0xff;
        p[i++] = (m_dataOffset >> 16) & 0xff;
        p[i++] = (m_dataOffset >>  8) & 0xff;
        p[i++] = (m_dataOffset >>  0) & 0xff;

        p[i++] = (m_namesOffset >> 24) & 0xff;
        p[i++] = (m_namesOffset >> 16) & 0xff;
        p[i++] = (m_namesOffset >>  8) & 0xff;
        p[i++] = (m_namesOffset >>  0) & 0xff;
    }
    return true;
}

QT_END_NAMESPACE

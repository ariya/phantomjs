/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Note: A copy of this file is used in Qt Designer (qttools/src/designer/src/lib/shared/rcc_p.h)

#ifndef RCC_H
#define RCC_H

#include <qstringlist.h>
#include <qhash.h>
#include <qstring.h>

QT_BEGIN_NAMESPACE

class RCCFileInfo;
class QIODevice;
class QTextStream;


class RCCResourceLibrary
{
    RCCResourceLibrary(const RCCResourceLibrary &);
    RCCResourceLibrary &operator=(const RCCResourceLibrary &);

public:
    RCCResourceLibrary();
    ~RCCResourceLibrary();

    bool output(QIODevice &outDevice, QIODevice &tempDevice, QIODevice &errorDevice);

    bool readFiles(bool ignoreErrors, QIODevice &errorDevice);

    enum Format { Binary, C_Code, Pass1, Pass2 };
    void setFormat(Format f) { m_format = f; }
    Format format() const { return m_format; }

    void setInputFiles(const QStringList &files) { m_fileNames = files; }
    QStringList inputFiles() const { return m_fileNames; }

    QStringList dataFiles() const;

    // Return a map of resource identifier (':/newPrefix/images/p1.png') to file.
    typedef QHash<QString, QString> ResourceDataFileMap;
    ResourceDataFileMap resourceDataFileMap() const;

    void setVerbose(bool b) { m_verbose = b; }
    bool verbose() const { return m_verbose; }

    void setInitName(const QString &name) { m_initName = name; }
    QString initName() const { return m_initName; }

    void setOutputName(const QString &name) { m_outputName = name; }
    QString outputName() const { return m_outputName; }

    void setCompressLevel(int c) { m_compressLevel = c; }
    int compressLevel() const { return m_compressLevel; }

    void setCompressThreshold(int t) { m_compressThreshold = t; }
    int compressThreshold() const { return m_compressThreshold; }

    void setResourceRoot(const QString &root) { m_resourceRoot = root; }
    QString resourceRoot() const { return m_resourceRoot; }

    void setUseNameSpace(bool v) { m_useNameSpace = v; }
    bool useNameSpace() const { return m_useNameSpace; }

    QStringList failedResources() const { return m_failedResources; }

private:
    struct Strings {
        Strings();
        const QString TAG_RCC;
        const QString TAG_RESOURCE;
        const QString TAG_FILE;
        const QString ATTRIBUTE_LANG;
        const QString ATTRIBUTE_PREFIX;
        const QString ATTRIBUTE_ALIAS;
        const QString ATTRIBUTE_THRESHOLD;
        const QString ATTRIBUTE_COMPRESS;
    };
    friend class RCCFileInfo;
    void reset();
    bool addFile(const QString &alias, const RCCFileInfo &file);
    bool interpretResourceFile(QIODevice *inputDevice, const QString &file,
        QString currentPath = QString(), bool ignoreErrors = false);
    bool writeHeader();
    bool writeDataBlobs();
    bool writeDataNames();
    bool writeDataStructure();
    bool writeInitializer();
    void writeMangleNamespaceFunction(const QByteArray &name);
    void writeAddNamespaceFunction(const QByteArray &name);
    void writeHex(quint8 number);
    void writeNumber2(quint16 number);
    void writeNumber4(quint32 number);
    void writeChar(char c) { m_out.append(c); }
    void writeByteArray(const QByteArray &);
    void write(const char *, int len);

    const Strings m_strings;
    RCCFileInfo *m_root;
    QStringList m_fileNames;
    QString m_resourceRoot;
    QString m_initName;
    QString m_outputName;
    Format m_format;
    bool m_verbose;
    int m_compressLevel;
    int m_compressThreshold;
    int m_treeOffset;
    int m_namesOffset;
    int m_dataOffset;
    bool m_useNameSpace;
    QStringList m_failedResources;
    QIODevice *m_errorDevice;
    QIODevice *m_outDevice;
    QByteArray m_out;
};

QT_END_NAMESPACE

#endif // RCC_H

/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utilities of the Qt Toolkit.
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

#ifndef SPECPARSER_H
#define SPECPARSER_H

#include <QStringList>
#include <QVariant>

class QTextStream;

struct Version {
    int major;
    int minor;
};

inline bool operator == (const Version &lhs, const Version &rhs)
{
    return (lhs.major == rhs.major && lhs.minor == rhs.minor);
}

inline bool operator < (const Version &lhs, const Version &rhs)
{
    if (lhs.major != rhs.major)
        return (lhs.major < rhs.major);
    else
        return (lhs.minor < rhs.minor);
}

inline bool operator > (const Version &lhs, const Version &rhs)
{
    if (lhs.major != rhs.major)
        return (lhs.major > rhs.major);
    else
        return (lhs.minor > rhs.minor);
}

inline uint qHash(const Version &v)
{
    return qHash(v.major * 100 + v.minor * 10);
}

struct VersionProfile
{
    enum OpenGLProfile {
        CoreProfile = 0,
        CompatibilityProfile
    };

    inline bool hasProfiles() const
    {
        return ( version.major > 3
                 || (version.major == 3 && version.minor > 1));
    }

    Version version;
    OpenGLProfile profile;
};

inline bool operator == (const VersionProfile &lhs, const VersionProfile &rhs)
{
    if (lhs.profile != rhs.profile)
        return false;
    return lhs.version == rhs.version;
}

inline bool operator < (const VersionProfile &lhs, const VersionProfile &rhs)
{
    if (lhs.profile != rhs.profile)
        return (lhs.profile < rhs.profile);
    return (lhs.version < rhs.version);
}

inline uint qHash(const VersionProfile &v)
{
    return qHash(static_cast<int>(v.profile * 1000) + v.version.major * 100 + v.version.minor * 10);
}

struct Argument
{
    enum Direction {
        In = 0,
        Out
    };

    enum Mode {
        Value = 0,
        Array,
        Reference
    };

    QString type;
    QString name;
    Direction direction;
    Mode mode;
};

struct Function
{
    QString returnType;
    QString name;
    QList<Argument> arguments;
};

typedef QList<Function> FunctionList;
typedef QMap<VersionProfile, FunctionList> FunctionCollection;

class SpecParser
{
public:
    explicit SpecParser();

    QString specFileName() const
    {
        return m_specFileName;
    }

    QString typeMapFileName() const
    {
        return m_typeMapFileName;
    }

    QList<Version> versions() const {return m_versions;}
    QList<VersionProfile> versionProfiles() const {return m_functions.uniqueKeys();}

    QList<Function> functionsForVersion(const VersionProfile &v) const
    {
        return m_functions.values(v);
    }

    QStringList extensions() const
    {
        return QStringList(m_extensionFunctions.uniqueKeys());
    }

    QList<Function> functionsForExtension(const QString &extension)
    {
        return m_extensionFunctions.values(extension);
    }

    void setSpecFileName(QString arg)
    {
        m_specFileName = arg;
    }

    void setTypeMapFileName(QString arg)
    {
        m_typeMapFileName = arg;
    }

    void parse();

protected:
    bool parseTypeMap();
    void parseEnums();
    void parseFunctions(QTextStream &stream);
    bool inDeprecationException(const QString &functionName) const;

private:
    QString m_specFileName;
    QString m_typeMapFileName;

    QMap<QString, QString> m_typeMap;
    QMultiMap<VersionProfile, Function> m_functions;

    QList<Version> m_versions;

    // Extension support
    QMultiMap<QString, Function> m_extensionFunctions;
};

#endif // SPECPARSER_H

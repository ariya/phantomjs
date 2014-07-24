/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CE_SDK_HANDLER_INCL
#define CE_SDK_HANDLER_INCL

#include <qstringlist.h>
#include <qdir.h>

QT_BEGIN_NAMESPACE

class CeSdkInfo
{
public:
    CeSdkInfo();
    inline QString name() const { return m_name; }
    inline QString binPath() const { return m_bin; }
    inline QString includePath() const { return m_include; }
    inline QString libPath() const { return m_lib; }
    inline bool isValid() const;
    inline int majorVersion() const { return m_major; }
    inline int minorVersion() const { return m_minor; }
    inline bool isSupported() const { return m_major >= 5; }
private:
    friend class CeSdkHandler;
    QString m_name;
    QString m_bin;
    QString m_include;
    QString m_lib;
    int m_major;
    int m_minor;
};

bool CeSdkInfo::isValid() const
{
    return !m_name.isEmpty() &&
           !m_bin.isEmpty() &&
           !m_include.isEmpty() &&
           !m_lib.isEmpty();
}

class CeSdkHandler
{
public:
    CeSdkHandler();
    bool parse();
    inline QList<CeSdkInfo> listAll() const { return m_list; }
private:
    inline QString fixPaths(QString path) const;
    QList<CeSdkInfo> m_list;
    QString m_vcInstallDir;
};

QT_END_NAMESPACE

#endif

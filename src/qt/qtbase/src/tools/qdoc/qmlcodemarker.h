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

/*
  qmlcodemarker.h
*/

#ifndef QMLCODEMARKER_H
#define QMLCODEMARKER_H

#include "qqmljsastfwd_p.h"
#include "cppcodemarker.h"

QT_BEGIN_NAMESPACE

class QmlCodeMarker : public CppCodeMarker
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::QmlCodeMarker)

public:
    QmlCodeMarker();
    ~QmlCodeMarker();

    virtual bool recognizeCode(const QString &code);
    virtual bool recognizeExtension(const QString &ext);
    virtual bool recognizeLanguage(const QString &language);
    virtual Atom::Type atomType() const;
    virtual QString markedUpCode(const QString &code,
                                 const Node *relative,
                                 const Location &location);

    virtual QString markedUpName(const Node *node);
    virtual QString markedUpFullName(const Node *node, const Node *relative);
    virtual QString markedUpIncludes(const QStringList &includes);
    virtual QString functionBeginRegExp(const QString &funcName);
    virtual QString functionEndRegExp(const QString &funcName);

    /* Copied from src/declarative/qml/qdeclarativescriptparser.cpp */
    QList<QQmlJS::AST::SourceLocation> extractPragmas(QString &script);

private:
    QString addMarkUp(const QString &code, const Node *relative,
                      const Location &location);
};

QT_END_NAMESPACE

#endif

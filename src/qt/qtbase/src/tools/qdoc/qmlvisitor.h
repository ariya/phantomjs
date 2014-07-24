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

#ifndef QMLVISITOR_H
#define QMLVISITOR_H

#include <qstring.h>
#include "qqmljsastvisitor_p.h"
#include "node.h"

QT_BEGIN_NAMESPACE

struct QmlPropArgs
{
    QString type_;
    QString module_;
    QString component_;
    QString name_;

    void clear() {
        type_.clear();
        module_.clear();
        component_.clear();
        name_.clear();
    }
};

class QmlDocVisitor : public QQmlJS::AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::QmlDocVisitor)

public:
    QmlDocVisitor(const QString &filePath,
                  const QString &code,
                  QQmlJS::Engine *engine,
                  const QSet<QString> &commands,
                  const QSet<QString> &topics);
    virtual ~QmlDocVisitor();

    bool visit(QQmlJS::AST::UiImport *import);
    void endVisit(QQmlJS::AST::UiImport *definition);

    bool visit(QQmlJS::AST::UiObjectDefinition *definition);
    void endVisit(QQmlJS::AST::UiObjectDefinition *definition);

    bool visit(QQmlJS::AST::UiPublicMember *member);
    void endVisit(QQmlJS::AST::UiPublicMember *definition);

    virtual bool visit(QQmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QQmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QQmlJS::AST::UiArrayBinding *);
    virtual bool visit(QQmlJS::AST::UiArrayBinding *);

    bool visit(QQmlJS::AST::IdentifierPropertyName *idproperty);

    bool visit(QQmlJS::AST::FunctionDeclaration *);
    void endVisit(QQmlJS::AST::FunctionDeclaration *);

    bool visit(QQmlJS::AST::UiScriptBinding *);
    void endVisit(QQmlJS::AST::UiScriptBinding *);

    bool visit(QQmlJS::AST::UiQualifiedId *);
    void endVisit(QQmlJS::AST::UiQualifiedId *);

private:
    QString getFullyQualifiedId(QQmlJS::AST::UiQualifiedId *id);
    QQmlJS::AST::SourceLocation precedingComment(quint32 offset) const;
    bool applyDocumentation(QQmlJS::AST::SourceLocation location, Node *node);
    void applyMetacommands(QQmlJS::AST::SourceLocation location, Node* node, Doc& doc);
    bool splitQmlPropertyArg(const Doc& doc,
                             const QString& arg,
                             QmlPropArgs& qpa);

    QQmlJS::Engine *engine;
    quint32 lastEndOffset;
    quint32 nestingLevel;
    QString filePath_;
    QString name;
    QString document;
    ImportList importList;
    QSet<QString> commands_;
    QSet<QString> topics_;
    QSet<quint32> usedComments;
    InnerNode *current;
};

QT_END_NAMESPACE

#endif

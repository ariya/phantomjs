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
  tree.h
*/

#ifndef TREE_H
#define TREE_H

#include "node.h"

QT_BEGIN_NAMESPACE

class QStringList;
class QDocDatabase;

class Tree
{
 private:
    friend class QDocDatabase;

    typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
    typedef QMap<PropertyNode*, RoleMap> PropertyMap;

    struct InheritanceBound
    {
        Node::Access access;
        QStringList basePath;
        QString dataTypeWithTemplateArgs;
        InnerNode* parent;

      InheritanceBound() : access(Node::Public) { }
      InheritanceBound(Node::Access access0,
                       const QStringList& basePath0,
                       const QString& dataTypeWithTemplateArgs0,
                       InnerNode* parent)
          : access(access0), basePath(basePath0),
            dataTypeWithTemplateArgs(dataTypeWithTemplateArgs0),
            parent(parent) { }
    };

    Tree(QDocDatabase* qdb);
    ~Tree();

    EnumNode* findEnumNode(const QStringList& path, Node* start = 0);
    ClassNode* findClassNode(const QStringList& path, Node* start = 0) const;
    QmlClassNode* findQmlTypeNode(const QStringList& path);
    NamespaceNode* findNamespaceNode(const QStringList& path) const;

    Node* findNodeByNameAndType(const QStringList& path,
                                Node::Type type,
                                Node::SubType subtype,
                                Node* start,
                                bool acceptCollision = false);

    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            Node* start,
                            Node::Type type,
                            Node::SubType subtype,
                            bool acceptCollision = false) const;

    const Node* findNode(const QStringList &path,
                         const Node* relative = 0,
                         int findFlags = 0,
                         const Node* self=0) const;

    const Node* findNode(const QStringList& path,
                         const Node* start,
                         int findFlags,
                         const Node* self,
                         bool qml) const;

    NameCollisionNode* checkForCollision(const QString& name) const;
    NameCollisionNode* findCollisionNode(const QString& name) const;
    FunctionNode *findFunctionNode(const QStringList &path,
                                   Node *relative = 0,
                                   int findFlags = 0);
    FunctionNode *findFunctionNode(const QStringList &parentPath,
                                   const FunctionNode *clone,
                                   Node *relative = 0,
                                   int findFlags = 0);
    void addBaseClass(ClassNode *subclass,
                      Node::Access access,
                      const QStringList &basePath,
                      const QString &dataTypeWithTemplateArgs,
                      InnerNode *parent);
    void addPropertyFunction(PropertyNode *property,
                             const QString &funcName,
                             PropertyNode::FunctionRole funcRole);
    void resolveInheritance(NamespaceNode *rootNode = 0);
    void resolveProperties();
    void resolveCppToQmlLinks();
    void fixInheritance(NamespaceNode *rootNode = 0);
    NamespaceNode *root() { return &root_; }

    const FunctionNode *findFunctionNode(const QStringList &path,
                                         const Node *relative = 0,
                                         int findFlags = 0) const;
    const FunctionNode *findFunctionNode(const QStringList &parentPath,
                                         const FunctionNode *clone,
                                         const Node *relative = 0,
                                         int findFlags = 0) const;
    const NamespaceNode *root() const { return &root_; }

    void resolveInheritance(int pass, ClassNode *classe);
    FunctionNode *findVirtualFunctionInBaseClasses(ClassNode *classe,
                                                   FunctionNode *clone);
    void fixPropertyUsingBaseClasses(ClassNode *classe, PropertyNode *property);
    NodeList allBaseClasses(const ClassNode *classe) const;

private:
    QDocDatabase* qdb_;
    NamespaceNode root_;
    QMap<ClassNode* , QList<InheritanceBound> > unresolvedInheritanceMap;
    PropertyMap unresolvedPropertyMap;
};

QT_END_NAMESPACE

#endif

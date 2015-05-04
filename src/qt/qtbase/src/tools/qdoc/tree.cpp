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

#include "doc.h"
#include "htmlgenerator.h"
#include "location.h"
#include "node.h"
#include "text.h"
#include "tree.h"
#include "qdocdatabase.h"
#include <limits.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
  \class Tree

  This class constructs and maintains a tree of instances of
  the subclasses of Node.

  This class is now private. Only class QDocDatabase has access.
  Please don't change this. If you must access class Tree, do it
  though the pointer to the singleton QDocDatabase.

  Tree is being converted to a forest. A static member provides a
  map of Tree* values with the module names as the keys. There is
  one Tree in the map for each index file read, and there is one
  tree that is not in the map for the module whose documentation
  is being generated.
 */

/*!
  Constructs a Tree. \a qdb is the pointer to the singleton
  qdoc database that is constructing the tree. This might not
  be necessary, and it might be removed later.
 */
Tree::Tree(const QString& module, QDocDatabase* qdb)
    : module_(module), qdb_(qdb), root_(0, QString())
{
    root_.setModuleName(module_);
    root_.setTree(this);
}

/*!
  Destroys the Tree. The root node is a data member
  of this object, so its destructor is called. The
  destructor of each child node is called, and these
  destructors are recursive. Thus the entire tree is
  destroyed.

  There are two maps of targets, keywords, and contents.
  One map is indexed by ref, the other by title. The ref
  is just the canonical form of the title. Both maps
  use the same set of TargetRec objects as the values,
  so the destructor only deletes the values from one of
  the maps. Then it clears both maps.
 */
Tree::~Tree()
{
    TargetMap::iterator i = nodesByTargetRef_.begin();
    while (i != nodesByTargetRef_.end()) {
        delete i.value();
        ++i;
    }
    nodesByTargetRef_.clear();
    nodesByTargetTitle_.clear();
}

/* API members */

/*!
  Calls findClassNode() first with \a path and \a start. If
  it finds a node, the node is returned. If not, it calls
  findNamespaceNode() with the same parameters. The result
  is returned.
 */
Node* Tree::findNodeForInclude(const QStringList& path) const
{
    Node* n = findClassNode(path);
    if (!n)
        n = findNamespaceNode(path);
    return n;
}

/*!
  Find the C++ class node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a C++ class node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
ClassNode* Tree::findClassNode(const QStringList& path, const Node* start) const
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<ClassNode*>(findNodeRecursive(path, 0, start, Node::Class));
}

/*!
  Find the Namespace node named \a path. Begin the search at
  the root of the tree. Only a Namespace node named \a path
  is acceptible. If one is not found, 0 is returned.
 */
NamespaceNode* Tree::findNamespaceNode(const QStringList& path) const
{
    Node* start = const_cast<NamespaceNode*>(root());
    return static_cast<NamespaceNode*>(findNodeRecursive(path, 0, start, Node::Namespace));
}

/*!
  This function first ignores the \a clone node and searches
  for the parent node with \a parentPath. If that search is
  successful, it searches for a child node of the parent that
  matches the \a clone node. If it finds a node that is just
  like the \a clone, it returns a pointer to the found node.

  Apparently the search order is important here. Don't change
  it unless you know what you are doing, or you will introduce
  qdoc warnings.
 */
FunctionNode* Tree::findFunctionNode(const QStringList& parentPath, const FunctionNode* clone)
{
    const Node* parent = findNamespaceNode(parentPath);
    if (parent == 0)
        parent = findClassNode(parentPath, 0);
    if (parent == 0)
        parent = findNode(parentPath, 0, 0, Node::DontCare);
    if (parent == 0 || !parent->isInnerNode())
        return 0;
    return ((InnerNode*)parent)->findFunctionNode(clone);
}


/*!
  Find the Qml type node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a Qml type node named <\a path is
  acceptible. If one is not found, 0 is returned.
 */
QmlClassNode* Tree::findQmlTypeNode(const QStringList& path)
{
    /*
      If the path contains one or two double colons ("::"),
      check first to see if the first two path strings refer
      to a QML element. If they do, path[0] will be the QML
      module identifier, and path[1] will be the QML type.
      If the anser is yes, the reference identifies a QML
      class node.
    */
    if (path.size() >= 2 && !path[0].isEmpty()) {
        QmlClassNode* qcn = qdb_->findQmlType(path[0], path[1]);
        if (qcn)
            return qcn;
    }
    return static_cast<QmlClassNode*>(findNodeRecursive(path, 0, root(), Node::QmlType));
}

/*!
  This function begins searching the tree at \a relative for
  the \l {FunctionNode} {function node} identified by \a path.
  The \a findFlags are used to restrict the search. If a node
  that matches the \a path is found, it is returned. Otherwise,
  0 is returned. If \a relative is 0, the root of the tree is
  used as the starting point.
 */
const FunctionNode* Tree::findFunctionNode(const QStringList& path,
                                           const Node* relative,
                                           int findFlags,
                                           Node::Genus genus) const
{
    if (path.size() == 3 && !path[0].isEmpty() && (genus != Node::CPP)) {
        QmlClassNode* qcn = lookupQmlType(QString(path[0] + "::" + path[1]));
        if (!qcn) {
            QStringList p(path[1]);
            Node* n = findNodeByNameAndType(p, Node::QmlType);
            if (n && n->isQmlType())
                qcn = static_cast<QmlClassNode*>(n);
        }
        if (qcn)
            return static_cast<const FunctionNode*>(qcn->findFunctionNode(path[2]));
    }

    if (!relative)
        relative = root();
    else if (genus != Node::DontCare) {
        if (genus != relative->genus())
            relative = root();
    }

    do {
        const Node* node = relative;
        int i;

        for (i = 0; i < path.size(); ++i) {
            if (node == 0 || !node->isInnerNode())
                break;

            const Node* next;
            if (i == path.size() - 1)
                next = ((InnerNode*) node)->findFunctionNode(path.at(i));
            else
                next = ((InnerNode*) node)->findChildNode(path.at(i), genus);

            if (!next && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode*>(node));
                foreach (const Node* baseClass, baseClasses) {
                    if (i == path.size() - 1)
                        next = static_cast<const InnerNode*>(baseClass)->findFunctionNode(path.at(i));
                    else
                        next = static_cast<const InnerNode*>(baseClass)->findChildNode(path.at(i), genus);

                    if (next)
                        break;
                }
            }

            node = next;
        }
        if (node && i == path.size() && node->isFunction()) {
            // CppCodeParser::processOtherMetaCommand ensures that reimplemented
            // functions are private.
            const FunctionNode* func = static_cast<const FunctionNode*>(node);
            while (func->access() == Node::Private) {
                const FunctionNode* from = func->reimplementedFrom();
                if (from != 0) {
                    if (from->access() != Node::Private)
                        return from;
                    else
                        func = from;
                }
                else
                    break;
            }
            return func;
        }
        relative = relative->parent();
    } while (relative);

    return 0;
}

static NodeTypeList t;
static const NodeTypeList& relatesTypes()
{
    if (t.isEmpty()) {
        t.reserve(3);
        t.append(NodeTypePair(Node::Class, Node::NoSubType));
        t.append(NodeTypePair(Node::Namespace, Node::NoSubType));
        t.append(NodeTypePair(Node::Document, Node::HeaderFile));
    }
    return t;
}

/*!
  This function searches for the node specified by \a path.
  The matching node can be one of several different types
  including a C++ class, a C++ namespace, or a C++ header
  file.

  I'm not sure if it can be a QML type, but if that is a
  possibility, the code can easily accommodate it.

  If a matching node is found, a pointer to it is returned.
  Otherwise 0 is returned.
 */
InnerNode* Tree::findRelatesNode(const QStringList& path)
{
    Node* n = findNodeRecursive(path, 0, root(), relatesTypes());
    return ((n && n->isInnerNode()) ? static_cast<InnerNode*>(n) : 0);
}

/*!
 */
void Tree::addPropertyFunction(PropertyNode* property,
                               const QString& funcName,
                               PropertyNode::FunctionRole funcRole)
{
    unresolvedPropertyMap[property].insert(funcRole, funcName);
}

/*!
  This function resolves C++ inheritance and reimplementation
  settings for each C++ class node found in the tree beginning
  at \a n. It also calls itself recursively for each C++ class
  node or namespace node it encounters. For each child of \a n
  that is a class node, it calls resolveInheritanceHelper().

  This function does not resolve QML inheritance.
 */
void Tree::resolveInheritance(InnerNode* n)
{
    if (!n)
        n = root();

    for (int pass = 0; pass < 2; pass++) {
        NodeList::ConstIterator c = n->childNodes().constBegin();
        while (c != n->childNodes().constEnd()) {
            if ((*c)->type() == Node::Class) {
                resolveInheritanceHelper(pass, (ClassNode*)*c);
                resolveInheritance((ClassNode*)*c);
            }
            else if ((*c)->type() == Node::Namespace) {
                NamespaceNode* ns = static_cast<NamespaceNode*>(*c);
                resolveInheritance(ns);
            }
            ++c;
        }
    }
}

/*!
  This function is run twice for eachclass node \a cn in the
  tree. First it is run with \a pass set to 0 for each
 class node \a cn. Then it is run with \a pass set to 1 for
  eachclass node \a cn.

  In \a pass 0, all the base classes ofclass node \a cn are
  found and added to the base class list forclass node \a cn.

  In \a pass 1, each child ofclass node \a cn that is a function
  that is reimplemented from one of the base classes is marked
  as being reimplemented from that class.

  Some property node fixing up is also done in \a pass 1.
 */
void Tree::resolveInheritanceHelper(int pass, ClassNode* cn)
{
    if (pass == 0) {
        QList<RelatedClass>& bases = cn->baseClasses();
        QList<RelatedClass>::iterator b = bases.begin();
        while (b != bases.end()) {
            if (!(*b).node_) {
                Node* n = qdb_->findClassNode((*b).path_);
#if 0
                /*
                  If the node for the base class was not found,
                  the reason might be that the subclass is in a
                  namespace and the base class is in the same
                  namespace, but the base class name was not
                  qualified with the namespace name. That is the
                  case most of the time. Then restart the search
                  at the parent of the subclass node (the namespace
                  node) using the unqualified base class name.
                 */
                if (!n) {
                    InnerNode* parent = cn->parent();
                    n = findClassNode((*b).path_, parent);
                }
#endif
                if (n) {
                    ClassNode* bcn = static_cast<ClassNode*>(n);
                    (*b).node_ = bcn;
                    bcn->addDerivedClass((*b).access_, cn);
                }
            }
            ++b;
        }
    }
    else {
        NodeList::ConstIterator c = cn->childNodes().constBegin();
        while (c != cn->childNodes().constEnd()) {
            if ((*c)->type() == Node::Function) {
                FunctionNode* func = (FunctionNode*)* c;
                FunctionNode* from = findVirtualFunctionInBaseClasses(cn, func);
                if (from != 0) {
                    if (func->virtualness() == FunctionNode::NonVirtual)
                        func->setVirtualness(FunctionNode::ImpureVirtual);
                    func->setReimplementedFrom(from);
                }
            }
            else if ((*c)->type() == Node::Property)
                cn->fixPropertyUsingBaseClasses(static_cast<PropertyNode*>(*c));
            ++c;
        }
    }
}

/*!
 */
void Tree::resolveProperties()
{
    PropertyMap::ConstIterator propEntry;

    propEntry = unresolvedPropertyMap.constBegin();
    while (propEntry != unresolvedPropertyMap.constEnd()) {
        PropertyNode* property = propEntry.key();
        InnerNode* parent = property->parent();
        QString getterName = (*propEntry)[PropertyNode::Getter];
        QString setterName = (*propEntry)[PropertyNode::Setter];
        QString resetterName = (*propEntry)[PropertyNode::Resetter];
        QString notifierName = (*propEntry)[PropertyNode::Notifier];

        NodeList::ConstIterator c = parent->childNodes().constBegin();
        while (c != parent->childNodes().constEnd()) {
            if ((*c)->type() == Node::Function) {
                FunctionNode* function = static_cast<FunctionNode*>(*c);
                if (function->access() == property->access() &&
                        (function->status() == property->status() ||
                         function->doc().isEmpty())) {
                    if (function->name() == getterName) {
                        property->addFunction(function, PropertyNode::Getter);
                    }
                    else if (function->name() == setterName) {
                        property->addFunction(function, PropertyNode::Setter);
                    }
                    else if (function->name() == resetterName) {
                        property->addFunction(function, PropertyNode::Resetter);
                    }
                    else if (function->name() == notifierName) {
                        property->addSignal(function, PropertyNode::Notifier);
                    }
                }
            }
            ++c;
        }
        ++propEntry;
    }

    propEntry = unresolvedPropertyMap.constBegin();
    while (propEntry != unresolvedPropertyMap.constEnd()) {
        PropertyNode* property = propEntry.key();
        // redo it to set the property functions
        if (property->overriddenFrom())
            property->setOverriddenFrom(property->overriddenFrom());
        ++propEntry;
    }

    unresolvedPropertyMap.clear();
}

/*!
  For each QML class node that points to a C++ class node,
  follow its C++ class node pointer and set the C++ class
  node's QML class node pointer back to the QML class node.
 */
void Tree::resolveCppToQmlLinks()
{

    foreach (Node* child, root_.childNodes()) {
        if (child->isQmlType()) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
            ClassNode* cn = const_cast<ClassNode*>(qcn->classNode());
            if (cn)
                cn->setQmlElement(qcn);
        }
    }
}

/*!
 */
void Tree::fixInheritance(NamespaceNode* rootNode)
{
    if (!rootNode)
        rootNode = root();

    NodeList::ConstIterator c = rootNode->childNodes().constBegin();
    while (c != rootNode->childNodes().constEnd()) {
        if ((*c)->type() == Node::Class)
            static_cast<ClassNode*>(*c)->fixBaseClasses();
        else if ((*c)->type() == Node::Namespace) {
            NamespaceNode* ns = static_cast<NamespaceNode*>(*c);
            fixInheritance(ns);
        }
        ++c;
    }
}

/*!
 */
FunctionNode* Tree::findVirtualFunctionInBaseClasses(ClassNode* cn, FunctionNode* clone)
{
    const QList<RelatedClass>& rc = cn->baseClasses();
    QList<RelatedClass>::ConstIterator r = rc.constBegin();
    while (r != rc.constEnd()) {
        FunctionNode* func;
        if ((*r).node_) {
            if (((func = findVirtualFunctionInBaseClasses((*r).node_, clone)) != 0 ||
                 (func = (*r).node_->findFunctionNode(clone)) != 0)) {
                if (func->virtualness() != FunctionNode::NonVirtual)
                    return func;
            }
        }
        ++r;
    }
    return 0;
}

/*!
 */
NodeList Tree::allBaseClasses(const ClassNode* classNode) const
{
    NodeList result;
    foreach (const RelatedClass& r, classNode->baseClasses()) {
        if (r.node_) {
            result += r.node_;
            result += allBaseClasses(r.node_);
        }
    }
    return result;
}

/*!
  Find the node with the specified \a path name that is of
  the specified \a type and \a subtype. Begin the search at
  the \a start node. If the \a start node is 0, begin the
  search at the tree root. \a subtype is not used unless
  \a type is \c{Document}.
 */
Node* Tree::findNodeByNameAndType(const QStringList& path, Node::Type type) const
{
    return findNodeRecursive(path, 0, root(), type);
}

/*!
  Recursive search for a node identified by \a path. Each
  path element is a name. \a pathIndex specifies the index
  of the name in \a path to try to match. \a start is the
  node whose children shoulod be searched for one that has
  that name. Each time a match is found, increment the
  \a pathIndex and call this function recursively.

  If the end of the path is reached (i.e. if a matching
  node is found for each name in the \a path), the \a type
  must match the type of the last matching node, and if the
  type is \e{Document}, the \a subtype must match as well.

  If the algorithm is successful, the pointer to the final
  node is returned. Otherwise 0 is returned.
 */
Node* Tree::findNodeRecursive(const QStringList& path,
                              int pathIndex,
                              const Node* start,
                              Node::Type type) const
{
    if (!start || path.isEmpty())
        return 0; // no place to start, or nothing to search for.
    Node* node = const_cast<Node*>(start);
    if (start->isLeaf()) {
        if (pathIndex >= path.size())
            return node; // found a match.
        return 0; // premature leaf
    }

    InnerNode* current = static_cast<InnerNode*>(node);
    const NodeList& children = current->childNodes();
    const QString& name = path.at(pathIndex);
    for (int i=0; i<children.size(); ++i) {
        Node* n = children.at(i);
        if (!n)
            continue;
        if (n->isQmlPropertyGroup()) {
            if (type == Node::QmlProperty) {
                n = findNodeRecursive(path, pathIndex, n, type);
                if (n)
                    return n;
            }
        }
        else if (n->name() == name) {
            if (pathIndex+1 >= path.size()) {
                if ((n->type() == type) || (type == Node::NoType))
                    return n;
                continue;
            }
            else { // Search the children of n for the next name in the path.
                n = findNodeRecursive(path, pathIndex+1, n, type);
                if (n)
                    return n;
            }
        }
    }
    return 0;
}

/*!
  Recursive search for a node identified by \a path. Each
  path element is a name. \a pathIndex specifies the index
  of the name in \a path to try to match. \a start is the
  node whose children shoulod be searched for one that has
  that name. Each time a name match is found, increment the
  \a pathIndex and call this function recursively.

  If the end of the path is reached (i.e. if a matching
  node is found for each name in the \a path), test the
  matching node's type and subtype values against the ones
  listed in \a types. If a match is found there, return the
  pointer to the final node. Otherwise return 0.
 */
Node* Tree::findNodeRecursive(const QStringList& path,
                              int pathIndex,
                              Node* start,
                              const NodeTypeList& types) const
{
    if (!start || path.isEmpty())
        return 0;
    if (start->isLeaf())
        return ((pathIndex >= path.size()) ? start : 0);
    if (pathIndex >= path.size())
        return 0;

    InnerNode* current = static_cast<InnerNode*>(start);
    const NodeList& children = current->childNodes();
    for (int i=0; i<children.size(); ++i) {
        Node* n = children.at(i);
        if (n && n->name() == path.at(pathIndex)) {
            if (pathIndex+1 >= path.size()) {
                if (n->match(types))
                    return n;
            }
            else if (!n->isLeaf()) {
                n = findNodeRecursive(path, pathIndex+1, n, types);
                if (n)
                    return n;
            }
        }
    }
    return 0;
}

/*!
  Searches the tree for a node that matches the \a path plus
  the \a target. The search begins at \a start and moves up
  the parent chain from there, or, if \a start is 0, the search
  begins at the root.

  The \a flags can indicate whether to search base classes and/or
  the enum values in enum types. \a genus can be a further restriction
  on what kind of node is an acceptible match, i.e. CPP or QML.

  If a matching node is found, \a ref is an output parameter that
  is set to the HTML reference to use for the link.
 */
const Node* Tree::findNodeForTarget(const QStringList& path,
                                    const QString& target,
                                    const Node* start,
                                    int flags,
                                    Node::Genus genus,
                                    QString& ref) const
{
    const Node* node = 0;
    QString p;
    if (path.size() > 1)
        p = path.join(QString("::"));
    else {
        p = path.at(0);
        node = findDocNodeByTitle(p);
        if (node) {
            if (!target.isEmpty()) {
                ref = getRef(target, node);
                if (ref.isEmpty())
                    node = 0;
            }
            if (node)
                return node;
        }
    }
    node = findUnambiguousTarget(p, ref);
    if (node) {
        if (!target.isEmpty()) {
            ref = getRef(target, node);
            if (ref.isEmpty())
                node = 0;
        }
        if (node)
            return node;
    }

    const Node* current = start;
    if (!current)
        current = root();

    /*
      If the path contains one or two double colons ("::"),
      check first to see if the first two path strings refer
      to a QML element. If they do, path[0] will be the QML
      module identifier, and path[1] will be the QML type.
      If the answer is yes, the reference identifies a QML
      type node.
    */
    int path_idx = 0;
    if ((genus != Node::CPP) && (path.size() >= 2) && !path[0].isEmpty()) {
        QmlClassNode* qcn = lookupQmlType(QString(path[0] + "::" + path[1]));
        if (qcn) {
            current = qcn;
            if (path.size() == 2) {
                if (!target.isEmpty()) {
                    ref = getRef(target, current);
                    if (!ref.isEmpty())
                        return current;
                    else if (genus == Node::QML)
                        return 0;
                }
                else
                    return current;
            }
            path_idx = 2;
        }
    }

    while (current) {
        if (current->isInnerNode()) {
            const Node* node = matchPathAndTarget(path, path_idx, target, current, flags, genus, ref);
            if (node)
                return node;
        }
        current = current->parent();
        path_idx = 0;
    }
    return 0;
}

/*!
  First, the \a path is used to find a node. The \a path
  matches some part of the node's fully quallified name.
  If the \a target is not empty, it must match a target
  in the matching node. If the matching of the \a path
  and the \a target (if present) is successful, \a ref
  is set from the \a target, and the pointer to the
  matching node is returned. \a idx is the index into the
  \a path where to begin the matching. The function is
  recursive with idx being incremented for each recursive
  call.

  The matching node must be of the correct \a genus, i.e.
  either QML or C++, but \a genus can be set to \c DontCare.
  \a flags indicates whether to search base classes and
  whether to search for an enum value. \a node points to
  the node where the search should begin, assuming the
  \a path is a not a fully-qualified name. \a node is
  most often the root of this Tree.
 */
const Node* Tree::matchPathAndTarget(const QStringList& path,
                                     int idx,
                                     const QString& target,
                                     const Node* node,
                                     int flags,
                                     Node::Genus genus,
                                     QString& ref) const
{
    /*
      If the path has been matched, then if there is a target,
      try to match the target. If there is a target, but you
      can't match it at the end of the path, give up; return 0.
     */
    if (idx == path.size()) {
        if (!target.isEmpty()) {
            ref = getRef(target, node);
            if (ref.isEmpty())
                return 0;
        }
        if (node->isFunction() && node->name() == node->parent()->name())
            node = node->parent();
        return node;
    }

    const Node* t = 0;
    QString name = path.at(idx);
    QList<Node*> nodes;
    node->findChildren(name, nodes);

    foreach (const Node* n, nodes) {
        if (genus != Node::DontCare) {
            if (n->genus() != genus)
                continue;
        }
        t = matchPathAndTarget(path, idx+1, target, n, flags, genus, ref);
        if (t && !t->isPrivate())
            return t;
    }
    if (target.isEmpty()) {
        if ((idx) == (path.size()-1) && node->isInnerNode() && (flags & SearchEnumValues)) {
            t = static_cast<const InnerNode*>(node)->findEnumNodeForValue(path.at(idx));
            if (t)
                return t;
        }
    }
    if ((genus != Node::QML) && node->isClass() && (flags & SearchBaseClasses)) {
        NodeList baseClasses = allBaseClasses(static_cast<const ClassNode*>(node));
        foreach (const Node* bc, baseClasses) {
            t = matchPathAndTarget(path, idx, target, bc, flags, genus, ref);
            if (t && ! t->isPrivate())
                return t;
            if (target.isEmpty()) {
                if ((idx) == (path.size()-1) && (flags & SearchEnumValues)) {
                    t = static_cast<const InnerNode*>(bc)->findEnumNodeForValue(path.at(idx));
                    if (t)
                        return t;
                }
            }
        }
    }
    return 0;
}

/*!
  Searches the tree for a node that matches the \a path. The
  search begins at \a start but can move up the parent chain
  recursively if no match is found.

  This findNode() callse the other findNode(), which is not
  called anywhere else.
 */
const Node* Tree::findNode(const QStringList& path,
                           const Node* start,
                           int findFlags,
                           Node::Genus genus) const
{
    const Node* current = start;
    if (!current)
        current = root();

    do {
        const Node* node = current;
        int i;
        int start_idx = 0;

        /*
          If the path contains one or two double colons ("::"),
          check first to see if the first two path strings refer
          to a QML element. If they do, path[0] will be the QML
          module identifier, and path[1] will be the QML type.
          If the answer is yes, the reference identifies a QML
          type node.
        */
        if ((genus != Node::CPP) && (path.size() >= 2) && !path[0].isEmpty()) {
            QmlClassNode* qcn = lookupQmlType(QString(path[0] + "::" + path[1]));
            if (qcn) {
                node = qcn;
                if (path.size() == 2)
                    return node;
                start_idx = 2;
            }
        }

        for (i = start_idx; i < path.size(); ++i) {
            if (node == 0 || !node->isInnerNode())
                break;

            const Node* next = static_cast<const InnerNode*>(node)->findChildNode(path.at(i), genus);
            if (!next && (findFlags & SearchEnumValues) && i == path.size()-1) {
                next = static_cast<const InnerNode*>(node)->findEnumNodeForValue(path.at(i));
            }
            if (!next && (genus != Node::QML) && node->isClass() && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode*>(node));
                foreach (const Node* baseClass, baseClasses) {
                    next = static_cast<const InnerNode*>(baseClass)->findChildNode(path.at(i), genus);
                    if (!next && (findFlags & SearchEnumValues) && i == path.size() - 1)
                        next = static_cast<const InnerNode*>(baseClass)->findEnumNodeForValue(path.at(i));
                    if (next) {
                        break;
                    }
                }
            }
            node = next;
        }
        if (node && i == path.size())
            return node;
        current = current->parent();
    } while (current);

    return 0;
}

/*!
  This function searches for a node with a canonical title
  constructed from \a target. If the node it finds is \a node,
  it returns the ref from that node. Otherwise it returns an
  empty string.
 */
QString Tree::getRef(const QString& target, const Node* node) const
{
    TargetMap::const_iterator i = nodesByTargetTitle_.constFind(target);
    if (i != nodesByTargetTitle_.constEnd()) {
        do {
            if (i.value()->node_ == node)
                return i.value()->ref_;
            ++i;
        } while (i != nodesByTargetTitle_.constEnd() && i.key() == target);
    }
    QString key = Doc::canonicalTitle(target);
    i = nodesByTargetRef_.constFind(key);
    if (i != nodesByTargetRef_.constEnd()) {
        do {
            if (i.value()->node_ == node)
                return i.value()->ref_;
            ++i;
        } while (i != nodesByTargetRef_.constEnd() && i.key() == key);
    }
    return QString();
}

/*!
  Inserts a new target into the target table. \a name is the
  key. The target record contains the \a type, a pointer to
  the \a node, the \a priority. and a canonicalized form of
  the \a name, which is later used.
 */
void Tree::insertTarget(const QString& name,
                        const QString& title,
                        TargetRec::Type type,
                        Node* node,
                        int priority)
{
    TargetRec* target = new TargetRec(name, title, type, node, priority);
    nodesByTargetRef_.insert(name, target);
    nodesByTargetTitle_.insert(title, target);
}

/*!
 */
void Tree::resolveTargets(InnerNode* root)
{
    // need recursion
    foreach (Node* child, root->childNodes()) {
        if (child->type() == Node::Document) {
            DocNode* node = static_cast<DocNode*>(child);
            QString key = node->title();
            if (!key.isEmpty()) {
                if (key.contains(QChar(' ')))
                    key = Doc::canonicalTitle(key);
                QList<DocNode*> nodes = docNodesByTitle_.values(key);
                bool alreadyThere = false;
                if (!nodes.empty()) {
                    for (int i=0; i< nodes.size(); ++i) {
                        if (nodes[i]->subType() == Node::ExternalPage) {
                            if (node->name() == nodes[i]->name()) {
                                alreadyThere = true;
                                break;
                            }
                        }
                    }
                }
                if (!alreadyThere)
                    docNodesByTitle_.insert(key, node);
            }
        }

        if (child->doc().hasTableOfContents()) {
            const QList<Atom*>& toc = child->doc().tableOfContents();
            for (int i = 0; i < toc.size(); ++i) {
                QString ref = refForAtom(toc.at(i));
                QString title = Text::sectionHeading(toc.at(i)).toString();
                if (!ref.isEmpty() && !title.isEmpty()) {
                    QString key = Doc::canonicalTitle(title);
                    TargetRec* target = new TargetRec(ref, title, TargetRec::Contents, child, 3);
                    nodesByTargetRef_.insert(key, target);
                    nodesByTargetTitle_.insert(title, target);
                }
            }
        }
        if (child->doc().hasKeywords()) {
            const QList<Atom*>& keywords = child->doc().keywords();
            for (int i = 0; i < keywords.size(); ++i) {
                QString ref = refForAtom(keywords.at(i));
                QString title = keywords.at(i)->string();
                if (!ref.isEmpty() && !title.isEmpty()) {
                    QString key = Doc::canonicalTitle(title);
                    TargetRec* target = new TargetRec(ref, title, TargetRec::Keyword, child, 1);
                    nodesByTargetRef_.insert(key, target);
                    nodesByTargetTitle_.insert(title, target);
                }
            }
        }
        if (child->doc().hasTargets()) {
            const QList<Atom*>& targets = child->doc().targets();
            for (int i = 0; i < targets.size(); ++i) {
                QString ref = refForAtom(targets.at(i));
                QString title = targets.at(i)->string();
                if (!ref.isEmpty() && !title.isEmpty()) {
                    QString key = Doc::canonicalTitle(title);
                    TargetRec* target = new TargetRec(ref, title, TargetRec::Target, child, 2);
                    nodesByTargetRef_.insert(key, target);
                    nodesByTargetTitle_.insert(title, target);
                }
            }
        }
    }
}

/*!
  This function searches for a \a target anchor node. If it
  finds one, it sets \a ref and returns the found node.
 */
const Node*
Tree::findUnambiguousTarget(const QString& target, QString& ref) const
{
    int numBestTargets = 0;
    TargetRec* bestTarget = 0;
    QList<TargetRec*> bestTargetList;

    QString key = target;
    TargetMap::const_iterator i = nodesByTargetTitle_.find(key);
    while (i != nodesByTargetTitle_.constEnd()) {
        if (i.key() != key)
            break;
        TargetRec* candidate = i.value();
        if (!bestTarget || (candidate->priority_ < bestTarget->priority_)) {
            bestTarget = candidate;
            bestTargetList.clear();
            bestTargetList.append(candidate);
            numBestTargets = 1;
        } else if (candidate->priority_ == bestTarget->priority_) {
            bestTargetList.append(candidate);
            ++numBestTargets;
        }
        ++i;
    }
    if (bestTarget) {
        ref = bestTarget->ref_;
        return bestTarget->node_;
    }

    numBestTargets = 0;
    bestTarget = 0;
    key = Doc::canonicalTitle(target);
    i = nodesByTargetRef_.find(key);
    while (i != nodesByTargetRef_.constEnd()) {
        if (i.key() != key)
            break;
        TargetRec* candidate = i.value();
        if (!bestTarget || (candidate->priority_ < bestTarget->priority_)) {
            bestTarget = candidate;
            bestTargetList.clear();
            bestTargetList.append(candidate);
            numBestTargets = 1;
        } else if (candidate->priority_ == bestTarget->priority_) {
            bestTargetList.append(candidate);
            ++numBestTargets;
        }
        ++i;
    }
    if (bestTarget) {
        ref = bestTarget->ref_;
        return bestTarget->node_;
    }

    ref.clear();
    return 0;
}

/*!
  This function searches for a node with the specified \a title.
 */
const DocNode* Tree::findDocNodeByTitle(const QString& title) const
{
    DocNodeMultiMap::const_iterator i;
    if (title.contains(QChar(' ')))
        i = docNodesByTitle_.constFind(Doc::canonicalTitle(title));
    else
        i = docNodesByTitle_.constFind(title);
    if (i != docNodesByTitle_.constEnd()) {
        /*
          Reporting all these duplicate section titles is probably
          overkill. We should report the duplicate file and let
          that suffice.
        */
        DocNodeMultiMap::const_iterator j = i;
        ++j;
        if (j != docNodesByTitle_.constEnd() && j.key() == i.key()) {
            QList<Location> internalLocations;
            while (j != docNodesByTitle_.constEnd()) {
                if (j.key() == i.key() && j.value()->url().isEmpty()) {
                    internalLocations.append(j.value()->location());
                    break; // Just report one duplicate for now.
                }
                ++j;
            }
            if (internalLocations.size() > 0) {
                i.value()->location().warning("This page title exists in more than one file: " + title);
                foreach (const Location &location, internalLocations)
                    location.warning("[It also exists here]");
            }
        }
        return i.value();
    }
    return 0;
}

/*!
  Returns a canonical title for the \a atom, if the \a atom
  is a SectionLeft or a Target.
 */
QString Tree::refForAtom(const Atom* atom)
{
    if (atom) {
        if (atom->type() == Atom::SectionLeft)
            return Doc::canonicalTitle(Text::sectionHeading(atom).toString());
        if (atom->type() == Atom::Target)
            return Doc::canonicalTitle(atom->string());
    }
    return QString();
}

/*!
  \fn const CNMap& Tree::groups() const
  Returns a const reference to the collection of all
  group nodes.
*/

/*!
  \fn const ModuleMap& Tree::modules() const
  Returns a const reference to the collection of all
  module nodes.
*/

/*!
  \fn const QmlModuleMap& Tree::qmlModules() const
  Returns a const reference to the collection of all
  QML module nodes.
*/

/*!
  Returns the collection node in this tree that has the same
  name and type as \a cn. Returns 0 if no match is found.

  If the matching node is \a cn, return 0.
 */
CollectionNode* Tree::getCorrespondingCollection(CollectionNode* cn)
{
    CollectionNode* ccn = 0;
    if (cn->isGroup())
        ccn = getGroup(cn->name());
    else if (cn->isModule())
        ccn = getModule(cn->name());
    else if (cn->isQmlModule())
        ccn = getQmlModule(cn->name());
    if (ccn == cn)
        ccn = 0;
    return ccn;
}

/*!
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, return 0.
 */
GroupNode* Tree::getGroup(const QString& name)
{
    CNMap::const_iterator i = groups_.find(name);
    if (i != groups_.end())
        return static_cast<GroupNode*>(i.value());
    return 0;
}

/*!
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found, return 0.
 */
ModuleNode* Tree::getModule(const QString& name)
{
    CNMap::const_iterator i = modules_.find(name);
    if (i != modules_.end())
        return static_cast<ModuleNode*>(i.value());
    return 0;
}

/*!
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found, return 0.
 */
QmlModuleNode* Tree::getQmlModule(const QString& name)
{
    CNMap::const_iterator i = qmlModules_.find(name);
    if (i != qmlModules_.end())
        return static_cast<QmlModuleNode*>(i.value());
    return 0;
}

/*!
  Find the group node named \a name and return a pointer
  to it. If the group node is not found, add a new group
  node named \a name and return a pointer to the new one.

  If a new group node is added, its parent is the tree root,
  and the new group node is marked \e{not seen}.
 */
GroupNode* Tree::findGroup(const QString& name)
{
    CNMap::const_iterator i = groups_.find(name);
    if (i != groups_.end())
        return static_cast<GroupNode*>(i.value());;
    GroupNode* gn = new GroupNode(root(), name);
    gn->markNotSeen();
    groups_.insert(name, gn);
    return gn;
}

/*!
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found, add a new module
  node named \a name and return a pointer to that one.

  If a new module node is added, its parent is the tree root,
  and the new module node is marked \e{not seen}.
 */
ModuleNode* Tree::findModule(const QString& name)
{
    CNMap::const_iterator i = modules_.find(name);
    if (i != modules_.end())
        return static_cast<ModuleNode*>(i.value());
    ModuleNode* mn = new ModuleNode(root(), name);
    mn->markNotSeen();
    modules_.insert(name, mn);
    return mn;
}

/*!
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found, add a new QML module
  node named \a name and return a pointer to that one.

  If a new QML module node is added, its parent is the tree root,
  and the new QML module node is marked \e{not seen}.
 */
QmlModuleNode* Tree::findQmlModule(const QString& name)
{
    CNMap::const_iterator i = qmlModules_.find(name);
    if (i != qmlModules_.end())
        return static_cast<QmlModuleNode*>(i.value());
    QmlModuleNode* qmn = new QmlModuleNode(root(), name);
    qmn->markNotSeen();
    qmn->setQmlModuleInfo(name);
    qmlModules_.insert(name, qmn);
    return qmn;
}

/*!
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new group node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned.
 */
GroupNode* Tree::addGroup(const QString& name)
{
    GroupNode* group = findGroup(name);
    return group;
}

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned.
 */
ModuleNode* Tree::addModule(const QString& name)
{
    ModuleNode* module = findModule(name);
    return module;
}

/*!
  Looks up the QML module node named \a name in the collection
  of all QML module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new QML module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned.
 */
QmlModuleNode* Tree::addQmlModule(const QString& name)
{
    QStringList blankSplit = name.split(QLatin1Char(' '));
    QmlModuleNode* qmn = findQmlModule(blankSplit[0]);
    qmn->setQmlModuleInfo(name);
    return qmn;
}

/*!
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is not found, a new group
  node named \a name is created and inserted into the collection.
  Then append \a node to the group's members list, and append the
  group name to the list of group names in \a node. The parent of
  \a node is not changed by this function. Returns a pointer to
  the group node.
 */
GroupNode* Tree::addToGroup(const QString& name, Node* node)
{
    GroupNode* gn = findGroup(name);
    if (!node->isInternal()) {
        gn->addMember(node);
        node->appendGroupName(name);
    }
    return gn;
}

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is not found, a new module
  node named \a name is created and inserted into the collection.
  Then append \a node to the module's members list. The parent of
  \a node is not changed by this function. Returns the module node.
 */
ModuleNode* Tree::addToModule(const QString& name, Node* node)
{
    ModuleNode* mn = findModule(name);
    mn->addMember(node);
    node->setModuleName(name);
    return mn;
}

/*!
  Looks up the QML module named \a name. If it isn't there,
  create it. Then append \a node to the QML module's member
  list. The parent of \a node is not changed by this function.
  Returns the pointer to the QML module node.
 */
QmlModuleNode* Tree::addToQmlModule(const QString& name, Node* node)
{
    QStringList qmid;
    QStringList dotSplit;
    QStringList blankSplit = name.split(QLatin1Char(' '));
    qmid.append(blankSplit[0]);
    if (blankSplit.size() > 1) {
        qmid.append(blankSplit[0] + blankSplit[1]);
        dotSplit = blankSplit[1].split(QLatin1Char('.'));
        qmid.append(blankSplit[0] + dotSplit[0]);
    }

    QmlModuleNode* qmn = findQmlModule(blankSplit[0]);
    qmn->addMember(node);
    node->setQmlModule(qmn);
    if (node->isQmlType()) {
        QmlClassNode* n = static_cast<QmlClassNode*>(node);
        for (int i=0; i<qmid.size(); ++i) {
            QString key = qmid[i] + "::" + node->name();
            insertQmlType(key, n);
        }
    }
    return qmn;
}

/*!
  If the QML type map does not contain \a key, insert node
  \a n with the specified \a key.
 */
void Tree::insertQmlType(const QString& key, QmlClassNode* n)
{
    if (!qmlTypeMap_.contains(key))
        qmlTypeMap_.insert(key,n);
}

/*!
  Split \a target on "::" and find the function node with that
  path.

  Called in HtmlGenerator, DitaXmlGenerator, and QdocDatabase.
 */
const Node* Tree::findFunctionNode(const QString& target, const Node* relative, Node::Genus genus)
{
    QString t = target;
    t.chop(2);
    QStringList path = t.split("::");
    const FunctionNode* fn = findFunctionNode(path, relative, SearchBaseClasses, genus);
    if (fn && fn->metaness() != FunctionNode::MacroWithoutParams)
        return fn;
    return 0;
}

/*!
  Search for a node that is identified by \a name.
  Return a pointer to a matching node, or 0.
*/
const Node* Tree::checkForCollision(const QString& name)
{
    return findNode(QStringList(name), 0, 0, Node::DontCare);
}

QT_END_NAMESPACE

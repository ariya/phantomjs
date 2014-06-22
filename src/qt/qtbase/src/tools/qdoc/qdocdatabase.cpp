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

#include "generator.h"
#include "atom.h"
#include "tree.h"
#include "qdocdatabase.h"
#include "qdoctagfiles.h"
#include "qdocindexfiles.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static NodeMap emptyNodeMap_;
static NodeMultiMap emptyNodeMultiMap_;

/*! \class QDocDatabase
 */

QDocDatabase* QDocDatabase::qdocDB_ = NULL;

/*!
  Constructs the singleton qdoc database object.
  It constructs a singleton Tree object with this
  qdoc database pointer.
 */
QDocDatabase::QDocDatabase() : showInternal_(false)
{
    tree_ = new Tree(this);
}

/*!
  Destroys the qdoc database object. This requires deleting
  the tree of nodes, which deletes each node.
 */
QDocDatabase::~QDocDatabase()
{
    masterMap_.clear();
    delete tree_;
}

/*! \fn Tree* QDocDatabase::tree()
  Returns the pointer to the tree. This function is for compatibility
  with the current qdoc. It will be removed when the QDocDatabase class
  replaces the current structures.
 */

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
*/
QDocDatabase* QDocDatabase::qdocDB()
{
   if (!qdocDB_)
      qdocDB_ = new QDocDatabase;
   return qdocDB_;
}

/*!
  Destroys the singleton.
 */
void QDocDatabase::destroyQdocDB()
{
    if (qdocDB_) {
        delete qdocDB_;
        qdocDB_ = 0;
    }
}

/*!
  \fn const DocNodeMap& QDocDatabase::groups() const
  Returns a const reference to the collection of all
  group nodes.
*/

/*!
  \fn const DocNodeMap& QDocDatabase::modules() const
  Returns a const reference to the collection of all
  module nodes.
*/

/*!
  \fn const DocNodeMap& QDocDatabase::qmlModules() const
  Returns a const reference to the collection of all
  QML module nodes.
*/

/*!
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, return 0.
 */
DocNode* QDocDatabase::getGroup(const QString& name)
{
    DocNodeMap::const_iterator i = groups_.find(name);
    if (i != groups_.end())
        return i.value();
    return 0;
}

/*!
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, add a new group
  node named \a name and return a pointer to that one.

  If a new group node is added, its parent is the tree root,
  and the new group node is marked \e{not seen}.
 */
DocNode* QDocDatabase::findGroup(const QString& name)
{
    DocNodeMap::const_iterator i = groups_.find(name);
    if (i != groups_.end())
        return i.value();
    DocNode* dn = new DocNode(tree_->root(), name, Node::Group, Node::OverviewPage);
    dn->markNotSeen();
    groups_.insert(name,dn);
    if (!masterMap_.contains(name,dn))
        masterMap_.insert(name,dn);
    return dn;
}

/*!
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found, add a new module
  node named \a name and return a pointer to that one.

  If a new module node is added, its parent is the tree root,
  and the new module node is marked \e{not seen}.
 */
DocNode* QDocDatabase::findModule(const QString& name)
{
    DocNodeMap::const_iterator i = modules_.find(name);
    if (i != modules_.end())
        return i.value();
    DocNode* dn = new DocNode(tree_->root(), name, Node::Module, Node::OverviewPage);
    dn->markNotSeen();
    modules_.insert(name,dn);
    if (!masterMap_.contains(name,dn))
        masterMap_.insert(name,dn);
    return dn;
}

/*!
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found, add a new QML module
  node named \a name and return a pointer to that one.

  If a new QML module node is added, its parent is the tree root,
  and the new QML module node is marked \e{not seen}.
 */
QmlModuleNode* QDocDatabase::findQmlModule(const QString& name)
{
    if (qmlModules_.contains(name))
        return static_cast<QmlModuleNode*>(qmlModules_.value(name));

    QmlModuleNode* qmn = new QmlModuleNode(tree_->root(), name);
    qmn->markNotSeen();
    qmn->setQmlModuleInfo(name);
    qmlModules_.insert(name, qmn);
    masterMap_.insert(name, qmn);
    return qmn;
}

/*!
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new group node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned. The group node is marked \e{seen}
  in either case.
 */
DocNode* QDocDatabase::addGroup(const QString& name)
{
    DocNode* group = findGroup(name);
    group->markSeen();
    return group;
}

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned. The module node is marked \e{seen}
  in either case.
 */
DocNode* QDocDatabase::addModule(const QString& name)
{
    DocNode* module = findModule(name);
    module->markSeen();
    return module;
}

/*!
  Looks up the QML module node named \a name in the collection
  of all QML module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new QML module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned. The QML module node is marked \e{seen}
  in either case.
 */
QmlModuleNode* QDocDatabase::addQmlModule(const QString& name)
{
    QStringList blankSplit = name.split(QLatin1Char(' '));
    QmlModuleNode* qmn = findQmlModule(blankSplit[0]);
    qmn->setQmlModuleInfo(name);
    qmn->markSeen();
    //masterMap_.insert(qmn->qmlModuleIdentifier(),qmn);
    return qmn;
}

/*!
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is not found, a new group
  node named \a name is created and inserted into the collection.
  Then append \a node to the group's members list, and append the
  group node to the member list of the \a node. The parent of the
  \a node is not changed by this function. Returns a pointer to
  the group node.
 */
DocNode* QDocDatabase::addToGroup(const QString& name, Node* node)
{
    DocNode* dn = findGroup(name);
    dn->addMember(node);
    node->addMember(dn);
    return dn;
}

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is not found, a new module
  node named \a name is created and inserted into the collection.
  Then append \a node to the module's members list. The parent of
  \a node is not changed by this function. Returns the module node.
 */
DocNode* QDocDatabase::addToModule(const QString& name, Node* node)
{
    DocNode* dn = findModule(name);
    dn->addMember(node);
    node->setModuleName(name);
    return dn;
}

/*!
  Looks up the QML module named \a name. If it isn't there,
  create it. Then append \a node to the QML module's member
  list. The parent of \a node is not changed by this function.
 */
void QDocDatabase::addToQmlModule(const QString& name, Node* node)
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

    if (node->subType() == Node::QmlClass) {
        QmlClassNode* n = static_cast<QmlClassNode*>(node);
        for (int i=0; i<qmid.size(); ++i) {
            QString key = qmid[i] + "::" + node->name();
            if (!qmlTypeMap_.contains(key))
                qmlTypeMap_.insert(key,n);
            if (!masterMap_.contains(key))
                masterMap_.insert(key,node);
        }
        if (!masterMap_.contains(node->name(),node))
            masterMap_.insert(node->name(),node);
    }
}

/*!
  Looks up the QML type node identified by the Qml module id
  \a qmid and QML type \a name and returns a pointer to the
  QML type node. The key is \a qmid + "::" + \a name.

  If the QML module id is empty, it looks up the QML type by
  \a name only.
 */
QmlClassNode* QDocDatabase::findQmlType(const QString& qmid, const QString& name) const
{
    if (!qmid.isEmpty())
        return qmlTypeMap_.value(qmid + "::" + name);

    QStringList path(name);
    Node* n = tree_->findNodeByNameAndType(path, Node::Document, Node::QmlClass, 0, true);
    if (n) {
        if (n->subType() == Node::QmlClass)
            return static_cast<QmlClassNode*>(n);
        else if (n->subType() == Node::Collision) {
            NameCollisionNode* ncn;
            ncn = static_cast<NameCollisionNode*>(n);
            return static_cast<QmlClassNode*>(ncn->findAny(Node::Document,Node::QmlClass));
        }
    }
    return 0;
}

/*!
  Looks up the QML type node identified by the Qml module id
  constructed from the strings in the \a import record and the
  QML type \a name and returns a pointer to the QML type node.
  If a QML type node is not found, 0 is returned.
 */
QmlClassNode* QDocDatabase::findQmlType(const ImportRec& import, const QString& name) const
{
    if (!import.isEmpty()) {
        QStringList dotSplit;
        dotSplit = name.split(QLatin1Char('.'));
        QString qmName;
        if (import.importUri_.isEmpty())
            qmName = import.name_;
        else
            qmName = import.importUri_;
        for (int i=0; i<dotSplit.size(); ++i) {
            QString qualifiedName = qmName + "::" + dotSplit[i];
            QmlClassNode* qcn = qmlTypeMap_.value(qualifiedName);
            if (qcn)
                return qcn;
        }
    }
    return 0;
}

/*!
  For debugging only.
 */
void QDocDatabase::printModules() const
{
    DocNodeMap::const_iterator i = modules_.begin();
    while (i != modules_.end()) {
        qDebug() << "  " << i.key();
        ++i;
    }
}

/*!
  For debugging only.
 */
void QDocDatabase::printQmlModules() const
{
    DocNodeMap::const_iterator i = qmlModules_.begin();
    while (i != qmlModules_.end()) {
        qDebug() << "  " << i.key();
        ++i;
    }
}

/*!
  Traverses the database to construct useful data structures
  for use when outputting certain significant collections of
  things, C++ classes, QML types, "since" lists, and other
  stuff.
 */
void QDocDatabase::buildCollections()
{
    nonCompatClasses_.clear();
    mainClasses_.clear();
    compatClasses_.clear();
    obsoleteClasses_.clear();
    funcIndex_.clear();
    legaleseTexts_.clear();
    serviceClasses_.clear();
    qmlClasses_.clear();

    findAllClasses(treeRoot());
    findAllFunctions(treeRoot());
    findAllLegaleseTexts(treeRoot());
    findAllNamespaces(treeRoot());
    findAllSince(treeRoot());
    findAllObsoleteThings(treeRoot());
}

/*!
  Finds all the C++ class nodes and QML type nodes and
  sorts them into maps.
 */
void QDocDatabase::findAllClasses(const InnerNode* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private && (!(*c)->isInternal() || showInternal_)) {
            if ((*c)->type() == Node::Class && !(*c)->doc().isEmpty()) {
                QString className = (*c)->name();
                if ((*c)->parent() &&
                        (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                    className = (*c)->parent()->name()+"::"+className;

                if ((*c)->status() == Node::Compat) {
                    compatClasses_.insert(className, *c);
                }
                else {
                    nonCompatClasses_.insert(className, *c);
                    if ((*c)->status() == Node::Main)
                        mainClasses_.insert(className, *c);
                }

                QString serviceName = (static_cast<const ClassNode *>(*c))->serviceName();
                if (!serviceName.isEmpty()) {
                    serviceClasses_.insert(serviceName, *c);
                }
            }
            else if ((*c)->type() == Node::Document &&
                     (*c)->subType() == Node::QmlClass &&
                     !(*c)->doc().isEmpty()) {
                QString qmlTypeName = (*c)->name();
                if (qmlTypeName.startsWith(QLatin1String("QML:")))
                    qmlClasses_.insert(qmlTypeName.mid(4),*c);
                else
                    qmlClasses_.insert(qmlTypeName,*c);
            }
            else if ((*c)->isInnerNode()) {
                findAllClasses(static_cast<InnerNode*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Finds all the function nodes
 */
void QDocDatabase::findAllFunctions(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode()) {
                findAllFunctions(static_cast<const InnerNode*>(*c));
            }
            else if ((*c)->type() == Node::Function) {
                const FunctionNode* func = static_cast<const FunctionNode*>(*c);
                if ((func->status() > Node::Obsolete) &&
                        !func->isInternal() &&
                        (func->metaness() != FunctionNode::Ctor) &&
                        (func->metaness() != FunctionNode::Dtor)) {
                    funcIndex_[(*c)->name()].insert((*c)->parent()->fullDocumentName(), *c);
                }
            }
        }
        ++c;
    }
}

/*!
  Finds all the nodes containing legalese text and puts them
  in a map.
 */
void QDocDatabase::findAllLegaleseTexts(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if (!(*c)->doc().legaleseText().isEmpty())
                legaleseTexts_.insertMulti((*c)->doc().legaleseText(), *c);
            if ((*c)->isInnerNode())
                findAllLegaleseTexts(static_cast<const InnerNode *>(*c));
        }
        ++c;
    }
}

/*!
  Finds all the namespace nodes and puts them in an index.
 */
void QDocDatabase::findAllNamespaces(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode()) {
                findAllNamespaces(static_cast<const InnerNode *>(*c));
                if ((*c)->type() == Node::Namespace) {
                    // Ensure that the namespace's name is not empty (the root
                    // namespace has no name).
                    if (!(*c)->name().isEmpty())
                        namespaceIndex_.insert((*c)->name(), *c);
                }
            }
        }
        ++c;
    }
}

/*!
  Finds all nodes with status = Obsolete and sorts them into
  maps. They can be C++ classes, QML types, or they can be
  functions, enum types, typedefs, methods, etc.
 */
void QDocDatabase::findAllObsoleteThings(const InnerNode* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            QString name = (*c)->name();
            if ((*c)->status() == Node::Obsolete) {
                if ((*c)->type() == Node::Class) {
                    if ((*c)->parent() && (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                        name = (*c)->parent()->name() + "::" + name;
                    obsoleteClasses_.insert(name, *c);
                }
                else if ((*c)->type() == Node::Document && (*c)->subType() == Node::QmlClass) {
                    if (name.startsWith(QLatin1String("QML:")))
                        name = name.mid(4);
                    name = (*c)->qmlModuleName() + "::" + name;
                    obsoleteQmlTypes_.insert(name,*c);
                }
            }
            else if ((*c)->type() == Node::Class) {
                InnerNode* n = static_cast<InnerNode*>(*c);
                bool inserted = false;
                NodeList::const_iterator p = n->childNodes().constBegin();
                while (p != n->childNodes().constEnd()) {
                    if ((*p)->access() != Node::Private) {
                        switch ((*p)->type()) {
                        case Node::Enum:
                        case Node::Typedef:
                        case Node::Function:
                        case Node::Property:
                        case Node::Variable:
                            if ((*p)->status() == Node::Obsolete) {
                                if ((*c)->parent() && (*c)->parent()->type() == Node::Namespace &&
                                    !(*c)->parent()->name().isEmpty())
                                    name = (*c)->parent()->name() + "::" + name;
                                classesWithObsoleteMembers_.insert(name, *c);
                                inserted = true;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    if (inserted)
                        break;
                    ++p;
                }
            }
            else if ((*c)->type() == Node::Document && (*c)->subType() == Node::QmlClass) {
                InnerNode* n = static_cast<InnerNode*>(*c);
                bool inserted = false;
                NodeList::const_iterator p = n->childNodes().constBegin();
                while (p != n->childNodes().constEnd()) {
                    if ((*p)->access() != Node::Private) {
                        switch ((*c)->type()) {
                        case Node::QmlProperty:
                        case Node::QmlSignal:
                        case Node::QmlSignalHandler:
                        case Node::QmlMethod:
                            if ((*c)->parent()) {
                                Node* parent = (*c)->parent();
                                if (parent->type() == Node::QmlPropertyGroup && parent->parent())
                                    parent = parent->parent();
                                if (parent && parent->subType() == Node::QmlClass && !parent->name().isEmpty())
                                    name = parent->name() + "::" + name;
                            }
                            qmlTypesWithObsoleteMembers_.insert(name,*c);
                            inserted = true;
                            break;
                        default:
                            break;
                        }
                    }
                    if (inserted)
                        break;
                    ++p;
                }
            }
            else if ((*c)->isInnerNode()) {
                findAllObsoleteThings(static_cast<InnerNode*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Finds all the nodes where a \e{since} command appeared in the
  qdoc comment and sorts them into maps according to the kind of
  node.

  This function is used for generating the "New Classes... in x.y"
  section on the \e{What's New in Qt x.y} page.
 */
void QDocDatabase::findAllSince(const InnerNode* node)
{
    NodeList::const_iterator child = node->childNodes().constBegin();
    while (child != node->childNodes().constEnd()) {
        QString sinceString = (*child)->since();
        // Insert a new entry into each map for each new since string found.
        if (((*child)->access() != Node::Private) && !sinceString.isEmpty()) {
            NodeMultiMapMap::iterator nsmap = newSinceMaps_.find(sinceString);
            if (nsmap == newSinceMaps_.end())
                nsmap = newSinceMaps_.insert(sinceString,NodeMultiMap());

            NodeMapMap::iterator ncmap = newClassMaps_.find(sinceString);
            if (ncmap == newClassMaps_.end())
                ncmap = newClassMaps_.insert(sinceString,NodeMap());

            NodeMapMap::iterator nqcmap = newQmlTypeMaps_.find(sinceString);
            if (nqcmap == newQmlTypeMaps_.end())
                nqcmap = newQmlTypeMaps_.insert(sinceString,NodeMap());

            if ((*child)->type() == Node::Function) {
                // Insert functions into the general since map.
                FunctionNode *func = static_cast<FunctionNode *>(*child);
                if ((func->status() > Node::Obsolete) &&
                    (func->metaness() != FunctionNode::Ctor) &&
                    (func->metaness() != FunctionNode::Dtor)) {
                    nsmap.value().insert(func->name(),(*child));
                }
            }
            else {
                if ((*child)->type() == Node::Class) {
                    // Insert classes into the since and class maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    ncmap.value().insert(className,(*child));
                }
                else if ((*child)->subType() == Node::QmlClass) {
                    // Insert QML elements into the since and element maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    nqcmap.value().insert(className,(*child));
                }
                else if ((*child)->type() == Node::QmlProperty) {
                    // Insert QML properties into the since map.
                    QString propertyName = (*child)->name();
                    nsmap.value().insert(propertyName,(*child));
                }
                else {
                    // Insert external documents into the general since map.
                    QString name = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        name = (*child)->parent()->name()+"::"+name;
                    }
                    nsmap.value().insert(name,(*child));
                }
            }
            // Recursively find child nodes with since commands.
            if ((*child)->isInnerNode()) {
                findAllSince(static_cast<InnerNode *>(*child));
            }
        }
        ++child;
    }
}

/*!
  Find the \a key in the map of new class maps, and return a
  reference to the value, which is a NodeMap. If \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getClassMap(const QString& key) const
{
    NodeMapMap::const_iterator i = newClassMaps_.constFind(key);
    if (i != newClassMaps_.constEnd())
        return i.value();
    return emptyNodeMap_;
}

/*!
  Find the \a key in the map of new QML type maps, and return a
  reference to the value, which is a NodeMap. If the \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getQmlTypeMap(const QString& key) const
{
    NodeMapMap::const_iterator i = newQmlTypeMaps_.constFind(key);
    if (i != newQmlTypeMaps_.constEnd())
        return i.value();
    return emptyNodeMap_;
}

/*!
  Find the \a key in the map of new \e {since} maps, and return
  a reference to the value, which is a NodeMultiMap. If \a key
  is not found, return a reference to an empty NodeMultiMap.
 */
const NodeMultiMap& QDocDatabase::getSinceMap(const QString& key) const
{
    NodeMultiMapMap::const_iterator i = newSinceMaps_.constFind(key);
    if (i != newSinceMaps_.constEnd())
        return i.value();
    return emptyNodeMultiMap_;
}

/*!
  Performs several housekeeping algorithms that create
  certain data structures and resolve lots of links, prior
  to generating documentation.
 */
void QDocDatabase::resolveIssues() {
    resolveQmlInheritance(treeRoot());
    resolveTargets(treeRoot());
    tree_->resolveCppToQmlLinks();
}

/*!
  Searches the \a database for a node named \a target and returns
  a pointer to it if found.
 */
const Node* QDocDatabase::resolveTarget(const QString& target,
                                        const Node* relative,
                                        const Node* self)
{
    const Node* node = 0;
    if (target.endsWith("()")) {
        QString funcName = target;
        funcName.chop(2);
        QStringList path = funcName.split("::");
        const FunctionNode* fn = tree_->findFunctionNode(path, relative, SearchBaseClasses);
        if (fn) {
            /*
              Why is this case not accepted?
             */
            if (fn->metaness() != FunctionNode::MacroWithoutParams)
                node = fn;
        }
    }
    else if (target.contains(QLatin1Char('#'))) {
        // This error message is never printed; I think we can remove the case.
        qDebug() << "qdoc: target case not handled:" << target;
    }
    else {
        QStringList path = target.split("::");
        int flags = SearchBaseClasses | SearchEnumValues | NonFunction;
        node = tree_->findNode(path, relative, flags, self);
    }
    return node;
}

/*!
  Finds the node that will generate the documentation that
  contains the \a target and returns a pointer to it.
 */
const Node* QDocDatabase::findNodeForTarget(const QString& target, const Node* relative)
{
    const Node* node = 0;
    if (target.isEmpty())
        node = relative;
    else if (target.endsWith(".html"))
        node = tree_->root()->findChildNodeByNameAndType(target, Node::Document);
    else {
        node = resolveTarget(target, relative);
        if (!node)
            node = findDocNodeByTitle(target, relative);
    }
    return node;
}

/*!
  Inserts a new target into the target table with the specified
  \a name, \a node, and \a priority.
 */
void QDocDatabase::insertTarget(const QString& name, TargetRec::Type type, Node* node, int priority)
{
    TargetRec target;
    target.type_ = type;
    target.node_ = node;
    target.priority_ = priority;
    Atom a = Atom(Atom::Target, name);
    target.ref_ = refForAtom(&a);
    targetRecMultiMap_.insert(name, target);
}

/*!
  This function searches for a \a target anchor node. If it
  finds one, it sets \a ref and returns the found node.
 */
const Node*
QDocDatabase::findUnambiguousTarget(const QString& target, QString& ref, const Node* relative)
{
    TargetRec bestTarget;
    int numBestTargets = 0;
    QList<TargetRec> bestTargetList;

    QString key = Doc::canonicalTitle(target);
    TargetRecMultiMap::iterator i = targetRecMultiMap_.find(key);
    while (i != targetRecMultiMap_.end()) {
        if (i.key() != key)
            break;
        const TargetRec& candidate = i.value();
        if (candidate.priority_ < bestTarget.priority_) {
            bestTarget = candidate;
            bestTargetList.clear();
            bestTargetList.append(candidate);
            numBestTargets = 1;
        } else if (candidate.priority_ == bestTarget.priority_) {
            bestTargetList.append(candidate);
            ++numBestTargets;
        }
        ++i;
    }
    if (numBestTargets > 0) {
        if (numBestTargets == 1) {
            ref = bestTarget.ref_;
            return bestTarget.node_;
        }
        else if (bestTargetList.size() > 1) {
            if (relative && !relative->qmlModuleName().isEmpty()) {
                for (int i=0; i<bestTargetList.size(); ++i) {
                    const Node* n = bestTargetList.at(i).node_;
                    if (n && relative->qmlModuleName() == n->qmlModuleName()) {
                        ref = bestTargetList.at(i).ref_;
                        return n;
                    }
                }
            }
        }
    }
    ref.clear();
    return 0;
}

/*!
  This function searches for a node with the specified \a title.
  If \a relative node is provided, it is used to disambiguate if
  it has a QML module identifier.
 */
const DocNode* QDocDatabase::findDocNodeByTitle(const QString& title, const Node* relative) const
{
    QString key = Doc::canonicalTitle(title);
    DocNodeMultiMap::const_iterator i = docNodesByTitle_.constFind(key);
    if (i != docNodesByTitle_.constEnd()) {
        if (relative && !relative->qmlModuleName().isEmpty()) {
            const DocNode* dn = i.value();
            InnerNode* parent = dn->parent();
            if (parent && parent->type() == Node::Document && parent->subType() == Node::Collision) {
                const NodeList& nl = parent->childNodes();
                NodeList::ConstIterator it = nl.constBegin();
                while (it != nl.constEnd()) {
                    if ((*it)->qmlModuleName() == relative->qmlModuleName()) {
                        /*
                          By returning here, we avoid printing
                          all the duplicate header warnings,
                          which are not really duplicates now,
                          because of the QML module name being
                          used as a namespace qualifier.
                        */
                        dn = static_cast<const DocNode*>(*it);
                        return dn;
                    }
                    ++it;
                }
            }
        }
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
                i.value()->location().warning(tr("This page title exists in more than one file: \"%1\"").arg(title));
                foreach (const Location &location, internalLocations)
                    location.warning(tr("[It also exists here]"));
            }
        }
        return i.value();
    }
    return 0;
}

/*!
  This function searches for a node with a canonical title
  constructed from \a target. If the node it finds is \a node,
  it returns the ref from that node. Otherwise it returns an
  empty string.
 */
QString QDocDatabase::findTarget(const QString& target, const Node* node) const
{
    QString key = Doc::canonicalTitle(target);
    TargetRecMultiMap::const_iterator i = targetRecMultiMap_.constFind(key);

    if (i != targetRecMultiMap_.constEnd()) {
        do {
            if (i.value().node_ == node)
                return i.value().ref_;
            ++i;
        } while (i != targetRecMultiMap_.constEnd() && i.key() == key);
    }
    return QString();
}

/*!
  For each QML Type node in the tree beginning at \a root,
  if it has a QML base type name but its QML base type node
  pointer is 0, use the QML base type name to look up the
  base type node. If the node is found in the tree, set the
  node's QML base type node pointer.
 */
void QDocDatabase::resolveQmlInheritance(InnerNode* root)
{
    // Dop we need recursion?
    foreach (Node* child, root->childNodes()) {
        if (child->type() == Node::Document && child->subType() == Node::QmlClass) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
            if ((qcn->qmlBaseNode() == 0) && !qcn->qmlBaseName().isEmpty()) {
                QmlClassNode* bqcn = 0;
                if (qcn->qmlBaseName().contains("::")) {
                    bqcn =  qmlTypeMap_.value(qcn->qmlBaseName());
                }
                else {
                    const ImportList& imports = qcn->importList();
                    for (int i=0; i<imports.size(); ++i) {
                        bqcn = findQmlType(imports[i], qcn->qmlBaseName());
                        if (bqcn)
                            break;
                    }
                }
                if (bqcn == 0) {
                    bqcn = findQmlType(QString(), qcn->qmlBaseName());
                }
                if (bqcn) {
                    qcn->setQmlBaseNode(bqcn);
                }
#if 0
                else {
                    qDebug() << "Temporary error message (ignore): UNABLE to resolve QML base type:"
                             << qcn->qmlBaseName() << "for QML type:" << qcn->name();
                }
#endif
            }
        }
    }
}

/*!
 */
void QDocDatabase::resolveTargets(InnerNode* root)
{
    // need recursion

    foreach (Node* child, root->childNodes()) {
        if (child->type() == Node::Document) {
            DocNode* node = static_cast<DocNode*>(child);
            if (!node->title().isEmpty()) {
                QString key = Doc::canonicalTitle(node->title());
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
                if (!alreadyThere) {
                    docNodesByTitle_.insert(key, node);
                }
            }
            if (node->subType() == Node::Collision) {
                resolveTargets(node);
            }
        }

        if (child->doc().hasTableOfContents()) {
            const QList<Atom*>& toc = child->doc().tableOfContents();
            TargetRec target;
            target.node_ = child;
            target.priority_ = 3;

            for (int i = 0; i < toc.size(); ++i) {
                target.ref_ = refForAtom(toc.at(i));
                QString title = Text::sectionHeading(toc.at(i)).toString();
                if (!title.isEmpty()) {
                    QString key = Doc::canonicalTitle(title);
                    targetRecMultiMap_.insert(key, target);
                }
            }
        }
        if (child->doc().hasKeywords()) {
            const QList<Atom*>& keywords = child->doc().keywords();
            TargetRec target;
            target.node_ = child;
            target.priority_ = 1;

            for (int i = 0; i < keywords.size(); ++i) {
                target.ref_ = refForAtom(keywords.at(i));
                QString key = Doc::canonicalTitle(keywords.at(i)->string());
                targetRecMultiMap_.insert(key, target);
            }
        }
        if (child->doc().hasTargets()) {
            const QList<Atom*>& toc = child->doc().targets();
            TargetRec target;
            target.node_ = child;
            target.priority_ = 2;

            for (int i = 0; i < toc.size(); ++i) {
                target.ref_ = refForAtom(toc.at(i));
                QString key = Doc::canonicalTitle(toc.at(i)->string());
                targetRecMultiMap_.insert(key, target);
            }
        }
    }
}

/*!
  Generates a tag file and writes it to \a name.
 */
void QDocDatabase::generateTagFile(const QString& name, Generator* g)
{
    if (!name.isEmpty()) {
        QDocTagFiles::qdocTagFiles()->generateTagFile(name, g);
        QDocTagFiles::destroyQDocTagFiles();
    }
}

/*!
  Reads and parses the qdoc index files listed in \a indexFiles.
 */
void QDocDatabase::readIndexes(const QStringList& indexFiles)
{
    QDocIndexFiles::qdocIndexFiles()->readIndexes(indexFiles);
    QDocIndexFiles::destroyQDocIndexFiles();
}

/*!
  Generates a qdoc index file and write it to \a fileName. The
  index file is generated with the parameters \a url, \a title,
  \a g, and \a generateInternalNodes.
 */
void QDocDatabase::generateIndex(const QString& fileName,
                                 const QString& url,
                                 const QString& title,
                                 Generator* g,
                                 bool generateInternalNodes)
{
    QDocIndexFiles::qdocIndexFiles()->generateIndex(fileName, url, title, g, generateInternalNodes);
    QDocIndexFiles::destroyQDocIndexFiles();
}

QString QDocDatabase::refForAtom(const Atom* atom)
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
  If there are open namespaces, search for the function node
  having the same function name as the \a clone node in each
  open namespace. The \a parentPath is a portion of the path
  name provided with the function name at the point of
  reference. \a parentPath is usually a class name. Return
  the pointer to the function node if one is found in an
  open namespace. Otherwise return 0.

  This open namespace concept is of dubious value and might
  be removed.
 */
FunctionNode* QDocDatabase::findNodeInOpenNamespace(const QStringList& parentPath,
                                                    const FunctionNode* clone)
{
    FunctionNode* fn = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList path = t.split("::") + parentPath;
            fn = findFunctionNode(path, clone);
            if (fn)
                break;
        }
    }
    return fn;
}

/*!
  Find a node of the specified \a type and \a subtype that is
  reached with the specified \a path. If such a node is found
  in an open namespace, prefix \a path with the name of the
  open namespace and "::" and return a pointer to the node.
  Othewrwise return 0.
 */
Node* QDocDatabase::findNodeInOpenNamespace(QStringList& path,
                                            Node::Type type,
                                            Node::SubType subtype)
{
    Node* n = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList p = t.split("::") + path;
            n = findNodeByNameAndType(p, type, subtype);
            if (n) {
                path = p;
                break;
            }
        }
    }
    return n;
}

QT_END_NAMESPACE

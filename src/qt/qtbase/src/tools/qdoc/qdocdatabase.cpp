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
bool QDocDatabase::debug = false;

/*! \class QDocForest
  This class manages a collection of trees. Each tree is an
  instance of class Tree, which is a private class.

  The forest is populated as each index file is loaded.
  Each index file adds a tree to the forest. Each tree
  is named with the name of the module it represents.

  The search order is created by searchOrder(), if it has
  not already been created. The search order and module
  names arrays have parallel structure, i.e. modulNames_[i]
  is the module name of the Tree at searchOrder_[i].
 */

/*!
  Destroys the qdoc forest. This requires deleting
  each Tree in the forest. Note that the forest has
  been transferred into the search order array, so
  what is really being used to destroy the forest
  is the search order array.
 */
QDocForest::~QDocForest()
{
    for (int i=0; i<searchOrder_.size(); ++i)
        delete searchOrder_.at(i);
    forest_.clear();
    searchOrder_.clear();
    indexSearchOrder_.clear();
    moduleNames_.clear();
    primaryTree_ = 0;
}

/*!
  Initializes the forest prior to a traversal and
  returns a pointer to the root node of the primary
  tree. If the forest is empty, it return 0
 */
NamespaceNode* QDocForest::firstRoot()
{
    currentIndex_ = 0;
    return (!searchOrder().isEmpty() ? searchOrder()[0]->root() : 0);
}

/*!
  Increments the forest's current tree index. If the current
  tree index is still within the forest, the function returns
  the root node of the current tree. Otherwise it returns 0.
 */
NamespaceNode* QDocForest::nextRoot()
{
    ++currentIndex_;
    return (currentIndex_ < searchOrder().size() ? searchOrder()[currentIndex_]->root() : 0);
}

/*!
  Initializes the forest prior to a traversal and
  returns a pointer to the primary tree. If the
  forest is empty, it returns 0.
 */
Tree* QDocForest::firstTree()
{
    currentIndex_ = 0;
    return (!searchOrder().isEmpty() ? searchOrder()[0] : 0);
}

/*!
  Increments the forest's current tree index. If the current
  tree index is still within the forest, the function returns
  the pointer to the current tree. Otherwise it returns 0.
 */
Tree* QDocForest::nextTree()
{
    ++currentIndex_;
    return (currentIndex_ < searchOrder().size() ? searchOrder()[currentIndex_] : 0);
}

/*!
  \fn Tree* QDocForest::primaryTree()

  Returns the pointer to the primary tree.
 */

/*!
  If the search order array is empty, create the search order.
  If the search order array is not empty, do nothing.
 */
void QDocForest::setSearchOrder()
{
    if (!searchOrder_.isEmpty())
        return;
    QString primaryName = primaryTree()->moduleName();
    searchOrder_.clear();
    searchOrder_.reserve(forest_.size()+1);
    moduleNames_.reserve(forest_.size()+1);
    searchOrder_.append(primaryTree_);
    moduleNames_.append(primaryName);
    QMap<QString, Tree*>::iterator i;
    if (primaryName != "QtCore") {
        i = forest_.find("QtCore");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtCore");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtGui") {
        i = forest_.find("QtGui");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtGui");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtNetwork") {
        i = forest_.find("QtNetwork");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtNetwork");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtOpenGL") {
        i = forest_.find("QtOpenGL");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtOpenGL");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtWidgets") {
        i = forest_.find("QtWidgets");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtWidgets");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtSql") {
        i = forest_.find("QtSql");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtSql");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtXml") {
        i = forest_.find("QtXml");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtXml");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtSvg") {
        i = forest_.find("QtSvg");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtSvg");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtDoc") {
        i = forest_.find("QtDoc");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtDoc");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtQuick") {
        i = forest_.find("QtQuick");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtQuick");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtQml") {
        i = forest_.find("QtQml");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtQml");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtPrintSupport") {
        i = forest_.find("QtPrintSupport");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtPrintSupport");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtGraphicalEffects") {
        i = forest_.find("QtGraphicalEffects");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtGraphicalEffects");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtConcurrent") {
        i = forest_.find("QtConcurrent");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtConcurrent");
            forest_.erase(i);
        }
    }
#if 0
    if (primaryName != "zzz") {
        i = forest_.find("zzz");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("zzz");
            forest_.erase(i);
        }
    }
#endif
    /*
      If any trees remain in the forest, just add them
      to the search order sequentially, because we don't
      know any better at this point.
     */
    if (!forest_.isEmpty()) {
        i = forest_.begin();
        while (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append(i.key());
            ++i;
        }
        forest_.clear();
    }

    /*
      Rebuild the forest after constructing the search order.
      It was destroyed during construction of the search order,
      but it is needed for module-specific searches.
     */
    for (int i=0; i<searchOrder_.size(); ++i) {
        forest_.insert(moduleNames_.at(i).toLower(), searchOrder_.at(i));
    }

#if 0
    qDebug() << "  SEARCH ORDER:";
    for (int i=0; i<moduleNames_.size(); ++i)
        qDebug() << "    " << i+1 << "." << moduleNames_.at(i);
#endif
}

/*!
  Returns an ordered array of Tree pointers that represents
  the order in which the trees should be searched. The first
  Tree in the array is the tree for the current module, i.e.
  the module for which qdoc is generating documentation.

  The other Tree pointers in the array represent the index
  files that were loaded in preparation for generating this
  module's documentation. Each Tree pointer represents one
  index file. The index file Tree points have been ordered
  heuristically to, hopefully, minimize searching. Thr order
  will probably be changed.

  If the search order array is empty, this function calls
  indexSearchOrder(). The search order array is empty while
  the index files are being loaded, but some searches must
  be performed during this time, notably searches for base
  class nodes. These searches require a temporary search
  order. The temporary order changes throughout the loading
  of the index files, but it is always the tree for the
  current index file first, followed by the trees for the
  index files that have already been loaded. The only
  ordering required in this temporary search order is that
  the current tree must be searched first.
 */
const QVector<Tree*>& QDocForest::searchOrder()
{
    if (searchOrder_.isEmpty())
        return indexSearchOrder();
    return searchOrder_;
}

/*!
  There are two search orders used by qdoc when searching for
  things. The normal search order is returned by searchOrder(),
  but this normal search order is not known until all the index
  files have been read. At that point, setSearchOrder() is
  called.

  During the reading of the index files, the vector holding
  the normal search order remains empty. Whenever the search
  order is requested, if that vector is empty, this function
  is called to return a temporary search order, which includes
  all the index files that have been read so far, plus the
  one being read now. That one is prepended to the front of
  the vector.
 */
const QVector<Tree*>& QDocForest::indexSearchOrder()
{
    if (forest_.size() > indexSearchOrder_.size())
        indexSearchOrder_.prepend(primaryTree_);
    return indexSearchOrder_;
}

/*!
  Create a new Tree for the index file for the specified
  \a module and add it to the forest. Return the pointer
  to its root.
 */
NamespaceNode* QDocForest::newIndexTree(const QString& module)
{
    primaryTree_ = new Tree(module, qdb_);
    forest_.insert(module, primaryTree_);
    return primaryTree_->root();
}

/*!
  Create a new Tree for use as the primary tree. This tree
  will represent the primary module.
 */
void QDocForest::newPrimaryTree(const QString& module)
{
    primaryTree_ = new Tree(module, qdb_);
}

/*!
  Searches through the forest for a node named \a targetPath
  and returns a pointer to it if found. The \a relative node
  is the starting point. It only makes sense for the primary
  tree, which is searched first. After the primary tree has
  been searched, \a relative is set to 0 for searching the
  other trees, which are all index trees. With relative set
  to 0, the starting point for each index tree is the root
  of the index tree.
 */
const Node* QDocForest::findNodeForTarget(QStringList& targetPath,
                                          const Node* relative,
                                          Node::Genus genus,
                                          QString& ref)
{
    int flags = SearchBaseClasses | SearchEnumValues;

    QString entity = targetPath.at(0);
    targetPath.removeFirst();
    QStringList entityPath = entity.split("::");

    QString target;
    if (!targetPath.isEmpty()) {
        target = targetPath.at(0);
        targetPath.removeFirst();
    }

    foreach (Tree* t, searchOrder()) {
        const Node* n = t->findNodeForTarget(entityPath, target, relative, flags, genus, ref);
        if (n)
            return n;
        relative = 0;
    }
    return 0;
}

/*!
  This function merges all the collection maps for collection
  nodes of node type \a t into the collection multimap \a cnmm,
  which is cleared before starting.

  This is mainly useful for groups, which often cross module
  boundaries. It might be true that neither modules nor QML
  modules cross module boundaries, but this function works for
  those cases as well.
 */
void QDocForest::mergeCollectionMaps(Node::Type nt, CNMultiMap& cnmm)
{
    foreach (Tree* t, searchOrder()) {
        const CNMap& cnm = t->getCollections(nt);
        if (!cnm.isEmpty()) {
            CNMap::const_iterator i = cnm.begin();
            while (i != cnm.end()) {
                if (!i.value()->isInternal())
                    cnmm.insert(i.key(), i.value());
                ++i;
            }
        }
    }
}

/*! \class QDocDatabase
  This class provides exclusive access to the qdoc database,
  which consists of a forrest of trees and a lot of maps and
  other useful data structures.
 */

QDocDatabase* QDocDatabase::qdocDB_ = NULL;
NodeMap QDocDatabase::typeNodeMap_;

/*!
  Constructs the singleton qdoc database object. The singleton
  constructs the \a forest_ object, which is also a singleton.
  \a showInternal_ is normally false. If it is true, qdoc will
  write documentation for nodes marked \c internal.
 */
QDocDatabase::QDocDatabase() : showInternal_(false), forest_(this)
{
    // nothing
}

/*!
  Destroys the qdoc database object. This requires destroying
  the forest object, which contains an array of tree pointers.
  Each tree is deleted.
 */
QDocDatabase::~QDocDatabase()
{
    // nothing.
}

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
*/
QDocDatabase* QDocDatabase::qdocDB()
{
    if (!qdocDB_) {
      qdocDB_ = new QDocDatabase;
      initializeDB();
    }
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
  Initialize data structures in the singleton qdoc database.

  In particular, the type node map is initialized with a lot
  type names that don't refer to documented types. For example,
  the C++ standard types are included. These might be documented
  here at some point, but for now they are not. Other examples
  include \c array and \c data, which are just generic names
  used as place holders in function signatures that appear in
  the documentation.

  Also calls Node::initialize() to initialize the search goal map.
 */
void QDocDatabase::initializeDB()
{
    Node::initialize();
    typeNodeMap_.insert( "accepted", 0);
    typeNodeMap_.insert( "actionPerformed", 0);
    typeNodeMap_.insert( "activated", 0);
    typeNodeMap_.insert( "alias", 0);
    typeNodeMap_.insert( "anchors", 0);
    typeNodeMap_.insert( "any", 0);
    typeNodeMap_.insert( "array", 0);
    typeNodeMap_.insert( "autoSearch", 0);
    typeNodeMap_.insert( "axis", 0);
    typeNodeMap_.insert( "backClicked", 0);
    typeNodeMap_.insert( "bool", 0);
    typeNodeMap_.insert( "boomTime", 0);
    typeNodeMap_.insert( "border", 0);
    typeNodeMap_.insert( "buttonClicked", 0);
    typeNodeMap_.insert( "callback", 0);
    typeNodeMap_.insert( "char", 0);
    typeNodeMap_.insert( "clicked", 0);
    typeNodeMap_.insert( "close", 0);
    typeNodeMap_.insert( "closed", 0);
    typeNodeMap_.insert( "color", 0);
    typeNodeMap_.insert( "cond", 0);
    typeNodeMap_.insert( "data", 0);
    typeNodeMap_.insert( "dataReady", 0);
    typeNodeMap_.insert( "dateString", 0);
    typeNodeMap_.insert( "dateTimeString", 0);
    typeNodeMap_.insert( "datetime", 0);
    typeNodeMap_.insert( "day", 0);
    typeNodeMap_.insert( "deactivated", 0);
    typeNodeMap_.insert( "double", 0);
    typeNodeMap_.insert( "drag", 0);
    typeNodeMap_.insert( "easing", 0);
    typeNodeMap_.insert( "enumeration", 0);
    typeNodeMap_.insert( "error", 0);
    typeNodeMap_.insert( "exposure", 0);
    typeNodeMap_.insert( "fatalError", 0);
    typeNodeMap_.insert( "fileSelected", 0);
    typeNodeMap_.insert( "flags", 0);
    typeNodeMap_.insert( "float", 0);
    typeNodeMap_.insert( "focus", 0);
    typeNodeMap_.insert( "focusZone", 0);
    typeNodeMap_.insert( "format", 0);
    typeNodeMap_.insert( "framePainted", 0);
    typeNodeMap_.insert( "from", 0);
    typeNodeMap_.insert( "frontClicked", 0);
    typeNodeMap_.insert( "function", 0);
    typeNodeMap_.insert( "hasOpened", 0);
    typeNodeMap_.insert( "hovered", 0);
    typeNodeMap_.insert( "hoveredTitle", 0);
    typeNodeMap_.insert( "hoveredUrl", 0);
    typeNodeMap_.insert( "imageCapture", 0);
    typeNodeMap_.insert( "imageProcessing", 0);
    typeNodeMap_.insert( "index", 0);
    typeNodeMap_.insert( "initialized", 0);
    typeNodeMap_.insert( "int", 0);
    typeNodeMap_.insert( "isLoaded", 0);
    typeNodeMap_.insert( "item", 0);
    typeNodeMap_.insert( "jsdict", 0);
    typeNodeMap_.insert( "jsobject", 0);
    typeNodeMap_.insert( "key", 0);
    typeNodeMap_.insert( "keysequence", 0);
    typeNodeMap_.insert( "list", 0);
    typeNodeMap_.insert( "listViewClicked", 0);
    typeNodeMap_.insert( "loadRequest", 0);
    typeNodeMap_.insert( "locale", 0);
    typeNodeMap_.insert( "location", 0);
    typeNodeMap_.insert( "long", 0);
    typeNodeMap_.insert( "message", 0);
    typeNodeMap_.insert( "messageReceived", 0);
    typeNodeMap_.insert( "mode", 0);
    typeNodeMap_.insert( "month", 0);
    typeNodeMap_.insert( "name", 0);
    typeNodeMap_.insert( "number", 0);
    typeNodeMap_.insert( "object", 0);
    typeNodeMap_.insert( "offset", 0);
    typeNodeMap_.insert( "ok", 0);
    typeNodeMap_.insert( "openCamera", 0);
    typeNodeMap_.insert( "openImage", 0);
    typeNodeMap_.insert( "openVideo", 0);
    typeNodeMap_.insert( "padding", 0);
    typeNodeMap_.insert( "parent", 0);
    typeNodeMap_.insert( "path", 0);
    typeNodeMap_.insert( "photoModeSelected", 0);
    typeNodeMap_.insert( "position", 0);
    typeNodeMap_.insert( "precision", 0);
    typeNodeMap_.insert( "presetClicked", 0);
    typeNodeMap_.insert( "preview", 0);
    typeNodeMap_.insert( "previewSelected", 0);
    typeNodeMap_.insert( "progress", 0);
    typeNodeMap_.insert( "puzzleLost", 0);
    typeNodeMap_.insert( "qmlSignal", 0);
    typeNodeMap_.insert( "real", 0);
    typeNodeMap_.insert( "rectangle", 0);
    typeNodeMap_.insert( "request", 0);
    typeNodeMap_.insert( "requestId", 0);
    typeNodeMap_.insert( "section", 0);
    typeNodeMap_.insert( "selected", 0);
    typeNodeMap_.insert( "send", 0);
    typeNodeMap_.insert( "settingsClicked", 0);
    typeNodeMap_.insert( "shoe", 0);
    typeNodeMap_.insert( "short", 0);
    typeNodeMap_.insert( "signed", 0);
    typeNodeMap_.insert( "sizeChanged", 0);
    typeNodeMap_.insert( "size_t", 0);
    typeNodeMap_.insert( "sockaddr", 0);
    typeNodeMap_.insert( "someOtherSignal", 0);
    typeNodeMap_.insert( "sourceSize", 0);
    typeNodeMap_.insert( "startButtonClicked", 0);
    typeNodeMap_.insert( "state", 0);
    typeNodeMap_.insert( "std::initializer_list", 0);
    typeNodeMap_.insert( "std::list", 0);
    typeNodeMap_.insert( "std::map", 0);
    typeNodeMap_.insert( "std::pair", 0);
    typeNodeMap_.insert( "std::string", 0);
    typeNodeMap_.insert( "std::vector", 0);
    typeNodeMap_.insert( "string", 0);
    typeNodeMap_.insert( "stringlist", 0);
    typeNodeMap_.insert( "swapPlayers", 0);
    typeNodeMap_.insert( "symbol", 0);
    typeNodeMap_.insert( "t", 0);
    typeNodeMap_.insert( "T", 0);
    typeNodeMap_.insert( "tagChanged", 0);
    typeNodeMap_.insert( "timeString", 0);
    typeNodeMap_.insert( "timeout", 0);
    typeNodeMap_.insert( "to", 0);
    typeNodeMap_.insert( "toggled", 0);
    typeNodeMap_.insert( "type", 0);
    typeNodeMap_.insert( "unsigned", 0);
    typeNodeMap_.insert( "urllist", 0);
    typeNodeMap_.insert( "va_list", 0);
    typeNodeMap_.insert( "value", 0);
    typeNodeMap_.insert( "valueEmitted", 0);
    typeNodeMap_.insert( "videoFramePainted", 0);
    typeNodeMap_.insert( "videoModeSelected", 0);
    typeNodeMap_.insert( "videoRecorder", 0);
    typeNodeMap_.insert( "void", 0);
    typeNodeMap_.insert( "volatile", 0);
    typeNodeMap_.insert( "wchar_t", 0);
    typeNodeMap_.insert( "x", 0);
    typeNodeMap_.insert( "y", 0);
    typeNodeMap_.insert( "zoom", 0);
    typeNodeMap_.insert( "zoomTo", 0);
}

/*! \fn NamespaceNode* QDocDatabase::primaryTreeRoot()
  Returns a pointer to the root node of the primary tree.
 */

/*!
  \fn const GroupMap& QDocDatabase::groups()
  Returns a const reference to the collection of all
  group nodes in the primary tree.
*/

/*!
  \fn const ModuleMap& QDocDatabase::modules()
  Returns a const reference to the collection of all
  module nodes in the primary tree.
*/

/*!
  \fn const QmlModuleMap& QDocDatabase::qmlModules()
  Returns a const reference to the collection of all
  QML module nodes in the primary tree.
*/

/*! \fn GroupNode* QDocDatabase::getGroup(const QString& name)
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, return 0.
 */

/*! \fn GroupNode* QDocDatabase::findGroup(const QString& name)
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, add a new group
  node named \a name and return a pointer to that one.

  If a new group node is added, its parent is the tree root,
  and the new group node is marked \e{not seen}.
 */

/*! \fn ModuleNode* QDocDatabase::findModule(const QString& name)
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found, add a new module
  node named \a name and return a pointer to that one.

  If a new module node is added, its parent is the tree root,
  and the new module node is marked \e{not seen}.
 */

/*! \fn QmlModuleNode* QDocDatabase::findQmlModule(const QString& name)
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found, add a new QML module
  node named \a name and return a pointer to that one.

  If a new QML module node is added, its parent is the tree root,
  and the new QML module node is marked \e{not seen}.
 */

/*! \fn GroupNode* QDocDatabase::addGroup(const QString& name)
  Looks up the group named \a name in the primary tree. If
  a match is found, a pointer to the node is returned.
  Otherwise, a new group node named \a name is created and
  inserted into the collection, and the pointer to that node
  is returned.
 */

/*! \fn ModuleNode* QDocDatabase::addModule(const QString& name)
  Looks up the module named \a name in the primary tree. If
  a match is found, a pointer to the node is returned.
  Otherwise, a new module node named \a name is created and
  inserted into the collection, and the pointer to that node
  is returned.
 */

/*! \fn QmlModuleNode* QDocDatabase::addQmlModule(const QString& name)
  Looks up the QML module named \a name in the primary tree.
  If a match is found, a pointer to the node is returned.
  Otherwise, a new QML module node named \a name is created
  and inserted into the collection, and the pointer to that
  node is returned.
 */

/*! \fn GroupNode* QDocDatabase::addToGroup(const QString& name, Node* node)
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is not found, a new group
  node named \a name is created and inserted into the collection.
  Then append \a node to the group's members list, and append the
  group node to the member list of the \a node. The parent of the
  \a node is not changed by this function. Returns a pointer to
  the group node.
 */

/*! \fn ModuleNode* QDocDatabase::addToModule(const QString& name, Node* node)
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is not found, a new module
  node named \a name is created and inserted into the collection.
  Then append \a node to the module's members list. The parent of
  \a node is not changed by this function. Returns the module node.
 */

/*! \fn QmlModuleNode* QDocDatabase::addToQmlModule(const QString& name, Node* node)
  Looks up the QML module named \a name. If it isn't there,
  create it. Then append \a node to the QML module's member
  list. The parent of \a node is not changed by this function.
 */

/*!
  Looks up the QML type node identified by the qualified Qml
  type \a name and returns a pointer to the QML type node.
 */
QmlClassNode* QDocDatabase::findQmlType(const QString& name)
{
    QmlClassNode* qcn = forest_.lookupQmlType(name);
    if (qcn)
        return qcn;
    return 0;
}

/*!
  Looks up the QML type node identified by the Qml module id
  \a qmid and QML type \a name and returns a pointer to the
  QML type node. The key is \a qmid + "::" + \a name.

  If the QML module id is empty, it looks up the QML type by
  \a name only.
 */
QmlClassNode* QDocDatabase::findQmlType(const QString& qmid, const QString& name)
{
    if (!qmid.isEmpty()) {
        QString t = qmid + "::" + name;
        QmlClassNode* qcn = forest_.lookupQmlType(t);
        if (qcn)
            return qcn;
    }

    QStringList path(name);
    Node* n = forest_.findNodeByNameAndType(path, Node::QmlType);
    if (n && n->isQmlType())
        return static_cast<QmlClassNode*>(n);
    return 0;
}

/*!
  Looks up the QML type node identified by the Qml module id
  constructed from the strings in the \a import record and the
  QML type \a name and returns a pointer to the QML type node.
  If a QML type node is not found, 0 is returned.
 */
QmlClassNode* QDocDatabase::findQmlType(const ImportRec& import, const QString& name)
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
            QmlClassNode* qcn = forest_.lookupQmlType(qualifiedName);
            if (qcn)
                return qcn;
        }
    }
    return 0;
}

/*!
  This function calls \a func for each tree in the forest.
 */
void QDocDatabase::processForest(void (QDocDatabase::*func) (InnerNode*))
{
    Tree* t = forest_.firstTree();
    while (t) {
        (this->*(func))(t->root());
        t = forest_.nextTree();
    }
}

/*!
  Constructs the collection of legalese texts, if it has not
  already been constructed and returns a reference to it.
 */
TextToNodeMap& QDocDatabase::getLegaleseTexts()
{
    if (legaleseTexts_.isEmpty())
        processForest(&QDocDatabase::findAllLegaleseTexts);
    return legaleseTexts_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of C++ classes with obsolete members.
 */
NodeMap& QDocDatabase::getClassesWithObsoleteMembers()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return classesWithObsoleteMembers_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of obsolete QML types.
 */
NodeMap& QDocDatabase::getObsoleteQmlTypes()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return obsoleteQmlTypes_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of QML types with obsolete members.
 */
NodeMap& QDocDatabase::getQmlTypesWithObsoleteMembers()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return qmlTypesWithObsoleteMembers_;
}

/*!
  Constructs the C++ namespace data structure, if it has not
  already been constructed. Returns a reference to it.
 */
NodeMap& QDocDatabase::getNamespaces()
{
    if (namespaceIndex_.isEmpty())
        processForest(&QDocDatabase::findAllNamespaces);
    return namespaceIndex_;
}

/*!
  Construct the C++ class data structures, if they have not
  already been constructed. Returns a reference to the map
  of C++ service clases.

  \note This is currently not used.
 */
NodeMap& QDocDatabase::getServiceClasses()
{
    if (nonCompatClasses_.isEmpty() && qmlClasses_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return serviceClasses_;
}

/*!
  Construct the data structures for QML basic types, if they
  have not already been constructed. Returns a reference to
  the map of QML basic types.
 */
NodeMap& QDocDatabase::getQmlBasicTypes()
{
    if (nonCompatClasses_.isEmpty() && qmlBasicTypes_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return qmlBasicTypes_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of obsolete QML types.
 */
NodeMap& QDocDatabase::getQmlTypes()
{
    if (nonCompatClasses_.isEmpty() && qmlClasses_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return qmlClasses_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of obsolete C++ clases.
 */
NodeMap& QDocDatabase::getObsoleteClasses()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return obsoleteClasses_;
}

/*!
  Construct the C++ class data structures, if they have not
  already been constructed. Returns a reference to the map
  of compatibility C++ clases.
 */
NodeMap& QDocDatabase::getCompatibilityClasses()
{
    if (nonCompatClasses_.isEmpty() && qmlClasses_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return compatClasses_;
}

/*!
  Construct the C++ class data structures, if they have not
  already been constructed. Returns a reference to the map
  of main C++ clases.

  \note The main C++ classes data structure is currently not
  used.
 */
NodeMap& QDocDatabase::getMainClasses()
{
    if (nonCompatClasses_.isEmpty() && qmlClasses_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return mainClasses_;
}

/*!
  Construct the C++ class data structures, if they have not
  already been constructed. Returns a reference to the map
  of all C++ classes.
 */
NodeMap& QDocDatabase::getCppClasses()
{
    if (nonCompatClasses_.isEmpty() && qmlClasses_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return nonCompatClasses_;
}

/*!
  Finds all the C++ class nodes and QML type nodes and
  sorts them into maps.
 */
void QDocDatabase::findAllClasses(InnerNode* node)
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
            else if (((*c)->isQmlType() || (*c)->isQmlBasicType())&& !(*c)->doc().isEmpty()) {
                QString qmlTypeName = (*c)->name();
                if (qmlTypeName.startsWith(QLatin1String("QML:")))
                    qmlClasses_.insert(qmlTypeName.mid(4),*c);
                else
                    qmlClasses_.insert(qmlTypeName,*c);

                //also add to the QML basic type map
                if ((*c)->isQmlBasicType())
                    qmlBasicTypes_.insert(qmlTypeName,*c);
            }
            else if ((*c)->isInnerNode()) {
                findAllClasses(static_cast<InnerNode*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Construct the function index data structure and return it.
  This data structure is used to output the function index page.
 */
NodeMapMap& QDocDatabase::getFunctionIndex()
{
    funcIndex_.clear();
    processForest(&QDocDatabase::findAllFunctions);
    return funcIndex_;
}

/*!
  Finds all the function nodes
 */
void QDocDatabase::findAllFunctions(InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode()) {
                findAllFunctions(static_cast<InnerNode*>(*c));
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
void QDocDatabase::findAllLegaleseTexts(InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if (!(*c)->doc().legaleseText().isEmpty())
                legaleseTexts_.insertMulti((*c)->doc().legaleseText(), *c);
            if ((*c)->isInnerNode())
                findAllLegaleseTexts(static_cast<InnerNode *>(*c));
        }
        ++c;
    }
}

/*!
  Finds all the namespace nodes and puts them in an index.
 */
void QDocDatabase::findAllNamespaces(InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode()) {
                findAllNamespaces(static_cast<InnerNode *>(*c));
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
void QDocDatabase::findAllObsoleteThings(InnerNode* node)
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
                else if ((*c)->isQmlType()) {
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
            else if ((*c)->isQmlType()) {
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
                                if (parent && parent->isQmlType() && !parent->name().isEmpty())
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
void QDocDatabase::findAllSince(InnerNode* node)
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
                else if ((*child)->isQmlType()) {
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
        }
        // Recursively find child nodes with since commands.
        if ((*child)->isInnerNode())
            findAllSince(static_cast<InnerNode *>(*child));

        ++child;
    }
}

/*!
  Find the \a key in the map of new class maps, and return a
  reference to the value, which is a NodeMap. If \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getClassMap(const QString& key)
{
    if (newSinceMaps_.isEmpty() && newClassMaps_.isEmpty() && newQmlTypeMaps_.isEmpty())
        processForest(&QDocDatabase::findAllSince);
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
const NodeMap& QDocDatabase::getQmlTypeMap(const QString& key)
{
    if (newSinceMaps_.isEmpty() && newClassMaps_.isEmpty() && newQmlTypeMaps_.isEmpty())
        processForest(&QDocDatabase::findAllSince);
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
const NodeMultiMap& QDocDatabase::getSinceMap(const QString& key)
{
    if (newSinceMaps_.isEmpty() && newClassMaps_.isEmpty() && newQmlTypeMaps_.isEmpty())
        processForest(&QDocDatabase::findAllSince);
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
    resolveQmlInheritance(primaryTreeRoot());
    resolveTargets();
    primaryTree()->resolveCppToQmlLinks();
}

/*!
  This function is called for autolinking to a \a type,
  which could be a function return type or a parameter
  type. The tree node that represents the \a type is
  returned. All the trees are searched until a match is
  found. When searching the primary tree, the search
  begins at \a relative and proceeds up the parent chain.
  When searching the index trees, the search begins at the
  root.
 */
const Node* QDocDatabase::findTypeNode(const QString& type, const Node* relative)
{
    QStringList path = type.split("::");
    if ((path.size() == 1) && (path.at(0)[0].isLower() || path.at(0) == QString("T"))) {
        NodeMap::iterator i = typeNodeMap_.find(path.at(0));
        if (i != typeNodeMap_.end())
            return i.value();
    }
    return forest_.findTypeNode(path, relative);
}

/*!
  Finds the node that will generate the documentation that
  contains the \a target and returns a pointer to it.

  Can this be improved by using the target map in Tree?
 */
const Node* QDocDatabase::findNodeForTarget(const QString& target, const Node* relative)
{
    const Node* node = 0;
    if (target.isEmpty())
        node = relative;
    else if (target.endsWith(".html"))
        node = findNodeByNameAndType(QStringList(target), Node::Document);
    else {
        QStringList path = target.split("::");
        int flags = SearchBaseClasses | SearchEnumValues; // | NonFunction;
        foreach (Tree* t, searchOrder()) {
            const Node* n = t->findNode(path, relative, flags, Node::DontCare);
            if (n)
                return n;
            relative = 0;
        }
        node = findDocNodeByTitle(target);
    }
    return node;
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
    NodeMap previousSearches;
    // Do we need recursion?
    foreach (Node* child, root->childNodes()) {
        if (child->isQmlType()) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
            if (qcn->qmlBaseNodeNotSet() && !qcn->qmlBaseName().isEmpty()) {
                QmlClassNode* bqcn = static_cast<QmlClassNode*>(previousSearches.value(qcn->qmlBaseName()));
                if (bqcn)
                    qcn->setQmlBaseNode(bqcn);
                else {
                    if (!qcn->importList().isEmpty()) {
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
                        previousSearches.insert(qcn->qmlBaseName(), bqcn);
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
  Find a node of the specified \a type that is reached with
  the specified \a path qualified with the name of one of the
  open namespaces (might not be any open ones). If the node
  is found in an open namespace, prefix \a path with the name
  of the open namespace and "::" and return a pointer to the
  node. Othewrwise return 0.

  This function only searches in the current primary tree.
 */
Node* QDocDatabase::findNodeInOpenNamespace(QStringList& path, Node::Type type)
{
    if (path.isEmpty())
        return 0;
    Node* n = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList p;
            if (t != path[0])
                p = t.split("::") + path;
            else
                p = path;
            n = primaryTree()->findNodeByNameAndType(p, type);
            if (n) {
                path = p;
                break;
            }
        }
    }
    return n;
}

/*!
  Finds all the collection nodes of type \a nt into the
  collection node map \a cnn. Nodes that match \a relative
  are not included.
 */
void QDocDatabase::mergeCollections(Node::Type nt, CNMap& cnm, const Node* relative)
{
    QRegExp singleDigit("\\b([0-9])\\b");
    CNMultiMap cnmm;
    forest_.mergeCollectionMaps(nt, cnmm);
    cnm.clear();
    if (cnmm.isEmpty())
        return;
    QStringList keys = cnmm.uniqueKeys();
    foreach (QString key, keys) {
        QList<CollectionNode*> values = cnmm.values(key);
        CollectionNode* n = 0;
        foreach (CollectionNode* v, values) {
            if (v && v->wasSeen() && (v != relative)) {
                n = v;
                break;
            }
        }
        if (n) {
            if (values.size() > 1) {
                foreach (CollectionNode* v, values) {
                    if (v != n) {
                        foreach (Node* t, v->members())
                            n->addMember(t);
                    }
                }
            }
            if (!n->members().isEmpty()) {
                QString sortKey = n->fullTitle().toLower();
                if (sortKey.startsWith("the "))
                    sortKey.remove(0, 4);
                sortKey.replace(singleDigit, "0\\1");
                cnm.insert(sortKey, n);
            }
        }
    }
}

/*!
  Finds all the collection nodes with the same name
  and type as \a cn and merges their members into the
  members list of \a cn.
 */
void QDocDatabase::mergeCollections(CollectionNode* cn)
{
    CollectionList cl;
    forest_.getCorrespondingCollections(cn, cl);
    if (!cl.empty()) {
        foreach (CollectionNode* v, cl) {
            if (v != cn) {
                foreach (Node* t, v->members())
                    cn->addMember(t);
            }
        }
    }
}

/*!
  Searches for the node that matches the path in \a atom. The
  \a relative node is used if the first leg of the path is
  empty, i.e. if the path begins with a hashtag. The function
  also sets \a ref if there remains an unused leg in the path
  after the node is found. The node is returned as well as the
  \a ref. If the returned node pointer is null, \a ref is not
  valid.
 */
const Node* QDocDatabase::findNodeForAtom(const Atom* atom, const Node* relative, QString& ref)
{
    const Node* node = 0;

    QStringList targetPath = atom->string().split("#");
    QString first = targetPath.first().trimmed();

    Tree* domain = 0;
    Node::Genus genus = Node::DontCare;
    // Reserved for future use
    //Node::Type goal = Node::NoType;

    if (atom->isLinkAtom()) {
        domain = atom->domain();
        genus = atom->genus();
        // Reserved for future use
        //goal = atom->goal();
    }

    if (first.isEmpty())
        node = relative; // search for a target on the current page.
    else if (domain) {
        if (first.endsWith(".html"))
            node = domain->findNodeByNameAndType(QStringList(first), Node::Document);
        else if (first.endsWith("()"))
            node = domain->findFunctionNode(first, 0, genus);
        else {
            int flags = SearchBaseClasses | SearchEnumValues;
            QStringList nodePath = first.split("::");
            QString target;
            targetPath.removeFirst();
            if (!targetPath.isEmpty()) {
                target = targetPath.at(0);
                targetPath.removeFirst();
            }
            if (relative && relative->tree()->moduleName() != domain->moduleName())
                relative = 0;
            node = domain->findNodeForTarget(nodePath, target, relative, flags, genus, ref);
            return node;
        }
    }
    else {
        if (first.endsWith(".html")) {
            node = findNodeByNameAndType(QStringList(first), Node::Document);
            // the path may also refer to an example file with .html extension
            if (!node && first.contains("/"))
                return findNodeForTarget(targetPath, relative, genus, ref);
        }
        else if (first.endsWith("()"))
            node = findFunctionNode(first, relative, genus);
        else {
            node = findNodeForTarget(targetPath, relative, genus, ref);
            return node;
        }
    }

    if (node && ref.isEmpty()) {
        if (!node->url().isEmpty())
            return node;
        targetPath.removeFirst();
        if (!targetPath.isEmpty()) {
            ref = node->root()->tree()->getRef(targetPath.first(), node);
            if (ref.isEmpty())
                node = 0;
        }
    }
    return node;
}

QT_END_NAMESPACE

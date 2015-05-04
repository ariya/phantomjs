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

#ifndef QDOCDATABASE_H
#define QDOCDATABASE_H

#include <qstring.h>
#include <qmap.h>
#include "tree.h"
#include "config.h"
#include "text.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

typedef QMap<QString, NodeMap> NodeMapMap;
typedef QMap<QString, NodeMultiMap> NodeMultiMapMap;
typedef QMultiMap<QString, Node*> QDocMultiMap;
typedef QMap<Text, const Node*> TextToNodeMap;
typedef QList<CollectionNode*> CollectionList;

class Atom;
class Generator;
class QDocDatabase;

enum FindFlag {
    SearchBaseClasses = 0x1,
    SearchEnumValues = 0x2,
    NonFunction = 0x4
};

class QDocForest
{
  public:
    friend class QDocDatabase;
    QDocForest(QDocDatabase* qdb)
        : qdb_(qdb), primaryTree_(0), currentIndex_(0) { }
    ~QDocForest();

    NamespaceNode* firstRoot();
    NamespaceNode* nextRoot();
    Tree* firstTree();
    Tree* nextTree();
    Tree* primaryTree() { return primaryTree_; }
    Tree* findTree(const QString& t) { return forest_.value(t); }
    NamespaceNode* primaryTreeRoot() { return (primaryTree_ ? primaryTree_->root() : 0); }
    bool isEmpty() { return searchOrder().isEmpty(); }
    bool done() { return (currentIndex_ >= searchOrder().size()); }
    const QVector<Tree*>& searchOrder();
    const QVector<Tree*>& indexSearchOrder();
    void setSearchOrder();

    const Node* findNode(const QStringList& path,
                         const Node* relative,
                         int findFlags,
                         Node::Genus genus) {
        foreach (Tree* t, searchOrder()) {
            const Node* n = t->findNode(path, relative, findFlags, genus);
            if (n)
                return n;
            relative = 0;
        }
        return 0;
    }

    Node* findNodeByNameAndType(const QStringList& path, Node::Type type) {
        foreach (Tree* t, searchOrder()) {
            Node* n = t->findNodeByNameAndType(path, type);
            if (n)
                return n;
        }
        return 0;
    }

    ClassNode* findClassNode(const QStringList& path) {
        foreach (Tree* t, searchOrder()) {
            ClassNode* n = t->findClassNode(path);
            if (n)
                return n;
        }
        return 0;
    }

    Node* findNodeForInclude(const QStringList& path) {
        foreach (Tree* t, searchOrder()) {
            Node* n = t->findNodeForInclude(path);
            if (n)
                return n;
        }
        return 0;
    }

    InnerNode* findRelatesNode(const QStringList& path) {
        foreach (Tree* t, searchOrder()) {
            InnerNode* n = t->findRelatesNode(path);
            if (n)
                return n;
        }
        return 0;
    }

    const Node* findFunctionNode(const QString& target,
                                 const Node* relative,
                                 Node::Genus genus) {
        foreach (Tree* t, searchOrder()) {
            const Node* n = t->findFunctionNode(target, relative, genus);
            if (n)
                return n;
            relative = 0;
        }
        return 0;
    }
    const Node* findNodeForTarget(QStringList& targetPath,
                                  const Node* relative,
                                  Node::Genus genus,
                                  QString& ref);

    const Node* findTypeNode(const QStringList& path, const Node* relative)
    {
        foreach (Tree* t, searchOrder()) {
            int flags = SearchBaseClasses | SearchEnumValues | NonFunction;
            const Node* n = t->findNode(path, relative, flags, Node::DontCare);
            if (n)
                return n;
            relative = 0;
        }
        return 0;
    }

    const DocNode* findDocNodeByTitle(const QString& title)
    {
        foreach (Tree* t, searchOrder()) {
            const DocNode* n = t->findDocNodeByTitle(title);
            if (n)
                return n;
        }
        return 0;
    }

    QmlClassNode* lookupQmlType(const QString& name)
    {
        foreach (Tree* t, searchOrder()) {
            QmlClassNode* qcn = t->lookupQmlType(name);
            if (qcn)
                return qcn;
        }
        return 0;
    }
    void mergeCollectionMaps(Node::Type nt, CNMultiMap& cnmm);
    void getCorrespondingCollections(CollectionNode* cn, CollectionList& cl)
    {
        foreach (Tree* t, searchOrder()) {
            CollectionNode* ccn = t->getCorrespondingCollection(cn);
            if (ccn)
                cl.append(ccn);
        }
    }

  private:
    void newPrimaryTree(const QString& module);
    NamespaceNode* newIndexTree(const QString& module);

  private:
    QDocDatabase*          qdb_;
    Tree*                  primaryTree_;
    int                    currentIndex_;
    QMap<QString, Tree*>   forest_;
    QVector<Tree*>         searchOrder_;
    QVector<Tree*>         indexSearchOrder_;
    QVector<QString>       moduleNames_;
};

class QDocDatabase
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::QDocDatabase)

  public:
    static QDocDatabase* qdocDB();
    static void destroyQdocDB();
    ~QDocDatabase();

    Tree* findTree(const QString& t) { return forest_.findTree(t); }
    const CNMap& groups() { return primaryTree()->groups(); }
    const CNMap& modules() { return primaryTree()->modules(); }
    const CNMap& qmlModules() { return primaryTree()->qmlModules(); }

    GroupNode* getGroup(const QString& name) { return primaryTree()->getGroup(name); }
    GroupNode* findGroup(const QString& name) { return primaryTree()->findGroup(name); }
    ModuleNode* findModule(const QString& name) { return primaryTree()->findModule(name); }
    QmlModuleNode* findQmlModule(const QString& name) { return primaryTree()->findQmlModule(name); }

    GroupNode* addGroup(const QString& name) { return primaryTree()->addGroup(name); }
    ModuleNode* addModule(const QString& name) { return primaryTree()->addModule(name); }
    QmlModuleNode* addQmlModule(const QString& name) { return primaryTree()->addQmlModule(name); }

    GroupNode* addToGroup(const QString& name, Node* node) {
        return primaryTree()->addToGroup(name, node);
    }
    ModuleNode* addToModule(const QString& name, Node* node) {
        return primaryTree()->addToModule(name, node);
    }
    QmlModuleNode* addToQmlModule(const QString& name, Node* node) {
        return primaryTree()->addToQmlModule(name, node);
    }
    void addExampleNode(ExampleNode* n) { primaryTree()->addExampleNode(n); }
    ExampleNodeMap& exampleNodeMap() { return primaryTree()->exampleNodeMap(); }

    QmlClassNode* findQmlType(const QString& name);
    QmlClassNode* findQmlType(const QString& qmid, const QString& name);
    QmlClassNode* findQmlType(const ImportRec& import, const QString& name);

 private:
    void findAllClasses(InnerNode *node);
    void findAllFunctions(InnerNode *node);
    void findAllLegaleseTexts(InnerNode *node);
    void findAllNamespaces(InnerNode *node);
    void findAllObsoleteThings(InnerNode* node);
    void findAllSince(InnerNode *node);

 public:
    /*******************************************************************
     special collection access functions
    ********************************************************************/
    NodeMap& getCppClasses();
    NodeMap& getMainClasses();
    NodeMap& getCompatibilityClasses();
    NodeMap& getObsoleteClasses();
    NodeMap& getClassesWithObsoleteMembers();
    NodeMap& getObsoleteQmlTypes();
    NodeMap& getQmlTypesWithObsoleteMembers();
    NodeMap& getNamespaces();
    NodeMap& getServiceClasses();
    NodeMap& getQmlBasicTypes();
    NodeMap& getQmlTypes();
    NodeMapMap& getFunctionIndex();
    TextToNodeMap& getLegaleseTexts();
    const NodeMap& getClassMap(const QString& key);
    const NodeMap& getQmlTypeMap(const QString& key);
    const NodeMultiMap& getSinceMap(const QString& key);

    /*******************************************************************
      Many of these will be either eliminated or replaced.
    ********************************************************************/
    void resolveInheritance() { primaryTree()->resolveInheritance(); }
    void resolveQmlInheritance(InnerNode* root);
    void resolveIssues();
    void fixInheritance() { primaryTree()->fixInheritance(); }
    void resolveProperties() { primaryTree()->resolveProperties(); }

    void resolveTargets() {
        primaryTree()->resolveTargets(primaryTreeRoot());
    }
    void insertTarget(const QString& name,
                      const QString& title,
                      TargetRec::Type type,
                      Node* node,
                      int priority) {
        primaryTree()->insertTarget(name, title, type, node, priority);
    }

    /*******************************************************************
      The functions declared below are called for the current tree only.
    ********************************************************************/
    FunctionNode* findFunctionNode(const QStringList& parentPath, const FunctionNode* clone) {
        return primaryTree()->findFunctionNode(parentPath, clone);
    }
    FunctionNode* findNodeInOpenNamespace(const QStringList& parentPath, const FunctionNode* clone);
    Node* findNodeInOpenNamespace(QStringList& path, Node::Type type);
    const Node* checkForCollision(const QString& name) {
        return primaryTree()->checkForCollision(name);
    }
    /*******************************************************************/

    /*******************************************************************
      The functions declared below handle the parameters in '[' ']'.
    ********************************************************************/
    const Node* findNodeForAtom(const Atom* atom, const Node* relative, QString& ref);
    /*******************************************************************/

    /*******************************************************************
      The functions declared below are called for all trees.
    ********************************************************************/
    ClassNode* findClassNode(const QStringList& path) { return forest_.findClassNode(path); }
    Node* findNodeForInclude(const QStringList& path) { return forest_.findNodeForInclude(path); }
    InnerNode* findRelatesNode(const QStringList& path) { return forest_.findRelatesNode(path); }
    const Node* findFunctionNode(const QString& target, const Node* relative, Node::Genus genus) {
        return forest_.findFunctionNode(target, relative, genus);
    }
    const Node* findTypeNode(const QString& type, const Node* relative);
    const Node* findNodeForTarget(const QString& target, const Node* relative);
    const DocNode* findDocNodeByTitle(const QString& title) {
        return forest_.findDocNodeByTitle(title);
    }
    Node* findNodeByNameAndType(const QStringList& path, Node::Type type) {
        return forest_.findNodeByNameAndType(path, type);
    }

  private:
    const Node* findNodeForTarget(QStringList& targetPath,
                                  const Node* relative,
                                  Node::Genus genus,
                                  QString& ref) {
        return forest_.findNodeForTarget(targetPath, relative, genus, ref);
    }

    /*******************************************************************/
  public:
    void addPropertyFunction(PropertyNode* property,
                             const QString& funcName,
                             PropertyNode::FunctionRole funcRole) {
        primaryTree()->addPropertyFunction(property, funcName, funcRole);
    }

    void setVersion(const QString& v) { version_ = v; }
    QString version() const { return version_; }

    void generateTagFile(const QString& name, Generator* g);
    void readIndexes(const QStringList& indexFiles);
    void generateIndex(const QString& fileName,
                       const QString& url,
                       const QString& title,
                       Generator* g,
                       bool generateInternalNodes = false);

    void clearOpenNamespaces() { openNamespaces_.clear(); }
    void insertOpenNamespace(const QString& path) { openNamespaces_.insert(path); }
    void setShowInternal(bool value) { showInternal_ = value; }

    // Try to make this function private.
    QDocForest& forest() { return forest_; }
    NamespaceNode* primaryTreeRoot() { return forest_.primaryTreeRoot(); }
    void newPrimaryTree(const QString& module) { forest_.newPrimaryTree(module); }
    NamespaceNode* newIndexTree(const QString& module) { return forest_.newIndexTree(module); }
    const QVector<Tree*>& searchOrder() { return forest_.searchOrder(); }
    void setLocalSearch() { forest_.searchOrder_ = QVector<Tree*>(1, primaryTree()); }
    void setSearchOrder(const QVector<Tree*>& searchOrder) { forest_.searchOrder_ = searchOrder; }
    void setSearchOrder() { forest_.setSearchOrder(); }
    void mergeCollections(Node::Type nt, CNMap& cnm, const Node* relative);
    void mergeCollections(CollectionNode* cn);

 private:
    friend class QDocIndexFiles;
    friend class QDocTagFiles;

    const Node* findNode(const QStringList& path,
                         const Node* relative,
                         int findFlags,
                         Node::Genus genus) {
        return forest_.findNode(path, relative, findFlags, genus);
    }
    void processForest(void (QDocDatabase::*) (InnerNode*));
    static void initializeDB();

 private:
    QDocDatabase();
    QDocDatabase(QDocDatabase const& ) : showInternal_(false), forest_(this) { }
    QDocDatabase& operator=(QDocDatabase const& );
    Tree* primaryTree() { return forest_.primaryTree(); }

 public:
    static bool             debug;

 private:
    static QDocDatabase*    qdocDB_;
    static NodeMap          typeNodeMap_;
    bool                    showInternal_;
    QString                 version_;
    QDocForest              forest_;

    NodeMap                 nonCompatClasses_;
    NodeMap                 mainClasses_;
    NodeMap                 compatClasses_;
    NodeMap                 obsoleteClasses_;
    NodeMap                 classesWithObsoleteMembers_;
    NodeMap                 obsoleteQmlTypes_;
    NodeMap                 qmlTypesWithObsoleteMembers_;
    NodeMap                 namespaceIndex_;
    NodeMap                 serviceClasses_;
    NodeMap                 qmlBasicTypes_;
    NodeMap                 qmlClasses_;
    NodeMapMap              newClassMaps_;
    NodeMapMap              newQmlTypeMaps_;
    NodeMultiMapMap         newSinceMaps_;
    NodeMapMap              funcIndex_;
    TextToNodeMap           legaleseTexts_;
    QSet<QString>           openNamespaces_;
};

QT_END_NAMESPACE

#endif

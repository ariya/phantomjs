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

#include "node.h"
#include "tree.h"
#include "codemarker.h"
#include "codeparser.h"
#include <quuid.h>
#include "qdocdatabase.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

int Node::propertyGroupCount_ = 0;
ExampleNodeMap ExampleNode::exampleNodeMap;
QStringMap Node::operators_;

/*!
  Increment the number of property groups seen in the current
  file, and return the new value.
 */
int Node::incPropertyGroupCount() { return ++propertyGroupCount_; }

/*!
  Reset the number of property groups seen in the current file
  to 0, because we are starting a new file.
 */
void Node::clearPropertyGroupCount() { propertyGroupCount_ = 0; }

/*!
  \class Node
  \brief The Node class is a node in the Tree.

  A Node represents a class or function or something else
  from the source code..
 */

/*!
  When this Node is destroyed, if it has a parent Node, it
  removes itself from the parent node's child list.
 */
Node::~Node()
{
    if (parent_)
        parent_->removeChild(this);
    if (relatesTo_)
        relatesTo_->removeRelated(this);
}

/*!
  Returns this node's name member. Appends "()" to the returned
  name, if this node is a function node.
 */
QString Node::plainName() const
{
    if (type() == Node::Function)
        return name_ + QLatin1String("()");
    return name_;
}

/*!
  Constructs and returns the node's fully qualified name by
  recursively ascending the parent links and prepending each
  parent name + "::". Breaks out when the parent pointer is
  \a relative. Almost all calls to this function pass 0 for
  \a relative.
 */
QString Node::plainFullName(const Node* relative) const
{
    if (name_.isEmpty())
        return QLatin1String("global");

    QString fullName;
    const Node* node = this;
    while (node) {
        fullName.prepend(node->plainName());
        if (node->parent() == relative || node->parent()->subType() == Node::Collision ||
            node->parent()->name().isEmpty())
            break;
        fullName.prepend(QLatin1String("::"));
        node = node->parent();
    }
    return fullName;
}

/*!
  Constructs and returns this node's full name. The \a relative
  node is either null or is a collision node.
 */
QString Node::fullName(const Node* relative) const
{
    if (type() == Node::Document) {
        const DocNode* dn = static_cast<const DocNode*>(this);
        // Only print modulename::type on collision pages.
        if (!dn->qmlModuleName().isEmpty() && relative != 0 && relative->isCollisionNode())
            return dn->qmlModuleName() + "::" + dn->title();
        return dn->title();
    }
    else if (type() == Node::Class) {
        const ClassNode* cn = static_cast<const ClassNode*>(this);
        if (!cn->serviceName().isEmpty())
            return cn->serviceName();
    }
    return plainFullName(relative);
}

/*!
  Sets this Node's Doc to \a doc. If \a replace is false and
  this Node already has a Doc, a warning is reported that the
  Doc is being overridden, and it reports where the previous
  Doc was found. If \a replace is true, the Doc is replaced
  silently.
 */
void Node::setDoc(const Doc& doc, bool replace)
{
    if (!doc_.isEmpty() && !replace) {
        doc.location().warning(tr("Overrides a previous doc"));
        doc_.location().warning(tr("(The previous doc is here)"));
    }
    doc_ = doc;
}

/*!
  Construct a node with the given \a type and having the
  given \a parent and \a name. The new node is added to the
  parent's child list.
 */
Node::Node(Type type, InnerNode *parent, const QString& name)
    : nodeType_(type),
      access_(Public),
      safeness_(UnspecifiedSafeness),
      pageType_(NoPageType),
      status_(Commendable),
      indexNodeFlag_(false),
      seen_(true),
      parent_(parent),
      relatesTo_(0),
      name_(name)
{
    if (parent_)
        parent_->addChild(this);
    outSubDir_ = CodeParser::currentOutputSubdirectory();
    if (operators_.isEmpty()) {
        operators_.insert("++","inc");
        operators_.insert("--","dec");
        operators_.insert("==","eq");
        operators_.insert("!=","ne");
        operators_.insert("<<","lt-lt");
        operators_.insert(">>","gt-gt");
        operators_.insert("+=","plus-assign");
        operators_.insert("-=","minus-assign");
        operators_.insert("*=","mult-assign");
        operators_.insert("/=","div-assign");
        operators_.insert("%=","mod-assign");
        operators_.insert("&=","bitwise-and-assign");
        operators_.insert("|=","bitwise-or-assign");
        operators_.insert("^=","bitwise-xor-assign");
        operators_.insert("<<=","bitwise-left-shift-assign");
        operators_.insert(">>=","bitwise-right-shift-assign");
        operators_.insert("||","logical-or");
        operators_.insert("&&","logical-and");
        operators_.insert("()","call");
        operators_.insert("[]","subscript");
        operators_.insert("->","pointer");
        operators_.insert("->*","pointer-star");
        operators_.insert("+","plus");
        operators_.insert("-","minus");
        operators_.insert("*","mult");
        operators_.insert("/","div");
        operators_.insert("%","mod");
        operators_.insert("|","bitwise-or");
        operators_.insert("&","bitwise-and");
        operators_.insert("^","bitwise-xor");
        operators_.insert("!","not");
        operators_.insert("~","bitwise-not");
        operators_.insert("<=","lt-eq");
        operators_.insert(">=","gt-eq");
        operators_.insert("<","lt");
        operators_.insert(">","gt");
        operators_.insert("=","assign");
        operators_.insert(",","comma");
        operators_.insert("delete[]","delete-array");
        operators_.insert("delete","delete");
        operators_.insert("new[]","new-array");
        operators_.insert("new","new");
    }
}

/*!
  Returns the node's URL.
 */
QString Node::url() const
{
    return url_;
}

/*!
  Sets the node's URL to \a url
 */
void Node::setUrl(const QString &url)
{
    url_ = url;
}

/*!
  Returns this node's page type as a string, for use as an
  attribute value in XML or HTML.
 */
QString Node::pageTypeString() const
{
    return pageTypeString(pageType_);
}

/*!
  Returns the page type \a t as a string, for use as an
  attribute value in XML or HTML.
 */
QString Node::pageTypeString(unsigned t)
{
    switch ((PageType)t) {
    case Node::ApiPage:
        return "api";
    case Node::ArticlePage:
        return "article";
    case Node::ExamplePage:
        return "example";
    case Node::HowToPage:
        return "howto";
    case Node::OverviewPage:
        return "overview";
    case Node::TutorialPage:
        return "tutorial";
    case Node::FAQPage:
        return "faq";
    case Node::DitaMapPage:
        return "ditamap";
    default:
        return "article";
    }
}

/*!
  Returns this node's type as a string for use as an
  attribute value in XML or HTML.
 */
QString Node::nodeTypeString() const
{
    return nodeTypeString(type());
}

/*!
  Returns the node type \a t as a string for use as an
  attribute value in XML or HTML.
 */
QString Node::nodeTypeString(unsigned t)
{
    switch ((Type)t) {
    case Namespace:
        return "namespace";
    case Class:
        return "class";
    case Document:
        return "document";
    case Enum:
        return "enum";
    case Typedef:
        return "typedef";
    case Function:
        return "function";
    case Property:
        return "property";
    case Variable:
        return "variable";
    case QmlProperty:
        return "QML property";
    case QmlPropertyGroup:
        return "QML property group";
    case QmlSignal:
        return "QML signal";
    case QmlSignalHandler:
        return "QML signal handler";
    case QmlMethod:
        return "QML method";
    default:
        break;
    }
    return QString();
}

/*!
  Returns this node's subtype as a string for use as an
  attribute value in XML or HTML. This is only useful
  in the case where the node type is Document.
 */
QString Node::nodeSubtypeString() const
{
    return nodeSubtypeString(subType());
}

/*!
  Returns the node subtype \a t as a string for use as an
  attribute value in XML or HTML. This is only useful
  in the case where the node type is Document.
 */
QString Node::nodeSubtypeString(unsigned t)
{
    switch ((SubType)t) {
    case Example:
        return "example";
    case HeaderFile:
        return "header file";
    case File:
        return "file";
    case Image:
        return "image";
    case Group:
        return "group";
    case Module:
        return "module";
    case Page:
        return "page";
    case ExternalPage:
        return "external page";
    case QmlClass:
        return "QML type";
    case QmlBasicType:
        return "QML basic type";
    case QmlModule:
        return "QML module";
    case DitaMap:
        return "ditamap";
    case Collision:
        return "collision";
    case NoSubType:
    default:
        break;
    }
    return QString();
}

/*!
  Set the page type according to the string \a t.
 */
void Node::setPageType(const QString& t)
{
    if ((t == "API") || (t == "api"))
        pageType_ = ApiPage;
    else if (t == "howto")
        pageType_ = HowToPage;
    else if (t == "overview")
        pageType_ = OverviewPage;
    else if (t == "tutorial")
        pageType_ = TutorialPage;
    else if (t == "howto")
        pageType_ = HowToPage;
    else if (t == "article")
        pageType_ = ArticlePage;
    else if (t == "example")
        pageType_ = ExamplePage;
    else if (t == "ditamap")
        pageType_ = DitaMapPage;
}

/*! Converts the boolean value \a b to an enum representation
  of the boolean type, which includes an enum value for the
  \e {default value} of the item, i.e. true, false, or default.
 */
Node::FlagValue Node::toFlagValue(bool b)
{
    return b ? FlagValueTrue : FlagValueFalse;
}

/*!
  Converts the enum \a fv back to a boolean value.
  If \a fv is neither the true enum value nor the
  false enum value, the boolean value returned is
  \a defaultValue.

  Note that runtimeDesignabilityFunction() should be called
  first. If that function returns the name of a function, it
  means the function must be called at runtime to determine
  whether the property is Designable.
 */
bool Node::fromFlagValue(FlagValue fv, bool defaultValue)
{
    switch (fv) {
    case FlagValueTrue:
        return true;
    case FlagValueFalse:
        return false;
    default:
        return defaultValue;
    }
}

/*!
  Sets the pointer to the node that this node relates to.
 */
void Node::setRelates(InnerNode *pseudoParent)
{
    if (relatesTo_) {
        relatesTo_->removeRelated(this);
    }
    relatesTo_ = pseudoParent;
    pseudoParent->related_.append(this);
}

/*!
  This function creates a pair that describes a link.
  The pair is composed from \a link and \a desc. The
  \a linkType is the map index the pair is filed under.
 */
void Node::setLink(LinkType linkType, const QString &link, const QString &desc)
{
    QPair<QString,QString> linkPair;
    linkPair.first = link;
    linkPair.second = desc;
    linkMap_[linkType] = linkPair;
}

/*!
    Sets the information about the project and version a node was introduced
    in. The string is simplified, removing excess whitespace before being
    stored.
*/
void Node::setSince(const QString &since)
{
    since_ = since.simplified();
}

/*!
  Returns a string representing the access specifier.
 */
QString Node::accessString() const
{
    switch (access_) {
    case Protected:
        return "protected";
    case Private:
        return "private";
    case Public:
    default:
        break;
    }
    return "public";
}

/*!
  Extract a class name from the type \a string and return it.
 */
QString Node::extractClassName(const QString &string) const
{
    QString result;
    for (int i=0; i<=string.size(); ++i) {
        QChar ch;
        if (i != string.size())
            ch = string.at(i);

        QChar lower = ch.toLower();
        if ((lower >= QLatin1Char('a') && lower <= QLatin1Char('z')) ||
                ch.digitValue() >= 0 ||
                ch == QLatin1Char('_') ||
                ch == QLatin1Char(':')) {
            result += ch;
        }
        else if (!result.isEmpty()) {
            if (result != QLatin1String("const"))
                return result;
            result.clear();
        }
    }
    return result;
}

/*!
  Returns a string representing the access specifier.
 */
QString RelatedClass::accessString() const
{
    switch (access) {
    case Node::Protected:
        return "protected";
    case Node::Private:
        return "private";
    case Node::Public:
    default:
        break;
    }
    return "public";
}

/*!
  Returns the inheritance status.
 */
Node::Status Node::inheritedStatus() const
{
    Status parentStatus = Commendable;
    if (parent_)
        parentStatus = parent_->inheritedStatus();
    return (Status)qMin((int)status_, (int)parentStatus);
}

/*!
  Returns the thread safeness value for whatever this node
  represents. But if this node has a parent and the thread
  safeness value of the parent is the same as the thread
  safeness value of this node, what is returned is the
  value \c{UnspecifiedSafeness}. Why?
 */
Node::ThreadSafeness Node::threadSafeness() const
{
    if (parent_ && safeness_ == parent_->inheritedThreadSafeness())
        return UnspecifiedSafeness;
    return safeness_;
}

/*!
  If this node has a parent, the parent's thread safeness
  value is returned. Otherwise, this node's thread safeness
  value is returned. Why?
 */
Node::ThreadSafeness Node::inheritedThreadSafeness() const
{
    if (parent_ && safeness_ == UnspecifiedSafeness)
        return parent_->inheritedThreadSafeness();
    return safeness_;
}

#if 0
/*!
  Returns the sanitized file name without the path.
  If the file is an html file, the html suffix
  is removed. Why?
 */
QString Node::fileBase() const
{
    QString base = name();
    if (base.endsWith(".html"))
        base.chop(5);
    base.replace(QRegExp("[^A-Za-z0-9]+"), " ");
    base = base.trimmed();
    base.replace(QLatin1Char(' '), QLatin1Char('-'));
    return base.toLower();
}
#endif
/*!
  Returns this node's Universally Unique IDentifier as a
  QString. Creates the UUID first, if it has not been created.
 */
QString Node::guid() const
{
    if (uuid_.isEmpty())
        uuid_ = idForNode();
    return uuid_;
}

/*!
  If this node is a QML class node, return a pointer to it.
  If it is a child of a QML class node, return a pointer to
  the QML class node. Otherwise, return 0;
 */
QmlClassNode* Node::qmlClassNode()
{
    if (isQmlNode()) {
        Node* n = this;
        while (n && n->subType() != Node::QmlClass)
            n = n->parent();
        if (n && n->subType() == Node::QmlClass)
            return static_cast<QmlClassNode*>(n);
    }
    return 0;
}

/*!
  If this node is a QML node, find its QML class node,
  and return a pointer to the C++ class node from the
  QML class node. That pointer will be null if the QML
  class node is a component. It will be non-null if
  the QML class node is a QML element.
 */
ClassNode* Node::declarativeCppNode()
{
    QmlClassNode* qcn = qmlClassNode();
    if (qcn)
        return qcn->classNode();
    return 0;
}

/*!
  Returns \c true if the node's status is Internal, or if its
  parent is a class with internal status.
 */
bool Node::isInternal() const
{
    if (status() == Internal)
        return true;
    if (parent() && parent()->status() == Internal)
        return true;
    if (relates() && relates()->status() == Internal)
        return true;
    return false;
}

/*!
  \class InnerNode
 */

/*!
  The inner node destructor deletes the children and removes
  this node from its related nodes.
 */
InnerNode::~InnerNode()
{
    deleteChildren();
    removeFromRelated();
}

/*!
  Returns \c true if this node's members coolection is not empty.
 */
bool InnerNode::hasMembers() const
{
    return !members_.isEmpty();
}

/*!
  Appends \a node to the members list, if and only if it
  isn't already in the members list.
 */
void InnerNode::addMember(Node* node)
{
    if (!members_.contains(node))
        members_.append(node);
}

/*!
  Returns \c true if this node's members collection contains at
  least one namespace node.
 */
bool InnerNode::hasNamespaces() const
{
    if (!members_.isEmpty()) {
        NodeList::const_iterator i = members_.begin();
        while (i != members_.end()) {
            if ((*i)->isNamespace())
                return true;
            ++i;
        }
    }
    return false;
}

/*!
  Returns \c true if this node's members collection contains at
  least one class node.
 */
bool InnerNode::hasClasses() const
{
    if (!members_.isEmpty()) {
        NodeList::const_iterator i = members_.begin();
        while (i != members_.end()) {
            if ((*i)->isClass())
                return true;
            ++i;
        }
    }
    return false;
}

/*!
  Loads \a out with all this node's member nodes that are namespace nodes.
 */
void InnerNode::getMemberNamespaces(NodeMap& out)
{
    out.clear();
    NodeList::const_iterator i = members_.begin();
    while (i != members_.end()) {
        if ((*i)->isNamespace())
            out.insert((*i)->name(),(*i));
        ++i;
    }
}

/*!
  Loads \a out with all this node's member nodes that are class nodes.
 */
void InnerNode::getMemberClasses(NodeMap& out)
{
    out.clear();
    NodeList::const_iterator i = members_.begin();
    while (i != members_.end()) {
        if ((*i)->isClass())
            out.insert((*i)->name(),(*i));
        ++i;
    }
}

/*!
  Find the node in this node's children that has the
  given \a name. If this node is a QML class node, be
  sure to also look in the children of its property
  group nodes. Return the matching node or 0.
 */
Node *InnerNode::findChildNodeByName(const QString& name)
{
    Node *node = childMap.value(name);
    if (node && node->type() != QmlPropertyGroup)
        return node;
    if ((type() == Document) && (subType() == QmlClass)) {
        for (int i=0; i<children_.size(); ++i) {
            Node* n = children_.at(i);
            if (n->type() == QmlPropertyGroup) {
                node = static_cast<InnerNode*>(n)->findChildNodeByName(name);
                if (node)
                    return node;
            }
        }
    }
    return primaryFunctionMap.value(name);
}
void InnerNode::findNodes(const QString& name, QList<Node*>& n)
{
    n.clear();
    Node* node = 0;
    QList<Node*> nodes = childMap.values(name);
    /*
      <sigh> If this node's child map contains no nodes named
      name, then if this node is a QML class, seach each of its
      property group nodes for a node named name. If a match is
      found, append it to the output list and return immediately.
     */
    if (nodes.isEmpty()) {
        if ((type() == Document) && (subType() == QmlClass)) {
            for (int i=0; i<children_.size(); ++i) {
                node = children_.at(i);
                if (node->type() == QmlPropertyGroup) {
                    node = static_cast<InnerNode*>(node)->findChildNodeByName(name);
                    if (node) {
                        n.append(node);
                        return;
                    }
                }
            }
        }
    }
    else {
        /*
          If the childMap does contain one or more nodes named
          name, traverse the list of matching nodes. Append each
          matching node that is not a property group node to the
          output list. Search each property group node for a node
          named name and append that node to the output list.
          This is overkill, I think, but should produce a useful
          list.
         */
        for (int i=0; i<nodes.size(); ++i) {
            node = nodes.at(i);
            if (node->type() != QmlPropertyGroup)
                n.append(node);
            else {
                node = static_cast<InnerNode*>(node)->findChildNodeByName(name);
                if (node)
                    n.append(node);
            }
        }
    }
    if (!n.isEmpty())
        return;
    node = primaryFunctionMap.value(name);
    if (node)
        n.append(node);
}

/*!
  Find the node in this node's children that has the given \a name. If
  this node is a QML class node, be sure to also look in the children
  of its property group nodes. Return the matching node or 0. This is
  not a recearsive search.

  If \a qml is true, only match a node for which node->isQmlNode()
  returns \c true. If \a qml is false, only match a node for which
  node->isQmlNode() returns \c false.
 */
Node* InnerNode::findChildNodeByName(const QString& name, bool qml)
{
    QList<Node*> nodes = childMap.values(name);
    if (!nodes.isEmpty()) {
        for (int i=0; i<nodes.size(); ++i) {
            Node* node = nodes.at(i);
            if (!qml) {
                if (!node->isQmlNode())
                    return node;
            }
            else if (node->isQmlNode() && (node->type() != QmlPropertyGroup))
                return node;
        }
    }
    if (qml && (type() == Document) && (subType() == QmlClass)) {
        for (int i=0; i<children_.size(); ++i) {
            Node* node = children_.at(i);
            if (node->type() == QmlPropertyGroup) {
                node = static_cast<InnerNode*>(node)->findChildNodeByName(name);
                if (node)
                    return node;
            }
        }
    }
    return primaryFunctionMap.value(name);
}

/*!
  This function is like findChildNodeByName(), but if a node
  with the specified \a name is found but it is not of the
  specified \a type, 0 is returned.

  This function is not recursive and therefore can't handle
  collisions. If it finds a collision node named \a name, it
  will return that node. But it might not find the collision
  node because it looks up \a name in the child map, not the
  list.
 */
Node* InnerNode::findChildNodeByNameAndType(const QString& name, Type type)
{
    if (type == Function)
        return primaryFunctionMap.value(name);
    else {
        QList<Node*> nodes = childMap.values(name);
        for (int i=0; i<nodes.size(); ++i) {
            Node* node = nodes.at(i);
            if (node->type() == type)
                return node;
        }
    }
    return 0;
}

/*!
  Find a function node that is a child of this nose, such
  that the function node has the specified \a name.
 */
FunctionNode *InnerNode::findFunctionNode(const QString& name)
{
    return static_cast<FunctionNode *>(primaryFunctionMap.value(name));
}

/*!
  Find the function node that is a child of this node, such
  that the function has the same name and signature as the
  \a clone node.
 */
FunctionNode *InnerNode::findFunctionNode(const FunctionNode *clone)
{
    QMap<QString,Node*>::ConstIterator c = primaryFunctionMap.constFind(clone->name());
    if (c != primaryFunctionMap.constEnd()) {
        if (isSameSignature(clone, (FunctionNode *) *c)) {
            return (FunctionNode *) *c;
        }
        else if (secondaryFunctionMap.contains(clone->name())) {
            const NodeList& secs = secondaryFunctionMap[clone->name()];
            NodeList::ConstIterator s = secs.constBegin();
            while (s != secs.constEnd()) {
                if (isSameSignature(clone, (FunctionNode *) *s))
                    return (FunctionNode *) *s;
                ++s;
            }
        }
    }
    return 0;
}

/*!
  Returns the list of keys from the primary function map.
 */
QStringList InnerNode::primaryKeys()
{
    QStringList t;
    QMap<QString, Node*>::iterator i = primaryFunctionMap.begin();
    while (i != primaryFunctionMap.end()) {
        t.append(i.key());
        ++i;
    }
    return t;
}

/*!
  Returns the list of keys from the secondary function map.
 */
QStringList InnerNode::secondaryKeys()
{
    QStringList t;
    QMap<QString, NodeList>::iterator i = secondaryFunctionMap.begin();
    while (i != secondaryFunctionMap.end()) {
        t.append(i.key());
        ++i;
    }
    return t;
}

/*!
 */
void InnerNode::setOverload(const FunctionNode *func, bool overlode)
{
    Node *node = (Node *) func;
    Node *&primary = primaryFunctionMap[func->name()];

    if (secondaryFunctionMap.contains(func->name())) {
        NodeList& secs = secondaryFunctionMap[func->name()];
        if (overlode) {
            if (primary == node) {
                primary = secs.first();
                secs.erase(secs.begin());
                secs.append(node);
            }
            else {
                secs.removeAll(node);
                secs.append(node);
            }
        }
        else {
            if (primary != node) {
                secs.removeAll(node);
                secs.prepend(primary);
                primary = node;
            }
        }
    }
}

/*!
  Mark all child nodes that have no documentation as having
  private access and internal status. qdoc will then ignore
  them for documentation purposes.

  \note Exception: Name collision nodes are not marked
  private/internal.
 */
void InnerNode::makeUndocumentedChildrenInternal()
{
    foreach (Node *child, childNodes()) {
        if (child->doc().isEmpty()) {
            if (child->subType() != Node::Collision) {
                child->setAccess(Node::Private);
                child->setStatus(Node::Internal);
            }
        }
    }
}

/*!
  In each child node that is a collision node,
  clear the current child pointer.
 */
void InnerNode::clearCurrentChildPointers()
{
    foreach (Node* child, childNodes()) {
        if (child->subType() == Collision) {
            child->clearCurrentChild();
        }
    }
}

/*!
 */
void InnerNode::normalizeOverloads()
{
    QMap<QString, Node *>::Iterator p1 = primaryFunctionMap.begin();
    while (p1 != primaryFunctionMap.end()) {
        FunctionNode *primaryFunc = (FunctionNode *) *p1;
        if (secondaryFunctionMap.contains(primaryFunc->name()) &&
                (primaryFunc->status() != Commendable ||
                 primaryFunc->access() == Private)) {

            NodeList& secs = secondaryFunctionMap[primaryFunc->name()];
            NodeList::ConstIterator s = secs.constBegin();
            while (s != secs.constEnd()) {
                FunctionNode *secondaryFunc = (FunctionNode *) *s;

                // Any non-obsolete, non-compatibility, non-private functions
                // (i.e, visible functions) are preferable to the primary
                // function.

                if (secondaryFunc->status() == Commendable &&
                        secondaryFunc->access() != Private) {

                    *p1 = secondaryFunc;
                    int index = secondaryFunctionMap[primaryFunc->name()].indexOf(secondaryFunc);
                    secondaryFunctionMap[primaryFunc->name()].replace(index, primaryFunc);
                    break;
                }
                ++s;
            }
        }
        ++p1;
    }

    QMap<QString, Node *>::ConstIterator p = primaryFunctionMap.constBegin();
    while (p != primaryFunctionMap.constEnd()) {
        FunctionNode *primaryFunc = (FunctionNode *) *p;
        if (primaryFunc->isOverload())
            primaryFunc->ove = false;
        if (secondaryFunctionMap.contains(primaryFunc->name())) {
            NodeList& secs = secondaryFunctionMap[primaryFunc->name()];
            NodeList::ConstIterator s = secs.constBegin();
            while (s != secs.constEnd()) {
                FunctionNode *secondaryFunc = (FunctionNode *) *s;
                if (!secondaryFunc->isOverload())
                    secondaryFunc->ove = true;
                ++s;
            }
        }
        ++p;
    }

    NodeList::ConstIterator c = childNodes().constBegin();
    while (c != childNodes().constEnd()) {
        if ((*c)->isInnerNode())
            ((InnerNode *) *c)->normalizeOverloads();
        ++c;
    }
}

/*!
 */
void InnerNode::removeFromRelated()
{
    while (!related_.isEmpty()) {
        Node *p = static_cast<Node *>(related_.takeFirst());

        if (p != 0 && p->relates() == this) p->clearRelated();
    }
}

/*!
  Deletes all this node's children.
 */
void InnerNode::deleteChildren()
{
    NodeList childrenCopy = children_; // `children_` will be changed in ~Node()
    qDeleteAll(childrenCopy);
}

/*! \fn bool InnerNode::isInnerNode() const
  Returns \c true because this is an inner node.
 */

/*!
  Returns \c true if the node is a class node or a QML type node
  that is marked as being a wrapper class or QML type, or if
  it is a member of a wrapper class or type.
 */
bool Node::isWrapper() const
{
    return (parent_ ? parent_->isWrapper() : false);
}

/*!
 */
const Node *InnerNode::findChildNodeByName(const QString& name) const
{
    InnerNode *that = (InnerNode *) this;
    return that->findChildNodeByName(name);
}

/*!
  If \a qml is true, only match a node for which node->isQmlNode()
  returns \c true. If \a qml is false, only match a node for which
  node->isQmlNode() returns \c false.
 */
const Node* InnerNode::findChildNodeByName(const QString& name, bool qml) const
{
    InnerNode*that = (InnerNode*) this;
    return that->findChildNodeByName(name, qml);
}

/*!
  Searches this node's children for a child named \a name
  with the specified node \a type.
 */
const Node* InnerNode::findChildNodeByNameAndType(const QString& name, Type type) const
{
    InnerNode *that = (InnerNode *) this;
    return that->findChildNodeByNameAndType(name, type);
}

/*!
  Find a function node that is a child of this nose, such
  that the function node has the specified \a name. This
  function calls the non-const version of itself.
 */
const FunctionNode *InnerNode::findFunctionNode(const QString& name) const
{
    InnerNode *that = (InnerNode *) this;
    return that->findFunctionNode(name);
}

/*!
  Find the function node that is a child of this node, such
  that the function has the same name and signature as the
  \a clone node. This function calls the non-const version.
 */
const FunctionNode *InnerNode::findFunctionNode(const FunctionNode *clone) const
{
    InnerNode *that = (InnerNode *) this;
    return that->findFunctionNode(clone);
}

/*!
 */
const EnumNode *InnerNode::findEnumNodeForValue(const QString &enumValue) const
{
    foreach (const Node *node, enumChildren_) {
        const EnumNode *enume = static_cast<const EnumNode *>(node);
        if (enume->hasItem(enumValue))
            return enume;
    }
    return 0;
}

/*!
  Returnds the sequence number of the function node \a func
  in the list of overloaded functions for a class, such that
  all the functions have the same name as the \a func.
 */
int InnerNode::overloadNumber(const FunctionNode *func) const
{
    Node *node = (Node *) func;
    if (primaryFunctionMap[func->name()] == node) {
        return 1;
    }
    else {
        return secondaryFunctionMap[func->name()].indexOf(node) + 2;
    }
}

/*!
  Returns a node list containing all the member functions of
  some class such that the functions overload the name \a funcName.
 */
NodeList InnerNode::overloads(const QString &funcName) const
{
    NodeList result;
    Node *primary = primaryFunctionMap.value(funcName);
    if (primary) {
        result << primary;
        result += secondaryFunctionMap[funcName];
    }
    return result;
}

/*!
  Construct an inner node (i.e., not a leaf node) of the
  given \a type and having the given \a parent and \a name.
 */
InnerNode::InnerNode(Type type, InnerNode *parent, const QString& name)
    : Node(type, parent, name)
{
    switch (type) {
    case Class:
    case Namespace:
        setPageType(ApiPage);
        break;
    default:
        break;
    }
}

/*!
  Appends an \a include file to the list of include files.
 */
void InnerNode::addInclude(const QString& include)
{
    includes_.append(include);
}

/*!
  Sets the list of include files to \a includes.
 */
void InnerNode::setIncludes(const QStringList& includes)
{
    includes_ = includes;
}

/*!
  f1 is always the clone
 */
bool InnerNode::isSameSignature(const FunctionNode *f1, const FunctionNode *f2)
{
    if (f1->parameters().count() != f2->parameters().count())
        return false;
    if (f1->isConst() != f2->isConst())
        return false;

    QList<Parameter>::ConstIterator p1 = f1->parameters().constBegin();
    QList<Parameter>::ConstIterator p2 = f2->parameters().constBegin();
    while (p2 != f2->parameters().constEnd()) {
        if ((*p1).hasType() && (*p2).hasType()) {
            if ((*p1).rightType() != (*p2).rightType())
                return false;

            QString t1 = p1->leftType();
            QString t2 = p2->leftType();

            if (t1.length() < t2.length())
                qSwap(t1, t2);

            /*
              ### hack for C++ to handle superfluous
              "Foo::" prefixes gracefully
            */
            if (t1 != t2 && t1 != (f2->parent()->name() + "::" + t2))
                return false;
        }
        ++p1;
        ++p2;
    }
    return true;
}

/*!
  Adds the \a child to this node's child list.
 */
void InnerNode::addChild(Node *child)
{
    children_.append(child);
    if ((child->type() == Function) || (child->type() == QmlMethod)) {
        FunctionNode *func = (FunctionNode *) child;
        if (!primaryFunctionMap.contains(func->name())) {
            primaryFunctionMap.insert(func->name(), func);
        }
        else {
            NodeList &secs = secondaryFunctionMap[func->name()];
            secs.append(func);
        }
    }
    else {
        if (child->type() == Enum)
            enumChildren_.append(child);
        childMap.insertMulti(child->name(), child);
    }
}

/*!
 */
void InnerNode::removeChild(Node *child)
{
    children_.removeAll(child);
    enumChildren_.removeAll(child);
    if (child->type() == Function) {
        QMap<QString, Node *>::Iterator prim =
                primaryFunctionMap.find(child->name());
        NodeList& secs = secondaryFunctionMap[child->name()];
        if (prim != primaryFunctionMap.end() && *prim == child) {
            if (secs.isEmpty()) {
                primaryFunctionMap.remove(child->name());
            }
            else {
                primaryFunctionMap.insert(child->name(), secs.takeFirst());
            }
        }
        else {
            secs.removeAll(child);
        }
    }
    QMap<QString, Node *>::Iterator ent = childMap.find(child->name());
    while (ent != childMap.end() && ent.key() == child->name()) {
        if (*ent == child) {
            childMap.erase(ent);
            break;
        }
        ++ent;
    }
}

/*!
  Find the module (Qt Core, Qt GUI, etc.) to which the class belongs.
  We do this by obtaining the full path to the header file's location
  and examine everything between "src/" and the filename.  This is
  semi-dirty because we are assuming a particular directory structure.

  This function is only really useful if the class's module has not
  been defined in the header file with a QT_MODULE macro or with an
  \inmodule command in the documentation.
*/
QString Node::moduleName() const
{
    if (!moduleName_.isEmpty())
        return moduleName_;

    QString path = location().filePath();
    QString pattern = QString("src") + QDir::separator();
    int start = path.lastIndexOf(pattern);

    if (start == -1)
        return QString();

    QString moduleDir = path.mid(start + pattern.size());
    int finish = moduleDir.indexOf(QDir::separator());

    if (finish == -1)
        return QString();

    QString moduleName = moduleDir.left(finish);

    if (moduleName == "corelib")
        return "QtCore";
    else if (moduleName == "uitools")
        return "QtUiTools";
    else if (moduleName == "gui")
        return "QtGui";
    else if (moduleName == "network")
        return "QtNetwork";
    else if (moduleName == "opengl")
        return "QtOpenGL";
    else if (moduleName == "svg")
        return "QtSvg";
    else if (moduleName == "sql")
        return "QtSql";
    else if (moduleName == "qtestlib")
        return "QtTest";
    else if (moduleDir.contains("webkit"))
        return "QtWebKit";
    else if (moduleName == "xml")
        return "QtXml";
    else
        return QString();
}

/*!
 */
void InnerNode::removeRelated(Node *pseudoChild)
{
    related_.removeAll(pseudoChild);
}

/*!
  If this node has a child that is a QML property named \a n,
  return the pointer to that child.
 */
QmlPropertyNode* InnerNode::hasQmlProperty(const QString& n) const
{
    foreach (Node* child, childNodes()) {
        if (child->type() == Node::QmlProperty) {
            if (child->name() == n)
                return static_cast<QmlPropertyNode*>(child);
        }
        else if (child->type() == Node::QmlPropertyGroup) {
            QmlPropertyNode* t = child->hasQmlProperty(n);
            if (t)
                return t;
        }
    }
    return 0;
}

/*!
  \class LeafNode
 */

/*! \fn bool LeafNode::isInnerNode() const
  Returns \c false because this is a LeafNode.
 */

/*!
  Constructs a leaf node named \a name of the specified
  \a type. The new leaf node becomes a child of \a parent.
 */
LeafNode::LeafNode(Type type, InnerNode *parent, const QString& name)
    : Node(type, parent, name)
{
    switch (type) {
    case Enum:
    case Function:
    case Typedef:
    case Variable:
    case QmlProperty:
    case QmlSignal:
    case QmlSignalHandler:
    case QmlMethod:
        setPageType(ApiPage);
        break;
    default:
        break;
    }
}

/*!
  This constructor should only be used when this node's parent
  is meant to be \a parent, but this node is not to be listed
  as a child of \a parent. It is currently only used for the
  documentation case where a \e{qmlproperty} command is used
  to override the QML definition of a QML property.
 */
LeafNode::LeafNode(InnerNode* parent, Type type, const QString& name)
    : Node(type, 0, name)
{
    setParent(parent);
    switch (type) {
    case Enum:
    case Function:
    case Typedef:
    case Variable:
    case QmlProperty:
    case QmlSignal:
    case QmlSignalHandler:
    case QmlMethod:
        setPageType(ApiPage);
        break;
    default:
        break;
    }
}


/*!
  \class NamespaceNode
 */

/*!
  Constructs a namespace node.
 */
NamespaceNode::NamespaceNode(InnerNode *parent, const QString& name)
    : InnerNode(Namespace, parent, name)
{
    setPageType(ApiPage);
}

/*!
  \class ClassNode
  \brief This class represents a C++ class.
 */

/*!
  Constructs a class node. A class node will generate an API page.
 */
ClassNode::ClassNode(InnerNode *parent, const QString& name)
    : InnerNode(Class, parent, name)
{
    abstract_ = false;
    wrapper_ = false;
    qmlelement = 0;
    setPageType(ApiPage);
}

/*!
 */
void ClassNode::addBaseClass(Access access,
                             ClassNode *node,
                             const QString &dataTypeWithTemplateArgs)
{
    bases.append(RelatedClass(access, node, dataTypeWithTemplateArgs));
    node->derived.append(RelatedClass(access, this));
}

/*!
 */
void ClassNode::fixBaseClasses()
{
    int i;
    i = 0;
    QSet<ClassNode *> found;

    // Remove private and duplicate base classes.
    while (i < bases.size()) {
        ClassNode* bc = bases.at(i).node;
        if (bc->access() == Node::Private || found.contains(bc)) {
            RelatedClass rc = bases.at(i);
            bases.removeAt(i);
            ignoredBases.append(rc);
            const QList<RelatedClass> &bb = bc->baseClasses();
            for (int j = bb.size() - 1; j >= 0; --j)
                bases.insert(i, bb.at(j));
        }
        else {
            ++i;
        }
        found.insert(bc);
    }

    i = 0;
    while (i < derived.size()) {
        ClassNode* dc = derived.at(i).node;
        if (dc->access() == Node::Private) {
            derived.removeAt(i);
            const QList<RelatedClass> &dd = dc->derivedClasses();
            for (int j = dd.size() - 1; j >= 0; --j)
                derived.insert(i, dd.at(j));
        }
        else {
            ++i;
        }
    }
}

/*!
  Search the child list to find the property node with the
  specified \a name.
 */
PropertyNode* ClassNode::findPropertyNode(const QString& name)
{
    Node* n = findChildNodeByNameAndType(name, Node::Property);

    if (n)
        return static_cast<PropertyNode*>(n);

    PropertyNode* pn = 0;

    const QList<RelatedClass> &bases = baseClasses();
    if (!bases.isEmpty()) {
        for (int i = 0; i < bases.size(); ++i) {
            ClassNode* cn = bases[i].node;
            pn = cn->findPropertyNode(name);
            if (pn)
                break;
        }
    }
    const QList<RelatedClass>& ignoredBases = ignoredBaseClasses();
    if (!ignoredBases.isEmpty()) {
        for (int i = 0; i < ignoredBases.size(); ++i) {
            ClassNode* cn = ignoredBases[i].node;
            pn = cn->findPropertyNode(name);
            if (pn)
                break;
        }
    }

    return pn;
}

/*!
  This function does a recursive search of this class node's
  base classes looking for one that has a QML element. If it
  finds one, it returns the pointer to that QML element. If
  it doesn't find one, it returns null.
 */
QmlClassNode* ClassNode::findQmlBaseNode()
{
    QmlClassNode* result = 0;
    const QList<RelatedClass>& bases = baseClasses();

    if (!bases.isEmpty()) {
        for (int i = 0; i < bases.size(); ++i) {
            ClassNode* cn = bases[i].node;
            if (cn && cn->qmlElement()) {
                return cn->qmlElement();
            }
        }
        for (int i = 0; i < bases.size(); ++i) {
            ClassNode* cn = bases[i].node;
            if (cn) {
                result = cn->findQmlBaseNode();
                if (result != 0) {
                    return result;
                }
            }
        }
    }
    return result;
}

/*!
  \class DocNode
 */

/*!
  The type of a DocNode is Document, and it has a \a subtype,
  which specifies the type of DocNode. The page type for
  the page index is set here.
 */
DocNode::DocNode(InnerNode* parent, const QString& name, SubType subtype, Node::PageType ptype)
    : InnerNode(Document, parent, name), nodeSubtype_(subtype)
{
    switch (subtype) {
    case Page:
        setPageType(ptype);
        break;
    case DitaMap:
        setPageType(ptype);
        break;
    case Module:
    case Group:
        setPageType(OverviewPage);
        break;
    case QmlModule:
        setPageType(OverviewPage);
        break;
    case QmlClass:
    case QmlBasicType:
        setPageType(ApiPage);
        break;
    case Example:
        setPageType(ExamplePage);
        break;
    case Collision:
        setPageType(ptype);
        break;
    default:
        break;
    }
}

/*!
  Returns the document node's title. This is used for the page title.
*/
QString DocNode::title() const
{
    return title_;
}

/*!
  Returns the document node's full title, which is usually
  just title(), but for some SubType values is different
  from title()
 */
QString DocNode::fullTitle() const
{
    if (nodeSubtype_ == File) {
        if (title().isEmpty())
            return name().mid(name().lastIndexOf('/') + 1) + " Example File";
        else
            return title();
    }
    else if (nodeSubtype_ == Image) {
        if (title().isEmpty())
            return name().mid(name().lastIndexOf('/') + 1) + " Image File";
        else
            return title();
    }
    else if (nodeSubtype_ == HeaderFile) {
        if (title().isEmpty())
            return name();
        else
            return name() + " - " + title();
    }
    else if (nodeSubtype_ == Collision) {
        return title();
    }
    else {
        return title();
    }
}

/*!
  Returns the subtitle.
 */
QString DocNode::subTitle() const
{
    if (!subtitle_.isEmpty())
        return subtitle_;

    if ((nodeSubtype_ == File) || (nodeSubtype_ == Image)) {
        if (title().isEmpty() && name().contains(QLatin1Char('/')))
            return name();
    }
    return QString();
}

/*!
  The constructor calls the DocNode constructor with
  \a parent, \a name, and Node::Example.
 */
ExampleNode::ExampleNode(InnerNode* parent, const QString& name)
    : DocNode(parent, name, Node::Example, Node::ExamplePage)
{
    // nothing
}

/*!
  \class EnumNode
 */

/*!
  The constructor for the node representing an enum type
  has a \a parent class and an enum type \a name.
 */
EnumNode::EnumNode(InnerNode *parent, const QString& name)
    : LeafNode(Enum, parent, name), ft(0)
{
    // nothing.
}

/*!
  Add \a item to the enum type's item list.
 */
void EnumNode::addItem(const EnumItem& item)
{
    itms.append(item);
    names.insert(item.name());
}

/*!
  Returns the access level of the enumeration item named \a name.
  Apparently it is private if it has been omitted by qdoc's
  omitvalue command. Otherwise it is public.
 */
Node::Access EnumNode::itemAccess(const QString &name) const
{
    if (doc().omitEnumItemNames().contains(name))
        return Private;
    return Public;
}

/*!
  Returns the enum value associated with the enum \a name.
 */
QString EnumNode::itemValue(const QString &name) const
{
    foreach (const EnumItem &item, itms) {
        if (item.name() == name)
            return item.value();
    }
    return QString();
}

/*!
  \class TypedefNode
 */

/*!
 */
TypedefNode::TypedefNode(InnerNode *parent, const QString& name)
    : LeafNode(Typedef, parent, name), ae(0)
{
}

/*!
 */
void TypedefNode::setAssociatedEnum(const EnumNode *enume)
{
    ae = enume;
}

/*!
  \class Parameter
  \brief The class Parameter contains one parameter.

  A parameter can be a function parameter or a macro
  parameter.
 */

/*!
  Constructs this parameter from the left and right types
  \a leftType and rightType, the parameter \a name, and the
  \a defaultValue. In practice, \a rightType is not used,
  and I don't know what is was meant for.
 */
Parameter::Parameter(const QString& leftType,
                     const QString& rightType,
                     const QString& name,
                     const QString& defaultValue)
    : lef(leftType), rig(rightType), nam(name), def(defaultValue)
{
}

/*!
  The standard copy constructor copies the strings from \a p.
 */
Parameter::Parameter(const Parameter& p)
    : lef(p.lef), rig(p.rig), nam(p.nam), def(p.def)
{
}

/*!
  Assigning Parameter \a p to this Parameter copies the
  strings across.
 */
Parameter& Parameter::operator=(const Parameter& p)
{
    lef = p.lef;
    rig = p.rig;
    nam = p.nam;
    def = p.def;
    return *this;
}

/*!
  Reconstructs the text describing the parameter and
  returns it. If \a value is true, the default value
  will be included, if there is one.
 */
QString Parameter::reconstruct(bool value) const
{
    QString p = lef + rig;
    if (!p.endsWith(QChar('*')) && !p.endsWith(QChar('&')) && !p.endsWith(QChar(' ')))
        p += QLatin1Char(' ');
    p += nam;
    if (value && !def.isEmpty())
        p += " = " + def;
    return p;
}


/*!
  \class FunctionNode
 */

/*!
  Construct a function node for a C++ function. It's parent
  is \a parent, and it's name is \a name.
 */
FunctionNode::FunctionNode(InnerNode *parent, const QString& name)
    : LeafNode(Function, parent, name),
      met(Plain),
      vir(NonVirtual),
      con(false),
      sta(false),
      ove(false),
      reimp(false),
      attached_(false),
      rf(0),
      ap(0)
{
    // nothing.
}

/*!
  Construct a function node for a QML method or signal, specified
  by \a type. It's parent is \a parent, and it's name is \a name.
  If \a attached is true, it is an attached method or signal.
 */
FunctionNode::FunctionNode(Type type, InnerNode *parent, const QString& name, bool attached)
    : LeafNode(type, parent, name),
      met(Plain),
      vir(NonVirtual),
      con(false),
      sta(false),
      ove(false),
      reimp(false),
      attached_(attached),
      rf(0),
      ap(0)
{
    if (type == QmlMethod || type == QmlSignal) {
        if (name.startsWith("__"))
            setStatus(Internal);
    }
}

/*!
  Sets the \a virtualness of this function. If the \a virtualness
  is PureVirtual, and if the parent() is a ClassNode, set the parent's
  \e abstract flag to true.
 */
void FunctionNode::setVirtualness(Virtualness virtualness)
{
    vir = virtualness;
    if ((virtualness == PureVirtual) && parent() &&
            (parent()->type() == Node::Class))
        parent()->setAbstract(true);
}

/*!
 */
void FunctionNode::setOverload(bool overlode)
{
    parent()->setOverload(this, overlode);
    ove = overlode;
}

/*!
  Sets the function node's reimplementation flag to \a r.
  When \a r is true, it is supposed to mean that this function
  is a reimplementation of a virtual function in a base class,
  but it really just means the \e reimp command was seen in the
  qdoc comment.
 */
void FunctionNode::setReimp(bool r)
{
    reimp = r;
}

/*!
 */
void FunctionNode::addParameter(const Parameter& parameter)
{
    params.append(parameter);
}

/*!
 */
void FunctionNode::borrowParameterNames(const FunctionNode *source)
{
    QList<Parameter>::Iterator t = params.begin();
    QList<Parameter>::ConstIterator s = source->params.constBegin();
    while (s != source->params.constEnd() && t != params.end()) {
        if (!(*s).name().isEmpty())
            (*t).setName((*s).name());
        ++s;
        ++t;
    }
}

/*!
  If this function is a reimplementation, \a from points
  to the FunctionNode of the function being reimplemented.
 */
void FunctionNode::setReimplementedFrom(FunctionNode *from)
{
    rf = from;
    from->rb.append(this);
}

/*!
  Sets the "associated" property to \a property. The function
  might be the setter or getter for a property, for example.
 */
void FunctionNode::setAssociatedProperty(PropertyNode *property)
{
    ap = property;
}

/*!
  Returns the overload number for this function obtained
  from the parent.
 */
int FunctionNode::overloadNumber() const
{
    return parent()->overloadNumber(this);
}

/*!
  Returns the list of parameter names.
 */
QStringList FunctionNode::parameterNames() const
{
    QStringList names;
    QList<Parameter>::ConstIterator p = parameters().constBegin();
    while (p != parameters().constEnd()) {
        names << (*p).name();
        ++p;
    }
    return names;
}

/*!
  Returns a raw list of parameters. If \a names is true, the
  names are included. If \a values is true, the default values
  are included, if any are present.
 */
QString FunctionNode::rawParameters(bool names, bool values) const
{
    QString raw;
    foreach (const Parameter &parameter, parameters()) {
        raw += parameter.leftType() + parameter.rightType();
        if (names)
            raw += parameter.name();
        if (values)
            raw += parameter.defaultValue();
    }
    return raw;
}

/*!
  Returns the list of reconstructed parameters. If \a values
  is true, the default values are included, if any are present.
 */
QStringList FunctionNode::reconstructParams(bool values) const
{
    QStringList params;
    QList<Parameter>::ConstIterator p = parameters().constBegin();
    while (p != parameters().constEnd()) {
        params << (*p).reconstruct(values);
        ++p;
    }
    return params;
}

/*!
  Reconstructs and returns the function's signature. If \a values
  is true, the default values of the parameters are included, if
  present.
 */
QString FunctionNode::signature(bool values) const
{
    QString s;
    if (!returnType().isEmpty())
        s = returnType() + QLatin1Char(' ');
    s += name() + QLatin1Char('(');
    QStringList params = reconstructParams(values);
    int p = params.size();
    if (p > 0) {
        for (int i=0; i<p; i++) {
            s += params[i];
            if (i < (p-1))
                s += ", ";
        }
    }
    s += QLatin1Char(')');
    return s;
}

/*!
  Print some debugging stuff.
 */
void FunctionNode::debug() const
{
    qDebug("QML METHOD %s rt %s pp %s",
           qPrintable(name()), qPrintable(rt), qPrintable(pp.join(' ')));
}

/*!
  \class PropertyNode

  This class describes one instance of using the Q_PROPERTY macro.
 */

/*!
  The constructor sets the \a parent and the \a name, but
  everything else is set to default values.
 */
PropertyNode::PropertyNode(InnerNode *parent, const QString& name)
    : LeafNode(Property, parent, name),
      stored_(FlagValueDefault),
      designable_(FlagValueDefault),
      scriptable_(FlagValueDefault),
      writable_(FlagValueDefault),
      user_(FlagValueDefault),
      cst(false),
      fnl(false),
      rev(-1),
      overrides(0)
{
    // nothing.
}

/*!
  Sets this property's \e {overridden from} property to
  \a baseProperty, which indicates that this property
  overrides \a baseProperty. To begin with, all the values
  in this property are set to the corresponding values in
  \a baseProperty.

  We probably should ensure that the constant and final
  attributes are not being overridden improperly.
 */
void PropertyNode::setOverriddenFrom(const PropertyNode* baseProperty)
{
    for (int i = 0; i < NumFunctionRoles; ++i) {
        if (funcs[i].isEmpty())
            funcs[i] = baseProperty->funcs[i];
    }
    if (stored_ == FlagValueDefault)
        stored_ = baseProperty->stored_;
    if (designable_ == FlagValueDefault)
        designable_ = baseProperty->designable_;
    if (scriptable_ == FlagValueDefault)
        scriptable_ = baseProperty->scriptable_;
    if (writable_ == FlagValueDefault)
        writable_ = baseProperty->writable_;
    if (user_ == FlagValueDefault)
        user_ = baseProperty->user_;
    overrides = baseProperty;
}

/*!
 */
QString PropertyNode::qualifiedDataType() const
{
    if (setters().isEmpty() && resetters().isEmpty()) {
        if (type_.contains(QLatin1Char('*')) || type_.contains(QLatin1Char('&'))) {
            // 'QWidget *' becomes 'QWidget *' const
            return type_ + " const";
        }
        else {
            /*
              'int' becomes 'const int' ('int const' is
              correct C++, but looks wrong)
            */
            return "const " + type_;
        }
    }
    else {
        return type_;
    }
}

bool QmlClassNode::qmlOnly = false;
QMultiMap<QString,Node*> QmlClassNode::inheritedBy;

/*!
  Constructs a Qml class node (i.e. a Document node with the
  subtype QmlClass. The new node has the given \a parent
  and \a name.
 */
QmlClassNode::QmlClassNode(InnerNode *parent, const QString& name)
    : DocNode(parent, name, QmlClass, Node::ApiPage),
      abstract_(false),
      cnodeRequired_(false),
      wrapper_(false),
      cnode_(0),
      qmlModule_(0),
      baseNode_(0)
{
    int i = 0;
    if (name.startsWith("QML:")) {
        qDebug() << "BOGUS QML qualifier:" << name;
        i = 4;
    }
    setTitle(name.mid(i));
}

/*!
  Needed for printing a debug messages.
 */
QmlClassNode::~QmlClassNode()
{
    // nothing.
}

/*!
  Clear the static maps so that subsequent runs don't try to use
  contents from a previous run.
 */
void QmlClassNode::terminate()
{
    inheritedBy.clear();
}

/*!
  Record the fact that QML class \a base is inherited by
  QML class \a sub.
 */
void QmlClassNode::addInheritedBy(const QString& base, Node* sub)
{
    if (inheritedBy.constFind(base,sub) == inheritedBy.constEnd()) {
        inheritedBy.insert(base,sub);
    }
}

/*!
  Loads the list \a subs with the nodes of all the subclasses of \a base.
 */
void QmlClassNode::subclasses(const QString& base, NodeList& subs)
{
    subs.clear();
    if (inheritedBy.count(base) > 0) {
        subs = inheritedBy.values(base);
    }
}

/*!
  This function splits \a arg on the blank character to get a
  QML module name and version number. If the version number is
  present, it spilts the version number on the '.' character to
  get a major version number and a minor vrsion number. If the
  version number is present, both the major and minor version
  numbers should be there, but the minor version number is not
  absolutely necessary.
 */
void QmlModuleNode::setQmlModuleInfo(const QString& arg)
{
    QStringList blankSplit = arg.split(QLatin1Char(' '));
    qmlModuleName_ = blankSplit[0];
    if (blankSplit.size() > 1) {
        QStringList dotSplit = blankSplit[1].split(QLatin1Char('.'));
        qmlModuleVersionMajor_ = dotSplit[0];
        if (dotSplit.size() > 1)
            qmlModuleVersionMinor_ = dotSplit[1];
        else
            qmlModuleVersionMinor_ = "0";
    }
}

/*!
  If this QML type node has a base type node,
  return the fully qualified name of that QML
  type, i.e. <QML-module-name>::<QML-type-name>.
 */
QString QmlClassNode::qmlFullBaseName() const
{
    QString result;
    if (baseNode_) {
        result = baseNode_->qmlModuleName() + "::" + baseNode_->name();
    }
    return result;
}

/*!
  The name of this QML class node might be the same as the
  name of some other QML class node. If so, then this node's
  parent will be a NameCollisionNode.This function sets the
  NameCollisionNode's current child to this node. This is
  important when outputting the documentation for this node,
  when, for example, the documentation contains a link to
  the page being output. We don't want to generate a link
  to the disambiguation page if we can avoid it, and to be
  able to avoid it, the NameCollisionNode must maintain the
  current child pointer. That's the purpose of this function.
 */
void QmlClassNode::setCurrentChild()
{
    if (parent()) {
        InnerNode* n = parent();
        if (n->subType() == Node::Collision)
            n->setCurrentChild(this);
    }
}

/*!
 */
void QmlClassNode::clearCurrentChild()
{
    if (parent()) {
        InnerNode* n = parent();
        if (n->subType() == Node::Collision)
            n->clearCurrentChild();
    }
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module name from the QML module node. Otherwise, return the
  empty string.
 */
QString QmlClassNode::qmlModuleName() const
{
    return (qmlModule_ ? qmlModule_->qmlModuleName() : QString());
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module version from the QML module node. Otherwise, return
  the empty string.
 */
QString QmlClassNode::qmlModuleVersion() const
{
    return (qmlModule_ ? qmlModule_->qmlModuleVersion() : QString());
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module identifier from the QML module node. Otherwise, return
  the empty string.
 */
QString QmlClassNode::qmlModuleIdentifier() const
{
    return (qmlModule_ ? qmlModule_->qmlModuleIdentifier() : QString());
}

/*!
  Constructs a Qml basic type node (i.e. a Document node with
  the subtype QmlBasicType. The new node has the given
  \a parent and \a name.
 */
QmlBasicTypeNode::QmlBasicTypeNode(InnerNode *parent,
                                   const QString& name)
    : DocNode(parent, name, QmlBasicType, Node::ApiPage)
{
    setTitle(name);
}

/*!
  Constructor for the Qml property group node. \a parent is
  always a QmlClassNode.
 */
QmlPropertyGroupNode::QmlPropertyGroupNode(QmlClassNode* parent, const QString& name)
    : InnerNode(QmlPropertyGroup, parent, name)
{
    idNumber_ = -1;
}

/*!
  Return the property group node's id number for use in
  constructing an id attribute for the property group.
  If the id number is currently -1, increment the global
  property group count and set the id number to the new
  value.
 */
QString QmlPropertyGroupNode::idNumber()
{
    if (idNumber_ == -1)
        idNumber_ = incPropertyGroupCount();
    return QString().setNum(idNumber_);
}

/*!
  Constructor for the QML property node.
 */
QmlPropertyNode::QmlPropertyNode(InnerNode* parent,
                                 const QString& name,
                                 const QString& type,
                                 bool attached)
    : LeafNode(QmlProperty, parent, name),
      type_(type),
      stored_(FlagValueDefault),
      designable_(FlagValueDefault),
      isAlias_(false),
      isdefault_(false),
      attached_(attached),
      readOnly_(FlagValueDefault)
{
    setPageType(ApiPage);
    if (type_ == QString("alias"))
        isAlias_ = true;
    if (name.startsWith("__"))
        setStatus(Internal);
}

/*!
  Returns \c true if a QML property or attached property is
  not read-only. The algorithm for figuring this out is long
  amd tedious and almost certainly will break. It currently
  doesn't work for the qmlproperty:

  \code
      bool PropertyChanges::explicit,
  \endcode

  ...because the tokenizer gets confused on \e{explicit}.
 */
bool QmlPropertyNode::isWritable(QDocDatabase* qdb)
{
    if (readOnly_ != FlagValueDefault)
        return !fromFlagValue(readOnly_, false);

    QmlClassNode* qcn = qmlClassNode();
    if (qcn) {
        if (qcn->cppClassRequired()) {
            if (qcn->classNode()) {
                PropertyNode* pn = correspondingProperty(qdb);
                if (pn)
                    return pn->isWritable();
                else
                    location().warning(tr("No Q_PROPERTY for QML property %1::%2::%3 "
                                          "in C++ class documented as QML type: "
                                          "(property not found in the C++ class or its base classes)")
                                       .arg(qmlModuleName()).arg(qmlTypeName()).arg(name()));
            }
            else
                location().warning(tr("No Q_PROPERTY for QML property %1::%2::%3 "
                                      "in C++ class documented as QML type: "
                                      "(C++ class not specified or not found).")
                                   .arg(qmlModuleName()).arg(qmlTypeName()).arg(name()));
        }
    }
    return true;
}

/*!
  Returns a pointer this QML property's corresponding C++
  property, if it has one.
 */
PropertyNode* QmlPropertyNode::correspondingProperty(QDocDatabase* qdb)
{
    PropertyNode* pn;

    Node* n = parent();
    while (n && n->subType() != Node::QmlClass)
        n = n->parent();
    if (n) {
        QmlClassNode* qcn = static_cast<QmlClassNode*>(n);
        ClassNode* cn = qcn->classNode();
        if (cn) {
            QStringList dotSplit = name().split(QChar('.'));
            pn = cn->findPropertyNode(dotSplit[0]);
            if (pn) {
                if (dotSplit.size() > 1) {
                    // Find the C++ property corresponding to the QML property in
                    // the property group, <group>.<property>.

                    QStringList path(extractClassName(pn->qualifiedDataType()));
                    Node* nn = qdb->findClassNode(path);
                    if (nn) {
                        ClassNode* cn = static_cast<ClassNode*>(nn);
                        PropertyNode *pn2 = cn->findPropertyNode(dotSplit[1]);
                        if (pn2)
                            return pn2; // Return the property for the QML property.
                        else
                            return pn;  // Return the property for the QML group.
                    }
                }
                else
                    return pn;
            }
            else {
                pn = cn->findPropertyNode(dotSplit[0]);
                if (pn)
                    return pn;
            }
        }
    }

    return 0;
}

/*! \class NameCollisionNode

  An instance of this node is inserted in the tree
  whenever qdoc discovers that two nodes have the
  same name.
 */

/*!
  Constructs a name collision node containing \a child
  as its first child. The parent of \a child becomes
  this node's parent.
 */
NameCollisionNode::NameCollisionNode(InnerNode* child)
    : DocNode(child->parent(), child->name(), Collision, Node::NoPageType)
{
    setTitle("Name Collision: " + child->name());
    addCollision(child);
    current = 0;
}

/*!
  Add a collision to this collision node. \a child has
  the same name as the other children in this collision
  node. \a child becomes the current child.
 */
void NameCollisionNode::addCollision(InnerNode* child)
{
    if (child) {
        if (child->parent())
            child->parent()->removeChild(child);
        child->setParent((InnerNode*)this);
        children_.append(child);
    }
}

/*!
  The destructor does nothing.
 */
NameCollisionNode::~NameCollisionNode()
{
    // nothing.
}

/*! \fn const InnerNode* NameCollisionNode::currentChild() const
  Returns a pointer to the current child, which may be 0.
 */

/*! \fn void NameCollisionNode::setCurrentChild(InnerNode* child)
  Sets the current child to \a child. The current child is
  valid only within the file where it is defined.
 */

/*! \fn void NameCollisionNode::clearCurrentChild()
  Sets the current child to 0. This should be called at the
  end of each file, because the current child is only valid
  within the file where the child is defined.
 */

/*!
  Returns \c true if this collision node's current node is a QML node.
 */
bool NameCollisionNode::isQmlNode() const
{
    if (current)
        return current->isQmlNode();
    return false;
}

/*!
  Find any of this collision node's children that has type \a t
  and subtype \a st and return a pointer to it.
*/
InnerNode* NameCollisionNode::findAny(Node::Type t, Node::SubType st)
{
    if (current) {
        if (current->type() == t && current->subType() == st)
            return current;
    }
    const NodeList& cn = childNodes();
    NodeList::ConstIterator i = cn.constBegin();
    while (i != cn.constEnd()) {
        if ((*i)->type() == t && (*i)->subType() == st)
            return static_cast<InnerNode*>(*i);
        ++i;
    }
    return 0;
}

/*!
  This node is a name collision node. Find a child of this node
  such that the child's QML module name matches origin's QML module
  Name. Return the matching node, or return this node if there is
  no matching node.
 */
const Node* NameCollisionNode::applyModuleName(const Node* origin) const
{
    if (origin && !origin->qmlModuleName().isEmpty()) {
        const NodeList& cn = childNodes();
        NodeList::ConstIterator i = cn.constBegin();
        while (i != cn.constEnd()) {
            if ((*i)->type() == Node::Document && (*i)->subType() == Node::QmlClass) {
                if (origin->qmlModuleName() == (*i)->qmlModuleName())
                    return (*i);
            }
            ++i;
        }
    }
    return this;
}

/*!
  Construct the full document name for this node and return it.
 */
QString Node::fullDocumentName() const
{
    QStringList pieces;
    const Node* n = this;

    do {
        if (!n->name().isEmpty() && n->type() != Node::QmlPropertyGroup)
            pieces.insert(0, n->name());

        if (n->type() == Node::Document) {
            if ((n->subType() == Node::QmlClass) && !n->qmlModuleName().isEmpty())
                pieces.insert(0, n->qmlModuleName());
            break;
        }

        // Examine the parent node if one exists.
        if (n->parent())
            n = n->parent();
        else
            break;
    } while (true);

    // Create a name based on the type of the ancestor node.
    QString concatenator = "::";
    if ((n->type() == Node::Document) && (n->subType() != Node::QmlClass))
        concatenator = QLatin1Char('#');

    return pieces.join(concatenator);
}

/*!
  Returns the \a str as an NCName, which means the name can
  be used as the value of an \e id attribute. Search for NCName
  on the internet for details of what can be an NCName.
 */
QString Node::cleanId(QString str)
{
    QString clean;
    QString name = str.simplified();

    if (name.isEmpty())
        return clean;

    name = name.replace("::","-");
    name = name.replace(QLatin1Char(' '), QLatin1Char('-'));
    name = name.replace("()","-call");

    clean.reserve(name.size() + 20);
    if (!str.startsWith("id-"))
        clean = "id-";
    const QChar c = name[0];
    const uint u = c.unicode();

    if ((u >= 'a' && u <= 'z') ||
            (u >= 'A' && u <= 'Z') ||
            (u >= '0' && u <= '9')) {
        clean += c;
    }
    else if (u == '~') {
        clean += "dtor.";
    }
    else if (u == '_') {
        clean += "underscore.";
    }
    else {
        clean += QLatin1Char('a');
    }

    for (int i = 1; i < (int) name.length(); i++) {
        const QChar c = name[i];
        const uint u = c.unicode();
        if ((u >= 'a' && u <= 'z') ||
                (u >= 'A' && u <= 'Z') ||
                (u >= '0' && u <= '9') || u == '-' ||
                u == '_' || u == '.') {
            clean += c;
        }
        else if (c.isSpace() || u == ':' ) {
            clean += QLatin1Char('-');
        }
        else if (u == '!') {
            clean += "-not";
        }
        else if (u == '&') {
            clean += "-and";
        }
        else if (u == '<') {
            clean += "-lt";
        }
        else if (u == '=') {
            clean += "-eq";
        }
        else if (u == '>') {
            clean += "-gt";
        }
        else if (u == '#') {
            clean += "-hash";
        }
        else if (u == '(') {
            clean += QLatin1Char('-');
        }
        else if (u == ')') {
            clean += QLatin1Char('-');
        }
        else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

/*!
  Creates a string that can be used as a UUID for the node,
  depending on the type and subtype of the node. Uniquenss
  is not guaranteed, but it is expected that strings created
  here will be unique within an XML document. Hence, the
  returned string can be used as the value of an \e id
  attribute.
 */
QString Node::idForNode() const
{
    const FunctionNode* func;
    const TypedefNode* tdn;
    QString str;

    switch (type()) {
    case Node::Namespace:
        str = "namespace-" + fullDocumentName();
        break;
    case Node::Class:
        str = "class-" + fullDocumentName();
        break;
    case Node::Enum:
        str = "enum-" + name();
        break;
    case Node::Typedef:
        tdn = static_cast<const TypedefNode*>(this);
        if (tdn->associatedEnum()) {
            return tdn->associatedEnum()->idForNode();
        }
        else {
            str = "typedef-" + name();
        }
        break;
    case Node::Function:
        func = static_cast<const FunctionNode*>(this);
        if (func->associatedProperty()) {
            return func->associatedProperty()->idForNode();
        }
        else {
            if (func->name().startsWith("operator")) {
                str.clear();
                /*
                  The test below should probably apply to all
                  functions, but for now, overloaded operators
                  are the only ones that produce duplicate id
                  attributes in the DITA XML files.
                 */
                if (relatesTo_)
                    str = "nonmember-";
                QString op = func->name().mid(8);
                if (!op.isEmpty()) {
                    int i = 0;
                    while (i<op.size() && op.at(i) == ' ')
                        ++i;
                    if (i>0 && i<op.size()) {
                        op = op.mid(i);
                    }
                    if (!op.isEmpty()) {
                        i = 0;
                        while (i < op.size()) {
                            const QChar c = op.at(i);
                            const uint u = c.unicode();
                            if ((u >= 'a' && u <= 'z') ||
                                    (u >= 'A' && u <= 'Z') ||
                                    (u >= '0' && u <= '9'))
                                break;
                            ++i;
                        }
                        str += "operator-";
                        if (i>0) {
                            QString tail = op.mid(i);
                            op = op.left(i);
                            if (operators_.contains(op)) {
                                str += operators_.value(op);
                                if (!tail.isEmpty())
                                    str += QLatin1Char('-') + tail;
                            }
                            else
                                qDebug() << "qdoc internal error: Operator missing from operators_ map:" << op;
                        }
                        else {
                            str += op;
                        }
                    }
                }
            }
            else if (parent_) {
                if (parent_->type() == Class)
                    str = "class-member-" + func->name();
                else if (parent_->type() == Namespace)
                    str = "namespace-member-" + func->name();
                else if (parent_->type() == Document) {
                    if (parent_->subType() == QmlClass)
                        str = "qml-method-" + parent_->name().toLower() + "-" + func->name();
                    else
                        qDebug() << "qdoc internal error: Node subtype not handled:"
                                 << parent_->subType() << func->name();
                }
                else
                    qDebug() << "qdoc internal error: Node type not handled:"
                             << parent_->type() << func->name();

            }
            if (func->overloadNumber() != 1)
                str += QLatin1Char('-') + QString::number(func->overloadNumber());
        }
        break;
    case Node::Document:
        {
            switch (subType()) {
            case Node::QmlClass:
                str = "qml-class-" + name();
                break;
            case Node::Page:
            case Node::Group:
            case Node::Module:
            case Node::HeaderFile:
                str = title();
                if (str.isEmpty()) {
                    str = name();
                    if (str.endsWith(".html"))
                        str.remove(str.size()-5,5);
                }
                str.replace(QLatin1Char('/'), QLatin1Char('-'));
                break;
            case Node::File:
                str = name();
                str.replace(QLatin1Char('/'), QLatin1Char('-'));
                break;
            case Node::Example:
                str = name();
                str.replace(QLatin1Char('/'), QLatin1Char('-'));
                break;
            case Node::QmlBasicType:
                str = "qml-basic-type-" + name();
                break;
            case Node::QmlModule:
                str = "qml-module-" + name();
                break;
            case Node::Collision:
                str = title();
                str.replace(": ","-");
                break;
            default:
                qDebug() << "ERROR: A case was not handled in Node::idForNode():"
                         << "subType():" << subType() << "type():" << type();
                break;
            }
        }
        break;
    case Node::QmlProperty:
        str = "qml-property-" + name();
        break;
    case Node::QmlPropertyGroup:
        {
            Node* n = const_cast<Node*>(this);
            str = "qml-propertygroup-" + n->name();
        }
        break;
    case Node::Property:
        str = "property-" + name();
        break;
    case Node::QmlSignal:
        str = "qml-signal-" + name();
        break;
    case Node::QmlSignalHandler:
        str = "qml-signal-handler-" + name();
        break;
    case Node::QmlMethod:
        func = static_cast<const FunctionNode*>(this);
        str = "qml-method-" + parent_->name().toLower() + "-" + func->name();
        if (func->overloadNumber() != 1)
            str += QLatin1Char('-') + QString::number(func->overloadNumber());
        break;
    case Node::Variable:
        str = "var-" + name();
        break;
    default:
        qDebug() << "ERROR: A case was not handled in Node::idForNode():"
                 << "type():" << type() << "subType():" << subType();
        break;
    }
    if (str.isEmpty()) {
        qDebug() << "ERROR: A link text was empty in Node::idForNode():"
                 << "type():" << type() << "subType():" << subType()
                 << "name():" << name()
                 << "title():" << title();
    }
    else {
        str = cleanId(str);
    }
    return str;
}

/*!
  Prints the inner node's list of children.
  For debugging only.
 */
void InnerNode::printChildren(const QString& title)
{
    qDebug() << title << name() << children_.size();
    if (children_.size() > 0) {
        for (int i=0; i<children_.size(); ++i) {
            Node* n = children_.at(i);
            qDebug() << "  CHILD:" << n->name() << n->nodeTypeString() << n->nodeSubtypeString();
        }
    }
}

/*!
  Prints the inner node's list of members.
  For debugging only.
 */
void InnerNode::printMembers(const QString& title)
{
    qDebug() << title << name() << members_.size();
    if (members_.size() > 0) {
        for (int i=0; i<members_.size(); ++i) {
            Node* n = members_.at(i);
            qDebug() << "  MEMBER:" << n->name() << n->nodeTypeString() << n->nodeSubtypeString();
        }
    }
}

QT_END_NAMESPACE

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

#include "qdom.h"
#include "qxmlstream.h"
#include "qdocindexfiles.h"
#include "qdoctagfiles.h"
#include "config.h"
#include "qdocdatabase.h"
#include "location.h"
#include "atom.h"
#include "generator.h"
#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
  \class QDocIndexFiles

  This class handles qdoc index files.
 */

QDocIndexFiles* QDocIndexFiles::qdocIndexFiles_ = NULL;

/*!
  Constructs the singleton QDocIndexFiles.
 */
QDocIndexFiles::QDocIndexFiles()
    : gen_( 0 )
{
    qdb_ = QDocDatabase::qdocDB();
}

/*!
  Destroys the singleton QDocIndexFiles.
 */
QDocIndexFiles::~QDocIndexFiles()
{
    qdb_ = 0;
}

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
 */
QDocIndexFiles* QDocIndexFiles::qdocIndexFiles()
{
   if (!qdocIndexFiles_)
      qdocIndexFiles_ = new QDocIndexFiles;
   return qdocIndexFiles_;
}

/*!
  Destroys the singleton.
 */
void QDocIndexFiles::destroyQDocIndexFiles()
{
    if (qdocIndexFiles_) {
        delete qdocIndexFiles_;
        qdocIndexFiles_ = 0;
    }
}

/*!
  Reads and parses the list of index files in \a indexFiles.
 */
void QDocIndexFiles::readIndexes(const QStringList& indexFiles)
{
    foreach (const QString& indexFile, indexFiles) {
        QString msg = "Loading index file: " + indexFile;
        Location::logToStdErr(msg);
        readIndexFile(indexFile);
    }
}

/*!
  Reads and parses the index file at \a path.
 */
void QDocIndexFiles::readIndexFile(const QString& path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        QDomDocument document;
        document.setContent(&file);
        file.close();

        QDomElement indexElement = document.documentElement();

        // Generate a relative URL between the install dir and the index file
        // when the -installdir command line option is set.
        QString indexUrl;
        if (Config::installDir.isEmpty()) {
            indexUrl = indexElement.attribute("url", QString());
        }
        else {
            // Use a fake directory, since we will copy the output to a sub directory of
            // installDir when using "make install". This is just for a proper relative path.
            //QDir installDir(path.section('/', 0, -3) + "/outputdir");
            QDir installDir(path.section('/', 0, -3) + '/' + Generator::outputSubdir());
            indexUrl = installDir.relativeFilePath(path).section('/', 0, -2);
        }
        project_ = indexElement.attribute("project", QString());

        basesList_.clear();
        relatedList_.clear();

        // Scan all elements in the XML file, constructing a map that contains
        // base classes for each class found.

        QDomElement child = indexElement.firstChildElement();
        while (!child.isNull()) {
            readIndexSection(child, qdb_->treeRoot(), indexUrl);
            child = child.nextSiblingElement();
        }

        // Now that all the base classes have been found for this index,
        // arrange them into an inheritance hierarchy.
        resolveIndex();
    }
}

/*!
  Read a <section> element from the index file and create the
  appropriate node(s).
 */
void QDocIndexFiles::readIndexSection(const QDomElement& element,
                                      InnerNode* parent,
                                      const QString& indexUrl)
{
    QString name = element.attribute("name");
    QString href = element.attribute("href");
    Node* node;
    Location location;

    QString filePath;
    int lineNo = 0;
    if (element.hasAttribute("filepath")) {
        filePath = element.attribute("filepath", QString());
        lineNo = element.attribute("lineno", QString()).toInt();
    }
    if (element.nodeName() == "namespace") {
        node = new NamespaceNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");

    }
    else if (element.nodeName() == "class") {
        node = new ClassNode(parent, name);
        basesList_.append(QPair<ClassNode*,QString>(static_cast<ClassNode*>(node),
                                                   element.attribute("bases")));

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");
        bool abstract = false;
        if (element.attribute("abstract") == "true")
            abstract = true;
        node->setAbstract(abstract);
    }
    else if ((element.nodeName() == "qmlclass") ||
             ((element.nodeName() == "page") && (element.attribute("subtype") == "qmlclass"))) {
        QmlClassNode* qcn = new QmlClassNode(parent, name);
        qcn->setTitle(element.attribute("title"));
        QString qmlModuleName = element.attribute("qml-module-name");
        if (!qmlModuleName.isEmpty())
            qdb_->addToQmlModule(qmlModuleName, qcn);
        bool abstract = false;
        if (element.attribute("abstract") == "true")
            abstract = true;
        qcn->setAbstract(abstract);
        QString qmlFullBaseName = element.attribute("qml-base-type");
        if (!qmlFullBaseName.isEmpty())
            qcn->setQmlBaseName(qmlFullBaseName);
        if (element.hasAttribute("location"))
            name = element.attribute("location", QString());
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qcn;
    }
    else if (element.nodeName() == "qmlbasictype") {
        QmlBasicTypeNode* qbtn = new QmlBasicTypeNode(parent, name);
        qbtn->setTitle(element.attribute("title"));
        if (element.hasAttribute("location"))
            name = element.attribute("location", QString());
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qbtn;
    }
    else if (element.nodeName() == "qmlpropertygroup") {
        QmlClassNode* qcn = static_cast<QmlClassNode*>(parent);
        QmlPropertyGroupNode* qpgn = new QmlPropertyGroupNode(qcn, name);
        if (element.hasAttribute("location"))
            name = element.attribute("location", QString());
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qpgn;
    }
    else if (element.nodeName() == "qmlproperty") {
        QString type = element.attribute("type");
        bool attached = false;
        if (element.attribute("attached") == "true")
            attached = true;
        bool readonly = false;
        if (element.attribute("writable") == "false")
            readonly = true;
        QmlPropertyNode* qpn = 0;
        if (parent->type() == Node::Document) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(parent);
            qpn = new QmlPropertyNode(qcn, name, type, attached);
        }
        else if (parent->type() == Node::QmlPropertyGroup) {
            QmlPropertyGroupNode* qpgn = static_cast<QmlPropertyGroupNode*>(parent);
            qpn = new QmlPropertyNode(qpgn, name, type, attached);
        }
        qpn->setReadOnly(readonly);
        node = qpn;
    }
    else if ((element.nodeName() == "qmlmethod") ||
             (element.nodeName() == "qmlsignal") ||
             (element.nodeName() == "qmlsignalhandler")) {
        Node::Type t = Node::QmlMethod;
        if (element.nodeName() == "qmlsignal")
            t = Node::QmlSignal;
        else if (element.nodeName() == "qmlsignalhandler")
            t = Node::QmlSignalHandler;
        bool attached = false;
        FunctionNode* fn = new FunctionNode(t, parent, name, attached);
        node = fn;
    }
    else if (element.nodeName() == "page") {
        Node::SubType subtype;
        Node::PageType ptype = Node::NoPageType;
        QString attr = element.attribute("subtype");
        if (attr == "example") {
            subtype = Node::Example;
            ptype = Node::ExamplePage;
        }
        else if (attr == "header") {
            subtype = Node::HeaderFile;
            ptype = Node::ApiPage;
        }
        else if (attr == "file") {
            subtype = Node::File;
            ptype = Node::NoPageType;
        }
        else if (attr == "group") {
            subtype = Node::Group;
            ptype = Node::OverviewPage;
        }
        else if (attr == "module") {
            subtype = Node::Module;
            ptype = Node::OverviewPage;
        }
        else if (attr == "qmlmodule") {
            subtype = Node::QmlModule;
            ptype = Node::OverviewPage;
        }
        else if (attr == "page") {
            subtype = Node::Page;
            ptype = Node::ArticlePage;
        }
        else if (attr == "externalpage") {
            subtype = Node::ExternalPage;
            ptype = Node::ArticlePage;
        }
        else if (attr == "qmlclass") {
            subtype = Node::QmlClass;
            ptype = Node::ApiPage;
        }
        else if (attr == "qmlbasictype") {
            subtype = Node::QmlBasicType;
            ptype = Node::ApiPage;
        }
        else
            return;

        DocNode* docNode = 0;
        if (subtype == Node::QmlModule) {
            QString t = element.attribute("qml-module-name") + " " +
                element.attribute("qml-module-version");
            docNode = qdb_->addQmlModule(t);
        }
        else
            docNode = new DocNode(parent, name, subtype, ptype);
        docNode->setTitle(element.attribute("title"));

        if (element.hasAttribute("location"))
            name = element.attribute("location", QString());

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);

        node = docNode;

    }
    else if (element.nodeName() == "enum") {
        EnumNode* enumNode = new EnumNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

        QDomElement child = element.firstChildElement("value");
        while (!child.isNull()) {
            EnumItem item(child.attribute("name"), child.attribute("value"));
            enumNode->addItem(item);
            child = child.nextSiblingElement("value");
        }

        node = enumNode;

    }
    else if (element.nodeName() == "typedef") {
        node = new TypedefNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (element.nodeName() == "property") {
        node = new PropertyNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (element.nodeName() == "function") {
        FunctionNode::Virtualness virt;
        if (element.attribute("virtual") == "non")
            virt = FunctionNode::NonVirtual;
        else if (element.attribute("virtual") == "impure")
            virt = FunctionNode::ImpureVirtual;
        else if (element.attribute("virtual") == "pure")
            virt = FunctionNode::PureVirtual;
        else
            return;

        FunctionNode::Metaness meta;
        if (element.attribute("meta") == "plain")
            meta = FunctionNode::Plain;
        else if (element.attribute("meta") == "signal")
            meta = FunctionNode::Signal;
        else if (element.attribute("meta") == "slot")
            meta = FunctionNode::Slot;
        else if (element.attribute("meta") == "constructor")
            meta = FunctionNode::Ctor;
        else if (element.attribute("meta") == "destructor")
            meta = FunctionNode::Dtor;
        else if (element.attribute("meta") == "macro")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithparams")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithoutparams")
            meta = FunctionNode::MacroWithoutParams;
        else
            return;

        FunctionNode* functionNode = new FunctionNode(parent, name);
        functionNode->setReturnType(element.attribute("return"));
        functionNode->setVirtualness(virt);
        functionNode->setMetaness(meta);
        functionNode->setConst(element.attribute("const") == "true");
        functionNode->setStatic(element.attribute("static") == "true");
        functionNode->setOverload(element.attribute("overload") == "true");

        if (element.hasAttribute("relates")
                && element.attribute("relates") != parent->name()) {
            relatedList_.append(
                        QPair<FunctionNode*,QString>(functionNode,
                                                     element.attribute("relates")));
        }

        QDomElement child = element.firstChildElement("parameter");
        while (!child.isNull()) {
            // Do not use the default value for the parameter; it is not
            // required, and has been known to cause problems.
            Parameter parameter(child.attribute("left"),
                                child.attribute("right"),
                                child.attribute("name"),
                                QString()); // child.attribute("default")
            functionNode->addParameter(parameter);
            child = child.nextSiblingElement("parameter");
        }

        node = functionNode;
        if (!indexUrl.isEmpty())
            location =  Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");
    }
    else if (element.nodeName() == "variable") {
        node = new VariableNode(parent, name);
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");
    }
    else if (element.nodeName() == "keyword") {
        qdb_->insertTarget(name, TargetRec::Keyword, parent, 1);
        return;
    }
    else if (element.nodeName() == "target") {
        qdb_->insertTarget(name, TargetRec::Target, parent, 2);
        return;
    }
    else if (element.nodeName() == "contents") {
        qdb_->insertTarget(name, TargetRec::Contents, parent, 3);
        return;
    }
    else
        return;

    QString access = element.attribute("access");
    if (access == "public")
        node->setAccess(Node::Public);
    else if (access == "protected")
        node->setAccess(Node::Protected);
    else if ((access == "private") || (access == "internal"))
        node->setAccess(Node::Private);
    else
        node->setAccess(Node::Public);

    if ((element.nodeName() != "page") &&
            (element.nodeName() != "qmlclass") &&
            (element.nodeName() != "qmlbasictype")) {
        QString threadSafety = element.attribute("threadsafety");
        if (threadSafety == "non-reentrant")
            node->setThreadSafeness(Node::NonReentrant);
        else if (threadSafety == "reentrant")
            node->setThreadSafeness(Node::Reentrant);
        else if (threadSafety == "thread safe")
            node->setThreadSafeness(Node::ThreadSafe);
        else
            node->setThreadSafeness(Node::UnspecifiedSafeness);
    }
    else
        node->setThreadSafeness(Node::UnspecifiedSafeness);

    QString status = element.attribute("status");
    if (status == "compat")
        node->setStatus(Node::Compat);
    else if (status == "obsolete")
        node->setStatus(Node::Obsolete);
    else if (status == "deprecated")
        node->setStatus(Node::Obsolete);
    else if (status == "preliminary")
        node->setStatus(Node::Preliminary);
    else if (status == "commendable")
        node->setStatus(Node::Commendable);
    else if (status == "internal")
        node->setStatus(Node::Internal);
    else if (status == "main")
        node->setStatus(Node::Main);
    else
        node->setStatus(Node::Commendable);

    QString moduleName = element.attribute("module");
    if (!moduleName.isEmpty())
        node->setModuleName(moduleName);
    if (!href.isEmpty()) {
        if (node->isExternalPage())
            node->setUrl(href);
        else if (!indexUrl.isEmpty())
            node->setUrl(indexUrl + QLatin1Char('/') + href);
    }

    QString since = element.attribute("since");
    if (!since.isEmpty()) {
        node->setSince(since);
    }

    QString groupsAttr = element.attribute("groups");
    if (!groupsAttr.isEmpty()) {
        QStringList groupNames = groupsAttr.split(",");
        for (int i=0; i<groupNames.size(); ++i) {
            DocNode* dn = qdb_->findGroup(groupNames[i]);
            if (dn) {
                dn->addMember(node);
            }
            else {
                qDebug() << "NODE:" << node->name() << "GROUPS:" << groupNames;
                qDebug() << "DID NOT FIND GROUP:" << dn->name() << "for:" << node->name();
            }
        }
    }

    // Create some content for the node.
    QSet<QString> emptySet;
    Location t(filePath);
    if (!filePath.isEmpty()) {
        t.setLineNo(lineNo);
        node->setLocation(t);
        location = t;
    }
    Doc doc(location, location, " ", emptySet, emptySet); // placeholder
    node->setDoc(doc);
    node->setIndexNodeFlag();
    node->setOutputSubdirectory(project_.toLower());
    QString briefAttr = element.attribute("brief");
    if (!briefAttr.isEmpty()) {
        node->setReconstitutedBrief(briefAttr);
    }

    if (node->isInnerNode()) {
        InnerNode* inner = static_cast<InnerNode*>(node);
        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            if (element.nodeName() == "class") {
                readIndexSection(child, inner, indexUrl);
            }
            else if (element.nodeName() == "qmlclass") {
                readIndexSection(child, inner, indexUrl);
            }
            else if (element.nodeName() == "page") {
                readIndexSection(child, inner, indexUrl);
            }
            else if (element.nodeName() == "namespace" && !name.isEmpty()) {
                // The root node in the index is a namespace with an empty name.
                readIndexSection(child, inner, indexUrl);
            }
            else {
                readIndexSection(child, parent, indexUrl);
            }
            child = child.nextSiblingElement();
        }
    }
}

/*!
 */
void QDocIndexFiles::resolveIndex()
{
    QPair<ClassNode*,QString> pair;
    foreach (pair, basesList_) {
        foreach (const QString& base, pair.second.split(QLatin1Char(','))) {
            Node* n = qdb_->treeRoot()->findChildNodeByNameAndType(base, Node::Class);
            if (n) {
                pair.first->addBaseClass(Node::Public, static_cast<ClassNode*>(n));
            }
        }
    }

    QPair<FunctionNode*,QString> relatedPair;
    foreach (relatedPair, relatedList_) {
        Node* n = qdb_->treeRoot()->findChildNodeByNameAndType(relatedPair.second, Node::Class);
        if (n)
            relatedPair.first->setRelates(static_cast<ClassNode*>(n));
    }
}

/*!
  Normally this is used for writing the \e groups attribute,
  but it can be used for writing any attribute with a list
  value that comes from some subset of the members of \a n.

  \note The members of \a n are \e not the children of \a n.

  The names we want to include are the names of the members
  of \a n that have node type \a t and node subtype \a st.
  The attribute name is \a attr. The names are joined with
  the space character and written with \a writer.
 */
void QDocIndexFiles::writeMembersAttribute(QXmlStreamWriter& writer,
                                           const InnerNode* n,
                                           Node::Type t,
                                           Node::SubType st,
                                           const QString& attr)
{
    const NodeList& members = n->members();
    if (!members.isEmpty()) {
        QStringList names;
        NodeList::ConstIterator i = members.constBegin();
        while (i != members.constEnd()) {
            if ((*i)->type() == t && (*i)->subType() == st)
                names.append((*i)->name());
            ++i;
        }
        if (!names.isEmpty())
            writer.writeAttribute(attr, names.join(","));
    }
}

/*!
  Generate the index section with the given \a writer for the \a node
  specified, returning true if an element was written; otherwise returns
  false.
 */
bool QDocIndexFiles::generateIndexSection(QXmlStreamWriter& writer,
                                          Node* node,
                                          bool generateInternalNodes)
{
    /*
      Don't include index nodes in a new index file. Or DITA map nodes.
     */
    if (node->isIndexNode() || node->subType() == Node::DitaMap)
        return false;

    QString nodeName;
    QString qmlModuleName;
    QString qmlModuleVersion;
    QString qmlFullBaseName;
    switch (node->type()) {
    case Node::Namespace:
        nodeName = "namespace";
        break;
    case Node::Class:
        nodeName = "class";
        break;
    case Node::Document:
        nodeName = "page";
        if (node->subType() == Node::QmlClass) {
            nodeName = "qmlclass";
            QmlModuleNode* qmn = node->qmlModule();
            if (qmn)
                qmlModuleName = qmn->qmlModuleName();
            qmlFullBaseName = node->qmlFullBaseName();
        }
        else if (node->subType() == Node::QmlBasicType)
            nodeName = "qmlbasictype";
        break;
    case Node::Enum:
        nodeName = "enum";
        break;
    case Node::Typedef:
        nodeName = "typedef";
        break;
    case Node::Property:
        nodeName = "property";
        break;
    case Node::Function:
        nodeName = "function";
        break;
    case Node::Variable:
        nodeName = "variable";
        break;
    case Node::QmlProperty:
        nodeName = "qmlproperty";
        break;
    case Node::QmlPropertyGroup:
        nodeName = "qmlpropertygroup";
        break;
    case Node::QmlSignal:
        nodeName = "qmlsignal";
        break;
    case Node::QmlSignalHandler:
        nodeName = "qmlsignalhandler";
        break;
    case Node::QmlMethod:
        nodeName = "qmlmethod";
        break;
    default:
        return false;
    }

    QString access;
    switch (node->access()) {
    case Node::Public:
        access = "public";
        break;
    case Node::Protected:
        access = "protected";
        break;
    case Node::Private:
#if 0
        // Do not include private non-internal nodes in the index.
        // (Internal public and protected nodes are marked as private
        // by qdoc. We can check their internal status to determine
        // whether they were really private to begin with.)
        if (node->status() == Node::Internal && generateInternalNodes)
            access = "internal";
        else
            return false;
#endif
        {
            access = "private";
            bool b = generateInternalNodes;
            if (b)
                b = false;
        }
        break;
    default:
        return false;
    }

    QString objName = node->name();
    // Special case: only the root node should have an empty name.
    if (objName.isEmpty() && node != qdb_->treeRoot())
        return false;

    writer.writeStartElement(nodeName);

    QXmlStreamAttributes attributes;

    if (node->type() != Node::Document) {
        QString threadSafety;
        switch (node->threadSafeness()) {
        case Node::NonReentrant:
            threadSafety = "non-reentrant";
            break;
        case Node::Reentrant:
            threadSafety = "reentrant";
            break;
        case Node::ThreadSafe:
            threadSafety = "thread safe";
            break;
        case Node::UnspecifiedSafeness:
        default:
            threadSafety = "unspecified";
            break;
        }
        writer.writeAttribute("threadsafety", threadSafety);
    }

    QString status;
    switch (node->status()) {
    case Node::Compat:
        status = "compat";
        break;
    case Node::Obsolete:
        status = "obsolete";
        break;
    case Node::Deprecated:
        status = "obsolete";
        break;
    case Node::Preliminary:
        status = "preliminary";
        break;
    case Node::Commendable:
        status = "commendable";
        break;
    case Node::Internal:
        status = "internal";
        break;
    case Node::Main:
    default:
        status = "main";
        break;
    }

    writer.writeAttribute("name", objName);
    if (node->isQmlModule()) {
        qmlModuleName = node->qmlModuleName();
        qmlModuleVersion = node->qmlModuleVersion();
    }
    if (!qmlModuleName.isEmpty()) {
        writer.writeAttribute("qml-module-name", qmlModuleName);
        if (node->isQmlModule())
            writer.writeAttribute("qml-module-version", qmlModuleVersion);
        if (!qmlFullBaseName.isEmpty())
            writer.writeAttribute("qml-base-type", qmlFullBaseName);
    }

    QString href;
    if (!node->isExternalPage()) {
        QString fullName = node->fullDocumentName();
        if (fullName != objName)
            writer.writeAttribute("fullname", fullName);
        if (Generator::useOutputSubdirs())
            href = node->outputSubdirectory();
        if (!href.isEmpty())
            href.append(QLatin1Char('/'));
        href.append(gen_->fullDocumentLocation(node));
    }
    else
        href = node->name();
    if (node->isQmlNode()) {
        InnerNode* p = node->parent();
        if (p) {
            if (p->isQmlPropertyGroup())
                p = p->parent();
            if (p && p->isQmlType() && p->isAbstract())
                href.clear();
        }
    }
    if (!href.isEmpty())
        writer.writeAttribute("href", href);

    writer.writeAttribute("access", access);
    writer.writeAttribute("status", status);
    if (node->isAbstract())
        writer.writeAttribute("abstract", "true");
    if (!node->location().fileName().isEmpty())
        writer.writeAttribute("location", node->location().fileName());
    if (!node->location().filePath().isEmpty()) {
        writer.writeAttribute("filepath", node->location().filePath());
        writer.writeAttribute("lineno", QString("%1").arg(node->location().lineNo()));
    }

    if (!node->since().isEmpty()) {
        writer.writeAttribute("since", node->since());
    }

    QString brief = node->doc().briefText().toString();
    switch (node->type()) {
    case Node::Class:
        {
            // Classes contain information about their base classes.
            const ClassNode* classNode = static_cast<const ClassNode*>(node);
            QList<RelatedClass> bases = classNode->baseClasses();
            QSet<QString> baseStrings;
            foreach (const RelatedClass& related, bases) {
                ClassNode* baseClassNode = related.node;
                baseStrings.insert(baseClassNode->name());
            }
            writer.writeAttribute("bases", QStringList(baseStrings.toList()).join(","));
            if (!node->moduleName().isEmpty())
                writer.writeAttribute("module", node->moduleName());
            writeMembersAttribute(writer, classNode, Node::Document, Node::Group, "groups");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Namespace:
        {
            const NamespaceNode* namespaceNode = static_cast<const NamespaceNode*>(node);
            if (!namespaceNode->moduleName().isEmpty())
                writer.writeAttribute("module", namespaceNode->moduleName());
            writeMembersAttribute(writer, namespaceNode, Node::Document, Node::Group, "groups");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Document:
        {
            /*
              Document nodes (such as manual pages) contain subtypes,
              titles and other attributes.
            */
            bool writeModuleName = false;
            const DocNode* docNode = static_cast<const DocNode*>(node);
            switch (docNode->subType()) {
            case Node::Example:
                writer.writeAttribute("subtype", "example");
                writeModuleName = true;
                break;
            case Node::HeaderFile:
                writer.writeAttribute("subtype", "header");
                writeModuleName = true;
                break;
            case Node::File:
                writer.writeAttribute("subtype", "file");
                break;
            case Node::Group:
                {
                    writer.writeAttribute("subtype", "group");
                    writer.writeAttribute("seen", docNode->wasSeen() ? "true" : "false");
                    // Groups contain information about their group members.
                    const NodeList& members = docNode->members();
                    QStringList names;
                    foreach (const Node* member, members) {
                        names.append(member->name());
                    }
                    writer.writeAttribute("members", names.join(","));
                    writeModuleName = true;
                }
                break;
            case Node::Module:
                writer.writeAttribute("subtype", "module");
                break;
            case Node::QmlModule:
                writer.writeAttribute("subtype", "qmlmodule");
                break;
            case Node::Page:
                writer.writeAttribute("subtype", "page");
                writeModuleName = true;
                break;
            case Node::ExternalPage:
                writer.writeAttribute("subtype", "externalpage");
                break;
            case Node::QmlClass:
                //writer.writeAttribute("subtype", "qmlclass");
                break;
            case Node::QmlBasicType:
                //writer.writeAttribute("subtype", "qmlbasictype");
                break;
            default:
                break;
            }
            writer.writeAttribute("title", docNode->title());
            writer.writeAttribute("fulltitle", docNode->fullTitle());
            writer.writeAttribute("subtitle", docNode->subTitle());
            if (!node->moduleName().isEmpty() && writeModuleName) {
                writer.writeAttribute("module", node->moduleName());
            }
            writeMembersAttribute(writer, docNode, Node::Document, Node::Group, "groups");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Function:
        {
            /*
              Function nodes contain information about the type of
              function being described.
            */
            const FunctionNode* functionNode = static_cast<const FunctionNode*>(node);
            switch (functionNode->virtualness()) {
            case FunctionNode::NonVirtual:
                writer.writeAttribute("virtual", "non");
                break;
            case FunctionNode::ImpureVirtual:
                writer.writeAttribute("virtual", "impure");
                break;
            case FunctionNode::PureVirtual:
                writer.writeAttribute("virtual", "pure");
                break;
            default:
                break;
            }

            switch (functionNode->metaness()) {
            case FunctionNode::Plain:
                writer.writeAttribute("meta", "plain");
                break;
            case FunctionNode::Signal:
                writer.writeAttribute("meta", "signal");
                break;
            case FunctionNode::Slot:
                writer.writeAttribute("meta", "slot");
                break;
            case FunctionNode::Ctor:
                writer.writeAttribute("meta", "constructor");
                break;
            case FunctionNode::Dtor:
                writer.writeAttribute("meta", "destructor");
                break;
            case FunctionNode::MacroWithParams:
                writer.writeAttribute("meta", "macrowithparams");
                break;
            case FunctionNode::MacroWithoutParams:
                writer.writeAttribute("meta", "macrowithoutparams");
                break;
            default:
                break;
            }
            writer.writeAttribute("const", functionNode->isConst()?"true":"false");
            writer.writeAttribute("static", functionNode->isStatic()?"true":"false");
            writer.writeAttribute("overload", functionNode->isOverload()?"true":"false");
            if (functionNode->isOverload())
                writer.writeAttribute("overload-number", QString::number(functionNode->overloadNumber()));
            if (functionNode->relates())
                writer.writeAttribute("relates", functionNode->relates()->name());
            const PropertyNode* propertyNode = functionNode->associatedProperty();
            if (propertyNode)
                writer.writeAttribute("associated-property", propertyNode->name());
            writer.writeAttribute("type", functionNode->returnType());
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlProperty:
        {
            QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
            writer.writeAttribute("type", qpn->dataType());
            writer.writeAttribute("attached", qpn->isAttached() ? "true" : "false");
            writer.writeAttribute("writable", qpn->isWritable(qdb_) ? "true" : "false");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlPropertyGroup:
        {
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Property:
        {
            const PropertyNode* propertyNode = static_cast<const PropertyNode*>(node);
            writer.writeAttribute("type", propertyNode->dataType());
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
            foreach (const Node* fnNode, propertyNode->getters()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("getter");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // getter
                }
            }
            foreach (const Node* fnNode, propertyNode->setters()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("setter");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // setter
                }
            }
            foreach (const Node* fnNode, propertyNode->resetters()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("resetter");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // resetter
                }
            }
            foreach (const Node* fnNode, propertyNode->notifiers()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("notifier");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // notifier
                }
            }
        }
        break;
    case Node::Variable:
        {
            const VariableNode* variableNode = static_cast<const VariableNode*>(node);
            writer.writeAttribute("type", variableNode->dataType());
            writer.writeAttribute("static", variableNode->isStatic() ? "true" : "false");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    default:
        break;
    }

    // Inner nodes and function nodes contain child nodes of some sort, either
    // actual child nodes or function parameters. For these, we close the
    // opening tag, create child elements, then add a closing tag for the
    // element. Elements for all other nodes are closed in the opening tag.

    if (node->isInnerNode()) {
        const InnerNode* inner = static_cast<const InnerNode*>(node);

        // For internal pages, we canonicalize the target, keyword and content
        // item names so that they can be used by qdoc for other sets of
        // documentation.
        // The reason we do this here is that we don't want to ruin
        // externally composed indexes, containing non-qdoc-style target names
        // when reading in indexes.

        if (inner->doc().hasTargets()) {
            bool external = false;
            if (inner->type() == Node::Document) {
                const DocNode* docNode = static_cast<const DocNode*>(inner);
                if (docNode->subType() == Node::ExternalPage)
                    external = true;
            }
            foreach (const Atom* target, inner->doc().targets()) {
                QString targetName = target->string();
                if (!external)
                    targetName = Doc::canonicalTitle(targetName);
                writer.writeStartElement("target");
                writer.writeAttribute("name", targetName);
                writer.writeEndElement(); // target
            }
        }
        if (inner->doc().hasKeywords()) {
            foreach (const Atom* keyword, inner->doc().keywords()) {
                writer.writeStartElement("keyword");
                writer.writeAttribute("name", Doc::canonicalTitle(keyword->string()));
                writer.writeEndElement(); // keyword
            }
        }
        if (inner->doc().hasTableOfContents()) {
            for (int i = 0; i < inner->doc().tableOfContents().size(); ++i) {
                Atom* item = inner->doc().tableOfContents()[i];
                int level = inner->doc().tableOfContentsLevels()[i];
                QString title = Text::sectionHeading(item).toString();
                writer.writeStartElement("contents");
                writer.writeAttribute("name", Doc::canonicalTitle(title));
                writer.writeAttribute("title", title);
                writer.writeAttribute("level", QString::number(level));
                writer.writeEndElement(); // contents
            }
        }
    }
    else if (node->type() == Node::Function) {
        const FunctionNode* functionNode = static_cast<const FunctionNode*>(node);
        // Write a signature attribute for convenience.
        QStringList signatureList;
        QStringList resolvedParameters;
        foreach (const Parameter& parameter, functionNode->parameters()) {
            QString leftType = parameter.leftType();
            const Node* leftNode = qdb_->findNode(parameter.leftType().split("::"),
                                                  0,
                                                  SearchBaseClasses|NonFunction);
            if (!leftNode || leftNode->type() != Node::Typedef) {
                leftNode = qdb_->findNode(parameter.leftType().split("::"),
                                          node->parent(),
                                          SearchBaseClasses|NonFunction);
            }
            if (leftNode && leftNode->type() == Node::Typedef) {
                if (leftNode->type() == Node::Typedef) {
                    const TypedefNode* typedefNode =  static_cast<const TypedefNode*>(leftNode);
                    if (typedefNode->associatedEnum()) {
                        leftType = "QFlags<" + typedefNode->associatedEnum()->fullDocumentName() +
                            QLatin1Char('>');
                    }
                }
                else
                    leftType = leftNode->fullDocumentName();
            }
            resolvedParameters.append(leftType);
            signatureList.append(leftType + QLatin1Char(' ') + parameter.name());
        }

        QString signature = functionNode->name() + QLatin1Char('(') + signatureList.join(", ") +
            QLatin1Char(')');
        if (functionNode->isConst())
            signature += " const";
        writer.writeAttribute("signature", signature);

        for (int i = 0; i < functionNode->parameters().size(); ++i) {
            Parameter parameter = functionNode->parameters()[i];
            writer.writeStartElement("parameter");
            writer.writeAttribute("left", resolvedParameters[i]);
            writer.writeAttribute("right", parameter.rightType());
            writer.writeAttribute("name", parameter.name());
            writer.writeAttribute("default", parameter.defaultValue());
            writer.writeEndElement(); // parameter
        }
    }
    else if (node->type() == Node::Enum) {
        const EnumNode* enumNode = static_cast<const EnumNode*>(node);
        if (enumNode->flagsType()) {
            writer.writeAttribute("typedef",enumNode->flagsType()->fullDocumentName());
        }
        foreach (const EnumItem& item, enumNode->items()) {
            writer.writeStartElement("value");
            writer.writeAttribute("name", item.name());
            writer.writeAttribute("value", item.value());
            writer.writeEndElement(); // value
        }
    }
    else if (node->type() == Node::Typedef) {
        const TypedefNode* typedefNode = static_cast<const TypedefNode*>(node);
        if (typedefNode->associatedEnum()) {
            writer.writeAttribute("enum",typedefNode->associatedEnum()->fullDocumentName());
        }
    }
    return true;
}

/*!
  Returns \c true if the node \a n1 is less than node \a n2. The
  comparison is performed by comparing properties of the nodes
  in order of increasing complexity.
*/
bool compareNodes(const Node* n1, const Node* n2)
{
    // Private nodes can occur in any order since they won't normally be
    // written to the index.
    if (n1->access() == Node::Private && n2->access() == Node::Private)
        return false;

    if (n1->location().filePath() < n2->location().filePath())
        return true;
    else if (n1->location().filePath() > n2->location().filePath())
        return false;

    if (n1->type() < n2->type())
        return true;
    else if (n1->type() > n2->type())
        return false;

    if (n1->name() < n2->name())
        return true;
    else if (n1->name() > n2->name())
        return false;

    if (n1->access() < n2->access())
        return true;
    else if (n1->access() > n2->access())
        return false;

    if (n1->type() == Node::Function && n2->type() == Node::Function) {
        const FunctionNode* f1 = static_cast<const FunctionNode*>(n1);
        const FunctionNode* f2 = static_cast<const FunctionNode*>(n2);

        if (f1->isConst() < f2->isConst())
            return true;
        else if (f1->isConst() > f2->isConst())
            return false;

        if (f1->signature() < f2->signature())
            return true;
        else if (f1->signature() > f2->signature())
            return false;
    }

    if (n1->type() == Node::Document && n2->type() == Node::Document) {
        const DocNode* f1 = static_cast<const DocNode*>(n1);
        const DocNode* f2 = static_cast<const DocNode*>(n2);
        if (f1->fullTitle() < f2->fullTitle())
            return true;
        else if (f1->fullTitle() > f2->fullTitle())
            return false;
    }

    return false;
}

/*!
  Generate index sections for the child nodes of the given \a node
  using the \a writer specified. If \a generateInternalNodes is true,
  nodes marked as internal will be included in the index; otherwise,
  they will be omitted.
*/
void QDocIndexFiles::generateIndexSections(QXmlStreamWriter& writer,
                                           Node* node,
                                           bool generateInternalNodes)
{
    /*
      Note that the groups are written after all the other nodes.
     */
    if (!node->isGroup() && generateIndexSection(writer, node, generateInternalNodes)) {
        if (node->isInnerNode()) {
            const InnerNode* inner = static_cast<const InnerNode*>(node);

            NodeList cnodes = inner->childNodes();
            std::sort(cnodes.begin(), cnodes.end(), compareNodes);

            foreach (Node* child, cnodes) {
                /*
                  Don't generate anything for a collision node. We want
                  children of collision nodes in the index, but leaving
                  out the parent collision page will make searching for
                  nodes easier.
                 */
                if (child->subType() == Node::Collision) {
                    const InnerNode* pgn = static_cast<const InnerNode*>(child);
                    foreach (Node* c, pgn->childNodes()) {
                        generateIndexSections(writer, c, generateInternalNodes);
                    }
                }
                else
                    generateIndexSections(writer, child, generateInternalNodes);
            }
        }
        writer.writeEndElement();
    }
}

/*!
  Outputs an index file.
 */
void QDocIndexFiles::generateIndex(const QString& fileName,
                                   const QString& url,
                                   const QString& title,
                                   Generator* g,
                                   bool generateInternalNodes)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;

    QString msg = "Writing index file: " + fileName;
    Location::logToStdErr(msg);

    gen_ = g;
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE QDOCINDEX>");

    writer.writeStartElement("INDEX");
    writer.writeAttribute("url", url);
    writer.writeAttribute("title", title);
    writer.writeAttribute("version", qdb_->version());
    writer.writeAttribute("project", g->config()->getString(CONFIG_PROJECT));

    generateIndexSections(writer, qdb_->treeRoot(), generateInternalNodes);

    /*
      We wait until the end of the index file to output the group elements.
      By waiting until the end, when we read each group element, its members
      will have already been created. It is then only necessary to create
      the group page and add each member to its member list.
     */
    const DocNodeMap& groups = qdb_->groups();
    if (!groups.isEmpty()) {
        DocNodeMap::ConstIterator g = groups.constBegin();
        while (g != groups.constEnd()) {
            if (generateIndexSection(writer, g.value(), generateInternalNodes))
                writer.writeEndElement();
            ++g;
        }
    }

    writer.writeEndElement(); // INDEX
    writer.writeEndElement(); // QDOCINDEX
    writer.writeEndDocument();
    file.close();
}

QT_END_NAMESPACE

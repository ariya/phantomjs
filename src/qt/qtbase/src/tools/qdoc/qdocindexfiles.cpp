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

static Node* top = 0;

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
    gen_ = 0;
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
        //qDebug() << msg;
        readIndexFile(indexFile);
    }
}

static bool readingRoot = true;

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

        readingRoot = true;
        NamespaceNode* root = qdb_->newIndexTree(project_);

        // Scan all elements in the XML file, constructing a map that contains
        // base classes for each class found.
        QDomElement child = indexElement.firstChildElement();
        while (!child.isNull()) {
            readIndexSection(child, root, indexUrl);
            child = child.nextSiblingElement();
            readingRoot = true;
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
                                      Node* current,
                                      const QString& indexUrl)
{
    QString name = element.attribute("name");
    QString href = element.attribute("href");
    Node* node;
    Location location;
    InnerNode* parent = 0;
    if (current->isInnerNode())
        parent = static_cast<InnerNode*>(current);

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
        if (element.hasAttribute("bases")) {
            QString bases = element.attribute("bases");
            if (!bases.isEmpty())
                basesList_.append(QPair<ClassNode*,QString>(static_cast<ClassNode*>(node), bases));
        }
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");
        bool abstract = false;
        if (element.attribute("abstract") == "true")
            abstract = true;
        node->setAbstract(abstract);
    }
    else if (element.nodeName() == "qmlclass") {
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
        if (!qmlFullBaseName.isEmpty()) {
            qcn->setQmlBaseName(qmlFullBaseName);
        }
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
        if (parent->isQmlType()) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(parent);
            qpn = new QmlPropertyNode(qcn, name, type, attached);
        }
        else if (parent->isQmlPropertyGroup()) {
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
    else if (element.nodeName() == "group") {
        GroupNode* gn = qdb_->addGroup(name);
        gn->setTitle(element.attribute("title"));
        gn->setSubTitle(element.attribute("subtitle"));
        if (element.attribute("seen") == "true")
            gn->markSeen();
        node = gn;
    }
    else if (element.nodeName() == "module") {
        ModuleNode* mn = qdb_->addModule(name);
        mn->setTitle(element.attribute("title"));
        mn->setSubTitle(element.attribute("subtitle"));
        if (element.attribute("seen") == "true")
            mn->markSeen();
        node = mn;
    }
    else if (element.nodeName() == "qmlmodule") {
        QString t = element.attribute("qml-module-name") + " " + element.attribute("qml-module-version");
        QmlModuleNode* qmn = qdb_->addQmlModule(t);
        qmn->setTitle(element.attribute("title"));
        qmn->setSubTitle(element.attribute("subtitle"));
        if (element.attribute("seen") == "true")
            qmn->markSeen();
        node = qmn;
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
        else if (attr == "page") {
            subtype = Node::Page;
            ptype = Node::ArticlePage;
        }
        else if (attr == "externalpage") {
            subtype = Node::ExternalPage;
            ptype = Node::ArticlePage;
        }
        else
            return;

        DocNode* docNode = new DocNode(parent, name, subtype, ptype);
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
        QString t = element.attribute("virtual");
        if (t == "non")
            virt = FunctionNode::NonVirtual;
        else if (t == "impure")
            virt = FunctionNode::ImpureVirtual;
        else if (t == "pure")
            virt = FunctionNode::PureVirtual;
        else
            return;

        t = element.attribute("meta");
        FunctionNode::Metaness meta;
        if (t == "plain")
            meta = FunctionNode::Plain;
        else if (t == "signal")
            meta = FunctionNode::Signal;
        else if (t == "slot")
            meta = FunctionNode::Slot;
        else if (t == "constructor")
            meta = FunctionNode::Ctor;
        else if (t == "destructor")
            meta = FunctionNode::Dtor;
        else if (t == "macro")
            meta = FunctionNode::MacroWithParams;
        else if (t == "macrowithparams")
            meta = FunctionNode::MacroWithParams;
        else if (t == "macrowithoutparams")
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
        /*
          Note: The "signature" attribute was written to the
          index file, but it is not read back in. Is that ok?
         */

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
        QString title = element.attribute("title");
        qdb_->insertTarget(name, title, TargetRec::Keyword, current, 1);
        return;
    }
    else if (element.nodeName() == "target") {
        QString title = element.attribute("title");
        qdb_->insertTarget(name, title, TargetRec::Target, current, 2);
        return;
    }
    else if (element.nodeName() == "contents") {
        QString title = element.attribute("title");
        qdb_->insertTarget(name, title, TargetRec::Contents, current, 3);
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
        qdb_->addToModule(moduleName, node);
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
        foreach (QString name, groupNames) {
            qdb_->addToGroup(name, node);
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

    // zzz
    bool useParent = (element.nodeName() == "namespace" && name.isEmpty());
    if (element.hasChildNodes()) {
        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            if (useParent)
                readIndexSection(child, parent, indexUrl);
            else
                readIndexSection(child, node, indexUrl);
            child = child.nextSiblingElement();
        }
    }
}

/*!
  This function tries to resolve class inheritance immediately
  after the index file is read. It is not always possible to
  resolve a class inheritance at this point, because the base
  class might be in an index file that hasn't been read yet, or
  it might be in one of the header files that will be read for
  the current module. These cases will be resolved after all
  the index files and header and source files have been read,
  just prior to beginning the generate phase for the current
  module.

  I don't think this is completely correct because it always
  sets the access to public.
 */
void QDocIndexFiles::resolveIndex()
{
    QPair<ClassNode*,QString> pair;
    foreach (pair, basesList_) {
        foreach (const QString& base, pair.second.split(QLatin1Char(','))) {
            QStringList basePath = base.split(QString("::"));
            Node* n = qdb_->findClassNode(basePath);
            if (n)
                pair.first->addResolvedBaseClass(Node::Public, static_cast<ClassNode*>(n));
            else
                pair.first->addUnresolvedBaseClass(Node::Public, basePath, QString());
        }
    }

    QPair<FunctionNode*,QString> relatedPair;
    foreach (relatedPair, relatedList_) {
        QStringList path = relatedPair.second.split("::");
        Node* n = qdb_->findRelatesNode(path);
        if (n)
            relatedPair.first->setRelates(static_cast<ClassNode*>(n));
    }

    // No longer needed.
    basesList_.clear();
    relatedList_.clear();
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
    case Node::QmlType:
        {
            nodeName = "qmlclass";
            QmlModuleNode* qmn = node->qmlModule();
            if (qmn)
                qmlModuleName = qmn->qmlModuleName();
            qmlFullBaseName = node->qmlFullBaseName();
        }
        break;
    case Node::QmlBasicType:
        nodeName = "qmlbasictype";
        break;
    case Node::Document:
        nodeName = "page";
        break;
    case Node::Group:
        nodeName = "group";
        break;
    case Node::Module:
        nodeName = "module";
        break;
    case Node::QmlModule:
        nodeName = "qmlmodule";
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
    if (objName.isEmpty() && node != qdb_->primaryTreeRoot())
        return false;

    writer.writeStartElement(nodeName);

    QXmlStreamAttributes attributes;

    if (!node->isDocNode() && !node->isGroup() && !node->isModule() && !node->isQmlModule()) {
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

    writer.writeAttribute("status", status);
    if (!node->isDocNode() && !node->isGroup() && !node->isModule() && !node->isQmlModule()) {
        writer.writeAttribute("access", access);
        if (node->isAbstract())
            writer.writeAttribute("abstract", "true");
    }
    if (!node->location().fileName().isEmpty())
        writer.writeAttribute("location", node->location().fileName());
    if (!node->location().filePath().isEmpty()) {
        writer.writeAttribute("filepath", node->location().filePath());
        writer.writeAttribute("lineno", QString("%1").arg(node->location().lineNo()));
    }

    if (!node->since().isEmpty()) {
        writer.writeAttribute("since", node->since());
    }

    QString brief = node->doc().trimmedBriefText(node->name()).toString();
    switch (node->type()) {
    case Node::Class:
        {
            // Classes contain information about their base classes.
            const ClassNode* classNode = static_cast<const ClassNode*>(node);
            QList<RelatedClass> bases = classNode->baseClasses();
            QSet<QString> baseStrings;
            foreach (const RelatedClass& related, bases) {
                ClassNode* n = related.node_;
                if (n)
                    baseStrings.insert(n->fullName());
            }
            if (!baseStrings.isEmpty())
                writer.writeAttribute("bases", QStringList(baseStrings.toList()).join(","));
            if (!node->moduleName().isEmpty())
                writer.writeAttribute("module", node->moduleName());
            if (!classNode->groupNames().isEmpty())
                writer.writeAttribute("groups", classNode->groupNames().join(","));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Namespace:
        {
            const NamespaceNode* namespaceNode = static_cast<const NamespaceNode*>(node);
            if (!namespaceNode->moduleName().isEmpty())
                writer.writeAttribute("module", namespaceNode->moduleName());
            if (!namespaceNode->groupNames().isEmpty())
                writer.writeAttribute("groups", namespaceNode->groupNames().join(","));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlType:
        {
            const QmlClassNode* qcn = static_cast<const QmlClassNode*>(node);
            writer.writeAttribute("title", qcn->title());
            writer.writeAttribute("fulltitle", qcn->fullTitle());
            writer.writeAttribute("subtitle", qcn->subTitle());
            if (!qcn->groupNames().isEmpty())
                writer.writeAttribute("groups", qcn->groupNames().join(","));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Document:
        {
            /*
              Document nodes (such as manual pages) have a subtype,
              a title, and other attributes.
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
            case Node::Page:
                writer.writeAttribute("subtype", "page");
                writeModuleName = true;
                break;
            case Node::ExternalPage:
                writer.writeAttribute("subtype", "externalpage");
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
            if (!docNode->groupNames().isEmpty())
                writer.writeAttribute("groups", docNode->groupNames().join(","));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Group:
        {
            const GroupNode* gn = static_cast<const GroupNode*>(node);
            writer.writeAttribute("seen", gn->wasSeen() ? "true" : "false");
            writer.writeAttribute("title", gn->title());
            if (!gn->subTitle().isEmpty())
                writer.writeAttribute("subtitle", gn->subTitle());
            if (!gn->moduleName().isEmpty())
                writer.writeAttribute("module", gn->moduleName());
            if (!gn->groupNames().isEmpty())
                writer.writeAttribute("groups", gn->groupNames().join(","));
            /*
              This is not read back in, so it probably
              shouldn't be written out in the first place.
            */
            if (!gn->members().isEmpty()) {
                QStringList names;
                foreach (const Node* member, gn->members())
                    names.append(member->name());
                writer.writeAttribute("members", names.join(","));
            }
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Module:
        {
            const ModuleNode* mn = static_cast<const ModuleNode*>(node);
            writer.writeAttribute("seen", mn->wasSeen() ? "true" : "false");
            writer.writeAttribute("title", mn->title());
            if (!mn->subTitle().isEmpty())
                writer.writeAttribute("subtitle", mn->subTitle());
            if (!mn->moduleName().isEmpty())
                writer.writeAttribute("module", mn->moduleName());
            if (!mn->groupNames().isEmpty())
                writer.writeAttribute("groups", mn->groupNames().join(","));
            /*
              This is not read back in, so it probably
              shouldn't be written out in the first place.
            */
            if (!mn->members().isEmpty()) {
                QStringList names;
                foreach (const Node* member, mn->members())
                    names.append(member->name());
                writer.writeAttribute("members", names.join(","));
            }
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlModule:
        {
            const QmlModuleNode* qmn = static_cast<const QmlModuleNode*>(node);
            writer.writeAttribute("seen", qmn->wasSeen() ? "true" : "false");
            writer.writeAttribute("title", qmn->title());
            if (!qmn->subTitle().isEmpty())
                writer.writeAttribute("subtitle", qmn->subTitle());
            if (!qmn->moduleName().isEmpty())
                writer.writeAttribute("module", qmn->moduleName());
            if (!qmn->groupNames().isEmpty())
                writer.writeAttribute("groups", qmn->groupNames().join(","));
            /*
              This is not read back in, so it probably
              shouldn't be written out in the first place.
            */
            if (!qmn->members().isEmpty()) {
                QStringList names;
                foreach (const Node* member, qmn->members())
                    names.append(member->name());
                writer.writeAttribute("members", names.join(","));
            }
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
            if (functionNode->relates()) {
                writer.writeAttribute("relates", functionNode->relates()->name());
            }
            const PropertyNode* propertyNode = functionNode->associatedProperty();
            if (propertyNode)
                writer.writeAttribute("associated-property", propertyNode->name());
            writer.writeAttribute("type", functionNode->returnType());
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);

            /*
              Note: The "signature" attribute is written to the
              index file, but it is not read back in. Is that ok?
            */
            QString signature = functionNode->signature();
            if (functionNode->isConst())
                signature += " const";
            writer.writeAttribute("signature", signature);

            for (int i = 0; i < functionNode->parameters().size(); ++i) {
                Parameter parameter = functionNode->parameters()[i];
                writer.writeStartElement("parameter");
                writer.writeAttribute("left", parameter.leftType());
                writer.writeAttribute("right", parameter.rightType());
                writer.writeAttribute("name", parameter.name());
                writer.writeAttribute("default", parameter.defaultValue());
                writer.writeEndElement(); // parameter
            }
        }
        break;
    case Node::QmlProperty:
        {
            QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
            writer.writeAttribute("type", qpn->dataType());
            writer.writeAttribute("attached", qpn->isAttached() ? "true" : "false");
            writer.writeAttribute("writable", qpn->isWritable() ? "true" : "false");
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
    case Node::Enum:
        {
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
        break;
    case Node::Typedef:
        {
            const TypedefNode* typedefNode = static_cast<const TypedefNode*>(node);
            if (typedefNode->associatedEnum()) {
                writer.writeAttribute("enum",typedefNode->associatedEnum()->fullDocumentName());
            }
        }
        break;
    default:
        break;
    }

    /*
      For our pages, we canonicalize the target, keyword and content
      item names so that they can be used by qdoc for other sets of
      documentation.

      The reason we do this here is that we don't want to ruin
      externally composed indexes, containing non-qdoc-style target names
      when reading in indexes.

      targets and keywords are now allowed in any node, not just inner nodes.
    */

    if (node->doc().hasTargets()) {
        bool external = false;
        if (node->type() == Node::Document) {
            const DocNode* docNode = static_cast<const DocNode*>(node);
            if (docNode->subType() == Node::ExternalPage)
                external = true;
        }
        foreach (const Atom* target, node->doc().targets()) {
            QString title = target->string();
            QString name =  Doc::canonicalTitle(title);
            writer.writeStartElement("target");
            if (!external)
                writer.writeAttribute("name", name);
            else
                writer.writeAttribute("name", title);
            if (name != title)
                writer.writeAttribute("title", title);
            writer.writeEndElement(); // target
        }
    }
    if (node->doc().hasKeywords()) {
        foreach (const Atom* keyword, node->doc().keywords()) {
            QString title = keyword->string();
            QString name =  Doc::canonicalTitle(title);
            writer.writeStartElement("keyword");
            writer.writeAttribute("name", name);
            if (name != title)
                writer.writeAttribute("title", title);
            writer.writeEndElement(); // keyword
        }
    }

    // Inner nodes and function nodes contain child nodes of some sort, either
    // actual child nodes or function parameters. For these, we close the
    // opening tag, create child elements, then add a closing tag for the
    // element. Elements for all other nodes are closed in the opening tag.

    if (node->isInnerNode()) {
        const InnerNode* inner = static_cast<const InnerNode*>(node);

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

    if (n1->isDocNode() && n2->isDocNode()) {
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
      Note that groups, modules, and QML modules are written
      after all the other nodes.
     */
    if (node->isGroup() || node->isModule() || node->isQmlModule())
        return;

    if (generateIndexSection(writer, node, generateInternalNodes)) {
        if (node->isInnerNode()) {
            const InnerNode* inner = static_cast<const InnerNode*>(node);

            NodeList cnodes = inner->childNodes();
            std::sort(cnodes.begin(), cnodes.end(), compareNodes);

            foreach (Node* child, cnodes) {
                generateIndexSections(writer, child, generateInternalNodes);
            }
        }

        if (node == top) {
            /*
              We wait until the end of the index file to output the group, module,
              and QML module elements. By outputting them at the end, when we read
              the index file back in, all the group, module, and QML module member
              elements will have already been created. It is then only necessary to
              create the group, module, or QML module element and add each member to
              its member list.
            */
            const CNMap& groups = qdb_->groups();
            if (!groups.isEmpty()) {
                CNMap::ConstIterator g = groups.constBegin();
                while (g != groups.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
            }

            const CNMap& modules = qdb_->modules();
            if (!modules.isEmpty()) {
                CNMap::ConstIterator g = modules.constBegin();
                while (g != modules.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
            }

            const CNMap& qmlModules = qdb_->qmlModules();
            if (!qmlModules.isEmpty()) {
                CNMap::ConstIterator g = qmlModules.constBegin();
                while (g != qmlModules.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
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

    top = qdb_->primaryTreeRoot();
    generateIndexSections(writer, top, generateInternalNodes);

    writer.writeEndElement(); // INDEX
    writer.writeEndElement(); // QDOCINDEX
    writer.writeEndDocument();
    file.close();
}

QT_END_NAMESPACE

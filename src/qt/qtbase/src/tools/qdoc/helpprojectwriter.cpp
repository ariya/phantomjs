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

#include <qcryptographichash.h>
#include <qdebug.h>
#include <qhash.h>
#include <qmap.h>

#include "atom.h"
#include "helpprojectwriter.h"
#include "htmlgenerator.h"
#include "config.h"
#include "node.h"
#include "qdocdatabase.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

HelpProjectWriter::HelpProjectWriter(const Config &config,
                                     const QString &defaultFileName,
                                     Generator* g)
    : gen_(g)
{
    /*
      Get the pointer to the singleton for the qdoc database and
      store it locally. This replaces all the local accesses to
      the node tree, which are now private.
     */
    qdb_ = QDocDatabase::qdocDB();

    // The output directory should already have been checked by the calling
    // generator.
    outputDir = config.getOutputDir();

    QStringList names = config.getStringList(CONFIG_QHP + Config::dot + "projects");

    foreach (const QString &projectName, names) {
        HelpProject project;
        project.name = projectName;

        QString prefix = CONFIG_QHP + Config::dot + projectName + Config::dot;
        project.helpNamespace = config.getString(prefix + "namespace");
        project.virtualFolder = config.getString(prefix + "virtualFolder");
        project.fileName = config.getString(prefix + "file");
        if (project.fileName.isEmpty())
            project.fileName = defaultFileName;
        project.extraFiles = config.getStringSet(prefix + "extraFiles");
        project.extraFiles += config.getStringSet(CONFIG_QHP + Config::dot + "extraFiles");
        project.indexTitle = config.getString(prefix + "indexTitle");
        project.indexRoot = config.getString(prefix + "indexRoot");
        project.filterAttributes = config.getStringList(prefix + "filterAttributes").toSet();
        project.includeIndexNodes = config.getBool(prefix + "includeIndexNodes");
        QSet<QString> customFilterNames = config.subVars(prefix + "customFilters");
        foreach (const QString &filterName, customFilterNames) {
            QString name = config.getString(prefix + "customFilters" + Config::dot + filterName + Config::dot + "name");
            QSet<QString> filters = config.getStringList(prefix + "customFilters" + Config::dot + filterName + Config::dot + "filterAttributes").toSet();
            project.customFilters[name] = filters;
        }
        //customFilters = config.defs.

        foreach (QString name, config.getStringSet(prefix + "excluded"))
            project.excluded.insert(name.replace(QLatin1Char('\\'), QLatin1Char('/')));

        foreach (const QString &name, config.getStringList(prefix + "subprojects")) {
            SubProject subproject;
            QString subprefix = prefix + "subprojects" + Config::dot + name + Config::dot;
            subproject.title = config.getString(subprefix + "title");
            subproject.indexTitle = config.getString(subprefix + "indexTitle");
            subproject.sortPages = config.getBool(subprefix + "sortPages");
            subproject.type = config.getString(subprefix + "type");
            readSelectors(subproject, config.getStringList(subprefix + "selectors"));
            project.subprojects[name] = subproject;
        }

        if (project.subprojects.isEmpty()) {
            SubProject subproject;
            readSelectors(subproject, config.getStringList(prefix + "selectors"));
            project.subprojects.insert(QString(), subproject);
        }

        projects.append(project);
    }
}

void HelpProjectWriter::readSelectors(SubProject &subproject, const QStringList &selectors)
{
    QHash<QString, Node::Type> typeHash;
    typeHash["namespace"] = Node::Namespace;
    typeHash["class"] = Node::Class;
    typeHash["fake"] = Node::Document;
    typeHash["enum"] = Node::Enum;
    typeHash["typedef"] = Node::Typedef;
    typeHash["function"] = Node::Function;
    typeHash["property"] = Node::Property;
    typeHash["variable"] = Node::Variable;
    typeHash["qmlproperty"] = Node::QmlProperty;
    typeHash["qmlsignal"] = Node::QmlSignal;
    typeHash["qmlsignalhandler"] = Node::QmlSignalHandler;
    typeHash["qmlmethod"] = Node::QmlMethod;
    typeHash["qmlpropertygroup"] = Node::QmlPropertyGroup;

    QHash<QString, Node::SubType> subTypeHash;
    subTypeHash["example"] = Node::Example;
    subTypeHash["headerfile"] = Node::HeaderFile;
    subTypeHash["file"] = Node::File;
    subTypeHash["group"] = Node::Group;
    subTypeHash["module"] = Node::Module;
    subTypeHash["page"] = Node::Page;
    subTypeHash["externalpage"] = Node::ExternalPage;
    subTypeHash["qmlclass"] = Node::QmlClass;
    subTypeHash["qmlbasictype"] = Node::QmlBasicType;

    QSet<Node::SubType> allSubTypes = QSet<Node::SubType>::fromList(subTypeHash.values());

    foreach (const QString &selector, selectors) {
        QStringList pieces = selector.split(QLatin1Char(':'));
        if (pieces.size() == 1) {
            QString lower = selector.toLower();
            if (typeHash.contains(lower))
                subproject.selectors[typeHash[lower]] = allSubTypes;
        } else if (pieces.size() >= 2) {
            QString lower = pieces[0].toLower();
            pieces = pieces[1].split(QLatin1Char(','));
            if (typeHash.contains(lower)) {
                QSet<Node::SubType> subTypes;
                for (int i = 0; i < pieces.size(); ++i) {
                    QString lower = pieces[i].toLower();
                    if (subTypeHash.contains(lower))
                        subTypes.insert(subTypeHash[lower]);
                }
                subproject.selectors[typeHash[lower]] = subTypes;
            }
        }
    }
}

void HelpProjectWriter::addExtraFile(const QString &file)
{
    for (int i = 0; i < projects.size(); ++i)
        projects[i].extraFiles.insert(file);
}

void HelpProjectWriter::addExtraFiles(const QSet<QString> &files)
{
    for (int i = 0; i < projects.size(); ++i)
        projects[i].extraFiles.unite(files);
}

/*
    Returns a list of strings describing the keyword details for a given node.

    The first string is the human-readable name to be shown in Assistant.
    The second string is a unique identifier.
    The third string is the location of the documentation for the keyword.
*/
QStringList HelpProjectWriter::keywordDetails(const Node *node) const
{
    QStringList details;

    if (node->type() == Node::QmlProperty) {
        // "name"
        details << node->name();
        // "id"
        details << node->parent()->parent()->name()+"::"+node->name();
    }
    else if (node->parent() && !node->parent()->name().isEmpty()) {
        // "name"
        if (node->type() == Node::Enum || node->type() == Node::Typedef)
            details << node->parent()->name()+"::"+node->name();
        else
            details << node->name();
        // "id"
        details << node->parent()->name()+"::"+node->name();
    }
    else if (node->type() == Node::Document) {
        const DocNode *fake = static_cast<const DocNode *>(node);
        if (fake->subType() == Node::QmlClass) {
            details << (QmlClassNode::qmlOnly ? fake->name() : fake->fullTitle());
            details << "QML." + fake->name();
        }
        else {
            details << fake->fullTitle();
            details << fake->fullTitle();
        }
    }
    else {
        details << node->name();
        details << node->name();
    }
    details << gen_->fullDocumentLocation(node,Generator::useOutputSubdirs());
    return details;
}

bool HelpProjectWriter::generateSection(HelpProject &project,
                                        QXmlStreamWriter & /* writer */,
                                        const Node *node)
{
    if (!node->url().isEmpty() && !(project.includeIndexNodes && !node->url().startsWith("http")))
        return false;

    if (node->access() == Node::Private || node->status() == Node::Internal)
        return false;

    if (node->name().isEmpty())
        return true;

    QString docPath = node->doc().location().filePath();
    if (!docPath.isEmpty() && project.excluded.contains(docPath))
        return false;

    QString objName;
    if (node->type() == Node::Document) {
        const DocNode *fake = static_cast<const DocNode *>(node);
        objName = fake->fullTitle();
    }
    else
        objName = node->fullDocumentName();

    // Only add nodes to the set for each subproject if they match a selector.
    // Those that match will be listed in the table of contents.

    foreach (const QString &name, project.subprojects.keys()) {
        SubProject subproject = project.subprojects[name];
        // No selectors: accept all nodes.
        if (subproject.selectors.isEmpty()) {
            project.subprojects[name].nodes[objName] = node;
        }
        else if (subproject.selectors.contains(node->type())) {
            // Accept only the node types in the selectors hash.
            if (node->type() != Node::Document)
                project.subprojects[name].nodes[objName] = node;
            else {
                // Accept only fake nodes with subtypes contained in the selector's
                // mask.
                const DocNode *docNode = static_cast<const DocNode *>(node);
                if (subproject.selectors[node->type()].contains(docNode->subType()) &&
                        docNode->subType() != Node::ExternalPage &&
                        !docNode->fullTitle().isEmpty()) {

                    project.subprojects[name].nodes[objName] = node;
                }
            }
        }
    }

    switch (node->type()) {

    case Node::Class:
        project.keywords.append(keywordDetails(node));
        project.files.insert(gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()));
        break;

    case Node::Namespace:
        project.keywords.append(keywordDetails(node));
        project.files.insert(gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()));
        break;

    case Node::Enum:
        project.keywords.append(keywordDetails(node));
    {
        const EnumNode *enumNode = static_cast<const EnumNode*>(node);
        foreach (const EnumItem &item, enumNode->items()) {
            QStringList details;

            if (enumNode->itemAccess(item.name()) == Node::Private)
                continue;

            if (!node->parent()->name().isEmpty()) {
                details << node->parent()->name()+"::"+item.name(); // "name"
                details << node->parent()->name()+"::"+item.name(); // "id"
            } else {
                details << item.name(); // "name"
                details << item.name(); // "id"
            }
            details << gen_->fullDocumentLocation(node,Generator::useOutputSubdirs());
            project.keywords.append(details);
        }
    }
        break;

    case Node::Property:
    case Node::QmlProperty:
    case Node::QmlSignal:
    case Node::QmlSignalHandler:
    case Node::QmlMethod:
        project.keywords.append(keywordDetails(node));
        break;

    case Node::Function:
    {
        const FunctionNode *funcNode = static_cast<const FunctionNode *>(node);

        // Only insert keywords for non-constructors. Constructors are covered
        // by the classes themselves.

        if (funcNode->metaness() != FunctionNode::Ctor)
            project.keywords.append(keywordDetails(node));

        // Insert member status flags into the entries for the parent
        // node of the function, or the node it is related to.
        // Since parent nodes should have already been inserted into
        // the set of files, we only need to ensure that related nodes
        // are inserted.

        if (node->relates()) {
            project.memberStatus[node->relates()].insert(node->status());
            project.files.insert(gen_->fullDocumentLocation(node->relates(),Generator::useOutputSubdirs()));
        } else if (node->parent())
            project.memberStatus[node->parent()].insert(node->status());
    }
        break;

    case Node::Typedef:
    {
        const TypedefNode *typedefNode = static_cast<const TypedefNode *>(node);
        QStringList typedefDetails = keywordDetails(node);
        const EnumNode *enumNode = typedefNode->associatedEnum();
        // Use the location of any associated enum node in preference
        // to that of the typedef.
        if (enumNode)
            typedefDetails[2] = gen_->fullDocumentLocation(enumNode,Generator::useOutputSubdirs());

        project.keywords.append(typedefDetails);
    }
        break;

    case Node::Variable:
    {
        QString location = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs());
        project.files.insert(location.left(location.lastIndexOf(QLatin1Char('#'))));
        project.keywords.append(keywordDetails(node));
    }
        break;

        // Document nodes (such as manual pages) contain subtypes, titles and other
        // attributes.
    case Node::Document: {
        const DocNode *docNode = static_cast<const DocNode*>(node);
        if (docNode->subType() != Node::ExternalPage &&
                docNode->subType() != Node::Image &&
                !docNode->fullTitle().isEmpty()) {

            if (docNode->subType() != Node::File) {
                if (docNode->doc().hasKeywords()) {
                    foreach (const Atom *keyword, docNode->doc().keywords()) {
                        if (!keyword->string().isEmpty()) {
                            QStringList details;
                            details << keyword->string()
                                    << keyword->string()
                                    << gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()) +
                                       QLatin1Char('#') + Doc::canonicalTitle(keyword->string());
                            project.keywords.append(details);
                        } else
                            docNode->doc().location().warning(
                                        tr("Bad keyword in %1").arg(gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()))
                                        );
                    }
                }
                project.keywords.append(keywordDetails(node));
            }
            project.files.insert(gen_->fullDocumentLocation(node,Generator::useOutputSubdirs()));
        }
        break;
    }
    default:
        ;
    }

    // Add all images referenced in the page to the set of files to include.
    const Atom *atom = node->doc().body().firstAtom();
    while (atom) {
        if (atom->type() == Atom::Image || atom->type() == Atom::InlineImage) {
            // Images are all placed within a single directory regardless of
            // whether the source images are in a nested directory structure.
            QStringList pieces = atom->string().split(QLatin1Char('/'));
            project.files.insert("images/" + pieces.last());
        }
        atom = atom->next();
    }

    return true;
}

void HelpProjectWriter::generateSections(HelpProject &project,
                                         QXmlStreamWriter &writer, const Node *node)
{
    /*
      Don't include index nodes in the help file. Or DITA map nodes.
     */
    if (node->isIndexNode() || node->subType() == Node::DitaMap)
        return;
    if (!generateSection(project, writer, node))
        return;

    if (node->isInnerNode()) {
        const InnerNode *inner = static_cast<const InnerNode *>(node);

        // Ensure that we don't visit nodes more than once.
        QMap<QString, const Node*> childMap;
        foreach (const Node *childNode, inner->childNodes()) {
            if (childNode->isIndexNode())
                continue;

            if (childNode->access() == Node::Private)
                continue;

            if (childNode->type() == Node::Document) {
                childMap[static_cast<const DocNode *>(childNode)->fullTitle()] = childNode;
            }
            else if (childNode->type() == Node::QmlPropertyGroup) {
                /*
                  Don't visit QML property group nodes,
                  but visit their children, which are all
                  QML property nodes.

                  This is probably not correct anymore,
                  because The Qml Property Group is an
                  actual documented thing.
                 */
                const InnerNode* inner = static_cast<const InnerNode*>(childNode);
                foreach (const Node* n, inner->childNodes()) {
                    if (n->access() == Node::Private)
                        continue;
                    childMap[n->fullDocumentName()] = n;
                }
            }
            else {
                // Store member status of children
                project.memberStatus[node].insert(childNode->status());
                if (childNode->relates()) {
                    project.memberStatus[childNode->relates()].insert(childNode->status());
                    project.files.insert(gen_->fullDocumentLocation(childNode->relates(),
                                                                    Generator::useOutputSubdirs()));
                }

                if (childNode->type() == Node::Function) {
                    const FunctionNode *funcNode = static_cast<const FunctionNode *>(childNode);
                    if (funcNode->isOverload())
                        continue;
                }
                childMap[childNode->fullDocumentName()] = childNode;
            }
        }
        // Insert files for all/compatibility/obsolete members
        addMembers(project, writer, node, false);
        if (node->relates())
            addMembers(project, writer, node->relates(), false);

        foreach (const Node *child, childMap)
            generateSections(project, writer, child);
    }
}

void HelpProjectWriter::generate()
{
    for (int i = 0; i < projects.size(); ++i)
        generateProject(projects[i]);
}

void HelpProjectWriter::writeHashFile(QFile &file)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);

    QFile hashFile(file.fileName() + ".sha1");
    if (!hashFile.open(QFile::WriteOnly | QFile::Text))
        return;

    hashFile.write(hash.result().toHex());
    hashFile.close();
}

void HelpProjectWriter::writeSection(QXmlStreamWriter &writer, const QString &path,
                                           const QString &value)
{
    writer.writeStartElement(QStringLiteral("section"));
    writer.writeAttribute(QStringLiteral("ref"), path);
    writer.writeAttribute(QStringLiteral("title"), value);
    writer.writeEndElement(); // section
}

/*
    Add files for all members, compatibility members and obsolete members
    Also write subsections for these depending on 'writeSections' (default=true).
*/
void HelpProjectWriter::addMembers(HelpProject &project, QXmlStreamWriter &writer,
                                          const Node *node, bool writeSections)
{
    QString href = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs());
    href = href.left(href.size()-5);
    if (href.isEmpty())
        return;

    Node::SubType subType = static_cast<const DocNode*>(node)->subType();

    bool derivedClass = false;
    if (node->type() == Node::Class)
        derivedClass = !(static_cast<const ClassNode *>(node)->baseClasses().isEmpty());

    // Do not generate a 'List of all members' for namespaces or header files,
    // but always generate it for derived classes and QML classes
    if (node->type() != Node::Namespace && subType != Node::HeaderFile &&
            (derivedClass || subType == Node::QmlClass ||
             !project.memberStatus[node].isEmpty())) {
        QString membersPath = href + QStringLiteral("-members.html");
        project.files.insert(membersPath);
        if (writeSections)
            writeSection(writer, membersPath, tr("List of all members"));
    }
    if (project.memberStatus[node].contains(Node::Compat)) {
        QString compatPath = href + QStringLiteral("-compat.html");
        project.files.insert(compatPath);
        if (writeSections)
            writeSection(writer, compatPath, tr("Compatibility members"));
    }
    if (project.memberStatus[node].contains(Node::Obsolete)) {
        QString obsoletePath = href + QStringLiteral("-obsolete.html");
        project.files.insert(obsoletePath);
        if (writeSections)
            writeSection(writer, obsoletePath, tr("Obsolete members"));
    }
}

void HelpProjectWriter::writeNode(HelpProject &project, QXmlStreamWriter &writer,
                                  const Node *node)
{
    QString href = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs());
    QString objName = node->name();

    switch (node->type()) {

    case Node::Class:
        writer.writeStartElement("section");
        writer.writeAttribute("ref", href);
        if (node->parent() && !node->parent()->name().isEmpty())
            writer.writeAttribute("title", tr("%1::%2 Class Reference").arg(node->parent()->name()).arg(objName));
        else
            writer.writeAttribute("title", tr("%1 Class Reference").arg(objName));

        addMembers(project, writer, node);
        writer.writeEndElement(); // section
        break;

    case Node::Namespace:
        writeSection(writer, href, objName);
        break;

    case Node::Document: {
        // Document nodes (such as manual pages) contain subtypes, titles and other
        // attributes.
        const DocNode *docNode = static_cast<const DocNode*>(node);

        writer.writeStartElement("section");
        writer.writeAttribute("ref", href);
        if (docNode->subType() == Node::QmlClass)
            writer.writeAttribute("title", tr("%1 Type Reference").arg(docNode->fullTitle()));
        else
            writer.writeAttribute("title", docNode->fullTitle());

        if ((docNode->subType() == Node::HeaderFile) || (docNode->subType() == Node::QmlClass))
            addMembers(project, writer, node);

        writer.writeEndElement(); // section
    }
        break;
    default:
        ;
    }
}

void HelpProjectWriter::generateProject(HelpProject &project)
{
    const Node *rootNode;
    if (!project.indexRoot.isEmpty())
        rootNode = qdb_->findDocNodeByTitle(project.indexRoot);
    else
        rootNode = qdb_->treeRoot();

    if (!rootNode)
        return;

    project.files.clear();
    project.keywords.clear();

    QFile file(outputDir + QDir::separator() + project.fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("QtHelpProject");
    writer.writeAttribute("version", "1.0");

    // Write metaData, virtualFolder and namespace elements.
    writer.writeTextElement("namespace", project.helpNamespace);
    writer.writeTextElement("virtualFolder", project.virtualFolder);

    // Write customFilter elements.
    QHash<QString, QSet<QString> >::ConstIterator it;
    for (it = project.customFilters.constBegin(); it != project.customFilters.constEnd(); ++it) {
        writer.writeStartElement("customFilter");
        writer.writeAttribute("name", it.key());
        foreach (const QString &filter, it.value())
            writer.writeTextElement("filterAttribute", filter);
        writer.writeEndElement(); // customFilter
    }

    // Start the filterSection.
    writer.writeStartElement("filterSection");

    // Write filterAttribute elements.
    foreach (const QString &filterName, project.filterAttributes)
        writer.writeTextElement("filterAttribute", filterName);

    writer.writeStartElement("toc");
    writer.writeStartElement("section");
    const Node* node = qdb_->findDocNodeByTitle(project.indexTitle);
    if (node == 0)
        node = qdb_->findNode(QStringList("index.html"));
    QString indexPath;
    // Never use a collision node as a landing page
    if (node && !node->isCollisionNode())
        indexPath = gen_->fullDocumentLocation(node,Generator::useOutputSubdirs());
    else
        indexPath = "index.html";
    writer.writeAttribute("ref", indexPath);
    writer.writeAttribute("title", project.indexTitle);
    project.files.insert(gen_->fullDocumentLocation(rootNode));

    generateSections(project, writer, rootNode);

    foreach (const QString &name, project.subprojects.keys()) {
        SubProject subproject = project.subprojects[name];

        if (subproject.type == QLatin1String("manual")) {

            const DocNode *indexPage = qdb_->findDocNodeByTitle(subproject.indexTitle);
            if (indexPage) {
                Text indexBody = indexPage->doc().body();
                const Atom *atom = indexBody.firstAtom();
                QStack<int> sectionStack;
                bool inItem = false;

                while (atom) {
                    switch (atom->type()) {
                    case Atom::ListLeft:
                        sectionStack.push(0);
                        break;
                    case Atom::ListRight:
                        if (sectionStack.pop() > 0)
                            writer.writeEndElement(); // section
                        break;
                    case Atom::ListItemLeft:
                        inItem = true;
                        break;
                    case Atom::ListItemRight:
                        inItem = false;
                        break;
                    case Atom::Link:
                        if (inItem) {
                            if (sectionStack.top() > 0)
                                writer.writeEndElement(); // section

                            const DocNode *page = qdb_->findDocNodeByTitle(atom->string());
                            writer.writeStartElement("section");
                            QString indexPath = gen_->fullDocumentLocation(page,
                                                                           Generator::useOutputSubdirs());
                            writer.writeAttribute("ref", indexPath);
                            writer.writeAttribute("title", atom->string());
                            project.files.insert(indexPath);

                            sectionStack.top() += 1;
                        }
                        break;
                    default:
                        ;
                    }

                    if (atom == indexBody.lastAtom())
                        break;
                    atom = atom->next();
                }
            } else
                rootNode->doc().location().warning(
                            tr("Failed to find index: %1").arg(subproject.indexTitle)
                            );

        } else {

            if (!name.isEmpty()) {
                writer.writeStartElement("section");
                QString indexPath = gen_->fullDocumentLocation(qdb_->findDocNodeByTitle(subproject.indexTitle),Generator::useOutputSubdirs());
                writer.writeAttribute("ref", indexPath);
                writer.writeAttribute("title", subproject.title);
                project.files.insert(indexPath);
            }
            if (subproject.sortPages) {
                QStringList titles = subproject.nodes.keys();
                titles.sort();
                foreach (const QString &title, titles) {
                    writeNode(project, writer, subproject.nodes[title]);
                }
            } else {
                // Find a contents node and navigate from there, using the NextLink values.
                QSet<QString> visited;

                foreach (const Node *node, subproject.nodes) {
                    QString nextTitle = node->links().value(Node::NextLink).first;
                    if (!nextTitle.isEmpty() &&
                            node->links().value(Node::ContentsLink).first.isEmpty()) {

                        DocNode *nextPage = const_cast<DocNode *>(qdb_->findDocNodeByTitle(nextTitle));

                        // Write the contents node.
                        writeNode(project, writer, node);

                        while (nextPage) {
                            writeNode(project, writer, nextPage);
                            nextTitle = nextPage->links().value(Node::NextLink).first;
                            if (nextTitle.isEmpty() || visited.contains(nextTitle))
                                break;
                            nextPage = const_cast<DocNode *>(qdb_->findDocNodeByTitle(nextTitle));
                            visited.insert(nextTitle);
                        }
                        break;
                    }
                }
            }

            if (!name.isEmpty())
                writer.writeEndElement(); // section
        }
    }

    writer.writeEndElement(); // section
    writer.writeEndElement(); // toc

    writer.writeStartElement("keywords");
    foreach (const QStringList &details, project.keywords) {
        writer.writeStartElement("keyword");
        writer.writeAttribute("name", details[0]);
        writer.writeAttribute("id", details[1]);
        writer.writeAttribute("ref", details[2]);
        writer.writeEndElement(); //keyword
    }
    writer.writeEndElement(); // keywords

    writer.writeStartElement("files");
    foreach (const QString &usedFile, project.files) {
        if (!usedFile.isEmpty())
            writer.writeTextElement("file", usedFile);
    }
    foreach (const QString &usedFile, project.extraFiles)
        writer.writeTextElement("file", usedFile);
    writer.writeEndElement(); // files

    writer.writeEndElement(); // filterSection
    writer.writeEndElement(); // QtHelpProject
    writer.writeEndDocument();
    writeHashFile(file);
    file.close();
}

QT_END_NAMESPACE

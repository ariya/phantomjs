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
  ditaxmlgenerator.cpp
*/

#include <qdebug.h>
#include <qlist.h>
#include <qiterator.h>
#include <qtextcodec.h>
#include <quuid.h>
#include "codemarker.h"
#include "codeparser.h"
#include "ditaxmlgenerator.h"
#include "node.h"
#include "quoter.h"
#include "separator.h"
#include "tree.h"
#include <ctype.h>
#include "qdocdatabase.h"

QT_BEGIN_NAMESPACE

#define COMMAND_VERSION                         Doc::alias("version")
int DitaXmlGenerator::id = 0;

/*
  The strings in this array must appear in the same order as
  the values in enum DitaXmlGenerator::DitaTag.
 */
QString DitaXmlGenerator::ditaTags[] =
{
    "",
    "alt",
    "apiData",
    "apiDef",
    "apiDefItem",
    "apiDesc",
    "apiDetail",
    "apiItemName",
    "APIMap",
    "apiName",
    "apiRef",
    "apiRelation",
    "audience",
    "author",
    "b",
    "body",
    "bodydiv",
    "brand",
    "category",
    "codeblock",
    "colspec",
    "comment",
    "component",
    "copyrholder",
    "copyright",
    "copyryear",
    "created",
    "critdates",
    "cxxAPIMap",
    "cxxClass",
    "cxxClassAbstract",
    "cxxClassAccessSpecifier",
    "cxxClassAPIItemLocation",
    "cxxClassBaseClass",
    "cxxClassDeclarationFile",
    "cxxClassDeclarationFileLine",
    "cxxClassDeclarationFileLineStart",
    "cxxClassDeclarationFileLineEnd",
    "cxxClassDefinition",
    "cxxClassDerivation",
    "cxxClassDerivationAccessSpecifier",
    "cxxClassDerivations",
    "cxxClassDetail",
    "cxxClassNested",
    "cxxClassNestedClass",
    "cxxClassNestedDetail",
    "cxxDefine",
    "cxxDefineAccessSpecifier",
    "cxxDefineAPIItemLocation",
    "cxxDefineDeclarationFile",
    "cxxDefineDeclarationFileLine",
    "cxxDefineDefinition",
    "cxxDefineDetail",
    "cxxDefineNameLookup",
    "cxxDefineParameter",
    "cxxDefineParameterDeclarationName",
    "cxxDefineParameters",
    "cxxDefinePrototype",
    "cxxDefineReimplemented",
    "cxxEnumeration",
    "cxxEnumerationAccessSpecifier",
    "cxxEnumerationAPIItemLocation",
    "cxxEnumerationDeclarationFile",
    "cxxEnumerationDeclarationFileLine",
    "cxxEnumerationDeclarationFileLineStart",
    "cxxEnumerationDeclarationFileLineEnd",
    "cxxEnumerationDefinition",
    "cxxEnumerationDetail",
    "cxxEnumerationNameLookup",
    "cxxEnumerationPrototype",
    "cxxEnumerationScopedName",
    "cxxEnumerator",
    "cxxEnumeratorInitialiser",
    "cxxEnumeratorNameLookup",
    "cxxEnumeratorPrototype",
    "cxxEnumerators",
    "cxxEnumeratorScopedName",
    "cxxFunction",
    "cxxFunctionAccessSpecifier",
    "cxxFunctionAPIItemLocation",
    "cxxFunctionConst",
    "cxxFunctionConstructor",
    "cxxFunctionDeclarationFile",
    "cxxFunctionDeclarationFileLine",
    "cxxFunctionDeclaredType",
    "cxxFunctionDefinition",
    "cxxFunctionDestructor",
    "cxxFunctionDetail",
    "cxxFunctionNameLookup",
    "cxxFunctionParameter",
    "cxxFunctionParameterDeclarationName",
    "cxxFunctionParameterDeclaredType",
    "cxxFunctionParameterDefaultValue",
    "cxxFunctionParameters",
    "cxxFunctionPrototype",
    "cxxFunctionPureVirtual",
    "cxxFunctionReimplemented",
    "cxxFunctionScopedName",
    "cxxFunctionStorageClassSpecifierStatic",
    "cxxFunctionVirtual",
    "cxxTypedef",
    "cxxTypedefAccessSpecifier",
    "cxxTypedefAPIItemLocation",
    "cxxTypedefDeclarationFile",
    "cxxTypedefDeclarationFileLine",
    "cxxTypedefDefinition",
    "cxxTypedefDetail",
    "cxxTypedefNameLookup",
    "cxxTypedefScopedName",
    "cxxVariable",
    "cxxVariableAccessSpecifier",
    "cxxVariableAPIItemLocation",
    "cxxVariableDeclarationFile",
    "cxxVariableDeclarationFileLine",
    "cxxVariableDeclaredType",
    "cxxVariableDefinition",
    "cxxVariableDetail",
    "cxxVariableNameLookup",
    "cxxVariablePrototype",
    "cxxVariableReimplemented",
    "cxxVariableScopedName",
    "cxxVariableStorageClassSpecifierStatic",
    "data",
    "data-about",
    "dd",
    "dl",
    "dlentry",
    "dt",
    "entry",
    "fig",
    "i",
    "image",
    "keyword",
    "keywords",
    "li",
    "link",
    "linktext",
    "lq",
    "map",
    "mapref",
    "metadata",
    "note",
    "ol",
    "othermeta",
    "p",
    "parameter",
    "permissions",
    "ph",
    "platform",
    "pre",
    "prodinfo",
    "prodname",
    "prolog",
    "publisher",
    "qmlAttached",
    "qmlDetail",
    "qmlImportModule",
    "qmlInheritedBy",
    "qmlInherits",
    "qmlInstantiates",
    "qmlMethod",
    "qmlMethodDef",
    "qmlMethodDetail",
    "qmlName",
    "qmlProperty",
    "qmlPropertyDef",
    "qmlPropertyDetail",
    "qmlPropertyGroup",
    "qmlPropertyGroupDef",
    "qmlPropertyGroupDetail",
    "qmlQualifier",
    "qmlSignal",
    "qmlSignalDef",
    "qmlSignalDetail",
    "qmlSignalHandler",
    "qmlSignalHandlerDef",
    "qmlSignalHandlerDetail",
    "qmlSignature",
    "qmlSince",
    "qmlType",
    "qmlTypeDef",
    "qmlTypeDetail",
    "related-links",
    "resourceid",
    "revised",
    "row",
    "section",
    "sectiondiv",
    "shortdesc",
    "simpletable",
    "source",
    "stentry",
    "sthead",
    "strow",
    "sub",
    "sup",
    "table",
    "tbody",
    "tgroup",
    "thead",
    "title",
    "tm",
    "topic",
    "topicmeta",
    "topicref",
    "tt",
    "u",
    "uicontrol",
    "ul",
    "unknown",
    "vrm",
    "vrmlist",
    "xref",
    ""
};

/*!
  Composes a string to be used as an href attribute in DITA
  XML. It is composed of the file name and the UUID separated
  by a '#'. If this node is a class node, the file name is
  taken from this node; if this node is a function node, the
  file name is taken from the parent node of this node.
 */
QString DitaXmlGenerator::ditaXmlHref(Node* n)
{
    QString href;
    if ((n->type() == Node::Function) ||
            (n->type() == Node::Property) ||
            (n->type() == Node::Variable)) {
        href = fileBase(n->parent());
    }
    else {
        href = fileBase(n);
    }
    if (!href.endsWith(".xml") && !href.endsWith(".dita"))
        href += ".dita";
    return href + QLatin1Char('#') + n->guid();
}

void DitaXmlGenerator::debugPara(const QString& t)
{
    writeStartTag(DT_p);
    xmlWriter().writeAttribute("outputclass",t);
    xmlWriter().writeCharacters(t);
    writeEndTag(); // </p>
}

static bool showBrokenLinks = false;

/*!
  Quick, dirty, and very ugly. Unescape \a text
  so QXmlStreamWriter::writeCharacters() can put
  the escapes back in again!
 */
void DitaXmlGenerator::writeCharacters(const QString& text)
{
    QString t = text;
    t = t.replace("&lt;","<");
    t = t.replace("&gt;",">");
    t = t.replace("&amp;","&");
    t = t.replace("&quot;","\"");
    xmlWriter().writeCharacters(t);
}

/*!
  Appends an <xref> element to the current XML stream
  with the \a href attribute and the \a text.
 */
void DitaXmlGenerator::addLink(const QString& href,
                               const QStringRef& text,
                               DitaTag t)
{
    if (!href.isEmpty()) {
        writeStartTag(t);
        // formathtml
        writeHrefAttribute(href);
        writeCharacters(text.toString());
        writeEndTag(); // </t>
    }
    else {
        writeCharacters(text.toString());
    }
}

/*!
  Push \a t onto the dita tag stack and write the appropriate
  start tag to the DITA XML file.
 */
void DitaXmlGenerator::writeStartTag(DitaTag t)
{
    xmlWriter().writeStartElement(ditaTags[t]);
    tagStack.push(t);
}

/*!
  Pop the current DITA tag off the stack, and write the
  appropriate end tag to the DITA XML file. If \a t is
  not \e DT_NONE (default), then \a t contains the enum
  value of the tag that should be on top of the stack.

  If the stack is empty, no end tag is written and false
  is returned. Otherwise, an end tag is written and true
  is returned.
 */
bool DitaXmlGenerator::writeEndTag(DitaTag t)
{
    if (tagStack.isEmpty())
        return false;
    DitaTag top = tagStack.pop();
    if (t > DT_NONE && top != t)
        qDebug() << "Expected:" << t << "ACTUAL:" << top;
    xmlWriter().writeEndElement();
    return true;
}

/*!
  Return the current DITA element tag, the one
  on top of the stack.
 */
DitaXmlGenerator::DitaTag DitaXmlGenerator::currentTag()
{
    return tagStack.top();
}

/*!
  Write the start \a tag. if \a title is not empty, generate
  a GUID from it and write the GUID as the value of the \e{id}
  attribute.

  Then if \a outputclass is not empty, write it as the value
  of the \a outputclass attribute.

  Fiunally, set the section nesting level to 1 and return 1.
 */
int DitaXmlGenerator::enterDesc(DitaTag tag, const QString& outputclass, const QString& title)
{
    writeStartTag(tag);
    if (!title.isEmpty()) {
        writeGuidAttribute(title);
        //Are there cases where the spectitle is required?
        //xmlWriter().writeAttribute("spectitle",title);
    }
    if (!outputclass.isEmpty())
        xmlWriter().writeAttribute("outputclass",outputclass);
    sectionNestingLevel = 1;
    return sectionNestingLevel;
}

/*!
  If the section nesting level is 0, output a \c{<section>}
  element with an \e id attribute generated from \a title and
  an \e outputclass attribute set to \a outputclass.
  If \a title is null, no \e id attribute is output.
  If \a outputclass is empty, no \e outputclass attribute
  is output.

  Finally, increment the section nesting level and return
  the new value.
 */
int DitaXmlGenerator::enterSection(const QString& outputclass, const QString& title)
{
    if (sectionNestingLevel == 0) {
        writeStartTag(DT_section);
        if (!title.isEmpty())
            writeGuidAttribute(title);
        if (!outputclass.isEmpty())
            xmlWriter().writeAttribute("outputclass",outputclass);
    }
    else if (!title.isEmpty()) {
        writeStartTag(DT_p);
        writeGuidAttribute(title);
        if (!outputclass.isEmpty())
            xmlWriter().writeAttribute("outputclass",outputclass);
        writeCharacters(title);
        writeEndTag(); // </p>
    }
    return ++sectionNestingLevel;
}

/*!
  If the section nesting level is greater than 0, decrement
  it. If it becomes 0, output a \c {</section>}. Return the
  decremented section nesting level.
 */
int DitaXmlGenerator::leaveSection()
{
    if (sectionNestingLevel > 0) {
        --sectionNestingLevel;
        if (sectionNestingLevel == 0)
            writeEndTag(); // </section> or </apiDesc>
    }
    return sectionNestingLevel;
}

/*!
  Constructs the DITA XML output generator.
 */
DitaXmlGenerator::DitaXmlGenerator()
    : inDetailedDescription(false),
      inLegaleseText(false),
      inObsoleteLink(false),
      inTableBody(false),
      noLinks(false),
      obsoleteLinks(false),
      divNestingLevel(0),
      sectionNestingLevel(0),
      tableColumnCount(0),
      funcLeftParen("\\S(\\()"),
      nodeTypeMaps(Node::LastType,0),
      nodeSubtypeMaps(Node::LastSubtype,0),
      pageTypeMaps(Node::OnBeyondZebra,0)
{
}

/*!
  Destroys the DITA XML output generator.
 */
DitaXmlGenerator::~DitaXmlGenerator()
{
    GuidMaps::iterator i = guidMaps.begin();
    while (i != guidMaps.end()) {
        delete i.value();
        ++i;
    }
}

/*!
  Initializes the DITA XML output generator's data structures
  from the configuration class \a config.
 */
void DitaXmlGenerator::initializeGenerator(const Config &config)
{
    Generator::initializeGenerator(config);
    obsoleteLinks = config.getBool(CONFIG_OBSOLETELINKS);
    setImageFileExtensions(QStringList() << "png" << "jpg" << "jpeg" << "gif");

    style = config.getString(DitaXmlGenerator::format() +
                             Config::dot +
                             DITAXMLGENERATOR_STYLE);
    postHeader = config.getString(DitaXmlGenerator::format() +
                                  Config::dot +
                                  DITAXMLGENERATOR_POSTHEADER);
    postPostHeader = config.getString(DitaXmlGenerator::format() +
                                      Config::dot +
                                      DITAXMLGENERATOR_POSTPOSTHEADER);
    footer = config.getString(DitaXmlGenerator::format() +
                              Config::dot +
                              DITAXMLGENERATOR_FOOTER);
    address = config.getString(DitaXmlGenerator::format() +
                               Config::dot +
                               DITAXMLGENERATOR_ADDRESS);
    pleaseGenerateMacRef = config.getBool(DitaXmlGenerator::format() +
                                          Config::dot +
                                          DITAXMLGENERATOR_GENERATEMACREFS);

    project = config.getString(CONFIG_PROJECT);
    projectDescription = config.getString(CONFIG_DESCRIPTION);
    if (projectDescription.isEmpty() && !project.isEmpty())
        projectDescription = project + " Reference Documentation";

    projectUrl = config.getString(CONFIG_URL);
    tagFile_ = config.getString(CONFIG_TAGFILE);

#ifndef QT_NO_TEXTCODEC
    outputEncoding = config.getString(CONFIG_OUTPUTENCODING);
    if (outputEncoding.isEmpty())
        outputEncoding = QLatin1String("ISO-8859-1");
    outputCodec = QTextCodec::codecForName(outputEncoding.toLocal8Bit());
#endif

    naturalLanguage = config.getString(CONFIG_NATURALLANGUAGE);
    if (naturalLanguage.isEmpty())
        naturalLanguage = QLatin1String("en");

    config.subVarsAndValues("dita.metadata.default",metadataDefaults);
    QSet<QString> editionNames = config.subVars(CONFIG_EDITION);
    QSet<QString>::ConstIterator edition = editionNames.constBegin();
    while (edition != editionNames.constEnd()) {
        QString editionName = *edition;
        QStringList editionModules = config.getStringList(CONFIG_EDITION +
                                                          Config::dot +
                                                          editionName +
                                                          Config::dot +
                                                          "modules");
        QStringList editionGroups = config.getStringList(CONFIG_EDITION +
                                                         Config::dot +
                                                         editionName +
                                                         Config::dot +
                                                         "groups");

        if (!editionModules.isEmpty())
            editionModuleMap[editionName] = editionModules;
        if (!editionGroups.isEmpty())
            editionGroupMap[editionName] = editionGroups;

        ++edition;
    }

    stylesheets = config.getStringList(DitaXmlGenerator::format() +
                                       Config::dot +
                                       DITAXMLGENERATOR_STYLESHEETS);
    customHeadElements = config.getStringList(DitaXmlGenerator::format() +
                                              Config::dot +
                                              DITAXMLGENERATOR_CUSTOMHEADELEMENTS);
    version = config.getString(CONFIG_VERSION);
    vrm = version.split(QLatin1Char('.'));
}

/*!
  Gracefully terminates the DITA XML output generator.
 */
void DitaXmlGenerator::terminateGenerator()
{
    Generator::terminateGenerator();
}

/*!
  Returns "DITAXML".
 */
QString DitaXmlGenerator::format()
{
    return "DITAXML";
}

/*!
  Calls lookupGuid() to get a GUID for \a text, then writes
  it to the XML stream as an "id" attribute, and returns it.
 */
QString DitaXmlGenerator::writeGuidAttribute(QString text)
{
    QString guid = lookupGuid(outFileName(),text);
    xmlWriter().writeAttribute("id",guid);
    return guid;
}


/*!
  Write's the GUID for the \a node to the current XML stream
  as an "id" attribute. If the \a node doesn't yet have a GUID,
  one is generated.
 */
void DitaXmlGenerator::writeGuidAttribute(Node* node)
{
    xmlWriter().writeAttribute("id",node->guid());
}

/*!
  Looks up \a text in the GUID map. If it finds \a text,
  it returns the associated GUID. Otherwise it inserts
  \a text into the map with a new GUID, and it returns
  the new GUID.
 */
QString DitaXmlGenerator::lookupGuid(QString text)
{
    QMap<QString, QString>::const_iterator i = name2guidMap.constFind(text);
    if (i != name2guidMap.constEnd())
        return i.value();
    QString guid = Node::cleanId(text);
    name2guidMap.insert(text,guid);
    return guid;
}

/*!
  First, look up the GUID map for \a fileName. If there isn't
  a GUID map for \a fileName, create one and insert it into
  the map of GUID maps. Then look up \a text in that GUID map.
  If \a text is found, return the associated GUID. Otherwise,
  insert \a text into the GUID map with a new GUID, and return
  the new GUID.
 */
QString DitaXmlGenerator::lookupGuid(const QString& fileName, const QString& text)
{
    GuidMap* gm = lookupGuidMap(fileName);
    GuidMap::const_iterator i = gm->constFind(text);
    if (i != gm->constEnd())
        return i.value();
    QString guid = Node::cleanId(text);
    gm->insert(text,guid);
    return guid;
}

/*!
  Looks up \a fileName in the map of GUID maps. If it finds
  \a fileName, it returns a pointer to the associated GUID
  map. Otherwise it creates a new GUID map and inserts it
  into the map of GUID maps with \a fileName as its key.
 */
GuidMap* DitaXmlGenerator::lookupGuidMap(const QString& fileName)
{
    GuidMaps::const_iterator i = guidMaps.constFind(fileName);
    if (i != guidMaps.constEnd())
        return i.value();
    GuidMap* gm = new GuidMap;
    guidMaps.insert(fileName,gm);
    return gm;
}

/*!
  Traverses the database generating all the DITA XML documentation.
 */
void DitaXmlGenerator::generateTree()
{
    qdb_->buildCollections();
    if (!runPrepareOnly()) {
        Generator::generateTree();
        generateCollisionPages();
    }

    if (!runGenerateOnly()) {
        QString fileBase = project.toLower().simplified().replace(QLatin1Char(' '), QLatin1Char('-'));
        qdb_->generateIndex(outputDir() + QLatin1Char('/') + fileBase + ".index",
                            projectUrl,
                            projectDescription,
                            this,
                            true);
    }

    if (!runPrepareOnly()) {
        writeDitaMap();
        /*
          Generate the XML tag file, if it was requested.
        */
        qdb_->generateTagFile(tagFile_, this);
    }
}

static int countTableColumns(const Atom* t)
{
    int result = 0;
    if (t->type() == Atom::TableHeaderLeft) {
        while (t->type() == Atom::TableHeaderLeft) {
            int count = 0;
            t = t->next();
            while (t->type() != Atom::TableHeaderRight) {
                if (t->type() == Atom::TableItemLeft) {
                    for (int i=0; i<t->count(); ++i) {
                        QString attr = t->string(i);
                        if (!attr.contains('=')) {
                            QStringList spans = attr.split(QLatin1Char(','));
                            if (spans.size() == 2) {
                                count += spans[0].toInt();
                            }
                            else {
                                ++count;
                            }
                        }
                    }
                }
                t = t->next();
            }
            if (count > result)
                result = count;
            t = t->next();
        }
    }
    else if (t->type() == Atom::TableRowLeft) {
        while (t->type() != Atom::TableRowRight) {
            if (t->type() == Atom::TableItemLeft) {
                for (int i=0; i<t->count(); ++i) {
                    QString attr = t->string(i);
                    if (!attr.contains('=')) {
                        QStringList spans = attr.split(QLatin1Char(','));
                        if (spans.size() == 2) {
                            result += spans[0].toInt();
                        }
                        else {
                            ++result;
                        }
                    }
                }
            }
            t = t->next();
        }
    }
    return result;
}

/*!
  Generate html from an instance of Atom.
 */
int DitaXmlGenerator::generateAtom(const Atom *atom,
                                   const Node *relative,
                                   CodeMarker *marker)
{
    int skipAhead = 0;
    QString hx, str;
    static bool in_para = false;
    QString guid, hc, attr;

    switch (atom->type()) {
    case Atom::AbstractLeft:
        break;
    case Atom::AbstractRight:
        break;
    case Atom::AutoLink:
        if (!noLinks && !inLink_ && !inContents_ && !inSectionHeading_) {
            const Node* node = 0;
            QString link = getLink(atom, relative, &node);
            if (!link.isEmpty()) {
                beginLink(link);
                generateLink(atom, marker);
                endLink();
            }
            else {
                writeCharacters(protectEnc(atom->string()));
            }
        }
        else {
            writeCharacters(protectEnc(atom->string()));
        }
        break;
    case Atom::BaseName:
        break;
    case Atom::BriefLeft:
        {
            Node::Type t = relative->type();
            if (inSection()) {
                in_para = true;
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","brief");
            }
            else {
                noLinks = true;
                writeStartTag(DT_shortdesc);
            }
            if (t == Node::Property || t == Node::Variable) {
                xmlWriter().writeCharacters("This ");
                if (relative->type() == Node::Property)
                    xmlWriter().writeCharacters("property");
                else if (relative->type() == Node::Variable)
                    xmlWriter().writeCharacters("variable");
                xmlWriter().writeCharacters(" holds ");
            }
            if (noLinks) {
                atom = atom->next();
                while (atom != 0 && atom->type() != Atom::BriefRight) {
                    if (atom->type() == Atom::String ||
                        atom->type() == Atom::AutoLink)
                        str += atom->string();
                    skipAhead++;
                    atom = atom->next();
                }
                if (t == Node::Property || t == Node::Variable)
                    str[0] = str[0].toLower();
                if (str.endsWith(QLatin1Char('.')))
                    str.truncate(str.length() - 1);
                writeCharacters(str + QLatin1Char('.'));
            }
        }
        break;
    case Atom::BriefRight:
        //        if (relative->type() != Node::Document)
        writeEndTag(); // </shortdesc> or </p>
        if (in_para)
            in_para = false;
        noLinks = false;
        break;
    case Atom::C:
        writeStartTag(DT_tt);
        if (inLink_) {
            writeCharacters(protectEnc(plainCode(atom->string())));
        }
        else {
            writeText(atom->string(), relative);
        }
        writeEndTag(); // see writeStartElement() above
        break;
    case Atom::Code:
        {
            writeStartTag(DT_codeblock);
            xmlWriter().writeAttribute("outputclass","cpp");
            writeCharacters("\n");
            writeText(trimmedTrailing(atom->string()), relative);
            writeEndTag(); // </codeblock>
        }
        break;
    case Atom::Qml:
        writeStartTag(DT_codeblock);
        xmlWriter().writeAttribute("outputclass","qml");
        writeCharacters("\n");
        writeText(trimmedTrailing(atom->string()), relative);
        writeEndTag(); // </codeblock>
        break;
    case Atom::CodeNew:
        writeStartTag(DT_p);
        xmlWriter().writeCharacters("you can rewrite it as");
        writeEndTag(); // </p>
        writeStartTag(DT_codeblock);
        writeCharacters("\n");
        writeText(trimmedTrailing(atom->string()), relative);
        writeEndTag(); // </codeblock>
        break;
    case Atom::CodeOld:
        writeStartTag(DT_p);
        xmlWriter().writeCharacters("For example, if you have code like");
        writeEndTag(); // </p>
        // fallthrough
    case Atom::CodeBad:
        writeStartTag(DT_codeblock);
        writeCharacters("\n");
        writeCharacters(trimmedTrailing(plainCode(atom->string())));
        writeEndTag(); // </codeblock>
        break;
    case Atom::DivLeft:
    {
        bool inStartElement = false;
        attr = atom->string();
        DitaTag t = currentTag();
        if ((t == DT_section) || (t == DT_sectiondiv)) {
            writeStartTag(DT_sectiondiv);
            divNestingLevel++;
            inStartElement = true;
        }
        else if ((t == DT_body) || (t == DT_bodydiv)) {
            writeStartTag(DT_bodydiv);
            divNestingLevel++;
            inStartElement = true;
        }
        if (!attr.isEmpty()) {
            if (attr.contains('=')) {
                int index = 0;
                int from = 0;
                QString values;
                while (index >= 0) {
                    index = attr.indexOf('"',from);
                    if (index >= 0) {
                        ++index;
                        from = index;
                        index = attr.indexOf('"',from);
                        if (index > from) {
                            if (!values.isEmpty())
                                values.append(' ');
                            values += attr.mid(from,index-from);
                            from = index+1;
                        }
                    }
                }
                attr = values;
            }
        }
        if (inStartElement)
            xmlWriter().writeAttribute("outputclass", attr);
    }
        break;
    case Atom::DivRight:
        if ((currentTag() == DT_sectiondiv) || (currentTag() == DT_bodydiv)) {
            writeEndTag(); // </sectiondiv>, </bodydiv>, or </p>
            if (divNestingLevel > 0)
                --divNestingLevel;
        }
        break;
    case Atom::FootnoteLeft:
        // ### For now
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        xmlWriter().writeCharacters("<!-- ");
        break;
    case Atom::FootnoteRight:
        // ### For now
        xmlWriter().writeCharacters("-->");
        break;
    case Atom::FormatElse:
    case Atom::FormatEndif:
    case Atom::FormatIf:
        break;
    case Atom::FormattingLeft:
        {
            DitaTag t = DT_LAST;
            if (atom->string() == ATOM_FORMATTING_BOLD)
                t = DT_b;
            else if (atom->string() == ATOM_FORMATTING_PARAMETER)
                t = DT_i;
            else if (atom->string() == ATOM_FORMATTING_ITALIC)
                t = DT_i;
            else if (atom->string() == ATOM_FORMATTING_TELETYPE)
                t = DT_tt;
            else if (atom->string().startsWith("span ")) {
                t = DT_keyword;
            }
            else if (atom->string() == ATOM_FORMATTING_UICONTROL)
                t = DT_uicontrol;
            else if (atom->string() == ATOM_FORMATTING_UNDERLINE)
                t = DT_u;
            else if (atom->string() == ATOM_FORMATTING_INDEX)
                t = DT_comment;
            else if (atom->string() == ATOM_FORMATTING_SUBSCRIPT)
                t = DT_sub;
            else if (atom->string() == ATOM_FORMATTING_SUPERSCRIPT)
                t = DT_sup;
            else
                qDebug() << "DT_LAST";
            writeStartTag(t);
            if (atom->string() == ATOM_FORMATTING_PARAMETER) {
                if (atom->next() != 0 && atom->next()->type() == Atom::String) {
                    QRegExp subscriptRegExp("([a-z]+)_([0-9n])");
                    if (subscriptRegExp.exactMatch(atom->next()->string())) {
                        xmlWriter().writeCharacters(subscriptRegExp.cap(1));
                        writeStartTag(DT_sub);
                        xmlWriter().writeCharacters(subscriptRegExp.cap(2));
                        writeEndTag(); // </sub>
                        skipAhead = 1;
                    }
                }
            }
            else if (t == DT_keyword) {
                QString attr = atom->string().mid(5);
                if (!attr.isEmpty()) {
                    if (attr.contains('=')) {
                        int index = 0;
                        int from = 0;
                        QString values;
                        while (index >= 0) {
                            index = attr.indexOf('"',from);
                            if (index >= 0) {
                                ++index;
                                from = index;
                                index = attr.indexOf('"',from);
                                if (index > from) {
                                    if (!values.isEmpty())
                                        values.append(' ');
                                    values += attr.mid(from,index-from);
                                    from = index+1;
                                }
                            }
                        }
                        attr = values;
                    }
                }
                xmlWriter().writeAttribute("outputclass", attr);
            }
        }
        break;
    case Atom::FormattingRight:
        if (atom->string() == ATOM_FORMATTING_LINK) {
            endLink();
        }
        else {
            writeEndTag(); // ?
        }
        break;
    case Atom::AnnotatedList:
        {
            DocNode* dn = qdb_->getGroup(atom->string());
            if (dn)
                generateAnnotatedList(relative, marker, dn->members());
        }
        break;
    case Atom::GeneratedList:
        if (atom->string() == "annotatedclasses") {
            generateAnnotatedList(relative, marker, qdb_->getCppClasses());
        }
        else if (atom->string() == "classes") {
            generateCompactList(Generic, relative, qdb_->getCppClasses(), true, QStringLiteral("Q"));
        }
        else if (atom->string() == "qmlclasses") {
            generateCompactList(Generic, relative, qdb_->getQmlTypes(), true, QStringLiteral(""));
        }
        else if (atom->string().contains("classesbymodule")) {
            QString arg = atom->string().trimmed();
            QString moduleName = atom->string().mid(atom->string().indexOf("classesbymodule") + 15).trimmed();
            QDocDatabase* qdb = QDocDatabase::qdocDB();
            DocNode* dn = qdb->findModule(moduleName);
            if (dn) {
                NodeMap m;
                dn->getMemberClasses(m);
                if (!m.isEmpty()) {
                    generateAnnotatedList(relative, marker, m);
                }
            }
        }
        else if (atom->string() == "classhierarchy") {
            generateClassHierarchy(relative, qdb_->getCppClasses());
        }
        else if (atom->string() == "compatclasses") {
            // "compatclasses" is no longer used. Delete this at some point.
            // mws 03/10/2013
            generateCompactList(Generic, relative, qdb_->getCompatibilityClasses(), false, QStringLiteral("Q"));
        }
        else if (atom->string() == "obsoleteclasses") {
            generateCompactList(Generic, relative, qdb_->getObsoleteClasses(), false, QStringLiteral("Q"));
        }
        else if (atom->string() == "obsoleteqmltypes") {
            generateCompactList(Generic, relative, qdb_->getObsoleteQmlTypes(), false, QStringLiteral(""));
        }
        else if (atom->string() == "obsoletecppmembers") {
            generateCompactList(Obsolete, relative, qdb_->getClassesWithObsoleteMembers(), false, QStringLiteral("Q"));
        }
        else if (atom->string() == "obsoleteqmlmembers") {
            generateCompactList(Obsolete, relative, qdb_->getQmlTypesWithObsoleteMembers(), false, QStringLiteral(""));
        }
        else if (atom->string() == "functionindex") {
            generateFunctionIndex(relative);
        }
        else if (atom->string() == "legalese") {
            generateLegaleseList(relative, marker);
        }
        else if (atom->string() == "mainclasses") {
            // "mainclasses" is no longer used. Delete this at some point.
            // mws 03/10/2013
            generateCompactList(Generic, relative, qdb_->getMainClasses(), true, QStringLiteral("Q"));
        }
        else if (atom->string() == "services") {
            // "services" is no longer used. Delete this at some point.
            // mws 03/10/2013
            generateCompactList(Generic, relative, qdb_->getServiceClasses(), false, QStringLiteral("Q"));
        }
        else if (atom->string() == "overviews") {
            generateOverviewList(relative);
        }
        else if (atom->string() == "namespaces") {
            generateAnnotatedList(relative, marker, qdb_->getNamespaces());
        }
        else if (atom->string() == "related") {
            const DocNode *dn = static_cast<const DocNode *>(relative);
            if (dn)
                generateAnnotatedList(dn, marker, dn->members());
        }
        break;
    case Atom::SinceList:
    {
        const NodeMultiMap& nsmap = qdb_->getSinceMap(atom->string());
        const NodeMap& ncmap = qdb_->getClassMap(atom->string());
        const NodeMap& nqcmap = qdb_->getQmlTypeMap(atom->string());
        if (!nsmap.isEmpty()) {
            QList<Section> sections;
            QList<Section>::ConstIterator s;

            for (int i=0; i<LastSinceType; ++i)
                sections.append(Section(sinceTitle(i),QString(),QString(),QString()));

            NodeMultiMap::const_iterator n = nsmap.constBegin();
            while (n != nsmap.constEnd()) {
                const Node* node = n.value();
                switch (node->type()) {
                case Node::Document:
                    if (node->subType() == Node::QmlClass) {
                        sections[QmlClass].appendMember((Node*)node);
                    }
                    break;
                case Node::Namespace:
                    sections[Namespace].appendMember((Node*)node);
                    break;
                case Node::Class:
                    sections[Class].appendMember((Node*)node);
                    break;
                case Node::Enum:
                    sections[Enum].appendMember((Node*)node);
                    break;
                case Node::Typedef:
                    sections[Typedef].appendMember((Node*)node);
                    break;
                case Node::Function: {
                    const FunctionNode* fn = static_cast<const FunctionNode*>(node);
                    if (fn->isMacro())
                        sections[Macro].appendMember((Node*)node);
                    else {
                        Node* p = fn->parent();
                        if (p) {
                            if (p->type() == Node::Class)
                                sections[MemberFunction].appendMember((Node*)node);
                            else if (p->type() == Node::Namespace) {
                                if (p->name().isEmpty())
                                    sections[GlobalFunction].appendMember((Node*)node);
                                else
                                    sections[NamespaceFunction].appendMember((Node*)node);
                            }
                            else
                                sections[GlobalFunction].appendMember((Node*)node);
                        }
                        else
                            sections[GlobalFunction].appendMember((Node*)node);
                    }
                    break;
                }
                case Node::Property:
                    sections[Property].appendMember((Node*)node);
                    break;
                case Node::Variable:
                    sections[Variable].appendMember((Node*)node);
                    break;
                case Node::QmlProperty:
                    sections[QmlProperty].appendMember((Node*)node);
                    break;
                case Node::QmlSignal:
                    sections[QmlSignal].appendMember((Node*)node);
                    break;
                case Node::QmlSignalHandler:
                    sections[QmlSignalHandler].appendMember((Node*)node);
                    break;
                case Node::QmlMethod:
                    sections[QmlMethod].appendMember((Node*)node);
                    break;
                default:
                    break;
                }
                ++n;
            }

            writeStartTag(DT_ul);
            s = sections.constBegin();
            while (s != sections.constEnd()) {
                if (!(*s).members.isEmpty()) {
                    QString li = outFileName() + QLatin1Char('#') + Doc::canonicalTitle((*s).name);
                    writeXrefListItem(li, (*s).name);
                }
                ++s;
            }
            writeEndTag(); // </ul>

            int idx = 0;
            s = sections.constBegin();
            while (s != sections.constEnd()) {
                if (!(*s).members.isEmpty()) {
                    writeStartTag(DT_p);
                    writeGuidAttribute(Doc::canonicalTitle((*s).name));
                    xmlWriter().writeAttribute("outputclass","h3");
                    writeCharacters(protectEnc((*s).name));
                    writeEndTag(); // </p>
                    if (idx == Class)
                        generateCompactList(Generic, 0, ncmap, false, QString("Q"));
                    else if (idx == QmlClass)
                        generateCompactList(Generic, 0, nqcmap, false, QString("Q"));
                    else if (idx == MemberFunction) {
                        ParentMaps parentmaps;
                        ParentMaps::iterator pmap;
                        NodeList::const_iterator i = s->members.constBegin();
                        while (i != s->members.constEnd()) {
                            Node* p = (*i)->parent();
                            pmap = parentmaps.find(p);
                            if (pmap == parentmaps.end())
                                pmap = parentmaps.insert(p,NodeMultiMap());
                            pmap->insert((*i)->name(),(*i));
                            ++i;
                        }
                        pmap = parentmaps.begin();
                        while (pmap != parentmaps.end()) {
                            NodeList nlist = pmap->values();
                            writeStartTag(DT_p);
                            xmlWriter().writeCharacters("Class ");
                            writeStartTag(DT_xref);
                            // formathtml
                            xmlWriter().writeAttribute("href",linkForNode(pmap.key(), 0));
                            QStringList pieces = pmap.key()->fullName().split("::");
                            writeCharacters(protectEnc(pieces.last()));
                            writeEndTag(); // </xref>
                            xmlWriter().writeCharacters(":");
                            writeEndTag(); // </p>

                            generateSection(nlist, 0, marker, CodeMarker::Summary);
                            ++pmap;
                        }
                    }
                    else {
                        generateSection(s->members, 0, marker, CodeMarker::Summary);
                    }
                }
                ++idx;
                ++s;
            }
        }
    }
        break;
    case Atom::BR:
        // DITA XML can't do <br>
        break;
    case Atom::HR: //<p outputclass="horizontal-rule" />
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","horizontal-rule");
        writeEndTag(); // </p>
        break;
    case Atom::Image:
    case Atom::InlineImage:
    {
        QString fileName = imageFileName(relative, atom->string());
        QString text;
        if (atom->next() != 0)
            text = atom->next()->string();
        if (fileName.isEmpty()) {
            relative->location().warning(tr("Missing image: %1").arg(protectEnc(atom->string())));
            QString images = "images";
            if (!atom->string().isEmpty() && atom->string()[0] != '/')
                images.append(QLatin1Char('/'));
            fileName = images + atom->string();
        }
        if (relative && (relative->type() == Node::Document) &&
                (relative->subType() == Node::Example)) {
            const ExampleNode* cen = static_cast<const ExampleNode*>(relative);
            if (cen->imageFileName().isEmpty()) {
                ExampleNode* en = const_cast<ExampleNode*>(cen);
                en->setImageFileName(fileName);
            }
        }

        if (currentTag() != DT_xref && atom->type() != Atom::InlineImage)
            writeStartTag(DT_fig);
        writeStartTag(DT_image);
        writeHrefAttribute(protectEnc(fileName));
        if (atom->type() == Atom::Image) {
            xmlWriter().writeAttribute("placement","break");
            xmlWriter().writeAttribute("align","center");
        }
        if (!text.isEmpty()) {
            writeStartTag(DT_alt);
            writeCharacters(protectEnc(text));
            writeEndTag(); // </alt>
        }
        writeEndTag(); // </image>
        if (currentTag() != DT_xref && atom->type() != Atom::InlineImage)
            writeEndTag(); // </fig>
    }
        break;
    case Atom::ImageText:
        // nothing
        break;
    case Atom::ImportantLeft:
        writeStartTag(DT_note);
        xmlWriter().writeAttribute("type","important");
        break;
    case Atom::ImportantRight:
        writeEndTag(); // </note>
        break;
    case Atom::NoteLeft:
        writeStartTag(DT_note);
        xmlWriter().writeAttribute("type","note");
        break;
    case Atom::NoteRight:
        writeEndTag(); // </note>
        break;
    case Atom::LegaleseLeft:
        inLegaleseText = true;
        break;
    case Atom::LegaleseRight:
        inLegaleseText = false;
        break;
    case Atom::LineBreak:
        //xmlWriter().writeEmptyElement("br");
        break;
    case Atom::Link:
        {
            const Node *node = 0;
            QString myLink = getLink(atom, relative, &node);
            if (myLink.isEmpty())
                myLink = getCollisionLink(atom);
            if (myLink.isEmpty() && !noLinkErrors())
                relative->doc().location().warning(tr("Can't link to '%1'").arg(atom->string()));
            else if (!inSectionHeading_)
                beginLink(myLink);
            skipAhead = 1;
        }
        break;
    case Atom::GuidLink:
        {
            beginLink(atom->string());
            skipAhead = 1;
        }
        break;
    case Atom::LinkNode:
        {
            const Node* node = CodeMarker::nodeForString(atom->string());
            beginLink(linkForNode(node, relative));
            skipAhead = 1;
        }
        break;
    case Atom::ListLeft:
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        if (atom->string() == ATOM_LIST_BULLET) {
            writeStartTag(DT_ul);
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            writeStartTag(DT_dl);
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            threeColumnEnumValueTable_ = isThreeColumnEnumValueTable(atom);
            if (threeColumnEnumValueTable_) {
                writeStartTag(DT_simpletable);
                xmlWriter().writeAttribute("outputclass","valuelist");
                writeStartTag(DT_sthead);
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Constant");
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Value");
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Description");
                writeEndTag(); // </stentry>
                writeEndTag(); // </sthead>
            }
            else {
                writeStartTag(DT_simpletable);
                xmlWriter().writeAttribute("outputclass","valuelist");
                writeStartTag(DT_sthead);
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Constant");
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Value");
                writeEndTag(); // </stentry>
                writeEndTag(); // </sthead>
            }
        }
        else {
            writeStartTag(DT_ol);
            if (atom->string() == ATOM_LIST_UPPERALPHA)
                xmlWriter().writeAttribute("outputclass","upperalpha");
            else if (atom->string() == ATOM_LIST_LOWERALPHA)
                xmlWriter().writeAttribute("outputclass","loweralpha");
            else if (atom->string() == ATOM_LIST_UPPERROMAN)
                xmlWriter().writeAttribute("outputclass","upperroman");
            else if (atom->string() == ATOM_LIST_LOWERROMAN)
                xmlWriter().writeAttribute("outputclass","lowerroman");
            else // (atom->string() == ATOM_LIST_NUMERIC)
                xmlWriter().writeAttribute("outputclass","numeric");
            if (atom->next() != 0 && atom->next()->string().toInt() != 1) {
                /*
                  This attribute is not supported in DITA, and at the
                  moment, including it is causing a validation error
                  wherever it is used. I think it is only used in the
                  qdoc manual.
                 */
                //xmlWriter().writeAttribute("start",atom->next()->string());
            }
        }
        break;
    case Atom::ListItemNumber:
        // nothing
        break;
    case Atom::ListTagLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            writeStartTag(DT_dt);
        }
        else { // (atom->string() == ATOM_LIST_VALUE)
            writeStartTag(DT_strow);
            writeStartTag(DT_stentry);
            writeStartTag(DT_tt);
            writeCharacters(protectEnc(plainCode(marker->markedUpEnumValue(atom->next()->string(),
                                                                           relative))));
            writeEndTag(); // </tt>
            writeEndTag(); // </stentry>
            writeStartTag(DT_stentry);

            QString itemValue;
            if (relative->type() == Node::Enum) {
                const EnumNode *enume = static_cast<const EnumNode *>(relative);
                itemValue = enume->itemValue(atom->next()->string());
            }

            if (itemValue.isEmpty())
                xmlWriter().writeCharacters("?");
            else {
                writeStartTag(DT_tt);
                writeCharacters(protectEnc(itemValue));
                writeEndTag(); // </tt>
            }
            skipAhead = 1;
        }
        break;
    case Atom::ListTagRight:
        if (atom->string() == ATOM_LIST_TAG)
            writeEndTag(); // </dt>
        break;
    case Atom::ListItemLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            writeStartTag(DT_dd);
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            if (threeColumnEnumValueTable_) {
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
            }
        }
        else {
            writeStartTag(DT_li);
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::ListItemRight:
        if (atom->string() == ATOM_LIST_TAG) {
            writeEndTag(); // </dd>
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            writeEndTag(); // </stentry>
            writeEndTag(); // </strow>
        }
        else {
            writeEndTag(); // </li>
        }
        break;
    case Atom::ListRight:
        if (atom->string() == ATOM_LIST_BULLET) {
            writeEndTag(); // </ul>
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            writeEndTag(); // </dl>
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            writeEndTag(); // </simpletable>
        }
        else {
            writeEndTag(); // </ol>
        }
        break;
    case Atom::Nop:
        // nothing
        break;
    case Atom::ParaLeft:
        writeStartTag(DT_p);
        if (inLegaleseText)
            xmlWriter().writeAttribute("outputclass","legalese");
        in_para = true;
        break;
    case Atom::ParaRight:
        endLink();
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        break;
    case Atom::QuotationLeft:
        writeStartTag(DT_lq);
        break;
    case Atom::QuotationRight:
        writeEndTag(); // </lq>
        break;
    case Atom::RawString:
        if (atom->string() == " ")
            break;
        if (atom->string().startsWith(QLatin1Char('&')))
            writeCharacters(atom->string());
        else if (atom->string() == "<sup>*</sup>") {
            writeStartTag(DT_sup);
            writeCharacters("*");
            writeEndTag(); // </sup>
        }
        else if (atom->string() == "<sup>&reg;</sup>") {
            writeStartTag(DT_tm);
            xmlWriter().writeAttribute("tmtype","reg");
            writeEndTag(); // </tm>
        }
        else {
            writeStartTag(DT_pre);
            xmlWriter().writeAttribute("outputclass","raw-html");
            writeCharacters(atom->string());
            writeEndTag(); // </pre>
        }
        break;
    case Atom::SectionLeft:
        enterSection("details",QString());
        break;
    case Atom::SectionRight:
        leaveSection();
        break;
    case Atom::SectionHeadingLeft:
    {
        writeStartTag(DT_p);
        QString id = Text::sectionHeading(atom).toString();
        id = stripMarkup(id);
        id = Doc::canonicalTitle(id);
        writeGuidAttribute(id);
        hx = QLatin1Char('h') + QString::number(atom->string().toInt() + hOffset(relative));
        xmlWriter().writeAttribute("outputclass",hx);
        inSectionHeading_ = true;
    }
        break;
    case Atom::SectionHeadingRight:
        writeEndTag(); // </title> (see case Atom::SectionHeadingLeft)
        inSectionHeading_ = false;
        break;
    case Atom::SidebarLeft:
        // nothing
        break;
    case Atom::SidebarRight:
        // nothing
        break;
    case Atom::String:
        if (inLink_ && !inContents_ && !inSectionHeading_) {
            generateLink(atom, marker);
        }
        else {
            writeCharacters(atom->string());
        }
        break;
    case Atom::TableLeft:
    {
        QString attr;
        if ((atom->count() > 0) && (atom->string(0) == "borderless"))
            attr = "borderless";
        else if ((atom->count() > 1) && (atom->string(1) == "borderless"))
            attr = "borderless";
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        writeStartTag(DT_table);
        if (!attr.isEmpty())
            xmlWriter().writeAttribute("outputclass",attr);
        numTableRows_ = 0;
        if (tableColumnCount != 0) {
            qDebug() << "ERROR: Nested tables!";
            tableColumnCount = 0;
        }
        tableColumnCount = countTableColumns(atom->next());
        writeStartTag(DT_tgroup);
        xmlWriter().writeAttribute("cols",QString::number(tableColumnCount));
        for (int i = 0; i < tableColumnCount; i++) {
            writeStartTag(DT_colspec);
            xmlWriter().writeAttribute("colname", QStringLiteral("col%1").arg(i));
            xmlWriter().writeAttribute("colnum", QString::number(i));
            xmlWriter().writeAttribute("colwidth", QStringLiteral("1*"));
            writeEndTag(); // DT_colspec
        }
        inTableHeader_ = false;
        inTableBody = false;
    }
        break;
    case Atom::TableRight:
        writeEndTag(); // </tbody>
        writeEndTag(); // </tgroup>
        writeEndTag(); // </table>
        inTableHeader_ = false;
        inTableBody = false;
        tableColumnCount = 0;
        currentColumn = 0;
        break;
    case Atom::TableHeaderLeft:
        if (inTableBody) {
            writeEndTag(); // </tbody>
            writeEndTag(); // </tgroup>
            writeEndTag(); // </table>
            inTableHeader_ = false;
            inTableBody = false;
            tableColumnCount = 0;
            writeStartTag(DT_table);
            numTableRows_ = 0;
            tableColumnCount = countTableColumns(atom);
            writeStartTag(DT_tgroup);
            xmlWriter().writeAttribute("cols",QString::number(tableColumnCount));
        }
        currentColumn = 0;
        writeStartTag(DT_thead);
        xmlWriter().writeAttribute("valign","top");
        writeStartTag(DT_row);
        xmlWriter().writeAttribute("valign","top");
        inTableHeader_ = true;
        inTableBody = false;
        break;
    case Atom::TableHeaderRight:
        writeEndTag(); // </row>
        if (matchAhead(atom, Atom::TableHeaderLeft)) {
            skipAhead = 1;
            writeStartTag(DT_row);
            xmlWriter().writeAttribute("valign","top");
        }
        else {
            writeEndTag(); // </thead>
            inTableHeader_ = false;
            inTableBody = true;
            writeStartTag(DT_tbody);
        }
        break;
    case Atom::TableRowLeft:
        if (!inTableHeader_ && !inTableBody) {
            inTableBody = true;
            writeStartTag(DT_tbody);
        }
        currentColumn = 0;
        writeStartTag(DT_row);
        attr = atom->string();
        if (!attr.isEmpty()) {
            if (attr.contains('=')) {
                int index = 0;
                int from = 0;
                QString values;
                while (index >= 0) {
                    index = attr.indexOf('"',from);
                    if (index >= 0) {
                        ++index;
                        from = index;
                        index = attr.indexOf('"',from);
                        if (index > from) {
                            if (!values.isEmpty())
                                values.append(' ');
                            values += attr.mid(from,index-from);
                            from = index+1;
                        }
                    }
                }
                attr = values;
            }
            xmlWriter().writeAttribute("outputclass", attr);
        }
        xmlWriter().writeAttribute("valign","top");
        break;
    case Atom::TableRowRight:
        writeEndTag(); // </row>
        break;
    case Atom::TableItemLeft:
        {
            QString values;
            writeStartTag(DT_entry);
            for (int i=0; i<atom->count(); ++i) {
                attr = atom->string(i);
                if (attr.contains('=')) {
                    int index = 0;
                    int from = 0;
                    while (index >= 0) {
                        index = attr.indexOf('"',from);
                        if (index >= 0) {
                            ++index;
                            from = index;
                            index = attr.indexOf('"',from);
                            if (index > from) {
                                if (!values.isEmpty())
                                    values.append(' ');
                                values += attr.mid(from,index-from);
                                from = index+1;
                            }
                        }
                    }
                }
                else {
                    QStringList spans = attr.split(QLatin1Char(','));
                    if (spans.size() == 2) {
                        if (spans[0].toInt()>1) {
                            xmlWriter().writeAttribute("namest",QStringLiteral("col%1").arg(currentColumn));
                            xmlWriter().writeAttribute("nameend",QStringLiteral("col%1")
                                                       .arg(currentColumn + (spans[0].toInt() - 1)));
                        }
                        if (spans[1].toInt()>1)
                            xmlWriter().writeAttribute("morerows",spans[1].simplified());
                        currentColumn += spans[0].toInt();
                    }
                }
            }
            if (!values.isEmpty())
                xmlWriter().writeAttribute("outputclass",values);
            if (matchAhead(atom, Atom::ParaLeft))
                skipAhead = 1;
        }
        break;
    case Atom::TableItemRight:
        if (inTableHeader_) {
            writeEndTag(); // </entry>
        }
        else {
            writeEndTag(); // </entry>
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::TableOfContents:
        {
            int numColumns = 1;
            const Node* node = relative;

            Doc::Sections sectionUnit = Doc::Section4;
            QStringList params = atom->string().split(QLatin1Char(','));
            QString columnText = params.at(0);
            QStringList pieces = columnText.split(QLatin1Char(' '), QString::SkipEmptyParts);
            if (pieces.size() >= 2) {
                columnText = pieces.at(0);
                pieces.pop_front();
                QString path = pieces.join(' ').trimmed();
                node = qdb_->findNodeForTarget(path, relative);
                if (!node)
                    relative->doc().location().warning(tr("Cannot link to '%1'").arg(path));
           }

            if (params.size() == 2) {
                numColumns = qMax(columnText.toInt(), numColumns);
                sectionUnit = (Doc::Sections)params.at(1).toInt();
            }

            if (node)
                generateTableOfContents(node,
                                        marker,
                                        sectionUnit,
                                        numColumns,
                                        relative);
        }
        break;
    case Atom::Target:
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        writeStartTag(DT_p);
        writeGuidAttribute(Doc::canonicalTitle(atom->string()));
        xmlWriter().writeAttribute("outputclass","target");
        //xmlWriter().writeCharacters(protectEnc(atom->string()));
        writeEndTag(); // </p>
        break;
    case Atom::UnhandledFormat:
        writeStartTag(DT_b);
        xmlWriter().writeAttribute("outputclass","error");
        xmlWriter().writeCharacters("<Missing DITAXML>");
        writeEndTag(); // </b>
        break;
    case Atom::UnknownCommand:
        writeStartTag(DT_b);
        xmlWriter().writeAttribute("outputclass","error unknown-command");
        writeCharacters(protectEnc(atom->string()));
        writeEndTag(); // </b>
        break;
    case Atom::QmlText:
    case Atom::EndQmlText:
        // don't do anything with these. They are just tags.
        break;
    default:
        //        unknownAtom(atom);
        break;
    }
    return skipAhead;
}

/*!
  Generate a <cxxClass> element (and all the stuff inside it)
  for the C++ class represented by \a innerNode. \a marker is
  for marking up the code. I don't know what that means exactly.
 */
void
DitaXmlGenerator::generateClassLikeNode(InnerNode* inner, CodeMarker* marker)
{
    QList<Section>::ConstIterator s;

    QString title;
    QString rawTitle;
    QString fullTitle;
    if (inner->type() == Node::Namespace) {
        const NamespaceNode* nsn = const_cast<NamespaceNode*>(static_cast<const NamespaceNode*>(inner));
        rawTitle = inner->plainName();
        fullTitle = inner->plainFullName();
        title = rawTitle + " Namespace";

        /*
          Note: Because the C++ specialization we are using
          has no <cxxNamespace> element, we are using the
          <cxxClass> element with an outputclass attribute
          set to "namespace" .
         */
        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        writeStartTag(DT_cxxClassDefinition);
        writeLocation(nsn);
        writeEndTag(); // <cxxClassDefinition>

        enterDesc(DT_apiDesc,QString(),title);
        generateStatus(nsn, marker);
        generateThreadSafeness(nsn, marker);
        generateSince(nsn, marker);

        enterSection(QString(), QString());
        generateBody(nsn, marker);
        generateAlsoList(nsn, marker);
        leaveSection();
        leaveSection(); // </apiDesc>

        bool needOtherSection = false;
        QList<Section> summarySections;
        summarySections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.constBegin();
            while (s != summarySections.constEnd()) {
                if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
                    if (!s->inherited.isEmpty())
                        needOtherSection = true;
                }
                else {
                    QString attr;
                    if (!s->members.isEmpty()) {
                        writeStartTag(DT_p);
                        attr  = cleanRef((*s).name).toLower() + " h2";
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc((*s).name));
                        writeEndTag(); // </title>
                        generateSection(s->members, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner);
                    }
                    if (!s->reimpMembers.isEmpty()) {
                        QString name = QString("Reimplemented ") + (*s).name;
                        attr = cleanRef(name).toLower() + " h2";
                        writeStartTag(DT_p);
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc(name));
                        writeEndTag(); // </title>
                        generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner);
                    }
                }
                ++s;
            }
            if (needOtherSection) {
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","h3");
                xmlWriter().writeCharacters("Additional Inherited Members");
                writeEndTag(); // </title>
                s = summarySections.constBegin();
                while (s != summarySections.constEnd()) {
                    if (s->members.isEmpty())
                        generateSectionInheritedList(*s, inner);
                    ++s;
                }
            }
            leaveSection();
        }

        writeEndTag(); // </cxxClassDetail>

        // not included: <related-links>
        // not included: <cxxClassNested>

        QList<Section> detailSections;
        detailSections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
        s = detailSections.constBegin();
        while (s != detailSections.constEnd()) {
            if ((*s).name == "Classes") {
                writeNestedClasses((*s),nsn);
                break;
            }
            ++s;
        }

        s = detailSections.constBegin();
        while (s != detailSections.constEnd()) {
            if ((*s).name == "Function Documentation") {
                writeFunctions((*s),nsn,marker);
            }
            else if ((*s).name == "Type Documentation") {
                writeEnumerations((*s),marker);
                writeTypedefs((*s),marker);
            }
            else if ((*s).name == "Namespaces") {
                qDebug() << "Nested namespaces" << outFileName();
            }
            else if ((*s).name == "Macro Documentation") {
                //writeMacros((*s),marker);
            }
            ++s;
        }

        generateLowStatusMembers(inner,marker,CodeMarker::Obsolete);
        generateLowStatusMembers(inner,marker,CodeMarker::Compat);
        writeEndTag(); // </cxxClass>
    }
    else if (inner->type() == Node::Class) {
        const ClassNode* cn = const_cast<ClassNode*>(static_cast<const ClassNode*>(inner));
        rawTitle = inner->plainName();
        fullTitle = inner->plainFullName();
        title = rawTitle + " Class";

        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        writeStartTag(DT_cxxClassDefinition);
        writeStartTag(DT_cxxClassAccessSpecifier);
        xmlWriter().writeAttribute("value",inner->accessString());
        writeEndTag(); // <cxxClassAccessSpecifier>
        if (cn->isAbstract()) {
            writeStartTag(DT_cxxClassAbstract);
            xmlWriter().writeAttribute("name","abstract");
            xmlWriter().writeAttribute("value","abstract");
            writeEndTag(); // </cxxClassAbstract>
        }
        writeDerivations(cn); // <cxxClassDerivations>

        // not included: <cxxClassTemplateParameters>

        writeLocation(cn);
        writeEndTag(); // <cxxClassDefinition>

        enterDesc(DT_apiDesc,QString(),title);
        generateStatus(cn, marker);
        generateInherits(cn, marker);
        generateInheritedBy(cn, marker);
        generateThreadSafeness(cn, marker);
        generateSince(cn, marker);
        enterSection(QString(), QString());
        generateBody(cn, marker);
        generateAlsoList(cn, marker);
        leaveSection();
        leaveSection(); // </apiDesc>

        bool needOtherSection = false;
        QList<Section> summarySections;
        summarySections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.constBegin();
            while (s != summarySections.constEnd()) {
                if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
                    if (!s->inherited.isEmpty())
                        needOtherSection = true;
                }
                else {
                    QString attr;
                    if (!s->members.isEmpty()) {
                        writeStartTag(DT_p);
                        attr = cleanRef((*s).name).toLower() + " h2";
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc((*s).name));
                        writeEndTag(); // </p>
                        generateSection(s->members, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner);
                    }
                    if (!s->reimpMembers.isEmpty()) {
                        QString name = QString("Reimplemented ") + (*s).name;
                        attr = cleanRef(name).toLower() + " h2";
                        writeStartTag(DT_p);
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc(name));
                        writeEndTag(); // </p>
                        generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner);
                    }
                }
                ++s;
            }
            if (needOtherSection) {
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","h3");
                xmlWriter().writeCharacters("Additional Inherited Members");
                writeEndTag(); // </p>
                s = summarySections.constBegin();
                while (s != summarySections.constEnd()) {
                    if (s->members.isEmpty())
                        generateSectionInheritedList(*s, inner);
                    ++s;
                }
            }
            leaveSection();
        }

        // not included: <example> or <apiImpl>

        writeEndTag(); // </cxxClassDetail>

        // not included: <related-links>
        // not included: <cxxClassNested>

        QList<Section> detailSections;
        detailSections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
        s = detailSections.constBegin();
        while (s != detailSections.constEnd()) {
            if ((*s).name == "Member Function Documentation") {
                writeFunctions((*s),cn,marker);
            }
            else if ((*s).name == "Member Type Documentation") {
                writeEnumerations((*s),marker);
                writeTypedefs((*s),marker);
            }
            else if ((*s).name == "Member Variable Documentation") {
                writeDataMembers((*s),marker);
            }
            else if ((*s).name == "Property Documentation") {
                writeProperties((*s),marker);
            }
            else if ((*s).name == "Macro Documentation") {
                //writeMacros((*s),marker);
            }
            else if ((*s).name == "Related Non-Members") {
                QString attribute("related-non-member");
                writeFunctions((*s),cn,marker,attribute);
            }
            ++s;
        }

        generateLowStatusMembers(inner,marker,CodeMarker::Obsolete);
        generateLowStatusMembers(inner,marker,CodeMarker::Compat);
        writeEndTag(); // </cxxClass>
    }
    else if ((inner->type() == Node::Document) && (inner->subType() == Node::HeaderFile)) {
        const DocNode* dn = const_cast<DocNode*>(static_cast<const DocNode*>(inner));
        rawTitle = inner->plainName();
        fullTitle = inner->plainFullName();
        title = rawTitle;

        /*
          Note: Because the C++ specialization we are using
          has no <cxxHeaderFile> element, we are using the
          <cxxClass> element with an outputclass attribute
          set to "headerfile" .
         */
        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        enterDesc(DT_apiDesc,QString(),title);
        generateStatus(dn, marker);
        generateThreadSafeness(dn, marker);
        generateSince(dn, marker);
        generateSince(dn, marker);
        enterSection(QString(), QString());
        generateBody(dn, marker);
        generateAlsoList(dn, marker);
        leaveSection();
        leaveSection(); // </apiDesc>

        bool needOtherSection = false;
        QList<Section> summarySections;
        summarySections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.constBegin();
            while (s != summarySections.constEnd()) {
                if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
                    if (!s->inherited.isEmpty())
                        needOtherSection = true;
                }
                else {
                    QString attr;
                    if (!s->members.isEmpty()) {
                        writeStartTag(DT_p);
                        attr = cleanRef((*s).name).toLower() + " h2";
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc((*s).name));
                        writeEndTag(); // </p>
                        generateSection(s->members, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner);
                    }
                    if (!s->reimpMembers.isEmpty()) {
                        QString name = QString("Reimplemented ") + (*s).name;
                        attr = cleanRef(name).toLower() + " h2";
                        writeStartTag(DT_p);
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc(name));
                        writeEndTag(); // </p>
                        generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner);
                    }
                }
                ++s;
            }
            if (needOtherSection) {
                enterSection("additional-inherited-members redundant",QString());
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","h3");
                xmlWriter().writeCharacters("Additional Inherited Members");
                writeEndTag(); // </p>
                s = summarySections.constBegin();
                while (s != summarySections.constEnd()) {
                    if (s->members.isEmpty())
                        generateSectionInheritedList(*s, inner);
                    ++s;
                }
            }
            leaveSection();
        }

        writeEndTag(); // </cxxClassDetail>

        // not included: <related-links>
        // not included: <cxxClassNested>

        QList<Section> detailSections;
        detailSections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
        s = detailSections.constBegin();
        while (s != detailSections.constEnd()) {
            if ((*s).name == "Classes") {
                writeNestedClasses((*s),dn);
                break;
            }
            ++s;
        }

        s = detailSections.constBegin();
        while (s != detailSections.constEnd()) {
            if ((*s).name == "Function Documentation") {
                writeFunctions((*s),dn,marker);
            }
            else if ((*s).name == "Type Documentation") {
                writeEnumerations((*s),marker);
                writeTypedefs((*s),marker);
            }
            else if ((*s).name == "Namespaces") {
                qDebug() << "Nested namespaces" << outFileName();
            }
            else if ((*s).name == "Macro Documentation") {
                //writeMacros((*s),marker);
            }
            ++s;
        }
        generateLowStatusMembers(inner,marker,CodeMarker::Obsolete);
        generateLowStatusMembers(inner,marker,CodeMarker::Compat);
        writeEndTag(); // </cxxClass>
    }
    else if ((inner->type() == Node::Document) && (inner->subType() == Node::QmlClass)) {
        QmlClassNode* qcn = const_cast<QmlClassNode*>(static_cast<const QmlClassNode*>(inner));
        ClassNode* cn = qcn->classNode();
        rawTitle = inner->plainName();
        fullTitle = inner->plainFullName();
        title = rawTitle + " Type";
        Node::clearPropertyGroupCount();

        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_qmlTypeDetail);
        generateQmlModuleDef(qcn);
        generateQmlInherits(qcn,marker);
        generateQmlInheritedBy(qcn, marker);
        generateQmlInstantiates(qcn,marker);
        generateQmlSince(qcn);

        enterDesc(DT_apiDesc,QString(),title);
        enterSection(QString(), QString());
        generateBody(qcn, marker);
        if (cn) {
            generateQmlText(cn->doc().body(), cn, marker, qcn->name());
            generateAlsoList(cn, marker);
        }
        leaveSection();
        leaveSection(); // </apiDesc>
        writeEndTag(); // </qmlTypeDetail>

        QList<Section> members = marker->qmlSections(qcn,CodeMarker::Detailed);
        if (!members.isEmpty()) {
            s = members.constBegin();
            while (s != members.constEnd()) {
                if (!s->members.isEmpty()) {
                    NodeList::ConstIterator m = (*s).members.constBegin();
                    while (m != (*s).members.constEnd()) {
                        generateDetailedQmlMember(*m, qcn, marker);
                        ++m;
                    }
                }
                ++s;
            }
        }
        writeEndTag(); // </apiRef>
    }
}

/*!
  Write a list item for a \a link with the given \a text.
 */
void DitaXmlGenerator::writeXrefListItem(const QString& link, const QString& text)
{
    writeStartTag(DT_li);
    writeStartTag(DT_xref);
    // formathtml
    writeHrefAttribute(link);
    writeCharacters(text);
    writeEndTag(); // </xref>
    writeEndTag(); // </li>
}

/*!
  Generate the DITA page for a qdoc file that doesn't map
  to an underlying c++ file.
 */
void DitaXmlGenerator::generateDocNode(DocNode* dn, CodeMarker* marker)
{
    /*
      If the dn node is a page node, and if the page type
      is DITA map page, write the node's contents as a dita
      map and return without doing anything else.
     */
    if (dn->subType() == Node::Page && dn->pageType() == Node::DitaMapPage) {
        const DitaMapNode* dmn = static_cast<const DitaMapNode*>(dn);
        writeDitaMap(dmn);
        return;
    }

    QList<Section> sections;
    QList<Section>::const_iterator s;
    QString fullTitle = dn->fullTitle();

    if (dn->subType() == Node::QmlBasicType) {
        fullTitle = "QML Basic Type: " + fullTitle;
    }
    else if (dn->subType() == Node::Collision) {
        fullTitle = "Name Collision: " + fullTitle;
    }

    generateHeader(dn, fullTitle);
    generateBrief(dn, marker); // <shortdesc>
    writeProlog(dn);

    writeStartTag(DT_body);
    enterSection(QString(), QString());
    if (dn->subType() == Node::Module) {
        generateStatus(dn, marker);
        NodeMap nm;
        dn->getMemberNamespaces(nm);
        if (!nm.isEmpty()) {
            enterSection("h2","Namespaces");
            generateAnnotatedList(dn, marker, nm);
            leaveSection();
        }
        nm.clear();
        dn->getMemberClasses(nm);
        if (!nm.isEmpty()) {
            enterSection("h2","Classes");
            generateAnnotatedList(dn, marker, nm);
            leaveSection();
        }
        nm.clear();
    }

    if (dn->doc().isEmpty()) {
        if (dn->subType() == Node::File) {
            Text text;
            Quoter quoter;
            writeStartTag(DT_p);
            xmlWriter().writeAttribute("outputclass", "small-subtitle");
            text << dn->subTitle();
            generateText(text, dn, marker);
            writeEndTag(); // </p>
            Doc::quoteFromFile(dn->doc().location(), quoter, dn->name());
            QString code = quoter.quoteTo(dn->location(), QString(), QString());
            text.clear();
            text << Atom(Atom::Code, code);
            generateText(text, dn, marker);
        }
    }
    else {
        if (dn->subType() == Node::Module) {
            enterSection(QString(), QString());
            generateBody(dn, marker);
            leaveSection();
        }
        else {
            generateBody(dn, marker);
        }
        generateAlsoList(dn, marker);
        generateAnnotatedList(dn, marker, dn->members());
    }
    leaveSection(); // </section>
    if (!writeEndTag()) { // </body>
        dn->doc().location().warning(tr("Pop of empty XML tag stack; generating DITA for '%1'").arg(dn->name()));
        return;
    }
    writeRelatedLinks(dn);
    writeEndTag(); // </topic>
}

/*!
  This function writes a \e{<link>} element inside a
  \e{<related-links>} element.

  \sa writeRelatedLinks()
 */
void DitaXmlGenerator::writeLink(const Node* node,
                                 const QString& text,
                                 const QString& role)
{
    if (node) {
        QString link = fileName(node) + QLatin1Char('#') + node->guid();
        if (link.endsWith(QLatin1Char('#')))
            qDebug() << "LINK ENDS WITH #:" << link << outFileName();
        writeStartTag(DT_link);
        writeHrefAttribute(link);
        xmlWriter().writeAttribute("role", role);
        writeStartTag(DT_linktext);
        writeCharacters(text);
        writeEndTag(); // </linktext>
        writeEndTag(); // </link>
    }
}

/*!
  This function writes a \e{<related-links>} element, which
  contains the \c{next}, \c{previous}, and \c{start}
  links for topic pages that have them. Note that the
  value of the \e role attribute is \c{parent} for the
  \c{start} link.
 */
void DitaXmlGenerator::writeRelatedLinks(const DocNode* node)
{
    const Node* linkNode = 0;
    QPair<QString,QString> linkPair;
    if (node && !node->links().empty()) {
        writeStartTag(DT_relatedLinks);
        if (node->links().contains(Node::PreviousLink)) {
            linkPair = node->links()[Node::PreviousLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (linkNode && linkNode->type() == Node::Document) {
                const DocNode *docNode = static_cast<const DocNode*>(linkNode);
                linkPair.second = docNode->title();
            }
            writeLink(linkNode, linkPair.second, "previous");
        }
        if (node->links().contains(Node::NextLink)) {
            linkPair = node->links()[Node::NextLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (linkNode && linkNode->type() == Node::Document) {
                const DocNode *docNode = static_cast<const DocNode*>(linkNode);
                linkPair.second = docNode->title();
            }
            writeLink(linkNode, linkPair.second, "next");
        }
        if (node->links().contains(Node::StartLink)) {
            linkPair = node->links()[Node::StartLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (linkNode && linkNode->type() == Node::Document) {
                const DocNode *docNode = static_cast<const DocNode*>(linkNode);
                linkPair.second = docNode->title();
            }
            writeLink(linkNode, linkPair.second, "parent");
        }
        writeEndTag(); // </related-links>
    }
}

/*!
  Returns "dita" for this subclass of class Generator.
 */
QString DitaXmlGenerator::fileExtension() const
{
    return "dita";
}

/*!
  Writes an XML file header to the current XML stream. This
  depends on which kind of DITA XML file is being generated,
  which is determined by the \a node type and subtype and the
  \a subpage flag. If the \subpage flag is true, a \c{<topic>}
  header is written, regardless of the type of \a node.
 */
void DitaXmlGenerator::generateHeader(const Node* node,
                                      const QString& name,
                                      bool subpage)
{
    if (!node)
        return;

    DitaTag mainTag = DT_cxxClass;
    DitaTag nameTag = DT_apiName;
    QString doctype;
    QString dtd;
    QString base;
    QString version;
    QString outputclass;

    if (node->type() == Node::Class) {
        mainTag = DT_cxxClass;
        nameTag = DT_apiName;
        dtd = "dtd/cxxClass.dtd";
        version = "0.7.0";
        doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                version + "//EN\" \"" + dtd + "\">";
    }
    else if (node->type() == Node::Namespace) {
        mainTag = DT_cxxClass;
        nameTag = DT_apiName;
        dtd = "dtd/cxxClass.dtd";
        version = "0.7.0";
        doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                version + "//EN\" \"" + dtd + "\">";
        outputclass = "namespace";
    }
    else if (node->type() == Node::Document || subpage) {
        if (node->subType() == Node::HeaderFile) {
            mainTag = DT_cxxClass;
            nameTag = DT_apiName;
            dtd = "dtd/cxxClass.dtd";
            version = "0.7.0";
            doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                    " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                    version + "//EN\" \"" + dtd + "\">";
            outputclass = "headerfile";
        }
        else if (node->subType() == Node::QmlClass) {
            mainTag = DT_qmlType;
            nameTag = DT_apiName;
            dtd = "dtd/qmlType.dtd";
            version = "0.1.0";
            doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                    " PUBLIC \"-//NOKIA//DTD DITA QML Type" +
                    "//EN\" \"" + dtd + "\">";
            outputclass = "QML-type";
        }
        else {
            mainTag = DT_topic;
            nameTag = DT_title;
            dtd = "dtd/topic.dtd";
            doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                    " PUBLIC \"-//OASIS//DTD DITA Topic//EN\" \"" + dtd + "\">";
            switch (node->subType()) {
            case Node::Page:
                outputclass = node->pageTypeString();
                break;
            case Node::Group:
                outputclass = "group";
                break;
            case Node::Example:
                outputclass = "example";
                break;
            case Node::File:
                outputclass = "file";
                break;
            case Node::Image:  // not used
                outputclass = "image";
                break;
            case Node::Module:
                outputclass = "module";
                break;
            case Node::ExternalPage: // not used
                outputclass = "externalpage";
                break;
            case Node::Collision:
                outputclass = "collision";
                break;
            default:
                outputclass = "page";
            }
        }
    }

    xmlWriter().writeDTD(doctype);
    xmlWriter().writeComment(node->doc().location().fileName());
    writeStartTag(mainTag);
    QString id = node->guid();
    xmlWriter().writeAttribute("id",id);
    if (!outputclass.isEmpty())
        xmlWriter().writeAttribute("outputclass",outputclass);
    writeStartTag(nameTag); // <title> or <apiName>
    if (!name.isEmpty())
        writeCharacters(name);
    else
        writeCharacters(node->name());
    writeEndTag(); // </title> or </apiName>
}

/*!
  Outputs the \e brief command as a <shortdesc> element.
 */
void DitaXmlGenerator::generateBrief(const Node* node, CodeMarker* marker)
{
    Text brief = node->doc().briefText(true); // zzz
    if (!brief.isEmpty()) {
        generateText(brief, node, marker);
    }
}

/*!
  zzz
  Generates a table of contents beginning at \a node.
  Currently just returns without writing anything.
 */
void DitaXmlGenerator::generateTableOfContents(const Node* node,
                                               CodeMarker* marker,
                                               Doc::Sections sectionUnit,
                                               int numColumns,
                                               const Node* relative)

{
    return;
    if (!node->doc().hasTableOfContents())
        return;
    QList<Atom *> toc = node->doc().tableOfContents();
    if (toc.isEmpty())
        return;

    QString nodeName;
    if (node != relative)
        nodeName = node->name();

    QStringList sectionNumber;
    int columnSize = 0;

    QString tdTag;
    if (numColumns > 1) {
        tdTag = "<td>"; /* width=\"" + QString::number((100 + numColumns - 1) / numColumns) + "%\">";*/
        out() << "<table class=\"toc\">\n<tr class=\"topAlign\">"
              << tdTag << '\n';
    }

    // disable nested links in table of contents
    inContents_ = true;
    inLink_ = true;

    for (int i = 0; i < toc.size(); ++i) {
        Atom *atom = toc.at(i);

        int nextLevel = atom->string().toInt();
        if (nextLevel > (int)sectionUnit)
            continue;

        if (sectionNumber.size() < nextLevel) {
            do {
                out() << "<ul>";
                sectionNumber.append("1");
            } while (sectionNumber.size() < nextLevel);
        }
        else {
            while (sectionNumber.size() > nextLevel) {
                out() << "</ul>\n";
                sectionNumber.removeLast();
            }
            sectionNumber.last() = QString::number(sectionNumber.last().toInt() + 1);
        }
        int numAtoms;
        Text headingText = Text::sectionHeading(atom);

        if (sectionNumber.size() == 1 && columnSize > toc.size() / numColumns) {
            out() << "</ul></td>" << tdTag << "<ul>\n";
            columnSize = 0;
        }
        out() << "<li>";
        out() << "<xref href=\""
              << nodeName
              << '#'
              << Doc::canonicalTitle(headingText.toString())
              << "\">";
        generateAtomList(headingText.firstAtom(), node, marker, true, numAtoms);
        out() << "</xref></li>\n";

        ++columnSize;
    }
    while (!sectionNumber.isEmpty()) {
        out() << "</ul>\n";
        sectionNumber.removeLast();
    }

    if (numColumns > 1)
        out() << "</td></tr></table>\n";

    inContents_ = false;
    inLink_ = false;
}

void DitaXmlGenerator::generateLowStatusMembers(InnerNode* inner,
                                                CodeMarker* marker,
                                                CodeMarker::Status status)
{
    QString attribute;
    if (status == CodeMarker::Compat)
        attribute = "Qt3-support";
    else if (status == CodeMarker::Obsolete)
        attribute = "obsolete";
    else
        return;

    QList<Section> sections = marker->sections(inner, CodeMarker::Detailed, status);
    QMutableListIterator<Section> j(sections);
    while (j.hasNext()) {
        if (j.next().members.size() == 0)
            j.remove();
    }
    if (sections.isEmpty())
        return;

    if (status == CodeMarker::Obsolete)
        inner->setObsoleteLink(fileBase(inner) + "-obsolete." + fileExtension());

    QList<Section>::ConstIterator s = sections.constBegin();
    while (s != sections.constEnd()) {
        if ((*s).name == "Member Function Documentation") {
            writeFunctions((*s),inner,marker,attribute);
        }
        else if ((*s).name == "Member Type Documentation") {
            writeEnumerations((*s),marker,attribute);
            writeTypedefs((*s),marker,attribute);
        }
        else if ((*s).name == "Member Variable Documentation") {
            writeDataMembers((*s),marker,attribute);
        }
        else if ((*s).name == "Property Documentation") {
            writeProperties((*s),marker,attribute);
        }
        else if ((*s).name == "Macro Documentation") {
            //writeMacros((*s),marker,attribute);
        }
        ++s;
    }
}

/*!
  Write the XML for the class hierarchy to the current XML stream.
 */
void DitaXmlGenerator::generateClassHierarchy(const Node* relative, NodeMap& classMap)
{
    if (classMap.isEmpty())
        return;

    NodeMap topLevel;
    NodeMap::Iterator c = classMap.begin();
    while (c != classMap.end()) {
        ClassNode* classe = static_cast<ClassNode*>(*c);
        if (classe->baseClasses().isEmpty())
            topLevel.insert(classe->name(), classe);
        ++c;
    }

    QStack<NodeMap > stack;
    stack.push(topLevel);

    writeStartTag(DT_ul);
    while (!stack.isEmpty()) {
        if (stack.top().isEmpty()) {
            stack.pop();
            writeEndTag(); // </ul>
            if (!stack.isEmpty())
                writeEndTag(); // </li>
        }
        else {
            ClassNode* child = static_cast<ClassNode*>(*stack.top().begin());
            writeStartTag(DT_li);
            generateFullName(child, relative);
            writeEndTag(); // </li>
            stack.top().erase(stack.top().begin());

            NodeMap newTop;
            foreach (const RelatedClass &d, child->derivedClasses()) {
                if (d.access != Node::Private && !d.node->doc().isEmpty())
                    newTop.insert(d.node->name(), d.node);
            }
            if (!newTop.isEmpty()) {
                stack.push(newTop);
                writeStartTag(DT_li);
                writeStartTag(DT_ul);
            }
        }
    }
}

/*!
  Output an annotated list of the nodes in \a nodeMap.
  A two-column table is output.
 */
void DitaXmlGenerator::generateAnnotatedList(const Node* relative,
                                             CodeMarker* marker,
                                             const NodeMap& nodeMap)
{
    if (nodeMap.isEmpty())
        return;
    NodeList nl;
    NodeMap::const_iterator i = nodeMap.begin();
    while (i != nodeMap.end()) {
        nl.append(i.value());
        ++i;
    }
    generateAnnotatedList(relative, marker, nl);
}

/*!
  Write XML for the contents of the \a nodes to the current
  XML stream.
 */
void DitaXmlGenerator::generateAnnotatedList(const Node* relative,
                                             CodeMarker* marker,
                                             const NodeList& nodes)
{
    if (nodes.isEmpty())
        return;
    bool allInternal = true;
    foreach (const Node* node, nodes) {
        if (!node->isInternal() && node->status() != Node::Obsolete) {
            allInternal = false;
        }
    }
    if (allInternal)
        return;

    writeStartTag(DT_table);
    xmlWriter().writeAttribute("outputclass","annotated");
    writeStartTag(DT_tgroup);
    xmlWriter().writeAttribute("cols","2");
    writeStartTag(DT_tbody);

    foreach (const Node* node, nodes) {
        if (node->isInternal() || node->status() == Node::Obsolete)
            continue;

        writeStartTag(DT_row);
        writeStartTag(DT_entry);
        writeStartTag(DT_p);
        generateFullName(node, relative);
        writeEndTag(); // </p>
        writeEndTag(); // <entry>

        if (!(node->type() == Node::Document)) {
            Text brief = node->doc().trimmedBriefText(node->name());
            if (!brief.isEmpty()) {
                writeStartTag(DT_entry);
                writeStartTag(DT_p);
                generateText(brief, node, marker);
                writeEndTag(); // </p>
                writeEndTag(); // <entry>
            }
            else if (!node->reconstitutedBrief().isEmpty()) {
                writeStartTag(DT_entry);
                writeStartTag(DT_p);
                writeCharacters(node->reconstitutedBrief());
                writeEndTag(); // </p>
                writeEndTag(); // <entry>
            }
        }
        else {
            writeStartTag(DT_entry);
            writeStartTag(DT_p);
            if (!node->reconstitutedBrief().isEmpty()) {
                writeCharacters(node->reconstitutedBrief());
            }
            else
                writeCharacters(protectEnc(node->doc().briefText().toString()));
            writeEndTag(); // </p>
            writeEndTag(); // <entry>
        }
        writeEndTag(); // </row>
    }
    writeEndTag(); // </tbody>
    writeEndTag(); // </tgroup>
    writeEndTag(); // </table>
}

/*!
  This function finds the common prefix of the names of all
  the classes in \a classMap and then generates a compact
  list of the class names alphabetized on the part of the
  name not including the common prefix. You can tell the
  function to use \a comonPrefix as the common prefix, but
  normally you let it figure it out itself by looking at
  the name of the first and last classes in \a classMap.
 */
void DitaXmlGenerator::generateCompactList(ListType , // currently not needed for DITA
                                           const Node* relative,
                                           const NodeMap& classMap,
                                           bool includeAlphabet,
                                           QString commonPrefix)
{
    const int NumParagraphs = 37; // '0' to '9', 'A' to 'Z', '_'

    if (classMap.isEmpty())
        return;

    /*
      If commonPrefix is not empty, then the caller knows what
      the common prefix is and has passed it in, so just use that
      one. But if the commonPrefix is empty (it normally is), then
      compute a common prefix using this simple algorithm. Note we
      assume the prefix length is 1, i.e. we will have a single
      character as the common prefix.
     */
    int commonPrefixLen = commonPrefix.length();
    if (commonPrefixLen == 0) {
        QVector<int> count(26);
        for (int i=0; i<26; ++i)
            count[i] = 0;

        NodeMap::const_iterator iter = classMap.constBegin();
        while (iter != classMap.constEnd()) {
            if (!iter.key().contains("::")) {
                QChar c = iter.key()[0];
                if ((c >= 'A') && (c <= 'Z')) {
                    int idx = c.unicode() - QChar('A').unicode();
                    ++count[idx];
                }
            }
            ++iter;
        }
        int highest = 0;
        int idx = -1;
        for (int i=0; i<26; ++i) {
            if (count[i] > highest) {
                highest = count[i];
                idx = i;
            }
        }
        idx += QChar('A').unicode();
        QChar common(idx);
        commonPrefix = common;
        commonPrefixLen = 1;
    }

    /*
      Divide the data into 37 paragraphs: 0, ..., 9, A, ..., Z,
      underscore (_). QAccel will fall in paragraph 10 (A) and
      QXtWidget in paragraph 33 (X). This is the only place where we
      assume that NumParagraphs is 37. Each paragraph is a NodeMap.
    */
    NodeMap paragraph[NumParagraphs+1];
    QString paragraphName[NumParagraphs+1];
    QSet<char> usedParagraphNames;

    NodeMap::ConstIterator c = classMap.constBegin();
    while (c != classMap.constEnd()) {
        QStringList pieces = c.key().split("::");
        QString key;
        int idx = commonPrefixLen;
        if (!pieces.last().startsWith(commonPrefix))
            idx = 0;
        if (pieces.size() == 1)
            key = pieces.last().mid(idx).toLower();
        else
            key = pieces.last().toLower();

        int paragraphNr = NumParagraphs - 1;

        if (key[0].digitValue() != -1) {
            paragraphNr = key[0].digitValue();
        }
        else if (key[0] >= QLatin1Char('a') && key[0] <= QLatin1Char('z')) {
            paragraphNr = 10 + key[0].unicode() - 'a';
        }

        paragraphName[paragraphNr] = key[0].toUpper();
        usedParagraphNames.insert(key[0].toLower().cell());
        paragraph[paragraphNr].insert(key, c.value());
        ++c;
    }

    /*
      Each paragraph j has a size: paragraph[j].count(). In the
      discussion, we will assume paragraphs 0 to 5 will have sizes
      3, 1, 4, 1, 5, 9.

      We now want to compute the paragraph offset. Paragraphs 0 to 6
      start at offsets 0, 3, 4, 8, 9, 14, 23.
    */
    int paragraphOffset[NumParagraphs + 1];     // 37 + 1
    paragraphOffset[0] = 0;
    for (int i=0; i<NumParagraphs; i++)         // i = 0..36
        paragraphOffset[i+1] = paragraphOffset[i] + paragraph[i].count();

    int curParNr = 0;
    int curParOffset = 0;
    QMap<QChar,QString> cmap;

    /*
      Output the alphabet as a row of links.
     */
    if (includeAlphabet) {
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","alphabet");
        for (int i = 0; i < 26; i++) {
            QChar ch('a' + i);
            if (usedParagraphNames.contains(char('a' + i))) {
                writeStartTag(DT_xref);
                // formathtml
                QString guid = lookupGuid(outFileName(),QString(ch));
                QString attr = outFileName() + QString("#%1").arg(guid);
                xmlWriter().writeAttribute("href", attr);
                xmlWriter().writeCharacters(QString(ch.toUpper()));
                writeEndTag(); // </xref>
            }
        }
        writeEndTag(); // </p>
    }

    /*
      Output a <p> element to contain all the <dl> elements.
     */
    writeStartTag(DT_p);
    xmlWriter().writeAttribute("outputclass","compactlist");

    for (int i=0; i<classMap.count()-1; i++) {
        while ((curParNr < NumParagraphs) &&
               (curParOffset == paragraph[curParNr].count())) {
            ++curParNr;
            curParOffset = 0;
        }

        /*
          Starting a new paragraph means starting a new <dl>.
        */
        if (curParOffset == 0) {
            if (i > 0) {
                writeEndTag(); // </dlentry>
                writeEndTag(); // </dl>
            }
            writeStartTag(DT_dl);
            writeStartTag(DT_dlentry);
            writeStartTag(DT_dt);
            if (includeAlphabet) {
                QChar c = paragraphName[curParNr][0].toLower();
                writeGuidAttribute(QString(c));
            }
            xmlWriter().writeAttribute("outputclass","sublist-header");
            xmlWriter().writeCharacters(paragraphName[curParNr]);
            writeEndTag(); // </dt>
        }

        /*
          Output a <dd> for the current offset in the current paragraph.
         */
        writeStartTag(DT_dd);
        if ((curParNr < NumParagraphs) &&
                !paragraphName[curParNr].isEmpty()) {
            NodeMap::Iterator it;
            it = paragraph[curParNr].begin();
            for (int i=0; i<curParOffset; i++)
                ++it;

            /*
              Previously, we used generateFullName() for this, but we
              require some special formatting.
            */
            writeStartTag(DT_xref);
            // formathtml
            writeHrefAttribute(linkForNode(it.value(), relative));

            QStringList pieces;
            if (it.value()->subType() == Node::QmlClass)
                pieces << it.value()->name();
            else
                pieces = it.value()->fullName(relative).split("::");
            xmlWriter().writeCharacters(protectEnc(pieces.last()));
            writeEndTag(); // </xref>
            if (pieces.size() > 1) {
                xmlWriter().writeCharacters(" (");
                generateFullName(it.value()->parent(),relative);
                xmlWriter().writeCharacters(")");
            }
        }
        writeEndTag(); // </dd>
        curParOffset++;
    }
    writeEndTag(); // </dlentry>
    writeEndTag(); // </dl>
    writeEndTag(); // </p>
}

/*!
  Write XML for a function index to the current XML stream.
 */
void DitaXmlGenerator::generateFunctionIndex(const Node* relative)
{
    writeStartTag(DT_p);
    xmlWriter().writeAttribute("outputclass","alphabet");
    for (int i = 0; i < 26; i++) {
        QChar ch('a' + i);
        writeStartTag(DT_xref);
        // formathtml
        QString guid = lookupGuid(outFileName(),QString(ch));
        QString attr = outFileName() + QString("#%1").arg(guid);
        xmlWriter().writeAttribute("href", attr);
        xmlWriter().writeCharacters(QString(ch.toUpper()));
        writeEndTag(); // </xref>

    }
    writeEndTag(); // </p>

    char nextLetter = 'a';
    char currentLetter;

    writeStartTag(DT_ul);
    NodeMapMap& funcIndex = qdb_->getFunctionIndex();
    NodeMapMap::ConstIterator f = funcIndex.constBegin();
    while (f != funcIndex.constEnd()) {
        writeStartTag(DT_li);
        currentLetter = f.key()[0].unicode();
        while (islower(currentLetter) && currentLetter >= nextLetter) {
            writeStartTag(DT_p);
            writeGuidAttribute(QString(nextLetter));
            xmlWriter().writeAttribute("outputclass","target");
            xmlWriter().writeCharacters(QString(nextLetter));
            writeEndTag(); // </p>
            nextLetter++;
        }
        xmlWriter().writeCharacters(protectEnc(f.key()));
        xmlWriter().writeCharacters(":");

        NodeMap::ConstIterator s = (*f).constBegin();
        while (s != (*f).constEnd()) {
            generateFullName((*s)->parent(), relative, *s);
            ++s;
        }
        writeEndTag(); // </li>
        ++f;
    }
    writeEndTag(); // </ul>
}

/*!
  Write the legalese texts as XML to the current XML stream.
 */
void DitaXmlGenerator::generateLegaleseList(const Node* relative, CodeMarker* marker)
{
    TextToNodeMap& legaleseTexts = qdb_->getLegaleseTexts();
    TextToNodeMap::ConstIterator it = legaleseTexts.constBegin();
    while (it != legaleseTexts.constEnd()) {
        Text text = it.key();
        generateText(text, relative, marker);
        writeStartTag(DT_ul);
        do {
            writeStartTag(DT_li);
            generateFullName(it.value(), relative);
            writeEndTag(); // </li>
            ++it;
        } while (it != legaleseTexts.constEnd() && it.key() == text);
        writeEndTag(); //</ul>
    }
}

/*!
  Generate the text for the QML item described by \a node
  and write it to the current XML stream.
 */
void DitaXmlGenerator::generateQmlItem(const Node* node,
                                       const Node* relative,
                                       CodeMarker* marker,
                                       bool summary)
{
    QString marked = marker->markedUpQmlItem(node,summary);
    QRegExp tag("(<[^@>]*>)");
    if (marked.indexOf(tag) != -1) {
        QString tmp = protectEnc(marked.mid(tag.pos(1), tag.cap(1).length()));
        marked.replace(tag.pos(1), tag.cap(1).length(), tmp);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i>\\1<sub>\\2</sub></i>");
    if (summary) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }
    writeText(marked, relative);
}

/*!
  Write the XML for the overview list to the current XML stream.
 */
void DitaXmlGenerator::generateOverviewList(const Node* relative)
{
    QMap<const DocNode*, QMap<QString, DocNode*> > docNodeMap;
    QMap<QString, const DocNode*> groupTitlesMap;
    QMap<QString, DocNode*> uncategorizedNodeMap;
    QRegExp singleDigit("\\b([0-9])\\b");

    const NodeList children = qdb_->treeRoot()->childNodes();
    foreach (Node* child, children) {
        if (child->type() == Node::Document && child != relative) {
            DocNode* docNode = static_cast<DocNode*>(child);

            // Check whether the page is part of a group or is the group
            // definition page.
            QString group;
            bool isGroupPage = false;
            if (docNode->doc().metaCommandsUsed().contains("group")) {
                group = docNode->doc().metaCommandArgs("group")[0].first;
                isGroupPage = true;
            }

            // there are too many examples; they would clutter the list
            if (docNode->subType() == Node::Example)
                continue;

            // not interested either in individual (Qt Designer etc.) manual chapters
            if (docNode->links().contains(Node::ContentsLink))
                continue;

            // Discard external nodes.
            if (docNode->subType() == Node::ExternalPage)
                continue;

            QString sortKey = docNode->fullTitle().toLower();
            if (sortKey.startsWith("the "))
                sortKey.remove(0, 4);
            sortKey.replace(singleDigit, "0\\1");

            if (!group.isEmpty()) {
                if (isGroupPage) {
                    // If we encounter a group definition page, we add all
                    // the pages in that group to the list for that group.
                    foreach (Node* member, docNode->members()) {
                        if (member->isInternal() || member->type() != Node::Document)
                            continue;
                        DocNode* page = static_cast<DocNode*>(member);
                        if (page) {
                            QString sortKey = page->fullTitle().toLower();
                            if (sortKey.startsWith("the "))
                                sortKey.remove(0, 4);
                            sortKey.replace(singleDigit, "0\\1");
                            docNodeMap[const_cast<const DocNode*>(docNode)].insert(sortKey, page);
                            groupTitlesMap[docNode->fullTitle()] = const_cast<const DocNode*>(docNode);
                        }
                    }
                }
                else if (!isGroupPage) {
                    // If we encounter a page that belongs to a group then
                    // we add that page to the list for that group.
                    const DocNode* gn = qdb_->getGroup(group);
                    if (gn && !docNode->isInternal())
                        docNodeMap[gn].insert(sortKey, docNode);
                }
            }
        }
    }

    // We now list all the pages found that belong to groups.
    // If only certain pages were found for a group, but the definition page
    // for that group wasn't listed, the list of pages will be intentionally
    // incomplete. However, if the group definition page was listed, all the
    // pages in that group are listed for completeness.

    if (!docNodeMap.isEmpty()) {
        foreach (const QString& groupTitle, groupTitlesMap.keys()) {
            const DocNode* groupNode = groupTitlesMap[groupTitle];
            writeStartTag(DT_p);
            xmlWriter().writeAttribute("outputclass","h3");
            writeStartTag(DT_xref);
            // formathtml
            xmlWriter().writeAttribute("href",linkForNode(groupNode, relative));
            writeCharacters(protectEnc(groupNode->fullTitle()));
            writeEndTag(); // </xref>
            writeEndTag(); // </p>
            if (docNodeMap[groupNode].count() == 0)
                continue;

            writeStartTag(DT_ul);
            foreach (const DocNode* docNode, docNodeMap[groupNode]) {
                QString title = docNode->fullTitle();
                if (title.startsWith("The "))
                    title.remove(0, 4);
                writeStartTag(DT_li);
                writeStartTag(DT_xref);
                // formathtml
                xmlWriter().writeAttribute("href",linkForNode(docNode, relative));
                writeCharacters(protectEnc(title));
                writeEndTag(); // </xref>
                writeEndTag(); // </li>
            }
            writeEndTag(); // </ul>
        }
    }

    if (!uncategorizedNodeMap.isEmpty()) {
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","h3");
        xmlWriter().writeCharacters("Miscellaneous");
        writeEndTag(); // </p>
        writeStartTag(DT_ul);
        foreach (const DocNode *docNode, uncategorizedNodeMap) {
            QString title = docNode->fullTitle();
            if (title.startsWith("The "))
                title.remove(0, 4);
            writeStartTag(DT_li);
            writeStartTag(DT_xref);
            // formathtml
            xmlWriter().writeAttribute("href",linkForNode(docNode, relative));
            writeCharacters(protectEnc(title));
            writeEndTag(); // </xref>
            writeEndTag(); // </li>
        }
        writeEndTag(); // </ul>
    }
}

/*!
  Write the XML for a standard section of a page, e.g.
  "Public Functions" or "Protected Slots." The section
  is written too the current XML stream as a table.
 */
void DitaXmlGenerator::generateSection(const NodeList& nl,
                                       const Node* relative,
                                       CodeMarker* marker,
                                       CodeMarker::SynopsisStyle style)
{
    if (!nl.isEmpty()) {
        writeStartTag(DT_ul);
        NodeList::ConstIterator m = nl.constBegin();
        while (m != nl.constEnd()) {
            if ((*m)->access() != Node::Private) {
                writeStartTag(DT_li);
                QString marked = getMarkedUpSynopsis(*m, relative, marker, style);
                writeText(marked, relative);
                writeEndTag(); // </li>
            }
            ++m;
        }
        writeEndTag(); // </ul>
    }
}

/*!
  Writes the "inherited from" list to the current XML stream.
 */
void DitaXmlGenerator::generateSectionInheritedList(const Section& section, const Node* relative)
{
    if (section.inherited.isEmpty())
        return;
    writeStartTag(DT_ul);
    QList<QPair<InnerNode*,int> >::ConstIterator p = section.inherited.constBegin();
    while (p != section.inherited.constEnd()) {
        writeStartTag(DT_li);
        QString text;
        text.setNum((*p).second);
        text += QLatin1Char(' ');
        if ((*p).second == 1)
            text += section.singularMember;
        else
            text += section.pluralMember;
        text += " inherited from ";
        writeCharacters(text);
        writeStartTag(DT_xref);
        // formathtml
        // zzz
        text = fileName((*p).first) + QLatin1Char('#');
        text += DitaXmlGenerator::cleanRef(section.name.toLower());
        xmlWriter().writeAttribute("href",text);
        text = protectEnc((*p).first->plainFullName(relative));
        writeCharacters(text);
        writeEndTag(); // </xref>
        writeEndTag(); // </li>
        ++p;
    }
    writeEndTag(); // </ul>
}

/*!
  Get the synopsis from the \a node using the \a relative
  node if needed, and mark up the synopsis using \a marker.
  Use the style to decide which kind of sysnopsis to build,
  normally \c Summary or \c Detailed. Return the marked up
  string.
 */
QString DitaXmlGenerator::getMarkedUpSynopsis(const Node* node,
                                              const Node* relative,
                                              CodeMarker* marker,
                                              CodeMarker::SynopsisStyle style)
{
    QString marked = marker->markedUpSynopsis(node, relative, style);
    QRegExp tag("(<[^@>]*>)");
    if (marked.indexOf(tag) != -1) {
        QString tmp = protectEnc(marked.mid(tag.pos(1), tag.cap(1).length()));
        marked.replace(tag.pos(1), tag.cap(1).length(), tmp);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i> \\1<sub>\\2</sub></i>");
    if (style == CodeMarker::Summary) {
        marked.remove("<@name>");   // was "<b>"
        marked.remove("</@name>");  // was "</b>"
    }

    if (style == CodeMarker::Subpage) {
        QRegExp extraRegExp("<@extra>.*</@extra>");
        extraRegExp.setMinimal(true);
        marked.remove(extraRegExp);
    }

    if (style != CodeMarker::Detailed) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }
    return marked;
}

/*!
  Renamed from highlightedCode() in the html generator. Gets the text
  from \a markedCode , and then the text is written to the current XML
  stream.
 */
void DitaXmlGenerator::writeText(const QString& markedCode, const Node* relative)
{
    QString src = markedCode;
    QString text;
    QStringRef arg;
    QStringRef par1;

    const QChar charLangle = '<';
    const QChar charAt = '@';

    /*
      First strip out all the extraneous markup. The table
      below contains the markup we want to keep. Everything
      else that begins with "<@" or "</@" is stripped out.
     */
    static const QString spanTags[] = {
        "<@link ",         "<@link ",
        "<@type>",         "<@type>",
        "<@headerfile>",   "<@headerfile>",
        "<@func>",         "<@func>",
        "<@func ",         "<@func ",
        "<@param>",        "<@param>",
        "<@extra>",        "<@extra>",
        "</@link>",        "</@link>",
        "</@type>",        "</@type>",
        "</@headerfile>",  "</@headerfile>",
        "</@func>",        "</@func>",
        "</@param>",        "</@param>",
        "</@extra>",        "</@extra>"
    };
    for (int i = 0, n = src.size(); i < n;) {
        if (src.at(i) == charLangle) {
            bool handled = false;
            for (int k = 0; k != 13; ++k) {
                const QString & tag = spanTags[2 * k];
                if (tag == QStringRef(&src, i, tag.length())) {
                    text += spanTags[2 * k + 1];
                    i += tag.length();
                    handled = true;
                    break;
                }
            }
            if (!handled) {
                ++i;
                if (src.at(i) == charAt ||
                        (src.at(i) == QLatin1Char('/') && src.at(i + 1) == charAt)) {
                    // drop 'our' unknown tags (the ones still containing '@')
                    while (i < n && src.at(i) != QLatin1Char('>'))
                        ++i;
                    ++i;
                }
                else {
                    // retain all others
                    text += charLangle;
                }
            }
        }
        else {
            text += src.at(i);
            ++i;
        }
    }

    // replace all <@link> tags: "(<@link node=\"([^\"]+)\">).*(</@link>)"
    // replace all "(<@(type|headerfile|func)(?: +[^>]*)?>)(.*)(</@\\2>)" tags
    src = text;
    text = QString();
    static const QString markTags[] = {
        // 0       1         2           3       4        5
        "link", "type", "headerfile", "func", "param", "extra"
    };

    for (int i = 0, n = src.size(); i < n;) {
        if (src.at(i) == charLangle && src.at(i + 1) == charAt) {
            i += 2;
            for (int k = 0; k != 6; ++k) {
                if (parseArg(src, markTags[k], &i, n, &arg, &par1)) {
                    const Node* n = 0;
                    if (k == 0) { // <@link>
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        n = CodeMarker::nodeForString(par1.toString());
                        QString link = linkForNode(n, relative);
                        addLink(link, arg);
                    }
                    else if (k == 4) { // <@param>
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        writeStartTag(DT_i);
                        //writeCharacters(" " + arg.toString());
                        writeCharacters(arg.toString());
                        writeEndTag(); // </i>
                    }
                    else if (k == 5) { // <@extra>
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        writeStartTag(DT_tt);
                        writeCharacters(arg.toString());
                        writeEndTag(); // </tt>
                    }
                    else {
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        par1 = QStringRef();
                        QString link;
                        n = qdb_->resolveTarget(arg.toString(), relative);
                        if (n && n->subType() == Node::QmlBasicType) {
                            if (relative && relative->subType() == Node::QmlClass) {
                                link = linkForNode(n,relative);
                                addLink(link, arg);
                            }
                            else {
                                writeCharacters(arg.toString());
                            }
                        }
                        else {
                            // (zzz) Is this correct for all cases?
                            link = linkForNode(n,relative);
                            addLink(link, arg);
                        }
                    }
                    break;
                }
            }
        }
        else {
            text += src.at(i++);
        }
    }
    if (!text.isEmpty()) {
        writeCharacters(text);
    }
}

void DitaXmlGenerator::generateLink(const Atom* atom, CodeMarker* marker)
{
    static QRegExp camelCase("[A-Z][A-Z][a-z]|[a-z][A-Z0-9]|_");

    if (funcLeftParen.indexIn(atom->string()) != -1 && marker->recognizeLanguage("Cpp")) {
        // hack for C++: move () outside of link
        int k = funcLeftParen.pos(1);
        writeCharacters(protectEnc(atom->string().left(k)));
        if (link_.isEmpty()) {
            if (showBrokenLinks)
                writeEndTag(); // </i>
        }
        else
            writeEndTag(); // </xref>
        inLink_ = false;
        writeCharacters(protectEnc(atom->string().mid(k)));
    }
    else if (marker->recognizeLanguage("Java")) {
        // hack for Java: remove () and use <tt> when appropriate
        bool func = atom->string().endsWith("()");
        bool tt = (func || atom->string().contains(camelCase));
        if (tt)
            writeStartTag(DT_tt);
        if (func)
            writeCharacters(protectEnc(atom->string().left(atom->string().length() - 2)));
        else
            writeCharacters(protectEnc(atom->string()));
        writeEndTag(); // </tt>
    }
    else
        writeCharacters(protectEnc(atom->string()));
}

QString DitaXmlGenerator::cleanRef(const QString& ref)
{
    QString clean;

    if (ref.isEmpty())
        return clean;

    clean.reserve(ref.size() + 20);
    const QChar c = ref[0];
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
        clean += QLatin1Char('A');
    }

    for (int i = 1; i < (int) ref.length(); i++) {
        const QChar c = ref[i];
        const uint u = c.unicode();
        if ((u >= 'a' && u <= 'z') ||
                (u >= 'A' && u <= 'Z') ||
                (u >= '0' && u <= '9') || u == '-' ||
                u == '_' || u == ':' || u == '.') {
            clean += c;
        }
        else if (c.isSpace()) {
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
            clean += QLatin1Char('#');
        }
        else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

QString DitaXmlGenerator::registerRef(const QString& ref)
{
    QString clean = DitaXmlGenerator::cleanRef(ref);

    for (;;) {
        QString& prevRef = refMap[clean.toLower()];
        if (prevRef.isEmpty()) {
            prevRef = ref;
            break;
        }
        else if (prevRef == ref)
            break;
        clean += QLatin1Char('x');
    }
    return clean;
}

/*!
  Calls protect() with the \a string. Returns the result.
 */
QString DitaXmlGenerator::protectEnc(const QString& string)
{
#ifndef QT_NO_TEXTCODEC
    return protect(string, outputEncoding);
#else
    return protect(string);
#endif
}

QString DitaXmlGenerator::protect(const QString& string, const QString& ) //outputEncoding)
{
#define APPEND(x) \
    if (xml.isEmpty()) { \
    xml = string; \
    xml.truncate(i); \
} \
    xml += (x);

    QString xml;
    int n = string.length();

    for (int i = 0; i < n; ++i) {
        QChar ch = string.at(i);

        if (ch == QLatin1Char('&')) {
            APPEND("&amp;");
        }
        else if (ch == QLatin1Char('<')) {
            APPEND("&lt;");
        }
        else if (ch == QLatin1Char('>')) {
            APPEND("&gt;");
        }
        else if (ch == QLatin1Char('"')) {
            APPEND("&quot;");
        }
        else {
            if (!xml.isEmpty())
                xml += ch;
        }
    }

    if (!xml.isEmpty())
        return xml;
    return string;

#undef APPEND
}

/*!
  Constructs a file name appropriate for the \a node
  and returns the file name.
 */
QString DitaXmlGenerator::fileBase(const Node* node) const
{
    QString result;
    result = Generator::fileBase(node);
    return result;
}

QString DitaXmlGenerator::guidForNode(const Node* node)
{
    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
    default:
        break;
    case Node::Enum:
        return node->guid();
    case Node::Typedef:
    {
        const TypedefNode* tdn = static_cast<const TypedefNode*>(node);
        if (tdn->associatedEnum())
            return guidForNode(tdn->associatedEnum());
    }
        return node->guid();
    case Node::Function:
    {
        const FunctionNode* fn = static_cast<const FunctionNode*>(node);
        if (fn->associatedProperty()) {
            return guidForNode(fn->associatedProperty());
        }
        else {
            QString ref = fn->name();
            if (fn->overloadNumber() != 1) {
                ref += QLatin1Char('-') + QString::number(fn->overloadNumber());
            }
        }
        return fn->guid();
    }
    case Node::Document:
        break;
    case Node::QmlPropertyGroup:
    case Node::QmlProperty:
    case Node::Property:
        return node->guid();
    case Node::QmlSignal:
        return node->guid();
    case Node::QmlSignalHandler:
        return node->guid();
    case Node::QmlMethod:
        return node->guid();
    case Node::Variable:
        return node->guid();
    }
    return QString();
}

/*!
  Constructs a file name appropriate for the \a node and returns
  it. If the \a node is not a fake node, or if it is a fake node but
  it is neither an external page node nor an image node or a ditamap,
  call the Generator::fileName() function.
 */
QString DitaXmlGenerator::fileName(const Node* node)
{
    if (node->type() == Node::Document) {
        if (static_cast<const DocNode*>(node)->pageType() == Node::DitaMapPage)
            return node->name();
        if (static_cast<const DocNode*>(node)->subType() == Node::ExternalPage)
            return node->name();
        if (static_cast<const DocNode*>(node)->subType() == Node::Image)
            return node->name();
    }
    return Generator::fileName(node);
}

QString DitaXmlGenerator::linkForNode(const Node* node, const Node* relative)
{
    if (node == 0 || node == relative)
        return QString();
    if (!node->url().isEmpty())
        return node->url();
    if (fileBase(node).isEmpty())
        return QString();
    if (node->access() == Node::Private)
        return QString();

    QString fn = fileName(node);
    if (node && relative && node->parent() != relative) {
        if (node->parent()->subType() == Node::QmlClass && relative->subType() == Node::QmlClass) {
            if (node->parent()->isAbstract()) {
                /*
                  This is a bit of a hack. What we discover with
                  the three 'if' statements immediately above,
                  is that node's parent is marked \qmlabstract
                  but the link appears in a qdoc comment for a
                  subclass of the node's parent. This means the
                  link should refer to the file for the relative
                  node, not the file for node.
                 */
                fn = fileName(relative);
            }
        }
    }
    QString link = fn;

    if (!node->isInnerNode() || node->type() == Node::QmlPropertyGroup) {
        QString guid = guidForNode(node);
        if (relative && fn == fileName(relative) && guid == guidForNode(relative)) {
            return QString();
        }
        link += QLatin1Char('#');
        link += guid;
    }
    /*
      If the output is going to subdirectories, then if the
      two nodes will be output to different directories, then
      the link must go up to the parent directory and then
      back down into the other subdirectory.
     */
    if (node && relative && (node != relative)) {
        if (useOutputSubdirs() && node->outputSubdirectory() != relative->outputSubdirectory())
            link.prepend(QString("../" + node->outputSubdirectory() + QLatin1Char('/')));
    }
    return link;
}

void DitaXmlGenerator::generateFullName(const Node* apparentNode,
                                        const Node* relative,
                                        const Node* actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    writeStartTag(DT_xref);
    // formathtml
    QString href = linkForNode(actualNode, relative);
    writeHrefAttribute(href);
    writeCharacters(protectEnc(apparentNode->fullName(relative)));
    writeEndTag(); // </xref>
}

/*!
  We're writing an attribute that indicates that the text
  data is a heading, hence, h1, h2, h3... etc, and we must
  decide which number to use.
 */
int DitaXmlGenerator::hOffset(const Node* node)
{
    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
        return 2;
    case Node::Document:
        return 1;
    case Node::Enum:
    case Node::Typedef:
    case Node::Function:
    case Node::Property:
    default:
        return 3;
    }
}

bool DitaXmlGenerator::isThreeColumnEnumValueTable(const Atom* atom)
{
    while (atom != 0 && !(atom->type() == Atom::ListRight && atom->string() == ATOM_LIST_VALUE)) {
        if (atom->type() == Atom::ListItemLeft && !matchAhead(atom, Atom::ListItemRight))
            return true;
        atom = atom->next();
    }
    return false;
}

const QPair<QString,QString> DitaXmlGenerator::anchorForNode(const Node* node)
{
    QPair<QString,QString> anchorPair;
    anchorPair.first = Generator::fileName(node);
    if (node->type() == Node::Document) {
        const DocNode *docNode = static_cast<const DocNode*>(node);
        anchorPair.second = docNode->title();
    }

    return anchorPair;
}

QString DitaXmlGenerator::getLink(const Atom* atom, const Node* relative, const Node** node)
{
    QString link;
    *node = 0;
    inObsoleteLink = false;

    if (atom->string().contains(QLatin1Char(':')) &&
            (atom->string().startsWith("file:")
             || atom->string().startsWith("http:")
             || atom->string().startsWith("https:")
             || atom->string().startsWith("ftp:")
             || atom->string().startsWith("mailto:"))) {

        link = atom->string();
    }
    else {
        QStringList path;
        if (atom->string().contains('#'))
            path = atom->string().split('#');
        else
            path.append(atom->string());

        QString ref;
        QString first = path.first().trimmed();

        if (first.isEmpty()) {
            *node = relative;
        }
        else if (first.endsWith(".html")) {
            *node = qdb_->treeRoot()->findChildNodeByNameAndType(first, Node::Document);
        }
        else {
            *node = qdb_->resolveTarget(first, relative);
            if (!*node)
                *node = qdb_->findDocNodeByTitle(first, relative);
            if (!*node)
                *node = qdb_->findUnambiguousTarget(first, ref, relative);
        }

        if (*node) {
            if (!(*node)->url().isEmpty())
                return (*node)->url();
            else
                path.removeFirst();
        }
        else
            *node = relative;

        if (*node && (*node)->status() == Node::Obsolete) {
            if (relative && (relative->parent() != *node) &&
                    (relative->status() != Node::Obsolete)) {
                bool porting = false;
                if (relative->type() == Node::Document) {
                    const DocNode* fake = static_cast<const DocNode*>(relative);
                    if (fake->title().startsWith("Porting"))
                        porting = true;
                }
                QString name = relative->plainFullName();
                if (!porting && !name.startsWith("Q3")) {
                    if (obsoleteLinks) {
                        relative->doc().location().warning(tr("Link to obsolete item '%1' in %2")
                                                           .arg(atom->string())
                                                           .arg(name));
                    }
                    inObsoleteLink = true;
                }
            }
        }

        while (!path.isEmpty()) {
            ref = qdb_->findTarget(path.first(), *node);
            if (ref.isEmpty())
                break;
            path.removeFirst();
        }

        if (path.isEmpty()) {
            link = linkForNode(*node, relative);
            if (*node && (*node)->subType() == Node::Image)
                link = "images/used-in-examples/" + link;
            if (!ref.isEmpty()) {
                if (link.isEmpty())
                    link = outFileName();
                QString guid = lookupGuid(link, ref);
                link += QLatin1Char('#') + guid;
            }
            else if (!link.isEmpty() && *node && (link.endsWith(".xml") || link.endsWith(".dita"))) {
                link += QLatin1Char('#') + (*node)->guid();
            }
        }
    }
    if (!link.isEmpty() && link[0] == '#') {
        link.prepend(outFileName());
    }
    return link;
}

void DitaXmlGenerator::generateStatus(const Node* node, CodeMarker* marker)
{
    Text text;

    switch (node->status()) {
    case Node::Obsolete:
        if (node->isInnerNode())
            Generator::generateStatus(node, marker);
        break;
    case Node::Compat:
        if (node->isInnerNode()) {
            text << Atom::ParaLeft
                 << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
                 << "This "
                 << typeString(node)
                 << " is part of the Qt 3 support library."
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
                 << " It is provided to keep old source code working. "
                 << "We strongly advise against "
                 << "using it in new code. See ";

            const DocNode *docNode = qdb_->findDocNodeByTitle("Porting To Qt 4");
            QString ref;
            if (docNode && node->type() == Node::Class) {
                QString oldName(node->name());
                oldName.remove(QLatin1Char('3'));
                ref = qdb_->findTarget(oldName,docNode);
            }

            if (!ref.isEmpty()) {
                QString fn = fileName(docNode);
                QString guid = lookupGuid(fn, ref);
                text << Atom(Atom::GuidLink, fn + QLatin1Char('#') + guid);
            }
            else
                text << Atom(Atom::Link, "Porting to Qt 4");

            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << Atom(Atom::String, "Porting to Qt 4")
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                 << " for more information."
                 << Atom::ParaRight;
        }
        generateText(text, node, marker);
        break;
    default:
        Generator::generateStatus(node, marker);
    }
}

void DitaXmlGenerator::beginLink(const QString& link)
{
    link_ = link;
    if (link_.isEmpty())
        return;
    writeStartTag(DT_xref);
    // formathtml
    writeHrefAttribute(link_);
    inLink_ = true;
}

void DitaXmlGenerator::endLink()
{
    if (inLink_) {
        if (link_.isEmpty()) {
            if (showBrokenLinks)
                writeEndTag(); // </i>
        }
        else {
            if (inObsoleteLink) {
                writeStartTag(DT_sup);
                xmlWriter().writeCharacters("(obsolete)");
                writeEndTag(); // </sup>
            }
            writeEndTag(); // </xref>
        }
    }
    inLink_ = false;
    inObsoleteLink = false;
}

/*!
  Generates the summary for the \a section. Only used for
  sections of QML element documentation.

  Currently handles only the QML property group.
 */
void DitaXmlGenerator::generateQmlSummary(const Section& section,
                                          const Node* relative,
                                          CodeMarker* marker)
{
    if (!section.members.isEmpty()) {
        writeStartTag(DT_ul);
        NodeList::ConstIterator m;
        m = section.members.constBegin();
        while (m != section.members.constEnd()) {
            writeStartTag(DT_li);
            generateQmlItem(*m,relative,marker,true);
            writeEndTag(); // </li>
            ++m;
        }
        writeEndTag(); // </ul>
    }
}

/*!
  Writes the QML property \a qpn to the current DITA XML file.
  Assumes that the correct start tag has already been written,
  but nothing has been written inside that tag. This function
  begins by writing the GUID id attribute for the property.
 */
void DitaXmlGenerator::startQmlProperty(QmlPropertyNode* qpn,
                                        const InnerNode* relative,
                                        CodeMarker* marker)
{
    writeStartTag(DT_qmlProperty);
    writeGuidAttribute((Node*)qpn);
    writeStartTag(DT_apiName);
    writeCharacters(qpn->name());
    writeEndTag(); // </apiName>
    generateBrief(qpn, marker); // <shortdesc>
    writeStartTag(DT_qmlPropertyDetail);
    writeStartTag(DT_qmlPropertyDef);
    if (!qpn->isReadOnlySet())
        qpn->setReadOnly(!qpn->isWritable(qdb_));
    if (qpn->isReadOnly()) {
        writeStartTag(DT_qmlQualifier);
        xmlWriter().writeAttribute("name","read-only");
        xmlWriter().writeAttribute("value","read-only");
        writeEndTag(); // </qmlQualifier>
    }
    if (qpn->isDefault()) {
        writeStartTag(DT_qmlQualifier);
        xmlWriter().writeAttribute("name","default");
        xmlWriter().writeAttribute("value","default");
        writeEndTag(); // </qmlQualifier>
    }
    if (qpn->isAttached()) {
        writeStartTag(DT_qmlAttached);
        xmlWriter().writeAttribute("name","attached");
        xmlWriter().writeAttribute("value","yes");
        writeEndTag(); // </qmlAttached>
    }
    writeStartTag(DT_apiData);
    generateQmlItem(qpn, relative, marker, false);
    writeEndTag(); // </apiData>
    writeEndTag(); // </qmlPropertyDef>
}

/*!
  Outputs the DITA detailed documentation for a section
  on a QML element reference page.
 */
void DitaXmlGenerator::generateDetailedQmlMember(Node* node,
                                                 const InnerNode* relative,
                                                 CodeMarker* marker)
{
    QString marked;
    QmlPropertyNode* qpn = 0;

    if (node->type() == Node::QmlPropertyGroup) {
        const QmlPropertyGroupNode* qpgn = static_cast<const QmlPropertyGroupNode*>(node);
        NodeList::ConstIterator p = qpgn->childNodes().constBegin();
        if (qpgn->childNodes().size() == 1) {
            qpn = static_cast<QmlPropertyNode*>(*p);
            startQmlProperty(qpn,relative,marker);
            writeApiDesc(node, marker, node->title());
            writeEndTag(); // </qmlPropertyDetail>
            writeEndTag(); // </qmlProperty>
        }
        else {
            writeStartTag(DT_qmlPropertyGroup);
            QString id = "id-qml-propertygroup-" + node->name();
            id.replace('.','-');
            xmlWriter().writeAttribute("id",id);
            writeStartTag(DT_apiName);
            //writeCharacters("...");
            writeEndTag(); // </apiName>
            writeStartTag(DT_qmlPropertyGroupDetail);
            writeApiDesc(node, marker, node->title());
            writeEndTag(); // </qmlPropertyGroupDetail>
            while (p != qpgn->childNodes().constEnd()) {
                if ((*p)->type() == Node::QmlProperty) {
                    qpn = static_cast<QmlPropertyNode*>(*p);
                    startQmlProperty(qpn,relative,marker);
                    writeEndTag(); // </qmlPropertyDetail>
                    writeEndTag(); // </qmlProperty>
                }
                ++p;
            }
            writeEndTag(); // </qmlPropertyGroup
        }
    }
    else if (node->type() == Node::QmlProperty) {
        qpn = static_cast<QmlPropertyNode*>(node);
        startQmlProperty(qpn,relative,marker);
        writeApiDesc(node, marker, node->title());
        writeEndTag(); // </qmlPropertyDetail>
        writeEndTag(); // </qmlProperty>
    }
    else if (node->type() == Node::QmlSignal)
        writeQmlRef(DT_qmlSignal,node,relative,marker);
    else if (node->type() == Node::QmlSignalHandler)
        writeQmlRef(DT_qmlSignalHandler,node,relative,marker);
    else if (node->type() == Node::QmlMethod)
        writeQmlRef(DT_qmlMethod,node,relative,marker);
}

/*!
  Outputs the DITA detailed documentation for a section
  on a QML element reference page.
 */
void DitaXmlGenerator::writeQmlRef(DitaTag tag,
                                   Node* node,
                                   const InnerNode* relative,
                                   CodeMarker* marker)
{
    writeStartTag(tag);
    Node* n = const_cast<Node*>(node);
    writeGuidAttribute(n);
    writeStartTag(DT_apiName);
    writeCharacters(n->name());
    writeEndTag(); // </apiName>
    writeStartTag((DitaTag)((int)tag+2));
    writeStartTag((DitaTag)((int)tag+1));
    writeStartTag(DT_apiData);
    QString marked = getMarkedUpSynopsis(n, relative, marker, CodeMarker::Detailed);
    writeText(marked, relative);
    writeEndTag(); // </apiData>
    if (node->isAttached()) {
        writeStartTag(DT_qmlAttached);
        xmlWriter().writeAttribute("name","attached");
        xmlWriter().writeAttribute("value","yes");
        writeEndTag(); // </qmlAttached>
    }
    writeEndTag(); // </qmlXxxDef>
    writeApiDesc(node, marker, node->title());
    writeEndTag(); // </qmlXxxDetail>
    writeEndTag(); // tag
}

/*!
  This generates a <qmlTypeDef> in which the
  QML module name and version number are specified.
 */
void DitaXmlGenerator::generateQmlModuleDef(QmlClassNode* qcn)
{
    writeStartTag(DT_qmlImportModule);
    writeStartTag(DT_apiItemName);
    writeCharacters(qcn->qmlModuleName());
    writeEndTag(); // </apiItemName>
    writeStartTag(DT_apiData);
    writeCharacters(qcn->qmlModuleVersion());
    writeEndTag(); // </apiData>
    writeEndTag(); // </qmlImportModule>
}

/*!
  Output the "Inherits" line for the QML element,
  if there should be one.
 */
void DitaXmlGenerator::generateQmlInherits(const QmlClassNode* qcn, CodeMarker* marker)
{
    if (!qcn)
        return;
    const QmlClassNode* base = qcn->qmlBaseNode();
    while (base && base->isInternal()) {
        base = base->qmlBaseNode();
    }
    if (base) {
        writeStartTag(DT_qmlInherits);
        //writeStartTag(DT_qmlTypeDef);
        //xmlWriter().writeAttribute("outputclass","inherits");
        writeStartTag(DT_apiData);
        Text text;
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(base));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, base->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        generateText(text, qcn, marker);
        writeEndTag(); // </apiData>
        writeEndTag(); // </qmlInherits>
    }
}

/*!
  Output the "Inherit by" list for the QML element,
  if it is inherited by any other elements.
 */
void DitaXmlGenerator::generateQmlInheritedBy(const QmlClassNode* qcn, CodeMarker* marker)
{
    if (qcn) {
        NodeList subs;
        QmlClassNode::subclasses(qcn->name(),subs);
        if (!subs.isEmpty()) {
            writeStartTag(DT_qmlInheritedBy);
            //writeStartTag(DT_qmlTypeDef);
            //xmlWriter().writeAttribute("outputclass","inherited-by");
            writeStartTag(DT_apiData);
            Text text;
            appendSortedQmlNames(text,qcn,subs);
            text << Atom::ParaRight;
            generateText(text, qcn, marker);
            writeEndTag(); // </apiData>
            writeEndTag(); // </qmlIneritedBy>
        }
    }
}

/*!
  Output the "[Xxx instantiates the C++ class QmlGraphicsXxx]"
  line for the QML element, if there should be one.

  If there is no class node, or if the class node status
  is set to Node::Internal, do nothing.
 */
void DitaXmlGenerator::generateQmlInstantiates(QmlClassNode* qcn, CodeMarker* marker)
{
    ClassNode* cn = qcn->classNode();
    if (cn && (cn->status() != Node::Internal)) {
        writeStartTag(DT_qmlInstantiates);
        //writeStartTag(DT_qmlTypeDef);
        //xmlWriter().writeAttribute("outputclass","instantiates");
        writeStartTag(DT_apiData);
        Text text;
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        generateText(text, qcn, marker);
        writeEndTag(); // </apiData>
        writeEndTag(); // </qmlInstantiates>
    }
}

/*!
  Generate a <qmlXxxDef> for the "since" version string, if there is one.
 */
void DitaXmlGenerator::generateQmlSince(const Node* node)
{
    if (!node->since().isEmpty()) {
        writeStartTag(DT_qmlSince);
        //writeStartTag(DT_qmlTypeDef);
        //xmlWriter().writeAttribute("outputclass","since");
        writeStartTag(DT_apiItemName);
        QStringList pieces = node->since().split(QLatin1Char(' '));
        writeCharacters(pieces[0]);
        writeEndTag(); // </apiItemName>
        writeStartTag(DT_apiData);
        if (pieces.size() > 1)
            writeCharacters(pieces[1]);
        writeEndTag(); // </apiData>
        writeEndTag(); // </qmlSince>
    }
}

/*!
  Output the "[QmlGraphicsXxx is instantiated by QML Type Xxx]"
  line for the class, if there should be one.

  If there is no QML element, or if the class node status
  is set to Node::Internal, do nothing.
 */
void DitaXmlGenerator::generateInstantiatedBy(ClassNode* cn, CodeMarker* marker)
{
    if (cn &&  cn->status() != Node::Internal && cn->qmlElement() != 0) {
        const QmlClassNode* qcn = cn->qmlElement();
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","instantiated-by");
        Text text;
        text << "[";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << " is instantiated by QML Type ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, qcn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << "]";
        generateText(text, cn, marker);
        writeEndTag(); // </p>
    }
}

/*!
  Return the full qualification of the node \a n, but without
  the name of \a n itself. e.g. A::B::C
 */
QString DitaXmlGenerator::fullQualification(const Node* n)
{
    QString fq;
    InnerNode* in = n->parent();
    while (in) {
        if ((in->type() == Node::Class) ||
                (in->type() == Node::Namespace)) {
            if (in->name().isEmpty())
                break;
            if (fq.isEmpty())
                fq = in->name();
            else
                fq = in->name() + "::" + fq;
        }
        else
            break;
        in = in->parent();
    }
    return fq;
}

/*!
  Outputs the <cxxClassDerivations> element.
  \code
 <cxxClassDerivations>
  <cxxClassDerivation>
   ...
  </cxxClassDerivation>
  ...
 </cxxClassDerivations>
  \endcode

  The <cxxClassDerivation> element is:

  \code
 <cxxClassDerivation>
  <cxxClassDerivationAccessSpecifier value="public"/>
  <cxxClassBaseClass href="class_base">Base</cxxClassBaseClass>
 </cxxClassDerivation>
  \endcode
 */
void DitaXmlGenerator::writeDerivations(const ClassNode* cn)
{
    QList<RelatedClass>::ConstIterator r;

    if (!cn->baseClasses().isEmpty()) {
        writeStartTag(DT_cxxClassDerivations);
        r = cn->baseClasses().constBegin();
        while (r != cn->baseClasses().constEnd()) {
            writeStartTag(DT_cxxClassDerivation);
            writeStartTag(DT_cxxClassDerivationAccessSpecifier);
            xmlWriter().writeAttribute("value",(*r).accessString());
            writeEndTag(); // </cxxClassDerivationAccessSpecifier>

            // not included: <cxxClassDerivationVirtual>

            writeStartTag(DT_cxxClassBaseClass);
            QString attr = fileName((*r).node) + QLatin1Char('#') + (*r).node->guid();
            xmlWriter().writeAttribute("href",attr);
            writeCharacters((*r).node->plainFullName());
            writeEndTag(); // </cxxClassBaseClass>

            // not included: <ClassBaseStruct> or <cxxClassBaseUnion>

            writeEndTag(); // </cxxClassDerivation>

            // not included: <cxxStructDerivation>

            ++r;
        }
        writeEndTag(); // </cxxClassDerivations>
    }
}

/*!
  Writes a <cxxXXXAPIItemLocation> element, depending on the
  type of the node \a n, which can be a class, function, enum,
  typedef, or property.
 */
void DitaXmlGenerator::writeLocation(const Node* n)
{
    DitaTag s1, s2, s3a, s3b;
    s1 = DT_cxxClassAPIItemLocation;
    s2 = DT_cxxClassDeclarationFile;
    s3a = DT_cxxClassDeclarationFileLineStart;
    s3b = DT_cxxClassDeclarationFileLineEnd;
    if (n->type() == Node::Class || n->type() == Node::Namespace) {
        s1 = DT_cxxClassAPIItemLocation;
        s2 = DT_cxxClassDeclarationFile;
        s3a = DT_cxxClassDeclarationFileLineStart;
        s3b = DT_cxxClassDeclarationFileLineEnd;
    }
    else if (n->type() == Node::Function) {
        FunctionNode* fn = const_cast<FunctionNode*>(static_cast<const FunctionNode*>(n));
        if (fn->isMacro()) {
            s1 = DT_cxxDefineAPIItemLocation;
            s2 = DT_cxxDefineDeclarationFile;
            s3a = DT_cxxDefineDeclarationFileLine;
            s3b = DT_NONE;
        }
        else {
            s1 = DT_cxxFunctionAPIItemLocation;
            s2 = DT_cxxFunctionDeclarationFile;
            s3a = DT_cxxFunctionDeclarationFileLine;
            s3b = DT_NONE;
        }
    }
    else if (n->type() == Node::Enum) {
        s1 = DT_cxxEnumerationAPIItemLocation;
        s2 = DT_cxxEnumerationDeclarationFile;
        s3a = DT_cxxEnumerationDeclarationFileLineStart;
        s3b = DT_cxxEnumerationDeclarationFileLineEnd;
    }
    else if (n->type() == Node::Typedef) {
        s1 = DT_cxxTypedefAPIItemLocation;
        s2 = DT_cxxTypedefDeclarationFile;
        s3a = DT_cxxTypedefDeclarationFileLine;
        s3b = DT_NONE;
    }
    else if ((n->type() == Node::Property) ||
             (n->type() == Node::Variable)) {
        s1 = DT_cxxVariableAPIItemLocation;
        s2 = DT_cxxVariableDeclarationFile;
        s3a = DT_cxxVariableDeclarationFileLine;
        s3b = DT_NONE;
    }
    writeStartTag(s1);
    writeStartTag(s2);
    xmlWriter().writeAttribute("name","filePath");
    xmlWriter().writeAttribute("value",n->location().filePath());
    writeEndTag(); // <s2>
    writeStartTag(s3a);
    xmlWriter().writeAttribute("name","lineNumber");
    QString lineNr;
    xmlWriter().writeAttribute("value",lineNr.setNum(n->location().lineNo()));
    writeEndTag(); // </s3a>
    if (s3b != DT_NONE) {
        writeStartTag(s3b);
        xmlWriter().writeAttribute("name","lineNumber");
        QString lineNr;
        xmlWriter().writeAttribute("value",lineNr.setNum(n->location().lineNo()));
        writeEndTag(); // </s3b>
    }
    writeEndTag(); // </cxx<s1>ApiItemLocation>
}

/*!
  Write the <cxxFunction> elements.
 */
void DitaXmlGenerator::writeFunctions(const Section& s,
                                      const InnerNode* parent,
                                      CodeMarker* marker,
                                      const QString& attribute)
{
    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Function) {
            FunctionNode* fn = const_cast<FunctionNode*>(static_cast<const FunctionNode*>(*m));
            writeStartTag(DT_cxxFunction);
            xmlWriter().writeAttribute("id",fn->guid());
            if (fn->metaness() == FunctionNode::Signal)
                xmlWriter().writeAttribute("otherprops","signal");
            else if (fn->metaness() == FunctionNode::Slot)
                xmlWriter().writeAttribute("otherprops","slot");
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(fn->name());
            writeEndTag(); // </apiName>
            generateBrief(fn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxFunctionDetail);
            writeStartTag(DT_cxxFunctionDefinition);
            writeStartTag(DT_cxxFunctionAccessSpecifier);
            xmlWriter().writeAttribute("value",fn->accessString());
            writeEndTag(); // <cxxFunctionAccessSpecifier>

            // not included: <cxxFunctionStorageClassSpecifierExtern>

            if (fn->isStatic()) {
                writeStartTag(DT_cxxFunctionStorageClassSpecifierStatic);
                xmlWriter().writeAttribute("name","static");
                xmlWriter().writeAttribute("value","static");
                writeEndTag(); // <cxxFunctionStorageClassSpecifierStatic>
            }

            // not included: <cxxFunctionStorageClassSpecifierMutable>,

            if (fn->isConst()) {
                writeStartTag(DT_cxxFunctionConst);
                xmlWriter().writeAttribute("name","const");
                xmlWriter().writeAttribute("value","const");
                writeEndTag(); // <cxxFunctionConst>
            }

            // not included: <cxxFunctionExplicit>
            //               <cxxFunctionInline

            if (fn->virtualness() != FunctionNode::NonVirtual) {
                writeStartTag(DT_cxxFunctionVirtual);
                xmlWriter().writeAttribute("name","virtual");
                xmlWriter().writeAttribute("value","virtual");
                writeEndTag(); // <cxxFunctionVirtual>
                if (fn->virtualness() == FunctionNode::PureVirtual) {
                    writeStartTag(DT_cxxFunctionPureVirtual);
                    xmlWriter().writeAttribute("name","pure virtual");
                    xmlWriter().writeAttribute("value","pure virtual");
                    writeEndTag(); // <cxxFunctionPureVirtual>
                }
            }

            if (fn->name() == parent->name()) {
                writeStartTag(DT_cxxFunctionConstructor);
                xmlWriter().writeAttribute("name","constructor");
                xmlWriter().writeAttribute("value","constructor");
                writeEndTag(); // <cxxFunctionConstructor>
            }
            else if (fn->name()[0] == QChar('~')) {
                writeStartTag(DT_cxxFunctionDestructor);
                xmlWriter().writeAttribute("name","destructor");
                xmlWriter().writeAttribute("value","destructor");
                writeEndTag(); // <cxxFunctionDestructor>
            }
            else {
                writeStartTag(DT_cxxFunctionDeclaredType);
                QString src = marker->typified(fn->returnType());
                replaceTypesWithLinks(fn,parent,src);
                writeEndTag(); // <cxxFunctionDeclaredType>
            }

            // not included: <cxxFunctionReturnType>

            QString fq = fullQualification(fn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxFunctionScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxFunctionScopedName>
            }
            writeStartTag(DT_cxxFunctionPrototype);
            writeCharacters(fn->signature(true));
            writeEndTag(); // <cxxFunctionPrototype>

            QString fnl = fn->signature(false);
            int idx = fnl.indexOf(' ');
            if (idx < 0)
                idx = 0;
            else
                ++idx;
            fnl = fn->parent()->name() + "::" + fnl.mid(idx);
            writeStartTag(DT_cxxFunctionNameLookup);
            writeCharacters(fnl);
            writeEndTag(); // <cxxFunctionNameLookup>

            if (!fn->isInternal() && fn->isReimp() && fn->reimplementedFrom() != 0) {
                FunctionNode* rfn = (FunctionNode*)fn->reimplementedFrom();
                if (rfn && !rfn->isInternal()) {
                    writeStartTag(DT_cxxFunctionReimplemented);
                    xmlWriter().writeAttribute("href",ditaXmlHref(rfn));
                    writeCharacters(rfn->plainFullName());
                    writeEndTag(); // </cxxFunctionReimplemented>
                }
            }
            writeParameters(fn,parent,marker);
            writeLocation(fn);
            writeEndTag(); // <cxxFunctionDefinition>

            writeApiDesc(fn, marker, QString());
            // generateAlsoList(inner, marker);

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxFunctionDetail>
            writeEndTag(); // </cxxFunction>

            if (fn->metaness() == FunctionNode::Ctor ||
                    fn->metaness() == FunctionNode::Dtor ||
                    fn->overloadNumber() != 1) {
            }
        }
        ++m;
    }
}

static const QString typeTag("type");
static const QChar charLangle = '<';
static const QChar charAt = '@';

/*!
  This function replaces class and enum names with <apiRelation>
  elements, i.e. links.
 */
void DitaXmlGenerator::replaceTypesWithLinks(const Node* n, const InnerNode* parent, QString& src)
{
    QStringRef arg;
    QStringRef par1;
    int srcSize = src.size();
    QString text;
    for (int i=0; i<srcSize;) {
        if (src.at(i) == charLangle && src.at(i+1) == charAt) {
            if (!text.isEmpty()) {
                writeCharacters(text);
                text.clear();
            }
            i += 2;
            if (parseArg(src, typeTag, &i, srcSize, &arg, &par1)) {
                const Node* tn = qdb_->resolveTarget(arg.toString(), parent, n);
                if (tn) {
                    //Do not generate a link from a C++ function to a QML Basic Type (such as int)
                    if (n->type() == Node::Function && tn->subType() == Node::QmlBasicType)
                        writeCharacters(arg.toString());
                    else
                        addLink(linkForNode(tn,parent),arg,DT_apiRelation);
                }
                else {
                    // Write simple arguments, like void and bool,
                    // which do not have a Qt defined target.
                    writeCharacters(arg.toString());
                }
            }
        }
        else {
            text += src.at(i++);
        }
    }
    if (!text.isEmpty()) {
        writeCharacters(text);
        text.clear();
    }
}

/*!
  This function writes the <cxxFunctionParameters> element.
 */
void DitaXmlGenerator::writeParameters(const FunctionNode* fn,
                                       const InnerNode* parent,
                                       CodeMarker* marker)
{
    const QList<Parameter>& parameters = fn->parameters();
    if (!parameters.isEmpty()) {
        writeStartTag(DT_cxxFunctionParameters);
        QList<Parameter>::ConstIterator p = parameters.constBegin();
        while (p != parameters.constEnd()) {
            writeStartTag(DT_cxxFunctionParameter);
            writeStartTag(DT_cxxFunctionParameterDeclaredType);
            QString src = marker->typified((*p).leftType());
            replaceTypesWithLinks(fn,parent,src);
            //writeCharacters((*p).leftType());
            if (!(*p).rightType().isEmpty())
                writeCharacters((*p).rightType());
            writeEndTag(); // <cxxFunctionParameterDeclaredType>
            writeStartTag(DT_cxxFunctionParameterDeclarationName);
            writeCharacters((*p).name());
            writeEndTag(); // <cxxFunctionParameterDeclarationName>

            // not included: <cxxFunctionParameterDefinitionName>

            if (!(*p).defaultValue().isEmpty()) {
                writeStartTag(DT_cxxFunctionParameterDefaultValue);
                writeCharacters((*p).defaultValue());
                writeEndTag(); // <cxxFunctionParameterDefaultValue>
            }

            // not included: <apiDefNote>

            writeEndTag(); // <cxxFunctionParameter>
            ++p;
        }
        writeEndTag(); // <cxxFunctionParameters>
    }
}

/*!
  This function writes the enum types.
 */
void DitaXmlGenerator::writeEnumerations(const Section& s,
                                         CodeMarker* marker,
                                         const QString& attribute)
{
    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Enum) {
            const EnumNode* en = static_cast<const EnumNode*>(*m);
            writeStartTag(DT_cxxEnumeration);
            xmlWriter().writeAttribute("id",en->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(en->name());
            writeEndTag(); // </apiName>
            generateBrief(en,marker);

            // not included <prolog>

            writeStartTag(DT_cxxEnumerationDetail);
            writeStartTag(DT_cxxEnumerationDefinition);
            writeStartTag(DT_cxxEnumerationAccessSpecifier);
            xmlWriter().writeAttribute("value",en->accessString());
            writeEndTag(); // <cxxEnumerationAccessSpecifier>

            QString fq = fullQualification(en);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxEnumerationScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxEnumerationScopedName>
            }
            const QList<EnumItem>& items = en->items();
            if (!items.isEmpty()) {
                writeStartTag(DT_cxxEnumerationPrototype);
                writeCharacters(en->name());
                xmlWriter().writeCharacters(" = { ");
                QList<EnumItem>::ConstIterator i = items.constBegin();
                while (i != items.constEnd()) {
                    writeCharacters((*i).name());
                    if (!(*i).value().isEmpty()) {
                        xmlWriter().writeCharacters(" = ");
                        writeCharacters((*i).value());
                    }
                    ++i;
                    if (i != items.constEnd())
                        xmlWriter().writeCharacters(", ");
                }
                xmlWriter().writeCharacters(" }");
                writeEndTag(); // <cxxEnumerationPrototype>
            }

            writeStartTag(DT_cxxEnumerationNameLookup);
            writeCharacters(en->parent()->name() + "::" + en->name());
            writeEndTag(); // <cxxEnumerationNameLookup>

            // not included: <cxxEnumerationReimplemented>

            if (!items.isEmpty()) {
                writeStartTag(DT_cxxEnumerators);
                QList<EnumItem>::ConstIterator i = items.constBegin();
                while (i != items.constEnd()) {
                    writeStartTag(DT_cxxEnumerator);
                    writeStartTag(DT_apiName);
                    writeCharacters((*i).name());
                    writeEndTag(); // </apiName>

                    QString fq = fullQualification(en->parent());
                    if (!fq.isEmpty()) {
                        writeStartTag(DT_cxxEnumeratorScopedName);
                        writeCharacters(fq + "::" + (*i).name());
                        writeEndTag(); // <cxxEnumeratorScopedName>
                    }
                    writeStartTag(DT_cxxEnumeratorPrototype);
                    writeCharacters((*i).name());
                    writeEndTag(); // <cxxEnumeratorPrototype>
                    writeStartTag(DT_cxxEnumeratorNameLookup);
                    writeCharacters(en->parent()->name() + "::" + (*i).name());
                    writeEndTag(); // <cxxEnumeratorNameLookup>

                    if (!(*i).value().isEmpty()) {
                        writeStartTag(DT_cxxEnumeratorInitialiser);
                        if ((*i).value().toInt(0,16) == 0)
                            xmlWriter().writeAttribute("value", "0");
                        else
                            xmlWriter().writeAttribute("value", (*i).value());
                        writeEndTag(); // <cxxEnumeratorInitialiser>
                    }

                    // not included: <cxxEnumeratorAPIItemLocation>

                    if (!(*i).text().isEmpty()) {
                        writeStartTag(DT_apiDesc);
                        generateText((*i).text(), en, marker);
                        writeEndTag(); // </apiDesc>
                    }
                    writeEndTag(); // <cxxEnumerator>
                    ++i;
                }
                writeEndTag(); // <cxxEnumerators>
            }

            writeLocation(en);
            writeEndTag(); // <cxxEnumerationDefinition>

            writeApiDesc(en, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxEnumerationDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxEnumeration>
        }
        ++m;
    }
}

/*!
  This function writes the output for the \typedef commands.
 */
void DitaXmlGenerator::writeTypedefs(const Section& s,
                                     CodeMarker* marker,
                                     const QString& attribute)

{
    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Typedef) {
            const TypedefNode* tn = static_cast<const TypedefNode*>(*m);
            writeStartTag(DT_cxxTypedef);
            xmlWriter().writeAttribute("id",tn->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(tn->name());
            writeEndTag(); // </apiName>
            generateBrief(tn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxTypedefDetail);
            writeStartTag(DT_cxxTypedefDefinition);
            writeStartTag(DT_cxxTypedefAccessSpecifier);
            xmlWriter().writeAttribute("value",tn->accessString());
            writeEndTag(); // <cxxTypedefAccessSpecifier>

            // not included: <cxxTypedefDeclaredType>

            QString fq = fullQualification(tn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxTypedefScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxTypedefScopedName>
            }

            // not included: <cxxTypedefPrototype>

            writeStartTag(DT_cxxTypedefNameLookup);
            writeCharacters(tn->parent()->name() + "::" + tn->name());
            writeEndTag(); // <cxxTypedefNameLookup>

            // not included: <cxxTypedefReimplemented>

            writeLocation(tn);
            writeEndTag(); // <cxxTypedefDefinition>

            writeApiDesc(tn, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxTypedefDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxTypedef>
        }
        ++m;
    }
}

/*!
  This function writes the output for the \property commands.
  This is the Q_PROPERTYs.
 */
void DitaXmlGenerator::writeProperties(const Section& s,
                                       CodeMarker* marker,
                                       const QString& attribute)
{
    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Property) {
            const PropertyNode* pn = static_cast<const PropertyNode*>(*m);
            writeStartTag(DT_cxxVariable);
            xmlWriter().writeAttribute("id",pn->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(pn->name());
            writeEndTag(); // </apiName>
            generateBrief(pn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxVariableDetail);
            writeStartTag(DT_cxxVariableDefinition);
            writeStartTag(DT_cxxVariableAccessSpecifier);
            xmlWriter().writeAttribute("value",pn->accessString());
            writeEndTag(); // <cxxVariableAccessSpecifier>

            // not included: <cxxVariableStorageClassSpecifierExtern>,
            //               <cxxVariableStorageClassSpecifierStatic>,
            //               <cxxVariableStorageClassSpecifierMutable>,
            //               <cxxVariableConst>, <cxxVariableVolatile>

            if (!pn->qualifiedDataType().isEmpty()) {
                writeStartTag(DT_cxxVariableDeclaredType);
                writeCharacters(pn->qualifiedDataType());
                writeEndTag(); // <cxxVariableDeclaredType>
            }
            QString fq = fullQualification(pn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxVariableScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxVariableScopedName>
            }

            writeStartTag(DT_cxxVariablePrototype);
            xmlWriter().writeCharacters("Q_PROPERTY(");
            writeCharacters(pn->qualifiedDataType());
            xmlWriter().writeCharacters(" ");
            writeCharacters(pn->name());
            writePropertyParameter("READ",pn->getters());
            writePropertyParameter("WRITE",pn->setters());
            writePropertyParameter("RESET",pn->resetters());
            writePropertyParameter("NOTIFY",pn->notifiers());
            if (pn->isDesignable() != pn->designableDefault()) {
                xmlWriter().writeCharacters(" DESIGNABLE ");
                if (!pn->runtimeDesignabilityFunction().isEmpty())
                    writeCharacters(pn->runtimeDesignabilityFunction());
                else
                    xmlWriter().writeCharacters(pn->isDesignable() ? "true" : "false");
            }
            if (pn->isScriptable() != pn->scriptableDefault()) {
                xmlWriter().writeCharacters(" SCRIPTABLE ");
                if (!pn->runtimeScriptabilityFunction().isEmpty())
                    writeCharacters(pn->runtimeScriptabilityFunction());
                else
                    xmlWriter().writeCharacters(pn->isScriptable() ? "true" : "false");
            }
            if (pn->isWritable() != pn->writableDefault()) {
                xmlWriter().writeCharacters(" STORED ");
                xmlWriter().writeCharacters(pn->isStored() ? "true" : "false");
            }
            if (pn->isUser() != pn->userDefault()) {
                xmlWriter().writeCharacters(" USER ");
                xmlWriter().writeCharacters(pn->isUser() ? "true" : "false");
            }
            if (pn->isConstant())
                xmlWriter().writeCharacters(" CONSTANT");
            if (pn->isFinal())
                xmlWriter().writeCharacters(" FINAL");
            xmlWriter().writeCharacters(")");
            writeEndTag(); // <cxxVariablePrototype>

            writeStartTag(DT_cxxVariableNameLookup);
            writeCharacters(pn->parent()->name() + "::" + pn->name());
            writeEndTag(); // <cxxVariableNameLookup>

            if (pn->overriddenFrom() != 0) {
                PropertyNode* opn = (PropertyNode*)pn->overriddenFrom();
                writeStartTag(DT_cxxVariableReimplemented);
                xmlWriter().writeAttribute("href",ditaXmlHref(opn));
                writeCharacters(opn->plainFullName());
                writeEndTag(); // </cxxVariableReimplemented>
            }

            writeLocation(pn);
            writeEndTag(); // <cxxVariableDefinition>

            writeApiDesc(pn, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxVariableDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxVariable>
        }
        ++m;
    }
}

/*!
  This function outputs the nodes resulting from \variable commands.
 */
void DitaXmlGenerator::writeDataMembers(const Section& s,
                                        CodeMarker* marker,
                                        const QString& attribute)
{
    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Variable) {
            const VariableNode* vn = static_cast<const VariableNode*>(*m);
            writeStartTag(DT_cxxVariable);
            xmlWriter().writeAttribute("id",vn->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(vn->name());
            writeEndTag(); // </apiName>
            generateBrief(vn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxVariableDetail);
            writeStartTag(DT_cxxVariableDefinition);
            writeStartTag(DT_cxxVariableAccessSpecifier);
            xmlWriter().writeAttribute("value",vn->accessString());
            writeEndTag(); // <cxxVariableAccessSpecifier>

            // not included: <cxxVAriableStorageClassSpecifierExtern>

            if (vn->isStatic()) {
                writeStartTag(DT_cxxVariableStorageClassSpecifierStatic);
                xmlWriter().writeAttribute("name","static");
                xmlWriter().writeAttribute("value","static");
                writeEndTag(); // <cxxVariableStorageClassSpecifierStatic>
            }

            // not included: <cxxVAriableStorageClassSpecifierMutable>,
            //               <cxxVariableConst>, <cxxVariableVolatile>

            writeStartTag(DT_cxxVariableDeclaredType);
            writeCharacters(vn->leftType());
            if (!vn->rightType().isEmpty())
                writeCharacters(vn->rightType());
            writeEndTag(); // <cxxVariableDeclaredType>

            QString fq = fullQualification(vn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxVariableScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxVariableScopedName>
            }

            writeStartTag(DT_cxxVariablePrototype);
            writeCharacters(vn->leftType() + QLatin1Char(' '));
            //writeCharacters(vn->parent()->name() + "::" + vn->name());
            writeCharacters(vn->name());
            if (!vn->rightType().isEmpty())
                writeCharacters(vn->rightType());
            writeEndTag(); // <cxxVariablePrototype>

            writeStartTag(DT_cxxVariableNameLookup);
            writeCharacters(vn->parent()->name() + "::" + vn->name());
            writeEndTag(); // <cxxVariableNameLookup>

            // not included: <cxxVariableReimplemented>

            writeLocation(vn);
            writeEndTag(); // <cxxVariableDefinition>

            writeApiDesc(vn, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxVariableDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxVariable>
        }
        ++m;
    }
}

/*!
  This function writes a \macro as a <cxxDefine>.
 */
void DitaXmlGenerator::writeMacros(const Section& s,
                                   CodeMarker* marker,
                                   const QString& attribute)
{
    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Function) {
            const FunctionNode* fn = static_cast<const FunctionNode*>(*m);
            if (fn->isMacro()) {
                writeStartTag(DT_cxxDefine);
                xmlWriter().writeAttribute("id",fn->guid());
                if (!attribute.isEmpty())
                    xmlWriter().writeAttribute("outputclass",attribute);
                writeStartTag(DT_apiName);
                writeCharacters(fn->name());
                writeEndTag(); // </apiName>
                generateBrief(fn,marker);

                // not included: <prolog>

                writeStartTag(DT_cxxDefineDetail);
                writeStartTag(DT_cxxDefineDefinition);
                writeStartTag(DT_cxxDefineAccessSpecifier);
                xmlWriter().writeAttribute("value",fn->accessString());
                writeEndTag(); // <cxxDefineAccessSpecifier>

                writeStartTag(DT_cxxDefinePrototype);
                xmlWriter().writeCharacters("#define ");
                writeCharacters(fn->name());
                if (fn->metaness() == FunctionNode::MacroWithParams) {
                    QStringList params = fn->parameterNames();
                    if (!params.isEmpty()) {
                        xmlWriter().writeCharacters("(");
                        for (int i = 0; i < params.size(); ++i) {
                            if (params[i].isEmpty())
                                xmlWriter().writeCharacters("...");
                            else
                                writeCharacters(params[i]);
                            if ((i+1) < params.size())
                                xmlWriter().writeCharacters(", ");
                        }
                        xmlWriter().writeCharacters(")");
                    }
                }
                writeEndTag(); // <cxxDefinePrototype>

                writeStartTag(DT_cxxDefineNameLookup);
                writeCharacters(fn->name());
                writeEndTag(); // <cxxDefineNameLookup>

                if (fn->reimplementedFrom() != 0) {
                    FunctionNode* rfn = (FunctionNode*)fn->reimplementedFrom();
                    writeStartTag(DT_cxxDefineReimplemented);
                    xmlWriter().writeAttribute("href",ditaXmlHref(rfn));
                    writeCharacters(rfn->plainFullName());
                    writeEndTag(); // </cxxDefineReimplemented>
                }

                if (fn->metaness() == FunctionNode::MacroWithParams) {
                    QStringList params = fn->parameterNames();
                    if (!params.isEmpty()) {
                        writeStartTag(DT_cxxDefineParameters);
                        for (int i = 0; i < params.size(); ++i) {
                            writeStartTag(DT_cxxDefineParameter);
                            writeStartTag(DT_cxxDefineParameterDeclarationName);
                            writeCharacters(params[i]);
                            writeEndTag(); // <cxxDefineParameterDeclarationName>

                            // not included: <apiDefNote>

                            writeEndTag(); // <cxxDefineParameter>
                        }
                        writeEndTag(); // <cxxDefineParameters>
                    }
                }

                writeLocation(fn);
                writeEndTag(); // <cxxDefineDefinition>

                writeApiDesc(fn, marker, QString());

                // not included: <example> or <apiImpl>

                writeEndTag(); // </cxxDefineDetail>

                // not included: <related-links>

                writeEndTag(); // </cxxDefine>
            }
        }
        ++m;
    }
}

/*!
  This function writes one parameter of a Q_PROPERTY macro.
  The property is identified by \a tag ("READ" "WRIE" etc),
  and it is found in the 'a nlist.
 */
void DitaXmlGenerator::writePropertyParameter(const QString& tag, const NodeList& nlist)
{
    NodeList::const_iterator n = nlist.constBegin();
    while (n != nlist.constEnd()) {
        xmlWriter().writeCharacters(" ");
        writeCharacters(tag);
        xmlWriter().writeCharacters(" ");
        writeCharacters((*n)->name());
        ++n;
    }
}

/*!
  Calls beginSubPage() in the base class to open the file.
  Then creates a new XML stream writer using the IO device
  from opened file and pushes the XML writer onto a stackj.
  Creates the file named \a fileName in the output directory.
  Attaches a QTextStream to the created file, which is written
  to all over the place using out(). Finally, it sets some
  parameters in the XML writer and calls writeStartDocument().

  It also ensures that a GUID map is created for the output file.
 */
void DitaXmlGenerator::beginSubPage(const InnerNode* node,
                                    const QString& fileName)
{
    Generator::beginSubPage(node,fileName);
    (void) lookupGuidMap(fileName);
    QXmlStreamWriter* writer = new QXmlStreamWriter(out().device());
    xmlWriterStack.push(writer);
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(4);
    writer->writeStartDocument();
    clearSectionNesting();
}

/*!
  Calls writeEndDocument() and then pops the XML stream writer
  off the stack and deletes it. Then it calls endSubPage() in
  the base class to close the device.
 */
void DitaXmlGenerator::endSubPage()
{
    if (inSection())
        qDebug() << "Missing </section> in" << outFileName() << sectionNestingLevel;
    xmlWriter().writeEndDocument();
    delete xmlWriterStack.pop();
    Generator::endSubPage();
}

/*!
  Returns a reference to the XML stream writer currently in use.
  There is one XML stream writer open for each XML file being
  written, and they are kept on a stack. The one on top of the
  stack is the one being written to at the moment.
 */
QXmlStreamWriter& DitaXmlGenerator::xmlWriter()
{
    return *xmlWriterStack.top();
}

/*!
  Writes the \e {<apiDesc>} element for \a node to the current XML
  stream using the code \a marker and the \a title.
 */
void DitaXmlGenerator::writeApiDesc(const Node* node,
                                    CodeMarker* marker,
                                    const QString& title)
{
    if (!node->doc().isEmpty()) {
        inDetailedDescription = true;
        enterDesc(DT_apiDesc,QString(),title);
        generateBody(node, marker);
        generateAlsoList(node, marker);
        leaveSection();
    }
    inDetailedDescription = false;
}

/*!
  Write the nested class elements.
 */
void DitaXmlGenerator::writeNestedClasses(const Section& s,
                                          const Node* n)
{
    if (s.members.isEmpty())
        return;
    writeStartTag(DT_cxxClassNested);
    writeStartTag(DT_cxxClassNestedDetail);

    NodeList::ConstIterator m = s.members.constBegin();
    while (m != s.members.constEnd()) {
        if ((*m)->type() == Node::Class) {
            writeStartTag(DT_cxxClassNestedClass);
            QString link = linkForNode((*m), n);
            xmlWriter().writeAttribute("href", link);
            QString name = n->name() + "::" + (*m)->name();
            writeCharacters(name);
            writeEndTag(); // <cxxClassNestedClass>
        }
        ++m;
    }
    writeEndTag(); // <cxxClassNestedDetail>
    writeEndTag(); // <cxxClassNested>
}

/*!
  Recursive writing of DITA XML files from the root \a node.
 */
void
DitaXmlGenerator::generateInnerNode(InnerNode* node)
{
    if (!node->url().isNull())
        return;

    if (node->type() == Node::Document) {
        DocNode* docNode = static_cast<DocNode*>(node);
        if (docNode->subType() == Node::ExternalPage)
            return;
        if (docNode->subType() == Node::Image)
            return;
        if (docNode->subType() == Node::Page) {
            if (node->count() > 0)
                qDebug("PAGE %s HAS CHILDREN", qPrintable(docNode->title()));
        }
    }
    else if (node->type() == Node::QmlPropertyGroup)
        return;

    /*
      Obtain a code marker for the source file.
     */
    CodeMarker *marker = CodeMarker::markerForFileName(node->location().filePath());
    if (node->parent() != 0) {
        /*
          Skip name collision nodes here and process them
          later in generateCollisionPages(). Each one is
          appended to a list for later.
         */
        if ((node->type() == Node::Document) && (node->subType() == Node::Collision)) {
            NameCollisionNode* ncn = static_cast<NameCollisionNode*>(node);
            collisionNodes.append(const_cast<NameCollisionNode*>(ncn));
        }
        else {
            if (!node->name().endsWith(".ditamap"))
                beginSubPage(node, fileName(node));
            if (node->type() == Node::Namespace || node->type() == Node::Class) {
                generateClassLikeNode(node, marker);
            }
            else if (node->type() == Node::Document) {
                if (node->subType() == Node::HeaderFile)
                    generateClassLikeNode(node, marker);
                else if (node->subType() == Node::QmlClass)
                    generateClassLikeNode(node, marker);
                else
                    generateDocNode(static_cast<DocNode*>(node), marker);
            }
            if (!node->name().endsWith(".ditamap"))
                endSubPage();
        }
    }

    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->isInnerNode() && (*c)->access() != Node::Private)
            generateInnerNode((InnerNode*)*c);
        ++c;
    }
}

/*!
  Returns \c true if \a format is "DITAXML" or "HTML" .
 */
bool DitaXmlGenerator::canHandleFormat(const QString& format)
{
    return (format == "HTML") || (format == this->format());
}

/*!
  If the node multimap \a nmm contains nodes mapped to \a key,
  if any of the nodes mapped to \a key has the same href as the
  \a node, return true. Otherwise, return false.
 */
bool DitaXmlGenerator::isDuplicate(NodeMultiMap* nmm, const QString& key, Node* node)
{
    QList<Node*> matches = nmm->values(key);
    if (!matches.isEmpty()) {
        for (int i=0; i<matches.size(); ++i) {
            if (matches[i] == node)
                return true;
            if (fileName(node) == fileName(matches[i]))
                return true;
        }
    }
    return false;
}
/*!
  Collect all the nodes in the tree according to their type or subtype.

  If a node is found that is named index.html, return that node as the
  root page node.

  type: Class
  type: Namespace

  subtype: Example
  subtype: External page
  subtype: Group
  subtype: Header file
  subtype: Module
  subtype: Page
  subtype: QML basic type
  subtype: QML class
  subtype: QML module
 */
Node* DitaXmlGenerator::collectNodesByTypeAndSubtype(const InnerNode* parent)
{
    Node* rootPageNode = 0;
    const NodeList& children = parent->childNodes();
    if (children.size() == 0)
        return rootPageNode;

    QString message;
    for (int i=0; i<children.size(); ++i) {
        Node* child = children[i];
        if ((child->type() == Node::Document) && (child->subType() == Node::Collision)) {
            const DocNode* fake = static_cast<const DocNode*>(child);
            Node* n = collectNodesByTypeAndSubtype(fake);
            if (n)
                rootPageNode = n;
            continue;
        }
        if (!child || child->isInternal() || child->doc().isEmpty() || child->isIndexNode())
            continue;

        if (child->name() == "index.html" || child->name() == "index") {
            rootPageNode = child;
        }

        switch (child->type()) {
        case Node::Namespace:
            if (!isDuplicate(nodeTypeMaps[Node::Namespace],child->name(),child))
                nodeTypeMaps[Node::Namespace]->insert(child->name(),child);
            break;
        case Node::Class:
            if (!isDuplicate(nodeTypeMaps[Node::Class],child->name(),child))
                nodeTypeMaps[Node::Class]->insert(child->name(),child);
            break;
        case Node::Document:
            switch (child->subType()) {
            case Node::Example:
                if (!isDuplicate(nodeSubtypeMaps[Node::Example],child->title(),child))
                    nodeSubtypeMaps[Node::Example]->insert(child->title(),child);
                break;
            case Node::HeaderFile:
                if (!isDuplicate(nodeSubtypeMaps[Node::HeaderFile],child->title(),child))
                    nodeSubtypeMaps[Node::HeaderFile]->insert(child->title(),child);
                break;
            case Node::File:
                break;
            case Node::Image:
                break;
            case Node::Group:
                if (!isDuplicate(nodeSubtypeMaps[Node::Group],child->title(),child))
                    nodeSubtypeMaps[Node::Group]->insert(child->title(),child);
                break;
            case Node::Module:
                if (!isDuplicate(nodeSubtypeMaps[Node::Module],child->title(),child))
                    nodeSubtypeMaps[Node::Module]->insert(child->title(),child);
                break;
            case Node::Page:
                if (!isDuplicate(pageTypeMaps[child->pageType()],child->title(),child))
                    pageTypeMaps[child->pageType()]->insert(child->title(),child);
                break;
            case Node::ExternalPage:
                if (!isDuplicate(nodeSubtypeMaps[Node::ExternalPage],child->title(),child))
                    nodeSubtypeMaps[Node::ExternalPage]->insert(child->title(),child);
                break;
            case Node::QmlClass:
                if (!isDuplicate(nodeSubtypeMaps[Node::QmlClass],child->title(),child))
                    nodeSubtypeMaps[Node::QmlClass]->insert(child->title(),child);
                break;
            case Node::QmlBasicType:
                if (!isDuplicate(nodeSubtypeMaps[Node::QmlBasicType],child->title(),child))
                    nodeSubtypeMaps[Node::QmlBasicType]->insert(child->title(),child);
                break;
            case Node::QmlModule:
                if (!isDuplicate(nodeSubtypeMaps[Node::QmlModule],child->title(),child))
                    nodeSubtypeMaps[Node::QmlModule]->insert(child->title(),child);
                break;
            case Node::Collision:
                if (!isDuplicate(nodeSubtypeMaps[Node::Collision],child->title(),child))
                    nodeSubtypeMaps[Node::Collision]->insert(child->title(),child);
                break;
            default:
                break;
            }
            break;
        case Node::Enum:
            break;
        case Node::Typedef:
            break;
        case Node::Function:
            break;
        case Node::Property:
            break;
        case Node::Variable:
            break;
        case Node::QmlProperty:
            break;
        case Node::QmlPropertyGroup:
            break;
        case Node::QmlSignal:
            break;
        case Node::QmlSignalHandler:
            break;
        case Node::QmlMethod:
            break;
        default:
            break;
        }
    }
    return rootPageNode;
}

/*!
  Creates the DITA map for the qdoc run. The map is written
  to the file \e{qt.ditamap" in the DITA XML output directory.
 */
void DitaXmlGenerator::writeDitaMap()
{
    QString doctype;

/*
    Remove #if 0 to get a flat ditamap.
*/
#if 0
    beginSubPage(qdb_->treeRoot(),"qt.ditamap");
    doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";
    xmlWriter().writeDTD(doctype);
    writeStartTag(DT_map);
    writeStartTag(DT_topicmeta);
    writeStartTag(DT_shortdesc);
    xmlWriter().writeCharacters("The top level map for the Qt documentation");
    writeEndTag(); // </shortdesc>
    writeEndTag(); // </topicmeta>
    GuidMaps::iterator i = guidMaps.constBegin();
    while (i != guidMaps.constEnd()) {
        writeStartTag(DT_topicref);
        if (i.key() != "qt.ditamap")
            xmlWriter().writeAttribute("href",i.key());
        writeEndTag(); // </topicref>
        ++i;
    }
    endSubPage();
#endif

    for (unsigned i=0; i<Node::LastType; ++i)
        nodeTypeMaps[i] = new NodeMultiMap;
    for (unsigned i=0; i<Node::LastSubtype; ++i)
        nodeSubtypeMaps[i] = new NodeMultiMap;
    for (unsigned i=0; i<Node::OnBeyondZebra; ++i)
        pageTypeMaps[i] = new NodeMultiMap;
    Node* rootPageNode = collectNodesByTypeAndSubtype(qdb_->treeRoot());

    beginSubPage(qdb_->treeRoot(),"qt.ditamap");

    doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";
    xmlWriter().writeDTD(doctype);
    writeStartTag(DT_map);
    writeStartTag(DT_topicmeta);
    writeStartTag(DT_shortdesc);
    xmlWriter().writeCharacters("The top level map for the Qt documentation");
    writeEndTag(); // </shortdesc>
    writeEndTag(); // </topicmeta>

    writeStartTag(DT_topicref);
    if (rootPageNode) {
        if (!rootPageNode->title().isEmpty())
            xmlWriter().writeAttribute("navtitle",rootPageNode->title());
        else
            xmlWriter().writeAttribute("navtitle",project);
        xmlWriter().writeAttribute("href",fileName(rootPageNode));
    }
    else
        xmlWriter().writeAttribute("navtitle",project);

    writeTopicrefs(pageTypeMaps[Node::OverviewPage], "Overviews");
    writeTopicrefs(pageTypeMaps[Node::HowToPage], "Howtos");
    writeTopicrefs(pageTypeMaps[Node::TutorialPage], "Tutorials");
    writeTopicrefs(pageTypeMaps[Node::FAQPage], "FAQs");
    writeTopicrefs(pageTypeMaps[Node::ArticlePage], "Articles");
    writeTopicrefs(nodeSubtypeMaps[Node::Example], "Examples");
    if (nodeSubtypeMaps[Node::QmlModule]->size() > 1)
        writeTopicrefs(nodeSubtypeMaps[Node::QmlModule], "QML modules");
    if (nodeSubtypeMaps[Node::QmlModule]->size() == 1)
        writeTopicrefs(nodeSubtypeMaps[Node::QmlClass], "QML types", nodeSubtypeMaps[Node::QmlModule]->values()[0]);
    else
        writeTopicrefs(nodeSubtypeMaps[Node::QmlClass], "QML types");
    writeTopicrefs(nodeSubtypeMaps[Node::QmlBasicType], "QML basic types");
    if (nodeSubtypeMaps[Node::Module]->size() > 1)
        writeTopicrefs(nodeSubtypeMaps[Node::Module], "Modules");
    if (nodeSubtypeMaps[Node::Module]->size() == 1)
        writeTopicrefs(nodeTypeMaps[Node::Class], "C++ classes", nodeSubtypeMaps[Node::Module]->values()[0]);
    else
        writeTopicrefs(nodeTypeMaps[Node::Class], "C++ classes");
    writeTopicrefs(nodeTypeMaps[Node::Namespace], "C++ namespaces");
    writeTopicrefs(nodeSubtypeMaps[Node::HeaderFile], "Header files");
    writeTopicrefs(nodeSubtypeMaps[Node::Group], "Groups");

    writeEndTag(); // </topicref>
    endSubPage();

    for (unsigned i=0; i<Node::LastType; ++i)
        delete nodeTypeMaps[i];
    for (unsigned i=0; i<Node::LastSubtype; ++i)
        delete nodeSubtypeMaps[i];
    for (unsigned i=0; i<Node::OnBeyondZebra; ++i)
        delete pageTypeMaps[i];
}

/*!
  Creates the DITA map from the topicrefs in \a node,
  which is a DitaMapNode.
 */
void DitaXmlGenerator::writeDitaMap(const DitaMapNode* node)
{
    beginSubPage(node,node->name());

    QString doctype;
    doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";
    xmlWriter().writeDTD(doctype);
    writeStartTag(DT_map);
    writeStartTag(DT_topicmeta);
    writeStartTag(DT_shortdesc);
    xmlWriter().writeCharacters(node->title());
    writeEndTag(); // </shortdesc>
    writeEndTag(); // </topicmeta>
    const DitaRefList map = node->map();
    writeDitaRefs(map);
    endSubPage();
}

/*!
  Write the \a ditarefs to the current output file.
 */
void DitaXmlGenerator::writeDitaRefs(const DitaRefList& ditarefs)
{
    foreach (DitaRef* t, ditarefs) {
        if (t->isMapRef())
            writeStartTag(DT_mapref);
        else
            writeStartTag(DT_topicref);
        xmlWriter().writeAttribute("navtitle",t->navtitle());
        if (t->href().isEmpty()) {
            const DocNode* dn = qdb_->findDocNodeByTitle(t->navtitle());
            if (dn)
                xmlWriter().writeAttribute("href",fileName(dn));
        }
        else
            xmlWriter().writeAttribute("href",t->href());
        if (t->subrefs() && !t->subrefs()->isEmpty())
            writeDitaRefs(*(t->subrefs()));
        writeEndTag(); // </topicref> or </mapref>
    }
}

void DitaXmlGenerator::writeTopicrefs(NodeMultiMap* nmm, const QString& navtitle, Node* headingnode)
{
    if (!nmm || nmm->isEmpty())
        return;
    writeStartTag(DT_topicref);
    xmlWriter().writeAttribute("navtitle",navtitle);
    if (headingnode)
        xmlWriter().writeAttribute("href",fileName(headingnode));
    NodeMultiMap::iterator i;
    NodeMultiMap *ditaMaps = pageTypeMaps[Node::DitaMapPage];

    /*!
       Put all pages that are already in a hand-written ditamap not in
       the qt.ditamap separately. It loops through all ditamaps recursively
       before deciding to write an article to qt.ditamap.
     */
    if ((navtitle == "Articles" && ditaMaps && ditaMaps->size() > 0)) {
        NodeMultiMap::iterator mapIterator = ditaMaps->begin();
        while (mapIterator != ditaMaps->end()) {
            writeStartTag(DT_mapref);
            xmlWriter().writeAttribute("navtitle",mapIterator.key());
            xmlWriter().writeAttribute("href",fileName(mapIterator.value()));
            writeEndTag();
            ++mapIterator;
        }
        i = nmm->begin();
        while (i != nmm->end()) {
            // Hardcode not writing index.dita multiple times in the tree.
            // index.dita should only appear at the top of the ditamap.
            if (fileName(i.value()) == "index.dita") {
                i++;
                continue;
            }
            bool foundInDitaMap = false;
            mapIterator = ditaMaps->begin();
            while (mapIterator != ditaMaps->end()) {
                const DitaMapNode *dmNode = static_cast<const DitaMapNode *>(mapIterator.value());
                for (int count = 0; count < dmNode->map().count(); count++) {
                    if (dmNode->map().at(count)->navtitle() == i.key()) {
                        foundInDitaMap = true;
                        break;
                    }
                }
                ++mapIterator;
            }
            if (!foundInDitaMap) {
                writeStartTag(DT_topicref);
                xmlWriter().writeAttribute("navtitle",i.key());
                xmlWriter().writeAttribute("href",fileName(i.value()));
                writeEndTag(); // </topicref>
            }
            ++i;
        }
    }
    /*!
       Shortcut when there are no hand-written ditamaps or when we are
       not generating the articles list.
     */
    else {
        i = nmm->begin();
        while (i != nmm->end()) {
            // Hardcode not writing index.dita multiple times in the tree.
            // index.dita should only appear at the top of the ditamap.
            if (fileName(i.value()) == "index.dita") {
                i++;
                continue;
            }
            writeStartTag(DT_topicref);
            xmlWriter().writeAttribute("navtitle",i.key());
            xmlWriter().writeAttribute("href",fileName(i.value()));
            switch (i.value()->type()) {
                case Node::Class:
                case Node::Namespace: {
                    const NamespaceNode* nn = static_cast<const NamespaceNode*>(i.value());
                    const NodeList& c = nn->childNodes();
                    QMap<QString, const Node*> tempMap;
                    for (int j=0; j<c.size(); ++j) {
                        if (c[j]->isInternal() || c[j]->access() == Node::Private || c[j]->doc().isEmpty())
                            continue;
                        if (c[j]->type() == Node::Class) {
                            tempMap.insert(c[j]->name(), c[j]);
                        }
                    }
                    QMap<QString, const Node*>::iterator classIterator = tempMap.begin();
                    while (classIterator != tempMap.end()) {
                        const Node* currentNode = static_cast<const Node*>(classIterator.value());
                        writeStartTag(DT_topicref);
                        xmlWriter().writeAttribute("navtitle",currentNode->name());
                        xmlWriter().writeAttribute("href",fileName(currentNode));
                        writeEndTag(); // </topicref>
                        ++classIterator;
                    }

                    break;
                }
                default:
                    break;
            }
            writeEndTag(); // </topicref>
            ++i;
        }
    }
    writeEndTag(); // </topicref>
}


/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If a value
  for the tag is found, the element is written with the
  found value. Otherwise if \a force is set, an empty
  element is written using the tag.

  Returns \c true or false depending on whether it writes
  an element using the tag \a t.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
bool DitaXmlGenerator::writeMetadataElement(const InnerNode* inner,
                                            DitaXmlGenerator::DitaTag t,
                                            bool force)
{
    QString s = getMetadataElement(inner,t);
    if (s.isEmpty() && !force)
        return false;
    writeStartTag(t);
    if (!s.isEmpty())
        xmlWriter().writeCharacters(s);
    writeEndTag();
    return true;
}


/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If one or more
  value sfor the tag are found, the elements are written.
  Otherwise nothing is written.

  Returns \c true or false depending on whether it writes
  at least one element using the tag \a t.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
bool DitaXmlGenerator::writeMetadataElements(const InnerNode* inner,
                                             DitaXmlGenerator::DitaTag t)
{
    QStringList s = getMetadataElements(inner,t);
    if (s.isEmpty())
        return false;
    for (int i=0; i<s.size(); ++i) {
        writeStartTag(t);
        xmlWriter().writeCharacters(s[i]);
        writeEndTag();
    }
    return true;
}

/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If a value
  for the tag is found, the value is returned.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
QString DitaXmlGenerator::getMetadataElement(const InnerNode* inner, DitaXmlGenerator::DitaTag t)
{
    QString s = Generator::getMetadataElement(inner, ditaTags[t]);
    if (s.isEmpty()) {
        QStringList sl = metadataDefault(t);
        if (!sl.isEmpty())
            s = sl[0];
    }
    return s;
}

/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If values
  for the tag are found, they are returned in a string
  list.

  \note If \a t is found in the metadata map, all the
  pairs having the key \a t are erased. i.e. Once you
  all this function for a particular \a t, you consume
  \a t.
 */
QStringList DitaXmlGenerator::getMetadataElements(const InnerNode* inner,
                                                  DitaXmlGenerator::DitaTag t)
{
    QStringList s = Generator::getMetadataElements(inner,ditaTags[t]);
    if (s.isEmpty())
        s = metadataDefault(t);
    return s;
}

/*!
  Returns the value of key \a t or an empty string
  if \a t is not found in the map.
 */
QStringList DitaXmlGenerator::metadataDefault(DitaTag t) const
{
    return metadataDefaults.value(ditaTags[t]).values_;
}

/*!
  Writes the <prolog> element for the \a inner node
  using the \a marker. The <prolog> element contains
  the <metadata> element, plus some others. This
  function writes one or more of these elements:

  \list
    \o <audience> *
    \o <author> *
    \o <brand> not used
    \o <category> *
    \o <compomnent> *
    \o <copyrholder> *
    \o <copyright> *
    \o <created> not used
    \o <copyryear> *
    \o <critdates> not used
    \o <keyword> not used
    \o <keywords> not used
    \o <metadata> *
    \o <othermeta> *
    \o <permissions> *
    \o <platform> not used
    \o <prodinfo> *
    \o <prodname> *
    \o <prolog> *
    \o <publisher> *
    \o <resourceid> not used
    \o <revised> not used
    \o <source> not used
    \o <tm> not used
    \o <unknown> not used
    \o <vrm> *
    \o <vrmlist> *
  \endlist

  \node * means the tag has been used.

 */
void
DitaXmlGenerator::writeProlog(const InnerNode* inner)
{
    if (!inner)
        return;
    writeStartTag(DT_prolog);
    writeMetadataElements(inner,DT_author);
    writeMetadataElement(inner,DT_publisher);
    QString s = getMetadataElement(inner,DT_copyryear);
    QString t = getMetadataElement(inner,DT_copyrholder);
    writeStartTag(DT_copyright);
    writeStartTag(DT_copyryear);
    if (!s.isEmpty())
        xmlWriter().writeAttribute("year",s);
    writeEndTag(); // </copyryear>
    writeStartTag(DT_copyrholder);
    if (!s.isEmpty())
        xmlWriter().writeCharacters(t);
    writeEndTag(); // </copyrholder>
    writeEndTag(); // </copyright>
    s = getMetadataElement(inner,DT_permissions);
    writeStartTag(DT_permissions);
    xmlWriter().writeAttribute("view",s);
    writeEndTag(); // </permissions>
    writeStartTag(DT_metadata);
    QStringList sl = getMetadataElements(inner,DT_audience);
    if (!sl.isEmpty()) {
        for (int i=0; i<sl.size(); ++i) {
            writeStartTag(DT_audience);
            xmlWriter().writeAttribute("type",sl[i]);
            writeEndTag(); // </audience>
        }
    }
    if (!writeMetadataElement(inner,DT_category,false)) {
        writeStartTag(DT_category);
        QString category = "Page";
        if (inner->type() == Node::Class)
            category = "Class reference";
        else if (inner->type() == Node::Namespace)
            category = "Namespace";
        else if (inner->type() == Node::Document) {
            if (inner->subType() == Node::QmlClass)
                category = "QML Reference";
            else if (inner->subType() == Node::QmlBasicType)
                category = "QML Basic Type";
            else if (inner->subType() == Node::HeaderFile)
                category = "Header File";
            else if (inner->subType() == Node::Module)
                category = "Module";
            else if (inner->subType() == Node::File)
                category = "Example Source File";
            else if (inner->subType() == Node::Example)
                category = "Example";
            else if (inner->subType() == Node::Image)
                category = "Image";
            else if (inner->subType() == Node::Group)
                category = "Group";
            else if (inner->subType() == Node::Page)
                category = "Page";
            else if (inner->subType() == Node::ExternalPage)
                category = "External Page"; // Is this necessary?
        }
        xmlWriter().writeCharacters(category);
        writeEndTag(); // </category>
    }
    if (vrm.size() > 0) {
        writeStartTag(DT_prodinfo);
        if (!writeMetadataElement(inner,DT_prodname,false)) {
            writeStartTag(DT_prodname);
            xmlWriter().writeCharacters(projectDescription);
            writeEndTag(); // </prodname>
        }
        writeStartTag(DT_vrmlist);
        writeStartTag(DT_vrm);
        if (vrm.size() > 0)
            xmlWriter().writeAttribute("version",vrm[0]);
        if (vrm.size() > 1)
            xmlWriter().writeAttribute("release",vrm[1]);
        if (vrm.size() > 2)
            xmlWriter().writeAttribute("modification",vrm[2]);
        writeEndTag(); // <vrm>
        writeEndTag(); // <vrmlist>
        if (!writeMetadataElement(inner,DT_component,false)) {
            QString component = inner->moduleName();
            if (!component.isEmpty()) {
                writeStartTag(DT_component);
                xmlWriter().writeCharacters(component);
                writeEndTag(); // </component>
            }
        }
        writeEndTag(); // </prodinfo>
    }
    const QStringMultiMap& metaTagMap = inner->doc().metaTagMap();
    QMapIterator<QString, QString> i(metaTagMap);
    while (i.hasNext()) {
        i.next();
        writeStartTag(DT_othermeta);
        xmlWriter().writeAttribute("name",i.key());
        xmlWriter().writeAttribute("content",i.value());
        writeEndTag(); // </othermeta>
    }
    if ((tagStack.first() == DT_cxxClass && !inner->includes().isEmpty()) ||
        (inner->type() == Node::Document && inner->subType() == Node::HeaderFile)) {
        writeStartTag(DT_othermeta);
        xmlWriter().writeAttribute("name","includeFile");
        QString text;
        QStringList::ConstIterator i = inner->includes().constBegin();
        while (i != inner->includes().constEnd()) {
            if ((*i).startsWith(QLatin1Char('<')) && (*i).endsWith(QLatin1Char('>')))
                text += *i;
            else
                text += QLatin1Char('<') + *i + QLatin1Char('>');
            ++i;
            if (i != inner->includes().constEnd())
                text += "\n";
        }
        xmlWriter().writeAttribute("content",text);
        writeEndTag(); // </othermeta>
    }
    writeEndTag(); // </metadata>
    writeEndTag(); // </prolog>
}

/*!
  This function should be called to write the \a href attribute
  if the href could be an \e http or \e ftp link. If \a href is
  one or the other, a \e scope attribute is also writen, with
  value \e external.
 */
void DitaXmlGenerator::writeHrefAttribute(const QString& href)
{
    xmlWriter().writeAttribute("href", href);
    if (href.startsWith("http:") || href.startsWith("ftp:") ||
            href.startsWith("https:") || href.startsWith("mailto:"))
        xmlWriter().writeAttribute("scope", "external");
}

/*!
  Strips the markup tags from \a src, when we are trying to
  create an \e{id} attribute. Returns the stripped text.
 */
QString DitaXmlGenerator::stripMarkup(const QString& src) const
{
    QString text;
    const QChar charAt = '@';
    const QChar charSlash = '/';
    const QChar charLangle = '<';
    const QChar charRangle = '>';

    int n = src.size();
    int i = 0;
    while (i < n) {
        if (src.at(i) == charLangle) {
            ++i;
            if (src.at(i) == charAt || (src.at(i) == charSlash && src.at(i+1) == charAt)) {
                while (i < n && src.at(i) != charRangle)
                    ++i;
                ++i;
            }
            else {
                text += charLangle;
            }
        }
        else
            text += src.at(i++);
    }
    return text;
}

/*!
  We delayed generation of the collision pages until now, after
  all the other pages have been generated. We do this because we might
  encounter a link command that tries to link to a target on a QML
  type page, but the link doesn't specify the module identifer
  for the QML type, and the QML type name without a module
  identifier is ambiguous. When such a link is found, qdoc can't find
  the target, so it appends the target to the NameCollisionNode. After
  the tree has been traversed and all these ambiguous links have been
  added to the name collision nodes, this function is called. The list
  of collision nodes is traversed here, and the collision page for
  each collision is generated. The collision page will not only
  disambiguate links to the QML type pages, but it will also disambiguate
  links to properties, section headers, etc.
 */
void DitaXmlGenerator::generateCollisionPages()
{
    if (collisionNodes.isEmpty())
        return;

    for (int i=0; i<collisionNodes.size(); ++i) {
        NameCollisionNode* ncn = collisionNodes.at(i);
        if (!ncn)
            continue;

        NodeList collisions;
        const NodeList& nl = ncn->childNodes();
        if (!nl.isEmpty()) {
            NodeList::ConstIterator it = nl.constBegin();
            while (it != nl.constEnd()) {
                if (!(*it)->isInternal())
                    collisions.append(*it);
                ++it;
            }
        }
        if (collisions.size() <= 1)
            continue;

        ncn->clearCurrentChild();
        beginSubPage(ncn, Generator::fileName(ncn));
        QString fullTitle = ncn->fullTitle();
        QString ditaTitle = fullTitle;
        CodeMarker* marker = CodeMarker::markerForFileName(ncn->location().filePath());
        if (ncn->isQmlNode()) {
            // Replace the marker with a QML code marker.
            if (ncn->isQmlNode())
                marker = CodeMarker::markerForLanguage(QLatin1String("QML"));
        }

        generateHeader(ncn, ditaTitle);
        writeProlog(ncn);
        writeStartTag(DT_body);
        enterSection(QString(), QString());

        NodeMap nm;
        for (int i=0; i<collisions.size(); ++i) {
            Node* n = collisions.at(i);
            QString t;
            if (!n->qmlModuleName().isEmpty())
                t = n->qmlModuleName() + QLatin1Char(' ');
            t += protectEnc(fullTitle);
            nm.insertMulti(t,n);
        }
        generateAnnotatedList(ncn, marker, nm);

        QList<QString> targets;
        if (!ncn->linkTargets().isEmpty()) {
            QMap<QString,QString>::ConstIterator t = ncn->linkTargets().constBegin();
            while (t != ncn->linkTargets().constEnd()) {
                int count = 0;
                for (int i=0; i<collisions.size(); ++i) {
                    InnerNode* n = static_cast<InnerNode*>(collisions.at(i));
                    if (n->findChildNodeByName(t.key())) {
                        ++count;
                        if (count > 1) {
                            targets.append(t.key());
                            break;
                        }
                    }
                }
                ++t;
            }
        }
        if (!targets.isEmpty()) {
            QList<QString>::ConstIterator t = targets.constBegin();
            while (t != targets.constEnd()) {
                writeStartTag(DT_p);
                writeGuidAttribute(Doc::canonicalTitle(*t));
                xmlWriter().writeAttribute("outputclass","h2");
                writeCharacters(protectEnc(*t));
                writeEndTag(); // </p>
                writeStartTag(DT_ul);
                for (int i=0; i<collisions.size(); ++i) {
                    InnerNode* n = static_cast<InnerNode*>(collisions.at(i));
                    Node* p = n->findChildNodeByName(*t);
                    if (p) {
                        QString link = linkForNode(p,0);
                        QString label;
                        if (!n->qmlModuleName().isEmpty())
                            label = n->qmlModuleName() + "::";
                        label += n->name() + "::" + p->name();
                        writeStartTag(DT_li);
                        writeStartTag(DT_xref);
                        xmlWriter().writeAttribute("href", link);
                        writeCharacters(protectEnc(label));
                        writeEndTag(); // </xref>
                        writeEndTag(); // </li>
                    }
                }
                writeEndTag(); // </ul>
                ++t;
            }
        }
        leaveSection(); // </section>
        writeEndTag(); // </body>
        endSubPage();
    }
}

QT_END_NAMESPACE

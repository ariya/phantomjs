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
  htmlgenerator.cpp
*/

#include "codemarker.h"
#include "codeparser.h"
#include "helpprojectwriter.h"
#include "htmlgenerator.h"
#include "node.h"
#include "qdocdatabase.h"
#include "separator.h"
#include "tree.h"
#include <ctype.h>
#include <qdebug.h>
#include <qlist.h>
#include <qiterator.h>
#include <qtextcodec.h>
#include <quuid.h>

QT_BEGIN_NAMESPACE

#define COMMAND_VERSION                 Doc::alias("version")
int HtmlGenerator::id = 0;
bool HtmlGenerator::debugging_on = false;

QString HtmlGenerator::divNavTop;

static bool showBrokenLinks = false;

static QRegExp linkTag("(<@link node=\"([^\"]+)\">).*(</@link>)");
static QRegExp funcTag("(<@func target=\"([^\"]*)\">)(.*)(</@func>)");
static QRegExp typeTag("(<@(type|headerfile|func)(?: +[^>]*)?>)(.*)(</@\\2>)");
static QRegExp spanTag("</@(?:comment|preprocessor|string|char|number|op|type|name|keyword)>");
static QRegExp unknownTag("</?@[^>]*>");

static void addLink(const QString &linkTarget,
                    const QStringRef &nestedStuff,
                    QString *res)
{
    if (!linkTarget.isEmpty()) {
        *res += "<a href=\"";
        *res += linkTarget;
        *res += "\">";
        *res += nestedStuff;
        *res += "</a>";
    }
    else {
        *res += nestedStuff;
    }
}

/*!
  Constructs the HTML output generator.
 */
HtmlGenerator::HtmlGenerator()
    : codeIndent(0),
      helpProjectWriter(0),
      inObsoleteLink(false),
      funcLeftParen("\\S(\\()"),
      obsoleteLinks(false)
{
}

/*!
  Destroys the HTML output generator. Deletes the singleton
  instance of HelpProjectWriter.
 */
HtmlGenerator::~HtmlGenerator()
{
    if (helpProjectWriter)
        delete helpProjectWriter;
}

/*!
  Initializes the HTML output generator's data structures
  from the configuration class \a config.
 */
void HtmlGenerator::initializeGenerator(const Config &config)
{
    static const struct {
        const char *key;
        const char *left;
        const char *right;
    } defaults[] = {
        { ATOM_FORMATTING_BOLD, "<b>", "</b>" },
        { ATOM_FORMATTING_INDEX, "<!--", "-->" },
        { ATOM_FORMATTING_ITALIC, "<i>", "</i>" },
        { ATOM_FORMATTING_PARAMETER, "<i>", "</i>" },
        { ATOM_FORMATTING_SUBSCRIPT, "<sub>", "</sub>" },
        { ATOM_FORMATTING_SUPERSCRIPT, "<sup>", "</sup>" },
        { ATOM_FORMATTING_TELETYPE, "<tt>", "</tt>" },
        { ATOM_FORMATTING_UICONTROL, "<b>", "</b>" },
        { ATOM_FORMATTING_UNDERLINE, "<u>", "</u>" },
        { 0, 0, 0 }
    };

    Generator::initializeGenerator(config);
    obsoleteLinks = config.getBool(CONFIG_OBSOLETELINKS);
    setImageFileExtensions(QStringList() << "png" << "jpg" << "jpeg" << "gif");
    int i = 0;
    while (defaults[i].key) {
        formattingLeftMap().insert(defaults[i].key, defaults[i].left);
        formattingRightMap().insert(defaults[i].key, defaults[i].right);
        i++;
    }

    style = config.getString(HtmlGenerator::format() +
                             Config::dot +
                             CONFIG_STYLE);
    endHeader = config.getString(HtmlGenerator::format() +
                                 Config::dot +
                                 CONFIG_ENDHEADER);
    postHeader = config.getString(HtmlGenerator::format() +
                                  Config::dot +
                                  HTMLGENERATOR_POSTHEADER);
    postPostHeader = config.getString(HtmlGenerator::format() +
                                      Config::dot +
                                      HTMLGENERATOR_POSTPOSTHEADER);
    footer = config.getString(HtmlGenerator::format() +
                              Config::dot +
                              HTMLGENERATOR_FOOTER);
    address = config.getString(HtmlGenerator::format() +
                               Config::dot +
                               HTMLGENERATOR_ADDRESS);
    pleaseGenerateMacRef = config.getBool(HtmlGenerator::format() +
                                          Config::dot +
                                          HTMLGENERATOR_GENERATEMACREFS);
    noNavigationBar = config.getBool(HtmlGenerator::format() +
                                   Config::dot +
                                   HTMLGENERATOR_NONAVIGATIONBAR);

    project = config.getString(CONFIG_PROJECT);

    projectDescription = config.getString(CONFIG_DESCRIPTION);
    if (projectDescription.isEmpty() && !project.isEmpty())
        projectDescription = project + " Reference Documentation";

    projectUrl = config.getString(CONFIG_URL);
    tagFile_ = config.getString(CONFIG_TAGFILE);

#ifndef QT_NO_TEXTCODEC
    outputEncoding = config.getString(CONFIG_OUTPUTENCODING);
    if (outputEncoding.isEmpty())
        outputEncoding = QLatin1String("UTF-8");
    outputCodec = QTextCodec::codecForName(outputEncoding.toLocal8Bit());
#endif

    naturalLanguage = config.getString(CONFIG_NATURALLANGUAGE);
    if (naturalLanguage.isEmpty())
        naturalLanguage = QLatin1String("en");

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

    // The following line was changed to fix QTBUG-27798
    //codeIndent = config.getInt(CONFIG_CODEINDENT);

    helpProjectWriter = new HelpProjectWriter(config, project.toLower() + ".qhp", this);

    // Documentation template handling
    headerScripts = config.getString(HtmlGenerator::format() + Config::dot + CONFIG_HEADERSCRIPTS);
    headerStyles = config.getString(HtmlGenerator::format() + Config::dot + CONFIG_HEADERSTYLES);

    QString prefix = CONFIG_QHP + Config::dot + project + Config::dot;
    manifestDir = "qthelp://" + config.getString(prefix + "namespace");
    manifestDir += QLatin1Char('/') + config.getString(prefix + "virtualFolder") + QLatin1Char('/');
    readManifestMetaContent(config);
    examplesPath = config.getString(CONFIG_EXAMPLESINSTALLPATH);
    if (!examplesPath.isEmpty())
        examplesPath += QLatin1Char('/');

    //retrieve the config for the navigation bar
    homepage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_HOMEPAGE);

    landingpage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_LANDINGPAGE);

    cppclassespage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_CPPCLASSESPAGE);

    qmltypespage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_QMLTYPESPAGE);

    buildversion = config.getString(CONFIG_BUILDVERSION);
}

/*!
  Gracefully terminates the HTML output generator.
 */
void HtmlGenerator::terminateGenerator()
{
    Generator::terminateGenerator();
}

QString HtmlGenerator::format()
{
    return "HTML";
}

/*!
  Traverses the database generating all the HTML documentation.
 */
void HtmlGenerator::generateTree()
{
    qdb_->buildCollections();
    Node* qflags = qdb_->findNodeByNameAndType(QStringList("QFlags"), Node::Class, Node::NoSubType);
    if (qflags)
        qflagsHref_ = linkForNode(qflags,0);
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
        helpProjectWriter->generate();
        generateManifestFiles();
        /*
          Generate the XML tag file, if it was requested.
        */
        qdb_->generateTagFile(tagFile_, this);
    }
}

/*!
  Generate html from an instance of Atom.
 */
int HtmlGenerator::generateAtom(const Atom *atom, const Node *relative, CodeMarker *marker)
{
    int skipAhead = 0;
    static bool in_para = false;

    if (Generator::debugging()) {
        atom->dump();
    }
    switch (atom->type()) {
    case Atom::AbstractLeft:
        if (relative)
            relative->doc().location().warning(tr("\abstract is not implemented."));
        else
            Location::information(tr("\abstract is not implemented."));
        break;
    case Atom::AbstractRight:
        break;
    case Atom::AutoLink:
        if (!inLink_ && !inContents_ && !inSectionHeading_) {
            const Node *node = 0;
            QString link = getLink(atom, relative, &node);
            if (!link.isEmpty()) {
                beginLink(link, node, relative);
                generateLink(atom, marker);
                endLink();
            }
            else {
                out() << protectEnc(atom->string());
            }
        }
        else {
            out() << protectEnc(atom->string());
        }
        break;
    case Atom::BaseName:
        break;
    case Atom::BriefLeft:
        if (relative->type() == Node::Document) {
            if (relative->subType() != Node::Example) {
                skipAhead = skipAtoms(atom, Atom::BriefRight);
                break;
            }
        }

        out() << "<p>";
        if (relative->type() == Node::Property ||
                relative->type() == Node::Variable) {
            QString str;
            atom = atom->next();
            while (atom != 0 && atom->type() != Atom::BriefRight) {
                if (atom->type() == Atom::String ||
                        atom->type() == Atom::AutoLink)
                    str += atom->string();
                skipAhead++;
                atom = atom->next();
            }
            str[0] = str[0].toLower();
            if (str.endsWith(QLatin1Char('.')))
                str.truncate(str.length() - 1);
            out() << "This ";
            if (relative->type() == Node::Property)
                out() << "property";
            else
                out() << "variable";
            QStringList words = str.split(QLatin1Char(' '));
            if (!(words.first() == "contains" || words.first() == "specifies"
                  || words.first() == "describes" || words.first() == "defines"
                  || words.first() == "holds" || words.first() == "determines"))
                out() << " holds ";
            else
                out() << ' ';
            out() << str << '.';
        }
        break;
    case Atom::BriefRight:
        if (relative->type() != Node::Document)
            out() << "</p>\n";
        break;
    case Atom::C:
        // This may at one time have been used to mark up C++ code but it is
        // now widely used to write teletype text. As a result, text marked
        // with the \c command is not passed to a code marker.
        out() << formattingLeftMap()[ATOM_FORMATTING_TELETYPE];
        if (inLink_) {
            out() << protectEnc(plainCode(atom->string()));
        }
        else {
            out() << protectEnc(plainCode(atom->string()));
        }
        out() << formattingRightMap()[ATOM_FORMATTING_TELETYPE];
        break;
    case Atom::CaptionLeft:
        out() << "<p class=\"figCaption\">";
        in_para = true;
        break;
    case Atom::CaptionRight:
        endLink();
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        break;
    case Atom::Code:
        out() << "<pre class=\"cpp\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative))
              << "</pre>\n";
        break;
    case Atom::Qml:
        out() << "<pre class=\"qml\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative))
              << "</pre>\n";
        break;
    case Atom::JavaScript:
        out() << "<pre class=\"js\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative))
              << "</pre>\n";
        break;
    case Atom::CodeNew:
        out() << "<p>you can rewrite it as</p>\n"
              << "<pre class=\"cpp\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative))
              << "</pre>\n";
        break;
    case Atom::CodeOld:
        out() << "<p>For example, if you have code like</p>\n";
        // fallthrough
    case Atom::CodeBad:
        out() << "<pre class=\"cpp\">"
              << trimmedTrailing(protectEnc(plainCode(indent(codeIndent,atom->string()))))
              << "</pre>\n";
        break;
    case Atom::DivLeft:
        out() << "<div";
        if (!atom->string().isEmpty())
            out() << ' ' << atom->string();
        out() << '>';
        break;
    case Atom::DivRight:
        out() << "</div>";
        break;
    case Atom::FootnoteLeft:
        // ### For now
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        out() << "<!-- ";
        break;
    case Atom::FootnoteRight:
        // ### For now
        out() << "-->";
        break;
    case Atom::FormatElse:
    case Atom::FormatEndif:
    case Atom::FormatIf:
        break;
    case Atom::FormattingLeft:
        if (atom->string().startsWith("span ")) {
            out() << '<' + atom->string() << '>';
        }
        else
            out() << formattingLeftMap()[atom->string()];
        if (atom->string() == ATOM_FORMATTING_PARAMETER) {
            if (atom->next() != 0 && atom->next()->type() == Atom::String) {
                QRegExp subscriptRegExp("([a-z]+)_([0-9n])");
                if (subscriptRegExp.exactMatch(atom->next()->string())) {
                    out() << subscriptRegExp.cap(1) << "<sub>"
                          << subscriptRegExp.cap(2) << "</sub>";
                    skipAhead = 1;
                }
            }
        }
        break;
    case Atom::FormattingRight:
        if (atom->string() == ATOM_FORMATTING_LINK) {
            endLink();
        }
        else if (atom->string().startsWith("span ")) {
            out() << "</span>";
        }
        else {
            out() << formattingRightMap()[atom->string()];
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
        else if (atom->string() == "qmltypes") {
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
        else if (atom->string() == "relatedinline") {
            const DocNode *dn = static_cast<const DocNode *>(relative);
            if (dn && !dn->members().isEmpty()) {
                // Reverse the list into the original scan order.
                // Should be sorted.  But on what?  It may not be a
                // regular class or page definition.
                QList<const Node *> list;
                foreach (const Node *node, dn->members())
                    list.prepend(node);
                foreach (const Node *node, list)
                    generateBody(node, marker);
            }
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

            out() << "<ul>\n";
            s = sections.constBegin();
            while (s != sections.constEnd()) {
                if (!(*s).members.isEmpty()) {

                    out() << "<li>"
                          << "<a href=\"#"
                          << Doc::canonicalTitle((*s).name)
                          << "\">"
                          << (*s).name
                          << "</a></li>\n";
                }
                ++s;
            }
            out() << "</ul>\n";

            int idx = 0;
            s = sections.constBegin();
            while (s != sections.constEnd()) {
                if (!(*s).members.isEmpty()) {
                    out() << "<a name=\""
                          << Doc::canonicalTitle((*s).name)
                          << "\"></a>\n";
                    out() << "<h3>" << protectEnc((*s).name) << "</h3>\n";
                    if (idx == Class)
                        generateCompactList(Generic, 0, ncmap, false, QStringLiteral("Q"));
                    else if (idx == QmlClass)
                        generateCompactList(Generic, 0, nqcmap, false, QStringLiteral(""));
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
                            out() << "<p>Class ";

                            out() << "<a href=\""
                                  << linkForNode(pmap.key(), 0)
                                  << "\">";
                            QStringList pieces = pmap.key()->fullName().split("::");
                            out() << protectEnc(pieces.last());
                            out() << "</a>"  << ":</p>\n";

                            generateSection(nlist, 0, marker, CodeMarker::Summary);
                            out() << "<br/>";
                            ++pmap;
                        }
                    }
                    else
                        generateSection(s->members, 0, marker, CodeMarker::Summary);
                }
                ++idx;
                ++s;
            }
        }
    }
        break;
    case Atom::BR:
        out() << "<br />\n";
        break;
    case Atom::HR:
        out() << "<hr />\n";
        break;
    case Atom::Image:
    case Atom::InlineImage:
    {
        QString fileName = imageFileName(relative, atom->string());
        QString text;
        if (atom->next() != 0)
            text = atom->next()->string();
        if (atom->type() == Atom::Image)
            out() << "<p class=\"centerAlign\">";
        if (fileName.isEmpty()) {
            relative->location().warning(tr("Missing image: %1").arg(protectEnc(atom->string())));
            out() << "<font color=\"red\">[Missing image "
                  << protectEnc(atom->string()) << "]</font>";
        }
        else {
            QString prefix;
            out() << "<img src=\"" << protectEnc(prefix + fileName) << '"';
            if (!text.isEmpty())
                out() << " alt=\"" << protectEnc(text) << '"';
            else
                out() << " alt=\"\"";
            out() << " />";
            helpProjectWriter->addExtraFile(fileName);
            if ((relative->type() == Node::Document) &&
                    (relative->subType() == Node::Example)) {
                const ExampleNode* cen = static_cast<const ExampleNode*>(relative);
                if (cen->imageFileName().isEmpty()) {
                    ExampleNode* en = const_cast<ExampleNode*>(cen);
                    en->setImageFileName(fileName);
                }
            }
        }
        if (atom->type() == Atom::Image)
            out() << "</p>";
    }
        break;
    case Atom::ImageText:
        break;
    case Atom::ImportantLeft:
        out() << "<p>";
        out() << formattingLeftMap()[ATOM_FORMATTING_BOLD];
        out() << "Important: ";
        out() << formattingRightMap()[ATOM_FORMATTING_BOLD];
        break;
    case Atom::ImportantRight:
        out() << "</p>";
        break;
    case Atom::NoteLeft:
        out() << "<p>";
        out() << formattingLeftMap()[ATOM_FORMATTING_BOLD];
        out() << "Note: ";
        out() << formattingRightMap()[ATOM_FORMATTING_BOLD];
        break;
    case Atom::NoteRight:
        out() << "</p>";
        break;
    case Atom::LegaleseLeft:
        out() << "<div class=\"LegaleseLeft\">";
        break;
    case Atom::LegaleseRight:
        out() << "</div>";
        break;
    case Atom::LineBreak:
        out() << "<br/>";
        break;
    case Atom::Link:
    {
        const Node *node = 0;
        QString myLink = getLink(atom, relative, &node);
        if (myLink.isEmpty()) {
            myLink = getCollisionLink(atom);
            if (myLink.isEmpty() && !noLinkErrors()) {
                relative->doc().location().warning(tr("Can't link to '%1'").arg(atom->string()));
            }
            else
                node = 0;
        }
        beginLink(myLink, node, relative);
        skipAhead = 1;
    }
        break;
    case Atom::LinkNode:
    {
        const Node *node = CodeMarker::nodeForString(atom->string());
        beginLink(linkForNode(node, relative), node, relative);
        skipAhead = 1;
    }
        break;
    case Atom::ListLeft:
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        if (atom->string() == ATOM_LIST_BULLET) {
            out() << "<ul>\n";
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            out() << "<dl>\n";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            threeColumnEnumValueTable_ = isThreeColumnEnumValueTable(atom);
            if (threeColumnEnumValueTable_) {
                out() << "<table class=\"valuelist\">";
                if (++numTableRows_ % 2 == 1)
                    out() << "<tr valign=\"top\" class=\"odd\">";
                else
                    out() << "<tr valign=\"top\" class=\"even\">";

                out() << "<th class=\"tblConst\">Constant</th>"
                      << "<th class=\"tblval\">Value</th>"
                      << "<th class=\"tbldscr\">Description</th></tr>\n";
            }
            else {
                out() << "<table class=\"valuelist\">"
                      << "<tr><th class=\"tblConst\">Constant</th><th class=\"tblVal\">Value</th></tr>\n";
            }
        }
        else {
            out() << "<ol class=";
            if (atom->string() == ATOM_LIST_UPPERALPHA) {
                out() << "\"A\"";
            } /* why type? changed to */
            else if (atom->string() == ATOM_LIST_LOWERALPHA) {
                out() << "\"a\"";
            }
            else if (atom->string() == ATOM_LIST_UPPERROMAN) {
                out() << "\"I\"";
            }
            else if (atom->string() == ATOM_LIST_LOWERROMAN) {
                out() << "\"i\"";
            }
            else { // (atom->string() == ATOM_LIST_NUMERIC)
                out() << "\"1\"";
            }
            if (atom->next() != 0 && atom->next()->string().toInt() != 1)
                out() << " start=\"" << atom->next()->string() << '"';
            out() << ">\n";
        }
        break;
    case Atom::ListItemNumber:
        break;
    case Atom::ListTagLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            out() << "<dt>";
        }
        else { // (atom->string() == ATOM_LIST_VALUE)
            // ### Trenton

            QString t= protectEnc(plainCode(marker->markedUpEnumValue(atom->next()->string(),relative)));
            out() << "<tr><td class=\"topAlign\"><tt>" << t << "</tt></td><td class=\"topAlign\">";

            QString itemValue;
            if (relative->type() == Node::Enum) {
                const EnumNode *enume = static_cast<const EnumNode *>(relative);
                itemValue = enume->itemValue(atom->next()->string());
            }

            if (itemValue.isEmpty())
                out() << '?';
            else
                out() << "<tt>" << protectEnc(itemValue) << "</tt>";

            skipAhead = 1;
        }
        break;
    case Atom::ListTagRight:
        if (atom->string() == ATOM_LIST_TAG)
            out() << "</dt>\n";
        break;
    case Atom::ListItemLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            out() << "<dd>";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            if (threeColumnEnumValueTable_) {
                out() << "</td><td class=\"topAlign\">";
                if (matchAhead(atom, Atom::ListItemRight))
                    out() << "&nbsp;";
            }
        }
        else {
            out() << "<li>";
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::ListItemRight:
        if (atom->string() == ATOM_LIST_TAG) {
            out() << "</dd>\n";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            out() << "</td></tr>\n";
        }
        else {
            out() << "</li>\n";
        }
        break;
    case Atom::ListRight:
        if (atom->string() == ATOM_LIST_BULLET) {
            out() << "</ul>\n";
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            out() << "</dl>\n";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            out() << "</table>\n";
        }
        else {
            out() << "</ol>\n";
        }
        break;
    case Atom::Nop:
        break;
    case Atom::ParaLeft:
        out() << "<p>";
        in_para = true;
        break;
    case Atom::ParaRight:
        endLink();
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        //if (!matchAhead(atom, Atom::ListItemRight) && !matchAhead(atom, Atom::TableItemRight))
        //    out() << "</p>\n";
        break;
    case Atom::QuotationLeft:
        out() << "<blockquote>";
        break;
    case Atom::QuotationRight:
        out() << "</blockquote>\n";
        break;
    case Atom::RawString:
        out() << atom->string();
        break;
    case Atom::SectionLeft:
        out() << "<a name=\"" << Doc::canonicalTitle(Text::sectionHeading(atom).toString())
              << "\"></a>" << divNavTop << '\n';
        break;
    case Atom::SectionRight:
        break;
    case Atom::SectionHeadingLeft:
        out() << "<h" + QString::number(atom->string().toInt() + hOffset(relative)) + QLatin1Char('>');
        inSectionHeading_ = true;
        break;
    case Atom::SectionHeadingRight:
        out() << "</h" + QString::number(atom->string().toInt() + hOffset(relative)) + ">\n";
        inSectionHeading_ = false;
        break;
    case Atom::SidebarLeft:
        break;
    case Atom::SidebarRight:
        break;
    case Atom::String:
        if (inLink_ && !inContents_ && !inSectionHeading_) {
            generateLink(atom, marker);
        }
        else {
            out() << protectEnc(atom->string());
        }
        break;
    case Atom::TableLeft:
    {
        QString p1, p2;
        QString attr = "generic";
        QString width;
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        if (atom->count() > 0) {
            p1 = atom->string(0);
            if (atom->count() > 1)
                p2 = atom->string(1);
        }
        if (!p1.isEmpty()) {
            if (p1 == "borderless")
                attr = p1;
            else if (p1.contains("%"))
                width = p1;
        }
        if (!p2.isEmpty()) {
            if (p2 == "borderless")
                attr = p2;
            else if (p2.contains("%"))
                width = p2;
        }
        out() << "<table class=\"" << attr << "\"";
        if (!width.isEmpty())
            out() << " width=\"" << width << "\"";
        out() << ">\n ";
        numTableRows_ = 0;
    }
        break;
    case Atom::TableRight:
        out() << "</table>\n";
        break;
    case Atom::TableHeaderLeft:
        out() << "<thead><tr class=\"qt-style\">";
        inTableHeader_ = true;
        break;
    case Atom::TableHeaderRight:
        out() << "</tr>";
        if (matchAhead(atom, Atom::TableHeaderLeft)) {
            skipAhead = 1;
            out() << "\n<tr class=\"qt-style\">";
        }
        else {
            out() << "</thead>\n";
            inTableHeader_ = false;
        }
        break;
    case Atom::TableRowLeft:
        if (!atom->string().isEmpty())
            out() << "<tr " << atom->string() << '>';
        else if (++numTableRows_ % 2 == 1)
            out() << "<tr valign=\"top\" class=\"odd\">";
        else
            out() << "<tr valign=\"top\" class=\"even\">";
        break;
    case Atom::TableRowRight:
        out() << "</tr>\n";
        break;
    case Atom::TableItemLeft:
    {
        if (inTableHeader_)
            out() << "<th ";
        else
            out() << "<td ";

        for (int i=0; i<atom->count(); ++i) {
            if (i > 0)
                out() << ' ';
            QString p = atom->string(i);
            if (p.contains('=')) {
                out() << p;
            }
            else {
                QStringList spans = p.split(",");
                if (spans.size() == 2) {
                    if (spans.at(0) != "1")
                        out() << " colspan=\"" << spans.at(0) << '"';
                    if (spans.at(1) != "1")
                        out() << " rowspan=\"" << spans.at(1) << '"';
                }
            }
        }
        if (inTableHeader_)
            out() << '>';
        else {
            out() << '>';
            //out() << "><p>";
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
    }
        break;
    case Atom::TableItemRight:
        if (inTableHeader_)
            out() << "</th>";
        else {
            out() << "</td>";
            //out() << "</p></td>";
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::TableOfContents:
        break;
    case Atom::Target:
        out() << "<a name=\"" << Doc::canonicalTitle(atom->string()) << "\"></a>";
        break;
    case Atom::UnhandledFormat:
        out() << "<b class=\"redFont\">&lt;Missing HTML&gt;</b>";
        break;
    case Atom::UnknownCommand:
        out() << "<b class=\"redFont\"><code>\\" << protectEnc(atom->string())
              << "</code></b>";
        break;
    case Atom::QmlText:
    case Atom::EndQmlText:
        // don't do anything with these. They are just tags.
        break;
    default:
        unknownAtom(atom);
    }
    return skipAhead;
}

/*!
  Generate a reference page for a C++ class.
 */
void HtmlGenerator::generateClassLikeNode(InnerNode* inner, CodeMarker* marker)
{
    QList<Section> sections;
    QList<Section>::ConstIterator s;

    QString title;
    QString rawTitle;
    QString fullTitle;
    if (inner->type() == Node::Namespace) {
        rawTitle = inner->plainName();
        fullTitle = inner->plainFullName();
        title = rawTitle + " Namespace";
    }
    else if (inner->type() == Node::Class) {
        rawTitle = inner->plainName();
        fullTitle = inner->plainFullName();
        title = rawTitle + " Class";
    }

    Text subtitleText;
    if (rawTitle != fullTitle)
        subtitleText << "(" << Atom(Atom::AutoLink, fullTitle) << ")" << Atom(Atom::LineBreak);

    generateHeader(title, inner, marker);
    sections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
    generateTableOfContents(inner,marker,&sections);
    generateTitle(title, subtitleText, SmallSubTitle, inner, marker);
    generateBrief(inner, marker);
    generateRequisites(inner, marker);
    generateStatus(inner, marker);
    generateThreadSafeness(inner, marker);

    out() << "<ul>\n";

    QString membersLink = generateListOfAllMemberFile(inner, marker);
    if (!membersLink.isEmpty())
        out() << "<li><a href=\"" << membersLink << "\">"
              << "List of all members, including inherited members</a></li>\n";

    QString obsoleteLink = generateLowStatusMemberFile(inner,
                                                       marker,
                                                       CodeMarker::Obsolete);
    if (!obsoleteLink.isEmpty()) {
        out() << "<li><a href=\"" << obsoleteLink << "\">"
              << "Obsolete members</a></li>\n";
    }

    QString compatLink = generateLowStatusMemberFile(inner,
                                                     marker,
                                                     CodeMarker::Compat);
    if (!compatLink.isEmpty())
        out() << "<li><a href=\"" << compatLink << "\">"
              << "Compatibility members</a></li>\n";

    out() << "</ul>\n";

    bool needOtherSection = false;

    /*
      sections is built above for the call to generateTableOfContents().
     */
    s = sections.constBegin();
    while (s != sections.constEnd()) {
        if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
            if (!s->inherited.isEmpty())
                needOtherSection = true;
        }
        else {
            if (!s->members.isEmpty()) {
                // out() << "<hr />\n";
                out() << "<a name=\""
                      << registerRef((*s).name.toLower())
                      << "\"></a>" << divNavTop << "\n";
                out() << "<h2>" << protectEnc((*s).name) << "</h2>\n";
                generateSection(s->members, inner, marker, CodeMarker::Summary);
            }
            if (!s->reimpMembers.isEmpty()) {
                QString name = QString("Reimplemented ") + (*s).name;
                //  out() << "<hr />\n";
                out() << "<a name=\""
                      << registerRef(name.toLower())
                      << "\"></a>" << divNavTop << "\n";
                out() << "<h2>" << protectEnc(name) << "</h2>\n";
                generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
            }

            if (!s->inherited.isEmpty()) {
                out() << "<ul>\n";
                generateSectionInheritedList(*s, inner);
                out() << "</ul>\n";
            }
        }
        ++s;
    }

    if (needOtherSection) {
        out() << "<h3>Additional Inherited Members</h3>\n"
                 "<ul>\n";

        s = sections.constBegin();
        while (s != sections.constEnd()) {
            if (s->members.isEmpty() && !s->inherited.isEmpty())
                generateSectionInheritedList(*s, inner);
            ++s;
        }
        out() << "</ul>\n";
    }

    out() << "<a name=\"" << registerRef("details") << "\"></a>" << divNavTop << '\n';

    if (!inner->doc().isEmpty()) {
        generateExtractionMark(inner, DetailedDescriptionMark);
        //out() << "<hr />\n"
        out() << "<div class=\"descr\">\n" // QTBUG-9504
              << "<h2>" << "Detailed Description" << "</h2>\n";
        generateBody(inner, marker);
        out() << "</div>\n"; // QTBUG-9504
        generateAlsoList(inner, marker);
        generateMaintainerList(inner, marker);
        generateExtractionMark(inner, EndMark);
    }

    sections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
    s = sections.constBegin();
    while (s != sections.constEnd()) {
        //out() << "<hr />\n";
        if (!(*s).divClass.isEmpty())
            out() << "<div class=\"" << (*s).divClass << "\">\n"; // QTBUG-9504
        out() << "<h2>" << protectEnc((*s).name) << "</h2>\n";

        NodeList::ConstIterator m = (*s).members.constBegin();
        while (m != (*s).members.constEnd()) {
            if ((*m)->access() != Node::Private) { // ### check necessary?
                if ((*m)->type() != Node::Class)
                    generateDetailedMember(*m, inner, marker);
                else {
                    out() << "<h3> class ";
                    generateFullName(*m, inner);
                    out() << "</h3>";
                    generateBrief(*m, marker, inner);
                }

                QStringList names;
                names << (*m)->name();
                if ((*m)->type() == Node::Function) {
                    const FunctionNode *func = reinterpret_cast<const FunctionNode *>(*m);
                    if (func->metaness() == FunctionNode::Ctor ||
                            func->metaness() == FunctionNode::Dtor ||
                            func->overloadNumber() != 1)
                        names.clear();
                }
                else if ((*m)->type() == Node::Property) {
                    const PropertyNode *prop = reinterpret_cast<const PropertyNode *>(*m);
                    if (!prop->getters().isEmpty() &&
                            !names.contains(prop->getters().first()->name()))
                        names << prop->getters().first()->name();
                    if (!prop->setters().isEmpty())
                        names << prop->setters().first()->name();
                    if (!prop->resetters().isEmpty())
                        names << prop->resetters().first()->name();
                    if (!prop->notifiers().isEmpty())
                        names << prop->notifiers().first()->name();
                }
                else if ((*m)->type() == Node::Enum) {
                    const EnumNode *enume = reinterpret_cast<const EnumNode*>(*m);
                    if (enume->flagsType())
                        names << enume->flagsType()->name();

                    foreach (const QString &enumName,
                             enume->doc().enumItemNames().toSet() -
                             enume->doc().omitEnumItemNames().toSet())
                        names << plainCode(marker->markedUpEnumValue(enumName,
                                                                     enume));
                }
            }
            ++m;
        }
        if (!(*s).divClass.isEmpty())
            out() << "</div>\n"; // QTBUG-9504
        ++s;
    }
    generateFooter(inner);
}

/*!
  We delayed generation of the disambiguation pages until now, after
  all the other pages have been generated. We do this because we might
  encounter a link command that tries to link to a target on a QML
  component page, but the link doesn't specify the module identifer
  for the component, and the component name without a module
  identifier is ambiguous. When such a link is found, qdoc can't find
  the target, so it appends the target to the NameCollisionNode. After
  the tree has been traversed and all these ambiguous links have been
  added to the name collision nodes, this function is called. The list
  of collision nodes is traversed here, and the disambiguation page for
  each collision is generated. The disambiguation page will not only
  disambiguate links to the component pages, but it will also disambiguate
  links to properties, section headers, etc.
 */
void HtmlGenerator::generateCollisionPages()
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
        QString htmlTitle = fullTitle;
        CodeMarker* marker = CodeMarker::markerForFileName(ncn->location().filePath());
        if (ncn->isQmlNode()) {
            // Replace the marker with a QML code marker.
            if (ncn->isQmlNode())
                marker = CodeMarker::markerForLanguage(QLatin1String("QML"));
        }

        generateHeader(htmlTitle, ncn, marker);
        if (!fullTitle.isEmpty())
            out() << "<h1 class=\"title\">" << protectEnc(fullTitle) << "</h1>\n";

        NodeMap nm;
        for (int i=0; i<collisions.size(); ++i) {
            Node* n = collisions.at(i);
            QString t;
            if (!n->qmlModuleName().isEmpty())
                t = n->qmlModuleName() + "::";
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
                out() << "<a name=\"" << Doc::canonicalTitle(*t) << "\"></a>";
                out() << "<h2 class=\"title\">" << protectEnc(*t) << "</h2>\n";
                out() << "<ul>\n";
                for (int i=0; i<collisions.size(); ++i) {
                    InnerNode* n = static_cast<InnerNode*>(collisions.at(i));
                    Node* p = n->findChildNodeByName(*t);
                    if (p) {
                        QString link = linkForNode(p,0);
                        QString label;
                        if (!n->qmlModuleName().isEmpty())
                            label = n->qmlModuleName() + "::";
                        label += n->name() + "::" + p->name();
                        out() << "<li>";
                        out() << "<a href=\"" << link << "\">";
                        out() << protectEnc(label) << "</a>";
                        out() << "</li>\n";
                    }
                }
                out() << "</ul>\n";
                ++t;
            }
        }

        generateFooter(ncn);
        endSubPage();
    }
}

/*!
  Generate the HTML page for an entity that doesn't map
  to any underlying parsable C++ class or QML component.
 */
void HtmlGenerator::generateDocNode(DocNode* dn, CodeMarker* marker)
{
    /*
      If the document node is a page node, and if the page type
      is DITA map page, write the node's contents as a dita
      map and return without doing anything else.
     */
    if (dn->subType() == Node::Page && dn->pageType() == Node::DitaMapPage) {
        const DitaMapNode* dmn = static_cast<const DitaMapNode*>(dn);
        writeDitaMap(dmn);
        return;
    }

    SubTitleSize subTitleSize = LargeSubTitle;
    QList<Section> sections;
    QList<Section>::const_iterator s;
    QString fullTitle = dn->fullTitle();
    QString htmlTitle = fullTitle;

    if (dn->subType() == Node::QmlBasicType) {
        fullTitle = "QML Basic Type: " + fullTitle;
        htmlTitle = fullTitle;

        // Replace the marker with a QML code marker.
        marker = CodeMarker::markerForLanguage(QLatin1String("QML"));
    }

    generateHeader(htmlTitle, dn, marker);
    /*
      Generate the TOC for the new doc format.
      Don't generate a TOC for the home page.
    */
    QmlClassNode* qml_cn = 0;
    if (dn->subType() == Node::QmlClass) {
        qml_cn = static_cast<QmlClassNode*>(dn);
        sections = marker->qmlSections(qml_cn,CodeMarker::Summary);
        generateTableOfContents(dn,marker,&sections);

        // Replace the marker with a QML code marker.
        marker = CodeMarker::markerForLanguage(QLatin1String("QML"));
    }
    else if (dn->subType() != Node::Collision && dn->name() != QString("index.html") && dn->name() != QString("qtexamplesandtutorials.html"))
        generateTableOfContents(dn,marker,0);

    generateTitle(fullTitle,
                  Text() << dn->subTitle(),
                  subTitleSize,
                  dn,
                  marker);

    if (dn->subType() == Node::Module) {
        // Generate brief text and status for modules.
        generateBrief(dn, marker);
        generateStatus(dn, marker);
        generateSince(dn, marker);

        NodeMap nm;
        dn->getMemberNamespaces(nm);
        if (!nm.isEmpty()) {
            out() << "<a name=\"" << registerRef("namespaces") << "\"></a>" << divNavTop << '\n';
            out() << "<h2>Namespaces</h2>\n";
            generateAnnotatedList(dn, marker, nm);
        }
        nm.clear();
        dn->getMemberClasses(nm);
        if (!nm.isEmpty()) {
            out() << "<a name=\"" << registerRef("classes") << "\"></a>" << divNavTop << '\n';
            out() << "<h2>Classes</h2>\n";
            generateAnnotatedList(dn, marker, nm);
        }
        nm.clear();
    }
    else if (dn->subType() == Node::HeaderFile) {
        // Generate brief text and status for modules.
        generateBrief(dn, marker);
        generateStatus(dn, marker);
        generateSince(dn, marker);

        out() << "<ul>\n";

        QString membersLink = generateListOfAllMemberFile(dn, marker);
        if (!membersLink.isEmpty())
            out() << "<li><a href=\"" << membersLink << "\">"
                  << "List of all members, including inherited members</a></li>\n";

        QString obsoleteLink = generateLowStatusMemberFile(dn,
                                                           marker,
                                                           CodeMarker::Obsolete);
        if (!obsoleteLink.isEmpty()) {
            out() << "<li><a href=\"" << obsoleteLink << "\">"
                  << "Obsolete members</a></li>\n";
        }

        QString compatLink = generateLowStatusMemberFile(dn,
                                                         marker,
                                                         CodeMarker::Compat);
        if (!compatLink.isEmpty())
            out() << "<li><a href=\"" << compatLink << "\">"
                  << "Compatibility members</a></li>\n";

        out() << "</ul>\n";
    }
    else if (dn->subType() == Node::QmlClass) {
        const_cast<DocNode*>(dn)->setCurrentChild();
        ClassNode* cn = qml_cn->classNode();
        generateBrief(qml_cn, marker);
        generateQmlRequisites(qml_cn, marker);

        QString allQmlMembersLink = generateAllQmlMembersFile(qml_cn, marker);
        if (!allQmlMembersLink.isEmpty()) {
            out() << "<ul>\n";
            out() << "<li><a href=\"" << allQmlMembersLink << "\">"
                  << "List of all members, including inherited members</a></li>\n";
            out() << "</ul>\n";
        }

        s = sections.constBegin();
        while (s != sections.constEnd()) {
            out() << "<a name=\"" << registerRef((*s).name.toLower())
                  << "\"></a>" << divNavTop << '\n';
            out() << "<h2>" << protectEnc((*s).name) << "</h2>\n";
            generateQmlSummary(*s,dn,marker);
            ++s;
        }

        generateExtractionMark(dn, DetailedDescriptionMark);
        out() << "<a name=\"" << registerRef("details") << "\"></a>" << divNavTop << '\n';
        out() << "<h2>" << "Detailed Description" << "</h2>\n";
        generateBody(dn, marker);
        if (cn)
            generateQmlText(cn->doc().body(), cn, marker, dn->name());
        generateAlsoList(dn, marker);
        generateExtractionMark(dn, EndMark);
        //out() << "<hr />\n";

        sections = marker->qmlSections(qml_cn,CodeMarker::Detailed);
        s = sections.constBegin();
        while (s != sections.constEnd()) {
            out() << "<h2>" << protectEnc((*s).name) << "</h2>\n";
            NodeList::ConstIterator m = (*s).members.constBegin();
            while (m != (*s).members.constEnd()) {
                generateDetailedQmlMember(*m, dn, marker);
                out() << "<br/>\n";
                ++m;
            }
            ++s;
        }
        generateFooter(dn);
        const_cast<DocNode*>(dn)->clearCurrentChild();
        return;
    }

    sections = marker->sections(dn, CodeMarker::Summary, CodeMarker::Okay);
    s = sections.constBegin();
    while (s != sections.constEnd()) {
        out() << "<a name=\"" << registerRef((*s).name) << "\"></a>" << divNavTop << '\n';
        out() << "<h2>" << protectEnc((*s).name) << "</h2>\n";
        generateSectionList(*s, dn, marker, CodeMarker::Summary);
        ++s;
    }

    Text brief = dn->doc().briefText();
    if (dn->subType() == Node::Module && !brief.isEmpty()) {
        generateExtractionMark(dn, DetailedDescriptionMark);
        out() << "<a name=\"" << registerRef("details") << "\"></a>" << divNavTop << '\n';
        out() << "<div class=\"descr\">\n"; // QTBUG-9504
        out() << "<h2>" << "Detailed Description" << "</h2>\n";
    }
    else {
        generateExtractionMark(dn, DetailedDescriptionMark);
        out() << "<div class=\"descr\"> <a name=\"" << registerRef("details") << "\"></a>\n"; // QTBUG-9504
    }

    generateBody(dn, marker);
    out() << "</div>\n"; // QTBUG-9504
    generateAlsoList(dn, marker);
    generateExtractionMark(dn, EndMark);

    if ((dn->subType() == Node::Group))
        generateAnnotatedList(dn, marker, dn->members());
    else if (dn->subType() == Node::QmlModule)
        generateAnnotatedList(dn, marker, dn->members());

    sections = marker->sections(dn, CodeMarker::Detailed, CodeMarker::Okay);
    s = sections.constBegin();
    while (s != sections.constEnd()) {
        //out() << "<hr />\n";
        out() << "<h2>" << protectEnc((*s).name) << "</h2>\n";

        NodeList::ConstIterator m = (*s).members.constBegin();
        while (m != (*s).members.constEnd()) {
            generateDetailedMember(*m, dn, marker);
            ++m;
        }
        ++s;
    }
    generateFooter(dn);
}

/*!
  Returns "html" for this subclass of Generator.
 */
QString HtmlGenerator::fileExtension() const
{
    return "html";
}

/*!
  Output navigation list in the html file.
 */
void HtmlGenerator::generateNavigationBar(const QString &title,
                                        const Node *node,
                                        CodeMarker *marker)
{
    if (noNavigationBar)
        return;

    Text navigationbar;

    if (homepage == title)
        return;
    if (!homepage.isEmpty())
        navigationbar << Atom(Atom::ListItemLeft)
                    << Atom(Atom::AutoLink, homepage)
                    << Atom(Atom::ListItemRight);
    if (!landingpage.isEmpty() && landingpage != title)
        navigationbar << Atom(Atom::ListItemLeft)
                    << Atom(Atom::AutoLink, landingpage)
                    << Atom(Atom::ListItemRight);

    if (node->type() == Node::Class) {
        const ClassNode *cn = static_cast<const ClassNode *>(node);
        QString name =  node->moduleName();

        if (!cppclassespage.isEmpty())
            navigationbar << Atom(Atom::ListItemLeft)
                        << Atom(Atom::Link, cppclassespage)
                        << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                        << Atom(Atom::String, QLatin1String("C++ Classes"))
                        << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                        << Atom(Atom::ListItemRight);

        if (!cn->name().isEmpty())
            navigationbar << Atom(Atom::ListItemLeft)
                        << Atom(Atom::String, cn->name())
                        << Atom(Atom::ListItemRight);
    }
    else if (node->type() == Node::Document) {
        const DocNode *dn = static_cast<const DocNode *>(node);
        if (node->subType() == Node::QmlClass || node->subType() == Node::QmlBasicType) {
            if (!qmltypespage.isEmpty())
                navigationbar << Atom(Atom::ListItemLeft)
                        << Atom(Atom::Link, qmltypespage)
                        << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                        << Atom(Atom::String, QLatin1String("QML Types"))
                        << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                        << Atom(Atom::ListItemRight);
        }
        else if (dn && dn->isExampleFile()) {
            navigationbar << Atom(Atom::ListItemLeft)
                          << Atom(Atom::Link, dn->parent()->name())
                          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                          << Atom(Atom::String, dn->parent()->title())
                          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                          << Atom(Atom::ListItemRight);

        }
        navigationbar << Atom(Atom::ListItemLeft)
                      << Atom(Atom::String, title)
                      << Atom(Atom::ListItemRight);
    }

    generateText(navigationbar, node, marker);
}

void HtmlGenerator::generateHeader(const QString& title,
                                   const Node *node,
                                   CodeMarker *marker)
{
#ifndef QT_NO_TEXTCODEC
    out() << QString("<?xml version=\"1.0\" encoding=\"%1\"?>\n").arg(outputEncoding);
#else
    out() << QString("<?xml version=\"1.0\"?>\n");
#endif
    out() << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
    out() << QString("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"%1\" lang=\"%1\">\n").arg(naturalLanguage);
    out() << "<head>\n";
    out() << "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
    if (node && !node->doc().location().isEmpty())
        out() << "<!-- " << node->doc().location().fileName() << " -->\n";

    QString shortVersion = qdb_->version();
    if (shortVersion.count(QChar('.')) == 2)
        shortVersion.truncate(shortVersion.lastIndexOf(QChar('.')));
    if (!project.isEmpty())
        shortVersion = QLatin1String(" | ") + project + QLatin1Char(' ') + shortVersion;
    else
        shortVersion = QLatin1String(" | ") + QLatin1String("Qt ") + shortVersion ;

    // Generating page title
    out() << "  <title>" << protectEnc(title) << shortVersion << "</title>\n";

    // Include style sheet and script links.
    out() << headerStyles;
    out() << headerScripts;
    if (endHeader.isEmpty())
        out() << "</head>\n<body>\n";
    else
        out() << endHeader;

#ifdef GENERATE_MAC_REFS
    if (mainPage)
        generateMacRef(node, marker);
#endif

    out() << QString(postHeader).replace("\\" + COMMAND_VERSION, qdb_->version());
    generateNavigationBar(title,node,marker);
    out() << "<li id=\"buildversion\">\n" << buildversion << "</li>\n";
    out() << QString(postPostHeader).replace("\\" + COMMAND_VERSION, qdb_->version());

    navigationLinks.clear();

    if (node && !node->links().empty()) {
        QPair<QString,QString> linkPair;
        QPair<QString,QString> anchorPair;
        const Node *linkNode;

        if (node->links().contains(Node::PreviousLink)) {
            linkPair = node->links()[Node::PreviousLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (!linkNode || linkNode == node)
                anchorPair = linkPair;
            else
                anchorPair = anchorForNode(linkNode);

            out() << "  <link rel=\"prev\" href=\""
                  << anchorPair.first << "\" />\n";

            navigationLinks += "<a class=\"prevPage\" href=\"" + anchorPair.first + "\">";
            if (linkPair.first == linkPair.second && !anchorPair.second.isEmpty())
                navigationLinks += protect(anchorPair.second);
            else
                navigationLinks += protect(linkPair.second);
            navigationLinks += "</a>\n";
        }
        if (node->links().contains(Node::NextLink)) {
            linkPair = node->links()[Node::NextLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (!linkNode || linkNode == node)
                anchorPair = linkPair;
            else
                anchorPair = anchorForNode(linkNode);

            out() << "  <link rel=\"next\" href=\""
                  << anchorPair.first << "\" />\n";

            navigationLinks += "<a class=\"nextPage\" href=\"" + anchorPair.first + "\">";
            if (linkPair.first == linkPair.second && !anchorPair.second.isEmpty())
                navigationLinks += protect(anchorPair.second);
            else
                navigationLinks += protect(linkPair.second);
            navigationLinks += "</a>\n";
        }
        if (node->links().contains(Node::StartLink)) {
            linkPair = node->links()[Node::StartLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (!linkNode || linkNode == node)
                anchorPair = linkPair;
            else
                anchorPair = anchorForNode(linkNode);
            out() << "  <link rel=\"start\" href=\""
                  << anchorPair.first << "\" />\n";
        }
    }

    if (node && !node->links().empty())
        out() << "<p class=\"naviNextPrevious headerNavi\">\n" << navigationLinks << "</p><p/>\n";
}

void HtmlGenerator::generateTitle(const QString& title,
                                  const Text &subTitle,
                                  SubTitleSize subTitleSize,
                                  const Node *relative,
                                  CodeMarker *marker)
{
    if (!title.isEmpty())
        out() << "<h1 class=\"title\">" << protectEnc(title) << "</h1>\n";
    if (!subTitle.isEmpty()) {
        out() << "<span";
        if (subTitleSize == SmallSubTitle)
            out() << " class=\"small-subtitle\">";
        else
            out() << " class=\"subtitle\">";
        generateText(subTitle, relative, marker);
        out() << "</span>\n";
    }
}

void HtmlGenerator::generateFooter(const Node *node)
{
    if (node && !node->links().empty())
        out() << "<p class=\"naviNextPrevious footerNavi\">\n" << navigationLinks << "</p>\n";

    out() << QString(footer).replace("\\" + COMMAND_VERSION, qdb_->version())
          << QString(address).replace("\\" + COMMAND_VERSION, qdb_->version());

    out() << "</body>\n";
    out() << "</html>\n";
}

/*!
Lists the required imports and includes in a table.
The number of rows is known, so this path is simpler than the generateSection() path.
*/
void HtmlGenerator::generateRequisites(InnerNode *inner, CodeMarker *marker)
{
    QMap<QString, Text> requisites;
    Text text;

    const QString headerText = "Header";
    const QString sinceText = "Since";
    const QString inheritedBytext = "Inherited By";
    const QString inheritsText = "Inherits";
    const QString instantiatedByText = "Instantiated By";
    const QString qtVariableText = "qmake";

    //add the includes to the map
    if (!inner->includes().isEmpty()) {
        text.clear();
        text << formattingRightMap()[ATOM_FORMATTING_BOLD]
             << formattingLeftMap()[ATOM_FORMATTING_TELETYPE]
             << highlightedCode(indent(codeIndent,
                                       marker->markedUpIncludes(inner->includes())),
                                       inner)
             << formattingRightMap()[ATOM_FORMATTING_TELETYPE];
        requisites.insert(headerText, text);
    }

    //The order of the requisites matter
    QStringList requisiteorder;
    requisiteorder << headerText
                   << qtVariableText
                   << sinceText
                   << instantiatedByText
                   << inheritsText
                   << inheritedBytext;

    //add the since and project into the map
    if (!inner->since().isEmpty()) {
        text.clear();
        QStringList since = inner->since().split(QLatin1Char(' '));
        if (since.count() == 1) {
            // If there is only one argument, assume it is the Qt version number.
            text << " Qt " << since[0];
        }
        else {
                //Otherwise, reconstruct the <project> <version> string.
                text << " " << since.join(' ');
        }
        text << Atom::ParaRight;
        requisites.insert(sinceText, text);
    }

    if (inner->type() == Node::Class || inner->type() == Node::Namespace) {
        //add the QT variable to the map
        if (!inner->moduleName().isEmpty()) {
            DocNode * moduleNode = qdb_->findModule(inner->moduleName());
            if (moduleNode && !moduleNode->qtVariable().isEmpty()) {
                text.clear();
                text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_TELETYPE)
                     << "QT += " + moduleNode->qtVariable()
                     << Atom(Atom::FormattingRight, ATOM_FORMATTING_TELETYPE);
                requisites.insert(qtVariableText, text);
            }
        }
    }

    if (inner->type() == Node::Class) {
        ClassNode* classe = static_cast<ClassNode*>(inner);
        if (classe->qmlElement() != 0 && classe->status() != Node::Internal) {
            text.clear();
            text << Atom(Atom::LinkNode, CodeMarker::stringForNode(classe->qmlElement()))
                 << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << Atom(Atom::String, classe->qmlElement()->name())
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
            requisites.insert(instantiatedByText, text);

        }

        //add the inherits to the map
        QList<RelatedClass>::ConstIterator r;
        int index;
        if (!classe->baseClasses().isEmpty()) {
            text.clear();
            r = classe->baseClasses().constBegin();
            index = 0;
            while (r != classe->baseClasses().constEnd()) {
                text << Atom(Atom::LinkNode, CodeMarker::stringForNode((*r).node))
                     << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                     << Atom(Atom::String, (*r).dataTypeWithTemplateArgs)
                     << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);

                if ((*r).access == Node::Protected) {
                    text << " (protected)";
                }
                else if ((*r).access == Node::Private) {
                    text << " (private)";
                }
                text << separator(index++, classe->baseClasses().count());
                ++r;
            }
            text << Atom::ParaRight;
            requisites.insert(inheritsText, text);
        }

        //add the inherited-by to the map
        if (!classe->derivedClasses().isEmpty()) {
            text.clear();
            text << Atom::ParaLeft;
            appendSortedNames(text, classe, classe->derivedClasses());
            text << Atom::ParaRight;
            requisites.insert(inheritedBytext, text);
        }
    }

    if (!requisites.isEmpty()) {
        //generate the table
        out() << "<table class=\"alignedsummary\">\n";

        QStringList::ConstIterator i;
        for (i = requisiteorder.begin(); i != requisiteorder.constEnd(); ++i) {

            if (requisites.contains(*i)) {
                out() << "<tr>"
                    << "<td class=\"memItemLeft rightAlign topAlign\"> "
                    << *i << ":"
                    << "</td><td class=\"memItemRight bottomAlign\"> ";

                if (*i == headerText)
                    out() << requisites.value(*i).toString();
                else
                    generateText(requisites.value(*i), inner, marker);
                out() << "</td></tr>";
            }
        }
        out() << "</table>";
    }
}

/*!
Lists the required imports and includes in a table.
The number of rows is known, so this path is simpler than the generateSection() path.
*/
void HtmlGenerator::generateQmlRequisites(QmlClassNode *qcn, CodeMarker *marker)
{
    if (!qcn)
        return;
    QMap<QString, Text> requisites;
    Text text;

    const QString importText = "Import Statement:";
    const QString sinceText = "Since:";
    const QString inheritedBytext = "Inherited By:";
    const QString inheritsText = "Inherits:";
    const QString instantiatesText = "Instantiates:";

    //The order of the requisites matter
    QStringList requisiteorder;
    requisiteorder << importText
                   << sinceText
                   << instantiatesText
                   << inheritsText
                   << inheritedBytext;

    //add the module name and version to the map
    QString qmlModuleVersion;
    DocNode* dn = qdb_->findQmlModule(qcn->qmlModuleName());
    if (dn)
        qmlModuleVersion = dn->qmlModuleVersion();
    else
        qmlModuleVersion = qcn->qmlModuleVersion();
    text.clear();
    text << formattingRightMap()[ATOM_FORMATTING_BOLD]
         << formattingLeftMap()[ATOM_FORMATTING_TELETYPE]
         << "import " + qcn->qmlModuleName() + " " + qmlModuleVersion
         << formattingRightMap()[ATOM_FORMATTING_TELETYPE];
    requisites.insert(importText, text);

    //add the since and project into the map
    if (!qcn->since().isEmpty()) {
        text.clear();
        QStringList since = qcn->since().split(QLatin1Char(' '));
        if (since.count() == 1) {
            // If there is only one argument, assume it is the Qt version number.
            text << " Qt " << since[0];
        }
        else {
                //Otherwise, reconstruct the <project> <version> string.
                text << " " << since.join(' ');
        }
        text << Atom::ParaRight;
        requisites.insert(sinceText, text);
    }

    //add the instantiates to the map
    ClassNode* cn = qcn->classNode();
    if (cn && (cn->status() != Node::Internal)) {
        text.clear();
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        requisites.insert(instantiatesText, text);
    }

    //add the inherits to the map
    const QmlClassNode* base = qcn->qmlBaseNode();
    while (base && base->isInternal()) {
        base = base->qmlBaseNode();
    }
    if (base) {
        text.clear();
        text << Atom::ParaLeft
             << Atom(Atom::LinkNode,CodeMarker::stringForNode(base))
             << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
             << Atom(Atom::String, base->name())
             << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
             << Atom::ParaRight;
        requisites.insert(inheritsText, text);
    }

    //add the inherited-by to the map
    NodeList subs;
    QmlClassNode::subclasses(qcn->name(), subs);
    if (!subs.isEmpty()) {
        text.clear();
        text << Atom::ParaLeft;
        appendSortedQmlNames(text, qcn, subs);
        text << Atom::ParaRight;
        requisites.insert(inheritedBytext, text);
    }

    if (!requisites.isEmpty()) {
        //generate the table
        out() << "<table class=\"alignedsummary\">\n";

        QStringList::ConstIterator i;
        for (i = requisiteorder.begin(); i != requisiteorder.constEnd(); ++i) {

            if (requisites.contains(*i)) {
                out() << "<tr>"
                    << "<td class=\"memItemLeft rightAlign topAlign\"> "
                    << *i
                    << "</td><td class=\"memItemRight bottomAlign\"> ";

                if (*i == importText)
                    out()<<requisites.value(*i).toString();
                else
                    generateText(requisites.value(*i), qcn, marker);
                out() << "</td></tr>";
            }
        }
        out() << "</table>";
    }
}

void HtmlGenerator::generateBrief(const Node *node, CodeMarker *marker,
                                  const Node *relative)
{
    Text brief = node->doc().briefText();
    if (!brief.isEmpty()) {
        generateExtractionMark(node, BriefMark);
        out() << "<p>";
        generateText(brief, node, marker);

        if (!relative || node == relative)
            out() << " <a href=\"#";
        else
            out() << " <a href=\"" << linkForNode(node, relative) << '#';
        out() << registerRef("details") << "\">More...</a></p>\n";


        generateExtractionMark(node, EndMark);
    }
}

void HtmlGenerator::generateIncludes(const InnerNode *inner, CodeMarker *marker)
{
    if (!inner->includes().isEmpty()) {
        out() << "<pre class=\"cpp\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,
                                                        marker->markedUpIncludes(inner->includes())),
                                                 inner))
              << "</pre>";
    }
}

/*!
  Revised for the new doc format.
  Generates a table of contents beginning at \a node.
 */
void HtmlGenerator::generateTableOfContents(const Node *node,
                                            CodeMarker *marker,
                                            QList<Section>* sections)
{
    QList<Atom*> toc;
    if (node->doc().hasTableOfContents())
        toc = node->doc().tableOfContents();
    if (toc.isEmpty() && !sections && (node->subType() != Node::Module))
        return;

    QStringList sectionNumber;
    int detailsBase = 0;

    // disable nested links in table of contents
    inContents_ = true;
    inLink_ = true;

    out() << "<div class=\"toc\">\n";
    out() << "<h3><a name=\"toc\">Contents</a></h3>\n";
    sectionNumber.append("1");
    out() << "<ul>\n";

    if (node->subType() == Node::Module) {
        if (node->hasNamespaces()) {
            out() << "<li class=\"level"
                  << sectionNumber.size()
                  << "\"><a href=\"#"
                  << registerRef("namespaces")
                  << "\">Namespaces</a></li>\n";
        }
        if (node->hasClasses()) {
            out() << "<li class=\"level"
                  << sectionNumber.size()
                  << "\"><a href=\"#"
                  << registerRef("classes")
                  << "\">Classes</a></li>\n";
        }
        out() << "<li class=\"level"
              << sectionNumber.size()
              << "\"><a href=\"#"
              << registerRef("details")
              << "\">Detailed Description</a></li>\n";
        for (int i = 0; i < toc.size(); ++i) {
            if (toc.at(i)->string().toInt() == 1) {
                detailsBase = 1;
                break;
            }
        }
    }
    else if (sections && ((node->type() == Node::Class) ||
                          (node->type() == Node::Namespace) ||
                          (node->subType() == Node::QmlClass))) {
        QList<Section>::ConstIterator s = sections->constBegin();
        while (s != sections->constEnd()) {
            if (!s->members.isEmpty() || !s->reimpMembers.isEmpty()) {
                out() << "<li class=\"level"
                      << sectionNumber.size()
                      << "\"><a href=\"#"
                      << registerRef((*s).pluralMember)
                      << "\">" << (*s).name
                      << "</a></li>\n";
            }
            ++s;
        }
        out() << "<li class=\"level"
              << sectionNumber.size()
              << "\"><a href=\"#"
              << registerRef("details")
              << "\">Detailed Description</a></li>\n";
        for (int i = 0; i < toc.size(); ++i) {
            if (toc.at(i)->string().toInt() == 1) {
                detailsBase = 1;
                break;
            }
        }
    }

    for (int i = 0; i < toc.size(); ++i) {
        Atom *atom = toc.at(i);
        int nextLevel = atom->string().toInt() + detailsBase;
        if (nextLevel >= 0) {
            if (sectionNumber.size() < nextLevel) {
                do {
                    sectionNumber.append("1");
                } while (sectionNumber.size() < nextLevel);
            }
            else {
                while (sectionNumber.size() > nextLevel) {
                    sectionNumber.removeLast();
                }
                sectionNumber.last() = QString::number(sectionNumber.last().toInt() + 1);
            }
        }
        int numAtoms;
        Text headingText = Text::sectionHeading(atom);
        QString s = headingText.toString();
        out() << "<li class=\"level"
              << sectionNumber.size()
              << "\">";
        out() << "<a href=\""
              << '#'
              << Doc::canonicalTitle(s)
              << "\">";
        generateAtomList(headingText.firstAtom(), node, marker, true, numAtoms);
        out() << "</a></li>\n";
    }
    while (!sectionNumber.isEmpty()) {
        sectionNumber.removeLast();
    }
    out() << "</ul>\n";
    out() << "</div>\n";
    inContents_ = false;
    inLink_ = false;
}

QString HtmlGenerator::generateListOfAllMemberFile(const InnerNode *inner,
                                                   CodeMarker *marker)
{
    QList<Section> sections;
    QList<Section>::ConstIterator s;

    sections = marker->sections(inner,
                                CodeMarker::Subpage,
                                CodeMarker::Okay);
    if (sections.isEmpty())
        return QString();

    QString fileName = fileBase(inner) + "-members." + fileExtension();
    beginSubPage(inner, fileName);
    QString title = "List of All Members for " + inner->name();
    generateHeader(title, inner, marker);
    generateTitle(title, Text(), SmallSubTitle, inner, marker);
    out() << "<p>This is the complete list of members for ";
    generateFullName(inner, 0);
    out() << ", including inherited members.</p>\n";

    Section section = sections.first();
    generateSectionList(section, 0, marker, CodeMarker::Subpage);

    generateFooter();
    endSubPage();
    return fileName;
}

/*!
  This function creates an html page on which are listed all
  the members of QML class \a qml_cn, including the inherited
  members. The \a marker is used for formatting stuff.
 */
QString HtmlGenerator::generateAllQmlMembersFile(const QmlClassNode* qml_cn,
                                                 CodeMarker* marker)
{
    QList<Section> sections;
    QList<Section>::ConstIterator s;

    sections = marker->qmlSections(qml_cn,CodeMarker::Subpage);
    if (sections.isEmpty())
        return QString();

    QString fileName = fileBase(qml_cn) + "-members." + fileExtension();
    beginSubPage(qml_cn, fileName);
    QString title = "List of All Members for " + qml_cn->name();
    generateHeader(title, qml_cn, marker);
    generateTitle(title, Text(), SmallSubTitle, qml_cn, marker);
    out() << "<p>This is the complete list of members for ";
    generateFullName(qml_cn, 0);
    out() << ", including inherited members.</p>\n";

    ClassKeysNodesList& cknl = sections.first().classKeysNodesList_;
    if (!cknl.isEmpty()) {
        for (int i=0; i<cknl.size(); i++) {
            ClassKeysNodes* ckn = cknl[i];
            const QmlClassNode* qcn = ckn->first;
            KeysAndNodes& kn = ckn->second;
            QStringList& keys = kn.first;
            NodeList& nodes = kn.second;
            if (nodes.isEmpty())
                continue;
            if (i != 0) {
                out() << "<p>The following members are inherited from ";
                generateFullName(qcn,0);
                out() << ".</p>\n";
            }
            out() << "<ul>\n";
            for (int j=0; j<keys.size(); j++) {
                if (nodes[j]->access() == Node::Private || nodes[j]->status() == Node::Internal) {
                    continue;
                }
                out() << "<li class=\"fn\">";
                QString prefix;
                if (!keys.isEmpty()) {
                    prefix = keys.at(j).mid(1);
                    prefix = prefix.left(keys.at(j).indexOf("::")+1);
                }
                generateQmlItem(nodes[j], qcn, marker, true);
                //generateSynopsis(nodes[j], qcn, marker, CodeMarker::Subpage, false, &prefix);
                out() << "</li>\n";
            }
            out() << "</ul>\n";
        }
    }

    generateFooter();
    endSubPage();
    return fileName;
}

QString HtmlGenerator::generateLowStatusMemberFile(InnerNode *inner,
                                                   CodeMarker *marker,
                                                   CodeMarker::Status status)
{
    QList<Section> sections = marker->sections(inner,
                                               CodeMarker::Summary,
                                               status);
    QMutableListIterator<Section> j(sections);
    while (j.hasNext()) {
        if (j.next().members.size() == 0)
            j.remove();
    }
    if (sections.isEmpty())
        return QString();

    int i;

    QString title;
    QString fileName;

    if (status == CodeMarker::Compat) {
        title = "Compatibility Members for " + inner->name();
        fileName = fileBase(inner) + "-compat." + fileExtension();
    }
    else {
        title = "Obsolete Members for " + inner->name();
        fileName = fileBase(inner) + "-obsolete." + fileExtension();
    }
    if (status == CodeMarker::Obsolete) {
        QString link;
        if (useOutputSubdirs() && !Generator::outputSubdir().isEmpty())
            link = QString("../" + Generator::outputSubdir() + QLatin1Char('/'));
        link += fileName;
        inner->setObsoleteLink(link);
    }

    beginSubPage(inner, fileName);
    generateHeader(title, inner, marker);
    generateTitle(title, Text(), SmallSubTitle, inner, marker);

    if (status == CodeMarker::Compat) {
        out() << "<p><b>The following members of class "
              << "<a href=\"" << linkForNode(inner, 0) << "\">"
              << protectEnc(inner->name()) << "</a>"
              << "are part of the "
                 "Qt compatibility layer.</b> We advise against "
                 "using them in new code.</p>\n";
    }
    else {
        out() << "<p><b>The following members of class "
              << "<a href=\"" << linkForNode(inner, 0) << "\">"
              << protectEnc(inner->name()) << "</a>"
              << " are obsolete.</b> "
              << "They are provided to keep old source code working. "
              << "We strongly advise against using them in new code.</p>\n";
    }

    for (i = 0; i < sections.size(); ++i) {
        out() << "<h2>" << protectEnc(sections.at(i).name) << "</h2>\n";
        generateSectionList(sections.at(i), inner, marker, CodeMarker::Summary);
    }

    sections = marker->sections(inner, CodeMarker::Detailed, status);
    for (i = 0; i < sections.size(); ++i) {
        //out() << "<hr />\n";
        out() << "<h2>" << protectEnc(sections.at(i).name) << "</h2>\n";

        NodeList::ConstIterator m = sections.at(i).members.constBegin();
        while (m != sections.at(i).members.constEnd()) {
            if ((*m)->access() != Node::Private)
                generateDetailedMember(*m, inner, marker);
            ++m;
        }
    }

    generateFooter();
    endSubPage();
    return fileName;
}

void HtmlGenerator::generateClassHierarchy(const Node *relative, NodeMap& classMap)
{
    if (classMap.isEmpty())
        return;

    NodeMap topLevel;
    NodeMap::Iterator c = classMap.begin();
    while (c != classMap.end()) {
        ClassNode *classe = static_cast<ClassNode *>(*c);
        if (classe->baseClasses().isEmpty())
            topLevel.insert(classe->name(), classe);
        ++c;
    }

    QStack<NodeMap > stack;
    stack.push(topLevel);

    out() << "<ul>\n";
    while (!stack.isEmpty()) {
        if (stack.top().isEmpty()) {
            stack.pop();
            out() << "</ul>\n";
        }
        else {
            ClassNode* child = static_cast<ClassNode*>(*stack.top().begin());
            out() << "<li>";
            generateFullName(child, relative);
            out() << "</li>\n";
            stack.top().erase(stack.top().begin());

            NodeMap newTop;
            foreach (const RelatedClass &d, child->derivedClasses()) {
                if (d.access != Node::Private && !d.node->doc().isEmpty())
                    newTop.insert(d.node->name(), d.node);
            }
            if (!newTop.isEmpty()) {
                stack.push(newTop);
                out() << "<ul>\n";
            }
        }
    }
}

/*!
  Output an annotated list of the nodes in \a nodeMap.
  A two-column table is output.
 */
void HtmlGenerator::generateAnnotatedList(const Node* relative,
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

void HtmlGenerator::generateAnnotatedList(const Node *relative,
                                          CodeMarker *marker,
                                          const NodeList& nodes)
{
    bool allInternal = true;
    foreach (const Node* node, nodes) {
        if (!node->isInternal() && node->status() != Node::Obsolete) {
            allInternal = false;
        }
    }
    if (allInternal)
        return;
    out() << "<table class=\"annotated\">\n";
    int row = 0;
    foreach (const Node* node, nodes) {
        if (node->isInternal() || node->status() == Node::Obsolete)
            continue;

        if (++row % 2 == 1)
            out() << "<tr class=\"odd topAlign\">";
        else
            out() << "<tr class=\"even topAlign\">";
        out() << "<td class=\"tblName\"><p>";
        generateFullName(node, relative);
        out() << "</p></td>";

        if (!(node->type() == Node::Document)) {
            Text brief = node->doc().trimmedBriefText(node->name());
            if (!brief.isEmpty()) {
                out() << "<td class=\"tblDescr\"><p>";
                generateText(brief, node, marker);
                out() << "</p></td>";
            }
            else if (!node->reconstitutedBrief().isEmpty()) {
                out() << "<td class=\"tblDescr\"><p>";
                out() << node->reconstitutedBrief();
                out() << "</p></td>";
            }
        }
        else {
            out() << "<td class=\"tblDescr\"><p>";
            if (!node->reconstitutedBrief().isEmpty()) {
                out() << node->reconstitutedBrief();
            }
            else
                out() << protectEnc(node->doc().briefText().toString());
            out() << "</p></td>";
        }
        out() << "</tr>\n";
    }
    out() << "</table>\n";
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
void HtmlGenerator::generateCompactList(ListType listType,
                                        const Node *relative,
                                        const NodeMap &classMap,
                                        bool includeAlphabet,
                                        QString commonPrefix)
{
    if (classMap.isEmpty())
        return;

    const int NumParagraphs = 37; // '0' to '9', 'A' to 'Z', '_'
    int commonPrefixLen = commonPrefix.length();

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
        if (idx > 0 && !pieces.last().startsWith(commonPrefix))
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

    /*
      Output the alphabet as a row of links.
     */
    if (includeAlphabet) {
        out() << "<p  class=\"centerAlign functionIndex\"><b>";
        for (int i = 0; i < 26; i++) {
            QChar ch('a' + i);
            if (usedParagraphNames.contains(char('a' + i)))
                out() << QString("<a href=\"#%1\">%2</a>&nbsp;").arg(ch).arg(ch.toUpper());
        }
        out() << "</b></p>\n";
    }

    /*
      Output a <div> element to contain all the <dl> elements.
     */
    out() << "<div class=\"flowListDiv\">\n";
    numTableRows_ = 0;

    int curParNr = 0;
    int curParOffset = 0;

    for (int i=0; i<classMap.count(); i++) {
        while ((curParNr < NumParagraphs) &&
               (curParOffset == paragraph[curParNr].count())) {
            ++curParNr;
            curParOffset = 0;
        }

        /*
          Starting a new paragraph means starting a new <dl>.
        */
        if (curParOffset == 0) {
            if (i > 0)
                out() << "</dl>\n";
            if (++numTableRows_ % 2 == 1)
                out() << "<dl class=\"flowList odd\">";
            else
                out() << "<dl class=\"flowList even\">";
            out() << "<dt class=\"alphaChar\">";
            if (includeAlphabet) {
                QChar c = paragraphName[curParNr][0].toLower();
                out() << QString("<a name=\"%1\"></a>").arg(c);
            }
            out() << "<b>"
                  << paragraphName[curParNr]
                  << "</b>";
            out() << "</dt>\n";
        }

        /*
          Output a <dd> for the current offset in the current paragraph.
         */
        out() << "<dd>";
        if ((curParNr < NumParagraphs) &&
                !paragraphName[curParNr].isEmpty()) {
            NodeMap::Iterator it;
            it = paragraph[curParNr].begin();
            for (int i=0; i<curParOffset; i++)
                ++it;

            if (listType == Generic) {
                /*
                  Previously, we used generateFullName() for this, but we
                  require some special formatting.
                */
                out() << "<a href=\"" << linkForNode(it.value(), relative) << "\">";
            }
            else if (listType == Obsolete) {
                QString fileName = fileBase(it.value()) + "-obsolete." + fileExtension();
                QString link;
                if (useOutputSubdirs())
                    link = QString("../" + it.value()->outputSubdirectory() + QLatin1Char('/'));
                link += fileName;
                out() << "<a href=\"" << link << "\">";
            }

            QStringList pieces;
            if (it.value()->subType() == Node::QmlClass)
                pieces << it.value()->name();
            else
                pieces = it.value()->fullName(relative).split("::");
            out() << protectEnc(pieces.last());
            out() << "</a>";
            if (pieces.size() > 1) {
                out() << " (";
                generateFullName(it.value()->parent(), relative);
                out() << ')';
            }
        }
        out() << "</dd>\n";
        curParOffset++;
    }
    if (classMap.count() > 0)
        out() << "</dl>\n";

    out() << "</div>\n";
}

void HtmlGenerator::generateFunctionIndex(const Node *relative)
{
    out() << "<p  class=\"centerAlign functionIndex\"><b>";
    for (int i = 0; i < 26; i++) {
        QChar ch('a' + i);
        out() << QString("<a href=\"#%1\">%2</a>&nbsp;").arg(ch).arg(ch.toUpper());
    }
    out() << "</b></p>\n";

    char nextLetter = 'a';
    char currentLetter;

    out() << "<ul>\n";
    NodeMapMap funcIndex = qdb_->getFunctionIndex();
    QMap<QString, NodeMap >::ConstIterator f = funcIndex.constBegin();
    while (f != funcIndex.constEnd()) {
        out() << "<li>";
        out() << protectEnc(f.key()) << ':';

        currentLetter = f.key()[0].unicode();
        while (islower(currentLetter) && currentLetter >= nextLetter) {
            out() << QString("<a name=\"%1\"></a>").arg(nextLetter);
            nextLetter++;
        }

        NodeMap::ConstIterator s = (*f).constBegin();
        while (s != (*f).constEnd()) {
            out() << ' ';
            generateFullName((*s)->parent(), relative, *s);
            ++s;
        }
        out() << "</li>";
        out() << '\n';
        ++f;
    }
    out() << "</ul>\n";
}

void HtmlGenerator::generateLegaleseList(const Node *relative, CodeMarker *marker)
{
    TextToNodeMap& legaleseTexts = qdb_->getLegaleseTexts();
    QMap<Text, const Node *>::ConstIterator it = legaleseTexts.constBegin();
    while (it != legaleseTexts.constEnd()) {
        Text text = it.key();
        //out() << "<hr />\n";
        generateText(text, relative, marker);
        out() << "<ul>\n";
        do {
            out() << "<li>";
            generateFullName(it.value(), relative);
            out() << "</li>\n";
            ++it;
        } while (it != legaleseTexts.constEnd() && it.key() == text);
        out() << "</ul>\n";
    }
}

void HtmlGenerator::generateQmlItem(const Node *node,
                                    const Node *relative,
                                    CodeMarker *marker,
                                    bool summary)
{
    QString marked = marker->markedUpQmlItem(node,summary);
    QRegExp templateTag("(<[^@>]*>)");
    if (marked.indexOf(templateTag) != -1) {
        QString contents = protectEnc(marked.mid(templateTag.pos(1),
                                                 templateTag.cap(1).length()));
        marked.replace(templateTag.pos(1), templateTag.cap(1).length(),
                       contents);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i>\\1<sub>\\2</sub></i>");
    marked.replace("<@param>", "<i>");
    marked.replace("</@param>", "</i>");

    if (summary)
        marked.replace("@name>", "b>");

    marked.replace("<@extra>", "<tt>");
    marked.replace("</@extra>", "</tt>");

    if (summary) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }
    out() << highlightedCode(marked, relative, false, node);
}

void HtmlGenerator::generateOverviewList(const Node *relative)
{
    QMap<const DocNode *, QMap<QString, DocNode *> > docNodeMap;
    QMap<QString, const DocNode *> groupTitlesMap;
    QMap<QString, DocNode *> uncategorizedNodeMap;
    QRegExp singleDigit("\\b([0-9])\\b");

    const NodeList children = qdb_->treeRoot()->childNodes();
    foreach (Node *child, children) {
        if (child->type() == Node::Document && child != relative) {
            DocNode *docNode = static_cast<DocNode *>(child);

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
                    foreach (Node *member, docNode->members()) {
                        if (member->isInternal() || member->type() != Node::Document)
                            continue;
                        DocNode *page = static_cast<DocNode *>(member);
                        if (page) {
                            QString sortKey = page->fullTitle().toLower();
                            if (sortKey.startsWith("the "))
                                sortKey.remove(0, 4);
                            sortKey.replace(singleDigit, "0\\1");
                            docNodeMap[const_cast<const DocNode *>(docNode)].insert(sortKey, page);
                            groupTitlesMap[docNode->fullTitle()] = const_cast<const DocNode *>(docNode);
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
        foreach (const QString &groupTitle, groupTitlesMap.keys()) {
            const DocNode *groupNode = groupTitlesMap[groupTitle];
            out() << QString("<h3><a href=\"%1\">%2</a></h3>\n").arg(
                         linkForNode(groupNode, relative)).arg(
                         protectEnc(groupNode->fullTitle()));

            if (docNodeMap[groupNode].count() == 0)
                continue;

            out() << "<ul>\n";

            foreach (const DocNode *docNode, docNodeMap[groupNode]) {
                QString title = docNode->fullTitle();
                if (title.startsWith("The "))
                    title.remove(0, 4);
                out() << "<li><a href=\"" << linkForNode(docNode, relative) << "\">"
                      << protectEnc(title) << "</a></li>\n";
            }
            out() << "</ul>\n";
        }
    }

    if (!uncategorizedNodeMap.isEmpty()) {
        out() << QString("<h3>Miscellaneous</h3>\n");
        out() << "<ul>\n";
        foreach (const DocNode *docNode, uncategorizedNodeMap) {
            QString title = docNode->fullTitle();
            if (title.startsWith("The "))
                title.remove(0, 4);
            out() << "<li><a href=\"" << linkForNode(docNode, relative) << "\">"
                  << protectEnc(title) << "</a></li>\n";
        }
        out() << "</ul>\n";
    }
}

void HtmlGenerator::generateSection(const NodeList& nl,
                                    const Node *relative,
                                    CodeMarker *marker,
                                    CodeMarker::SynopsisStyle style)
{
    bool alignNames = true;
    if (!nl.isEmpty()) {
        bool twoColumn = false;
        if (style == CodeMarker::Subpage) {
            alignNames = false;
            twoColumn = (nl.count() >= 16);
        }
        else if (nl.first()->type() == Node::Property) {
            twoColumn = (nl.count() >= 5);
            alignNames = false;
        }
        if (alignNames) {
            out() << "<table class=\"alignedsummary\">\n";
        }
        else {
            if (twoColumn)
                out() << "<table class=\"propsummary\">\n"
                      << "<tr><td class=\"topAlign\">";
            out() << "<ul>\n";
        }

        int i = 0;
        NodeList::ConstIterator m = nl.constBegin();
        while (m != nl.constEnd()) {
            if ((*m)->access() == Node::Private) {
                ++m;
                continue;
            }

            if (alignNames) {
                out() << "<tr><td class=\"memItemLeft rightAlign topAlign\"> ";
            }
            else {
                if (twoColumn && i == (int) (nl.count() + 1) / 2)
                    out() << "</ul></td><td class=\"topAlign\"><ul>\n";
                out() << "<li class=\"fn\">";
            }

            generateSynopsis(*m, relative, marker, style, alignNames);
            if (alignNames)
                out() << "</td></tr>\n";
            else
                out() << "</li>\n";
            i++;
            ++m;
        }
        if (alignNames)
            out() << "</table>\n";
        else {
            out() << "</ul>\n";
            if (twoColumn)
                out() << "</td></tr>\n</table>\n";
        }
    }
}

void HtmlGenerator::generateSectionList(const Section& section,
                                        const Node *relative,
                                        CodeMarker *marker,
                                        CodeMarker::SynopsisStyle style)
{
    bool alignNames = true;
    if (!section.members.isEmpty()) {
        bool twoColumn = false;
        if (style == CodeMarker::Subpage) {
            alignNames = false;
            twoColumn = (section.members.count() >= 16);
        }
        else if (section.members.first()->type() == Node::Property) {
            twoColumn = (section.members.count() >= 5);
            alignNames = false;
        }
        if (alignNames) {
            out() << "<table class=\"alignedsummary\">\n";
        }
        else {
            if (twoColumn)
                out() << "<table class=\"propsummary\">\n"
                      << "<tr><td class=\"topAlign\">";
            out() << "<ul>\n";
        }

        int i = 0;
        NodeList::ConstIterator m = section.members.constBegin();
        while (m != section.members.constEnd()) {
            if ((*m)->access() == Node::Private) {
                ++m;
                continue;
            }

            if (alignNames) {
                out() << "<tr><td class=\"memItemLeft topAlign rightAlign\"> ";
            }
            else {
                if (twoColumn && i == (int) (section.members.count() + 1) / 2)
                    out() << "</ul></td><td class=\"topAlign\"><ul>\n";
                out() << "<li class=\"fn\">";
            }

            QString prefix;
            if (!section.keys.isEmpty()) {
                prefix = section.keys.at(i).mid(1);
                prefix = prefix.left(section.keys.at(i).indexOf("::")+1);
            }
            generateSynopsis(*m, relative, marker, style, alignNames, &prefix);
            if (alignNames)
                out() << "</td></tr>\n";
            else
                out() << "</li>\n";
            i++;
            ++m;
        }
        if (alignNames)
            out() << "</table>\n";
        else {
            out() << "</ul>\n";
            if (twoColumn)
                out() << "</td></tr>\n</table>\n";
        }
    }

    if (style == CodeMarker::Summary && !section.inherited.isEmpty()) {
        out() << "<ul>\n";
        generateSectionInheritedList(section, relative);
        out() << "</ul>\n";
    }
}

void HtmlGenerator::generateSectionInheritedList(const Section& section, const Node *relative)
{
    QList<QPair<InnerNode *, int> >::ConstIterator p = section.inherited.constBegin();
    while (p != section.inherited.constEnd()) {
        out() << "<li class=\"fn\">";
        out() << (*p).second << ' ';
        if ((*p).second == 1) {
            out() << section.singularMember;
        }
        else {
            out() << section.pluralMember;
        }
        out() << " inherited from <a href=\"" << fileName((*p).first)
              << '#' << HtmlGenerator::cleanRef(section.name.toLower()) << "\">"
              << protectEnc((*p).first->plainFullName(relative))
              << "</a></li>\n";
        ++p;
    }
}

void HtmlGenerator::generateSynopsis(const Node *node,
                                     const Node *relative,
                                     CodeMarker *marker,
                                     CodeMarker::SynopsisStyle style,
                                     bool alignNames,
                                     const QString* prefix)
{
    QString marked = marker->markedUpSynopsis(node, relative, style);

    if (prefix)
        marked.prepend(*prefix);
    QRegExp templateTag("(<[^@>]*>)");
    if (marked.indexOf(templateTag) != -1) {
        QString contents = protectEnc(marked.mid(templateTag.pos(1),
                                                 templateTag.cap(1).length()));
        marked.replace(templateTag.pos(1), templateTag.cap(1).length(),
                       contents);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i>\\1<sub>\\2</sub></i>");
    marked.replace("<@param>", "<i> ");
    marked.replace("</@param>", "</i>");

    if (style == CodeMarker::Summary) {
        marked.remove("<@name>");   // was "<b>"
        marked.remove("</@name>");  // was "</b>"
    }

    if (style == CodeMarker::Subpage) {
        QRegExp extraRegExp("<@extra>.*</@extra>");
        extraRegExp.setMinimal(true);
        marked.remove(extraRegExp);
    } else {
        marked.replace("<@extra>", "<tt>");
        marked.replace("</@extra>", "</tt>");
    }

    if (style != CodeMarker::Detailed) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }

    out() << highlightedCode(marked, relative, alignNames);
}

QString HtmlGenerator::highlightedCode(const QString& markedCode,
                                       const Node* relative,
                                       bool alignNames,
                                       const Node* self)
{
    QString src = markedCode;
    QString html;
    QStringRef arg;
    QStringRef par1;

    const QChar charLangle = '<';
    const QChar charAt = '@';

    static const QString typeTag("type");
    static const QString headerTag("headerfile");
    static const QString funcTag("func");
    static const QString linkTag("link");

    // replace all <@link> tags: "(<@link node=\"([^\"]+)\">).*(</@link>)"
    bool done = false;
    for (int i = 0, srcSize = src.size(); i < srcSize;) {
        if (src.at(i) == charLangle && src.at(i + 1) == charAt) {
            if (alignNames && !done) {
                html += "</td><td class=\"memItemRight bottomAlign\">";
                done = true;
            }
            i += 2;
            if (parseArg(src, linkTag, &i, srcSize, &arg, &par1)) {
                html += "<b>";
                const Node* n = CodeMarker::nodeForString(par1.toString());
                QString link = linkForNode(n, relative);
                addLink(link, arg, &html);
                html += "</b>";
            }
            else {
                html += charLangle;
                html += charAt;
            }
        }
        else {
            html += src.at(i++);
        }
    }

    // replace all <@func> tags: "(<@func target=\"([^\"]*)\">)(.*)(</@func>)"
    src = html;
    html = QString();
    for (int i = 0, srcSize = src.size(); i < srcSize;) {
        if (src.at(i) == charLangle && src.at(i + 1) == charAt) {
            i += 2;
            if (parseArg(src, funcTag, &i, srcSize, &arg, &par1)) {

                const Node* n = qdb_->resolveTarget(par1.toString(), relative);
                QString link = linkForNode(n, relative);
                addLink(link, arg, &html);
                par1 = QStringRef();
            }
            else {
                html += charLangle;
                html += charAt;
            }
        }
        else {
            html += src.at(i++);
        }
    }

    // replace all "(<@(type|headerfile|func)(?: +[^>]*)?>)(.*)(</@\\2>)" tags
    src = html;
    html = QString();

    for (int i=0, srcSize=src.size(); i<srcSize;) {
        if (src.at(i) == charLangle && src.at(i+1) == charAt) {
            i += 2;
            bool handled = false;
            if (parseArg(src, typeTag, &i, srcSize, &arg, &par1)) {
                par1 = QStringRef();
                const Node* n = qdb_->resolveTarget(arg.toString(), relative, self);
                html += QLatin1String("<span class=\"type\">");
                if (n && n->subType() == Node::QmlBasicType) {
                    if (relative && relative->subType() == Node::QmlClass)
                        addLink(linkForNode(n,relative), arg, &html);
                    else
                        html += arg.toString();
                }
                else
                    addLink(linkForNode(n,relative), arg, &html);
                html += QLatin1String("</span>");
                handled = true;
            }
            else if (parseArg(src, headerTag, &i, srcSize, &arg, &par1)) {
                par1 = QStringRef();
                const Node* n = qdb_->resolveTarget(arg.toString(), relative);
                addLink(linkForNode(n,relative), arg, &html);
                handled = true;
            }
            else if (parseArg(src, funcTag, &i, srcSize, &arg, &par1)) {
                par1 = QStringRef();
                const Node* n = qdb_->resolveTarget(arg.toString(), relative);
                addLink(linkForNode(n,relative), arg, &html);
                handled = true;
            }

            if (!handled) {
                html += charLangle;
                html += charAt;
            }
        }
        else {
            html += src.at(i++);
        }
    }

    // replace all
    // "<@comment>" -> "<span class=\"comment\">";
    // "<@preprocessor>" -> "<span class=\"preprocessor\">";
    // "<@string>" -> "<span class=\"string\">";
    // "<@char>" -> "<span class=\"char\">";
    // "<@number>" -> "<span class=\"number\">";
    // "<@op>" -> "<span class=\"operator\">";
    // "<@type>" -> "<span class=\"type\">";
    // "<@name>" -> "<span class=\"name\">";
    // "<@keyword>" -> "<span class=\"keyword\">";
    // "</@(?:comment|preprocessor|string|char|number|op|type|name|keyword)>" -> "</span>"
    src = html;
    html = QString();
    static const QString spanTags[] = {
        "<@comment>",       "<span class=\"comment\">",
        "<@preprocessor>",  "<span class=\"preprocessor\">",
        "<@string>",        "<span class=\"string\">",
        "<@char>",          "<span class=\"char\">",
        "<@number>",        "<span class=\"number\">",
        "<@op>",            "<span class=\"operator\">",
        "<@type>",          "<span class=\"type\">",
        "<@name>",          "<span class=\"name\">",
        "<@keyword>",       "<span class=\"keyword\">",
        "</@comment>",      "</span>",
        "</@preprocessor>", "</span>",
        "</@string>",       "</span>",
        "</@char>",         "</span>",
        "</@number>",       "</span>",
        "</@op>",           "</span>",
        "</@type>",         "</span>",
        "</@name>",         "</span>",
        "</@keyword>",      "</span>",
    };
    // Update the upper bound of k in the following code to match the length
    // of the above array.
    for (int i = 0, n = src.size(); i < n;) {
        if (src.at(i) == charLangle) {
            bool handled = false;
            for (int k = 0; k != 18; ++k) {
                const QString & tag = spanTags[2 * k];
                if (tag == QStringRef(&src, i, tag.length())) {
                    html += spanTags[2 * k + 1];
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
                    html += charLangle;
                }
            }
        }
        else {
            html += src.at(i);
            ++i;
        }
    }
    return html;
}

void HtmlGenerator::generateLink(const Atom* atom, CodeMarker* marker)
{
    static QRegExp camelCase("[A-Z][A-Z][a-z]|[a-z][A-Z0-9]|_");

    if (funcLeftParen.indexIn(atom->string()) != -1 && marker->recognizeLanguage("Cpp")) {
        // hack for C++: move () outside of link
        int k = funcLeftParen.pos(1);
        out() << protectEnc(atom->string().left(k));
        if (link_.isEmpty()) {
            if (showBrokenLinks)
                out() << "</i>";
        } else {
            out() << "</a>";
        }
        inLink_ = false;
        out() << protectEnc(atom->string().mid(k));
    } else {
        out() << protectEnc(atom->string());
    }
}

QString HtmlGenerator::cleanRef(const QString& ref)
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
    } else if (u == '~') {
        clean += "dtor.";
    } else if (u == '_') {
        clean += "underscore.";
    } else {
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
        } else if (c.isSpace()) {
            clean += QLatin1Char('-');
        } else if (u == '!') {
            clean += "-not";
        } else if (u == '&') {
            clean += "-and";
        } else if (u == '<') {
            clean += "-lt";
        } else if (u == '=') {
            clean += "-eq";
        } else if (u == '>') {
            clean += "-gt";
        } else if (u == '#') {
            clean += QLatin1Char('#');
        } else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

QString HtmlGenerator::registerRef(const QString& ref)
{
    QString clean = HtmlGenerator::cleanRef(ref);

    for (;;) {
        QString& prevRef = refMap[clean.toLower()];
        if (prevRef.isEmpty()) {
            prevRef = ref;
            break;
        } else if (prevRef == ref) {
            break;
        }
        clean += QLatin1Char('x');
    }
    return clean;
}

QString HtmlGenerator::protectEnc(const QString &string)
{
#ifndef QT_NO_TEXTCODEC
    return protect(string, outputEncoding);
#else
    return protect(string);
#endif
}

QString HtmlGenerator::protect(const QString &string, const QString &outputEncoding)
{
#define APPEND(x) \
    if (html.isEmpty()) { \
    html = string; \
    html.truncate(i); \
} \
    html += (x);

    QString html;
    int n = string.length();

    for (int i = 0; i < n; ++i) {
        QChar ch = string.at(i);

        if (ch == QLatin1Char('&')) {
            APPEND("&amp;");
        } else if (ch == QLatin1Char('<')) {
            APPEND("&lt;");
        } else if (ch == QLatin1Char('>')) {
            APPEND("&gt;");
        } else if (ch == QLatin1Char('"')) {
            APPEND("&quot;");
        } else if ((outputEncoding == "ISO-8859-1" && ch.unicode() > 0x007F)
                   || (ch == QLatin1Char('*') && i + 1 < n && string.at(i) == QLatin1Char('/'))
                   || (ch == QLatin1Char('.') && i > 2 && string.at(i - 2) == QLatin1Char('.'))) {
            // we escape '*/' and the last dot in 'e.g.' and 'i.e.' for the Javadoc generator
            APPEND("&#x");
            html += QString::number(ch.unicode(), 16);
            html += QLatin1Char(';');
        } else {
            if (!html.isEmpty())
                html += ch;
        }
    }

    if (!html.isEmpty())
        return html;
    return string;

#undef APPEND
}

QString HtmlGenerator::fileBase(const Node *node) const
{
    QString result;

    result = Generator::fileBase(node);

    if (!node->isInnerNode()) {
        switch (node->status()) {
        case Node::Compat:
            result += "-compat";
            break;
        case Node::Obsolete:
            result += "-obsolete";
            break;
        default:
            ;
        }
    }
    return result;
}

QString HtmlGenerator::fileName(const Node *node)
{
    if (node->type() == Node::Document) {
        if (static_cast<const DocNode *>(node)->subType() == Node::ExternalPage)
            return node->name();
        if (static_cast<const DocNode *>(node)->subType() == Node::Image)
            return node->name();
    }
    return Generator::fileName(node);
}

QString HtmlGenerator::refForNode(const Node *node)
{
    const FunctionNode *func;
    const TypedefNode *typedeffe;
    QString ref;

    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
    default:
        break;
    case Node::Enum:
        ref = node->name() + "-enum";
        break;
    case Node::Typedef:
        typedeffe = static_cast<const TypedefNode *>(node);
        if (typedeffe->associatedEnum()) {
            return refForNode(typedeffe->associatedEnum());
        }
        else {
            ref = node->name() + "-typedef";
        }
        break;
    case Node::Function:
        func = static_cast<const FunctionNode *>(node);
        if (func->associatedProperty()) {
            return refForNode(func->associatedProperty());
        }
        else {
            ref = func->name();
            if (func->overloadNumber() != 1)
                ref += QLatin1Char('-') + QString::number(func->overloadNumber());
        }
        break;
    case Node::Document:
        break;
    case Node::QmlPropertyGroup:
    case Node::QmlProperty:
    case Node::Property:
        ref = node->name() + "-prop";
        break;
    case Node::QmlSignal:
        ref = node->name() + "-signal";
        break;
    case Node::QmlSignalHandler:
        ref = node->name() + "-signal-handler";
        break;
    case Node::QmlMethod:
        func = static_cast<const FunctionNode *>(node);
        ref = func->name() + "-method";
        if (func->overloadNumber() != 1)
            ref += QLatin1Char('-') + QString::number(func->overloadNumber());
        break;
    case Node::Variable:
        ref = node->name() + "-var";
        break;
    }
    return registerRef(ref);
}

#define DEBUG_ABSTRACT 0

/*!
  Construct the link string for the \a node and return it.
  The \a relative node is use to decide the link we are
  generating is in the same file as the target. Note the
  relative node can be 0, which pretty much guarantees
  that the link and the target aren't in the same file.
  */
QString HtmlGenerator::linkForNode(const Node *node, const Node *relative)
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
        QString ref = refForNode(node);
        if (relative && fn == fileName(relative) && ref == refForNode(relative))
            return QString();

        link += QLatin1Char('#');
        link += ref;
    }
    /*
      If the output is going to subdirectories, then if the
      two nodes will be output to different directories, then
      the link must go up to the parent directory and then
      back down into the other subdirectory.
     */
    if (node && relative && (node != relative)) {
        if (useOutputSubdirs() && !node->isExternalPage() &&
               node->outputSubdirectory() != relative->outputSubdirectory()) {
            link.prepend(QString("../" + node->outputSubdirectory() + QLatin1Char('/')));
        }
    }
    return link;
}

void HtmlGenerator::generateFullName(const Node *apparentNode, const Node *relative, const Node *actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    out() << "<a href=\"" << linkForNode(actualNode, relative);
    if (true || relative == 0 || relative->status() != actualNode->status()) {
        switch (actualNode->status()) {
        case Node::Obsolete:
            out() << "\" class=\"obsolete";
            break;
        case Node::Compat:
            out() << "\" class=\"compat";
            break;
        default:
            ;
        }
    }
    out() << "\">";
    out() << protectEnc(apparentNode->fullName(relative));
    out() << "</a>";
}

void HtmlGenerator::generateDetailedMember(const Node *node,
                                           const InnerNode *relative,
                                           CodeMarker *marker)
{
    const EnumNode *enume;

#ifdef GENERATE_MAC_REFS
    generateMacRef(node, marker);
#endif
    generateExtractionMark(node, MemberMark);
    if (node->type() == Node::Enum
            && (enume = static_cast<const EnumNode *>(node))->flagsType()) {
#ifdef GENERATE_MAC_REFS
        generateMacRef(enume->flagsType(), marker);
#endif
        out() << "<h3 class=\"flags\">";
        out() << "<a name=\"" + refForNode(node) + "\"></a>";
        generateSynopsis(enume, relative, marker, CodeMarker::Detailed);
        out() << "<br/>";
        generateSynopsis(enume->flagsType(),
                         relative,
                         marker,
                         CodeMarker::Detailed);
        out() << "</h3>\n";
    }
    else {
        out() << "<h3 class=\"fn\">";
        out() << "<a name=\"" + refForNode(node) + "\"></a>";
        generateSynopsis(node, relative, marker, CodeMarker::Detailed);
        out() << "</h3>" << divNavTop << '\n';
    }

    generateStatus(node, marker);
    generateBody(node, marker);
    generateThreadSafeness(node, marker);
    generateSince(node, marker);

    if (node->type() == Node::Property) {
        const PropertyNode *property = static_cast<const PropertyNode *>(node);
        Section section;

        section.members += property->getters();
        section.members += property->setters();
        section.members += property->resetters();

        if (!section.members.isEmpty()) {
            out() << "<p><b>Access functions:</b></p>\n";
            generateSectionList(section, node, marker, CodeMarker::Accessors);
        }

        Section notifiers;
        notifiers.members += property->notifiers();

        if (!notifiers.members.isEmpty()) {
            out() << "<p><b>Notifier signal:</b></p>\n";
            //out() << "<p>This signal is emitted when the property value is changed.</p>\n";
            generateSectionList(notifiers, node, marker, CodeMarker::Accessors);
        }
    }
    else if (node->type() == Node::Enum) {
        const EnumNode *enume = static_cast<const EnumNode *>(node);
        if (enume->flagsType()) {
            out() << "<p>The " << protectEnc(enume->flagsType()->name())
                  << " type is a typedef for "
                  << "<a href=\"" << qflagsHref_ << "\">QFlags</a>&lt;"
                  << protectEnc(enume->name())
                  << "&gt;. It stores an OR combination of "
                  << protectEnc(enume->name())
                  << " values.</p>\n";
        }
    }
    generateAlsoList(node, marker);
    generateExtractionMark(node, EndMark);
}

int HtmlGenerator::hOffset(const Node *node)
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

bool HtmlGenerator::isThreeColumnEnumValueTable(const Atom *atom)
{
    while (atom != 0 && !(atom->type() == Atom::ListRight && atom->string() == ATOM_LIST_VALUE)) {
        if (atom->type() == Atom::ListItemLeft && !matchAhead(atom, Atom::ListItemRight))
            return true;
        atom = atom->next();
    }
    return false;
}


const QPair<QString,QString> HtmlGenerator::anchorForNode(const Node *node)
{
    QPair<QString,QString> anchorPair;

    anchorPair.first = Generator::fileName(node);
    if (node->type() == Node::Document) {
        const DocNode *docNode = static_cast<const DocNode*>(node);
        anchorPair.second = docNode->title();
    }

    return anchorPair;
}

QString HtmlGenerator::getLink(const Atom *atom, const Node *relative, const Node** node)
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
        if (atom->string().contains('#')) {
            path = atom->string().split('#');
        }
        else {
            path.append(atom->string());
        }

        QString ref;
        QString first = path.first().trimmed();
        if (first.isEmpty()) {
            *node = relative;
        }
        else if (first.endsWith(".html")) {
            /*
              This is not a recursive search. That's ok in
              this case, because we are searching for a page
              node, which must be a direct child of the tree
              root.
            */
            *node = qdb_->treeRoot()->findChildNodeByNameAndType(first, Node::Document);
        }
        else {
            *node = qdb_->resolveTarget(first, relative);
            if (!*node) {
                *node = qdb_->findDocNodeByTitle(first, relative);
            }
            if (!*node) {
                *node = qdb_->findUnambiguousTarget(first, ref, relative);
                if (*node && !(*node)->url().isEmpty() && !ref.isEmpty()) {
                    QString final = (*node)->url() + "#" + ref;
                    return final;
                }
            }
        }
        if (*node) {
            if (!(*node)->url().isEmpty()) {
                return (*node)->url();
            }
            else {
                path.removeFirst();
            }
        }
        else {
            *node = relative;
        }

        if (*node) {
            if ((*node)->status() == Node::Obsolete) {
                if (relative) {
                    if (relative->parent() != *node) {
                        if (relative->status() != Node::Obsolete) {
                            bool porting = false;
                            if (relative->type() == Node::Document) {
                                const DocNode* dn = static_cast<const DocNode*>(relative);
                                if (dn->title().startsWith("Porting"))
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
                }
                else {
                    qDebug() << "Link to Obsolete entity"
                             << (*node)->name() << "no relative";
                }
            }
        }

        /*
          This loop really only makes sense if *node is not 0.
          In that case, The node *node points to represents a
          qdoc page, so the link will ultimately point to some
          target on that page. This loop finds that target on
          the page that *node represents. ref is that target.
         */
        while (!path.isEmpty()) {
            ref = qdb_->findTarget(path.first(), *node);
            if (ref.isEmpty())
                break;
            path.removeFirst();
        }

        /*
          Given that *node is not null, we now cconstruct a link
          to the page that *node represents, and then if there is
          a target on that page, we connect the target to the link
          with '#'.
         */
        if (path.isEmpty()) {
            link = linkForNode(*node, relative);
            if (*node && (*node)->subType() == Node::Image)
                link = "images/used-in-examples/" + link;
            if (!ref.isEmpty()) {
                link += QLatin1Char('#') + ref;
            }
        }
    }
    return link;
}

void HtmlGenerator::generateStatus(const Node *node, CodeMarker *marker)
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
                ref = qdb_->findTarget(oldName, docNode);
            }

            if (!ref.isEmpty()) {
                text << Atom(Atom::Link, linkForNode(docNode, node) + QLatin1Char('#') + ref);
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

#ifdef GENERATE_MAC_REFS
/*
  No longer valid.
 */
void HtmlGenerator::generateMacRef(const Node *node, CodeMarker *marker)
{
    if (!pleaseGenerateMacRef || marker == 0)
        return;

    QStringList macRefs = marker->macRefsForNode(node);
    foreach (const QString &macRef, macRefs)
        out() << "<a name=\"" << "//apple_ref/" << macRef << "\"></a>\n";
}
#endif

void HtmlGenerator::beginLink(const QString &link, const Node *node, const Node *relative)
{
    link_ = link;
    if (link_.isEmpty()) {
        if (showBrokenLinks)
            out() << "<i>";
    }
    else if (node == 0 ||
             (relative != 0 && node->status() == relative->status())) {
        out() << "<a href=\"" << link_ << "\">";
    }
    else {
        switch (node->status()) {
        case Node::Obsolete:
            out() << "<a href=\"" << link_ << "\" class=\"obsolete\">";
            break;
        case Node::Compat:
            out() << "<a href=\"" << link_ << "\" class=\"compat\">";
            break;
        default:
            out() << "<a href=\"" << link_ << "\">";
        }
    }
    inLink_ = true;
}

void HtmlGenerator::endLink()
{
    if (inLink_) {
        if (link_.isEmpty()) {
            if (showBrokenLinks)
                out() << "</i>";
        }
        else {
            if (inObsoleteLink) {
                out() << "<sup>(obsolete)</sup>";
            }
            out() << "</a>";
        }
    }
    inLink_ = false;
    inObsoleteLink = false;
}

/*!
  Generates the summary for the \a section. Only used for
  sections of QML element documentation.
 */
void HtmlGenerator::generateQmlSummary(const Section& section,
                                       const Node *relative,
                                       CodeMarker *marker)
{
    if (!section.members.isEmpty()) {
        out() << "<ul>\n";
        NodeList::ConstIterator m;
        m = section.members.constBegin();
        while (m != section.members.constEnd()) {
            out() << "<li class=\"fn\">";
            generateQmlItem(*m,relative,marker,true);
            if ((*m)->type() == Node::QmlPropertyGroup) {
                const QmlPropertyGroupNode* qpgn = static_cast<const QmlPropertyGroupNode*>(*m);
                if (!qpgn->childNodes().isEmpty()) {
                    NodeList::ConstIterator p = qpgn->childNodes().constBegin();
                    out() << "<ul>\n";
                    while (p != qpgn->childNodes().constEnd()) {
                        if ((*p)->type() == Node::QmlProperty) {
                            out() << "<li class=\"fn\">";
                            generateQmlItem(*p, relative, marker, true);
                            out() << "</li>\n";
                        }
                        ++p;
                    }
                    out() << "</ul>\n";
                }
            }
            out() << "</li>\n";
            ++m;
        }
        out() << "</ul>\n";
    }
}

/*!
  Outputs the html detailed documentation for a section
  on a QML element reference page.
 */
void HtmlGenerator::generateDetailedQmlMember(Node *node,
                                              const InnerNode *relative,
                                              CodeMarker *marker)
{
    QmlPropertyNode* qpn = 0;
#ifdef GENERATE_MAC_REFS
    generateMacRef(node, marker);
#endif
    generateExtractionMark(node, MemberMark);
    out() << "<div class=\"qmlitem\">";
    if (node->type() == Node::QmlPropertyGroup) {
        const QmlPropertyGroupNode* qpgn = static_cast<const QmlPropertyGroupNode*>(node);
        NodeList::ConstIterator p = qpgn->childNodes().constBegin();
        out() << "<div class=\"qmlproto\">";
        out() << "<table class=\"qmlname\">";

        QString heading = qpgn->name() + " group";
        out() << "<tr valign=\"top\" class=\"even\">";
        out() << "<th class=\"centerAlign\"><p>";
        out() << "<a name=\"" + refForNode(qpgn) + "\"></a>";
        out() << "<b>" << heading << "</b>";
        out() << "</p></th></tr>";
        while (p != qpgn->childNodes().constEnd()) {
            if ((*p)->type() == Node::QmlProperty) {
                qpn = static_cast<QmlPropertyNode*>(*p);
                out() << "<tr valign=\"top\" class=\"odd\">";
                out() << "<td class=\"tblQmlPropNode\"><p>";
                out() << "<a name=\"" + refForNode(qpn) + "\"></a>";

                if (!qpn->isWritable(qdb_))
                    out() << "<span class=\"qmlreadonly\">read-only</span>";
                if (qpn->isDefault())
                    out() << "<span class=\"qmldefault\">default</span>";
                generateQmlItem(qpn, relative, marker, false);
                out() << "</p></td></tr>";
            }
            ++p;
        }
        out() << "</table>";
        out() << "</div>";
    }
    else if (node->type() == Node::QmlProperty) {
        qpn = static_cast<QmlPropertyNode*>(node);
        out() << "<div class=\"qmlproto\">";
        out() << "<table class=\"qmlname\">";
        out() << "<tr valign=\"top\" class=\"odd\">";
        out() << "<td class=\"tblQmlPropNode\"><p>";
        out() << "<a name=\"" + refForNode(qpn) + "\"></a>";
        if (!qpn->isReadOnlySet()) {
            if (qpn->declarativeCppNode())
                qpn->setReadOnly(!qpn->isWritable(qdb_));
        }
        if (qpn->isReadOnly())
            out() << "<span class=\"qmlreadonly\">read-only</span>";
        if (qpn->isDefault())
            out() << "<span class=\"qmldefault\">default</span>";
        generateQmlItem(qpn, relative, marker, false);
        out() << "</p></td></tr>";
        out() << "</table>";
        out() << "</div>";
    }
    else if (node->type() == Node::QmlSignal) {
        const FunctionNode* qsn = static_cast<const FunctionNode*>(node);
        out() << "<div class=\"qmlproto\">";
        out() << "<table class=\"qmlname\">";
        out() << "<tr valign=\"top\" class=\"odd\">";
        out() << "<td class=\"tblQmlFuncNode\"><p>";
        out() << "<a name=\"" + refForNode(qsn) + "\"></a>";
        generateSynopsis(qsn,relative,marker,CodeMarker::Detailed,false);
        out() << "</p></td></tr>";
        out() << "</table>";
        out() << "</div>";
    }
    else if (node->type() == Node::QmlSignalHandler) {
        const FunctionNode* qshn = static_cast<const FunctionNode*>(node);
        out() << "<div class=\"qmlproto\">";
        out() << "<table class=\"qmlname\">";
        out() << "<tr valign=\"top\" class=\"odd\">";
        out() << "<td class=\"tblQmlFuncNode\"><p>";
        out() << "<a name=\"" + refForNode(qshn) + "\"></a>";
        generateSynopsis(qshn,relative,marker,CodeMarker::Detailed,false);
        out() << "</p></td></tr>";
        out() << "</table>";
        out() << "</div>";
    }
    else if (node->type() == Node::QmlMethod) {
        const FunctionNode* qmn = static_cast<const FunctionNode*>(node);
        out() << "<div class=\"qmlproto\">";
        out() << "<table class=\"qmlname\">";
        out() << "<tr valign=\"top\" class=\"odd\">";
        out() << "<td class=\"tblQmlFuncNode\"><p>";
        out() << "<a name=\"" + refForNode(qmn) + "\"></a>";
        generateSynopsis(qmn,relative,marker,CodeMarker::Detailed,false);
        out() << "</p></td></tr>";
        out() << "</table>";
        out() << "</div>";
    }
    out() << "<div class=\"qmldoc\">";
    generateStatus(node, marker);
    generateBody(node, marker);
    generateThreadSafeness(node, marker);
    generateSince(node, marker);
    generateAlsoList(node, marker);
    out() << "</div>";
    out() << "</div>";
    generateExtractionMark(node, EndMark);
}

/*!
  Output the "Inherits" line for the QML element,
  if there should be one.
 */
void HtmlGenerator::generateQmlInherits(const QmlClassNode* qcn, CodeMarker* marker)
{
    if (!qcn)
        return;
    const QmlClassNode* base = qcn->qmlBaseNode();
    while (base && base->isInternal()) {
        base = base->qmlBaseNode();
    }
    if (base) {
        Text text;
        text << Atom::ParaLeft << "Inherits ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(base));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, base->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << Atom::ParaRight;
        generateText(text, qcn, marker);
    }
}

/*!
  Output the "[Xxx instantiates the C++ class QmlGraphicsXxx]"
  line for the QML element, if there should be one.

  If there is no class node, or if the class node status
  is set to Node::Internal, do nothing.
 */
void HtmlGenerator::generateQmlInstantiates(QmlClassNode* qcn, CodeMarker* marker)
{
    ClassNode* cn = qcn->classNode();
    if (cn && (cn->status() != Node::Internal)) {
        Text text;
        text << Atom::ParaLeft;
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        QString name = qcn->name();
        /*
          Remove the "QML:" prefix, if present.
          It shouldn't be present anymore.
        */
        if (name.startsWith(QLatin1String("QML:")))
            name = name.mid(4); // remove the "QML:" prefix
        text << Atom(Atom::String, name);
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << " instantiates the C++ class ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << Atom::ParaRight;
        generateText(text, qcn, marker);
    }
}

/*!
  Output the "[QmlGraphicsXxx is instantiated by QML Type Xxx]"
  line for the class, if there should be one.

  If there is no QML element, or if the class node status
  is set to Node::Internal, do nothing.
 */
void HtmlGenerator::generateInstantiatedBy(ClassNode* cn, CodeMarker* marker)
{
    if (cn &&  cn->status() != Node::Internal && cn->qmlElement() != 0) {
        const QmlClassNode* qcn = cn->qmlElement();
        Text text;
        text << Atom::ParaLeft;
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << " is instantiated by QML Type ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, qcn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << Atom::ParaRight;
        generateText(text, cn, marker);
    }
}

void HtmlGenerator::generateExtractionMark(const Node *node, ExtractionMarkType markType)
{
    if (markType != EndMark) {
        out() << "<!-- $$$" + node->name();
        if (markType == MemberMark) {
            if (node->type() == Node::Function) {
                const FunctionNode *func = static_cast<const FunctionNode *>(node);
                if (!func->associatedProperty()) {
                    if (func->overloadNumber() == 1)
                        out() << "[overload1]";
                    out() << "$$$" + func->name() + func->rawParameters().remove(' ');
                }
            } else if (node->type() == Node::Property) {
                out() << "-prop";
                const PropertyNode *prop = static_cast<const PropertyNode *>(node);
                const NodeList &list = prop->functions();
                foreach (const Node *propFuncNode, list) {
                    if (propFuncNode->type() == Node::Function) {
                        const FunctionNode *func = static_cast<const FunctionNode *>(propFuncNode);
                        out() << "$$$" + func->name() + func->rawParameters().remove(' ');
                    }
                }
            } else if (node->type() == Node::Enum) {
                const EnumNode *enumNode = static_cast<const EnumNode *>(node);
                foreach (const EnumItem &item, enumNode->items())
                    out() << "$$$" + item.name();
            }
        } else if (markType == BriefMark) {
            out() << "-brief";
        } else if (markType == DetailedDescriptionMark) {
            out() << "-description";
        }
        out() << " -->\n";
    } else {
        out() << "<!-- @@@" + node->name() + " -->\n";
    }
}


/*!
  This function outputs one or more manifest files in XML.
  They are used by Creator.
 */
void HtmlGenerator::generateManifestFiles()
{
    generateManifestFile("examples", "example");
    generateManifestFile("demos", "demo");
    ExampleNode::exampleNodeMap.clear();
    manifestMetaContent.clear();
}

/*!
  This function is called by generateManifestFiles(), once
  for each manifest file to be generated. \a manifest is the
  type of manifest file.
 */
void HtmlGenerator::generateManifestFile(QString manifest, QString element)
{
    if (ExampleNode::exampleNodeMap.isEmpty())
        return;
    QString fileName = manifest +"-manifest.xml";
    QFile file(outputDir() + QLatin1Char('/') + fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return ;
    bool demos = false;
    if (manifest == "demos")
        demos = true;

    bool proceed = false;
    ExampleNodeMap::Iterator i = ExampleNode::exampleNodeMap.begin();
    while (i != ExampleNode::exampleNodeMap.end()) {
        const ExampleNode* en = i.value();
        if (demos) {
            if (en->name().startsWith("demos")) {
                proceed = true;
                break;
            }
        }
        else if (!en->name().startsWith("demos")) {
            proceed = true;
            break;
        }
        ++i;
    }
    if (!proceed)
        return;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("instructionals");
    writer.writeAttribute("module", project);
    writer.writeStartElement(manifest);

    i = ExampleNode::exampleNodeMap.begin();
    while (i != ExampleNode::exampleNodeMap.end()) {
        const ExampleNode* en = i.value();
        if (demos) {
            if (!en->name().startsWith("demos")) {
                ++i;
                continue;
            }
        }
        else if (en->name().startsWith("demos")) {
            ++i;
            continue;
        }
        writer.writeStartElement(element);
        writer.writeAttribute("name", en->title());
        QString docUrl = manifestDir + fileBase(en) + ".html";
        writer.writeAttribute("docUrl", docUrl);
        QStringList proFiles;
        foreach (const Node* child, en->childNodes()) {
            if (child->subType() == Node::File) {
                QString file = child->name();
                if (file.endsWith(".pro") || file.endsWith(".qmlproject")) {
                    proFiles << file;
                }
            }
        }
        if (!proFiles.isEmpty()) {
            if (proFiles.size() == 1) {
                writer.writeAttribute("projectPath", examplesPath + proFiles[0]);
            }
            else {
                QString exampleName = en->name().split('/').last();
                bool proWithExampleNameFound = false;
                for (int j = 0; j < proFiles.size(); j++)
                {
                    if (proFiles[j].endsWith(QStringLiteral("%1/%1.pro").arg(exampleName))
                            || proFiles[j].endsWith(QStringLiteral("%1/%1.qmlproject").arg(exampleName))) {
                        writer.writeAttribute("projectPath", examplesPath + proFiles[j]);
                        proWithExampleNameFound = true;
                        break;
                    }
                }
                if (!proWithExampleNameFound)
                    writer.writeAttribute("projectPath", examplesPath + proFiles[0]);
            }
        }
        if (!en->imageFileName().isEmpty())
            writer.writeAttribute("imageUrl", manifestDir + en->imageFileName());

        QString fullName = project + QLatin1Char('/') + en->title();
        QSet<QString> tags;
        for (int idx=0; idx < manifestMetaContent.size(); ++idx) {
            foreach (const QString &name, manifestMetaContent[idx].names) {
                bool match = false;
                int wildcard = name.indexOf(QChar('*'));
                switch (wildcard) {
                case -1: // no wildcard, exact match
                    match = (fullName == name);
                    break;
                case 0: // '*' matches all
                    match = true;
                    break;
                default: // match with wildcard at the end
                    match = fullName.startsWith(name.left(wildcard));
                }
                if (match) {
                    tags += manifestMetaContent[idx].tags;
                    foreach (const QString &attr, manifestMetaContent[idx].attributes) {
                        QLatin1Char div(':');
                        QStringList attrList = attr.split(div);
                        if (attrList.count() == 1)
                            attrList.append(QStringLiteral("true"));
                        QString attrName = attrList.takeFirst();
                        writer.writeAttribute(attrName, attrList.join(div));
                    }
                }
            }
        }

        writer.writeStartElement("description");
        Text brief = en->doc().briefText();
        if (!brief.isEmpty())
            writer.writeCDATA(brief.toString());
        else
            writer.writeCDATA(QString("No description available"));
        writer.writeEndElement(); // description

        // Add words from module name as tags (QtQuickControls -> qt,quick,controls)
        QRegExp re("([A-Z]+[a-z0-9]*)");
        int pos = 0;
        while ((pos = re.indexIn(project, pos)) != -1) {
            tags << re.cap(1).toLower();
            pos += re.matchedLength();
        }
        tags += QSet<QString>::fromList(en->title().toLower().split(QLatin1Char(' ')));
        if (!tags.isEmpty()) {
            writer.writeStartElement("tags");
            bool wrote_one = false;
            // Exclude invalid and common words
            foreach (QString tag, tags) {
                if (tag.length() < 2)
                    continue;
                if (tag.at(0).isDigit())
                    continue;
                if (tag.at(0) == '-')
                    continue;
                if (tag == QStringLiteral("qt"))
                    continue;
                if (tag.startsWith("example"))
                    continue;
                if (tag.startsWith("chapter"))
                    continue;
                if (tag.endsWith(QLatin1Char(':')))
                    tag.chop(1);
                if (wrote_one)
                    writer.writeCharacters(",");
                writer.writeCharacters(tag);
                wrote_one = true;
            }
            writer.writeEndElement(); // tags
        }

        QString ename = en->name().mid(en->name().lastIndexOf('/')+1);
        QSet<QString> usedNames;
        foreach (const Node* child, en->childNodes()) {
            if (child->subType() == Node::File) {
                QString file = child->name();
                QString fileName = file.mid(file.lastIndexOf('/')+1);
                QString baseName = fileName;
                if ((fileName.count(QChar('.')) > 0) &&
                        (fileName.endsWith(".cpp") ||
                         fileName.endsWith(".h") ||
                         fileName.endsWith(".qml")))
                    baseName.truncate(baseName.lastIndexOf(QChar('.')));
                if (baseName.compare(ename, Qt::CaseInsensitive) == 0) {
                    if (!usedNames.contains(fileName)) {
                        writer.writeStartElement("fileToOpen");
                        writer.writeCharacters(examplesPath + file);
                        writer.writeEndElement(); // fileToOpen
                        usedNames.insert(fileName);
                    }
                }
                else if (fileName.toLower().endsWith("main.cpp") ||
                         fileName.toLower().endsWith("main.qml")) {
                    if (!usedNames.contains(fileName)) {
                        writer.writeStartElement("fileToOpen");
                        writer.writeCharacters(examplesPath + file);
                        writer.writeEndElement(); // fileToOpen
                        usedNames.insert(fileName);
                    }
                }
            }
        }
        writer.writeEndElement(); // example
        ++i;
    }

    writer.writeEndElement(); // examples
    writer.writeEndElement(); // instructionals
    writer.writeEndDocument();
    file.close();
}

/*!
  Reads metacontent - additional attributes and tags to apply
  when generating manifest files, read from config. Takes the
  configuration class \a config as a parameter.
 */
void HtmlGenerator::readManifestMetaContent(const Config &config)
{
    QStringList names = config.getStringList(CONFIG_MANIFESTMETA + Config::dot + QStringLiteral("filters"));

    foreach (const QString &manifest, names) {
        ManifestMetaFilter filter;
        QString prefix = CONFIG_MANIFESTMETA + Config::dot + manifest + Config::dot;
        filter.names = config.getStringSet(prefix + QStringLiteral("names"));
        filter.attributes = config.getStringSet(prefix + QStringLiteral("attributes"));
        filter.tags = config.getStringSet(prefix + QStringLiteral("tags"));
        manifestMetaContent.append(filter);
    }
}

/*!
  Find global entities that have documentation but no
  \e{relates} comand. Report these as errors if they
  are not also marked \e {internal}.

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
void HtmlGenerator::reportOrphans(const InnerNode* parent)
{
    const NodeList& children = parent->childNodes();
    if (children.size() == 0)
        return;

    bool related;
    QString message;
    for (int i=0; i<children.size(); ++i) {
        Node* child = children[i];
        if (!child || child->isInternal() || child->doc().isEmpty())
            continue;
        if (child->relates()) {
            related = true;
            message = child->relates()->name();
        }
        else {
            related = false;
            message = "has documentation but no \\relates command";
        }
        switch (child->type()) {
        case Node::Namespace:
            break;
        case Node::Class:
            break;
        case Node::Document:
            switch (child->subType()) {
            case Node::Example:
                break;
            case Node::HeaderFile:
                break;
            case Node::File:
                break;
            case Node::Image:
                break;
            case Node::Group:
                break;
            case Node::Module:
                break;
            case Node::Page:
                break;
            case Node::ExternalPage:
                break;
            case Node::QmlClass:
                break;
            case Node::QmlBasicType:
                break;
            case Node::QmlModule:
                break;
            case Node::Collision:
                break;
            default:
                break;
            }
            break;
        case Node::Enum:
            if (!related)
                child->location().warning(tr("Global enum, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::Typedef:
            if (!related)
                child->location().warning(tr("Global typedef, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::Function:
            if (!related) {
                const FunctionNode* fn = static_cast<const FunctionNode*>(child);
                if (fn->isMacro())
                    child->location().warning(tr("Global macro, %1, %2").arg(child->name()).arg(message));
                else
                    child->location().warning(tr("Global function, %1(), %2").arg(child->name()).arg(message));
            }
            break;
        case Node::Property:
            break;
        case Node::Variable:
            if (!related)
                child->location().warning(tr("Global variable, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::QmlPropertyGroup:
            break;
        case Node::QmlProperty:
            if (!related)
                child->location().warning(tr("Global QML property, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::QmlSignal:
            if (!related)
                child->location().warning(tr("Global QML, signal, %1 %2").arg(child->name()).arg(message));
            break;
        case Node::QmlSignalHandler:
            if (!related)
                child->location().warning(tr("Global QML signal handler, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::QmlMethod:
            if (!related)
                child->location().warning(tr("Global QML method, %1, %2").arg(child->name()).arg(message));
            break;
        default:
            break;
        }
    }
}

/*!
  Returns a reference to the XML stream writer currently in use.
  There is one XML stream writer open for each XML file being
  written, and they are kept on a stack. The one on top of the
  stack is the one being written to at the moment. In the HTML
  output generator, it is perhaps impossible for there to ever
  be more than one writer open.
 */
QXmlStreamWriter& HtmlGenerator::xmlWriter()
{
    return *xmlWriterStack.top();
}

/*!
  This function is only called for writing ditamaps.

  Calls beginSubPage() in the base class to open the file.
  Then creates a new XML stream writer using the IO device
  from opened file and pushes the XML writer onto a stackj.
  Creates the file named \a fileName in the output directory.
  Attaches a QTextStream to the created file, which is written
  to all over the place using out(). Finally, it sets some
  parameters in the XML writer and calls writeStartDocument().

  It also ensures that a GUID map is created for the output file.
 */
void HtmlGenerator::beginDitamapPage(const InnerNode* node, const QString& fileName)
{
    Generator::beginSubPage(node,fileName);
    QXmlStreamWriter* writer = new QXmlStreamWriter(out().device());
    xmlWriterStack.push(writer);
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(4);
    writer->writeStartDocument();
}

/*!
  This function is only called for writing ditamaps.

  Calls writeEndDocument() and then pops the XML stream writer
  off the stack and deletes it. Then it calls endSubPage() in
  the base class to close the device.
 */
void HtmlGenerator::endDitamapPage()
{
    xmlWriter().writeEndDocument();
    delete xmlWriterStack.pop();
    Generator::endSubPage();
}

/*!
  This function is only called for writing ditamaps.

  Creates the DITA map from the topicrefs in \a node,
  which is a DitaMapNode.
 */
void HtmlGenerator::writeDitaMap(const DitaMapNode* node)
{
    beginDitamapPage(node,node->name());

    QString doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";

    xmlWriter().writeDTD(doctype);
    xmlWriter().writeStartElement("map");
    xmlWriter().writeStartElement("topicmeta");
    xmlWriter().writeStartElement("shortdesc");
    xmlWriter().writeCharacters(node->title());
    xmlWriter().writeEndElement(); // </shortdesc>
    xmlWriter().writeEndElement(); // </topicmeta>
    DitaRefList map = node->map();
    writeDitaRefs(map);
    endDitamapPage();
}

/*!
  Write the \a ditarefs to the current output file.
 */
void HtmlGenerator::writeDitaRefs(const DitaRefList& ditarefs)
{
    foreach (DitaRef* t, ditarefs) {
        if (t->isMapRef())
            xmlWriter().writeStartElement("mapref");
        else
            xmlWriter().writeStartElement("topicref");
        xmlWriter().writeAttribute("navtitle",t->navtitle());
        if (t->href().isEmpty()) {
            const DocNode* fn = qdb_->findDocNodeByTitle(t->navtitle());
            if (fn)
                xmlWriter().writeAttribute("href",fileName(fn));
        }
        else
            xmlWriter().writeAttribute("href",t->href());
        if (t->subrefs() && !t->subrefs()->isEmpty())
            writeDitaRefs(*(t->subrefs()));
        xmlWriter().writeEndElement(); // </topicref> or </mapref>
    }
}

QT_END_NAMESPACE

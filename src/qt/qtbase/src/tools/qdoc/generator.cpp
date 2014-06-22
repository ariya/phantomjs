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
  generator.cpp
*/
#include <qdir.h>
#include <qdebug.h>
#include "codemarker.h"
#include "config.h"
#include "ditaxmlgenerator.h"
#include "doc.h"
#include "editdistance.h"
#include "generator.h"
#include "openedlist.h"
#include "quoter.h"
#include "separator.h"
#include "tokenizer.h"
#include "qdocdatabase.h"

QT_BEGIN_NAMESPACE

Generator* Generator::currentGenerator_;
QStringList Generator::exampleDirs;
QStringList Generator::exampleImgExts;
QMap<QString, QMap<QString, QString> > Generator::fmtLeftMaps;
QMap<QString, QMap<QString, QString> > Generator::fmtRightMaps;
QList<Generator *> Generator::generators;
QStringList Generator::imageDirs;
QStringList Generator::imageFiles;
QMap<QString, QStringList> Generator::imgFileExts;
QString Generator::outDir_;
QString Generator::outSubdir_;
QSet<QString> Generator::outputFormats;
QHash<QString, QString> Generator::outputPrefixes;
QString Generator::project;
QStringList Generator::scriptDirs;
QStringList Generator::scriptFiles;
QString Generator::sinceTitles[] =
{
    "    New Namespaces",
    "    New Classes",
    "    New Member Functions",
    "    New Functions in Namespaces",
    "    New Global Functions",
    "    New Macros",
    "    New Enum Types",
    "    New Typedefs",
    "    New Properties",
    "    New Variables",
    "    New QML Types",
    "    New QML Properties",
    "    New QML Signals",
    "    New QML Signal Handlers",
    "    New QML Methods",
    ""
};
QStringList Generator::styleDirs;
QStringList Generator::styleFiles;
bool Generator::debugging_ = false;
bool Generator::noLinkErrors_ = false;
bool Generator::redirectDocumentationToDevNull_ = false;
Generator::Passes Generator::qdocPass_ = Both;
bool Generator::useOutputSubdirs_ = true;

void Generator::setDebugSegfaultFlag(bool b)
{
    if (b)
        qDebug() << "DEBUG: Setting debug flag.";
    else
        qDebug() << "DEBUG: Clearing debug flag.";
    debugging_ = b;
}

/*!
  Prints \a message as an aid to debugging the release version.
 */
void Generator::debugSegfault(const QString& message)
{
    if (debugging())
        qDebug() << "DEBUG:" << message;
}

/*!
  Constructs the generator base class. Prepends the newly
  constructed generator to the list of output generators.
  Sets a pointer to the QDoc database singleton, which is
  available to the generator subclasses.
 */
Generator::Generator()
    : amp("&amp;"),
      gt("&gt;"),
      lt("&lt;"),
      quot("&quot;"),
      tag("</?@[^>]*>"),
      inLink_(false),
      inContents_(false),
      inSectionHeading_(false),
      inTableHeader_(false),
      threeColumnEnumValueTable_(true),
      showInternal_(false),
      numTableRows_(0)
{
    qdb_ = QDocDatabase::qdocDB();
    generators.prepend(this);
}

/*!
  Destroys the generator after removing it from the list of
  output generators.
 */
Generator::~Generator()
{
    generators.removeAll(this);
}

void Generator::appendFullName(Text& text,
                               const Node *apparentNode,
                               const Node *relative,
                               const Node *actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    text << Atom(Atom::LinkNode, CodeMarker::stringForNode(actualNode))
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
         << Atom(Atom::String, apparentNode->plainFullName(relative))
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
}

void Generator::appendFullName(Text& text,
                               const Node *apparentNode,
                               const QString& fullName,
                               const Node *actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    text << Atom(Atom::LinkNode, CodeMarker::stringForNode(actualNode))
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
         << Atom(Atom::String, fullName)
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
}

void Generator::appendFullNames(Text& text, const NodeList& nodes, const Node* relative)
{
    NodeList::ConstIterator n = nodes.constBegin();
    int index = 0;
    while (n != nodes.constEnd()) {
        appendFullName(text,*n,relative);
        text << comma(index++,nodes.count());
        ++n;
    }
}

void Generator::appendSortedNames(Text& text, const ClassNode *classe, const QList<RelatedClass> &classes)
{
    QList<RelatedClass>::ConstIterator r;
    QMap<QString,Text> classMap;
    int index = 0;

    r = classes.constBegin();
    while (r != classes.constEnd()) {
        if ((*r).node->access() == Node::Public &&
                (*r).node->status() != Node::Internal
                && !(*r).node->doc().isEmpty()) {
            Text className;
            appendFullName(className, (*r).node, classe);
            classMap[className.toString().toLower()] = className;
        }
        ++r;
    }

    QStringList classNames = classMap.keys();
    classNames.sort();

    foreach (const QString &className, classNames) {
        text << classMap[className];
        text << separator(index++, classNames.count());
    }
}

void Generator::appendSortedQmlNames(Text& text, const Node* base, const NodeList& subs)
{
    QMap<QString,Text> classMap;
    int index = 0;

    for (int i = 0; i < subs.size(); ++i) {
        Text t;
        if (!base->isQtQuickNode() || !subs[i]->isQtQuickNode() ||
                (base->qmlModuleName() == subs[i]->qmlModuleName())) {
            appendFullName(t, subs[i], base);
            classMap[t.toString().toLower()] = t;
        }
    }

    QStringList names = classMap.keys();
    names.sort();

    foreach (const QString &name, names) {
        text << classMap[name];
        text << separator(index++, names.count());
    }
}

QMultiMap<QString,QString> outFileNames;

/*!
  For debugging qdoc.
 */
void Generator::writeOutFileNames()
{
    QFile files("outputlist.txt");
    if (!files.open(QFile::WriteOnly))
        return;
    QTextStream filesout(&files);
    QMultiMap<QString,QString>::ConstIterator i = outFileNames.begin();
    while (i != outFileNames.end()) {
        filesout << i.key() << "\n";
        ++i;
    }
}

/*!
  Creates the file named \a fileName in the output directory.
  Attaches a QTextStream to the created file, which is written
  to all over the place using out().
 */
void Generator::beginSubPage(const InnerNode* node, const QString& fileName)
{
    QString path = outputDir() + QLatin1Char('/');
    if (Generator::useOutputSubdirs() && !node->outputSubdirectory().isEmpty())
        path += node->outputSubdirectory() + QLatin1Char('/');
    path += fileName;

    QFile* outFile = new QFile(redirectDocumentationToDevNull_ ? QStringLiteral("/dev/null") : path);
    if (!redirectDocumentationToDevNull_ && outFile->exists())
        node->location().error(tr("HTML file already exists; overwriting %1").arg(outFile->fileName()));
    if (!outFile->open(QFile::WriteOnly))
        node->location().fatal(tr("Cannot open output file '%1'").arg(outFile->fileName()));
    Generator::debugSegfault("Writing: " + path);
    outFileNames.insert(fileName,fileName);
    QTextStream* out = new QTextStream(outFile);

#ifndef QT_NO_TEXTCODEC
    if (outputCodec)
        out->setCodec(outputCodec);
#endif
    outStreamStack.push(out);
    const_cast<InnerNode*>(node)->setOutputFileName(fileName);
}

/*!
  Flush the text stream associated with the subpage, and
  then pop it off the text stream stack and delete it.
  This terminates output of the subpage.
 */
void Generator::endSubPage()
{
    outStreamStack.top()->flush();
    delete outStreamStack.top()->device();
    delete outStreamStack.pop();
}

QString Generator::fileBase(const Node *node) const
{
    if (node->relates())
        node = node->relates();
    else if (!node->isInnerNode())
        node = node->parent();
    if (node->type() == Node::QmlPropertyGroup) {
        node = node->parent();
    }

    if (node->type() == Node::Document && node->subType() == Node::Collision) {
        const NameCollisionNode* ncn = static_cast<const NameCollisionNode*>(node);
        if (ncn->currentChild())
            return fileBase(ncn->currentChild());
    }

    if (node->hasBaseName())
        return node->baseName();

    QString base;
    if (node->type() == Node::Document) {
        base = node->name();
        if (node->subType() == Node::Collision) {
            const NameCollisionNode* ncn = static_cast<const NameCollisionNode*>(node);
            if (ncn->currentChild())
                return fileBase(ncn->currentChild());
            base.prepend("collision-");
        }
        //Was QDOC2_COMPAT, required for index.html
        if (base.endsWith(".html"))
            base.truncate(base.length() - 5);

        if (node->isQmlNode()) {
            if (!node->qmlModuleName().isEmpty()) {
                base.prepend(node->qmlModuleName() + QLatin1Char('-'));
            }
            /*
              To avoid file name conflicts in the html directory,
              we prepend a prefix (by default, "qml-") to the file name of QML
              element doc files.
            */
            if ((node->subType() == Node::QmlClass) || (node->subType() == Node::QmlBasicType)) {
                base.prepend(outputPrefix(QLatin1String("QML")));
            }
        }
        else if (node->subType() == Node::QmlModule) {
            base.append("-qmlmodule");
        }
        else if (node->subType() == Node::Module) {
            base.append("-module");
        }
        if (node->isExample() || node->isExampleFile()) {
            QString modPrefix(node->moduleName());
            if (modPrefix.isEmpty()) {
                modPrefix = project;
            }
            base.prepend(modPrefix.toLower() + QLatin1Char('-'));
        }
        if (node->isExample()) {
            base.append(QLatin1String("-example"));
        }
    }
    else {
        const Node *p = node;
        forever {
            const Node *pp = p->parent();
            base.prepend(p->name());
            if (!pp || pp->name().isEmpty() || pp->type() == Node::Document)
                break;
            base.prepend(QLatin1Char('-'));
            p = pp;
        }
    }

    // the code below is effectively equivalent to:
    //   base.replace(QRegExp("[^A-Za-z0-9]+"), " ");
    //   base = base.trimmed();
    //   base.replace(QLatin1Char(' '), QLatin1Char('-'));
    //   base = base.toLower();
    // as this function accounted for ~8% of total running time
    // we optimize a bit...

    QString res;
    // +5 prevents realloc in fileName() below
    res.reserve(base.size() + 5);
    bool begun = false;
    for (int i = 0; i != base.size(); ++i) {
        QChar c = base.at(i);
        uint u = c.unicode();
        if (u >= 'A' && u <= 'Z')
            u += 'a' - 'A';
        if ((u >= 'a' &&  u <= 'z') || (u >= '0' && u <= '9')) {
            res += QLatin1Char(u);
            begun = true;
        }
        else if (begun) {
            res += QLatin1Char('-');
            begun = false;
        }
    }
    while (res.endsWith(QLatin1Char('-')))
        res.chop(1);
    Node* n = const_cast<Node*>(node);
    n->setBaseName(res);
    return res;
}

/*!
  If the \a node has a URL, return the URL as the file name.
  Otherwise, construct the file name from the fileBase() and
  the fileExtension(), and return the constructed name.
 */
QString Generator::fileName(const Node* node) const
{
    if (!node->url().isEmpty())
        return node->url();

    QString name = fileBase(node);
    name += QLatin1Char('.');
    name += fileExtension();
    return name;
}

QMap<QString, QString>& Generator::formattingLeftMap()
{
    return fmtLeftMaps[format()];
}

QMap<QString, QString>& Generator::formattingRightMap()
{
    return fmtRightMaps[format()];
}

/*!
  Returns the full document location.
 */
QString Generator::fullDocumentLocation(const Node *node, bool useSubdir)
{
    if (!node)
        return QString();
    if (!node->url().isEmpty())
        return node->url();

    QString parentName;
    QString anchorRef;
    QString fdl;

    /*
      If the useSubdir parameter is set, then the output is
      being sent to subdirectories of the output directory.
      Prepend the subdirectory name + '/' to the result.
     */
    if (useSubdir) {
        fdl = node->outputSubdirectory();
        if (!fdl.isEmpty())
            fdl.append(QLatin1Char('/'));
    }
    if (node->type() == Node::Namespace) {

        // The root namespace has no name - check for this before creating
        // an attribute containing the location of any documentation.

        if (!fileBase(node).isEmpty())
            parentName = fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
        else
            return QString();
    }
    else if (node->type() == Node::Document) {
        if ((node->subType() == Node::QmlClass) ||
                (node->subType() == Node::QmlBasicType)) {
            QString fb = fileBase(node);
            if (fb.startsWith(Generator::outputPrefix(QLatin1String("QML"))))
                return fb + QLatin1Char('.') + currentGenerator()->fileExtension();
            else {
                QString mq;
                if (!node->qmlModuleName().isEmpty()) {
                    mq = node->qmlModuleName().replace(QChar('.'),QChar('-'));
                    mq = mq.toLower() + QLatin1Char('-');
                }
                return fdl+ Generator::outputPrefix(QLatin1String("QML")) + mq +
                        fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
            }
        }
        else {
            parentName = fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
        }
    }
    else if (fileBase(node).isEmpty())
        return QString();

    Node *parentNode = 0;

    if ((parentNode = node->relates())) {
        parentName = fullDocumentLocation(node->relates());
    }
    else if ((parentNode = node->parent())) {
        if (parentNode->type() == Node::QmlPropertyGroup) {
            parentNode = parentNode->parent();
            parentName = fullDocumentLocation(parentNode);
        }
        else {
            parentName = fullDocumentLocation(node->parent());
        }
    }

    switch (node->type()) {
    case Node::Class:
    case Node::Namespace:
        parentName = fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
        break;
    case Node::Function:
    {
        const FunctionNode *functionNode =
                static_cast<const FunctionNode *>(node);

        if (functionNode->metaness() == FunctionNode::Dtor)
            anchorRef = "#dtor." + functionNode->name().mid(1);

        else if (functionNode->associatedProperty())
            return fullDocumentLocation(functionNode->associatedProperty());

        else if (functionNode->overloadNumber() > 1)
            anchorRef = QLatin1Char('#') + functionNode->name()
                    + QLatin1Char('-') + QString::number(functionNode->overloadNumber());
        else
            anchorRef = QLatin1Char('#') + functionNode->name();
        break;
    }
    /*
      Use node->name() instead of fileBase(node) as
      the latter returns the name in lower-case. For
      HTML anchors, we need to preserve the case.
    */
    case Node::Enum:
        anchorRef = QLatin1Char('#') + node->name() + "-enum";
        break;
    case Node::Typedef:
    {
        const TypedefNode *tdef = static_cast<const TypedefNode *>(node);
        if (tdef->associatedEnum()) {
            return fullDocumentLocation(tdef->associatedEnum());
        }
        anchorRef = QLatin1Char('#') + node->name() + "-typedef";
        break;
    }
    case Node::Property:
        anchorRef = QLatin1Char('#') + node->name() + "-prop";
        break;
    case Node::QmlProperty:
        anchorRef = QLatin1Char('#') + node->name() + "-prop";
        break;
    case Node::QmlSignal:
        anchorRef = QLatin1Char('#') + node->name() + "-signal";
        break;
    case Node::QmlSignalHandler:
        anchorRef = QLatin1Char('#') + node->name() + "-signal-handler";
        break;
    case Node::QmlMethod:
        anchorRef = QLatin1Char('#') + node->name() + "-method";
        break;
    case Node::Variable:
        anchorRef = QLatin1Char('#') + node->name() + "-var";
        break;
    case Node::Document:
    {
        parentName = fileBase(node);
        parentName.replace(QLatin1Char('/'), QLatin1Char('-')).replace(QLatin1Char('.'), QLatin1Char('-'));
        parentName += QLatin1Char('.') + currentGenerator()->fileExtension();
    }
        break;
    default:
        break;
    }

    // Various objects can be compat (deprecated) or obsolete.
    if (node->type() != Node::Class && node->type() != Node::Namespace) {
        switch (node->status()) {
        case Node::Compat:
            parentName.replace(QLatin1Char('.') + currentGenerator()->fileExtension(),
                               "-compat." + currentGenerator()->fileExtension());
            break;
        case Node::Obsolete:
            parentName.replace(QLatin1Char('.') + currentGenerator()->fileExtension(),
                               "-obsolete." + currentGenerator()->fileExtension());
            break;
        default:
            ;
        }
    }

    return fdl + parentName.toLower() + anchorRef;
}

void Generator::generateAlsoList(const Node *node, CodeMarker *marker)
{
    QList<Text> alsoList = node->doc().alsoList();
    supplementAlsoList(node, alsoList);

    if (!alsoList.isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "See also "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        for (int i = 0; i < alsoList.size(); ++i)
            text << alsoList.at(i) << separator(i, alsoList.size());

        text << Atom::ParaRight;
        generateText(text, node, marker);
    }
}

int Generator::generateAtom(const Atom * /* atom */,
                            const Node * /* relative */,
                            CodeMarker * /* marker */)
{
    return 0;
}

const Atom *Generator::generateAtomList(const Atom *atom,
                                        const Node *relative,
                                        CodeMarker *marker,
                                        bool generate,
                                        int &numAtoms)
{
    while (atom) {
        if (atom->type() == Atom::FormatIf) {
            int numAtoms0 = numAtoms;
            bool rightFormat = canHandleFormat(atom->string());
            atom = generateAtomList(atom->next(),
                                    relative,
                                    marker,
                                    generate && rightFormat,
                                    numAtoms);
            if (!atom)
                return 0;

            if (atom->type() == Atom::FormatElse) {
                ++numAtoms;
                atom = generateAtomList(atom->next(),
                                        relative,
                                        marker,
                                        generate && !rightFormat,
                                        numAtoms);
                if (!atom)
                    return 0;
            }

            if (atom->type() == Atom::FormatEndif) {
                if (generate && numAtoms0 == numAtoms) {
                    relative->location().warning(tr("Output format %1 not handled %2")
                                                 .arg(format()).arg(outFileName()));
                    Atom unhandledFormatAtom(Atom::UnhandledFormat, format());
                    generateAtomList(&unhandledFormatAtom,
                                     relative,
                                     marker,
                                     generate,
                                     numAtoms);
                }
                atom = atom->next();
            }
        }
        else if (atom->type() == Atom::FormatElse ||
                 atom->type() == Atom::FormatEndif) {
            return atom;
        }
        else {
            int n = 1;
            if (generate) {
                n += generateAtom(atom, relative, marker);
                numAtoms += n;
            }
            while (n-- > 0)
                atom = atom->next();
        }
    }
    return 0;
}

void Generator::generateBody(const Node *node, CodeMarker *marker)
{
    bool quiet = false;

    if (node->type() == Node::Document) {
        const DocNode *dn = static_cast<const DocNode *>(node);
        if ((dn->subType() == Node::File) || (dn->subType() == Node::Image)) {
            quiet = true;
        }
    }
    if (node->doc().isEmpty()) {
        if (!node->isWrapper() && !quiet && !node->isReimp()) { // ### might be unnecessary
            node->location().warning(tr("No documentation for '%1'").arg(node->plainFullName()));
        }
    }
    else {
        if (node->type() == Node::Function) {
            const FunctionNode *func = static_cast<const FunctionNode *>(node);
            if (func->reimplementedFrom() != 0)
                generateReimplementedFrom(func, marker);
        }

        if (!generateText(node->doc().body(), node, marker)) {
            if (node->isReimp())
                return;
        }

        if (node->type() == Node::Enum) {
            const EnumNode *enume = (const EnumNode *) node;

            QSet<QString> definedItems;
            QList<EnumItem>::ConstIterator it = enume->items().constBegin();
            while (it != enume->items().constEnd()) {
                definedItems.insert((*it).name());
                ++it;
            }

            QSet<QString> documentedItems = enume->doc().enumItemNames().toSet();
            QSet<QString> allItems = definedItems + documentedItems;
            if (allItems.count() > definedItems.count() ||
                    allItems.count() > documentedItems.count()) {
                QSet<QString>::ConstIterator a = allItems.constBegin();
                while (a != allItems.constEnd()) {
                    if (!definedItems.contains(*a)) {
                        QString details;
                        QString best = nearestName(*a, definedItems);
                        if (!best.isEmpty() && !documentedItems.contains(best))
                            details = tr("Maybe you meant '%1'?").arg(best);

                        node->doc().location().warning(tr("No such enum item '%1' in %2")
                                                       .arg(*a).arg(node->plainFullName()), details);
                        if (*a == "Void")
                            qDebug() << "VOID:" << node->name() << definedItems;
                    }
                    else if (!documentedItems.contains(*a)) {
                        node->doc().location().warning(tr("Undocumented enum item '%1' in %2")
                                                       .arg(*a).arg(node->plainFullName()));
                    }
                    ++a;
                }
            }
        }
        else if (node->type() == Node::Function) {
            const FunctionNode *func = static_cast<const FunctionNode *>(node);
            QSet<QString> definedParams;
            QList<Parameter>::ConstIterator p = func->parameters().constBegin();
            while (p != func->parameters().constEnd()) {
                if ((*p).name().isEmpty() && (*p).leftType() != QLatin1String("...")
                        && func->name() != QLatin1String("operator++")
                        && func->name() != QLatin1String("operator--")) {
                    node->doc().location().warning(tr("Missing parameter name"));
                }
                else {
                    definedParams.insert((*p).name());
                }
                ++p;
            }

            QSet<QString> documentedParams = func->doc().parameterNames();
            QSet<QString> allParams = definedParams + documentedParams;
            if (allParams.count() > definedParams.count()
                    || allParams.count() > documentedParams.count()) {
                QSet<QString>::ConstIterator a = allParams.constBegin();
                while (a != allParams.constEnd()) {
                    if (!definedParams.contains(*a)) {
                        QString details;
                        QString best = nearestName(*a, definedParams);
                        if (!best.isEmpty())
                            details = tr("Maybe you meant '%1'?").arg(best);

                        node->doc().location().warning(
                                    tr("No such parameter '%1' in %2").arg(*a).arg(node->plainFullName()),
                                    details);
                    }
                    else if (!(*a).isEmpty() && !documentedParams.contains(*a)) {
                        bool needWarning = (func->status() > Node::Obsolete);
                        if (func->overloadNumber() > 1) {
                            FunctionNode *primaryFunc =
                                    func->parent()->findFunctionNode(func->name());
                            if (primaryFunc) {
                                foreach (const Parameter &param,
                                         primaryFunc->parameters()) {
                                    if (param.name() == *a) {
                                        needWarning = false;
                                        break;
                                    }
                                }
                            }
                        }
                        if (needWarning && !func->isReimp())
                            node->doc().location().warning(
                                        tr("Undocumented parameter '%1' in %2")
                                        .arg(*a).arg(node->plainFullName()));
                    }
                    ++a;
                }
            }
            /*
              Something like this return value check should
              be implemented at some point.
            */
            if (func->status() > Node::Obsolete && func->returnType() == "bool"
                    && func->reimplementedFrom() == 0 && !func->isOverload()) {
                QString body = func->doc().body().toString();
                if (!body.contains("return", Qt::CaseInsensitive))
                    node->doc().location().warning(tr("Undocumented return value"));
            }
        }
    }

    if (node->type() == Node::Document) {
        const DocNode *dn = static_cast<const DocNode *>(node);
        if (dn->subType() == Node::Example) {
            generateExampleFiles(dn, marker);
        }
        else if (dn->subType() == Node::File) {
            Text text;
            Quoter quoter;
            Doc::quoteFromFile(dn->doc().location(), quoter, dn->name());
            QString code = quoter.quoteTo(dn->location(), QString(), QString());
            CodeMarker *codeMarker = CodeMarker::markerForFileName(dn->name());
            text << Atom(codeMarker->atomType(), code);
            generateText(text, dn, codeMarker);
        }
    }
}

void Generator::generateClassLikeNode(InnerNode* /* classe */, CodeMarker* /* marker */)
{
}

void Generator::generateExampleFiles(const DocNode *dn, CodeMarker *marker)
{
    if (dn->childNodes().isEmpty())
        return;
    generateFileList(dn, marker, Node::File, QString("Files:"));
    generateFileList(dn, marker, Node::Image, QString("Images:"));
}

void Generator::generateDocNode(DocNode* /* dn */, CodeMarker* /* marker */)
{
}

/*!
  This function is called when the documentation for an
  example is being formatted. It outputs the list of source
  files comprising the example, and the list of images used
  by the example. The images are copied into a subtree of
  \c{...doc/html/images/used-in-examples/...}
 */
void Generator::generateFileList(const DocNode* dn,
                                 CodeMarker* marker,
                                 Node::SubType subtype,
                                 const QString& tag)
{
    int count = 0;
    Text text;
    OpenedList openedList(OpenedList::Bullet);

    text << Atom::ParaLeft << tag << Atom::ParaRight
         << Atom(Atom::ListLeft, openedList.styleString());

    foreach (const Node* child, dn->childNodes()) {
        if (child->subType() == subtype) {
            ++count;
            QString file = child->name();
            if (subtype == Node::Image) {
                if (!file.isEmpty()) {
                    QDir dirInfo;
                    QString userFriendlyFilePath;
                    QString srcPath = Config::findFile(dn->location(),
                                                       QStringList(),
                                                       exampleDirs,
                                                       file,
                                                       exampleImgExts,
                                                       userFriendlyFilePath);
                    userFriendlyFilePath.truncate(userFriendlyFilePath.lastIndexOf('/'));

                    QString imgOutDir = outDir_ + "/images/used-in-examples/" + userFriendlyFilePath;
                    if (!dirInfo.mkpath(imgOutDir))
                        dn->location().fatal(tr("Cannot create output directory '%1'")
                                               .arg(imgOutDir));

                    QString imgOutName = Config::copyFile(dn->location(),
                                                          srcPath,
                                                          file,
                                                          imgOutDir);
                }

            }

            openedList.next();
            text << Atom(Atom::ListItemNumber, openedList.numberString())
                 << Atom(Atom::ListItemLeft, openedList.styleString())
                 << Atom::ParaLeft
                 << Atom(Atom::Link, file)
                 << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << file
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                 << Atom::ParaRight
                 << Atom(Atom::ListItemRight, openedList.styleString());
        }
    }
    text << Atom(Atom::ListRight, openedList.styleString());
    if (count > 0)
        generateText(text, dn, marker);
}

void Generator::generateInheritedBy(const ClassNode *classe, CodeMarker *marker)
{
    if (!classe->derivedClasses().isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Inherited by: "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        appendSortedNames(text, classe, classe->derivedClasses());
        text << Atom::ParaRight;
        generateText(text, classe, marker);
    }
}

void Generator::generateInherits(const ClassNode *classe, CodeMarker *marker)
{
    QList<RelatedClass>::ConstIterator r;
    int index;

    if (!classe->baseClasses().isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Inherits: "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

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
        generateText(text, classe, marker);
    }
}

/*!
  Recursive writing of HTML files from the root \a node.

  \note NameCollisionNodes are skipped here and processed
  later. See HtmlGenerator::generateCollisionPages() for
  more on this.
 */
void Generator::generateInnerNode(InnerNode* node)
{
    if (!node->url().isNull())
        return;
    if (node->isIndexNode())
        return;
    if (node->isInternal() && !showInternal_)
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
            beginSubPage(node, fileName(node));
            if (node->type() == Node::Namespace || node->type() == Node::Class) {
                generateClassLikeNode(node, marker);
            }
            else if (node->type() == Node::Document) {
                generateDocNode(static_cast<DocNode*>(node), marker);
            }
            endSubPage();
        }
    }

    int i = 0;
    while (i < node->childNodes().count()) {
        Node *c = node->childNodes().at(i);
        if (c->isInnerNode() && c->access() != Node::Private) {
            generateInnerNode((InnerNode*)c);
        }
        ++i;
    }
}

/*!
  Generate a list of maintainers in the output
 */
void Generator::generateMaintainerList(const InnerNode* node, CodeMarker* marker)
{
    QStringList sl = getMetadataElements(node,"maintainer");

    if (!sl.isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Maintained by: "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        for (int i = 0; i < sl.size(); ++i)
            text << sl.at(i) << separator(i, sl.size());

        text << Atom::ParaRight;
        generateText(text, node, marker);
    }
}

/*!
  Output the "Inherit by" list for the QML element,
  if it is inherited by any other elements.
 */
void Generator::generateQmlInheritedBy(const QmlClassNode* qcn,
                                              CodeMarker* marker)
{
    if (qcn) {
        NodeList subs;
        QmlClassNode::subclasses(qcn->name(),subs);
        if (!subs.isEmpty()) {
            Text text;
            text << Atom::ParaLeft << "Inherited by ";
            appendSortedQmlNames(text,qcn,subs);
            text << Atom::ParaRight;
            generateText(text, qcn, marker);
        }
    }
}

/*!
 */
void Generator::generateQmlInherits(const QmlClassNode* , CodeMarker* )
{
    // stub.
}

/*!
  Extract sections of markup text surrounded by \e qmltext
  and \e endqmltext and output them.
 */
bool Generator::generateQmlText(const Text& text,
                                const Node *relative,
                                CodeMarker *marker,
                                const QString& /* qmlName */ )
{
    const Atom* atom = text.firstAtom();
    bool result = false;

    if (atom != 0) {
        initializeTextOutput();
        while (atom) {
            if (atom->type() != Atom::QmlText)
                atom = atom->next();
            else {
                atom = atom->next();
                while (atom && (atom->type() != Atom::EndQmlText)) {
                    int n = 1 + generateAtom(atom, relative, marker);
                    while (n-- > 0)
                        atom = atom->next();
                }
            }
        }
        result = true;
    }
    return result;
}

void Generator::generateReimplementedFrom(const FunctionNode *func,
                                          CodeMarker *marker)
{
    if (func->reimplementedFrom() != 0) {
        const FunctionNode *from = func->reimplementedFrom();
        if (from->access() != Node::Private &&
                from->parent()->access() != Node::Private) {
            Text text;
            text << Atom::ParaLeft << "Reimplemented from ";
            QString fullName =  from->parent()->name() + "::" + from->name() + "()";
            appendFullName(text, from->parent(), fullName, from);
            text << "." << Atom::ParaRight;
            generateText(text, func, marker);
        }
    }
}

void Generator::generateSince(const Node *node, CodeMarker *marker)
{
    if (!node->since().isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << "This "
             << typeString(node);
        if (node->type() == Node::Enum)
            text << " was introduced or modified in ";
        else
            text << " was introduced in ";

        QStringList since = node->since().split(QLatin1Char(' '));
        if (since.count() == 1) {
            // If there is only one argument, assume it is the Qt version number.
            text << " Qt " << since[0];
        } else {
            // Otherwise, reconstruct the <project> <version> string.
            text << " " << since.join(' ');
        }

        text << "." << Atom::ParaRight;
        generateText(text, node, marker);
    }
}

void Generator::generateStatus(const Node *node, CodeMarker *marker)
{
    Text text;

    switch (node->status()) {
    case Node::Commendable:
    case Node::Main:
        break;
    case Node::Preliminary:
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD)
             << "This "
             << typeString(node)
             << " is under development and is subject to change."
             << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
             << Atom::ParaRight;
        break;
    case Node::Deprecated:
        text << Atom::ParaLeft;
        if (node->isInnerNode())
            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD);
        text << "This " << typeString(node) << " is deprecated.";
        if (node->isInnerNode())
            text << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD);
        text << Atom::ParaRight;
        break;
    case Node::Obsolete:
        text << Atom::ParaLeft;
        if (node->isInnerNode())
            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD);
        text << "This " << typeString(node) << " is obsolete.";
        if (node->isInnerNode())
            text << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD);
        text << " It is provided to keep old source code working. "
             << "We strongly advise against "
             << "using it in new code." << Atom::ParaRight;
        break;
    case Node::Compat:
        // reimplemented in HtmlGenerator subclass
        if (node->isInnerNode()) {
            text << Atom::ParaLeft
                 << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD)
                 << "This "
                 << typeString(node)
                 << " is part of the Qt compatibility layer."
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
                 << " It is provided to keep old source code working. "
                 << "We strongly advise against using it in new code."
                 << Atom::ParaRight;
        }
        break;
    case Node::Internal:
    default:
        break;
    }
    generateText(text, node, marker);
}

bool Generator::generateText(const Text& text,
                             const Node *relative,
                             CodeMarker *marker)
{
    bool result = false;
    if (text.firstAtom() != 0) {
        int numAtoms = 0;
        initializeTextOutput();
        generateAtomList(text.firstAtom(),
                         relative,
                         marker,
                         true,
                         numAtoms);
        result = true;
    }
    return result;
}

void Generator::generateThreadSafeness(const Node *node, CodeMarker *marker)
{
    Text text;
    Node::ThreadSafeness threadSafeness = node->threadSafeness();

    Text rlink;
    rlink << Atom(Atom::Link,"reentrant")
          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
          << "reentrant"
          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);

    Text tlink;
    tlink << Atom(Atom::Link,"thread-safe")
          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
          << "thread-safe"
          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);

    switch (threadSafeness) {
    case Node::UnspecifiedSafeness:
        break;
    case Node::NonReentrant:
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Warning:"
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD)
             << " This "
             << typeString(node)
             << " is not "
             << rlink
             << "."
             << Atom::ParaRight;
        break;
    case Node::Reentrant:
    case Node::ThreadSafe:
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Note:"
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD)
             << " ";

        if (node->isInnerNode()) {
            const InnerNode* innerNode = static_cast<const InnerNode*>(node);
            text << "All functions in this "
                 << typeString(node)
                 << " are ";
            if (threadSafeness == Node::ThreadSafe)
                text << tlink;
            else
                text << rlink;

            bool exceptions = false;
            NodeList reentrant;
            NodeList threadsafe;
            NodeList nonreentrant;
            NodeList::ConstIterator c = innerNode->childNodes().constBegin();
            while (c != innerNode->childNodes().constEnd()) {

                if ((*c)->status() != Node::Obsolete){
                    switch ((*c)->threadSafeness()) {
                    case Node::Reentrant:
                        reentrant.append(*c);
                        if (threadSafeness == Node::ThreadSafe)
                            exceptions = true;
                        break;
                    case Node::ThreadSafe:
                        threadsafe.append(*c);
                        if (threadSafeness == Node::Reentrant)
                            exceptions = true;
                        break;
                    case Node::NonReentrant:
                        nonreentrant.append(*c);
                        exceptions = true;
                        break;
                    default:
                        break;
                    }
                }
                ++c;
            }
            if (!exceptions)
                text << ".";
            else if (threadSafeness == Node::Reentrant) {
                if (nonreentrant.isEmpty()) {
                    if (!threadsafe.isEmpty()) {
                        text << ", but ";
                        appendFullNames(text,threadsafe,innerNode);
                        singularPlural(text,threadsafe);
                        text << " also " << tlink << ".";
                    }
                    else
                        text << ".";
                }
                else {
                    text << ", except for ";
                    appendFullNames(text,nonreentrant,innerNode);
                    text << ", which";
                    singularPlural(text,nonreentrant);
                    text << " nonreentrant.";
                    if (!threadsafe.isEmpty()) {
                        text << " ";
                        appendFullNames(text,threadsafe,innerNode);
                        singularPlural(text,threadsafe);
                        text << " " << tlink << ".";
                    }
                }
            }
            else { // thread-safe
                if (!nonreentrant.isEmpty() || !reentrant.isEmpty()) {
                    text << ", except for ";
                    if (!reentrant.isEmpty()) {
                        appendFullNames(text,reentrant,innerNode);
                        text << ", which";
                        singularPlural(text,reentrant);
                        text << " only " << rlink;
                        if (!nonreentrant.isEmpty())
                            text << ", and ";
                    }
                    if (!nonreentrant.isEmpty()) {
                        appendFullNames(text,nonreentrant,innerNode);
                        text << ", which";
                        singularPlural(text,nonreentrant);
                        text << " nonreentrant.";
                    }
                    text << ".";
                }
            }
        }
        else {
            text << "This " << typeString(node) << " is ";
            if (threadSafeness == Node::ThreadSafe)
                text << tlink;
            else
                text << rlink;
            text << ".";
        }
        text << Atom::ParaRight;
    }
    generateText(text,node,marker);
}

/*!
  Traverses the database recursivly to generate all the documentation.
 */
void Generator::generateTree()
{
    generateInnerNode(qdb_->treeRoot());
}

Generator *Generator::generatorForFormat(const QString& format)
{
    QList<Generator *>::ConstIterator g = generators.constBegin();
    while (g != generators.constEnd()) {
        if ((*g)->format() == format)
            return *g;
        ++g;
    }
    return 0;
}

/*!
  This function can be called if getLink() returns an empty
  string. It tests the \a atom string to see if it is a link
  of the form <element> :: <name>, where <element> is a QML
  element or component without a module qualifier. If so, it
  constructs a link to the <name> clause on the disambiguation
  page for <element> and returns that link string. It also
  adds the <name> as a target in the NameCollisionNode for
  <element>. These clauses are then constructed when the
  disambiguation page is actually generated.
 */
QString Generator::getCollisionLink(const Atom* atom)
{
    QString link;
    if (!atom->string().contains("::"))
        return link;
    QStringList path = atom->string().split("::");
    NameCollisionNode* ncn = qdb_->findCollisionNode(path[0]);
    if (ncn) {
        QString label;
        if (atom->next() && atom->next()->next()) {
            if (atom->next()->type() == Atom::FormattingLeft &&
                    atom->next()->next()->type() == Atom::String)
                label = atom->next()->next()->string();
        }
        ncn->addLinkTarget(path[1],label);
        link = fileName(ncn);
        link += QLatin1Char('#');
        link += Doc::canonicalTitle(path[1]);
    }
    return link;
}


/*!
  Looks up the tag \a t in the map of metadata values for the
  current topic in \a inner. If a value for the tag is found,
  the value is returned.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
QString Generator::getMetadataElement(const InnerNode* inner, const QString& t)
{
    QString s;
    QStringMultiMap& metaTagMap = const_cast<QStringMultiMap&>(inner->doc().metaTagMap());
    QStringMultiMap::iterator i = metaTagMap.find(t);
    if (i != metaTagMap.end()) {
        s = i.value();
        metaTagMap.erase(i);
    }
    return s;
}

/*!
  Looks up the tag \a t in the map of metadata values for the
  current topic in \a inner. If values for the tag are found,
  they are returned in a string list.

  \note If \a t is found in the metadata map, all the pairs
  having the key \a t are erased. i.e. Once you call this
  function for a particular \a t, you consume \a t.
 */
QStringList Generator::getMetadataElements(const InnerNode* inner, const QString& t)
{
    QStringList s;
    QStringMultiMap& metaTagMap = const_cast<QStringMultiMap&>(inner->doc().metaTagMap());
    s = metaTagMap.values(t);
    if (!s.isEmpty())
        metaTagMap.remove(t);
    return s;
}

/*!
  Returns a relative path name for an image.
 */
QString Generator::imageFileName(const Node *relative, const QString& fileBase)
{
    QString userFriendlyFilePath;
    QString filePath = Config::findFile(relative->doc().location(),
                                        imageFiles,
                                        imageDirs,
                                        fileBase,
                                        imgFileExts[format()],
                                        userFriendlyFilePath);

    if (filePath.isEmpty())
        return QString();

    QString path = Config::copyFile(relative->doc().location(),
                                    filePath,
                                    userFriendlyFilePath,
                                    outputDir() + QLatin1String("/images"));
    int images_slash = path.lastIndexOf("images/");
    QString relImagePath;
    if (images_slash != -1)
        relImagePath = path.mid(images_slash);
    return relImagePath;
}

QString Generator::indent(int level, const QString& markedCode)
{
    if (level == 0)
        return markedCode;

    QString t;
    int column = 0;

    int i = 0;
    while (i < (int) markedCode.length()) {
        if (markedCode.at(i) == QLatin1Char('\n')) {
            column = 0;
        }
        else {
            if (column == 0) {
                for (int j = 0; j < level; j++)
                    t += QLatin1Char(' ');
            }
            column++;
        }
        t += markedCode.at(i++);
    }
    return t;
}

void Generator::initialize(const Config &config)
{

    if (config.getBool(QString("HTML.nosubdirs")))
        resetUseOutputSubdirs();

    outputFormats = config.getOutputFormats();
    redirectDocumentationToDevNull_ = config.getBool(CONFIG_REDIRECTDOCUMENTATIONTODEVNULL);
    if (!outputFormats.isEmpty()) {
        outDir_ = config.getOutputDir();
        if (outDir_.isEmpty()) {
            config.lastLocation().fatal(tr("No output directory specified in "
                                           "configuration file or on the command line"));
        }
        else {
            outSubdir_ = outDir_.mid(outDir_.lastIndexOf('/') + 1);
        }

        QDir dirInfo;
        if (dirInfo.exists(outDir_)) {
            if (!runGenerateOnly() && Generator::useOutputSubdirs()) {
                if (!Config::removeDirContents(outDir_))
                    config.lastLocation().error(tr("Cannot empty output directory '%1'").arg(outDir_));
            }
        }
        else {
            if (!dirInfo.mkpath(outDir_))
                config.lastLocation().fatal(tr("Cannot create output directory '%1'").arg(outDir_));
        }

        if (!dirInfo.exists(outDir_ + "/images") && !dirInfo.mkdir(outDir_ + "/images"))
            config.lastLocation().fatal(tr("Cannot create images directory '%1'").arg(outDir_ + "/images"));
        if (!dirInfo.exists(outDir_ + "/images/used-in-examples") && !dirInfo.mkdir(outDir_ + "/images/used-in-examples"))
            config.lastLocation().fatal(tr("Cannot create images used in examples directory '%1'").arg(outDir_ + "/images/used-in-examples"));
        if (!dirInfo.exists(outDir_ + "/scripts") && !dirInfo.mkdir(outDir_ + "/scripts"))
            config.lastLocation().fatal(tr("Cannot create scripts directory '%1'").arg(outDir_ + "/scripts"));
        if (!dirInfo.exists(outDir_ + "/style") && !dirInfo.mkdir(outDir_ + "/style"))
            config.lastLocation().fatal(tr("Cannot create style directory '%1'").arg(outDir_ + "/style"));
    }

    imageFiles = config.getCleanPathList(CONFIG_IMAGES);
    imageDirs = config.getCleanPathList(CONFIG_IMAGEDIRS);
    scriptFiles = config.getCleanPathList(CONFIG_SCRIPTS);
    scriptDirs = config.getCleanPathList(CONFIG_SCRIPTDIRS);
    styleFiles = config.getCleanPathList(CONFIG_STYLES);
    styleDirs = config.getCleanPathList(CONFIG_STYLEDIRS);
    exampleDirs = config.getCleanPathList(CONFIG_EXAMPLEDIRS);
    exampleImgExts = config.getStringList(CONFIG_EXAMPLES + Config::dot + CONFIG_IMAGEEXTENSIONS);

    QString imagesDotFileExtensions = CONFIG_IMAGES + Config::dot + CONFIG_FILEEXTENSIONS;
    QSet<QString> formats = config.subVars(imagesDotFileExtensions);
    QSet<QString>::ConstIterator f = formats.constBegin();
    while (f != formats.constEnd()) {
        imgFileExts[*f] = config.getStringList(imagesDotFileExtensions + Config::dot + *f);
        ++f;
    }

    QList<Generator *>::ConstIterator g = generators.constBegin();
    while (g != generators.constEnd()) {
        if (outputFormats.contains((*g)->format())) {
            currentGenerator_ = (*g);
            (*g)->initializeGenerator(config);
            QStringList extraImages = config.getPathList((*g)->format() +
                                                         Config::dot +
                                                         CONFIG_EXTRAIMAGES);
            QStringList::ConstIterator e = extraImages.constBegin();
            while (e != extraImages.constEnd()) {
                QString filePath = *e;
                if (!filePath.isEmpty())
                    Config::copyFile(config.lastLocation(), filePath, filePath,
                                     (*g)->outputDir() + "/images");
                ++e;
            }

            // Documentation template handling
            QStringList scripts = config.getPathList((*g)->format()+Config::dot+CONFIG_SCRIPTS);
            e = scripts.constBegin();
            while (e != scripts.constEnd()) {
                QString filePath = *e;
                if (!filePath.isEmpty())
                    Config::copyFile(config.lastLocation(), filePath, filePath,
                                     (*g)->outputDir() + "/scripts");
                ++e;
            }

            QStringList styles = config.getPathList((*g)->format()+Config::dot+CONFIG_STYLESHEETS);
            e = styles.constBegin();
            while (e != styles.constEnd()) {
                QString filePath = *e;
                if (!filePath.isEmpty())
                    Config::copyFile(config.lastLocation(), filePath, filePath,
                                     (*g)->outputDir() + "/style");
                ++e;
            }
        }
        ++g;
    }

    QRegExp secondParamAndAbove("[\2-\7]");
    QSet<QString> formattingNames = config.subVars(CONFIG_FORMATTING);
    QSet<QString>::ConstIterator n = formattingNames.constBegin();
    while (n != formattingNames.constEnd()) {
        QString formattingDotName = CONFIG_FORMATTING + Config::dot + *n;
        QSet<QString> formats = config.subVars(formattingDotName);
        QSet<QString>::ConstIterator f = formats.constBegin();
        while (f != formats.constEnd()) {
            QString def = config.getString(formattingDotName + Config::dot + *f);
            if (!def.isEmpty()) {
                int numParams = Config::numParams(def);
                int numOccs = def.count("\1");
                if (numParams != 1) {
                    config.lastLocation().warning(tr("Formatting '%1' must "
                                                     "have exactly one "
                                                     "parameter (found %2)")
                                                  .arg(*n).arg(numParams));
                }
                else if (numOccs > 1) {
                    config.lastLocation().fatal(tr("Formatting '%1' must "
                                                   "contain exactly one "
                                                   "occurrence of '\\1' "
                                                   "(found %2)")
                                                .arg(*n).arg(numOccs));
                }
                else {
                    int paramPos = def.indexOf("\1");
                    fmtLeftMaps[*f].insert(*n, def.left(paramPos));
                    fmtRightMaps[*f].insert(*n, def.mid(paramPos + 1));
                }
            }
            ++f;
        }
        ++n;
    }

    project = config.getString(CONFIG_PROJECT);

    QStringList prefixes = config.getStringList(CONFIG_OUTPUTPREFIXES);
    if (!prefixes.isEmpty()) {
        foreach (const QString &prefix, prefixes)
            outputPrefixes[prefix] = config.getString(CONFIG_OUTPUTPREFIXES + Config::dot + prefix);
    }
    else
        outputPrefixes[QLatin1String("QML")] = QLatin1String("qml-");
    noLinkErrors_ = config.getBool(CONFIG_NOLINKERRORS);
}

/*!
  Appends each directory path in \a moreImageDirs to the
  list of image directories.
 */
void Generator::augmentImageDirs(QSet<QString>& moreImageDirs)
{
    if (moreImageDirs.isEmpty())
        return;
    QSet<QString>::const_iterator i = moreImageDirs.begin();
    while (i != moreImageDirs.end()) {
        imageDirs.append(*i);
        ++i;
    }
}

/*!
  Sets the generator's pointer to the Config instance.
 */
void Generator::initializeGenerator(const Config& config)
{
    config_ = &config;
    showInternal_ = config.getBool(CONFIG_SHOWINTERNAL);
}

bool Generator::matchAhead(const Atom *atom, Atom::Type expectedAtomType)
{
    return atom->next() != 0 && atom->next()->type() == expectedAtomType;
}

/*!
  Used for writing to the current output stream. Returns a
  reference to the current output stream, which is then used
  with the \c {<<} operator for writing.
 */
QTextStream &Generator::out()
{
    return *outStreamStack.top();
}

QString Generator::outFileName()
{
    return QFileInfo(static_cast<QFile*>(out().device())->fileName()).fileName();
}

QString Generator::outputPrefix(const QString &nodeType)
{
    return outputPrefixes[nodeType];
}

bool Generator::parseArg(const QString& src,
                             const QString& tag,
                             int* pos,
                             int n,
                             QStringRef* contents,
                             QStringRef* par1,
                             bool debug)
{
#define SKIP_CHAR(c) \
    if (debug) \
    qDebug() << "looking for " << c << " at " << QString(src.data() + i, n - i); \
    if (i >= n || src[i] != c) { \
    if (debug) \
    qDebug() << " char '" << c << "' not found"; \
    return false; \
} \
    ++i;


#define SKIP_SPACE \
    while (i < n && src[i] == ' ') \
    ++i;

    int i = *pos;
    int j = i;

    // assume "<@" has been parsed outside
    //SKIP_CHAR('<');
    //SKIP_CHAR('@');

    if (tag != QStringRef(&src, i, tag.length())) {
        if (0 && debug)
            qDebug() << "tag " << tag << " not found at " << i;
        return false;
    }

    if (debug)
        qDebug() << "haystack:" << src << "needle:" << tag << "i:" <<i;

    // skip tag
    i += tag.length();

    // parse stuff like:  linkTag("(<@link node=\"([^\"]+)\">).*(</@link>)");
    if (par1) {
        SKIP_SPACE;
        // read parameter name
        j = i;
        while (i < n && src[i].isLetter())
            ++i;
        if (src[i] == '=') {
            if (debug)
                qDebug() << "read parameter" << QString(src.data() + j, i - j);
            SKIP_CHAR('=');
            SKIP_CHAR('"');
            // skip parameter name
            j = i;
            while (i < n && src[i] != '"')
                ++i;
            *par1 = QStringRef(&src, j, i - j);
            SKIP_CHAR('"');
            SKIP_SPACE;
        } else {
            if (debug)
                qDebug() << "no optional parameter found";
        }
    }
    SKIP_SPACE;
    SKIP_CHAR('>');

    // find contents up to closing "</@tag>
    j = i;
    for (; true; ++i) {
        if (i + 4 + tag.length() > n)
            return false;
        if (src[i] != '<')
            continue;
        if (src[i + 1] != '/')
            continue;
        if (src[i + 2] != '@')
            continue;
        if (tag != QStringRef(&src, i + 3, tag.length()))
            continue;
        if (src[i + 3 + tag.length()] != '>')
            continue;
        break;
    }

    *contents = QStringRef(&src, j, i - j);

    i += tag.length() + 4;

    *pos = i;
    if (debug)
        qDebug() << " tag " << tag << " found: pos now: " << i;
    return true;
#undef SKIP_CHAR
}

QString Generator::plainCode(const QString& markedCode)
{
    QString t = markedCode;
    t.replace(tag, QString());
    t.replace(quot, QLatin1String("\""));
    t.replace(gt, QLatin1String(">"));
    t.replace(lt, QLatin1String("<"));
    t.replace(amp, QLatin1String("&"));
    return t;
}

void Generator::setImageFileExtensions(const QStringList& extensions)
{
    imgFileExts[format()] = extensions;
}

void Generator::singularPlural(Text& text, const NodeList& nodes)
{
    if (nodes.count() == 1)
        text << " is";
    else
        text << " are";
}

int Generator::skipAtoms(const Atom *atom, Atom::Type type) const
{
    int skipAhead = 0;
    atom = atom->next();
    while (atom != 0 && atom->type() != type) {
        skipAhead++;
        atom = atom->next();
    }
    return skipAhead;
}

/*!
  Resets the variables used during text output.
 */
void Generator::initializeTextOutput()
{
    inLink_ = false;
    inContents_ = false;
    inSectionHeading_ = false;
    inTableHeader_ = false;
    numTableRows_ = 0;
    threeColumnEnumValueTable_ = true;
    link_.clear();
    sectionNumber_.clear();
}

void Generator::supplementAlsoList(const Node *node, QList<Text> &alsoList)
{
    if (node->type() == Node::Function) {
        const FunctionNode *func = static_cast<const FunctionNode *>(node);
        if (func->overloadNumber() == 1) {
            QString alternateName;
            const FunctionNode *alternateFunc = 0;

            if (func->name().startsWith("set") && func->name().size() >= 4) {
                alternateName = func->name()[3].toLower();
                alternateName += func->name().mid(4);
                alternateFunc = func->parent()->findFunctionNode(alternateName);

                if (!alternateFunc) {
                    alternateName = "is" + func->name().mid(3);
                    alternateFunc = func->parent()->findFunctionNode(alternateName);
                    if (!alternateFunc) {
                        alternateName = "has" + func->name().mid(3);
                        alternateFunc = func->parent()->findFunctionNode(alternateName);
                    }
                }
            }
            else if (!func->name().isEmpty()) {
                alternateName = "set";
                alternateName += func->name()[0].toUpper();
                alternateName += func->name().mid(1);
                alternateFunc = func->parent()->findFunctionNode(alternateName);
            }

            if (alternateFunc && alternateFunc->access() != Node::Private) {
                int i;
                for (i = 0; i < alsoList.size(); ++i) {
                    if (alsoList.at(i).toString().contains(alternateName))
                        break;
                }

                if (i == alsoList.size()) {
                    alternateName += "()";

                    Text also;
                    also << Atom(Atom::Link, alternateName)
                         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                         << alternateName
                         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                    alsoList.prepend(also);
                }
            }
        }
    }
}

void Generator::terminate()
{
    QList<Generator *>::ConstIterator g = generators.constBegin();
    while (g != generators.constEnd()) {
        if (outputFormats.contains((*g)->format()))
            (*g)->terminateGenerator();
        ++g;
    }

    fmtLeftMaps.clear();
    fmtRightMaps.clear();
    imgFileExts.clear();
    imageFiles.clear();
    imageDirs.clear();
    outDir_.clear();
    QmlClassNode::terminate();
    ExampleNode::terminate();
}

void Generator::terminateGenerator()
{
}

/*!
  Trims trailing whitespace off the \a string and returns
  the trimmed string.
 */
QString Generator::trimmedTrailing(const QString& string)
{
    QString trimmed = string;
    while (trimmed.length() > 0 && trimmed[trimmed.length() - 1].isSpace())
        trimmed.truncate(trimmed.length() - 1);
    return trimmed;
}

QString Generator::typeString(const Node *node)
{
    switch (node->type()) {
    case Node::Namespace:
        return "namespace";
    case Node::Class:
        return "class";
    case Node::Document:
    {
        switch (node->subType()) {
        case Node::QmlClass:
            return "type";
        case Node::QmlBasicType:
            return "type";
        default:
            return "documentation";
        }
    }
    case Node::Enum:
        return "enum";
    case Node::Typedef:
        return "typedef";
    case Node::Function:
        return "function";
    case Node::Property:
        return "property";
    case Node::QmlPropertyGroup:
        return "property group";
    case Node::QmlProperty:
        return "QML property";
    case Node::QmlSignal:
        return "QML signal";
    case Node::QmlSignalHandler:
        return "QML signal handler";
    case Node::QmlMethod:
        return "QML method";
    default:
        return "documentation";
    }
}

void Generator::unknownAtom(const Atom *atom)
{
    Location::internalError(tr("unknown atom type '%1' in %2 generator")
                            .arg(atom->typeString()).arg(format()));
}

QT_END_NAMESPACE

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

#include <qobjectdefs.h>
#include "codemarker.h"
#include "config.h"
#include "node.h"
#include <qdebug.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

QString CodeMarker::defaultLang;
QList<CodeMarker *> CodeMarker::markers;

/*!
  When a code marker constructs itself, it puts itself into
  the static list of code markers. All the code markers in
  the static list get initialized in initialize(), which is
  not called until after the qdoc configuration file has
  been read.
 */
CodeMarker::CodeMarker()
{
    markers.prepend(this);
}

/*!
  When a code marker destroys itself, it removes itself from
  the static list of code markers.
 */
CodeMarker::~CodeMarker()
{
    markers.removeAll(this);
}

/*!
  A code market performs no initialization by default. Marker-specific
  initialization is performed in subclasses.
 */
void CodeMarker::initializeMarker(const Config& ) // config
{
}

/*!
  Terminating a code marker is trivial.
 */
void CodeMarker::terminateMarker()
{
    // nothing.
}

/*!
  All the code markers in the static list are initialized
  here, after the qdoc configuration file has been loaded.
 */
void CodeMarker::initialize(const Config& config)
{
    defaultLang = config.getString(CONFIG_LANGUAGE);
    QList<CodeMarker *>::ConstIterator m = markers.constBegin();
    while (m != markers.constEnd()) {
        (*m)->initializeMarker(config);
        ++m;
    }
}

/*!
  All the code markers in the static list are terminated here.
 */
void CodeMarker::terminate()
{
    QList<CodeMarker *>::ConstIterator m = markers.constBegin();
    while (m != markers.constEnd()) {
        (*m)->terminateMarker();
        ++m;
    }
}

CodeMarker *CodeMarker::markerForCode(const QString& code)
{
    CodeMarker *defaultMarker = markerForLanguage(defaultLang);
    if (defaultMarker != 0 && defaultMarker->recognizeCode(code))
        return defaultMarker;

    QList<CodeMarker *>::ConstIterator m = markers.constBegin();
    while (m != markers.constEnd()) {
        if ((*m)->recognizeCode(code))
            return *m;
        ++m;
    }
    return defaultMarker;
}

CodeMarker *CodeMarker::markerForFileName(const QString& fileName)
{
    CodeMarker *defaultMarker = markerForLanguage(defaultLang);
    int dot = -1;
    while ((dot = fileName.lastIndexOf(QLatin1Char('.'), dot)) != -1) {
        QString ext = fileName.mid(dot + 1);
        if (defaultMarker != 0 && defaultMarker->recognizeExtension(ext))
            return defaultMarker;
        QList<CodeMarker *>::ConstIterator m = markers.constBegin();
        while (m != markers.constEnd()) {
            if ((*m)->recognizeExtension(ext))
                return *m;
            ++m;
        }
        --dot;
    }
    return defaultMarker;
}

CodeMarker *CodeMarker::markerForLanguage(const QString& lang)
{
    QList<CodeMarker *>::ConstIterator m = markers.constBegin();
    while (m != markers.constEnd()) {
        if ((*m)->recognizeLanguage(lang))
            return *m;
        ++m;
    }
    return 0;
}

const Node *CodeMarker::nodeForString(const QString& string)
{
    if (sizeof(const Node *) == sizeof(uint)) {
        return reinterpret_cast<const Node *>(string.toUInt());
    }
    else {
        return reinterpret_cast<const Node *>(string.toULongLong());
    }
}

QString CodeMarker::stringForNode(const Node *node)
{
    if (sizeof(const Node *) == sizeof(ulong)) {
        return QString::number(reinterpret_cast<quintptr>(node));
    }
    else {
        return QString::number(reinterpret_cast<qulonglong>(node));
    }
}

static const QString samp  = QLatin1String("&amp;");
static const QString slt   = QLatin1String("&lt;");
static const QString sgt   = QLatin1String("&gt;");
static const QString squot = QLatin1String("&quot;");

QString CodeMarker::protect(const QString& str)
{
    int n = str.length();
    QString marked;
    marked.reserve(n * 2 + 30);
    const QChar *data = str.constData();
    for (int i = 0; i != n; ++i) {
        switch (data[i].unicode()) {
        case '&': marked += samp;  break;
        case '<': marked += slt;   break;
        case '>': marked += sgt;   break;
        case '"': marked += squot; break;
        default : marked += data[i];
        }
    }
    return marked;
}

QString CodeMarker::typified(const QString &string)
{
    QString result;
    QString pendingWord;

    for (int i = 0; i <= string.size(); ++i) {
        QChar ch;
        if (i != string.size())
            ch = string.at(i);

        QChar lower = ch.toLower();
        if ((lower >= QLatin1Char('a') && lower <= QLatin1Char('z'))
                || ch.digitValue() >= 0 || ch == QLatin1Char('_')
                || ch == QLatin1Char(':')) {
            pendingWord += ch;
        }
        else {
            if (!pendingWord.isEmpty()) {
                bool isProbablyType = (pendingWord != QLatin1String("const"));
                if (isProbablyType)
                    result += QLatin1String("<@type>");
                result += pendingWord;
                if (isProbablyType)
                    result += QLatin1String("</@type>");
            }
            pendingWord.clear();

            switch (ch.unicode()) {
            case '\0':
                break;
            case '&':
                result += QLatin1String("&amp;");
                break;
            case '<':
                result += QLatin1String("&lt;");
                break;
            case '>':
                result += QLatin1String("&gt;");
                break;
            default:
                result += ch;
            }
        }
    }
    return result;
}

QString CodeMarker::taggedNode(const Node* node)
{
    QString tag;
    QString name = node->name();

    switch (node->type()) {
    case Node::Namespace:
        tag = QLatin1String("@namespace");
        break;
    case Node::Class:
        tag = QLatin1String("@class");
        break;
    case Node::Enum:
        tag = QLatin1String("@enum");
        break;
    case Node::Typedef:
        tag = QLatin1String("@typedef");
        break;
    case Node::Function:
        tag = QLatin1String("@function");
        break;
    case Node::Property:
        tag = QLatin1String("@property");
        break;
    case Node::Document:
        /*
          Remove the "QML:" prefix, if present.
          There shouldn't be any of these "QML:"
          prefixes in the documentation sources
          after the switch to using QML module
          qualifiers, but this code is kept to
          be backward compatible.
        */
        if (node->subType() == Node::QmlClass) {
            if (node->name().startsWith(QLatin1String("QML:")))
                name = name.mid(4);
        }
        tag = QLatin1String("@property");
        break;
    case Node::QmlMethod:
    case Node::QmlSignal:
    case Node::QmlSignalHandler:
        tag = QLatin1String("@function");
        break;
    default:
        tag = QLatin1String("@unknown");
        break;
    }
    return (QLatin1Char('<') + tag + QLatin1Char('>') + protect(name)
            + QLatin1String("</") + tag + QLatin1Char('>'));
}

QString CodeMarker::taggedQmlNode(const Node* node)
{
    QString tag;
    switch (node->type()) {
    case Node::QmlProperty:
        tag = QLatin1String("@property");
        break;
    case Node::QmlSignal:
        tag = QLatin1String("@signal");
        break;
    case Node::QmlSignalHandler:
        tag = QLatin1String("@signalhandler");
        break;
    case Node::QmlMethod:
        tag = QLatin1String("@method");
        break;
    default:
        tag = QLatin1String("@unknown");
        break;
    }
    return QLatin1Char('<') + tag + QLatin1Char('>') + protect(node->name())
            + QLatin1String("</") + tag + QLatin1Char('>');
}

QString CodeMarker::linkTag(const Node *node, const QString& body)
{
    return QLatin1String("<@link node=\"") + stringForNode(node)
            + QLatin1String("\">") + body + QLatin1String("</@link>");
}

QString CodeMarker::sortName(const Node *node, const QString* name)
{
    QString nodeName;
    if (name != 0)
        nodeName = *name;
    else
        nodeName = node->name();
    int numDigits = 0;
    for (int i = nodeName.size() - 1; i > 0; --i) {
        if (nodeName.at(i).digitValue() == -1)
            break;
        ++numDigits;
    }

    // we want 'qint8' to appear before 'qint16'
    if (numDigits > 0) {
        for (int i = 0; i < 4 - numDigits; ++i)
            nodeName.insert(nodeName.size()-numDigits-1, QLatin1Char('0'));
    }

    if (node->type() == Node::Function) {
        const FunctionNode *func = static_cast<const FunctionNode *>(node);
        QString sortNo;
        if (func->metaness() == FunctionNode::Ctor) {
            sortNo = QLatin1String("C");
        }
        else if (func->metaness() == FunctionNode::Dtor) {
            sortNo = QLatin1String("D");
        }
        else {
            if (nodeName.startsWith(QLatin1String("operator"))
                    && nodeName.length() > 8
                    && !nodeName[8].isLetterOrNumber())
                sortNo = QLatin1String("F");
            else
                sortNo = QLatin1String("E");
        }
        return sortNo + nodeName + QLatin1Char(' ')
                + QString::number(func->overloadNumber(), 36);
    }

    if (node->type() == Node::Class)
        return QLatin1Char('A') + nodeName;

    if (node->type() == Node::Property || node->type() == Node::Variable)
        return QLatin1Char('E') + nodeName;

    if ((node->type() == Node::QmlMethod) ||
        (node->type() == Node::QmlSignal) ||
        (node->type() == Node::QmlSignalHandler)) {
        //const FunctionNode* func = static_cast<const FunctionNode *>(node);
        //return QLatin1Char('E') + func->name();
        return QLatin1Char('E') + nodeName;
    }

    return QLatin1Char('B') + nodeName;
}

void CodeMarker::insert(FastSection &fastSection,
                        Node *node,
                        SynopsisStyle style,
                        Status status)
{
    bool irrelevant = false;
    bool inheritedMember = false;
    if (!node->relates()) {
        InnerNode* p = node->parent();
        if (p->type() == Node::QmlPropertyGroup)
            p = p->parent();
        if (p != fastSection.parent_) { // && !node->parent()->isAbstract()) {
            if (p->subType() != Node::QmlClass || !p->isAbstract()) {
                //if (node->type() != Node::QmlProperty) {
                inheritedMember = true;
            }
        }
    }

    if (node->access() == Node::Private) {
        irrelevant = true;
    }
    else if (node->type() == Node::Function) {
        FunctionNode *func = (FunctionNode *) node;
        irrelevant = (inheritedMember
                      && (func->metaness() == FunctionNode::Ctor ||
                          func->metaness() == FunctionNode::Dtor));
    }
    else if (node->type() == Node::Class || node->type() == Node::Enum
             || node->type() == Node::Typedef) {
        irrelevant = (inheritedMember && style != Subpage);
        if (!irrelevant && style == Detailed && node->type() == Node::Typedef) {
            const TypedefNode* typedeffe = static_cast<const TypedefNode*>(node);
            if (typedeffe->associatedEnum())
                irrelevant = true;
        }
    }

    if (!irrelevant) {
        if (status == Compat) {
            irrelevant = (node->status() != Node::Compat);
        }
        else if (status == Obsolete) {
            irrelevant = (node->status() != Node::Obsolete);
        }
        else {
            irrelevant = (node->status() == Node::Compat ||
                          node->status() == Node::Obsolete);
        }
    }

    if (!irrelevant) {
        if (!inheritedMember || style == Subpage) {
            QString key = sortName(node);
            fastSection.memberMap.insertMulti(key, node);
        }
        else {
            if (node->parent()->type() == Node::Class) {
                if (fastSection.inherited.isEmpty()
                        || fastSection.inherited.last().first != node->parent()) {
                    QPair<InnerNode *, int> p(node->parent(), 0);
                    fastSection.inherited.append(p);
                }
                fastSection.inherited.last().second++;
            }
        }
    }
}

/*!
  Returns \c true if \a node represents a reimplemented member
  function in the class of the FastSection \a fs. If it is
  a reimplemented function, then it is inserted into the
  reimplemented member map in \a fs. The test is performed
  only if \a status is \e OK. True is returned if \a node
  is inserted into the map. Otherwise, false is returned.
 */
bool CodeMarker::insertReimpFunc(FastSection& fs, Node* node, Status status)
{
    if ((node->access() != Node::Private) && (node->relates() == 0)) {
        const FunctionNode* fn = static_cast<const FunctionNode*>(node);
        if ((fn->reimplementedFrom() != 0) && (status == Okay)) {
            if (fn->parent() == fs.parent_) {
                QString key = sortName(fn);
                if (!fs.reimpMemberMap.contains(key)) {
                    fs.reimpMemberMap.insert(key,node);
                    return true;
                }
            }
        }
    }
    return false;
}

/*!
  If \a fs is not empty, convert it to a Section and append
  the new Section to \a sectionList.
 */
void CodeMarker::append(QList<Section>& sectionList, const FastSection& fs, bool includeKeys)
{
    if (!fs.isEmpty()) {
        if (fs.classMapList_.isEmpty()) {
            Section section(fs.name,fs.divClass,fs.singularMember,fs.pluralMember);
            if (includeKeys) {
                section.keys = fs.memberMap.keys();
            }
            section.members = fs.memberMap.values();
            section.reimpMembers = fs.reimpMemberMap.values();
            section.inherited = fs.inherited;
            sectionList.append(section);
        }
        else {
            Section section(fs.name,fs.divClass,fs.singularMember,fs.pluralMember);
            sectionList.append(section);
            Section* s = &sectionList[sectionList.size()-1];
            for (int i=0; i<fs.classMapList_.size(); i++) {
                ClassMap* classMap = fs.classMapList_[i];
                ClassKeysNodes* ckn = new ClassKeysNodes;
                ckn->first = classMap->first;
                ckn->second.second = classMap->second.values();
                ckn->second.first = classMap->second.keys();
                s->classKeysNodesList_.append(ckn);
             }
        }
    }
}

/*!
  The destructor must delete each member of the
  list of QML class lists, if it is not empty;
 */
Section::~Section()
{
    if (!classKeysNodesList_.isEmpty()) {
        for (int i=0; i<classKeysNodesList_.size(); i++) {
            ClassKeysNodes* classKeysNodes = classKeysNodesList_[i];
            classKeysNodesList_[i] = 0;
            delete classKeysNodes;
        }
    }
}

/*!
  The destructor must delete the QML class maps in the class
  map list, if the class map list is not empty.
 */
FastSection::~FastSection()
{
    if (!classMapList_.isEmpty()) {
        for (int i=0; i<classMapList_.size(); i++) {
            ClassMap* classMap = classMapList_[i];
            classMapList_[i] = 0;
            delete classMap;
        }
    }
}

static QString encode(const QString &string)
{
    return string;
}

QStringList CodeMarker::macRefsForNode(Node *node)
{
    QString result = QLatin1String("cpp/");
    switch (node->type()) {
    case Node::Class:
    {
        const ClassNode *classe = static_cast<const ClassNode *>(node);
        {
            result += QLatin1String("cl/");
        }
        result += macName(classe); // ### Maybe plainName?
    }
        break;
    case Node::Enum:
    {
        QStringList stringList;
        stringList << encode(result + QLatin1String("tag/") +
                             macName(node));
        foreach (const QString &enumName, node->doc().enumItemNames()) {
            // ### Write a plainEnumValue() and use it here
            stringList << encode(result + QLatin1String("econst/") +
                                 macName(node->parent(), enumName));
        }
        return stringList;
    }
    case Node::Typedef:
        result += QLatin1String("tdef/") + macName(node);
        break;
    case Node::Function:
    {
        bool isMacro = false;
        Q_UNUSED(isMacro)
        const FunctionNode *func = static_cast<const FunctionNode *>(node);

        // overloads are too clever for the Xcode documentation browser
        if (func->isOverload())
            return QStringList();

        if (func->metaness() == FunctionNode::MacroWithParams
                || func->metaness() == FunctionNode::MacroWithoutParams) {
            result += QLatin1String("macro/");
        }
        else if (func->isStatic()) {
            result += QLatin1String("clm/");
        }
        else if (!func->parent()->name().isEmpty()) {
            result += QLatin1String("instm/");
        }
        else {
            result += QLatin1String("func/");
        }

        result += macName(func);
        if (result.endsWith(QLatin1String("()")))
            result.chop(2);
    }
        break;
    case Node::Variable:
        result += QLatin1String("data/") + macName(node);
        break;
    case Node::Property:
    {
        NodeList list = static_cast<const PropertyNode*>(node)->functions();
        QStringList stringList;
        foreach (Node* node, list) {
            stringList += macRefsForNode(node);
        }
        return stringList;
    }
    case Node::Namespace:
    case Node::Document:
    default:
        return QStringList();
    }

    return QStringList(encode(result));
}

QString CodeMarker::macName(const Node *node, const QString &name)
{
    QString myName = name;
    if (myName.isEmpty()) {
        myName = node->name();
        node = node->parent();
    }

    if (node->name().isEmpty()) {
        return QLatin1Char('/') + protect(myName);
    }
    else {
        return node->plainFullName() + QLatin1Char('/') + protect(myName);
    }
}

/*!
  Returns an empty list of documentation sections.
 */
QList<Section> CodeMarker::qmlSections(const QmlClassNode* , SynopsisStyle )
{
    return QList<Section>();
}

QT_END_NAMESPACE

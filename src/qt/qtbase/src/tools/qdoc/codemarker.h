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
  codemarker.h
*/

#ifndef CODEMARKER_H
#define CODEMARKER_H

#include <qpair.h>

#include "atom.h"
#include "node.h"

QT_BEGIN_NAMESPACE

class Config;
class Tree;

typedef QMultiMap<QString, Node*> MemberMap; // the string is the member signature
typedef QPair<const QmlClassNode*, MemberMap> ClassMap;    // the node is the QML type
typedef QList<ClassMap*> ClassMapList;

typedef QPair<QStringList, NodeList> KeysAndNodes;
typedef QPair<const QmlClassNode*, KeysAndNodes> ClassKeysNodes;
typedef QList<ClassKeysNodes*> ClassKeysNodesList;

struct Section
{
    QString name;
    QString divClass;
    QString singularMember;
    QString pluralMember;
    QStringList keys;
    NodeList members;
    NodeList reimpMembers;
    QList<QPair<InnerNode *, int> > inherited;
    ClassKeysNodesList classKeysNodesList_;

    Section() { }
    Section(const QString& name0,
            const QString& divClass0,
            const QString& singularMember0,
            const QString& pluralMember0)
        : name(name0),
          divClass(divClass0),
          singularMember(singularMember0),
          pluralMember(pluralMember0) { }
    ~Section();
    void appendMember(Node* node) { members.append(node); }
    void appendReimpMember(Node* node) { reimpMembers.append(node); }
};

struct FastSection
{
    const InnerNode *parent_;
    QString name;
    QString divClass;
    QString singularMember;
    QString pluralMember;
    QMultiMap<QString, Node *> memberMap;
    QMultiMap<QString, Node *> reimpMemberMap;
    ClassMapList classMapList_;
    QList<QPair<InnerNode *, int> > inherited;

    FastSection(const InnerNode *parent,
                const QString& name0,
                const QString& divClass0,
                const QString& singularMember0,
                const QString& pluralMember0)
        : parent_(parent),
          name(name0),
          divClass(divClass0),
          singularMember(singularMember0),
          pluralMember(pluralMember0) { }
    ~FastSection();
    bool isEmpty() const {
        return (memberMap.isEmpty() &&
                inherited.isEmpty() &&
                reimpMemberMap.isEmpty() &&
                classMapList_.isEmpty());
    }

};

class CodeMarker
{
public:
    enum SynopsisStyle { Summary, Detailed, Subpage, Accessors };
    enum Status { Compat, Obsolete, Okay };

    CodeMarker();
    virtual ~CodeMarker();

    virtual void initializeMarker(const Config& config);
    virtual void terminateMarker();
    virtual bool recognizeCode(const QString& code) = 0;
    virtual bool recognizeExtension(const QString& ext) = 0;
    virtual bool recognizeLanguage(const QString& lang) = 0;
    virtual Atom::Type atomType() const = 0;
    virtual QString markedUpCode(const QString& code,
                                 const Node *relative,
                                 const Location &location) = 0;
    virtual QString markedUpSynopsis(const Node *node,
                                     const Node *relative,
                                     SynopsisStyle style) = 0;
    virtual QString markedUpQmlItem(const Node* , bool) { return QString(); }
    virtual QString markedUpName(const Node *node) = 0;
    virtual QString markedUpFullName(const Node *node,
                                     const Node *relative = 0) = 0;
    virtual QString markedUpEnumValue(const QString &enumValue,
                                      const Node *relative) = 0;
    virtual QString markedUpIncludes(const QStringList& includes) = 0;
    virtual QString functionBeginRegExp(const QString& funcName) = 0;
    virtual QString functionEndRegExp(const QString& funcName) = 0;
    virtual QList<Section> sections(const InnerNode *inner,
                                    SynopsisStyle style,
                                    Status status) = 0;
    virtual QList<Section> qmlSections(const QmlClassNode* qmlClassNode, SynopsisStyle style);
    virtual QStringList macRefsForNode(Node* node);

    static void initialize(const Config& config);
    static void terminate();
    static CodeMarker *markerForCode(const QString& code);
    static CodeMarker *markerForFileName(const QString& fileName);
    static CodeMarker *markerForLanguage(const QString& lang);
    static const Node *nodeForString(const QString& string);
    static QString stringForNode(const Node *node);

    QString typified(const QString &string);

protected:
    virtual QString sortName(const Node *node, const QString* name = 0);
    QString protect(const QString &string);
    QString taggedNode(const Node* node);
    QString taggedQmlNode(const Node* node);
    QString linkTag(const Node *node, const QString& body);
    void insert(FastSection &fastSection,
                Node *node,
                SynopsisStyle style,
                Status status);
    bool insertReimpFunc(FastSection& fs, Node* node, Status status);
    void append(QList<Section>& sectionList, const FastSection& fastSection, bool includeKeys = false);

private:
    QString macName(const Node *parent, const QString &name = QString());

    static QString defaultLang;
    static QList<CodeMarker *> markers;
};

QT_END_NAMESPACE

#endif

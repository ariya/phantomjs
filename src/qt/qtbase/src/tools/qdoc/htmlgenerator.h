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
  htmlgenerator.h
*/

#ifndef HTMLGENERATOR_H
#define HTMLGENERATOR_H

#include <qmap.h>
#include <qregexp.h>
#include <qxmlstream.h>
#include "codemarker.h"
#include "config.h"
#include "generator.h"

QT_BEGIN_NAMESPACE

class HelpProjectWriter;

class HtmlGenerator : public Generator
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::HtmlGenerator)

public:
    enum SinceType {
        Namespace,
        Class,
        MemberFunction,
        NamespaceFunction,
        GlobalFunction,
        Macro,
        Enum,
        Typedef,
        Property,
        Variable,
        QmlClass,
        QmlProperty,
        QmlSignal,
        QmlSignalHandler,
        QmlMethod,
        LastSinceType
    };

public:
    HtmlGenerator();
    ~HtmlGenerator();

    virtual void initializeGenerator(const Config& config);
    virtual void terminateGenerator();
    virtual QString format();
    virtual void generateTree();
    void generateCollisionPages();
    void generateManifestFiles();

    QString protectEnc(const QString &string);
    static QString protect(const QString &string, const QString &encoding = "ISO-8859-1");
    static QString cleanRef(const QString& ref);
    static QString sinceTitle(int i) { return sinceTitles[i]; }

protected:
    virtual int generateAtom(const Atom *atom,
                             const Node *relative,
                             CodeMarker *marker);
    virtual void generateClassLikeNode(InnerNode* inner, CodeMarker* marker);
    virtual void generateDocNode(DocNode* dn, CodeMarker* marker);
    virtual QString fileExtension() const;
    virtual QString refForNode(const Node *node);
    virtual QString linkForNode(const Node *node, const Node *relative);

    void generateManifestFile(QString manifest, QString element);
    void readManifestMetaContent(const Config &config);

private:
    enum SubTitleSize { SmallSubTitle, LargeSubTitle };
    enum ExtractionMarkType {
        BriefMark,
        DetailedDescriptionMark,
        MemberMark,
        EndMark
    };

    struct ManifestMetaFilter
    {
        QSet<QString> names;
        QSet<QString> attributes;
        QSet<QString> tags;
    };

    const QPair<QString,QString> anchorForNode(const Node *node);
    void generateNavigationBar(const QString& title,
                             const Node *node,
                             CodeMarker *marker);
    void generateHeader(const QString& title,
                        const Node *node = 0,
                        CodeMarker *marker = 0);
    void generateTitle(const QString& title,
                       const Text &subTitle,
                       SubTitleSize subTitleSize,
                       const Node *relative,
                       CodeMarker *marker);
    void generateFooter(const Node *node = 0);
    void generateRequisites(InnerNode *inner,
                            CodeMarker *marker);
    void generateQmlRequisites(QmlClassNode *qcn,
                            CodeMarker *marker);
    void generateBrief(const Node *node,
                       CodeMarker *marker,
                       const Node *relative = 0);
    void generateIncludes(const InnerNode *inner, CodeMarker *marker);
    void generateTableOfContents(const Node *node,
                                 CodeMarker *marker,
                                 QList<Section>* sections = 0);
    QString generateListOfAllMemberFile(const InnerNode *inner,
                                        CodeMarker *marker);
    QString generateAllQmlMembersFile(const QmlClassNode* qml_cn,
                                      CodeMarker* marker);
    QString generateLowStatusMemberFile(InnerNode *inner,
                                        CodeMarker *marker,
                                        CodeMarker::Status status);
    void generateClassHierarchy(const Node *relative, NodeMap &classMap);
    void generateAnnotatedList(const Node* relative, CodeMarker* marker, const NodeMap& nodeMap);
    void generateAnnotatedList(const Node* relative, CodeMarker* marker, const NodeList& nodes);
    void generateCompactList(ListType listType,
                             const Node *relative,
                             const NodeMap &classMap,
                             bool includeAlphabet,
                             QString commonPrefix);
    void generateFunctionIndex(const Node *relative);
    void generateLegaleseList(const Node *relative, CodeMarker *marker);
    void generateOverviewList(const Node *relative);
    void generateSectionList(const Section& section,
                             const Node *relative,
                             CodeMarker *marker,
                             CodeMarker::SynopsisStyle style);
    void generateQmlSummary(const Section& section,
                            const Node *relative,
                            CodeMarker *marker);
    void generateQmlItem(const Node *node,
                         const Node *relative,
                         CodeMarker *marker,
                         bool summary);
    void generateDetailedQmlMember(Node *node,
                                   const InnerNode *relative,
                                   CodeMarker *marker);
    void generateQmlInherits(const QmlClassNode* qcn, CodeMarker* marker);
    void generateQmlInstantiates(QmlClassNode* qcn, CodeMarker* marker);
    void generateInstantiatedBy(ClassNode* cn, CodeMarker* marker);

    void generateRequisitesTable(const QStringList& requisitesOrder, QMap<QString, Text>& requisites);
    void generateSection(const NodeList& nl,
                         const Node *relative,
                         CodeMarker *marker,
                         CodeMarker::SynopsisStyle style);
    void generateSynopsis(const Node *node,
                          const Node *relative,
                          CodeMarker *marker,
                          CodeMarker::SynopsisStyle style,
                          bool alignNames = false,
                          const QString* prefix = 0);
    void generateSectionInheritedList(const Section& section, const Node *relative);
    QString highlightedCode(const QString& markedCode,
                            const Node* relative,
                            bool alignNames = false,
                            const Node* self = 0);

    void generateFullName(const Node *apparentNode, const Node *relative, const Node *actualNode = 0);
    void generateDetailedMember(const Node *node,
                                const InnerNode *relative,
                                CodeMarker *marker);
    void generateLink(const Atom *atom, CodeMarker *marker);
    void generateStatus(const Node *node, CodeMarker *marker);

    QString registerRef(const QString& ref);
    virtual QString fileBase(const Node *node) const;
    QString fileName(const Node *node);
    static int hOffset(const Node *node);
    static bool isThreeColumnEnumValueTable(const Atom *atom);
    QString getLink(const Atom *atom, const Node *relative, const Node** node);
#ifdef GENERATE_MAC_REFS
    void generateMacRef(const Node *node, CodeMarker *marker);
#endif
    void beginLink(const QString &link, const Node *node, const Node *relative);
    void endLink();
    void generateExtractionMark(const Node *node, ExtractionMarkType markType);
    void reportOrphans(const InnerNode* parent);

    void beginDitamapPage(const InnerNode* node, const QString& fileName);
    void endDitamapPage();
    void writeDitaMap(const DitaMapNode* node);
    void writeDitaRefs(const DitaRefList& ditarefs);
    QXmlStreamWriter& xmlWriter();

    QMap<QString, QString> refMap;
    int codeIndent;
    HelpProjectWriter *helpProjectWriter;
    bool inObsoleteLink;
    QRegExp funcLeftParen;
    QString style;
    QString headerScripts;
    QString headerStyles;
    QString endHeader;
    QString postHeader;
    QString postPostHeader;
    QString footer;
    QString address;
    bool pleaseGenerateMacRef;
    bool noNavigationBar;
    QString project;
    QString projectDescription;
    QString projectUrl;
    QString navigationLinks;
    QString manifestDir;
    QString examplesPath;
    QStringList stylesheets;
    QStringList customHeadElements;
    bool obsoleteLinks;
    QStack<QXmlStreamWriter*> xmlWriterStack;
    static int id;
    QList<ManifestMetaFilter> manifestMetaContent;
    QString homepage;
    QString landingpage;
    QString cppclassespage;
    QString qmltypespage;
    QString buildversion;
    QString qflagsHref_;

public:
    static bool debugging_on;
    static QString divNavTop;
};

#define HTMLGENERATOR_ADDRESS           "address"
#define HTMLGENERATOR_FOOTER            "footer"
#define HTMLGENERATOR_GENERATEMACREFS   "generatemacrefs" // ### document me
#define HTMLGENERATOR_POSTHEADER        "postheader"
#define HTMLGENERATOR_POSTPOSTHEADER    "postpostheader"
#define HTMLGENERATOR_NONAVIGATIONBAR   "nonavigationbar"
#define HTMLGENERATOR_NOSUBDIRS         "nosubdirs"


QT_END_NAMESPACE

#endif

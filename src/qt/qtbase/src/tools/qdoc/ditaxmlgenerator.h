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

#ifndef DITAXMLGENERATOR_H
#define DITAXMLGENERATOR_H

#include <qmap.h>
#include <qregexp.h>
#include <qxmlstream.h>
#include "codemarker.h"
#include "generator.h"

QT_BEGIN_NAMESPACE

typedef QMap<QString, QString> GuidMap;
typedef QMap<QString, GuidMap*> GuidMaps;

class DitaXmlGenerator : public Generator
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::DitaXmlGenerator)

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

    enum DitaTag {
        DT_NONE,
        DT_alt,
        DT_apiData,
        DT_apiDef,
        DT_apiDefItem,
        DT_apiDesc,
        DT_apiDetail,
        DT_apiItemName,
        DT_APIMap,
        DT_apiName,
        DT_apiRef,
        DT_apiRelation,
        DT_audience,
        DT_author,
        DT_b,
        DT_body,
        DT_bodydiv,
        DT_brand,
        DT_category,
        DT_codeblock,
        DT_colspec,
        DT_comment,
        DT_component,
        DT_copyrholder,
        DT_copyright,
        DT_copyryear,
        DT_created,
        DT_critdates,
        DT_cxxAPIMap,
        DT_cxxClass,
        DT_cxxClassAbstract,
        DT_cxxClassAccessSpecifier,
        DT_cxxClassAPIItemLocation,
        DT_cxxClassBaseClass,
        DT_cxxClassDeclarationFile,
        DT_cxxClassDeclarationFileLine,
        DT_cxxClassDeclarationFileLineStart,
        DT_cxxClassDeclarationFileLineEnd,
        DT_cxxClassDefinition,
        DT_cxxClassDerivation,
        DT_cxxClassDerivationAccessSpecifier,
        DT_cxxClassDerivations,
        DT_cxxClassDetail,
        DT_cxxClassNested,
        DT_cxxClassNestedClass,
        DT_cxxClassNestedDetail,
        DT_cxxDefine,
        DT_cxxDefineAccessSpecifier,
        DT_cxxDefineAPIItemLocation,
        DT_cxxDefineDeclarationFile,
        DT_cxxDefineDeclarationFileLine,
        DT_cxxDefineDefinition,
        DT_cxxDefineDetail,
        DT_cxxDefineNameLookup,
        DT_cxxDefineParameter,
        DT_cxxDefineParameterDeclarationName,
        DT_cxxDefineParameters,
        DT_cxxDefinePrototype,
        DT_cxxDefineReimplemented,
        DT_cxxEnumeration,
        DT_cxxEnumerationAccessSpecifier,
        DT_cxxEnumerationAPIItemLocation,
        DT_cxxEnumerationDeclarationFile,
        DT_cxxEnumerationDeclarationFileLine,
        DT_cxxEnumerationDeclarationFileLineStart,
        DT_cxxEnumerationDeclarationFileLineEnd,
        DT_cxxEnumerationDefinition,
        DT_cxxEnumerationDetail,
        DT_cxxEnumerationNameLookup,
        DT_cxxEnumerationPrototype,
        DT_cxxEnumerationScopedName,
        DT_cxxEnumerator,
        DT_cxxEnumeratorInitialiser,
        DT_cxxEnumeratorNameLookup,
        DT_cxxEnumeratorPrototype,
        DT_cxxEnumerators,
        DT_cxxEnumeratorScopedName,
        DT_cxxFunction,
        DT_cxxFunctionAccessSpecifier,
        DT_cxxFunctionAPIItemLocation,
        DT_cxxFunctionConst,
        DT_cxxFunctionConstructor,
        DT_cxxFunctionDeclarationFile,
        DT_cxxFunctionDeclarationFileLine,
        DT_cxxFunctionDeclaredType,
        DT_cxxFunctionDefinition,
        DT_cxxFunctionDestructor,
        DT_cxxFunctionDetail,
        DT_cxxFunctionNameLookup,
        DT_cxxFunctionParameter,
        DT_cxxFunctionParameterDeclarationName,
        DT_cxxFunctionParameterDeclaredType,
        DT_cxxFunctionParameterDefaultValue,
        DT_cxxFunctionParameters,
        DT_cxxFunctionPrototype,
        DT_cxxFunctionPureVirtual,
        DT_cxxFunctionReimplemented,
        DT_cxxFunctionScopedName,
        DT_cxxFunctionStorageClassSpecifierStatic,
        DT_cxxFunctionVirtual,
        DT_cxxTypedef,
        DT_cxxTypedefAccessSpecifier,
        DT_cxxTypedefAPIItemLocation,
        DT_cxxTypedefDeclarationFile,
        DT_cxxTypedefDeclarationFileLine,
        DT_cxxTypedefDefinition,
        DT_cxxTypedefDetail,
        DT_cxxTypedefNameLookup,
        DT_cxxTypedefScopedName,
        DT_cxxVariable,
        DT_cxxVariableAccessSpecifier,
        DT_cxxVariableAPIItemLocation,
        DT_cxxVariableDeclarationFile,
        DT_cxxVariableDeclarationFileLine,
        DT_cxxVariableDeclaredType,
        DT_cxxVariableDefinition,
        DT_cxxVariableDetail,
        DT_cxxVariableNameLookup,
        DT_cxxVariablePrototype,
        DT_cxxVariableReimplemented,
        DT_cxxVariableScopedName,
        DT_cxxVariableStorageClassSpecifierStatic,
        DT_data,
        DT_dataabout,
        DT_dd,
        DT_dl,
        DT_dlentry,
        DT_dt,
        DT_entry,
        DT_fig,
        DT_i,
        DT_image,
        DT_keyword,
        DT_keywords,
        DT_li,
        DT_link,
        DT_linktext,
        DT_lq,
        DT_map,
        DT_mapref,
        DT_metadata,
        DT_note,
        DT_ol,
        DT_othermeta,
        DT_p,
        DT_parameter,
        DT_permissions,
        DT_ph,
        DT_platform,
        DT_pre,
        DT_prodinfo,
        DT_prodname,
        DT_prolog,
        DT_publisher,
        DT_qmlAttached,
        DT_qmlDetail,
        DT_qmlImportModule,
        DT_qmlInheritedBy,
        DT_qmlInherits,
        DT_qmlInstantiates,
        DT_qmlMethod,
        DT_qmlMethodDef,
        DT_qmlMethodDetail,
        DT_qmlName,
        DT_qmlProperty,
        DT_qmlPropertyDef,
        DT_qmlPropertyDetail,
        DT_qmlPropertyGroup,
        DT_qmlPropertyGroupDef,
        DT_qmlPropertyGroupDetail,
        DT_qmlQualifier,
        DT_qmlSignal,
        DT_qmlSignalDef,
        DT_qmlSignalDetail,
        DT_qmlSignalHandler,
        DT_qmlSignalHandlerDEf,
        DT_qmlSignalHandlerDetail,
        DT_qmlSignature,
        DT_qmlSince,
        DT_qmlType,
        DT_qmlTypeDef,
        DT_qmlTypeDetail,
        DT_relatedLinks,
        DT_resourceid,
        DT_revised,
        DT_row,
        DT_section,
        DT_sectiondiv,
        DT_shortdesc,
        DT_simpletable,
        DT_source,
        DT_stentry,
        DT_sthead,
        DT_strow,
        DT_sub,
        DT_sup,
        DT_table,
        DT_tbody,
        DT_tgroup,
        DT_thead,
        DT_title,
        DT_tm,
        DT_topic,
        DT_topicmeta,
        DT_topicref,
        DT_tt,
        DT_u,
        DT_uicontrol,
        DT_ul,
        DT_unknown,
        DT_vrm,
        DT_vrmlist,
        DT_xref,
        DT_LAST
    };

public:
    DitaXmlGenerator();
    ~DitaXmlGenerator();

    virtual void initializeGenerator(const Config& config);
    virtual void terminateGenerator();
    virtual QString format();
    virtual bool canHandleFormat(const QString& format);
    virtual void generateTree();
    void generateCollisionPages();

    QString protectEnc(const QString& string);
    static QString protect(const QString& string, const QString& encoding = "ISO-8859-1");
    static QString cleanRef(const QString& ref);
    static QString sinceTitle(int i) { return sinceTitles[i]; }

protected:
    virtual int generateAtom(const Atom* atom,
                             const Node* relative,
                             CodeMarker* marker);
    virtual void generateClassLikeNode(InnerNode* inner, CodeMarker* marker);
    virtual void generateDocNode(DocNode* dn, CodeMarker* marker);
    virtual QString fileExtension() const;
    virtual QString guidForNode(const Node* node);
    virtual QString linkForNode(const Node* node, const Node* relative);

    void writeXrefListItem(const QString& link, const QString& text);
    QString fullQualification(const Node* n);

    void writeCharacters(const QString& text);
    void writeDerivations(const ClassNode* cn);
    void writeLocation(const Node* n);
    void writeFunctions(const Section& s,
                        const InnerNode* parent,
                        CodeMarker* marker,
                        const QString& attribute = QString());
    void writeNestedClasses(const Section& s, const Node* n);
    void replaceTypesWithLinks(const Node* n, const InnerNode* parent, QString& src);
    void writeParameters(const FunctionNode* fn, const InnerNode* parent, CodeMarker* marker);
    void writeEnumerations(const Section& s,
                           CodeMarker* marker,
                           const QString& attribute = QString());
    void writeTypedefs(const Section& s,
                       CodeMarker* marker,
                       const QString& attribute = QString());
    void writeDataMembers(const Section& s,
                          CodeMarker* marker,
                          const QString& attribute = QString());
    void writeProperties(const Section& s,
                         CodeMarker* marker,
                         const QString& attribute = QString());
    void writeMacros(const Section& s,
                     CodeMarker* marker,
                     const QString& attribute = QString());
    void writePropertyParameter(const QString& tag, const NodeList& nlist);
    void writeRelatedLinks(const DocNode* dn);
    void writeLink(const Node* node, const QString& tex, const QString& role);
    void writeProlog(const InnerNode* inner);
    bool writeMetadataElement(const InnerNode* inner,
                              DitaXmlGenerator::DitaTag t,
                              bool force=true);
    bool writeMetadataElements(const InnerNode* inner, DitaXmlGenerator::DitaTag t);
    void writeHrefAttribute(const QString& href);
    QString getMetadataElement(const InnerNode* inner, DitaXmlGenerator::DitaTag t);
    QStringList getMetadataElements(const InnerNode* inner, DitaXmlGenerator::DitaTag t);

private:
    enum SubTitleSize { SmallSubTitle, LargeSubTitle };

    const QPair<QString,QString> anchorForNode(const Node* node);
    void generateHeader(const Node* node,
                        const QString& name,
                        bool subpage = false);
    void generateBrief(const Node* node, CodeMarker* marker);
    void generateTableOfContents(const Node* node,
                                 CodeMarker* marker,
                                 Doc::Sections sectioningUnit,
                                 int numColumns,
                                 const Node* relative = 0);
    void generateLowStatusMembers(InnerNode* inner, CodeMarker* marker, CodeMarker::Status status);
    void generateClassHierarchy(const Node* relative, NodeMap& classMap);
    void generateAnnotatedList(const Node* relative, CodeMarker* marker, const NodeMap& nodeMap);
    void generateAnnotatedList(const Node* relative, CodeMarker* marker, const NodeList& nodes);
    void generateCompactList(ListType listType,
                             const Node* relative,
                             const NodeMap& classMap,
                             bool includeAlphabet,
                             QString commonPrefix);
    void generateFunctionIndex(const Node* relative);
    void generateLegaleseList(const Node* relative, CodeMarker* marker);
    void generateOverviewList(const Node* relative);

    void generateQmlSummary(const Section& section,
                            const Node* relative,
                            CodeMarker* marker);
    void generateQmlItem(const Node* node,
                         const Node* relative,
                         CodeMarker* marker,
                         bool summary);
    void startQmlProperty(QmlPropertyNode* qpn,
                          const InnerNode* relative,
                          CodeMarker* marker);
    void writeQmlRef(DitaTag tag,
                     Node* node,
                     const InnerNode* relative,
                     CodeMarker* marker);
    void generateDetailedQmlMember(Node* node,
                                   const InnerNode* relative,
                                   CodeMarker* marker);
    void generateQmlInherits(const QmlClassNode* qcn, CodeMarker* marker);
    void generateQmlInheritedBy(const QmlClassNode* qcn, CodeMarker* marker);
    void generateQmlInstantiates(QmlClassNode* qcn, CodeMarker* marker);
    void generateInstantiatedBy(ClassNode* cn, CodeMarker* marker);
    void generateQmlModuleDef(QmlClassNode* qcn);
    void generateQmlSince(const Node* node);

    void generateSection(const NodeList& nl,
                         const Node* relative,
                         CodeMarker* marker,
                         CodeMarker::SynopsisStyle style);
    QString getMarkedUpSynopsis(const Node* node,
                                const Node* relative,
                                CodeMarker* marker,
                                CodeMarker::SynopsisStyle style);
    void generateSectionInheritedList(const Section& section, const Node* relative);
    void writeText(const QString& markedCode, const Node* relative);

    void generateFullName(const Node* apparentNode, const Node* relative, const Node* actualNode = 0);
    void generateLink(const Atom* atom, CodeMarker* marker);
    void generateStatus(const Node* node, CodeMarker* marker);

    QString registerRef(const QString& ref);
    virtual QString fileBase(const Node *node) const;
    QString fileName(const Node *node);
    static int hOffset(const Node *node);
    static bool isThreeColumnEnumValueTable(const Atom *atom);
    QString getLink(const Atom *atom, const Node *relative, const Node **node);
#ifdef GENERATE_MAC_REFS
    void generateMacRef(const Node* node, CodeMarker* marker);
#endif
    void beginLink(const QString& link);
    void endLink();
    QString writeGuidAttribute(QString text);
    void writeGuidAttribute(Node* node);
    QString lookupGuid(QString text);
    QString lookupGuid(const QString& fileName, const QString& text);
    GuidMap* lookupGuidMap(const QString& fileName);
    virtual void beginSubPage(const InnerNode* node, const QString& fileName);
    virtual void endSubPage();
    virtual void generateInnerNode(InnerNode* node);
    QXmlStreamWriter& xmlWriter();
    void writeApiDesc(const Node* node, CodeMarker* marker, const QString& title);
    void addLink(const QString& href, const QStringRef& text, DitaTag t = DT_xref);
    void writeDitaMap();
    void writeDitaMap(const DitaMapNode* node);
    void writeStartTag(DitaTag t);
    bool writeEndTag(DitaTag t=DT_NONE);
    DitaTag currentTag();
    void clearSectionNesting() { sectionNestingLevel = 0; }
    int enterDesc(DitaTag tag, const QString& outputclass, const QString& title);
    int enterSection(const QString& outputclass, const QString& title);
    int leaveSection();
    bool inSection() const { return (sectionNestingLevel > 0); }
    int currentSectionNestingLevel() const { return sectionNestingLevel; }
    QStringList metadataDefault(DitaTag t) const;
    QString stripMarkup(const QString& src) const;
    Node* collectNodesByTypeAndSubtype(const InnerNode* parent);
    void writeDitaRefs(const DitaRefList& ditarefs);
    void writeTopicrefs(NodeMultiMap* nmm, const QString& navtitle, Node* headingnode = 0);
    bool isDuplicate(NodeMultiMap* nmm, const QString& key, Node* node);
    void debugPara(const QString& t);
    QString ditaXmlHref(Node* n);

private:
    /*
      These flags indicate which elements the generator
      is currently outputting.
     */
    bool inDetailedDescription;
    bool inLegaleseText;
    bool inObsoleteLink;
    bool inTableBody;

    bool noLinks;
    bool obsoleteLinks;

    int divNestingLevel;
    int sectionNestingLevel;
    int tableColumnCount;
    int currentColumn;

    QRegExp funcLeftParen;
    QString style;
    QString postHeader;
    QString postPostHeader;
    QString footer;
    QString address;
    bool pleaseGenerateMacRef;
    QString project;
    QString projectDescription;
    QString projectUrl;
    QString navigationLinks;
    QString version;
    QStringList vrm;
    QStringList stylesheets;
    QStringList customHeadElements;
    QMap<QString, QString> refMap;
    QMap<QString, QString> name2guidMap;
    GuidMaps guidMaps;
    static int id;
    static QString ditaTags[];
    QStack<QXmlStreamWriter*> xmlWriterStack;
    QStack<DitaTag> tagStack;
    ConfigVarMultimap metadataDefaults;
    QVector<NodeMultiMap*> nodeTypeMaps;
    QVector<NodeMultiMap*> nodeSubtypeMaps;
    QVector<NodeMultiMap*> pageTypeMaps;
};

#define DITAXMLGENERATOR_ADDRESS           "address"
#define DITAXMLGENERATOR_FOOTER            "footer"
#define DITAXMLGENERATOR_GENERATEMACREFS   "generatemacrefs"
#define DITAXMLGENERATOR_POSTHEADER        "postheader"
#define DITAXMLGENERATOR_POSTPOSTHEADER    "postpostheader"
#define DITAXMLGENERATOR_STYLE             "style"
#define DITAXMLGENERATOR_STYLESHEETS       "stylesheets"
#define DITAXMLGENERATOR_CUSTOMHEADELEMENTS "customheadelements"

QT_END_NAMESPACE

#endif

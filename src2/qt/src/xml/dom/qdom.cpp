/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtXml module of the Qt Toolkit.
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

#include <qplatformdefs.h>
#include <qdom.h>
#include "private/qxmlutils_p.h"

#ifndef QT_NO_DOM

#include <qatomic.h>
#include <qbuffer.h>
#include <qhash.h>
#include <qiodevice.h>
#include <qlist.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <qtextstream.h>
#include <qxml.h>
#include <qvariant.h>
#include <qmap.h>
#include <qshareddata.h>
#include <qdebug.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

/*
  ### old todo comments -- I don't know if they still apply...

  If the document dies, remove all pointers to it from children
  which can not be deleted at this time.

  If a node dies and has direct children which can not be deleted,
  then remove the pointer to the parent.

  createElement and friends create double reference counts.
*/

/* ##### new TODOs:

  Remove emtpy emthods in the *Private classes

  Make a lot of the (mostly empty) methods in the public classes inline.
  Specially constructors assignment operators and comparison operators are candidates.

  The virtual isXxx functions in *Private can probably be replaced by inline methods checking the nodeType().
*/

/*
  Reference counting:

  Some simple rules:
  1) If an intern object returns a pointer to another intern object
     then the reference count of the returned object is not increased.
  2) If an extern object is created and gets a pointer to some intern
     object, then the extern object increases the intern objects reference count.
  3) If an extern object is deleted, then it decreases the reference count
     on its associated intern object and deletes it if nobody else hold references
     on the intern object.
*/


/*
  Helper to split a qualified name in the prefix and local name.
*/
static void qt_split_namespace(QString& prefix, QString& name, const QString& qName, bool hasURI)
{
    int i = qName.indexOf(QLatin1Char(':'));
    if (i == -1) {
        if (hasURI)
            prefix = QLatin1String("");
        else
            prefix.clear();
        name = qName;
    } else {
        prefix = qName.left(i);
        name = qName.mid(i + 1);
    }
}

/**************************************************************
 *
 * Private class declerations
 *
 **************************************************************/

class QDomImplementationPrivate
{
public:
    inline QDomImplementationPrivate() {}

    QDomImplementationPrivate* clone();
    QAtomicInt ref;
    static QDomImplementation::InvalidDataPolicy invalidDataPolicy;
};

class QDomNodePrivate
{
public:
    QDomNodePrivate(QDomDocumentPrivate*, QDomNodePrivate* parent = 0);
    QDomNodePrivate(QDomNodePrivate* n, bool deep);
    virtual ~QDomNodePrivate();

    QString nodeName() const { return name; }
    QString nodeValue() const { return value; }
    virtual void setNodeValue(const QString& v) { value = v; }

    QDomDocumentPrivate* ownerDocument();
    void setOwnerDocument(QDomDocumentPrivate* doc);

    virtual QDomNodePrivate* insertBefore(QDomNodePrivate* newChild, QDomNodePrivate* refChild);
    virtual QDomNodePrivate* insertAfter(QDomNodePrivate* newChild, QDomNodePrivate* refChild);
    virtual QDomNodePrivate* replaceChild(QDomNodePrivate* newChild, QDomNodePrivate* oldChild);
    virtual QDomNodePrivate* removeChild(QDomNodePrivate* oldChild);
    virtual QDomNodePrivate* appendChild(QDomNodePrivate* newChild);

    QDomNodePrivate* namedItem(const QString& name);

    virtual QDomNodePrivate* cloneNode(bool deep = true);
    virtual void normalize();
    virtual void clear();

    inline QDomNodePrivate* parent() const { return hasParent ? ownerNode : 0; }
    inline void setParent(QDomNodePrivate *p) { ownerNode = p; hasParent = true; }

    void setNoParent() {
        ownerNode = hasParent ? (QDomNodePrivate*)ownerDocument() : 0;
        hasParent = false;
    }

    // Dynamic cast
    virtual bool isAttr() const                     { return false; }
    virtual bool isCDATASection() const             { return false; }
    virtual bool isDocumentFragment() const         { return false; }
    virtual bool isDocument() const                 { return false; }
    virtual bool isDocumentType() const             { return false; }
    virtual bool isElement() const                  { return false; }
    virtual bool isEntityReference() const          { return false; }
    virtual bool isText() const                     { return false; }
    virtual bool isEntity() const                   { return false; }
    virtual bool isNotation() const                 { return false; }
    virtual bool isProcessingInstruction() const    { return false; }
    virtual bool isCharacterData() const            { return false; }
    virtual bool isComment() const                  { return false; }

    virtual QDomNode::NodeType nodeType() const { return QDomNode::BaseNode; }

    virtual void save(QTextStream&, int, int) const;

    void setLocation(int lineNumber, int columnNumber);

    // Variables
    QAtomicInt ref;
    QDomNodePrivate* prev;
    QDomNodePrivate* next;
    QDomNodePrivate* ownerNode; // either the node's parent or the node's owner document
    QDomNodePrivate* first;
    QDomNodePrivate* last;

    QString name; // this is the local name if prefix != null
    QString value;
    QString prefix; // set this only for ElementNode and AttributeNode
    QString namespaceURI; // set this only for ElementNode and AttributeNode
    bool createdWithDom1Interface : 1;
    bool hasParent                : 1;

    int lineNumber;
    int columnNumber;
};

class QDomNodeListPrivate
{
public:
    QDomNodeListPrivate(QDomNodePrivate*);
    QDomNodeListPrivate(QDomNodePrivate*, const QString& );
    QDomNodeListPrivate(QDomNodePrivate*, const QString&, const QString& );
    ~QDomNodeListPrivate();

    bool operator== (const QDomNodeListPrivate&) const;
    bool operator!= (const QDomNodeListPrivate&) const;

    void createList();
    QDomNodePrivate* item(int index);
    uint length() const;

    QAtomicInt ref;
    /*
      This list contains the children of this node.
     */
    QDomNodePrivate* node_impl;
    QString tagname;
    QString nsURI;
    QList<QDomNodePrivate*> list;
    long timestamp;
};

class QDomNamedNodeMapPrivate
{
public:
    QDomNamedNodeMapPrivate(QDomNodePrivate*);
    ~QDomNamedNodeMapPrivate();

    QDomNodePrivate* namedItem(const QString& name) const;
    QDomNodePrivate* namedItemNS(const QString& nsURI, const QString& localName) const;
    QDomNodePrivate* setNamedItem(QDomNodePrivate* arg);
    QDomNodePrivate* setNamedItemNS(QDomNodePrivate* arg);
    QDomNodePrivate* removeNamedItem(const QString& name);
    QDomNodePrivate* item(int index) const;
    uint length() const;
    bool contains(const QString& name) const;
    bool containsNS(const QString& nsURI, const QString & localName) const;

    /**
     * Remove all children from the map.
     */
    void clearMap();
    bool isReadOnly() { return readonly; }
    void setReadOnly(bool r) { readonly = r; }
    bool isAppendToParent() { return appendToParent; }
    /**
     * If true, then the node will redirect insert/remove calls
     * to its parent by calling QDomNodePrivate::appendChild or removeChild.
     * In addition the map wont increase or decrease the reference count
     * of the nodes it contains.
     *
     * By default this value is false and the map will handle reference counting
     * by itself.
     */
    void setAppendToParent(bool b) { appendToParent = b; }

    /**
     * Creates a copy of the map. It is a deep copy
     * that means that all children are cloned.
     */
    QDomNamedNodeMapPrivate* clone(QDomNodePrivate* parent);

    // Variables
    QAtomicInt ref;
    QHash<QString, QDomNodePrivate *> map;
    QDomNodePrivate* parent;
    bool readonly;
    bool appendToParent;
};

class QDomDocumentTypePrivate : public QDomNodePrivate
{
public:
    QDomDocumentTypePrivate(QDomDocumentPrivate*, QDomNodePrivate* parent = 0);
    QDomDocumentTypePrivate(QDomDocumentTypePrivate* n, bool deep);
    ~QDomDocumentTypePrivate();
    void init();

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    QDomNodePrivate* insertBefore(QDomNodePrivate* newChild, QDomNodePrivate* refChild);
    QDomNodePrivate* insertAfter(QDomNodePrivate* newChild, QDomNodePrivate* refChild);
    QDomNodePrivate* replaceChild(QDomNodePrivate* newChild, QDomNodePrivate* oldChild);
    QDomNodePrivate* removeChild(QDomNodePrivate* oldChild);
    QDomNodePrivate* appendChild(QDomNodePrivate* newChild);

    virtual bool isDocumentType() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::DocumentTypeNode; }

    void save(QTextStream& s, int, int) const;

    // Variables
    QDomNamedNodeMapPrivate* entities;
    QDomNamedNodeMapPrivate* notations;
    QString publicId;
    QString systemId;
    QString internalSubset;
};

class QDomDocumentFragmentPrivate : public QDomNodePrivate
{
public:
    QDomDocumentFragmentPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent = 0);
    QDomDocumentFragmentPrivate(QDomNodePrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    virtual QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isDocumentFragment() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::DocumentFragmentNode; }
};

class QDomCharacterDataPrivate : public QDomNodePrivate
{
public:
    QDomCharacterDataPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& data);
    QDomCharacterDataPrivate(QDomCharacterDataPrivate* n, bool deep);

    uint dataLength() const;
    QString substringData(unsigned long offset, unsigned long count) const;
    void appendData(const QString& arg);
    void insertData(unsigned long offset, const QString& arg);
    void deleteData(unsigned long offset, unsigned long count);
    void replaceData(unsigned long offset, unsigned long count, const QString& arg);

    // Reimplemented from QDomNodePrivate
    virtual bool isCharacterData() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::CharacterDataNode; }
    QDomNodePrivate* cloneNode(bool deep = true);
};

class QDomTextPrivate : public QDomCharacterDataPrivate
{
public:
    QDomTextPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& val);
    QDomTextPrivate(QDomTextPrivate* n, bool deep);

    QDomTextPrivate* splitText(int offset);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isText() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::TextNode; }
    virtual void save(QTextStream& s, int, int) const;
};

class QDomAttrPrivate : public QDomNodePrivate
{
public:
    QDomAttrPrivate(QDomDocumentPrivate*, QDomNodePrivate*, const QString& name);
    QDomAttrPrivate(QDomDocumentPrivate*, QDomNodePrivate*, const QString& nsURI, const QString& qName);
    QDomAttrPrivate(QDomAttrPrivate* n, bool deep);

    bool specified() const;

    // Reimplemented from QDomNodePrivate
    void setNodeValue(const QString& v);
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isAttr() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::AttributeNode; }
    virtual void save(QTextStream& s, int, int) const;

    // Variables
    bool m_specified;
};

class QDomElementPrivate : public QDomNodePrivate
{
public:
    QDomElementPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& name);
    QDomElementPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& nsURI, const QString& qName);
    QDomElementPrivate(QDomElementPrivate* n, bool deep);
    ~QDomElementPrivate();

    QString attribute(const QString& name, const QString& defValue) const;
    QString attributeNS(const QString& nsURI, const QString& localName, const QString& defValue) const;
    void setAttribute(const QString& name, const QString& value);
    void setAttributeNS(const QString& nsURI, const QString& qName, const QString& newValue);
    void removeAttribute(const QString& name);
    QDomAttrPrivate* attributeNode(const QString& name);
    QDomAttrPrivate* attributeNodeNS(const QString& nsURI, const QString& localName);
    QDomAttrPrivate* setAttributeNode(QDomAttrPrivate* newAttr);
    QDomAttrPrivate* setAttributeNodeNS(QDomAttrPrivate* newAttr);
    QDomAttrPrivate* removeAttributeNode(QDomAttrPrivate* oldAttr);
    bool hasAttribute(const QString& name);
    bool hasAttributeNS(const QString& nsURI, const QString& localName);

    QString text();

    // Reimplemented from QDomNodePrivate
    QDomNamedNodeMapPrivate* attributes() { return m_attr; }
    bool hasAttributes() { return (m_attr->length() > 0); }
    virtual bool isElement() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::ElementNode; }
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual void save(QTextStream& s, int, int) const;

    // Variables
    QDomNamedNodeMapPrivate* m_attr;
};


class QDomCommentPrivate : public QDomCharacterDataPrivate
{
public:
    QDomCommentPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& val);
    QDomCommentPrivate(QDomCommentPrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isComment() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::CommentNode; }
    virtual void save(QTextStream& s, int, int) const;
};

class QDomCDATASectionPrivate : public QDomTextPrivate
{
public:
    QDomCDATASectionPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& val);
    QDomCDATASectionPrivate(QDomCDATASectionPrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isCDATASection() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::CDATASectionNode; }
    virtual void save(QTextStream& s, int, int) const;
};

class QDomNotationPrivate : public QDomNodePrivate
{
public:
    QDomNotationPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& name,
                          const QString& pub, const QString& sys);
    QDomNotationPrivate(QDomNotationPrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isNotation() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::NotationNode; }
    virtual void save(QTextStream& s, int, int) const;

    // Variables
    QString m_sys;
    QString m_pub;
};

class QDomEntityPrivate : public QDomNodePrivate
{
public:
    QDomEntityPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& name,
                        const QString& pub, const QString& sys, const QString& notation);
    QDomEntityPrivate(QDomEntityPrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isEntity() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::EntityNode; }
    virtual void save(QTextStream& s, int, int) const;

    // Variables
    QString m_sys;
    QString m_pub;
    QString m_notationName;
};

class QDomEntityReferencePrivate : public QDomNodePrivate
{
public:
    QDomEntityReferencePrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& name);
    QDomEntityReferencePrivate(QDomNodePrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    bool isEntityReference() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::EntityReferenceNode; }
    virtual void save(QTextStream& s, int, int) const;
};

class QDomProcessingInstructionPrivate : public QDomNodePrivate
{
public:
    QDomProcessingInstructionPrivate(QDomDocumentPrivate*, QDomNodePrivate* parent, const QString& target,
                                       const QString& data);
    QDomProcessingInstructionPrivate(QDomProcessingInstructionPrivate* n, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    virtual bool isProcessingInstruction() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::ProcessingInstructionNode; }
    virtual void save(QTextStream& s, int, int) const;
};

class QDomDocumentPrivate : public QDomNodePrivate
{
public:
    QDomDocumentPrivate();
    QDomDocumentPrivate(const QString& name);
    QDomDocumentPrivate(QDomDocumentTypePrivate* dt);
    QDomDocumentPrivate(QDomDocumentPrivate* n, bool deep);
    ~QDomDocumentPrivate();

    bool setContent(QXmlInputSource *source, bool namespaceProcessing, QString *errorMsg, int *errorLine, int *errorColumn);
    bool setContent(QXmlInputSource *source, QXmlReader *reader, QString *errorMsg, int *errorLine, int *errorColumn);

    // Attributes
    QDomDocumentTypePrivate* doctype() { return type.data(); }
    QDomImplementationPrivate* implementation() { return impl.data(); }
    QDomElementPrivate* documentElement();

    // Factories
    QDomElementPrivate* createElement(const QString& tagName);
    QDomElementPrivate*        createElementNS(const QString& nsURI, const QString& qName);
    QDomDocumentFragmentPrivate* createDocumentFragment();
    QDomTextPrivate* createTextNode(const QString& data);
    QDomCommentPrivate* createComment(const QString& data);
    QDomCDATASectionPrivate* createCDATASection(const QString& data);
    QDomProcessingInstructionPrivate* createProcessingInstruction(const QString& target, const QString& data);
    QDomAttrPrivate* createAttribute(const QString& name);
    QDomAttrPrivate* createAttributeNS(const QString& nsURI, const QString& qName);
    QDomEntityReferencePrivate* createEntityReference(const QString& name);

    QDomNodePrivate* importNode(const QDomNodePrivate* importedNode, bool deep);

    // Reimplemented from QDomNodePrivate
    QDomNodePrivate* cloneNode(bool deep = true);
    bool isDocument() const { return true; }
    QDomNode::NodeType nodeType() const { return QDomNode::DocumentNode; }
    void clear();

    // Variables
    QExplicitlySharedDataPointer<QDomImplementationPrivate> impl;
    QExplicitlySharedDataPointer<QDomDocumentTypePrivate> type;

    void saveDocument(QTextStream& stream, const int indent, QDomNode::EncodingPolicy encUsed) const;

    /* \internal
       Counter for the QDomNodeListPrivate timestamps.

       This is a cache optimization, that might in some cases be effective. The
       dilemma is that QDomNode::childNodes() returns a list, but the
       implementation stores the children in a linked list. Hence, in order to
       get the children out through childNodes(), a list must be populated each
       time, which is O(N).

       DOM has the requirement of node references being live, see DOM Core
       Level 3, 1.1.1 The DOM Structure Model, which means that changes to the
       underlying documents must be reflected in node lists.

       This mechanism, nodeListTime, is a caching optimization that reduces the
       amount of times the node list is rebuilt, by only doing so when the
       document actually changes. However, a change to anywhere in any document
       invalidate all lists, since no dependency tracking is done.

       It functions by that all modifying functions(insertBefore() and so on)
       increment the count; each QDomNodeListPrivate copies nodeListTime on
       construction, and compares its own value to nodeListTime in order to
       determine whether it needs to rebuild.

       This is reentrant. The nodeListTime may overflow, but that's ok since we
       check for equalness, not whether nodeListTime is smaller than the list's
       stored timestamp.
    */
    long nodeListTime;
};

/**************************************************************
 *
 * QDomHandler
 *
 **************************************************************/

class QDomHandler : public QXmlDefaultHandler
{
public:
    QDomHandler(QDomDocumentPrivate* d, bool namespaceProcessing);
    ~QDomHandler();

    // content handler
    bool endDocument();
    bool startElement(const QString& nsURI, const QString& localName, const QString& qName, const QXmlAttributes& atts);
    bool endElement(const QString& nsURI, const QString& localName, const QString& qName);
    bool characters(const QString& ch);
    bool processingInstruction(const QString& target, const QString& data);
    bool skippedEntity(const QString& name);

    // error handler
    bool fatalError(const QXmlParseException& exception);

    // lexical handler
    bool startCDATA();
    bool endCDATA();
    bool startEntity(const QString &);
    bool endEntity(const QString &);
    bool startDTD(const QString& name, const QString& publicId, const QString& systemId);
    bool comment(const QString& ch);

    // decl handler
    bool externalEntityDecl(const QString &name, const QString &publicId, const QString &systemId) ;

    // DTD handler
    bool notationDecl(const QString & name, const QString & publicId, const QString & systemId);
    bool unparsedEntityDecl(const QString &name, const QString &publicId, const QString &systemId, const QString &notationName) ;

    void setDocumentLocator(QXmlLocator *locator);

    QString errorMsg;
    int errorLine;
    int errorColumn;

private:
    QDomDocumentPrivate *doc;
    QDomNodePrivate *node;
    QString entityName;
    bool cdata;
    bool nsProcessing;
    QXmlLocator *locator;

#ifdef Q_OS_SYMBIAN
    // Workaround crash in elf2e32 under Wine.
    virtual void dummy() {}
#endif
};

/**************************************************************
 *
 * Functions for verifying legal data
 *
 **************************************************************/
QDomImplementation::InvalidDataPolicy QDomImplementationPrivate::invalidDataPolicy
    = QDomImplementation::AcceptInvalidChars;

// [5] Name ::= (Letter | '_' | ':') (NameChar)*

static QString fixedXmlName(const QString &_name, bool *ok, bool namespaces = false)
{
    QString name, prefix;
    if (namespaces)
        qt_split_namespace(prefix, name, _name, true);
    else
        name = _name;

    if (name.isEmpty()) {
        *ok = false;
        return QString();
    }

    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return _name;
    }

    QString result;
    bool firstChar = true;
    for (int i = 0; i < name.size(); ++i) {
        QChar c = name.at(i);
        if (firstChar) {
            if (QXmlUtils::isLetter(c) || c.unicode() == '_' || c.unicode() == ':') {
                result.append(c);
                firstChar = false;
            } else if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
                *ok = false;
                return QString();
            }
        } else {
            if (QXmlUtils::isNameChar(c))
                result.append(c);
            else if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
                *ok = false;
                return QString();
            }
        }
    }

    if (result.isEmpty()) {
        *ok = false;
        return QString();
    }

    *ok = true;
    if (namespaces && !prefix.isEmpty())
        return prefix + QLatin1Char(':') + result;
    return result;
}

// [14] CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*)
// '<', '&' and "]]>" will be escaped when writing

static QString fixedCharData(const QString &data, bool *ok)
{
    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return data;
    }

    QString result;
    for (int i = 0; i < data.size(); ++i) {
        QChar c = data.at(i);
        if (QXmlUtils::isChar(c)) {
            result.append(c);
        } else if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
            *ok = false;
            return QString();
        }
    }

    *ok = true;
    return result;
}

// [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
// can't escape "--", since entities are not recognised within comments

static QString fixedComment(const QString &data, bool *ok)
{
    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return data;
    }

    QString fixedData = fixedCharData(data, ok);
    if (!*ok)
        return QString();

    for (;;) {
        int idx = fixedData.indexOf(QLatin1String("--"));
        if (idx == -1)
            break;
        if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
            *ok = false;
            return QString();
        }
        fixedData.remove(idx, 2);
    }

    *ok = true;
    return fixedData;
}

// [20] CData ::= (Char* - (Char* ']]>' Char*))
// can't escape "]]>", since entities are not recognised within comments

static QString fixedCDataSection(const QString &data, bool *ok)
{
    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return data;
    }

    QString fixedData = fixedCharData(data, ok);
    if (!*ok)
        return QString();

    for (;;) {
        int idx = fixedData.indexOf(QLatin1String("]]>"));
        if (idx == -1)
            break;
        if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
            *ok = false;
            return QString();
        }
        fixedData.remove(idx, 3);
    }

    *ok = true;
    return fixedData;
}

// [16] PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'

static QString fixedPIData(const QString &data, bool *ok)
{
    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return data;
    }

    QString fixedData = fixedCharData(data, ok);
    if (!*ok)
        return QString();

    for (;;) {
        int idx = fixedData.indexOf(QLatin1String("?>"));
        if (idx == -1)
            break;
        if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
            *ok = false;
            return QString();
        }
        fixedData.remove(idx, 2);
    }

    *ok = true;
    return fixedData;
}

// [12] PubidLiteral ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
// The correct quote will be chosen when writing

static QString fixedPubidLiteral(const QString &data, bool *ok)
{
    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return data;
    }

    QString result;

    if(QXmlUtils::isPublicID(data))
        result = data;
    else if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
        *ok = false;
        return QString();
    }

    if (result.indexOf(QLatin1Char('\'')) != -1
        && result.indexOf(QLatin1Char('"')) != -1) {
        if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
            *ok = false;
            return QString();
        } else {
            result.remove(QLatin1Char('\''));
        }
    }

    *ok = true;
    return result;
}

// [11] SystemLiteral ::= ('"' [^"]* '"') | ("'" [^']* "'")
// The correct quote will be chosen when writing

static QString fixedSystemLiteral(const QString &data, bool *ok)
{
    if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::AcceptInvalidChars) {
        *ok = true;
        return data;
    }

    QString result = data;

    if (result.indexOf(QLatin1Char('\'')) != -1
        && result.indexOf(QLatin1Char('"')) != -1) {
        if (QDomImplementationPrivate::invalidDataPolicy == QDomImplementation::ReturnNullNode) {
            *ok = false;
            return QString();
        } else {
            result.remove(QLatin1Char('\''));
        }
    }

    *ok = true;
    return result;
}

/**************************************************************
 *
 * QDomImplementationPrivate
 *
 **************************************************************/

QDomImplementationPrivate* QDomImplementationPrivate::clone()
{
    return new QDomImplementationPrivate;
}

/**************************************************************
 *
 * QDomImplementation
 *
 **************************************************************/

/*!
    \class QDomImplementation
    \reentrant
    \brief The QDomImplementation class provides information about the
    features of the DOM implementation.

    \inmodule QtXml
    \ingroup xml-tools

    This class describes the features that are supported by the DOM
    implementation. Currently the XML subset of DOM Level 1 and DOM
    Level 2 Core are supported.

    Normally you will use the function QDomDocument::implementation()
    to get the implementation object.

    You can create a new document type with createDocumentType() and a
    new document with createDocument().

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}. For a more
    general introduction of the DOM implementation see the QDomDocument
    documentation.

    The QDom classes have a few issues of nonconformance with the XML
    specifications that cannot be fixed in Qt 4 without breaking backward
    compatibility. The QtXmlPatterns module and the QXmlStreamReader and
    QXmlStreamWriter classes have a higher degree of a conformance.

    \sa hasFeature()
*/

/*!
    Constructs a QDomImplementation object.
*/
QDomImplementation::QDomImplementation()
{
    impl = 0;
}

/*!
    Constructs a copy of \a x.
*/
QDomImplementation::QDomImplementation(const QDomImplementation &x)
{
    impl = x.impl;
    if (impl)
        impl->ref.ref();
}

QDomImplementation::QDomImplementation(QDomImplementationPrivate *p)
{
    // We want to be co-owners, so increase the reference count
    impl = p;
    if (impl)
        impl->ref.ref();
}

/*!
    Assigns \a x to this DOM implementation.
*/
QDomImplementation& QDomImplementation::operator=(const QDomImplementation &x)
{
    if (x.impl)
        x.impl->ref.ref();
    if (impl && !impl->ref.deref())
        delete impl;
    impl = x.impl;
    return *this;
}

/*!
    Returns true if \a x and this DOM implementation object were
    created from the same QDomDocument; otherwise returns false.
*/
bool QDomImplementation::operator==(const QDomImplementation &x) const
{
    return (impl == x.impl);
}

/*!
    Returns true if \a x and this DOM implementation object were
    created from different QDomDocuments; otherwise returns false.
*/
bool QDomImplementation::operator!=(const QDomImplementation &x) const
{
    return (impl != x.impl);
}

/*!
    Destroys the object and frees its resources.
*/
QDomImplementation::~QDomImplementation()
{
    if (impl && !impl->ref.deref())
        delete impl;
}

/*!
    The function returns true if QDom implements the requested \a
    version of a \a feature; otherwise returns false.

    The currently supported features and their versions:
    \table
    \header \i Feature \i Version
    \row \i XML \i 1.0
    \endtable
*/
bool QDomImplementation::hasFeature(const QString& feature, const QString& version) const
{
    if (feature == QLatin1String("XML")) {
        if (version.isEmpty() || version == QLatin1String("1.0")) {
            return true;
        }
    }
    // ### add DOM level 2 features
    return false;
}

/*!
    Creates a document type node for the name \a qName.

    \a publicId specifies the public identifier of the external
    subset. If you specify an empty string (QString()) as the \a
    publicId, this means that the document type has no public
    identifier.

    \a systemId specifies the system identifier of the external
    subset. If you specify an empty string as the \a systemId, this
    means that the document type has no system identifier.

    Since you cannot have a public identifier without a system
    identifier, the public identifier is set to an empty string if
    there is no system identifier.

    DOM level 2 does not support any other document type declaration
    features.

    The only way you can use a document type that was created this
    way, is in combination with the createDocument() function to
    create a QDomDocument with this document type.

    In the DOM specification, this is the only way to create a non-null
    document. For historical reasons, Qt also allows to create the
    document using the default empty constructor. The resulting document
    is null, but becomes non-null when a factory function, for example
    QDomDocument::createElement(), is called. The document also becomes
    non-null when setContent() is called.

    \sa createDocument()
*/
QDomDocumentType QDomImplementation::createDocumentType(const QString& qName, const QString& publicId, const QString& systemId)
{
    bool ok;
    QString fixedName = fixedXmlName(qName, &ok, true);
    if (!ok)
        return QDomDocumentType();

    QString fixedPublicId = fixedPubidLiteral(publicId, &ok);
    if (!ok)
        return QDomDocumentType();

    QString fixedSystemId = fixedSystemLiteral(systemId, &ok);
    if (!ok)
        return QDomDocumentType();

    QDomDocumentTypePrivate *dt = new QDomDocumentTypePrivate(0);
    dt->name = fixedName;
    if (systemId.isNull()) {
        dt->publicId.clear();
        dt->systemId.clear();
    } else {
        dt->publicId = fixedPublicId;
        dt->systemId = fixedSystemId;
    }
    dt->ref.deref();
    return QDomDocumentType(dt);
}

/*!
    Creates a DOM document with the document type \a doctype. This
    function also adds a root element node with the qualified name \a
    qName and the namespace URI \a nsURI.
*/
QDomDocument QDomImplementation::createDocument(const QString& nsURI, const QString& qName, const QDomDocumentType& doctype)
{
    QDomDocument doc(doctype);
    QDomElement root = doc.createElementNS(nsURI, qName);
    if (root.isNull())
        return QDomDocument();
    doc.appendChild(root);
    return doc;
}

/*!
    Returns false if the object was created by
    QDomDocument::implementation(); otherwise returns true.
*/
bool QDomImplementation::isNull()
{
    return (impl == 0);
}

/*!
    \enum QDomImplementation::InvalidDataPolicy

    This enum specifies what should be done when a factory function
    in QDomDocument is called with invalid data.
    \value AcceptInvalidChars The data should be stored in the DOM object
        anyway. In this case the resulting XML document might not be well-formed.
        This is the default value and QDom's behavior in Qt < 4.1.
    \value DropInvalidChars The invalid characters should be removed from
        the data.
    \value ReturnNullNode The factory function should return a null node.

    \sa setInvalidDataPolicy() invalidDataPolicy()
*/

/*!
   \enum QDomNode::EncodingPolicy
   \since 4.3

   This enum specifies how QDomNode::save() determines what encoding to use
   when serializing.

   \value EncodingFromDocument The encoding is fetched from the document.
   \value EncodingFromTextStream The encoding is fetched from the QTextStream.

   See also the overload of the save() function that takes an EncodingPolicy.
*/

/*!
    \since 4.1
    \nonreentrant

    Returns the invalid data policy, which specifies what should be done when
    a factory function in QDomDocument is passed invalid data.

    \sa setInvalidDataPolicy() InvalidDataPolicy
*/

QDomImplementation::InvalidDataPolicy QDomImplementation::invalidDataPolicy()
{
    return QDomImplementationPrivate::invalidDataPolicy;
}

/*!
    \since 4.1
    \nonreentrant

    Sets the invalid data policy, which specifies what should be done when
    a factory function in QDomDocument is passed invalid data.

    The \a policy is set for all instances of QDomDocument which already
    exist and which will be created in the future.

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 0

    \sa invalidDataPolicy() InvalidDataPolicy
*/

void QDomImplementation::setInvalidDataPolicy(InvalidDataPolicy policy)
{
    QDomImplementationPrivate::invalidDataPolicy = policy;
}

/**************************************************************
 *
 * QDomNodeListPrivate
 *
 **************************************************************/

QDomNodeListPrivate::QDomNodeListPrivate(QDomNodePrivate *n_impl)
{
    ref  = 1;
    node_impl = n_impl;
    if (node_impl)
        node_impl->ref.ref();
    timestamp = 0;
}

QDomNodeListPrivate::QDomNodeListPrivate(QDomNodePrivate *n_impl, const QString &name)
{
    ref = 1;
    node_impl = n_impl;
    if (node_impl)
        node_impl->ref.ref();
    tagname = name;
    timestamp = 0;
}

QDomNodeListPrivate::QDomNodeListPrivate(QDomNodePrivate *n_impl, const QString &_nsURI, const QString &localName)
{
    ref = 1;
    node_impl = n_impl;
    if (node_impl)
        node_impl->ref.ref();
    tagname = localName;
    nsURI = _nsURI;
    timestamp = 0;
}

QDomNodeListPrivate::~QDomNodeListPrivate()
{
    if (node_impl && !node_impl->ref.deref())
        delete node_impl;
}

bool QDomNodeListPrivate::operator==(const QDomNodeListPrivate &other) const
{
    return (node_impl == other.node_impl) && (tagname == other.tagname);
}

bool QDomNodeListPrivate::operator!=(const QDomNodeListPrivate &other) const
{
    return (node_impl != other.node_impl) || (tagname != other.tagname);
}

void QDomNodeListPrivate::createList()
{
    if (!node_impl)
        return;

    const QDomDocumentPrivate *const doc = node_impl->ownerDocument();
    if (doc && timestamp != doc->nodeListTime)
        timestamp = doc->nodeListTime;

    QDomNodePrivate* p = node_impl->first;

    list.clear();
    if (tagname.isNull()) {
        while (p) {
            list.append(p);
            p = p->next;
        }
    } else if (nsURI.isNull()) {
        while (p && p != node_impl) {
            if (p->isElement() && p->nodeName() == tagname) {
                list.append(p);
            }
            if (p->first)
                p = p->first;
            else if (p->next)
                p = p->next;
            else {
                p = p->parent();
                while (p && p != node_impl && !p->next)
                    p = p->parent();
                if (p && p != node_impl)
                    p = p->next;
            }
        }
    } else {
        while (p && p != node_impl) {
            if (p->isElement() && p->name==tagname && p->namespaceURI==nsURI) {
                list.append(p);
            }
            if (p->first)
                p = p->first;
            else if (p->next)
                p = p->next;
            else {
                p = p->parent();
                while (p && p != node_impl && !p->next)
                    p = p->parent();
                if (p && p != node_impl)
                    p = p->next;
            }
        }
    }
}

QDomNodePrivate* QDomNodeListPrivate::item(int index)
{
    if (!node_impl)
        return 0;

    const QDomDocumentPrivate *const doc = node_impl->ownerDocument();
    if (!doc || timestamp != doc->nodeListTime)
        createList();

    if (index >= list.size())
        return 0;

    return list.at(index);
}

uint QDomNodeListPrivate::length() const
{
    if (!node_impl)
        return 0;

    const QDomDocumentPrivate *const doc = node_impl->ownerDocument();
    if (!doc || timestamp != doc->nodeListTime) {
        QDomNodeListPrivate *that = const_cast<QDomNodeListPrivate *>(this);
        that->createList();
    }

    return list.count();
}

/**************************************************************
 *
 * QDomNodeList
 *
 **************************************************************/

/*!
    \class QDomNodeList
    \reentrant
    \brief The QDomNodeList class is a list of QDomNode objects.

    \inmodule QtXml
    \ingroup xml-tools

    Lists can be obtained by QDomDocument::elementsByTagName() and
    QDomNode::childNodes(). The Document Object Model (DOM) requires
    these lists to be "live": whenever you change the underlying
    document, the contents of the list will get updated.

    You can get a particular node from the list with item(). The
    number of items in the list is returned by length().

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.

    \sa QDomNode::childNodes() QDomDocument::elementsByTagName()
*/

/*!
    Creates an empty node list.
*/
QDomNodeList::QDomNodeList()
{
    impl = 0;
}

QDomNodeList::QDomNodeList(QDomNodeListPrivate* p)
{
    impl = p;
}

/*!
    Constructs a copy of \a n.
*/
QDomNodeList::QDomNodeList(const QDomNodeList& n)
{
    impl = n.impl;
    if (impl)
        impl->ref.ref();
}

/*!
    Assigns \a n to this node list.
*/
QDomNodeList& QDomNodeList::operator=(const QDomNodeList &n)
{
    if (n.impl)
        n.impl->ref.ref();
    if (impl && !impl->ref.deref())
        delete impl;
    impl = n.impl;
    return *this;
}

/*!
    Returns true if the node list \a n and this node list are equal;
    otherwise returns false.
*/
bool QDomNodeList::operator==(const QDomNodeList &n) const
{
    if (impl == n.impl)
        return true;
    if (!impl || !n.impl)
        return false;
    return (*impl == *n.impl);
}

/*!
    Returns true the node list \a n and this node list are not equal;
    otherwise returns false.
*/
bool QDomNodeList::operator!=(const QDomNodeList &n) const
{
    return !operator==(n);
}

/*!
    Destroys the object and frees its resources.
*/
QDomNodeList::~QDomNodeList()
{
    if (impl && !impl->ref.deref())
        delete impl;
}

/*!
    Returns the node at position \a index.

    If \a index is negative or if \a index >= length() then a null
    node is returned (i.e. a node for which QDomNode::isNull() returns
    true).

    \sa length()
*/
QDomNode QDomNodeList::item(int index) const
{
    if (!impl)
        return QDomNode();

    return QDomNode(impl->item(index));
}

/*!
    Returns the number of nodes in the list.
*/
uint QDomNodeList::length() const
{
    if (!impl)
        return 0;
    return impl->length();
}

/*!
    \fn bool QDomNodeList::isEmpty() const

    Returns true if the list contains no items; otherwise returns false.
    This function is provided for Qt API consistency.
*/

/*!
    \fn int QDomNodeList::count() const

    This function is provided for Qt API consistency. It is equivalent to length().
*/

/*!
    \fn int QDomNodeList::size() const

    This function is provided for Qt API consistency. It is equivalent to length().
*/

/*!
    \fn QDomNode QDomNodeList::at(int index) const

    This function is provided for Qt API consistency. It is equivalent
    to item().

    If \a index is negative or if \a index >= length() then a null
    node is returned (i.e. a node for which QDomNode::isNull() returns
    true).
*/

/**************************************************************
 *
 * QDomNodePrivate
 *
 **************************************************************/

inline void QDomNodePrivate::setOwnerDocument(QDomDocumentPrivate *doc)
{
    ownerNode = doc;
    hasParent = false;
}

QDomNodePrivate::QDomNodePrivate(QDomDocumentPrivate *doc, QDomNodePrivate *par)
{
    ref = 1;
    if (par)
        setParent(par);
    else
        setOwnerDocument(doc);
    prev = 0;
    next = 0;
    first = 0;
    last = 0;
    createdWithDom1Interface = true;
    lineNumber = -1;
    columnNumber = -1;
}

QDomNodePrivate::QDomNodePrivate(QDomNodePrivate *n, bool deep)
{
    ref = 1;
    setOwnerDocument(n->ownerDocument());
    prev = 0;
    next = 0;
    first = 0;
    last = 0;

    name = n->name;
    value = n->value;
    prefix = n->prefix;
    namespaceURI = n->namespaceURI;
    createdWithDom1Interface = n->createdWithDom1Interface;
    lineNumber = -1;
    columnNumber = -1;

    if (!deep)
        return;

    for (QDomNodePrivate* x = n->first; x; x = x->next)
        appendChild(x->cloneNode(true));
}

QDomNodePrivate::~QDomNodePrivate()
{
    QDomNodePrivate* p = first;
    QDomNodePrivate* n;

    while (p) {
        n = p->next;
        if (!p->ref.deref())
            delete p;
        else
            p->setNoParent();
        p = n;
    }
    first = 0;
    last = 0;
}

void QDomNodePrivate::clear()
{
    QDomNodePrivate* p = first;
    QDomNodePrivate* n;

    while (p) {
        n = p->next;
        if (!p->ref.deref())
            delete p;
        p = n;
    }
    first = 0;
    last = 0;
}

QDomNodePrivate* QDomNodePrivate::namedItem(const QString &n)
{
    QDomNodePrivate* p = first;
    while (p) {
        if (p->nodeName() == n)
            return p;
        p = p->next;
    }
    return 0;
}


QDomNodePrivate* QDomNodePrivate::insertBefore(QDomNodePrivate* newChild, QDomNodePrivate* refChild)
{
    // Error check
    if (!newChild)
        return 0;

    // Error check
    if (newChild == refChild)
        return 0;

    // Error check
    if (refChild && refChild->parent() != this)
        return 0;

    // "mark lists as dirty"
    QDomDocumentPrivate *const doc = ownerDocument();
    if(doc)
        doc->nodeListTime++;

    // Special handling for inserting a fragment. We just insert
    // all elements of the fragment instead of the fragment itself.
    if (newChild->isDocumentFragment()) {
        // Fragment is empty ?
        if (newChild->first == 0)
            return newChild;

        // New parent
        QDomNodePrivate* n = newChild->first;
        while (n)  {
            n->setParent(this);
            n = n->next;
        }

        // Insert at the beginning ?
        if (!refChild || refChild->prev == 0) {
            if (first)
                first->prev = newChild->last;
            newChild->last->next = first;
            if (!last)
                last = newChild->last;
            first = newChild->first;
        } else {
            // Insert in the middle
            newChild->last->next = refChild;
            newChild->first->prev = refChild->prev;
            refChild->prev->next = newChild->first;
            refChild->prev = newChild->last;
        }

        // No need to increase the reference since QDomDocumentFragment
        // does not decrease the reference.

        // Remove the nodes from the fragment
        newChild->first = 0;
        newChild->last = 0;
        return newChild;
    }

    // No more errors can occur now, so we take
    // ownership of the node.
    newChild->ref.ref();

    if (newChild->parent())
        newChild->parent()->removeChild(newChild);

    newChild->setParent(this);

    if (!refChild) {
        if (first)
            first->prev = newChild;
        newChild->next = first;
        if (!last)
            last = newChild;
        first = newChild;
        return newChild;
    }

    if (refChild->prev == 0) {
        if (first)
            first->prev = newChild;
        newChild->next = first;
        if (!last)
            last = newChild;
        first = newChild;
        return newChild;
    }

    newChild->next = refChild;
    newChild->prev = refChild->prev;
    refChild->prev->next = newChild;
    refChild->prev = newChild;

    return newChild;
}

QDomNodePrivate* QDomNodePrivate::insertAfter(QDomNodePrivate* newChild, QDomNodePrivate* refChild)
{
    // Error check
    if (!newChild)
        return 0;

    // Error check
    if (newChild == refChild)
        return 0;

    // Error check
    if (refChild && refChild->parent() != this)
        return 0;

    // "mark lists as dirty"
    QDomDocumentPrivate *const doc = ownerDocument();
    if(doc)
        doc->nodeListTime++;

    // Special handling for inserting a fragment. We just insert
    // all elements of the fragment instead of the fragment itself.
    if (newChild->isDocumentFragment()) {
        // Fragment is empty ?
        if (newChild->first == 0)
            return newChild;

        // New parent
        QDomNodePrivate* n = newChild->first;
        while (n) {
            n->setParent(this);
            n = n->next;
        }

        // Insert at the end
        if (!refChild || refChild->next == 0) {
            if (last)
                last->next = newChild->first;
            newChild->first->prev = last;
            if (!first)
                first = newChild->first;
            last = newChild->last;
        } else { // Insert in the middle
            newChild->first->prev = refChild;
            newChild->last->next = refChild->next;
            refChild->next->prev = newChild->last;
            refChild->next = newChild->first;
        }

        // No need to increase the reference since QDomDocumentFragment
        // does not decrease the reference.

        // Remove the nodes from the fragment
        newChild->first = 0;
        newChild->last = 0;
        return newChild;
    }

    // Release new node from its current parent
    if (newChild->parent())
        newChild->parent()->removeChild(newChild);

    // No more errors can occur now, so we take
    // ownership of the node
    newChild->ref.ref();

    newChild->setParent(this);

    // Insert at the end
    if (!refChild) {
        if (last)
            last->next = newChild;
        newChild->prev = last;
        if (!first)
            first = newChild;
        last = newChild;
        return newChild;
    }

    if (refChild->next == 0) {
        if (last)
            last->next = newChild;
        newChild->prev = last;
        if (!first)
            first = newChild;
        last = newChild;
        return newChild;
    }

    newChild->prev = refChild;
    newChild->next = refChild->next;
    refChild->next->prev = newChild;
    refChild->next = newChild;

    return newChild;
}

QDomNodePrivate* QDomNodePrivate::replaceChild(QDomNodePrivate* newChild, QDomNodePrivate* oldChild)
{
    if (!newChild || !oldChild)
        return 0;
    if (oldChild->parent() != this)
        return 0;
    if (newChild == oldChild)
        return 0;

    // mark lists as dirty
    QDomDocumentPrivate *const doc = ownerDocument();
    if(doc)
        doc->nodeListTime++;

    // Special handling for inserting a fragment. We just insert
    // all elements of the fragment instead of the fragment itself.
    if (newChild->isDocumentFragment()) {
        // Fragment is empty ?
        if (newChild->first == 0)
            return newChild;

        // New parent
        QDomNodePrivate* n = newChild->first;
        while (n) {
            n->setParent(this);
            n = n->next;
        }


        if (oldChild->next)
            oldChild->next->prev = newChild->last;
        if (oldChild->prev)
            oldChild->prev->next = newChild->first;

        newChild->last->next = oldChild->next;
        newChild->first->prev = oldChild->prev;

        if (first == oldChild)
            first = newChild->first;
        if (last == oldChild)
            last = newChild->last;

        oldChild->setNoParent();
        oldChild->next = 0;
        oldChild->prev = 0;

        // No need to increase the reference since QDomDocumentFragment
        // does not decrease the reference.

        // Remove the nodes from the fragment
        newChild->first = 0;
        newChild->last = 0;

        // We are no longer interested in the old node
        if (oldChild)
            oldChild->ref.deref();

        return oldChild;
    }

    // No more errors can occur now, so we take
    // ownership of the node
    newChild->ref.ref();

    // Release new node from its current parent
    if (newChild->parent())
        newChild->parent()->removeChild(newChild);

    newChild->setParent(this);

    if (oldChild->next)
        oldChild->next->prev = newChild;
    if (oldChild->prev)
        oldChild->prev->next = newChild;

    newChild->next = oldChild->next;
    newChild->prev = oldChild->prev;

    if (first == oldChild)
        first = newChild;
    if (last == oldChild)
        last = newChild;

    oldChild->setNoParent();
    oldChild->next = 0;
    oldChild->prev = 0;

    // We are no longer interested in the old node
    if (oldChild)
        oldChild->ref.deref();

    return oldChild;
}

QDomNodePrivate* QDomNodePrivate::removeChild(QDomNodePrivate* oldChild)
{
    // Error check
    if (oldChild->parent() != this)
        return 0;

    // "mark lists as dirty"
    QDomDocumentPrivate *const doc = ownerDocument();
    if(doc)
        doc->nodeListTime++;

    // Perhaps oldChild was just created with "createElement" or that. In this case
    // its parent is QDomDocument but it is not part of the documents child list.
    if (oldChild->next == 0 && oldChild->prev == 0 && first != oldChild)
        return 0;

    if (oldChild->next)
        oldChild->next->prev = oldChild->prev;
    if (oldChild->prev)
        oldChild->prev->next = oldChild->next;

    if (last == oldChild)
        last = oldChild->prev;
    if (first == oldChild)
        first = oldChild->next;

    oldChild->setNoParent();
    oldChild->next = 0;
    oldChild->prev = 0;

    // We are no longer interested in the old node
    oldChild->ref.deref();

    return oldChild;
}

QDomNodePrivate* QDomNodePrivate::appendChild(QDomNodePrivate* newChild)
{
    // No reference manipulation needed. Done in insertAfter.
    return insertAfter(newChild, 0);
}

QDomDocumentPrivate* QDomNodePrivate::ownerDocument()
{
    QDomNodePrivate* p = this;
    while (p && !p->isDocument()) {
        if (!p->hasParent)
            return (QDomDocumentPrivate*)p->ownerNode;
        p = p->parent();
    }

    return static_cast<QDomDocumentPrivate *>(p);
}

QDomNodePrivate* QDomNodePrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomNodePrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

static void qNormalizeNode(QDomNodePrivate* n)
{
    QDomNodePrivate* p = n->first;
    QDomTextPrivate* t = 0;

    while (p) {
        if (p->isText()) {
            if (t) {
                QDomNodePrivate* tmp = p->next;
                t->appendData(p->nodeValue());
                n->removeChild(p);
                p = tmp;
            } else {
                t = (QDomTextPrivate*)p;
                p = p->next;
            }
        } else {
            p = p->next;
            t = 0;
        }
    }
}
void QDomNodePrivate::normalize()
{
    // ### This one has moved from QDomElementPrivate to this position. It is
    // not tested.
    qNormalizeNode(this);
}

/*! \internal
  \a depth is used for indentation, it seems.
 */
void QDomNodePrivate::save(QTextStream& s, int depth, int indent) const
{
    const QDomNodePrivate* n = first;
    while (n) {
        n->save(s, depth, indent);
        n = n->next;
    }
}

void QDomNodePrivate::setLocation(int lineNumber, int columnNumber)
{
    this->lineNumber = lineNumber;
    this->columnNumber = columnNumber;
}

/**************************************************************
 *
 * QDomNode
 *
 **************************************************************/

#define IMPL ((QDomNodePrivate*)impl)

/*!
    \class QDomNode
    \reentrant
    \brief The QDomNode class is the base class for all the nodes in a DOM tree.

    \inmodule QtXml
    \ingroup xml-tools


    Many functions in the DOM return a QDomNode.

    You can find out the type of a node using isAttr(),
    isCDATASection(), isDocumentFragment(), isDocument(),
    isDocumentType(), isElement(), isEntityReference(), isText(),
    isEntity(), isNotation(), isProcessingInstruction(),
    isCharacterData() and isComment().

    A QDomNode can be converted into one of its subclasses using
    toAttr(), toCDATASection(), toDocumentFragment(), toDocument(),
    toDocumentType(), toElement(), toEntityReference(), toText(),
    toEntity(), toNotation(), toProcessingInstruction(),
    toCharacterData() or toComment(). You can convert a node to a null
    node with clear().

    Copies of the QDomNode class share their data using explicit
    sharing. This means that modifying one node will change all
    copies. This is especially useful in combination with functions
    which return a QDomNode, e.g. firstChild(). You can make an
    independent (deep) copy of the node with cloneNode().

    A QDomNode can be null, much like a null pointer. Creating a copy
    of a null node results in another null node. It is not
    possible to modify a null node, but it is possible to assign another,
    possibly non-null node to it. In this case, the copy of the null node
    will remain null. You can check if a QDomNode is null by calling isNull().
    The empty constructor of a QDomNode (or any of the derived classes) creates
    a null node.

    Nodes are inserted with insertBefore(), insertAfter() or
    appendChild(). You can replace one node with another using
    replaceChild() and remove a node with removeChild().

    To traverse nodes use firstChild() to get a node's first child (if
    any), and nextSibling() to traverse. QDomNode also provides
    lastChild(), previousSibling() and parentNode(). To find the first
    child node with a particular node name use namedItem().

    To find out if a node has children use hasChildNodes() and to get
    a list of all of a node's children use childNodes().

    The node's name and value (the meaning of which varies depending
    on its type) is returned by nodeName() and nodeValue()
    respectively. The node's type is returned by nodeType(). The
    node's value can be set with setNodeValue().

    The document to which the node belongs is returned by
    ownerDocument().

    Adjacent QDomText nodes can be merged into a single node with
    normalize().

    \l QDomElement nodes have attributes which can be retrieved with
    attributes().

    QDomElement and QDomAttr nodes can have namespaces which can be
    retrieved with namespaceURI(). Their local name is retrieved with
    localName(), and their prefix with prefix(). The prefix can be set
    with setPrefix().

    You can write the XML representation of the node to a text stream
    with save().

    The following example looks for the first element in an XML document and
    prints the names of all the elements that are its direct children.
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 1

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs a \link isNull() null\endlink node.
*/
QDomNode::QDomNode()
{
    impl = 0;
}

/*!
    Constructs a copy of \a n.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomNode::QDomNode(const QDomNode &n)
{
    impl = n.impl;
    if (impl)
        impl->ref.ref();
}

/*!  \internal
  Constructs a new node for the data \a n.
*/
QDomNode::QDomNode(QDomNodePrivate *n)
{
    impl = n;
    if (impl)
        impl->ref.ref();
}

/*!
    Assigns a copy of \a n to this DOM node.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomNode& QDomNode::operator=(const QDomNode &n)
{
    if (n.impl)
        n.impl->ref.ref();
    if (impl && !impl->ref.deref())
        delete impl;
    impl = n.impl;
    return *this;
}

/*!
    Returns true if \a n and this DOM node are equal; otherwise
    returns false.

    Any instance of QDomNode acts as a reference to an underlying data
    structure in QDomDocument. The test for equality checks if the two
    references point to the same underlying node. For example:

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 2

    The two nodes (QDomElement is a QDomNode subclass) both refer to
    the document's root element, and \c {element1 == element2} will
    return true. On the other hand:

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 3

    Even though both nodes are empty elements carrying the same name,
    \c {element3 == element4} will return false because they refer to
    two different nodes in the underlying data structure.
*/
bool QDomNode::operator== (const QDomNode& n) const
{
    return (impl == n.impl);
}

/*!
    Returns true if \a n and this DOM node are not equal; otherwise
    returns false.
*/
bool QDomNode::operator!= (const QDomNode& n) const
{
    return (impl != n.impl);
}

/*!
    Destroys the object and frees its resources.
*/
QDomNode::~QDomNode()
{
    if (impl && !impl->ref.deref())
        delete impl;
}

/*!
    Returns the name of the node.

    The meaning of the name depends on the subclass:

    \table
    \header \i Name \i Meaning
    \row \i QDomAttr \i The name of the attribute
    \row \i QDomCDATASection \i The string "#cdata-section"
    \row \i QDomComment \i The string "#comment"
    \row \i QDomDocument \i The string "#document"
    \row \i QDomDocumentFragment \i The string "#document-fragment"
    \row \i QDomDocumentType \i The name of the document type
    \row \i QDomElement \i The tag name
    \row \i QDomEntity \i The name of the entity
    \row \i QDomEntityReference \i The name of the referenced entity
    \row \i QDomNotation \i The name of the notation
    \row \i QDomProcessingInstruction \i The target of the processing instruction
    \row \i QDomText \i The string "#text"
    \endtable

    \bold{Note:} This function does not take the presence of namespaces into account
    when processing the names of element and attribute nodes. As a result, the
    returned name can contain any namespace prefix that may be present.
    To obtain the node name of an element or attribute, use localName(); to
    obtain the namespace prefix, use namespaceURI().

    \sa nodeValue()
*/
QString QDomNode::nodeName() const
{
    if (!impl)
        return QString();

    if (!IMPL->prefix.isEmpty())
        return IMPL->prefix + QLatin1Char(':') + IMPL->name;
    return IMPL->name;
}

/*!
    Returns the value of the node.

    The meaning of the value depends on the subclass:
    \table
    \header \i Name \i Meaning
    \row \i QDomAttr \i The attribute value
    \row \i QDomCDATASection \i The content of the CDATA section
    \row \i QDomComment \i The comment
    \row \i QDomProcessingInstruction \i The data of the processing instruction
    \row \i QDomText \i The text
    \endtable

    All the other subclasses do not have a node value and will return
    an empty string.

    \sa setNodeValue() nodeName()
*/
QString QDomNode::nodeValue() const
{
    if (!impl)
        return QString();
    return IMPL->value;
}

/*!
    Sets the node's value to \a v.

    \sa nodeValue()
*/
void QDomNode::setNodeValue(const QString& v)
{
    if (!impl)
        return;
    IMPL->setNodeValue(v);
}

/*!
    \enum QDomNode::NodeType

    This enum defines the type of the node:
    \value ElementNode
    \value AttributeNode
    \value TextNode
    \value CDATASectionNode
    \value EntityReferenceNode
    \value EntityNode
    \value ProcessingInstructionNode
    \value CommentNode
    \value DocumentNode
    \value DocumentTypeNode
    \value DocumentFragmentNode
    \value NotationNode
    \value BaseNode  A QDomNode object, i.e. not a QDomNode subclass.
    \value CharacterDataNode
*/

/*!
    Returns the type of the node.

    \sa toAttr(), toCDATASection(), toDocumentFragment(),
    toDocument() toDocumentType(), toElement(), toEntityReference(),
    toText(), toEntity() toNotation(), toProcessingInstruction(),
    toCharacterData(), toComment()
*/
QDomNode::NodeType QDomNode::nodeType() const
{
    if (!impl)
        return QDomNode::BaseNode;
    return IMPL->nodeType();
}

/*!
    Returns the parent node. If this node has no parent, a null node
    is returned (i.e. a node for which isNull() returns true).
*/
QDomNode QDomNode::parentNode() const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->parent());
}

/*!
    Returns a list of all direct child nodes.

    Most often you will call this function on a QDomElement object.

    For example, if the XML document looks like this:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 4
    Then the list of child nodes for the "body"-element will contain
    the node created by the &lt;h1&gt; tag and the node created by the
    &lt;p&gt; tag.

    The nodes in the list are not copied; so changing the nodes in the
    list will also change the children of this node.

    \sa firstChild() lastChild()
*/
QDomNodeList QDomNode::childNodes() const
{
    if (!impl)
        return QDomNodeList();
    return QDomNodeList(new QDomNodeListPrivate(impl));
}

/*!
    Returns the first child of the node. If there is no child node, a
    \link isNull() null node\endlink is returned. Changing the
    returned node will also change the node in the document tree.

    \sa lastChild() childNodes()
*/
QDomNode QDomNode::firstChild() const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->first);
}

/*!
    Returns the last child of the node. If there is no child node, a
    \link isNull() null node\endlink is returned. Changing the
    returned node will also change the node in the document tree.

    \sa firstChild() childNodes()
*/
QDomNode QDomNode::lastChild() const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->last);
}

/*!
    Returns the previous sibling in the document tree. Changing the
    returned node will also change the node in the document tree.

    For example, if you have XML like this:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 5
    and this QDomNode represents the &lt;p&gt; tag, previousSibling()
    will return the node representing the &lt;h1&gt; tag.

    \sa nextSibling()
*/
QDomNode QDomNode::previousSibling() const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->prev);
}

/*!
    Returns the next sibling in the document tree. Changing the
    returned node will also change the node in the document tree.

    If you have XML like this:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 6
    and this QDomNode represents the <p> tag, nextSibling() will
    return the node representing the <h2> tag.

    \sa previousSibling()
*/
QDomNode QDomNode::nextSibling() const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->next);
}


// ###### don't think this is part of the DOM and
/*!
    Returns a named node map of all attributes. Attributes are only
    provided for \l{QDomElement}s.

    Changing the attributes in the map will also change the attributes
    of this QDomNode.
*/
QDomNamedNodeMap QDomNode::attributes() const
{
    if (!impl || !impl->isElement())
        return QDomNamedNodeMap();

    return QDomNamedNodeMap(static_cast<QDomElementPrivate *>(impl)->attributes());
}

/*!
    Returns the document to which this node belongs.
*/
QDomDocument QDomNode::ownerDocument() const
{
    if (!impl)
        return QDomDocument();
    return QDomDocument(IMPL->ownerDocument());
}

/*!
    Creates a deep (not shallow) copy of the QDomNode.

    If \a deep is true, then the cloning is done recursively which
    means that all the node's children are deep copied too. If \a deep
    is false only the node itself is copied and the copy will have no
    child nodes.
*/
QDomNode QDomNode::cloneNode(bool deep) const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->cloneNode(deep));
}

/*!
    Calling normalize() on an element converts all its children into a
    standard form. This means that adjacent QDomText objects will be
    merged into a single text object (QDomCDATASection nodes are not
    merged).
*/
void QDomNode::normalize()
{
    if (!impl)
        return;
    IMPL->normalize();
}

/*!
    Returns true if the DOM implementation implements the feature \a
    feature and this feature is supported by this node in the version
    \a version; otherwise returns false.

    \sa QDomImplementation::hasFeature()
*/
bool QDomNode::isSupported(const QString& feature, const QString& version) const
{
    QDomImplementation i;
    return i.hasFeature(feature, version);
}

/*!
    Returns the namespace URI of this node or an empty string if the
    node has no namespace URI.

    Only nodes of type \link QDomNode::NodeType ElementNode\endlink or
    \link QDomNode::NodeType AttributeNode\endlink can have
    namespaces. A namespace URI must be specified at creation time and
    cannot be changed later.

    \sa prefix() localName() QDomDocument::createElementNS()
    QDomDocument::createAttributeNS()
*/
QString QDomNode::namespaceURI() const
{
    if (!impl)
        return QString();
    return IMPL->namespaceURI;
}

/*!
    Returns the namespace prefix of the node or an empty string if the
    node has no namespace prefix.

    Only nodes of type \link QDomNode::NodeType ElementNode\endlink or
    \link QDomNode::NodeType AttributeNode\endlink can have
    namespaces. A namespace prefix must be specified at creation time.
    If a node was created with a namespace prefix, you can change it
    later with setPrefix().

    If you create an element or attribute with
    QDomDocument::createElement() or QDomDocument::createAttribute(),
    the prefix will be an empty string. If you use
    QDomDocument::createElementNS() or
    QDomDocument::createAttributeNS() instead, the prefix will not be
    an empty string; but it might be an empty string if the name does
    not have a prefix.

    \sa setPrefix() localName() namespaceURI()
    QDomDocument::createElementNS() QDomDocument::createAttributeNS()
*/
QString QDomNode::prefix() const
{
    if (!impl)
        return QString();
    return IMPL->prefix;
}

/*!
    If the node has a namespace prefix, this function changes the
    namespace prefix of the node to \a pre. Otherwise this function
    does nothing.

    Only nodes of type \link QDomNode::NodeType ElementNode\endlink or
    \link QDomNode::NodeType AttributeNode\endlink can have
    namespaces. A namespace prefix must have be specified at creation
    time; it is not possible to add a namespace prefix afterwards.

    \sa prefix() localName() namespaceURI()
    QDomDocument::createElementNS() QDomDocument::createAttributeNS()
*/
void QDomNode::setPrefix(const QString& pre)
{
    if (!impl || IMPL->prefix.isNull())
        return;
    if (isAttr() || isElement())
        IMPL->prefix = pre;
}

/*!
    If the node uses namespaces, this function returns the local name
    of the node; otherwise it returns an empty string.

    Only nodes of type \link QDomNode::NodeType ElementNode\endlink or
    \link QDomNode::NodeType AttributeNode\endlink can have
    namespaces. A namespace must have been specified at creation time;
    it is not possible to add a namespace afterwards.

    \sa prefix() namespaceURI() QDomDocument::createElementNS()
    QDomDocument::createAttributeNS()
*/
QString QDomNode::localName() const
{
    if (!impl || IMPL->createdWithDom1Interface)
        return QString();
    return IMPL->name;
}

/*!
    Returns true if the node has attributes; otherwise returns false.

    \sa attributes()
*/
bool QDomNode::hasAttributes() const
{
    if (!impl || !impl->isElement())
        return false;
    return static_cast<QDomElementPrivate *>(impl)->hasAttributes();
}

/*!
    Inserts the node \a newChild before the child node \a refChild.
    \a refChild must be a direct child of this node. If \a refChild is
    \link isNull() null\endlink then \a newChild is inserted as the
    node's first child.

    If \a newChild is the child of another node, it is reparented to
    this node. If \a newChild is a child of this node, then its
    position in the list of children is changed.

    If \a newChild is a QDomDocumentFragment, then the children of the
    fragment are removed from the fragment and inserted before \a
    refChild.

    Returns a new reference to \a newChild on success or a \link
    isNull() null node\endlink on failure.

    The DOM specification disallow inserting attribute nodes, but due
    to historical reasons QDom accept them nevertheless.

    \sa insertAfter() replaceChild() removeChild() appendChild()
*/
QDomNode QDomNode::insertBefore(const QDomNode& newChild, const QDomNode& refChild)
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->insertBefore(newChild.impl, refChild.impl));
}

/*!
    Inserts the node \a newChild after the child node \a refChild. \a
    refChild must be a direct child of this node. If \a refChild is
    \link isNull() null\endlink then \a newChild is appended as this
    node's last child.

    If \a newChild is the child of another node, it is reparented to
    this node. If \a newChild is a child of this node, then its
    position in the list of children is changed.

    If \a newChild is a QDomDocumentFragment, then the children of the
    fragment are removed from the fragment and inserted after \a
    refChild.

    Returns a new reference to \a newChild on success or a \link
    isNull() null node\endlink on failure.

    The DOM specification disallow inserting attribute nodes, but due
    to historical reasons QDom accept them nevertheless.

    \sa insertBefore() replaceChild() removeChild() appendChild()
*/
QDomNode QDomNode::insertAfter(const QDomNode& newChild, const QDomNode& refChild)
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->insertAfter(newChild.impl, refChild.impl));
}

/*!
    Replaces \a oldChild with \a newChild. \a oldChild must be a
    direct child of this node.

    If \a newChild is the child of another node, it is reparented to
    this node. If \a newChild is a child of this node, then its
    position in the list of children is changed.

    If \a newChild is a QDomDocumentFragment, then \a oldChild is
    replaced by all of the children of the fragment.

    Returns a new reference to \a oldChild on success or a \link
    isNull() null node\endlink an failure.

    \sa insertBefore() insertAfter() removeChild() appendChild()
*/
QDomNode QDomNode::replaceChild(const QDomNode& newChild, const QDomNode& oldChild)
{
    if (!impl || !newChild.impl || !oldChild.impl)
        return QDomNode();
    return QDomNode(IMPL->replaceChild(newChild.impl, oldChild.impl));
}

/*!
    Removes \a oldChild from the list of children. \a oldChild must be
    a direct child of this node.

    Returns a new reference to \a oldChild on success or a \link
    isNull() null node\endlink on failure.

    \sa insertBefore() insertAfter() replaceChild() appendChild()
*/
QDomNode QDomNode::removeChild(const QDomNode& oldChild)
{
    if (!impl)
        return QDomNode();

    if (oldChild.isNull())
        return QDomNode();

    return QDomNode(IMPL->removeChild(oldChild.impl));
}

/*!
    Appends \a newChild as the node's last child.

    If \a newChild is the child of another node, it is reparented to
    this node. If \a newChild is a child of this node, then its
    position in the list of children is changed.

    If \a newChild is a QDomDocumentFragment, then the children of the
    fragment are removed from the fragment and appended.

    If \a newChild is a QDomElement and this node is a QDomDocument that
    already has an element node as a child, \a newChild is not added as
    a child and a null node is returned.

    Returns a new reference to \a newChild on success or a \link
    isNull() null node\endlink on failure.

    Calling this function on a null node(created, for example, with
    the default constructor) does nothing and returns a \link isNull()
    null node\endlink.

    The DOM specification disallow inserting attribute nodes, but for
    historical reasons, QDom accepts them anyway.

    \sa insertBefore() insertAfter() replaceChild() removeChild()
*/
QDomNode QDomNode::appendChild(const QDomNode& newChild)
{
    if (!impl) {
        qWarning("Calling appendChild() on a null node does nothing.");
        return QDomNode();
    }
    return QDomNode(IMPL->appendChild(newChild.impl));
}

/*!
    Returns true if the node has one or more children; otherwise
    returns false.
*/
bool QDomNode::hasChildNodes() const
{
    if (!impl)
        return false;
    return IMPL->first != 0;
}

/*!
    Returns true if this node is null (i.e. if it has no type or
    contents); otherwise returns false.
*/
bool QDomNode::isNull() const
{
    return (impl == 0);
}

/*!
    Converts the node into a null node; if it was not a null node
    before, its type and contents are deleted.

    \sa isNull()
*/
void QDomNode::clear()
{
    if (impl && !impl->ref.deref())
        delete impl;
    impl = 0;
}

/*!
    Returns the first direct child node for which nodeName() equals \a
    name.

    If no such direct child exists, a \link isNull() null node\endlink
    is returned.

    \sa nodeName()
*/
QDomNode QDomNode::namedItem(const QString& name) const
{
    if (!impl)
        return QDomNode();
    return QDomNode(impl->namedItem(name));
}

/*!
    Writes the XML representation of the node and all its children to
    the stream \a str. This function uses \a indent as the amount of
    space to indent the node.

    If this node is a document node, the encoding of text stream \a str's encoding is
    set by treating a processing instruction by name "xml" as an XML declaration, if such a one exists,
    and otherwise defaults to UTF-8. XML declarations are not processing instructions, but this
    behavior exists for historical reasons. If this node is not a document node,
    the text stream's encoding is used.

    If the document contains invalid XML characters or characters that cannot be
    encoded in the given encoding, the result and behavior is undefined.

*/
void QDomNode::save(QTextStream& str, int indent) const
{
    save(str, indent, QDomNode::EncodingFromDocument);
}

/*!
    If \a encodingPolicy is QDomNode::EncodingFromDocument, this function behaves as save(QTextStream &str, int indent).

    If \a encodingPolicy is EncodingFromTextStream and this node is a document node, this
    function behaves as save(QTextStream &str, int indent) with the exception that the encoding
    specified in the text stream \a str is used.

    If the document contains invalid XML characters or characters that cannot be
    encoded in the given encoding, the result and behavior is undefined.

    \since 4.2
 */
void QDomNode::save(QTextStream& str, int indent, EncodingPolicy encodingPolicy) const
{
    if (!impl)
        return;

    if(isDocument())
        static_cast<const QDomDocumentPrivate *>(impl)->saveDocument(str, indent, encodingPolicy);
    else
        IMPL->save(str, 1, indent);
}

/*!
    \relates QDomNode

    Writes the XML representation of the node \a node and all its
    children to the stream \a str.
*/
QTextStream& operator<<(QTextStream& str, const QDomNode& node)
{
    node.save(str, 1);

    return str;
}

/*!
    Returns true if the node is an attribute; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomAttribute; you can get the QDomAttribute with
    toAttribute().

    \sa toAttr()
*/
bool QDomNode::isAttr() const
{
    if(impl)
        return impl->isAttr();
    return false;
}

/*!
    Returns true if the node is a CDATA section; otherwise returns
    false.

    If this function returns true, it does not imply that this object
    is a QDomCDATASection; you can get the QDomCDATASection with
    toCDATASection().

    \sa toCDATASection()
*/
bool QDomNode::isCDATASection() const
{
    if(impl)
        return impl->isCDATASection();
    return false;
}

/*!
    Returns true if the node is a document fragment; otherwise returns
    false.

    If this function returns true, it does not imply that this object
    is a QDomDocumentFragment; you can get the QDomDocumentFragment
    with toDocumentFragment().

    \sa toDocumentFragment()
*/
bool QDomNode::isDocumentFragment() const
{
    if(impl)
        return impl->isDocumentFragment();
    return false;
}

/*!
    Returns true if the node is a document; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomDocument; you can get the QDomDocument with toDocument().

    \sa toDocument()
*/
bool QDomNode::isDocument() const
{
    if(impl)
        return impl->isDocument();
    return false;
}

/*!
    Returns true if the node is a document type; otherwise returns
    false.

    If this function returns true, it does not imply that this object
    is a QDomDocumentType; you can get the QDomDocumentType with
    toDocumentType().

    \sa toDocumentType()
*/
bool QDomNode::isDocumentType() const
{
    if(impl)
        return impl->isDocumentType();
    return false;
}

/*!
    Returns true if the node is an element; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomElement; you can get the QDomElement with toElement().

    \sa toElement()
*/
bool QDomNode::isElement() const
{
    if(impl)
        return impl->isElement();
    return false;
}

/*!
    Returns true if the node is an entity reference; otherwise returns
    false.

    If this function returns true, it does not imply that this object
    is a QDomEntityReference; you can get the QDomEntityReference with
    toEntityReference().

    \sa toEntityReference()
*/
bool QDomNode::isEntityReference() const
{
    if(impl)
        return impl->isEntityReference();
    return false;
}

/*!
    Returns true if the node is a text node; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomText; you can get the QDomText with toText().

    \sa toText()
*/
bool QDomNode::isText() const
{
    if(impl)
        return impl->isText();
    return false;
}

/*!
    Returns true if the node is an entity; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomEntity; you can get the QDomEntity with toEntity().

    \sa toEntity()
*/
bool QDomNode::isEntity() const
{
    if(impl)
        return impl->isEntity();
    return false;
}

/*!
    Returns true if the node is a notation; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomNotation; you can get the QDomNotation with toNotation().

    \sa toNotation()
*/
bool QDomNode::isNotation() const
{
    if(impl)
        return impl->isNotation();
    return false;
}

/*!
    Returns true if the node is a processing instruction; otherwise
    returns false.

    If this function returns true, it does not imply that this object
    is a QDomProcessingInstruction; you can get the
    QProcessingInstruction with toProcessingInstruction().

    \sa toProcessingInstruction()
*/
bool QDomNode::isProcessingInstruction() const
{
    if(impl)
        return impl->isProcessingInstruction();
    return false;
}

/*!
    Returns true if the node is a character data node; otherwise
    returns false.

    If this function returns true, it does not imply that this object
    is a QDomCharacterData; you can get the QDomCharacterData with
    toCharacterData().

    \sa toCharacterData()
*/
bool QDomNode::isCharacterData() const
{
    if (impl)
        return impl->isCharacterData();
    return false;
}

/*!
    Returns true if the node is a comment; otherwise returns false.

    If this function returns true, it does not imply that this object
    is a QDomComment; you can get the QDomComment with toComment().

    \sa toComment()
*/
bool QDomNode::isComment() const
{
    if (impl)
        return impl->isComment();
    return false;
}

#undef IMPL

/*!
    Returns the first child element with tag name \a tagName if tagName is non-empty;
    otherwise returns the first child element.  Returns a null element if no
    such child exists.

    \sa lastChildElement() previousSiblingElement() nextSiblingElement()
*/

QDomElement QDomNode::firstChildElement(const QString &tagName) const
{
    for (QDomNode child = firstChild(); !child.isNull(); child = child.nextSibling()) {
        if (child.isElement()) {
            QDomElement elt = child.toElement();
            if (tagName.isEmpty() || elt.tagName() == tagName)
                return elt;
        }
    }
    return QDomElement();
}

/*!
    Returns the last child element with tag name \a tagName if tagName is non-empty;
    otherwise returns the last child element. Returns a null element if no
    such child exists.

    \sa firstChildElement() previousSiblingElement() nextSiblingElement()
*/

QDomElement QDomNode::lastChildElement(const QString &tagName) const
{
    for (QDomNode child = lastChild(); !child.isNull(); child = child.previousSibling()) {
        if (child.isElement()) {
            QDomElement elt = child.toElement();
            if (tagName.isEmpty() || elt.tagName() == tagName)
                return elt;
        }
    }
    return QDomElement();
}

/*!
    Returns the next sibling element with tag name \a tagName if \a tagName
    is non-empty; otherwise returns any next sibling element.
    Returns a null element if no such sibling exists.

    \sa firstChildElement() previousSiblingElement() lastChildElement()
*/

QDomElement QDomNode::nextSiblingElement(const QString &tagName) const
{
    for (QDomNode sib = nextSibling(); !sib.isNull(); sib = sib.nextSibling()) {
        if (sib.isElement()) {
            QDomElement elt = sib.toElement();
            if (tagName.isEmpty() || elt.tagName() == tagName)
                return elt;
        }
    }
    return QDomElement();
}

/*!
    Returns the previous sibilng element with tag name \a tagName if \a tagName
    is non-empty; otherwise returns any previous sibling element.
    Returns a null element if no such sibling exists.

    \sa firstChildElement(), nextSiblingElement(), lastChildElement()
*/

QDomElement QDomNode::previousSiblingElement(const QString &tagName) const
{
    for (QDomNode sib = previousSibling(); !sib.isNull(); sib = sib.previousSibling()) {
        if (sib.isElement()) {
            QDomElement elt = sib.toElement();
            if (tagName.isEmpty() || elt.tagName() == tagName)
                return elt;
        }
    }
    return QDomElement();
}

/*!
    \since 4.1

    For nodes created by QDomDocument::setContent(), this function
    returns the line number in the XML document where the node was parsed.
    Otherwise, -1 is returned.

    \sa columnNumber(), QDomDocument::setContent()
*/
int QDomNode::lineNumber() const
{
    return impl ? impl->lineNumber : -1;
}

/*!
    \since 4.1

    For nodes created by QDomDocument::setContent(), this function
    returns the column number in the XML document where the node was parsed.
    Otherwise, -1 is returned.

    \sa lineNumber(), QDomDocument::setContent()
*/
int QDomNode::columnNumber() const
{
    return impl ? impl->columnNumber : -1;
}


/**************************************************************
 *
 * QDomNamedNodeMapPrivate
 *
 **************************************************************/

QDomNamedNodeMapPrivate::QDomNamedNodeMapPrivate(QDomNodePrivate* n)
{
    ref = 1;
    readonly = false;
    parent = n;
    appendToParent = false;
}

QDomNamedNodeMapPrivate::~QDomNamedNodeMapPrivate()
{
    clearMap();
}

QDomNamedNodeMapPrivate* QDomNamedNodeMapPrivate::clone(QDomNodePrivate* p)
{
    QScopedPointer<QDomNamedNodeMapPrivate> m(new QDomNamedNodeMapPrivate(p));
    m->readonly = readonly;
    m->appendToParent = appendToParent;

    QHash<QString, QDomNodePrivate*>::const_iterator it = map.constBegin();
    for (; it != map.constEnd(); ++it) {
        QDomNodePrivate *new_node = (*it)->cloneNode();
        new_node->setParent(p);
        m->setNamedItem(new_node);
    }

    // we are no longer interested in ownership
    m->ref.deref();
    return m.take();
}

void QDomNamedNodeMapPrivate::clearMap()
{
    // Dereference all of our children if we took references
    if (!appendToParent) {
        QHash<QString, QDomNodePrivate *>::const_iterator it = map.constBegin();
        for (; it != map.constEnd(); ++it)
            if (!(*it)->ref.deref())
                delete *it;
    }
    map.clear();
}

QDomNodePrivate* QDomNamedNodeMapPrivate::namedItem(const QString& name) const
{
    QDomNodePrivate* p = map[name];
    return p;
}

QDomNodePrivate* QDomNamedNodeMapPrivate::namedItemNS(const QString& nsURI, const QString& localName) const
{
    QHash<QString, QDomNodePrivate *>::const_iterator it = map.constBegin();
    QDomNodePrivate *n;
    for (; it != map.constEnd(); ++it) {
        n = *it;
        if (!n->prefix.isNull()) {
            // node has a namespace
            if (n->namespaceURI == nsURI && n->name == localName)
                return n;
        }
    }
    return 0;
}

QDomNodePrivate* QDomNamedNodeMapPrivate::setNamedItem(QDomNodePrivate* arg)
{
    if (readonly || !arg)
        return 0;

    if (appendToParent)
        return parent->appendChild(arg);

    QDomNodePrivate *n = map.value(arg->nodeName());
    // We take a reference
    arg->ref.ref();
    map.insertMulti(arg->nodeName(), arg);
    return n;
}

QDomNodePrivate* QDomNamedNodeMapPrivate::setNamedItemNS(QDomNodePrivate* arg)
{
    if (readonly || !arg)
        return 0;

    if (appendToParent)
        return parent->appendChild(arg);

    if (!arg->prefix.isNull()) {
        // node has a namespace
        QDomNodePrivate *n = namedItemNS(arg->namespaceURI, arg->name);
        // We take a reference
        arg->ref.ref();
        map.insertMulti(arg->nodeName(), arg);
        return n;
    } else {
        // ### check the following code if it is ok
        return setNamedItem(arg);
    }
}

QDomNodePrivate* QDomNamedNodeMapPrivate::removeNamedItem(const QString& name)
{
    if (readonly)
        return 0;

    QDomNodePrivate* p = namedItem(name);
    if (p == 0)
        return 0;
    if (appendToParent)
        return parent->removeChild(p);

    map.remove(p->nodeName());
    // We took a reference, so we have to free one here
    p->ref.deref();
    return p;
}

QDomNodePrivate* QDomNamedNodeMapPrivate::item(int index) const
{
    if ((uint)index >= length())
        return 0;
    return *(map.constBegin() + index);
}

// ### Qt 5: convert all length/size() functions in QDom to use int instead of uint.
uint QDomNamedNodeMapPrivate::length() const
{
    return map.count();
}

bool QDomNamedNodeMapPrivate::contains(const QString& name) const
{
    return map.value(name) != 0;
}

bool QDomNamedNodeMapPrivate::containsNS(const QString& nsURI, const QString & localName) const
{
    return namedItemNS(nsURI, localName) != 0;
}

/**************************************************************
 *
 * QDomNamedNodeMap
 *
 **************************************************************/

#define IMPL ((QDomNamedNodeMapPrivate*)impl)

/*!
    \class QDomNamedNodeMap
    \reentrant
    \brief The QDomNamedNodeMap class contains a collection of nodes
    that can be accessed by name.

    \inmodule QtXml
    \ingroup xml-tools

    Note that QDomNamedNodeMap does not inherit from QDomNodeList.
    QDomNamedNodeMaps do not provide any specific node ordering.
    Although nodes in a QDomNamedNodeMap may be accessed by an ordinal
    index, this is simply to allow a convenient enumeration of the
    contents of a QDomNamedNodeMap, and does not imply that the DOM
    specifies an ordering of the nodes.

    The QDomNamedNodeMap is used in three places:
    \list 1
    \i QDomDocumentType::entities() returns a map of all entities
        described in the DTD.
    \i QDomDocumentType::notations() returns a map of all notations
        described in the DTD.
    \i QDomNode::attributes() returns a map of all attributes of an
        element.
    \endlist

    Items in the map are identified by the name which QDomNode::name()
    returns. Nodes are retrieved using namedItem(), namedItemNS() or
    item(). New nodes are inserted with setNamedItem() or
    setNamedItemNS() and removed with removeNamedItem() or
    removeNamedItemNS(). Use contains() to see if an item with the
    given name is in the named node map. The number of items is
    returned by length().

    Terminology: in this class we use "item" and "node"
    interchangeably.
*/

/*!
    Constructs an empty named node map.
*/
QDomNamedNodeMap::QDomNamedNodeMap()
{
    impl = 0;
}

/*!
    Constructs a copy of \a n.
*/
QDomNamedNodeMap::QDomNamedNodeMap(const QDomNamedNodeMap &n)
{
    impl = n.impl;
    if (impl)
        impl->ref.ref();
}

QDomNamedNodeMap::QDomNamedNodeMap(QDomNamedNodeMapPrivate *n)
{
    impl = n;
    if (impl)
        impl->ref.ref();
}

/*!
    Assigns \a n to this named node map.
*/
QDomNamedNodeMap& QDomNamedNodeMap::operator=(const QDomNamedNodeMap &n)
{
    if (n.impl)
        n.impl->ref.ref();
    if (impl && !impl->ref.deref())
        delete impl;
    impl = n.impl;
    return *this;
}

/*!
    Returns true if \a n and this named node map are equal; otherwise
    returns false.
*/
bool QDomNamedNodeMap::operator== (const QDomNamedNodeMap& n) const
{
    return (impl == n.impl);
}

/*!
    Returns true if \a n and this named node map are not equal;
    otherwise returns false.
*/
bool QDomNamedNodeMap::operator!= (const QDomNamedNodeMap& n) const
{
    return (impl != n.impl);
}

/*!
    Destroys the object and frees its resources.
*/
QDomNamedNodeMap::~QDomNamedNodeMap()
{
    if (impl && !impl->ref.deref())
        delete impl;
}

/*!
    Returns the node called \a name.

    If the named node map does not contain such a node, a \link
    QDomNode::isNull() null node\endlink is returned. A node's name is
    the name returned by QDomNode::nodeName().

    \sa setNamedItem() namedItemNS()
*/
QDomNode QDomNamedNodeMap::namedItem(const QString& name) const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->namedItem(name));
}

/*!
    Inserts the node \a newNode into the named node map. The name used
    by the map is the node name of \a newNode as returned by
    QDomNode::nodeName().

    If the new node replaces an existing node, i.e. the map contains a
    node with the same name, the replaced node is returned.

    \sa namedItem() removeNamedItem() setNamedItemNS()
*/
QDomNode QDomNamedNodeMap::setNamedItem(const QDomNode& newNode)
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->setNamedItem((QDomNodePrivate*)newNode.impl));
}

/*!
    Removes the node called \a name from the map.

    The function returns the removed node or a \link
    QDomNode::isNull() null node\endlink if the map did not contain a
    node called \a name.

    \sa setNamedItem() namedItem() removeNamedItemNS()
*/
QDomNode QDomNamedNodeMap::removeNamedItem(const QString& name)
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->removeNamedItem(name));
}

/*!
    Retrieves the node at position \a index.

    This can be used to iterate over the map. Note that the nodes in
    the map are ordered arbitrarily.

    \sa length()
*/
QDomNode QDomNamedNodeMap::item(int index) const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->item(index));
}

/*!
    Returns the node associated with the local name \a localName and
    the namespace URI \a nsURI.

    If the map does not contain such a node, a \link
    QDomNode::isNull() null node\endlink is returned.

    \sa setNamedItemNS() namedItem()
*/
QDomNode QDomNamedNodeMap::namedItemNS(const QString& nsURI, const QString& localName) const
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->namedItemNS(nsURI, localName));
}

/*!
    Inserts the node \a newNode in the map. If a node with the same
    namespace URI and the same local name already exists in the map,
    it is replaced by \a newNode. If the new node replaces an existing
    node, the replaced node is returned.

    \sa namedItemNS() removeNamedItemNS() setNamedItem()
*/
QDomNode QDomNamedNodeMap::setNamedItemNS(const QDomNode& newNode)
{
    if (!impl)
        return QDomNode();
    return QDomNode(IMPL->setNamedItemNS((QDomNodePrivate*)newNode.impl));
}

/*!
    Removes the node with the local name \a localName and the
    namespace URI \a nsURI from the map.

    The function returns the removed node or a \link
    QDomNode::isNull() null node\endlink if the map did not contain a
    node with the local name \a localName and the namespace URI \a
    nsURI.

    \sa setNamedItemNS() namedItemNS() removeNamedItem()
*/
QDomNode QDomNamedNodeMap::removeNamedItemNS(const QString& nsURI, const QString& localName)
{
    if (!impl)
        return QDomNode();
    QDomNodePrivate *n = IMPL->namedItemNS(nsURI, localName);
    if (!n)
        return QDomNode();
    return QDomNode(IMPL->removeNamedItem(n->name));
}

/*!
    Returns the number of nodes in the map.

    \sa item()
*/
uint QDomNamedNodeMap::length() const
{
    if (!impl)
        return 0;
    return IMPL->length();
}

/*!
    \fn bool QDomNamedNodeMap::isEmpty() const

    Returns true if the map is empty; otherwise returns false. This function is
    provided for Qt API consistency.
*/

/*!
    \fn int QDomNamedNodeMap::count() const

    This function is provided for Qt API consistency. It is equivalent to length().
*/

/*!
    \fn int QDomNamedNodeMap::size() const

    This function is provided for Qt API consistency. It is equivalent to length().
*/

/*!
    Returns true if the map contains a node called \a name; otherwise
    returns false.

    \bold{Note:} This function does not take the presence of namespaces into account.
    Use namedItemNS() to test whether the map contains a node with a specific namespace
    URI and name.
*/
bool QDomNamedNodeMap::contains(const QString& name) const
{
    if (!impl)
        return false;
    return IMPL->contains(name);
}

#undef IMPL

/**************************************************************
 *
 * QDomDocumentTypePrivate
 *
 **************************************************************/

QDomDocumentTypePrivate::QDomDocumentTypePrivate(QDomDocumentPrivate* doc, QDomNodePrivate* parent)
    : QDomNodePrivate(doc, parent)
{
    init();
}

QDomDocumentTypePrivate::QDomDocumentTypePrivate(QDomDocumentTypePrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
    init();
    // Refill the maps with our new children
    QDomNodePrivate* p = first;
    while (p) {
        if (p->isEntity())
            // Don't use normal insert function since we would create infinite recursion
            entities->map.insertMulti(p->nodeName(), p);
        if (p->isNotation())
            // Don't use normal insert function since we would create infinite recursion
            notations->map.insertMulti(p->nodeName(), p);
        p = p->next;
    }
}

QDomDocumentTypePrivate::~QDomDocumentTypePrivate()
{
    if (!entities->ref.deref())
        delete entities;
    if (!notations->ref.deref())
        delete notations;
}

void QDomDocumentTypePrivate::init()
{
    entities = new QDomNamedNodeMapPrivate(this);
    QT_TRY {
        notations = new QDomNamedNodeMapPrivate(this);
        publicId.clear();
        systemId.clear();
        internalSubset.clear();

        entities->setAppendToParent(true);
        notations->setAppendToParent(true);
    } QT_CATCH(...) {
        delete entities;
        QT_RETHROW;
    }
}

QDomNodePrivate* QDomDocumentTypePrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomDocumentTypePrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

QDomNodePrivate* QDomDocumentTypePrivate::insertBefore(QDomNodePrivate* newChild, QDomNodePrivate* refChild)
{
    // Call the origianl implementation
    QDomNodePrivate* p = QDomNodePrivate::insertBefore(newChild, refChild);
    // Update the maps
    if (p && p->isEntity())
        entities->map.insertMulti(p->nodeName(), p);
    else if (p && p->isNotation())
        notations->map.insertMulti(p->nodeName(), p);

    return p;
}

QDomNodePrivate* QDomDocumentTypePrivate::insertAfter(QDomNodePrivate* newChild, QDomNodePrivate* refChild)
{
    // Call the origianl implementation
    QDomNodePrivate* p = QDomNodePrivate::insertAfter(newChild, refChild);
    // Update the maps
    if (p && p->isEntity())
        entities->map.insertMulti(p->nodeName(), p);
    else if (p && p->isNotation())
        notations->map.insertMulti(p->nodeName(), p);

    return p;
}

QDomNodePrivate* QDomDocumentTypePrivate::replaceChild(QDomNodePrivate* newChild, QDomNodePrivate* oldChild)
{
    // Call the origianl implementation
    QDomNodePrivate* p = QDomNodePrivate::replaceChild(newChild, oldChild);
    // Update the maps
    if (p) {
        if (oldChild && oldChild->isEntity())
            entities->map.remove(oldChild->nodeName());
        else if (oldChild && oldChild->isNotation())
            notations->map.remove(oldChild->nodeName());

        if (p->isEntity())
            entities->map.insertMulti(p->nodeName(), p);
        else if (p->isNotation())
            notations->map.insertMulti(p->nodeName(), p);
    }

    return p;
}

QDomNodePrivate* QDomDocumentTypePrivate::removeChild(QDomNodePrivate* oldChild)
{
    // Call the origianl implementation
    QDomNodePrivate* p = QDomNodePrivate::removeChild( oldChild);
    // Update the maps
    if (p && p->isEntity())
        entities->map.remove(p->nodeName());
    else if (p && p->isNotation())
        notations->map.remove(p ->nodeName());

    return p;
}

QDomNodePrivate* QDomDocumentTypePrivate::appendChild(QDomNodePrivate* newChild)
{
    return insertAfter(newChild, 0);
}

static QString quotedValue(const QString &data)
{
    QChar quote = data.indexOf(QLatin1Char('\'')) == -1
                    ? QLatin1Char('\'')
                    : QLatin1Char('"');
    return quote + data + quote;
}

void QDomDocumentTypePrivate::save(QTextStream& s, int, int indent) const
{
    if (name.isEmpty())
        return;

    s << "<!DOCTYPE " << name;

    if (!publicId.isNull()) {
        s << " PUBLIC " << quotedValue(publicId);
        if (!systemId.isNull()) {
            s << ' ' << quotedValue(systemId);
        }
    } else if (!systemId.isNull()) {
        s << " SYSTEM " << quotedValue(systemId);
    }

    if (entities->length()>0 || notations->length()>0) {
        s << " [" << endl;

        QHash<QString, QDomNodePrivate *>::const_iterator it2 = notations->map.constBegin();
        for (; it2 != notations->map.constEnd(); ++it2)
            (*it2)->save(s, 0, indent);

        QHash<QString, QDomNodePrivate *>::const_iterator it = entities->map.constBegin();
        for (; it != entities->map.constEnd(); ++it)
            (*it)->save(s, 0, indent);

        s << ']';
    }

    s << '>' << endl;
}

/**************************************************************
 *
 * QDomDocumentType
 *
 **************************************************************/

#define IMPL ((QDomDocumentTypePrivate*)impl)

/*!
    \class QDomDocumentType
    \reentrant
    \brief The QDomDocumentType class is the representation of the DTD
    in the document tree.

    \inmodule QtXml
    \ingroup xml-tools

    The QDomDocumentType class allows read-only access to some of the
    data structures in the DTD: it can return a map of all entities()
    and notations(). In addition the function name() returns the name
    of the document type as specified in the &lt;!DOCTYPE name&gt;
    tag. This class also provides the publicId(), systemId() and
    internalSubset() functions.

    \sa QDomDocument
*/

/*!
    Creates an empty QDomDocumentType object.
*/
QDomDocumentType::QDomDocumentType() : QDomNode()
{
}

/*!
    Constructs a copy of \a n.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomDocumentType::QDomDocumentType(const QDomDocumentType& n)
    : QDomNode(n)
{
}

QDomDocumentType::QDomDocumentType(QDomDocumentTypePrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a n to this document type.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomDocumentType& QDomDocumentType::operator= (const QDomDocumentType& n)
{
    return (QDomDocumentType&) QDomNode::operator=(n);
}

/*!
    Returns the name of the document type as specified in the
    &lt;!DOCTYPE name&gt; tag.

    \sa nodeName()
*/
QString QDomDocumentType::name() const
{
    if (!impl)
        return QString();
    return IMPL->nodeName();
}

/*!
    Returns a map of all entities described in the DTD.
*/
QDomNamedNodeMap QDomDocumentType::entities() const
{
    if (!impl)
        return QDomNamedNodeMap();
    return QDomNamedNodeMap(IMPL->entities);
}

/*!
    Returns a map of all notations described in the DTD.
*/
QDomNamedNodeMap QDomDocumentType::notations() const
{
    if (!impl)
        return QDomNamedNodeMap();
    return QDomNamedNodeMap(IMPL->notations);
}

/*!
    Returns the public identifier of the external DTD subset or
    an empty string if there is no public identifier.

    \sa systemId() internalSubset() QDomImplementation::createDocumentType()
*/
QString QDomDocumentType::publicId() const
{
    if (!impl)
        return QString();
    return IMPL->publicId;
}

/*!
    Returns the system identifier of the external DTD subset or
    an empty string if there is no system identifier.

    \sa publicId() internalSubset() QDomImplementation::createDocumentType()
*/
QString QDomDocumentType::systemId() const
{
    if (!impl)
        return QString();
    return IMPL->systemId;
}

/*!
    Returns the internal subset of the document type or an empty
    string if there is no internal subset.

    \sa publicId() systemId()
*/
QString QDomDocumentType::internalSubset() const
{
    if (!impl)
        return QString();
    return IMPL->internalSubset;
}

/*
    Are these needed at all? The only difference when removing these
    two methods in all subclasses is that we'd get a different type
    for null nodes.
*/

/*!
    \fn QDomNode::NodeType QDomDocumentType::nodeType() const

    Returns \c DocumentTypeNode.

    \sa isDocumentType() QDomNode::toDocumentType()
*/

#undef IMPL

/**************************************************************
 *
 * QDomDocumentFragmentPrivate
 *
 **************************************************************/

QDomDocumentFragmentPrivate::QDomDocumentFragmentPrivate(QDomDocumentPrivate* doc, QDomNodePrivate* parent)
    : QDomNodePrivate(doc, parent)
{
    name = QLatin1String("#document-fragment");
}

QDomDocumentFragmentPrivate::QDomDocumentFragmentPrivate(QDomNodePrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
}

QDomNodePrivate* QDomDocumentFragmentPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomDocumentFragmentPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

/**************************************************************
 *
 * QDomDocumentFragment
 *
 **************************************************************/

/*!
    \class QDomDocumentFragment
    \reentrant
    \brief The QDomDocumentFragment class is a tree of QDomNodes which is not usually a complete QDomDocument.

    \inmodule QtXml
    \ingroup xml-tools

    If you want to do complex tree operations it is useful to have a
    lightweight class to store nodes and their relations.
    QDomDocumentFragment stores a subtree of a document which does not
    necessarily represent a well-formed XML document.

    QDomDocumentFragment is also useful if you want to group several
    nodes in a list and insert them all together as children of some
    node. In these cases QDomDocumentFragment can be used as a
    temporary container for this list of children.

    The most important feature of QDomDocumentFragment is that it is
    treated in a special way by QDomNode::insertAfter(),
    QDomNode::insertBefore(), QDomNode::replaceChild() and
    QDomNode::appendChild(): instead of inserting the fragment itself, all
    the fragment's children are inserted.
*/

/*!
    Constructs an empty document fragment.
*/
QDomDocumentFragment::QDomDocumentFragment()
{
}

QDomDocumentFragment::QDomDocumentFragment(QDomDocumentFragmentPrivate* n)
    : QDomNode(n)
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomDocumentFragment::QDomDocumentFragment(const QDomDocumentFragment& x)
    : QDomNode(x)
{
}

/*!
    Assigns \a x to this DOM document fragment.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomDocumentFragment& QDomDocumentFragment::operator= (const QDomDocumentFragment& x)
{
    return (QDomDocumentFragment&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomDocumentFragment::nodeType() const

    Returns \c DocumentFragment.

    \sa isDocumentFragment() QDomNode::toDocumentFragment()
*/

/**************************************************************
 *
 * QDomCharacterDataPrivate
 *
 **************************************************************/

QDomCharacterDataPrivate::QDomCharacterDataPrivate(QDomDocumentPrivate* d, QDomNodePrivate* p,
                                                      const QString& data)
    : QDomNodePrivate(d, p)
{
    value = data;
    name = QLatin1String("#character-data");
}

QDomCharacterDataPrivate::QDomCharacterDataPrivate(QDomCharacterDataPrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
}

QDomNodePrivate* QDomCharacterDataPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomCharacterDataPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

uint QDomCharacterDataPrivate::dataLength() const
{
    return value.length();
}

QString QDomCharacterDataPrivate::substringData(unsigned long offset, unsigned long n) const
{
    return value.mid(offset, n);
}

void QDomCharacterDataPrivate::insertData(unsigned long offset, const QString& arg)
{
    value.insert(offset, arg);
}

void QDomCharacterDataPrivate::deleteData(unsigned long offset, unsigned long n)
{
    value.remove(offset, n);
}

void QDomCharacterDataPrivate::replaceData(unsigned long offset, unsigned long n, const QString& arg)
{
    value.replace(offset, n, arg);
}

void QDomCharacterDataPrivate::appendData(const QString& arg)
{
    value += arg;
}

/**************************************************************
 *
 * QDomCharacterData
 *
 **************************************************************/

#define IMPL ((QDomCharacterDataPrivate*)impl)

/*!
    \class QDomCharacterData
    \reentrant
    \brief The QDomCharacterData class represents a generic string in the DOM.

    \inmodule QtXml
    \ingroup xml-tools

    Character data as used in XML specifies a generic data string.
    More specialized versions of this class are QDomText, QDomComment
    and QDomCDATASection.

    The data string is set with setData() and retrieved with data().
    You can retrieve a portion of the data string using
    substringData(). Extra data can be appended with appendData(), or
    inserted with insertData(). Portions of the data string can be
    deleted with deleteData() or replaced with replaceData(). The
    length of the data string is returned by length().

    The node type of the node containing this character data is
    returned by nodeType().

    \sa QDomText QDomComment QDomCDATASection
*/

/*!
    Constructs an empty character data object.
*/
QDomCharacterData::QDomCharacterData()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomCharacterData::QDomCharacterData(const QDomCharacterData& x)
    : QDomNode(x)
{
}

QDomCharacterData::QDomCharacterData(QDomCharacterDataPrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this character data.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomCharacterData& QDomCharacterData::operator= (const QDomCharacterData& x)
{
    return (QDomCharacterData&) QDomNode::operator=(x);
}

/*!
    Returns the string stored in this object.

    If the node is a \link isNull() null node\endlink, it will return
    an empty string.
*/
QString QDomCharacterData::data() const
{
    if (!impl)
        return QString();
    return impl->nodeValue();
}

/*!
    Sets this object's string to \a v.
*/
void QDomCharacterData::setData(const QString& v)
{
    if (impl)
        impl->setNodeValue(v);
}

/*!
    Returns the length of the stored string.
*/
uint QDomCharacterData::length() const
{
    if (impl)
        return IMPL->dataLength();
    return 0;
}

/*!
    Returns the substring of length \a count from position \a offset.
*/
QString QDomCharacterData::substringData(unsigned long offset, unsigned long count)
{
    if (!impl)
        return QString();
    return IMPL->substringData(offset, count);
}

/*!
    Appends the string \a arg to the stored string.
*/
void QDomCharacterData::appendData(const QString& arg)
{
    if (impl)
        IMPL->appendData(arg);
}

/*!
    Inserts the string \a arg into the stored string at position \a offset.
*/
void QDomCharacterData::insertData(unsigned long offset, const QString& arg)
{
    if (impl)
        IMPL->insertData(offset, arg);
}

/*!
    Deletes a substring of length \a count from position \a offset.
*/
void QDomCharacterData::deleteData(unsigned long offset, unsigned long count)
{
    if (impl)
        IMPL->deleteData(offset, count);
}

/*!
    Replaces the substring of length \a count starting at position \a
    offset with the string \a arg.
*/
void QDomCharacterData::replaceData(unsigned long offset, unsigned long count, const QString& arg)
{
    if (impl)
        IMPL->replaceData(offset, count, arg);
}

/*!
    Returns the type of node this object refers to (i.e. \c TextNode,
    \c CDATASectionNode, \c CommentNode or \c CharacterDataNode). For
    a \link isNull() null node\endlink, returns \c CharacterDataNode.
*/
QDomNode::NodeType QDomCharacterData::nodeType() const
{
    if (!impl)
        return CharacterDataNode;
    return QDomNode::nodeType();
}

#undef IMPL

/**************************************************************
 *
 * QDomAttrPrivate
 *
 **************************************************************/

QDomAttrPrivate::QDomAttrPrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent, const QString& name_)
    : QDomNodePrivate(d, parent)
{
    name = name_;
    m_specified = false;
}

QDomAttrPrivate::QDomAttrPrivate(QDomDocumentPrivate* d, QDomNodePrivate* p, const QString& nsURI, const QString& qName)
    : QDomNodePrivate(d, p)
{
    qt_split_namespace(prefix, name, qName, !nsURI.isNull());
    namespaceURI = nsURI;
    createdWithDom1Interface = false;
    m_specified = false;
}

QDomAttrPrivate::QDomAttrPrivate(QDomAttrPrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
    m_specified = n->specified();
}

void QDomAttrPrivate::setNodeValue(const QString& v)
{
    value = v;
    QDomTextPrivate *t = new QDomTextPrivate(0, this, v);
    // keep the refcount balanced: appendChild() does a ref anyway.
    t->ref.deref();
    if (first) {
        delete removeChild(first);
    }
    appendChild(t);
}

QDomNodePrivate* QDomAttrPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomAttrPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

bool QDomAttrPrivate::specified() const
{
    return m_specified;
}

/* \internal
  Encode & escape \a str. Yes, it makes no sense to return a QString,
  but is so for legacy reasons.

  Remember that content produced should be able to roundtrip with 2.11 End-of-Line Handling
  and 3.3.3 Attribute-Value Normalization.

  If \a performAVN is true, characters will be escaped to survive Attribute Value Normalization.
  If \a encodeEOLs is true, characters will be escaped to survive End-of-Line Handling.
*/
static QString encodeText(const QString &str,
                          QTextStream &s,
                          const bool encodeQuotes = true,
                          const bool performAVN = false,
                          const bool encodeEOLs = false)
{
#ifdef QT_NO_TEXTCODEC
    Q_UNUSED(s);
#else
    const QTextCodec *const codec = s.codec();
    Q_ASSERT(codec);
#endif
    QString retval(str);
    int len = retval.length();
    int i = 0;

    while (i < len) {
        const QChar ati(retval.at(i));

        if (ati == QLatin1Char('<')) {
            retval.replace(i, 1, QLatin1String("&lt;"));
            len += 3;
            i += 4;
        } else if (encodeQuotes && (ati == QLatin1Char('"'))) {
            retval.replace(i, 1, QLatin1String("&quot;"));
            len += 5;
            i += 6;
        } else if (ati == QLatin1Char('&')) {
            retval.replace(i, 1, QLatin1String("&amp;"));
            len += 4;
            i += 5;
        } else if (ati == QLatin1Char('>') && i >= 2 && retval[i - 1] == QLatin1Char(']') && retval[i - 2] == QLatin1Char(']')) {
            retval.replace(i, 1, QLatin1String("&gt;"));
            len += 3;
            i += 4;
        } else if (performAVN &&
                   (ati == QChar(0xA) ||
                    ati == QChar(0xD) ||
                    ati == QChar(0x9))) {
            const QString replacement(QLatin1String("&#x") + QString::number(ati.unicode(), 16) + QLatin1Char(';'));
            retval.replace(i, 1, replacement);
            i += replacement.length();
            len += replacement.length() - 1;
        } else if (encodeEOLs && ati == QChar(0xD)) {
            retval.replace(i, 1, QLatin1String("&#xd;")); // Replace a single 0xD with a ref for 0xD
            len += 4;
            i += 5;
        } else {
#ifndef QT_NO_TEXTCODEC
            if(codec->canEncode(ati))
                ++i;
            else
#endif
            {
                // We have to use a character reference to get it through.
                const ushort codepoint(ati.unicode());
                const QString replacement(QLatin1String("&#x") + QString::number(codepoint, 16) + QLatin1Char(';'));
                retval.replace(i, 1, replacement);
                i += replacement.length();
                len += replacement.length() - 1;
            }
        }
    }

    return retval;
}

void QDomAttrPrivate::save(QTextStream& s, int, int) const
{
    if (namespaceURI.isNull()) {
        s << name << "=\"" << encodeText(value, s, true, true) << '\"';
    } else {
        s << prefix << ':' << name << "=\"" << encodeText(value, s, true, true) << '\"';
        /* This is a fix for 138243, as good as it gets.
         *
         * QDomElementPrivate::save() output a namespace declaration if
         * the element is in a namespace, no matter what. This function do as well, meaning
         * that we get two identical namespace declaration if we don't have the if-
         * statement below.
         *
         * This doesn't work when the parent element has the same prefix as us but
         * a different namespace. However, this can only occur by the user modifying the element,
         * and we don't do fixups by that anyway, and hence it's the user responsibility to not
         * arrive in those situations. */
        if(!ownerNode ||
           ownerNode->prefix != prefix) {
            s << " xmlns:" << prefix << "=\"" << encodeText(namespaceURI, s, true, true) << '\"';
        }
    }
}

/**************************************************************
 *
 * QDomAttr
 *
 **************************************************************/

#define IMPL ((QDomAttrPrivate*)impl)

/*!
    \class QDomAttr
    \reentrant
    \brief The QDomAttr class represents one attribute of a QDomElement.

    \inmodule QtXml
    \ingroup xml-tools

    For example, the following piece of XML produces an element with
    no children, but two attributes:

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 7

    You can access the attributes of an element with code like this:

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 8

    This example also shows that changing an attribute received from
    an element changes the attribute of the element. If you do not
    want to change the value of the element's attribute you must
    use cloneNode() to get an independent copy of the attribute.

    QDomAttr can return the name() and value() of an attribute. An
    attribute's value is set with setValue(). If specified() returns
    true the value was set with setValue(). The node this
    attribute is attached to (if any) is returned by ownerElement().

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/


/*!
    Constructs an empty attribute.
*/
QDomAttr::QDomAttr()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomAttr::QDomAttr(const QDomAttr& x)
    : QDomNode(x)
{
}

QDomAttr::QDomAttr(QDomAttrPrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this DOM attribute.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomAttr& QDomAttr::operator= (const QDomAttr& x)
{
    return (QDomAttr&) QDomNode::operator=(x);
}

/*!
    Returns the attribute's name.
*/
QString QDomAttr::name() const
{
    if (!impl)
        return QString();
    return impl->nodeName();
}

/*!
    Returns true if the attribute has been set by the user with setValue().
    Returns false if the value hasn't been specified or set.

    \sa setValue()
*/
bool QDomAttr::specified() const
{
    if (!impl)
        return false;
    return IMPL->specified();
}

/*!
    Returns the element node this attribute is attached to or a \link
    QDomNode::isNull() null node\endlink if this attribute is not
    attached to any element.
*/
QDomElement QDomAttr::ownerElement() const
{
    Q_ASSERT(impl->parent());
    if (!impl->parent()->isElement())
        return QDomElement();
    return QDomElement((QDomElementPrivate*)(impl->parent()));
}

/*!
    Returns the value of the attribute or an empty string if the
    attribute has not been specified.

    \sa specified() setValue()
*/
QString QDomAttr::value() const
{
    if (!impl)
        return QString();
    return impl->nodeValue();
}

/*!
    Sets the attribute's value to \a v.

    \sa value()
*/
void QDomAttr::setValue(const QString& v)
{
    if (!impl)
        return;
    impl->setNodeValue(v);
    IMPL->m_specified = true;
}

/*!
    \fn QDomNode::NodeType QDomAttr::nodeType() const

    Returns \link QDomNode::NodeType AttributeNode\endlink.
*/

#undef IMPL

/**************************************************************
 *
 * QDomElementPrivate
 *
 **************************************************************/

QDomElementPrivate::QDomElementPrivate(QDomDocumentPrivate* d, QDomNodePrivate* p,
                                          const QString& tagname)
    : QDomNodePrivate(d, p)
{
    name = tagname;
    m_attr = new QDomNamedNodeMapPrivate(this);
}

QDomElementPrivate::QDomElementPrivate(QDomDocumentPrivate* d, QDomNodePrivate* p,
        const QString& nsURI, const QString& qName)
    : QDomNodePrivate(d, p)
{
    qt_split_namespace(prefix, name, qName, !nsURI.isNull());
    namespaceURI = nsURI;
    createdWithDom1Interface = false;
    m_attr = new QDomNamedNodeMapPrivate(this);
}

QDomElementPrivate::QDomElementPrivate(QDomElementPrivate* n, bool deep) :
    QDomNodePrivate(n, deep)
{
    m_attr = n->m_attr->clone(this);
    // Reference is down to 0, so we set it to 1 here.
    m_attr->ref.ref();
}

QDomElementPrivate::~QDomElementPrivate()
{
    if (!m_attr->ref.deref())
        delete m_attr;
}

QDomNodePrivate* QDomElementPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomElementPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

QString QDomElementPrivate::attribute(const QString& name_, const QString& defValue) const
{
    QDomNodePrivate* n = m_attr->namedItem(name_);
    if (!n)
        return defValue;

    return n->nodeValue();
}

QString QDomElementPrivate::attributeNS(const QString& nsURI, const QString& localName, const QString& defValue) const
{
    QDomNodePrivate* n = m_attr->namedItemNS(nsURI, localName);
    if (!n)
        return defValue;

    return n->nodeValue();
}

void QDomElementPrivate::setAttribute(const QString& aname, const QString& newValue)
{
    QDomNodePrivate* n = m_attr->namedItem(aname);
    if (!n) {
        n = new QDomAttrPrivate(ownerDocument(), this, aname);
        n->setNodeValue(newValue);

        // Referencing is done by the map, so we set the reference counter back
        // to 0 here. This is ok since we created the QDomAttrPrivate.
        n->ref.deref();
        m_attr->setNamedItem(n);
    } else {
        n->setNodeValue(newValue);
    }
}

void QDomElementPrivate::setAttributeNS(const QString& nsURI, const QString& qName, const QString& newValue)
{
    QString prefix, localName;
    qt_split_namespace(prefix, localName, qName, true);
    QDomNodePrivate* n = m_attr->namedItemNS(nsURI, localName);
    if (!n) {
        n = new QDomAttrPrivate(ownerDocument(), this, nsURI, qName);
        n->setNodeValue(newValue);

        // Referencing is done by the map, so we set the reference counter back
        // to 0 here. This is ok since we created the QDomAttrPrivate.
        n->ref.deref();
        m_attr->setNamedItem(n);
    } else {
        n->setNodeValue(newValue);
        n->prefix = prefix;
    }
}

void QDomElementPrivate::removeAttribute(const QString& aname)
{
    QDomNodePrivate* p = m_attr->removeNamedItem(aname);
    if (p && p->ref == 0)
        delete p;
}

QDomAttrPrivate* QDomElementPrivate::attributeNode(const QString& aname)
{
    return (QDomAttrPrivate*)m_attr->namedItem(aname);
}

QDomAttrPrivate* QDomElementPrivate::attributeNodeNS(const QString& nsURI, const QString& localName)
{
    return (QDomAttrPrivate*)m_attr->namedItemNS(nsURI, localName);
}

QDomAttrPrivate* QDomElementPrivate::setAttributeNode(QDomAttrPrivate* newAttr)
{
    QDomNodePrivate* n = m_attr->namedItem(newAttr->nodeName());

    // Referencing is done by the maps
    m_attr->setNamedItem(newAttr);

    newAttr->setParent(this);

    return (QDomAttrPrivate*)n;
}

QDomAttrPrivate* QDomElementPrivate::setAttributeNodeNS(QDomAttrPrivate* newAttr)
{
    QDomNodePrivate* n = 0;
    if (!newAttr->prefix.isNull())
        n = m_attr->namedItemNS(newAttr->namespaceURI, newAttr->name);

    // Referencing is done by the maps
    m_attr->setNamedItem(newAttr);

    return (QDomAttrPrivate*)n;
}

QDomAttrPrivate* QDomElementPrivate::removeAttributeNode(QDomAttrPrivate* oldAttr)
{
    return (QDomAttrPrivate*)m_attr->removeNamedItem(oldAttr->nodeName());
}

bool QDomElementPrivate::hasAttribute(const QString& aname)
{
    return m_attr->contains(aname);
}

bool QDomElementPrivate::hasAttributeNS(const QString& nsURI, const QString& localName)
{
    return m_attr->containsNS(nsURI, localName);
}

QString QDomElementPrivate::text()
{
    QString t(QLatin1String(""));

    QDomNodePrivate* p = first;
    while (p) {
        if (p->isText() || p->isCDATASection())
            t += p->nodeValue();
        else if (p->isElement())
            t += ((QDomElementPrivate*)p)->text();
        p = p->next;
    }

    return t;
}

void QDomElementPrivate::save(QTextStream& s, int depth, int indent) const
{
    if (!(prev && prev->isText()))
        s << QString(indent < 1 ? 0 : depth * indent, QLatin1Char(' '));

    QString qName(name);
    QString nsDecl(QLatin1String(""));
    if (!namespaceURI.isNull()) {
        /** ### Qt 5:
         *
         * If we still have QDom, optimize this so that we only declare namespaces that are not
         * yet declared. We loose default namespace mappings, so maybe we should rather store
         * the information that we get from startPrefixMapping()/endPrefixMapping() and use them.
         * Modifications becomes more complex then, however.
         *
         * We cannot do this during the Qt 4 series because it would require too invasive changes, and
         * hence possibly behavioral changes.
         */
        if (prefix.isEmpty()) {
            nsDecl = QLatin1String(" xmlns");
        } else {
            qName = prefix + QLatin1Char(':') + name;
            nsDecl = QLatin1String(" xmlns:") + prefix;
        }
        nsDecl += QLatin1String("=\"") + encodeText(namespaceURI, s) + QLatin1Char('\"');
    }
    s << '<' << qName << nsDecl;

    QSet<QString> outputtedPrefixes;

    /* Write out attributes. */
    if (!m_attr->map.isEmpty()) {
        QHash<QString, QDomNodePrivate *>::const_iterator it = m_attr->map.constBegin();
        for (; it != m_attr->map.constEnd(); ++it) {
            s << ' ';
            if (it.value()->namespaceURI.isNull()) {
                s << it.value()->name << "=\"" << encodeText(it.value()->value, s, true, true) << '\"';
            } else {
                s << it.value()->prefix << ':' << it.value()->name << "=\"" << encodeText(it.value()->value, s, true, true) << '\"';
                /* This is a fix for 138243, as good as it gets.
                 *
                 * QDomElementPrivate::save() output a namespace declaration if
                 * the element is in a namespace, no matter what. This function do as well, meaning
                 * that we get two identical namespace declaration if we don't have the if-
                 * statement below.
                 *
                 * This doesn't work when the parent element has the same prefix as us but
                 * a different namespace. However, this can only occur by the user modifying the element,
                 * and we don't do fixups by that anyway, and hence it's the user responsibility to not
                 * arrive in those situations. */
                if((!it.value()->ownerNode ||
                   it.value()->ownerNode->prefix != it.value()->prefix) &&
                   !outputtedPrefixes.contains(it.value()->prefix)) {
                    s << " xmlns:" << it.value()->prefix << "=\"" << encodeText(it.value()->namespaceURI, s, true, true) << '\"';
                    outputtedPrefixes.insert(it.value()->prefix);
                }
            }
        }
    }

    if (last) {
        // has child nodes
        if (first->isText())
            s << '>';
        else {
            s << '>';

            /* -1 disables new lines. */
            if (indent != -1)
                s << endl;
        }
        QDomNodePrivate::save(s, depth + 1, indent); if (!last->isText())
            s << QString(indent < 1 ? 0 : depth * indent, QLatin1Char(' '));

        s << "</" << qName << '>';
    } else {
        s << "/>";
    }
    if (!(next && next->isText())) {
        /* -1 disables new lines. */
        if (indent != -1)
            s << endl;
    }
}

/**************************************************************
 *
 * QDomElement
 *
 **************************************************************/

#define IMPL ((QDomElementPrivate*)impl)

/*!
    \class QDomElement
    \reentrant
    \brief The QDomElement class represents one element in the DOM tree.

    \inmodule QtXml
    \ingroup xml-tools

    Elements have a tagName() and zero or more attributes associated
    with them. The tag name can be changed with setTagName().

    Element attributes are represented by QDomAttr objects that can
    be queried using the attribute() and attributeNode() functions.
    You can set attributes with the setAttribute() and
    setAttributeNode() functions. Attributes can be removed with
    removeAttribute(). There are namespace-aware equivalents to these
    functions, i.e. setAttributeNS(), setAttributeNodeNS() and
    removeAttributeNS().

    If you want to access the text of a node use text(), e.g.
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 9
    The text() function operates recursively to find the text (since
    not all elements contain text). If you want to find all the text
    in all of a node's children, iterate over the children looking for
    QDomText nodes, e.g.
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 10
    Note that we attempt to convert each node to a text node and use
    text() rather than using firstChild().toText().data() or
    n.toText().data() directly on the node, because the node may not
    be a text element.

    You can get a list of all the decendents of an element which have
    a specified tag name with elementsByTagName() or
    elementsByTagNameNS().

    To browse the elements of a dom document use firstChildElement(), lastChildElement(),
    nextSiblingElement() and previousSiblingElement(). For example, to iterate over all
    child elements called "entry" in a root element called "database", you can use:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 11

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs an empty element. Use the QDomDocument::createElement()
    function to construct elements with content.
*/
QDomElement::QDomElement()
    : QDomNode()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomElement::QDomElement(const QDomElement& x)
    : QDomNode(x)
{
}

QDomElement::QDomElement(QDomElementPrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this DOM element.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomElement& QDomElement::operator= (const QDomElement& x)
{
    return (QDomElement&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomElement::nodeType() const

    Returns \c ElementNode.
*/

/*!
    Sets this element's tag name to \a name.

    \sa tagName()
*/
void QDomElement::setTagName(const QString& name)
{
    if (impl)
        impl->name = name;
}

/*!
    Returns the tag name of this element. For an XML element like this:

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 12

    the tagname would return "img".

    \sa setTagName()
*/
QString QDomElement::tagName() const
{
    if (!impl)
        return QString();
    return impl->nodeName();
}


/*!
    Returns a QDomNamedNodeMap containing all this element's attributes.

    \sa attribute() setAttribute() attributeNode() setAttributeNode()
*/
QDomNamedNodeMap QDomElement::attributes() const
{
    if (!impl)
        return QDomNamedNodeMap();
    return QDomNamedNodeMap(IMPL->attributes());
}

/*!
    Returns the attribute called \a name. If the attribute does not
    exist \a defValue is returned.

    \sa setAttribute() attributeNode() setAttributeNode() attributeNS()
*/
QString QDomElement::attribute(const QString& name, const QString& defValue) const
{
    if (!impl)
        return defValue;
    return IMPL->attribute(name, defValue);
}

/*!
    Adds an attribute called \a name with value \a value. If an
    attribute with the same name exists, its value is replaced by \a
    value.

    \sa attribute() setAttributeNode() setAttributeNS()
*/
void QDomElement::setAttribute(const QString& name, const QString& value)
{
    if (!impl)
        return;
    IMPL->setAttribute(name, value);
}

/*!
  \fn void QDomElement::setAttribute(const QString& name, int value)

    \overload
    The number is formatted according to the current locale.
*/

/*!
  \fn void QDomElement::setAttribute(const QString& name, uint value)

    \overload
    The number is formatted according to the current locale.
*/

/*!
    \overload

    The number is formatted according to the current locale.
*/
void QDomElement::setAttribute(const QString& name, qlonglong value)
{
    if (!impl)
        return;
    QString x;
    x.setNum(value);
    IMPL->setAttribute(name, x);
}

/*!
    \overload

    The number is formatted according to the current locale.
*/
void QDomElement::setAttribute(const QString& name, qulonglong value)
{
    if (!impl)
        return;
    QString x;
    x.setNum(value);
    IMPL->setAttribute(name, x);
}

/*!
    \overload

    The number is formatted according to the current locale.
*/
void QDomElement::setAttribute(const QString& name, float value)
{
    if (!impl)
        return;
    QString x;
    x.setNum(value);
    IMPL->setAttribute(name, x);
}

/*!
    \overload

    The number is formatted according to the current locale.
*/
void QDomElement::setAttribute(const QString& name, double value)
{
    if (!impl)
        return;
    QString x;
    char buf[256];
    int count = qsnprintf(buf, sizeof(buf), "%.16g", value);
    if (count > 0)
        x = QString::fromLatin1(buf, count);
    else
        x.setNum(value); // Fallback
    IMPL->setAttribute(name, x);
}

/*!
    Removes the attribute called name \a name from this element.

    \sa setAttribute() attribute() removeAttributeNS()
*/
void QDomElement::removeAttribute(const QString& name)
{
    if (!impl)
        return;
    IMPL->removeAttribute(name);
}

/*!
    Returns the QDomAttr object that corresponds to the attribute
    called \a name. If no such attribute exists a \link
    QDomNode::isNull() null attribute\endlink is returned.

    \sa setAttributeNode() attribute() setAttribute() attributeNodeNS()
*/
QDomAttr QDomElement::attributeNode(const QString& name)
{
    if (!impl)
        return QDomAttr();
    return QDomAttr(IMPL->attributeNode(name));
}

/*!
    Adds the attribute \a newAttr to this element.

    If the element has another attribute that has the same name as \a
    newAttr, this function replaces that attribute and returns it;
    otherwise the function returns a \link QDomNode::isNull() null
    attribute\endlink.

    \sa attributeNode() setAttribute() setAttributeNodeNS()
*/
QDomAttr QDomElement::setAttributeNode(const QDomAttr& newAttr)
{
    if (!impl)
        return QDomAttr();
    return QDomAttr(IMPL->setAttributeNode(((QDomAttrPrivate*)newAttr.impl)));
}

/*!
    Removes the attribute \a oldAttr from the element and returns it.

    \sa attributeNode() setAttributeNode()
*/
QDomAttr QDomElement::removeAttributeNode(const QDomAttr& oldAttr)
{
    if (!impl)
        return QDomAttr(); // ### should this return oldAttr?
    return QDomAttr(IMPL->removeAttributeNode(((QDomAttrPrivate*)oldAttr.impl)));
}

/*!
  Returns a QDomNodeList containing all descendants of this element
  named \a tagname encountered during a preorder traversal of the
  element subtree with this element as its root. The order of the
  elements in the returned list is the order they are encountered
  during the preorder traversal.

  \sa elementsByTagNameNS() QDomDocument::elementsByTagName()
*/
QDomNodeList QDomElement::elementsByTagName(const QString& tagname) const
{
    return QDomNodeList(new QDomNodeListPrivate(impl, tagname));
}

/*!
  Returns true if this element has an attribute called \a name;
  otherwise returns false.

  \bold{Note:} This function does not take the presence of namespaces
  into account.  As a result, the specified name will be tested
  against fully-qualified attribute names that include any namespace
  prefixes that may be present.

  Use hasAttributeNS() to explicitly test for attributes with specific
  namespaces and names.
*/
bool QDomElement::hasAttribute(const QString& name) const
{
    if (!impl)
        return false;
    return IMPL->hasAttribute(name);
}

/*!
    Returns the attribute with the local name \a localName and the
    namespace URI \a nsURI. If the attribute does not exist \a
    defValue is returned.

    \sa setAttributeNS() attributeNodeNS() setAttributeNodeNS() attribute()
*/
QString QDomElement::attributeNS(const QString nsURI, const QString& localName, const QString& defValue) const
{
    if (!impl)
        return defValue;
    return IMPL->attributeNS(nsURI, localName, defValue);
}

/*!
    Adds an attribute with the qualified name \a qName and the
    namespace URI \a nsURI with the value \a value. If an attribute
    with the same local name and namespace URI exists, its prefix is
    replaced by the prefix of \a qName and its value is repaced by \a
    value.

    Although \a qName is the qualified name, the local name is used to
    decide if an existing attribute's value should be replaced.

    \sa attributeNS() setAttributeNodeNS() setAttribute()
*/
void QDomElement::setAttributeNS(const QString nsURI, const QString& qName, const QString& value)
{
    if (!impl)
        return;
    IMPL->setAttributeNS(nsURI, qName, value);
}

/*!
  \fn void QDomElement::setAttributeNS(const QString nsURI, const QString& qName, int value)

    \overload
*/

/*!
  \fn void QDomElement::setAttributeNS(const QString nsURI, const QString& qName, uint value)

    \overload
*/

/*!
    \overload
*/
void QDomElement::setAttributeNS(const QString nsURI, const QString& qName, qlonglong value)
{
    if (!impl)
        return;
    QString x;
    x.setNum(value);
    IMPL->setAttributeNS(nsURI, qName, x);
}

/*!
    \overload
*/
void QDomElement::setAttributeNS(const QString nsURI, const QString& qName, qulonglong value)
{
    if (!impl)
        return;
    QString x;
    x.setNum(value);
    IMPL->setAttributeNS(nsURI, qName, x);
}

/*!
    \overload
*/
void QDomElement::setAttributeNS(const QString nsURI, const QString& qName, double value)
{
    if (!impl)
        return;
    QString x;
    x.setNum(value);
    IMPL->setAttributeNS(nsURI, qName, x);
}

/*!
    Removes the attribute with the local name \a localName and the
    namespace URI \a nsURI from this element.

    \sa setAttributeNS() attributeNS() removeAttribute()
*/
void QDomElement::removeAttributeNS(const QString& nsURI, const QString& localName)
{
    if (!impl)
        return;
    QDomNodePrivate *n = IMPL->attributeNodeNS(nsURI, localName);
    if (!n)
        return;
    IMPL->removeAttribute(n->nodeName());
}

/*!
    Returns the QDomAttr object that corresponds to the attribute
    with the local name \a localName and the namespace URI \a nsURI.
    If no such attribute exists a \l{QDomNode::isNull()}{null
    attribute} is returned.

    \sa setAttributeNode() attribute() setAttribute()
*/
QDomAttr QDomElement::attributeNodeNS(const QString& nsURI, const QString& localName)
{
    if (!impl)
        return QDomAttr();
    return QDomAttr(IMPL->attributeNodeNS(nsURI, localName));
}

/*!
    Adds the attribute \a newAttr to this element.

    If the element has another attribute that has the same local name
    and namespace URI as \a newAttr, this function replaces that
    attribute and returns it; otherwise the function returns a \link
    QDomNode::isNull() null attribute\endlink.

    \sa attributeNodeNS() setAttributeNS() setAttributeNode()
*/
QDomAttr QDomElement::setAttributeNodeNS(const QDomAttr& newAttr)
{
    if (!impl)
        return QDomAttr();
    return QDomAttr(IMPL->setAttributeNodeNS(((QDomAttrPrivate*)newAttr.impl)));
}

/*!
  Returns a QDomNodeList containing all descendants of this element
  with local name \a localName and namespace URI \a nsURI encountered
  during a preorder traversal of the element subtree with this element
  as its root. The order of the elements in the returned list is the
  order they are encountered during the preorder traversal.

  \sa elementsByTagName() QDomDocument::elementsByTagNameNS()
*/
QDomNodeList QDomElement::elementsByTagNameNS(const QString& nsURI, const QString& localName) const
{
    return QDomNodeList(new QDomNodeListPrivate(impl, nsURI, localName));
}

/*!
    Returns true if this element has an attribute with the local name
    \a localName and the namespace URI \a nsURI; otherwise returns
    false.
*/
bool QDomElement::hasAttributeNS(const QString& nsURI, const QString& localName) const
{
    if (!impl)
        return false;
    return IMPL->hasAttributeNS(nsURI, localName);
}

/*!
    Returns the element's text or an empty string.

    Example:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 13

    The function text() of the QDomElement for the \c{<h1>} tag,
    will return the following text:

    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 14

    Comments are ignored by this function. It only evaluates QDomText
    and QDomCDATASection objects.
*/
QString QDomElement::text() const
{
    if (!impl)
        return QString();
    return IMPL->text();
}

#undef IMPL

/**************************************************************
 *
 * QDomTextPrivate
 *
 **************************************************************/

QDomTextPrivate::QDomTextPrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent, const QString& val)
    : QDomCharacterDataPrivate(d, parent, val)
{
    name = QLatin1String("#text");
}

QDomTextPrivate::QDomTextPrivate(QDomTextPrivate* n, bool deep)
    : QDomCharacterDataPrivate(n, deep)
{
}

QDomNodePrivate* QDomTextPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomTextPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

QDomTextPrivate* QDomTextPrivate::splitText(int offset)
{
    if (!parent()) {
        qWarning("QDomText::splitText  The node has no parent. So I can not split");
        return 0;
    }

    QDomTextPrivate* t = new QDomTextPrivate(ownerDocument(), 0, value.mid(offset));
    value.truncate(offset);

    parent()->insertAfter(t, this);

    return t;
}

void QDomTextPrivate::save(QTextStream& s, int, int) const
{
    QDomTextPrivate *that = const_cast<QDomTextPrivate*>(this);
    s << encodeText(value, s, !(that->parent() && that->parent()->isElement()), false, true);
}

/**************************************************************
 *
 * QDomText
 *
 **************************************************************/

#define IMPL ((QDomTextPrivate*)impl)

/*!
    \class QDomText
    \reentrant
    \brief The QDomText class represents text data in the parsed XML document.

    \inmodule QtXml
    \ingroup xml-tools

    You can split the text in a QDomText object over two QDomText
    objecs with splitText().

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs an empty QDomText object.

    To construct a QDomText with content, use QDomDocument::createTextNode().
*/
QDomText::QDomText()
    : QDomCharacterData()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomText::QDomText(const QDomText& x)
    : QDomCharacterData(x)
{
}

QDomText::QDomText(QDomTextPrivate* n)
    : QDomCharacterData(n)
{
}

/*!
    Assigns \a x to this DOM text.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomText& QDomText::operator= (const QDomText& x)
{
    return (QDomText&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomText::nodeType() const

    Returns \c TextNode.
*/

/*!
    Splits this DOM text object into two QDomText objects. This object
    keeps its first \a offset characters and the second (newly
    created) object is inserted into the document tree after this
    object with the remaining characters.

    The function returns the newly created object.

    \sa QDomNode::normalize()
*/
QDomText QDomText::splitText(int offset)
{
    if (!impl)
        return QDomText();
    return QDomText(IMPL->splitText(offset));
}

#undef IMPL

/**************************************************************
 *
 * QDomCommentPrivate
 *
 **************************************************************/

QDomCommentPrivate::QDomCommentPrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent, const QString& val)
    : QDomCharacterDataPrivate(d, parent, val)
{
    name = QLatin1String("#comment");
}

QDomCommentPrivate::QDomCommentPrivate(QDomCommentPrivate* n, bool deep)
    : QDomCharacterDataPrivate(n, deep)
{
}


QDomNodePrivate* QDomCommentPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomCommentPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

void QDomCommentPrivate::save(QTextStream& s, int depth, int indent) const
{
    /* We don't output whitespace if we would pollute a text node. */
    if (!(prev && prev->isText()))
        s << QString(indent < 1 ? 0 : depth * indent, QLatin1Char(' '));

    s << "<!--" << value;
    if (value.endsWith(QLatin1Char('-')))
        s << ' '; // Ensures that XML comment doesn't end with --->
    s << "-->";

    if (!(next && next->isText()))
        s << endl;
}

/**************************************************************
 *
 * QDomComment
 *
 **************************************************************/

/*!
    \class QDomComment
    \reentrant
    \brief The QDomComment class represents an XML comment.

    \inmodule QtXml
    \ingroup xml-tools

    A comment in the parsed XML such as this:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 15
    is represented by QDomComment objects in the parsed Dom tree.

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs an empty comment. To construct a comment with content,
    use the QDomDocument::createComment() function.
*/
QDomComment::QDomComment()
    : QDomCharacterData()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomComment::QDomComment(const QDomComment& x)
    : QDomCharacterData(x)
{
}

QDomComment::QDomComment(QDomCommentPrivate* n)
    : QDomCharacterData(n)
{
}

/*!
    Assigns \a x to this DOM comment.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomComment& QDomComment::operator= (const QDomComment& x)
{
    return (QDomComment&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomComment::nodeType() const

    Returns \c CommentNode.
*/

/**************************************************************
 *
 * QDomCDATASectionPrivate
 *
 **************************************************************/

QDomCDATASectionPrivate::QDomCDATASectionPrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent,
                                                    const QString& val)
    : QDomTextPrivate(d, parent, val)
{
    name = QLatin1String("#cdata-section");
}

QDomCDATASectionPrivate::QDomCDATASectionPrivate(QDomCDATASectionPrivate* n, bool deep)
    : QDomTextPrivate(n, deep)
{
}

QDomNodePrivate* QDomCDATASectionPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomCDATASectionPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

void QDomCDATASectionPrivate::save(QTextStream& s, int, int) const
{
    // ### How do we escape "]]>" ?
    // "]]>" is not allowed; so there should be none in value anyway
    s << "<![CDATA[" << value << "]]>";
}

/**************************************************************
 *
 * QDomCDATASection
 *
 **************************************************************/

/*!
    \class QDomCDATASection
    \reentrant
    \brief The QDomCDATASection class represents an XML CDATA section.

    \inmodule QtXml
    \ingroup xml-tools

    CDATA sections are used to escape blocks of text containing
    characters that would otherwise be regarded as markup. The only
    delimiter that is recognized in a CDATA section is the "]]&gt;"
    string that terminates the CDATA section. CDATA sections cannot be
    nested. Their primary purpose is for including material such as
    XML fragments, without needing to escape all the delimiters.

    Adjacent QDomCDATASection nodes are not merged by the
    QDomNode::normalize() function.

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs an empty CDATA section. To create a CDATA section with
    content, use the QDomDocument::createCDATASection() function.
*/
QDomCDATASection::QDomCDATASection()
    : QDomText()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomCDATASection::QDomCDATASection(const QDomCDATASection& x)
    : QDomText(x)
{
}

QDomCDATASection::QDomCDATASection(QDomCDATASectionPrivate* n)
    : QDomText(n)
{
}

/*!
    Assigns \a x to this CDATA section.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomCDATASection& QDomCDATASection::operator= (const QDomCDATASection& x)
{
    return (QDomCDATASection&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomCDATASection::nodeType() const

    Returns \c CDATASection.
*/

/**************************************************************
 *
 * QDomNotationPrivate
 *
 **************************************************************/

QDomNotationPrivate::QDomNotationPrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent,
                                            const QString& aname,
                                            const QString& pub, const QString& sys)
    : QDomNodePrivate(d, parent)
{
    name = aname;
    m_pub = pub;
    m_sys = sys;
}

QDomNotationPrivate::QDomNotationPrivate(QDomNotationPrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
    m_sys = n->m_sys;
    m_pub = n->m_pub;
}

QDomNodePrivate* QDomNotationPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomNotationPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

void QDomNotationPrivate::save(QTextStream& s, int, int) const
{
    s << "<!NOTATION " << name << ' ';
    if (!m_pub.isNull())  {
        s << "PUBLIC " << quotedValue(m_pub);
        if (!m_sys.isNull())
            s << ' ' << quotedValue(m_sys);
    }  else {
        s << "SYSTEM " << quotedValue(m_sys);
    }
    s << '>' << endl;
}

/**************************************************************
 *
 * QDomNotation
 *
 **************************************************************/

#define IMPL ((QDomNotationPrivate*)impl)

/*!
    \class QDomNotation
    \reentrant
    \brief The QDomNotation class represents an XML notation.

    \inmodule QtXml
    \ingroup xml-tools

    A notation either declares, by name, the format of an unparsed
    entity (see section 4.7 of the XML 1.0 specification), or is used
    for formal declaration of processing instruction targets (see
    section 2.6 of the XML 1.0 specification).

    DOM does not support editing notation nodes; they are therefore
    read-only.

    A notation node does not have any parent.

    You can retrieve the publicId() and systemId() from a notation
    node.

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/


/*!
    Constructor.
*/
QDomNotation::QDomNotation()
    : QDomNode()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomNotation::QDomNotation(const QDomNotation& x)
    : QDomNode(x)
{
}

QDomNotation::QDomNotation(QDomNotationPrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this DOM notation.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomNotation& QDomNotation::operator= (const QDomNotation& x)
{
    return (QDomNotation&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomNotation::nodeType() const

    Returns \c NotationNode.
*/

/*!
    Returns the public identifier of this notation.
*/
QString QDomNotation::publicId() const
{
    if (!impl)
        return QString();
    return IMPL->m_pub;
}

/*!
    Returns the system identifier of this notation.
*/
QString QDomNotation::systemId() const
{
    if (!impl)
        return QString();
    return IMPL->m_sys;
}

#undef IMPL

/**************************************************************
 *
 * QDomEntityPrivate
 *
 **************************************************************/

QDomEntityPrivate::QDomEntityPrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent,
                                        const QString& aname,
                                        const QString& pub, const QString& sys, const QString& notation)
    : QDomNodePrivate(d, parent)
{
    name = aname;
    m_pub = pub;
    m_sys = sys;
    m_notationName = notation;
}

QDomEntityPrivate::QDomEntityPrivate(QDomEntityPrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
    m_sys = n->m_sys;
    m_pub = n->m_pub;
    m_notationName = n->m_notationName;
}

QDomNodePrivate* QDomEntityPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomEntityPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

/*
  Encode an entity value upon saving.
*/
static QByteArray encodeEntity(const QByteArray& str)
{
    QByteArray tmp(str);
    uint len = tmp.size();
    uint i = 0;
    const char* d = tmp.data();
    while (i < len) {
        if (d[i] == '%'){
            tmp.replace(i, 1, "&#60;");
            d = tmp;
            len += 4;
            i += 5;
        }
        else if (d[i] == '"') {
            tmp.replace(i, 1, "&#34;");
            d = tmp;
            len += 4;
            i += 5;
        } else if (d[i] == '&' && i + 1 < len && d[i+1] == '#') {
            // Don't encode &lt; or &quot; or &custom;.
            // Only encode character references
            tmp.replace(i, 1, "&#38;");
            d = tmp;
            len += 4;
            i += 5;
        } else {
            ++i;
        }
    }

    return tmp;
}

void QDomEntityPrivate::save(QTextStream& s, int, int) const
{
    QString _name = name;
    if (_name.startsWith(QLatin1Char('%')))
        _name = QLatin1String("% ") + _name.mid(1);

    if (m_sys.isNull() && m_pub.isNull()) {
        s << "<!ENTITY " << _name << " \"" << encodeEntity(value.toUtf8()) << "\">" << endl;
    } else {
        s << "<!ENTITY " << _name << ' ';
        if (m_pub.isNull()) {
            s << "SYSTEM " << quotedValue(m_sys);
        } else {
            s << "PUBLIC " << quotedValue(m_pub) << ' ' << quotedValue(m_sys);
        }
        if (! m_notationName.isNull()) {
            s << " NDATA " << m_notationName;
        }
        s << '>' << endl;
    }
}

/**************************************************************
 *
 * QDomEntity
 *
 **************************************************************/

#define IMPL ((QDomEntityPrivate*)impl)

/*!
    \class QDomEntity
    \reentrant
    \brief The QDomEntity class represents an XML entity.

    \inmodule QtXml
    \ingroup xml-tools

    This class represents an entity in an XML document, either parsed
    or unparsed. Note that this models the entity itself not the
    entity declaration.

    DOM does not support editing entity nodes; if a user wants to make
    changes to the contents of an entity, every related
    QDomEntityReference node must be replaced in the DOM tree by a
    clone of the entity's contents, and then the desired changes must
    be made to each of the clones instead. All the descendants of an
    entity node are read-only.

    An entity node does not have any parent.

    You can access the entity's publicId(), systemId() and
    notationName() when available.

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/


/*!
    Constructs an empty entity.
*/
QDomEntity::QDomEntity()
    : QDomNode()
{
}


/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomEntity::QDomEntity(const QDomEntity& x)
    : QDomNode(x)
{
}

QDomEntity::QDomEntity(QDomEntityPrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this DOM entity.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomEntity& QDomEntity::operator= (const QDomEntity& x)
{
    return (QDomEntity&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomEntity::nodeType() const

    Returns \c EntityNode.
*/

/*!
    Returns the public identifier associated with this entity. If the
    public identifier was not specified an empty string is returned.
*/
QString QDomEntity::publicId() const
{
    if (!impl)
        return QString();
    return IMPL->m_pub;
}

/*!
    Returns the system identifier associated with this entity. If the
    system identifier was not specified an empty string is returned.
*/
QString QDomEntity::systemId() const
{
    if (!impl)
        return QString();
    return IMPL->m_sys;
}

/*!
    For unparsed entities this function returns the name of the
    notation for the entity. For parsed entities this function returns
    an empty string.
*/
QString QDomEntity::notationName() const
{
    if (!impl)
        return QString();
    return IMPL->m_notationName;
}

#undef IMPL

/**************************************************************
 *
 * QDomEntityReferencePrivate
 *
 **************************************************************/

QDomEntityReferencePrivate::QDomEntityReferencePrivate(QDomDocumentPrivate* d, QDomNodePrivate* parent, const QString& aname)
    : QDomNodePrivate(d, parent)
{
    name = aname;
}

QDomEntityReferencePrivate::QDomEntityReferencePrivate(QDomNodePrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
}

QDomNodePrivate* QDomEntityReferencePrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomEntityReferencePrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

void QDomEntityReferencePrivate::save(QTextStream& s, int, int) const
{
    s << '&' << name << ';';
}

/**************************************************************
 *
 * QDomEntityReference
 *
 **************************************************************/

/*!
    \class QDomEntityReference
    \reentrant
    \brief The QDomEntityReference class represents an XML entity reference.

    \inmodule QtXml
    \ingroup xml-tools

    A QDomEntityReference object may be inserted into the DOM tree
    when an entity reference is in the source document, or when the
    user wishes to insert an entity reference.

    Note that character references and references to predefined
    entities are expanded by the XML processor so that characters are
    represented by their Unicode equivalent rather than by an entity
    reference.

    Moreover, the XML processor may completely expand references to
    entities while building the DOM tree, instead of providing
    QDomEntityReference objects.

    If it does provide such objects, then for a given entity reference
    node, it may be that there is no entity node representing the
    referenced entity; but if such an entity exists, then the child
    list of the entity reference node is the same as that of the
    entity  node. As with the entity node, all descendants of the
    entity reference are read-only.

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs an empty entity reference. Use
    QDomDocument::createEntityReference() to create a entity reference
    with content.
*/
QDomEntityReference::QDomEntityReference()
    : QDomNode()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomEntityReference::QDomEntityReference(const QDomEntityReference& x)
    : QDomNode(x)
{
}

QDomEntityReference::QDomEntityReference(QDomEntityReferencePrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this entity reference.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomEntityReference& QDomEntityReference::operator= (const QDomEntityReference& x)
{
    return (QDomEntityReference&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomEntityReference::nodeType() const

    Returns \c EntityReference.
*/

/**************************************************************
 *
 * QDomProcessingInstructionPrivate
 *
 **************************************************************/

QDomProcessingInstructionPrivate::QDomProcessingInstructionPrivate(QDomDocumentPrivate* d,
        QDomNodePrivate* parent, const QString& target, const QString& data)
    : QDomNodePrivate(d, parent)
{
    name = target;
    value = data;
}

QDomProcessingInstructionPrivate::QDomProcessingInstructionPrivate(QDomProcessingInstructionPrivate* n, bool deep)
    : QDomNodePrivate(n, deep)
{
}


QDomNodePrivate* QDomProcessingInstructionPrivate::cloneNode(bool deep)
{
    QDomNodePrivate* p = new QDomProcessingInstructionPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

void QDomProcessingInstructionPrivate::save(QTextStream& s, int, int) const
{
    s << "<?" << name << ' ' << value << "?>" << endl;
}

/**************************************************************
 *
 * QDomProcessingInstruction
 *
 **************************************************************/

/*!
    \class QDomProcessingInstruction
    \reentrant
    \brief The QDomProcessingInstruction class represents an XML processing
    instruction.

    \inmodule QtXml
    \ingroup xml-tools

    Processing instructions are used in XML to keep processor-specific
    information in the text of the document.

    The XML declaration that appears at the top of an XML document,
    typically \tt{<?xml version='1.0' encoding='UTF-8'?>}, is treated by QDom as a
    processing instruction. This is unfortunate, since the XML declaration is
    not a processing instruction; among other differences, it cannot be
    inserted into a document anywhere but on the first line.

    Do not use this function to create an xml declaration, since although it
    has the same syntax as a processing instruction, it isn't, and might not
    be treated by QDom as such.

    The content of the processing instruction is retrieved with data()
    and set with setData(). The processing instruction's target is
    retrieved with target().

    For further information about the Document Object Model see
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}.
    For a more general introduction of the DOM implementation see the
    QDomDocument documentation.
*/

/*!
    Constructs an empty processing instruction. Use
    QDomDocument::createProcessingInstruction() to create a processing
    instruction with content.
*/
QDomProcessingInstruction::QDomProcessingInstruction()
    : QDomNode()
{
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomProcessingInstruction::QDomProcessingInstruction(const QDomProcessingInstruction& x)
    : QDomNode(x)
{
}

QDomProcessingInstruction::QDomProcessingInstruction(QDomProcessingInstructionPrivate* n)
    : QDomNode(n)
{
}

/*!
    Assigns \a x to this processing instruction.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomProcessingInstruction& QDomProcessingInstruction::operator= (const QDomProcessingInstruction& x)
{
    return (QDomProcessingInstruction&) QDomNode::operator=(x);
}

/*!
    \fn QDomNode::NodeType QDomProcessingInstruction::nodeType() const

    Returns \c ProcessingInstructionNode.
*/

/*!
    Returns the target of this processing instruction.

    \sa data()
*/
QString QDomProcessingInstruction::target() const
{
    if (!impl)
        return QString();
    return impl->nodeName();
}

/*!
    Returns the content of this processing instruction.

    \sa setData() target()
*/
QString QDomProcessingInstruction::data() const
{
    if (!impl)
        return QString();
    return impl->nodeValue();
}

/*!
    Sets the data contained in the processing instruction to \a d.

    \sa data()
*/
void QDomProcessingInstruction::setData(const QString& d)
{
    if (!impl)
        return;
    impl->setNodeValue(d);
}

/**************************************************************
 *
 * QDomDocumentPrivate
 *
 **************************************************************/

QDomDocumentPrivate::QDomDocumentPrivate()
    : QDomNodePrivate(0),
      impl(new QDomImplementationPrivate),
      nodeListTime(1)
{
    type = new QDomDocumentTypePrivate(this, this);
    type->ref.deref();

    name = QLatin1String("#document");
}

QDomDocumentPrivate::QDomDocumentPrivate(const QString& aname)
    : QDomNodePrivate(0),
      impl(new QDomImplementationPrivate),
      nodeListTime(1)
{
    type = new QDomDocumentTypePrivate(this, this);
    type->ref.deref();
    type->name = aname;

    name = QLatin1String("#document");
}

QDomDocumentPrivate::QDomDocumentPrivate(QDomDocumentTypePrivate* dt)
    : QDomNodePrivate(0),
      impl(new QDomImplementationPrivate),
      nodeListTime(1)
{
    if (dt != 0) {
        type = dt;
    } else {
        type = new QDomDocumentTypePrivate(this, this);
        type->ref.deref();
    }

    name = QLatin1String("#document");
}

QDomDocumentPrivate::QDomDocumentPrivate(QDomDocumentPrivate* n, bool deep)
    : QDomNodePrivate(n, deep),
      impl(n->impl->clone()),
      nodeListTime(1)
{
    type = static_cast<QDomDocumentTypePrivate*>(n->type->cloneNode());
    type->setParent(this);
}

QDomDocumentPrivate::~QDomDocumentPrivate()
{
}

void QDomDocumentPrivate::clear()
{
    impl.reset();
    type.reset();
    QDomNodePrivate::clear();
}

static void initializeReader(QXmlSimpleReader &reader, bool namespaceProcessing)
{
    reader.setFeature(QLatin1String("http://xml.org/sax/features/namespaces"), namespaceProcessing);
    reader.setFeature(QLatin1String("http://xml.org/sax/features/namespace-prefixes"), !namespaceProcessing);
    reader.setFeature(QLatin1String("http://trolltech.com/xml/features/report-whitespace-only-CharData"), false); // Shouldn't change in Qt 4
}

bool QDomDocumentPrivate::setContent(QXmlInputSource *source, bool namespaceProcessing, QString *errorMsg, int *errorLine, int *errorColumn)
{
    QXmlSimpleReader reader;
    initializeReader(reader, namespaceProcessing);
    return setContent(source, &reader, errorMsg, errorLine, errorColumn);
}

bool QDomDocumentPrivate::setContent(QXmlInputSource *source, QXmlReader *reader, QString *errorMsg, int *errorLine, int *errorColumn)
{
    clear();
    impl = new QDomImplementationPrivate;
    type = new QDomDocumentTypePrivate(this, this);
    type->ref.deref();

    bool namespaceProcessing = reader->feature(QLatin1String("http://xml.org/sax/features/namespaces"))
        && !reader->feature(QLatin1String("http://xml.org/sax/features/namespace-prefixes"));

    QDomHandler hnd(this, namespaceProcessing);
    reader->setContentHandler(&hnd);
    reader->setErrorHandler(&hnd);
    reader->setLexicalHandler(&hnd);
    reader->setDeclHandler(&hnd);
    reader->setDTDHandler(&hnd);

    if (!reader->parse(source)) {
        if (errorMsg)
            *errorMsg = hnd.errorMsg;
        if (errorLine)
            *errorLine = hnd.errorLine;
        if (errorColumn)
            *errorColumn = hnd.errorColumn;
        return false;
    }

    return true;
}

QDomNodePrivate* QDomDocumentPrivate::cloneNode(bool deep)
{
    QDomNodePrivate *p = new QDomDocumentPrivate(this, deep);
    // We are not interested in this node
    p->ref.deref();
    return p;
}

QDomElementPrivate* QDomDocumentPrivate::documentElement()
{
    QDomNodePrivate *p = first;
    while (p && !p->isElement())
        p = p->next;

    return static_cast<QDomElementPrivate *>(p);
}

QDomElementPrivate* QDomDocumentPrivate::createElement(const QString &tagName)
{
    bool ok;
    QString fixedName = fixedXmlName(tagName, &ok);
    if (!ok)
        return 0;

    QDomElementPrivate *e = new QDomElementPrivate(this, 0, fixedName);
    e->ref.deref();
    return e;
}

QDomElementPrivate* QDomDocumentPrivate::createElementNS(const QString &nsURI, const QString &qName)
{
    bool ok;
    QString fixedName = fixedXmlName(qName, &ok, true);
    if (!ok)
        return 0;

    QDomElementPrivate *e = new QDomElementPrivate(this, 0, nsURI, fixedName);
    e->ref.deref();
    return e;
}

QDomDocumentFragmentPrivate* QDomDocumentPrivate::createDocumentFragment()
{
    QDomDocumentFragmentPrivate *f = new QDomDocumentFragmentPrivate(this, (QDomNodePrivate*)0);
    f->ref.deref();
    return f;
}

QDomTextPrivate* QDomDocumentPrivate::createTextNode(const QString &data)
{
    bool ok;
    QString fixedData = fixedCharData(data, &ok);
    if (!ok)
        return 0;

    QDomTextPrivate *t = new QDomTextPrivate(this, 0, fixedData);
    t->ref.deref();
    return t;
}

QDomCommentPrivate* QDomDocumentPrivate::createComment(const QString &data)
{
    bool ok;
    QString fixedData = fixedComment(data, &ok);
    if (!ok)
        return 0;

    QDomCommentPrivate *c = new QDomCommentPrivate(this, 0, fixedData);
    c->ref.deref();
    return c;
}

QDomCDATASectionPrivate* QDomDocumentPrivate::createCDATASection(const QString &data)
{
    bool ok;
    QString fixedData = fixedCDataSection(data, &ok);
    if (!ok)
        return 0;

    QDomCDATASectionPrivate *c = new QDomCDATASectionPrivate(this, 0, fixedData);
    c->ref.deref();
    return c;
}

QDomProcessingInstructionPrivate* QDomDocumentPrivate::createProcessingInstruction(const QString &target,
                                                                                   const QString &data)
{
    bool ok;
    QString fixedData = fixedPIData(data, &ok);
    if (!ok)
        return 0;
    // [17] PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))
    QString fixedTarget = fixedXmlName(target, &ok);
    if (!ok)
        return 0;

    QDomProcessingInstructionPrivate *p = new QDomProcessingInstructionPrivate(this, 0, fixedTarget, fixedData);
    p->ref.deref();
    return p;
}
QDomAttrPrivate* QDomDocumentPrivate::createAttribute(const QString &aname)
{
    bool ok;
    QString fixedName = fixedXmlName(aname, &ok);
    if (!ok)
        return 0;

    QDomAttrPrivate *a = new QDomAttrPrivate(this, 0, fixedName);
    a->ref.deref();
    return a;
}

QDomAttrPrivate* QDomDocumentPrivate::createAttributeNS(const QString &nsURI, const QString &qName)
{
    bool ok;
    QString fixedName = fixedXmlName(qName, &ok, true);
    if (!ok)
        return 0;

    QDomAttrPrivate *a = new QDomAttrPrivate(this, 0, nsURI, fixedName);
    a->ref.deref();
    return a;
}

QDomEntityReferencePrivate* QDomDocumentPrivate::createEntityReference(const QString &aname)
{
    bool ok;
    QString fixedName = fixedXmlName(aname, &ok);
    if (!ok)
        return 0;

    QDomEntityReferencePrivate *e = new QDomEntityReferencePrivate(this, 0, fixedName);
    e->ref.deref();
    return e;
}

QDomNodePrivate* QDomDocumentPrivate::importNode(const QDomNodePrivate *importedNode, bool deep)
{
    QDomNodePrivate *node = 0;
    switch (importedNode->nodeType()) {
        case QDomNode::AttributeNode:
            node = new QDomAttrPrivate((QDomAttrPrivate*)importedNode, true);
            break;
        case QDomNode::DocumentFragmentNode:
            node = new QDomDocumentFragmentPrivate((QDomDocumentFragmentPrivate*)importedNode, deep);
            break;
        case QDomNode::ElementNode:
            node = new QDomElementPrivate((QDomElementPrivate*)importedNode, deep);
            break;
        case QDomNode::EntityNode:
            node = new QDomEntityPrivate((QDomEntityPrivate*)importedNode, deep);
            break;
        case QDomNode::EntityReferenceNode:
            node = new QDomEntityReferencePrivate((QDomEntityReferencePrivate*)importedNode, false);
            break;
        case QDomNode::NotationNode:
            node = new QDomNotationPrivate((QDomNotationPrivate*)importedNode, deep);
            break;
        case QDomNode::ProcessingInstructionNode:
            node = new QDomProcessingInstructionPrivate((QDomProcessingInstructionPrivate*)importedNode, deep);
            break;
        case QDomNode::TextNode:
            node = new QDomTextPrivate((QDomTextPrivate*)importedNode, deep);
            break;
        case QDomNode::CDATASectionNode:
            node = new QDomCDATASectionPrivate((QDomCDATASectionPrivate*)importedNode, deep);
            break;
        case QDomNode::CommentNode:
            node = new QDomCommentPrivate((QDomCommentPrivate*)importedNode, deep);
            break;
        default:
            break;
    }
    if (node) {
        node->setOwnerDocument(this);
        // The QDomNode constructor increases the refcount, so deref first to
        // keep refcount balanced.
        node->ref.deref();
    }
    return node;
}

void QDomDocumentPrivate::saveDocument(QTextStream& s, const int indent, QDomNode::EncodingPolicy encUsed) const
{
    const QDomNodePrivate* n = first;

    if(encUsed == QDomNode::EncodingFromDocument) {
#ifndef QT_NO_TEXTCODEC
        const QDomNodePrivate* n = first;

        QTextCodec *codec = 0;

        if (n && n->isProcessingInstruction() && n->nodeName() == QLatin1String("xml")) {
            // we have an XML declaration
            QString data = n->nodeValue();
            QRegExp encoding(QString::fromLatin1("encoding\\s*=\\s*((\"([^\"]*)\")|('([^']*)'))"));
            encoding.indexIn(data);
            QString enc = encoding.cap(3);
            if (enc.isEmpty())
                enc = encoding.cap(5);
            if (!enc.isEmpty())
                codec = QTextCodec::codecForName(enc.toLatin1().data());
        }
        if (!codec)
            codec = QTextCodec::codecForName("UTF-8");
        if (codec)
            s.setCodec(codec);
#endif
        bool doc = false;

        while (n) {
            if (!doc && !(n->isProcessingInstruction() && n->nodeName() == QLatin1String("xml"))) {
                // save doctype after XML declaration
                type->save(s, 0, indent);
                doc = true;
            }
            n->save(s, 0, indent);
            n = n->next;
        }
    }
    else {

        // Write out the XML declaration.
#ifdef QT_NO_TEXTCODEC
        const QLatin1String codecName("iso-8859-1");
#else
        const QTextCodec *const codec = s.codec();
        Q_ASSERT_X(codec, "QDomNode::save()", "A codec must be specified in the text stream.");
        const QByteArray codecName = codec->name();
#endif

        s << "<?xml version=\"1.0\" encoding=\""
          << codecName
          << "\"?>\n";

        //  Skip the first processing instruction by name "xml", if any such exists.
        const QDomNodePrivate* startNode = n;

        // First, we try to find the PI and sets the startNode to the one appearing after it.
        while (n) {
            if(n->isProcessingInstruction() && n->nodeName() == QLatin1String("xml")) {
                startNode = n->next;
                break;
            }
            else
                n = n->next;
        }

        // Now we serialize all the nodes after the faked XML declaration(the PI).
        while(startNode) {
            startNode->save(s, 0, indent);
            startNode = startNode->next;
        }
    }
}

/**************************************************************
 *
 * QDomDocument
 *
 **************************************************************/

#define IMPL ((QDomDocumentPrivate*)impl)

/*!
    \class QDomDocument
    \reentrant
    \brief The QDomDocument class represents an XML document.

    \inmodule QtXml

    \ingroup xml-tools

    The QDomDocument class represents the entire XML document.
    Conceptually, it is the root of the document tree, and provides
    the primary access to the document's data.

    Since elements, text nodes, comments, processing instructions,
    etc., cannot exist outside the context of a document, the document
    class also contains the factory functions needed to create these
    objects. The node objects created have an ownerDocument() function
    which associates them with the document within whose context they
    were created. The DOM classes that will be used most often are
    QDomNode, QDomDocument, QDomElement and QDomText.

    The parsed XML is represented internally by a tree of objects that
    can be accessed using the various QDom classes. All QDom classes
    only \e reference objects in the internal tree. The internal
    objects in the DOM tree will get deleted once the last QDom
    object referencing them and the QDomDocument itself are deleted.

    Creation of elements, text nodes, etc. is done using the various
    factory functions provided in this class. Using the default
    constructors of the QDom classes will only result in empty
    objects that cannot be manipulated or inserted into the Document.

    The QDomDocument class has several functions for creating document
    data, for example, createElement(), createTextNode(),
    createComment(), createCDATASection(),
    createProcessingInstruction(), createAttribute() and
    createEntityReference(). Some of these functions have versions
    that support namespaces, i.e. createElementNS() and
    createAttributeNS(). The createDocumentFragment() function is used
    to hold parts of the document; this is useful for manipulating for
    complex documents.

    The entire content of the document is set with setContent(). This
    function parses the string it is passed as an XML document and
    creates the DOM tree that represents the document. The root
    element is available using documentElement(). The textual
    representation of the document can be obtained using toString().

    \note The DOM tree might end up reserving a lot of memory if the XML
    document is big. For big XML documents, the QXmlStreamReader or the QXmlQuery
    classes might be better solutions.

    It is possible to insert a node from another document into the
    document using importNode().

    You can obtain a list of all the elements that have a particular
    tag with elementsByTagName() or with elementsByTagNameNS().

    The QDom classes are typically used as follows:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 16

    Once \c doc and \c elem go out of scope, the whole internal tree
    representing the XML document is deleted.

    To create a document using DOM use code like this:
    \snippet doc/src/snippets/code/src_xml_dom_qdom.cpp 17

    For further information about the Document Object Model see
    the Document Object Model (DOM)
    \l{http://www.w3.org/TR/REC-DOM-Level-1/}{Level 1} and
    \l{http://www.w3.org/TR/DOM-Level-2-Core/}{Level 2 Core}
    Specifications.

    \sa {DOM Bookmarks Example}, {Simple DOM Model Example}
*/


/*!
    Constructs an empty document.
*/
QDomDocument::QDomDocument()
{
    impl = 0;
}

/*!
    Creates a document and sets the name of the document type to \a
    name.
*/
QDomDocument::QDomDocument(const QString& name)
{
    // We take over ownership
    impl = new QDomDocumentPrivate(name);
}

/*!
    Creates a document with the document type \a doctype.

    \sa QDomImplementation::createDocumentType()
*/
QDomDocument::QDomDocument(const QDomDocumentType& doctype)
{
    impl = new QDomDocumentPrivate((QDomDocumentTypePrivate*)(doctype.impl));
}

/*!
    Constructs a copy of \a x.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomDocument::QDomDocument(const QDomDocument& x)
    : QDomNode(x)
{
}

QDomDocument::QDomDocument(QDomDocumentPrivate* x)
    : QDomNode(x)
{
}

/*!
    Assigns \a x to this DOM document.

    The data of the copy is shared (shallow copy): modifying one node
    will also change the other. If you want to make a deep copy, use
    cloneNode().
*/
QDomDocument& QDomDocument::operator= (const QDomDocument& x)
{
    return (QDomDocument&) QDomNode::operator=(x);
}

/*!
    Destroys the object and frees its resources.
*/
QDomDocument::~QDomDocument()
{
}

/*!
    \overload

    This function reads the XML document from the string \a text, returning
    true if the content was successfully parsed; otherwise returns false.
    Since \a text is already a Unicode string, no encoding detection
    is done.
*/
bool QDomDocument::setContent(const QString& text, bool namespaceProcessing, QString *errorMsg, int *errorLine, int *errorColumn)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    QXmlInputSource source;
    source.setData(text);
    return IMPL->setContent(&source, namespaceProcessing, errorMsg, errorLine, errorColumn);
}

/*!
    \nonreentrant

    This function parses the XML document from the byte array \a
    data and sets it as the content of the document. It tries to
    detect the encoding of the document as required by the XML
    specification.

    If \a namespaceProcessing is true, the parser recognizes
    namespaces in the XML file and sets the prefix name, local name
    and namespace URI to appropriate values. If \a namespaceProcessing
    is false, the parser does no namespace processing when it reads
    the XML file.

    If a parse error occurs, this function returns false and the error
    message is placed in \c{*}\a{errorMsg}, the line number in
    \c{*}\a{errorLine} and the column number in \c{*}\a{errorColumn}
    (unless the associated pointer is set to 0); otherwise this
    function returns true. The various error messages are described in
    the QXmlParseException class documentation. Note that, if you
    want to display these error messages to your application's users,
    they will be displayed in English unless they are explicitly
    translated.

    If \a namespaceProcessing is true, the function QDomNode::prefix()
    returns a string for all elements and attributes. It returns an
    empty string if the element or attribute has no prefix.

    Text nodes consisting only of whitespace are stripped and won't
    appear in the QDomDocument. If this behavior is not desired,
    one can use the setContent() overload that allows a QXmlReader to be
    supplied.

    If \a namespaceProcessing is false, the functions
    QDomNode::prefix(), QDomNode::localName() and
    QDomNode::namespaceURI() return an empty string.

    Entity references are handled as follows:
    \list
    \o References to internal general entities and character entities occurring in the
        content are included. The result is a QDomText node with the references replaced
        by their corresponding entity values.
    \o References to parameter entities occurring in the internal subset are included.
        The result is a QDomDocumentType node which contains entity and notation declarations
        with the references replaced by their corresponding entity values.
    \o Any general parsed entity reference which is not defined in the internal subset and
        which occurs in the content is represented as a QDomEntityReference node.
    \o Any parsed entity reference which is not defined in the internal subset and which
        occurs outside of the content is replaced with an empty string.
    \o Any unparsed entity reference is replaced with an empty string.
    \endlist

    \sa QDomNode::namespaceURI() QDomNode::localName()
    QDomNode::prefix() QString::isNull() QString::isEmpty()
*/
bool QDomDocument::setContent(const QByteArray &data, bool namespaceProcessing, QString *errorMsg, int *errorLine, int *errorColumn)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    QBuffer buf;
    buf.setData(data);
    QXmlInputSource source(&buf);
    return IMPL->setContent(&source, namespaceProcessing, errorMsg, errorLine, errorColumn);
}

/*!
    \overload

    This function reads the XML document from the IO device \a dev, returning
    true if the content was successfully parsed; otherwise returns false.
*/
bool QDomDocument::setContent(QIODevice* dev, bool namespaceProcessing, QString *errorMsg, int *errorLine, int *errorColumn)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    QXmlInputSource source(dev);
    return IMPL->setContent(&source, namespaceProcessing, errorMsg, errorLine, errorColumn);
}

/*!
    \overload
    \since 4.5

    This function reads the XML document from the QXmlInputSource \a source,
    returning true if the content was successfully parsed; otherwise returns false.

*/
bool QDomDocument::setContent(QXmlInputSource *source, bool namespaceProcessing, QString *errorMsg, int *errorLine, int *errorColumn )
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    QXmlSimpleReader reader;
    initializeReader(reader, namespaceProcessing);
    return IMPL->setContent(source, &reader, errorMsg, errorLine, errorColumn);
}

/*!
    \overload

    This function reads the XML document from the string \a text, returning
    true if the content was successfully parsed; otherwise returns false.
    Since \a text is already a Unicode string, no encoding detection
    is performed.

    No namespace processing is performed either.
*/
bool QDomDocument::setContent(const QString& text, QString *errorMsg, int *errorLine, int *errorColumn)
{
    return setContent(text, false, errorMsg, errorLine, errorColumn);
}

/*!
    \overload

    This function reads the XML document from the byte array \a buffer,
    returning true if the content was successfully parsed; otherwise returns
    false.

    No namespace processing is performed.
*/
bool QDomDocument::setContent(const QByteArray& buffer, QString *errorMsg, int *errorLine, int *errorColumn )
{
    return setContent(buffer, false, errorMsg, errorLine, errorColumn);
}

/*!
    \overload

    This function reads the XML document from the IO device \a dev, returning
    true if the content was successfully parsed; otherwise returns false.

    No namespace processing is performed.
*/
bool QDomDocument::setContent(QIODevice* dev, QString *errorMsg, int *errorLine, int *errorColumn )
{
    return setContent(dev, false, errorMsg, errorLine, errorColumn);
}

/*!
    \overload

    This function reads the XML document from the QXmlInputSource \a source and
    parses it with the QXmlReader \a reader, returning true if the content was
    successfully parsed; otherwise returns false.

    This function doesn't change the features of the \a reader. If you want to
    use certain features for parsing you can use this function to set up the
    reader appropriately.

    \sa QXmlSimpleReader
*/
bool QDomDocument::setContent(QXmlInputSource *source, QXmlReader *reader, QString *errorMsg, int *errorLine, int *errorColumn )
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return IMPL->setContent(source, reader, errorMsg, errorLine, errorColumn);
}

/*!
    Converts the parsed document back to its textual representation.

    This function uses \a indent as the amount of space to indent
    subelements.

    If \a indent is -1, no whitespace at all is added.
*/
QString QDomDocument::toString(int indent) const
{
    QString str;
    QTextStream s(&str, QIODevice::WriteOnly);
    save(s, indent);
    return str;
}

/*!
    Converts the parsed document back to its textual representation
    and returns a QByteArray containing the data encoded as UTF-8.

    This function uses \a indent as the amount of space to indent
    subelements.

    \sa toString()
*/
QByteArray QDomDocument::toByteArray(int indent) const
{
    // ### if there is an encoding specified in the xml declaration, this
    // encoding declaration should be changed to utf8
    return toString(indent).toUtf8();
}


/*!
    Returns the document type of this document.
*/
QDomDocumentType QDomDocument::doctype() const
{
    if (!impl)
        return QDomDocumentType();
    return QDomDocumentType(IMPL->doctype());
}

/*!
    Returns a QDomImplementation object.
*/
QDomImplementation QDomDocument::implementation() const
{
    if (!impl)
        return QDomImplementation();
    return QDomImplementation(IMPL->implementation());
}

/*!
    Returns the root element of the document.
*/
QDomElement QDomDocument::documentElement() const
{
    if (!impl)
        return QDomElement();
    return QDomElement(IMPL->documentElement());
}

/*!
    Creates a new element called \a tagName that can be inserted into
    the DOM tree, e.g. using QDomNode::appendChild().

    If \a tagName is not a valid XML name, the behavior of this function is governed
    by QDomImplementation::InvalidDataPolicy.

    \sa createElementNS() QDomNode::appendChild() QDomNode::insertBefore()
    QDomNode::insertAfter()
*/
QDomElement QDomDocument::createElement(const QString& tagName)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomElement(IMPL->createElement(tagName));
}

/*!
    Creates a new document fragment, that can be used to hold parts of
    the document, e.g. when doing complex manipulations of the
    document tree.
*/
QDomDocumentFragment QDomDocument::createDocumentFragment()
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomDocumentFragment(IMPL->createDocumentFragment());
}

/*!
    Creates a text node for the string \a value that can be inserted
    into the document tree, e.g. using QDomNode::appendChild().

    If \a value contains characters which cannot be stored as character
    data of an XML document (even in the form of character references), the
    behavior of this function is governed by QDomImplementation::InvalidDataPolicy.

    \sa QDomNode::appendChild() QDomNode::insertBefore() QDomNode::insertAfter()
*/
QDomText QDomDocument::createTextNode(const QString& value)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomText(IMPL->createTextNode(value));
}

/*!
    Creates a new comment for the string \a value that can be inserted
    into the document, e.g. using QDomNode::appendChild().

    If \a value contains characters which cannot be stored in an XML comment,
    the behavior of this function is governed by QDomImplementation::InvalidDataPolicy.

    \sa QDomNode::appendChild() QDomNode::insertBefore() QDomNode::insertAfter()
*/
QDomComment QDomDocument::createComment(const QString& value)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomComment(IMPL->createComment(value));
}

/*!
    Creates a new CDATA section for the string \a value that can be
    inserted into the document, e.g. using QDomNode::appendChild().

    If \a value contains characters which cannot be stored in a CDATA section,
    the behavior of this function is governed by
    QDomImplementation::InvalidDataPolicy.

    \sa QDomNode::appendChild() QDomNode::insertBefore() QDomNode::insertAfter()
*/
QDomCDATASection QDomDocument::createCDATASection(const QString& value)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomCDATASection(IMPL->createCDATASection(value));
}

/*!
    Creates a new processing instruction that can be inserted into the
    document, e.g. using QDomNode::appendChild(). This function sets
    the target for the processing instruction to \a target and the
    data to \a data.

    If \a target is not a valid XML name, or data if contains characters which cannot
    appear in a processing instruction, the behavior of this function is governed by
    QDomImplementation::InvalidDataPolicy.

    \sa QDomNode::appendChild() QDomNode::insertBefore() QDomNode::insertAfter()
*/
QDomProcessingInstruction QDomDocument::createProcessingInstruction(const QString& target,
                                                                    const QString& data)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomProcessingInstruction(IMPL->createProcessingInstruction(target, data));
}


/*!
    Creates a new attribute called \a name that can be inserted into
    an element, e.g. using QDomElement::setAttributeNode().

    If \a name is not a valid XML name, the behavior of this function is governed by
    QDomImplementation::InvalidDataPolicy.

    \sa createAttributeNS()
*/
QDomAttr QDomDocument::createAttribute(const QString& name)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomAttr(IMPL->createAttribute(name));
}

/*!
    Creates a new entity reference called \a name that can be inserted
    into the document, e.g. using QDomNode::appendChild().

    If \a name is not a valid XML name, the behavior of this function is governed by
    QDomImplementation::InvalidDataPolicy.

    \sa QDomNode::appendChild() QDomNode::insertBefore() QDomNode::insertAfter()
*/
QDomEntityReference QDomDocument::createEntityReference(const QString& name)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomEntityReference(IMPL->createEntityReference(name));
}

/*!
    Returns a QDomNodeList, that contains all the elements in the
    document with the name \a tagname. The order of the node list is
    the order they are encountered in a preorder traversal of the
    element tree.

    \sa elementsByTagNameNS() QDomElement::elementsByTagName()
*/
QDomNodeList QDomDocument::elementsByTagName(const QString& tagname) const
{
    return QDomNodeList(new QDomNodeListPrivate(impl, tagname));
}

/*!
    Imports the node \a importedNode from another document to this
    document. \a importedNode remains in the original document; this
    function creates a copy that can be used within this document.

    This function returns the imported node that belongs to this
    document. The returned node has no parent. It is not possible to
    import QDomDocument and QDomDocumentType nodes. In those cases
    this function returns a \link QDomNode::isNull() null node\endlink.

    If \a deep is true, this function imports not only the node \a
    importedNode but its whole subtree; if it is false, only the \a
    importedNode is imported. The argument \a deep has no effect on
    QDomAttr and QDomEntityReference nodes, since the descendants of
    QDomAttr nodes are always imported and those of
    QDomEntityReference nodes are never imported.

    The behavior of this function is slightly different depending on
    the node types:
    \table
    \header \i Node Type \i Behavior
    \row \i QDomAttr
         \i The owner element is set to 0 and the specified flag is
            set to true in the generated attribute. The whole subtree
            of \a importedNode is always imported for attribute nodes:
            \a deep has no effect.
    \row \i QDomDocument
         \i Document nodes cannot be imported.
    \row \i QDomDocumentFragment
         \i If \a deep is true, this function imports the whole
            document fragment; otherwise it only generates an empty
            document fragment.
    \row \i QDomDocumentType
         \i Document type nodes cannot be imported.
    \row \i QDomElement
         \i Attributes for which QDomAttr::specified() is true are
            also imported, other attributes are not imported. If \a
            deep is true, this function also imports the subtree of \a
            importedNode; otherwise it imports only the element node
            (and some attributes, see above).
    \row \i QDomEntity
         \i Entity nodes can be imported, but at the moment there is
            no way to use them since the document type is read-only in
            DOM level 2.
    \row \i QDomEntityReference
         \i Descendants of entity reference nodes are never imported:
            \a deep has no effect.
    \row \i QDomNotation
         \i Notation nodes can be imported, but at the moment there is
            no way to use them since the document type is read-only in
            DOM level 2.
    \row \i QDomProcessingInstruction
         \i The target and value of the processing instruction is
            copied to the new node.
    \row \i QDomText
         \i The text is copied to the new node.
    \row \i QDomCDATASection
         \i The text is copied to the new node.
    \row \i QDomComment
         \i The text is copied to the new node.
    \endtable

    \sa QDomElement::setAttribute() QDomNode::insertBefore()
        QDomNode::insertAfter() QDomNode::replaceChild() QDomNode::removeChild()
        QDomNode::appendChild()
*/
QDomNode QDomDocument::importNode(const QDomNode& importedNode, bool deep)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomNode(IMPL->importNode(importedNode.impl, deep));
}

/*!
    Creates a new element with namespace support that can be inserted
    into the DOM tree. The name of the element is \a qName and the
    namespace URI is \a nsURI. This function also sets
    QDomNode::prefix() and QDomNode::localName() to appropriate values
    (depending on \a qName).

    If \a qName is an empty string, returns a null element regardless of
    whether the invalid data policy is set.

    \sa createElement()
*/
QDomElement QDomDocument::createElementNS(const QString& nsURI, const QString& qName)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomElement(IMPL->createElementNS(nsURI, qName));
}

/*!
    Creates a new attribute with namespace support that can be
    inserted into an element. The name of the attribute is \a qName
    and the namespace URI is \a nsURI. This function also sets
    QDomNode::prefix() and QDomNode::localName() to appropriate values
    (depending on \a qName).

    If \a qName is not a valid XML name, the behavior of this function is governed by
    QDomImplementation::InvalidDataPolicy.

    \sa createAttribute()
*/
QDomAttr QDomDocument::createAttributeNS(const QString& nsURI, const QString& qName)
{
    if (!impl)
        impl = new QDomDocumentPrivate();
    return QDomAttr(IMPL->createAttributeNS(nsURI, qName));
}

/*!
    Returns a QDomNodeList that contains all the elements in the
    document with the local name \a localName and a namespace URI of
    \a nsURI. The order of the node list is the order they are
    encountered in a preorder traversal of the element tree.

    \sa elementsByTagName() QDomElement::elementsByTagNameNS()
*/
QDomNodeList QDomDocument::elementsByTagNameNS(const QString& nsURI, const QString& localName)
{
    return QDomNodeList(new QDomNodeListPrivate(impl, nsURI, localName));
}

/*!
    Returns the element whose ID is equal to \a elementId. If no
    element with the ID was found, this function returns a \link
    QDomNode::isNull() null element\endlink.

    Since the QDomClasses do not know which attributes are element
    IDs, this function returns always a \link QDomNode::isNull() null
    element\endlink. This may change in a future version.
*/
QDomElement QDomDocument::elementById(const QString& /*elementId*/)
{
    qWarning("elementById() is not implemented and will always return a null node.");
    return QDomElement();
}

/*!
    \fn QDomNode::NodeType QDomDocument::nodeType() const

    Returns \c DocumentNode.
*/

#undef IMPL

/**************************************************************
 *
 * Node casting functions
 *
 **************************************************************/

/*!
    Converts a QDomNode into a QDomAttr. If the node is not an
    attribute, the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isAttr()
*/
QDomAttr QDomNode::toAttr() const
{
    if (impl && impl->isAttr())
        return QDomAttr(((QDomAttrPrivate*)impl));
    return QDomAttr();
}

/*!
    Converts a QDomNode into a QDomCDATASection. If the node is not a
    CDATA section, the returned object will be \link
    QDomNode::isNull() null\endlink.

    \sa isCDATASection()
*/
QDomCDATASection QDomNode::toCDATASection() const
{
    if (impl && impl->isCDATASection())
        return QDomCDATASection(((QDomCDATASectionPrivate*)impl));
    return QDomCDATASection();
}

/*!
    Converts a QDomNode into a QDomDocumentFragment. If the node is
    not a document fragment the returned object will be \link
    QDomNode::isNull() null\endlink.

    \sa isDocumentFragment()
*/
QDomDocumentFragment QDomNode::toDocumentFragment() const
{
    if (impl && impl->isDocumentFragment())
        return QDomDocumentFragment(((QDomDocumentFragmentPrivate*)impl));
    return QDomDocumentFragment();
}

/*!
    Converts a QDomNode into a QDomDocument. If the node is not a
    document the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isDocument()
*/
QDomDocument QDomNode::toDocument() const
{
    if (impl && impl->isDocument())
        return QDomDocument(((QDomDocumentPrivate*)impl));
    return QDomDocument();
}

/*!
    Converts a QDomNode into a QDomDocumentType. If the node is not a
    document type the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isDocumentType()
*/
QDomDocumentType QDomNode::toDocumentType() const
{
    if (impl && impl->isDocumentType())
        return QDomDocumentType(((QDomDocumentTypePrivate*)impl));
    return QDomDocumentType();
}

/*!
    Converts a QDomNode into a QDomElement. If the node is not an
    element the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isElement()
*/
QDomElement QDomNode::toElement() const
{
    if (impl && impl->isElement())
        return QDomElement(((QDomElementPrivate*)impl));
    return QDomElement();
}

/*!
    Converts a QDomNode into a QDomEntityReference. If the node is not
    an entity reference, the returned object will be \link
    QDomNode::isNull() null\endlink.

    \sa isEntityReference()
*/
QDomEntityReference QDomNode::toEntityReference() const
{
    if (impl && impl->isEntityReference())
        return QDomEntityReference(((QDomEntityReferencePrivate*)impl));
    return QDomEntityReference();
}

/*!
    Converts a QDomNode into a QDomText. If the node is not a text,
    the returned object will be \link QDomNode::isNull() null\endlink.

    \sa isText()
*/
QDomText QDomNode::toText() const
{
    if (impl && impl->isText())
        return QDomText(((QDomTextPrivate*)impl));
    return QDomText();
}

/*!
    Converts a QDomNode into a QDomEntity. If the node is not an
    entity the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isEntity()
*/
QDomEntity QDomNode::toEntity() const
{
    if (impl && impl->isEntity())
        return QDomEntity(((QDomEntityPrivate*)impl));
    return QDomEntity();
}

/*!
    Converts a QDomNode into a QDomNotation. If the node is not a
    notation the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isNotation()
*/
QDomNotation QDomNode::toNotation() const
{
    if (impl && impl->isNotation())
        return QDomNotation(((QDomNotationPrivate*)impl));
    return QDomNotation();
}

/*!
    Converts a QDomNode into a QDomProcessingInstruction. If the node
    is not a processing instruction the returned object will be \link
    QDomNode::isNull() null\endlink.

    \sa isProcessingInstruction()
*/
QDomProcessingInstruction QDomNode::toProcessingInstruction() const
{
    if (impl && impl->isProcessingInstruction())
        return QDomProcessingInstruction(((QDomProcessingInstructionPrivate*)impl));
    return QDomProcessingInstruction();
}

/*!
    Converts a QDomNode into a QDomCharacterData. If the node is not a
    character data node the returned object will be \link
    QDomNode::isNull() null\endlink.

    \sa isCharacterData()
*/
QDomCharacterData QDomNode::toCharacterData() const
{
    if (impl && impl->isCharacterData())
        return QDomCharacterData(((QDomCharacterDataPrivate*)impl));
    return QDomCharacterData();
}

/*!
    Converts a QDomNode into a QDomComment. If the node is not a
    comment the returned object will be \link QDomNode::isNull()
    null\endlink.

    \sa isComment()
*/
QDomComment QDomNode::toComment() const
{
    if (impl && impl->isComment())
        return QDomComment(((QDomCommentPrivate*)impl));
    return QDomComment();
}

/**************************************************************
 *
 * QDomHandler
 *
 **************************************************************/

QDomHandler::QDomHandler(QDomDocumentPrivate* adoc, bool namespaceProcessing)
    : errorLine(0), errorColumn(0), doc(adoc), node(adoc), cdata(false),
        nsProcessing(namespaceProcessing), locator(0)
{
}

QDomHandler::~QDomHandler()
{
}

bool QDomHandler::endDocument()
{
    // ### is this really necessary? (rms)
    if (node != doc)
        return false;
    return true;
}

bool QDomHandler::startDTD(const QString& name, const QString& publicId, const QString& systemId)
{
    doc->doctype()->name = name;
    doc->doctype()->publicId = publicId;
    doc->doctype()->systemId = systemId;
    return true;
}

bool QDomHandler::startElement(const QString& nsURI, const QString&, const QString& qName, const QXmlAttributes& atts)
{
    // tag name
    QDomNodePrivate* n;
    if (nsProcessing) {
        n = doc->createElementNS(nsURI, qName);
    } else {
        n = doc->createElement(qName);
    }

    if (!n)
        return false;

    n->setLocation(locator->lineNumber(), locator->columnNumber());

    node->appendChild(n);
    node = n;

    // attributes
    for (int i=0; i<atts.length(); i++)
    {
        if (nsProcessing) {
            ((QDomElementPrivate*)node)->setAttributeNS(atts.uri(i), atts.qName(i), atts.value(i));
        } else {
            ((QDomElementPrivate*)node)->setAttribute(atts.qName(i), atts.value(i));
        }
    }

    return true;
}

bool QDomHandler::endElement(const QString&, const QString&, const QString&)
{
    if (!node || node == doc)
        return false;
    node = node->parent();

    return true;
}

bool QDomHandler::characters(const QString&  ch)
{
    // No text as child of some document
    if (node == doc)
        return false;

    QScopedPointer<QDomNodePrivate> n;
    if (cdata) {
        n.reset(doc->createCDATASection(ch));
    } else if (!entityName.isEmpty()) {
        QScopedPointer<QDomEntityPrivate> e(new QDomEntityPrivate(doc, 0, entityName,
                QString(), QString(), QString()));
        e->value = ch;
        e->ref.deref();
        doc->doctype()->appendChild(e.data());
        e.take();
        n.reset(doc->createEntityReference(entityName));
    } else {
        n.reset(doc->createTextNode(ch));
    }
    n->setLocation(locator->lineNumber(), locator->columnNumber());
    node->appendChild(n.data());
    n.take();

    return true;
}

bool QDomHandler::processingInstruction(const QString& target, const QString& data)
{
    QDomNodePrivate *n;
    n = doc->createProcessingInstruction(target, data);
    if (n) {
        n->setLocation(locator->lineNumber(), locator->columnNumber());
        node->appendChild(n);
        return true;
    }
    else
        return false;
}

extern bool qt_xml_skipped_entity_in_content;
bool QDomHandler::skippedEntity(const QString& name)
{
    // we can only handle inserting entity references into content
    if (!qt_xml_skipped_entity_in_content)
        return true;

    QDomNodePrivate *n = doc->createEntityReference(name);
    n->setLocation(locator->lineNumber(), locator->columnNumber());
    node->appendChild(n);
    return true;
}

bool QDomHandler::fatalError(const QXmlParseException& exception)
{
    errorMsg = exception.message();
    errorLine =  exception.lineNumber();
    errorColumn =  exception.columnNumber();
    return QXmlDefaultHandler::fatalError(exception);
}

bool QDomHandler::startCDATA()
{
    cdata = true;
    return true;
}

bool QDomHandler::endCDATA()
{
    cdata = false;
    return true;
}

bool QDomHandler::startEntity(const QString &name)
{
    entityName = name;
    return true;
}

bool QDomHandler::endEntity(const QString &)
{
    entityName.clear();
    return true;
}

bool QDomHandler::comment(const QString& ch)
{
    QDomNodePrivate *n;
    n = doc->createComment(ch);
    n->setLocation(locator->lineNumber(), locator->columnNumber());
    node->appendChild(n);
    return true;
}

bool QDomHandler::unparsedEntityDecl(const QString &name, const QString &publicId, const QString &systemId, const QString &notationName)
{
    QDomEntityPrivate* e = new QDomEntityPrivate(doc, 0, name,
            publicId, systemId, notationName);
    // keep the refcount balanced: appendChild() does a ref anyway.
    e->ref.deref();
    doc->doctype()->appendChild(e);
    return true;
}

bool QDomHandler::externalEntityDecl(const QString &name, const QString &publicId, const QString &systemId)
{
    return unparsedEntityDecl(name, publicId, systemId, QString());
}

bool QDomHandler::notationDecl(const QString & name, const QString & publicId, const QString & systemId)
{
    QDomNotationPrivate* n = new QDomNotationPrivate(doc, 0, name, publicId, systemId);
    // keep the refcount balanced: appendChild() does a ref anyway.
    n->ref.deref();
    doc->doctype()->appendChild(n);
    return true;
}

void QDomHandler::setDocumentLocator(QXmlLocator *locator)
{
    this->locator = locator;
}

QT_END_NAMESPACE

#endif // QT_NO_DOM

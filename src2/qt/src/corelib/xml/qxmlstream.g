----------------------------------------------------------------------------
--
-- Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
-- Contact: http://www.qt-project.org/legal
--
-- This file is part of the QtCore module of the Qt Toolkit.
--
-- $QT_BEGIN_LICENSE:LGPL$
-- Commercial License Usage
-- Licensees holding valid commercial Qt licenses may use this file in
-- accordance with the commercial license agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and Digia.  For licensing terms and
-- conditions see http://qt.digia.com/licensing.  For further information
-- use the contact form at http://qt.digia.com/contact-us.
--
-- GNU Lesser General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU Lesser
-- General Public License version 2.1 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU Lesser General Public License version 2.1 requirements
-- will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
--
-- In addition, as a special exception, Digia gives you certain additional
-- rights.  These rights are described in the Digia Qt LGPL Exception
-- version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
--
-- GNU General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU
-- General Public License version 3.0 as published by the Free Software
-- Foundation and appearing in the file LICENSE.GPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU General Public License version 3.0 requirements will be
-- met: http://www.gnu.org/copyleft/gpl.html.
--
--
-- $QT_END_LICENSE$
--
----------------------------------------------------------------------------

%parser QXmlStreamReader_Table

%merged_output qxmlstream_p.h

%token NOTOKEN
%token SPACE " "
%token LANGLE "<"
%token RANGLE ">"
%token AMPERSAND "&"
%token HASH "#"
%token QUOTE "\'"
%token DBLQUOTE "\""
%token LBRACK "["
%token RBRACK "]"
%token LPAREN "("
%token RPAREN ")"
%token PIPE "|"
%token EQ "="
%token PERCENT "%"
%token SLASH "/"
%token COLON ":"
%token SEMICOLON ";"
%token COMMA ","
%token DASH "-"
%token PLUS "+"
%token STAR "*"
%token DOT "."
%token QUESTIONMARK "?"
%token BANG "!"
%token LETTER "[a-zA-Z]"
%token DIGIT "[0-9]"

-- after langle_bang
%token CDATA_START "[CDATA["
%token DOCTYPE "DOCTYPE"
%token ELEMENT "ELEMENT"
%token ATTLIST "ATTLIST"
%token ENTITY "ENTITY"
%token NOTATION "NOTATION"

-- entity decl
%token SYSTEM "SYSTEM"
%token PUBLIC "PUBLIC"
%token NDATA "NDATA"

-- default decl
%token REQUIRED "REQUIRED"
%token IMPLIED "IMPLIED"
%token FIXED "FIXED"

-- conent spec
%token EMPTY "EMPTY"
%token ANY "ANY"
%token PCDATA "PCDATA"

-- error
%token ERROR

-- entities
%token PARSE_ENTITY
%token ENTITY_DONE
%token UNRESOLVED_ENTITY

-- att type
%token CDATA "CDATA"
%token ID "ID"
%token IDREF "IDREF"
%token IDREFS "IDREFS"
%token ENTITY "ENTITY"
%token ENTITIES "ENTITIES"
%token NMTOKEN "NMTOKEN"
%token NMTOKENS "NMTOKENS"

-- xml declaration
%token XML "<?xml"
%token VERSION "version"

%nonassoc SHIFT_THERE
%nonassoc AMPERSAND
          BANG
          COLON
          COMMA
          DASH
          DBLQUOTE
          DIGIT
          DOT
          ENTITY_DONE
          EQ
          HASH
          LBRACK
          LETTER
          LPAREN
          PERCENT
          PIPE
          PLUS
          QUESTIONMARK
          QUOTE
          RANGLE
          RBRACK
          RPAREN
          SEMICOLON
          SLASH
          SPACE
          STAR

%start document

/.
template <typename T> class QXmlStreamSimpleStack {
    T *data;
    int tos, cap;
public:
    inline QXmlStreamSimpleStack():data(0), tos(-1), cap(0){}
    inline ~QXmlStreamSimpleStack(){ if (data) qFree(data); }

    inline void reserve(int extraCapacity) {
        if (tos + extraCapacity + 1 > cap) {
            cap = qMax(tos + extraCapacity + 1, cap << 1 );
            data = reinterpret_cast<T *>(qRealloc(data, cap * sizeof(T)));
            Q_CHECK_PTR(data);
        }
    }

    inline T &push() { reserve(1); return data[++tos]; }
    inline T &rawPush() { return data[++tos]; }
    inline const T &top() const { return data[tos]; }
    inline T &top() { return data[tos]; }
    inline T &pop() { return data[tos--]; }
    inline T &operator[](int index) { return data[index]; }
    inline const T &at(int index) const { return data[index]; }
    inline int size() const { return tos + 1; }
    inline void resize(int s) { tos = s - 1; }
    inline bool isEmpty() const { return tos < 0; }
    inline void clear() { tos = -1; }
};


class QXmlStream
{
    Q_DECLARE_TR_FUNCTIONS(QXmlStream)
};

class QXmlStreamPrivateTagStack {
public:
    struct NamespaceDeclaration
    {
        QStringRef prefix;
        QStringRef namespaceUri;
    };

    struct Tag
    {
        QStringRef name;
        QStringRef qualifiedName;
        NamespaceDeclaration namespaceDeclaration;
        int tagStackStringStorageSize;
        int namespaceDeclarationsSize;
    };


    QXmlStreamPrivateTagStack();
    QXmlStreamSimpleStack<NamespaceDeclaration> namespaceDeclarations;
    QString tagStackStringStorage;
    int tagStackStringStorageSize;
    bool tagsDone;

    inline QStringRef addToStringStorage(const QStringRef &s) {
        int pos = tagStackStringStorageSize;
	int sz = s.size();
	if (pos != tagStackStringStorage.size())
	    tagStackStringStorage.resize(pos);
        tagStackStringStorage.insert(pos, s.unicode(), sz);
        tagStackStringStorageSize += sz;
        return QStringRef(&tagStackStringStorage, pos, sz);
    }
    inline QStringRef addToStringStorage(const QString &s) {
        int pos = tagStackStringStorageSize;
	int sz = s.size();
	if (pos != tagStackStringStorage.size())
	    tagStackStringStorage.resize(pos);
        tagStackStringStorage.insert(pos, s.unicode(), sz);
        tagStackStringStorageSize += sz;
        return QStringRef(&tagStackStringStorage, pos, sz);
    }

    QXmlStreamSimpleStack<Tag> tagStack;


    inline Tag &tagStack_pop() {
        Tag& tag = tagStack.pop();
        tagStackStringStorageSize = tag.tagStackStringStorageSize;
        namespaceDeclarations.resize(tag.namespaceDeclarationsSize);
        tagsDone = tagStack.isEmpty();
        return tag;
    }
    inline Tag &tagStack_push() {
        Tag &tag = tagStack.push();
        tag.tagStackStringStorageSize = tagStackStringStorageSize;
        tag.namespaceDeclarationsSize = namespaceDeclarations.size();
        return tag;
    }
};


class QXmlStreamEntityResolver;
#ifndef QT_NO_XMLSTREAMREADER
class QXmlStreamReaderPrivate : public QXmlStreamReader_Table, public QXmlStreamPrivateTagStack{
    QXmlStreamReader *q_ptr;
    Q_DECLARE_PUBLIC(QXmlStreamReader)
public:
    QXmlStreamReaderPrivate(QXmlStreamReader *q);
    ~QXmlStreamReaderPrivate();
    void init();

    QByteArray rawReadBuffer;
    QByteArray dataBuffer;
    uchar firstByte;
    qint64 nbytesread;
    QString readBuffer;
    int readBufferPos;
    QXmlStreamSimpleStack<uint> putStack;
    struct Entity {
        Entity(const QString& str = QString())
            :value(str), external(false), unparsed(false), literal(false),
             hasBeenParsed(false), isCurrentlyReferenced(false){}
        static inline Entity createLiteral(const QString &entity)
            { Entity result(entity); result.literal = result.hasBeenParsed = true; return result; }
        QString value;
        uint external : 1;
        uint unparsed : 1;
        uint literal : 1;
        uint hasBeenParsed : 1;
        uint isCurrentlyReferenced : 1;
    };
    QHash<QString, Entity> entityHash;
    QHash<QString, Entity> parameterEntityHash;
    QXmlStreamSimpleStack<Entity *>entityReferenceStack;
    inline bool referenceEntity(Entity &entity) {
        if (entity.isCurrentlyReferenced) {
            raiseWellFormedError(QXmlStream::tr("Recursive entity detected."));
            return false;
        }
        entity.isCurrentlyReferenced = true;
        entityReferenceStack.push() = &entity;
        injectToken(ENTITY_DONE);
        return true;
    }


    QIODevice *device;
    bool deleteDevice;
#ifndef QT_NO_TEXTCODEC
    QTextCodec *codec;
    QTextDecoder *decoder;
#endif
    bool atEnd;

    /*!
      \sa setType()
     */
    QXmlStreamReader::TokenType type;
    QXmlStreamReader::Error error;
    QString errorString;
    QString unresolvedEntity;

    qint64 lineNumber, lastLineStart, characterOffset;


    void write(const QString &);
    void write(const char *);


    QXmlStreamAttributes attributes;
    QStringRef namespaceForPrefix(const QStringRef &prefix);
    void resolveTag();
    void resolvePublicNamespaces();
    void resolveDtd();
    uint resolveCharRef(int symbolIndex);
    bool checkStartDocument();
    void startDocument();
    void parseError();
    void checkPublicLiteral(const QStringRef &publicId);

    bool scanDtd;
    QStringRef lastAttributeValue;
    bool lastAttributeIsCData;
    struct DtdAttribute {
        QStringRef tagName;
        QStringRef attributeQualifiedName;
        QStringRef attributePrefix;
        QStringRef attributeName;
        QStringRef defaultValue;
        bool isCDATA;
        bool isNamespaceAttribute;
    };
    QXmlStreamSimpleStack<DtdAttribute> dtdAttributes;
    struct NotationDeclaration {
        QStringRef name;
        QStringRef publicId;
        QStringRef systemId;
    };
    QXmlStreamSimpleStack<NotationDeclaration> notationDeclarations;
    QXmlStreamNotationDeclarations publicNotationDeclarations;
    QXmlStreamNamespaceDeclarations publicNamespaceDeclarations;

    struct EntityDeclaration {
        QStringRef name;
        QStringRef notationName;
        QStringRef publicId;
        QStringRef systemId;
        QStringRef value;
        bool parameter;
        bool external;
        inline void clear() {
            name.clear();
            notationName.clear();
            publicId.clear();
            systemId.clear();
            value.clear();
            parameter = external = false;
        }
    };
    QXmlStreamSimpleStack<EntityDeclaration> entityDeclarations;
    QXmlStreamEntityDeclarations publicEntityDeclarations;

    QStringRef text;

    QStringRef prefix, namespaceUri, qualifiedName, name;
    QStringRef processingInstructionTarget, processingInstructionData;
    QStringRef dtdName, dtdPublicId, dtdSystemId;
    QStringRef documentVersion, documentEncoding;
    uint isEmptyElement : 1;
    uint isWhitespace : 1;
    uint isCDATA : 1;
    uint standalone : 1;
    uint hasCheckedStartDocument : 1;
    uint normalizeLiterals : 1;
    uint hasSeenTag : 1;
    uint inParseEntity : 1;
    uint referenceToUnparsedEntityDetected : 1;
    uint referenceToParameterEntityDetected : 1;
    uint hasExternalDtdSubset : 1;
    uint lockEncoding : 1;
    uint namespaceProcessing : 1;

    int resumeReduction;
    void resume(int rule);

    inline bool entitiesMustBeDeclared() const {
        return (!inParseEntity
                && (standalone
                    || (!referenceToUnparsedEntityDetected
                        && !referenceToParameterEntityDetected // Errata 13 as of 2006-04-25
                        && !hasExternalDtdSubset)));
    }

    // qlalr parser
    int tos;
    int stack_size;
    struct Value {
        int pos;
        int len;
        int prefix;
        ushort c;
    };

    Value *sym_stack;
    int *state_stack;
    inline void reallocateStack();
    inline Value &sym(int index) const
    { return sym_stack[tos + index - 1]; }
    QString textBuffer;
    inline void clearTextBuffer() {
        if (!scanDtd) {
            textBuffer.resize(0);
            textBuffer.reserve(256);
        }
    }
    struct Attribute {
        Value key;
        Value value;
    };
    QXmlStreamSimpleStack<Attribute> attributeStack;

    inline QStringRef symString(int index) {
        const Value &symbol = sym(index);
        return QStringRef(&textBuffer, symbol.pos + symbol.prefix, symbol.len - symbol.prefix);
    }
    inline QStringRef symName(int index) {
        const Value &symbol = sym(index);
        return QStringRef(&textBuffer, symbol.pos, symbol.len);
    }
    inline QStringRef symString(int index, int offset) {
        const Value &symbol = sym(index);
        return QStringRef(&textBuffer, symbol.pos + symbol.prefix + offset, symbol.len - symbol.prefix -  offset);
    }
    inline QStringRef symPrefix(int index) {
        const Value &symbol = sym(index);
        if (symbol.prefix)
            return QStringRef(&textBuffer, symbol.pos, symbol.prefix - 1);
        return QStringRef();
    }
    inline QStringRef symString(const Value &symbol) {
        return QStringRef(&textBuffer, symbol.pos + symbol.prefix, symbol.len - symbol.prefix);
    }
    inline QStringRef symName(const Value &symbol) {
        return QStringRef(&textBuffer, symbol.pos, symbol.len);
    }
    inline QStringRef symPrefix(const Value &symbol) {
        if (symbol.prefix)
            return QStringRef(&textBuffer, symbol.pos, symbol.prefix - 1);
        return QStringRef();
    }

    inline void clearSym() { Value &val = sym(1); val.pos = textBuffer.size(); val.len = 0; }


    short token;
    ushort token_char;

    uint filterCarriageReturn();
    inline uint getChar();
    inline uint peekChar();
    inline void putChar(uint c) { putStack.push() = c; }
    inline void putChar(QChar c) { putStack.push() =  c.unicode(); }
    void putString(const QString &s, int from = 0);
    void putStringLiteral(const QString &s);
    void putReplacement(const QString &s);
    void putReplacementInAttributeValue(const QString &s);
    ushort getChar_helper();

    bool scanUntil(const char *str, short tokenToInject = -1);
    bool scanString(const char *str, short tokenToInject, bool requireSpace = true);
    inline void injectToken(ushort tokenToInject) {
        putChar(int(tokenToInject) << 16);
    }

    QString resolveUndeclaredEntity(const QString &name);
    void parseEntity(const QString &value);
    QXmlStreamReaderPrivate *entityParser;

    bool scanAfterLangleBang();
    bool scanPublicOrSystem();
    bool scanNData();
    bool scanAfterDefaultDecl();
    bool scanAttType();


    // scan optimization functions. Not strictly necessary but LALR is
    // not very well suited for scanning fast
    int fastScanLiteralContent();
    int fastScanSpace();
    int fastScanContentCharList();
    int fastScanName(int *prefix = 0);
    inline int fastScanNMTOKEN();


    bool parse();
    inline void consumeRule(int);

    void raiseError(QXmlStreamReader::Error error, const QString& message = QString());
    void raiseWellFormedError(const QString &message);

    QXmlStreamEntityResolver *entityResolver;

private:
    /*! \internal
       Never assign to variable type directly. Instead use this function.

       This prevents errors from being ignored.
     */
    inline void setType(const QXmlStreamReader::TokenType t)
    {
        if(type != QXmlStreamReader::Invalid)
            type = t;
    }
};

bool QXmlStreamReaderPrivate::parse()
{
    // cleanup currently reported token

    switch (type) {
    case QXmlStreamReader::StartElement:
        name.clear();
        prefix.clear();
	qualifiedName.clear();
        namespaceUri.clear();
        if (publicNamespaceDeclarations.size())
            publicNamespaceDeclarations.clear();
        if (attributes.size())
            attributes.resize(0);
        if (isEmptyElement) {
            setType(QXmlStreamReader::EndElement);
            Tag &tag = tagStack_pop();
            namespaceUri = tag.namespaceDeclaration.namespaceUri;
            name = tag.name;
	    qualifiedName = tag.qualifiedName;
            isEmptyElement = false;
            return true;
        }
        clearTextBuffer();
        break;
    case QXmlStreamReader::EndElement:
        name.clear();
        prefix.clear();
	qualifiedName.clear();
        namespaceUri.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::DTD:
        publicNotationDeclarations.clear();
        publicEntityDeclarations.clear();
        dtdName.clear();
        dtdPublicId.clear();
        dtdSystemId.clear();
        // fall through
    case QXmlStreamReader::Comment:
    case QXmlStreamReader::Characters:
        isCDATA = false;
	isWhitespace = true;
        text.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::EntityReference:
        text.clear();
        name.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::ProcessingInstruction:
        processingInstructionTarget.clear();
        processingInstructionData.clear();
	clearTextBuffer();
        break;
    case QXmlStreamReader::NoToken:
    case QXmlStreamReader::Invalid:
        break;
    case QXmlStreamReader::StartDocument:
	lockEncoding = true;
        documentVersion.clear();
        documentEncoding.clear();
#ifndef QT_NO_TEXTCODEC
	if(decoder->hasFailure()) {
	    raiseWellFormedError(QXmlStream::tr("Encountered incorrectly encoded content."));
	    readBuffer.clear();
	    return false;
	}
#endif
        // fall through
    default:
        clearTextBuffer();
        ;
    }

    setType(QXmlStreamReader::NoToken);


    // the main parse loop
    int act, r;

    if (resumeReduction) {
        act = state_stack[tos-1];
        r = resumeReduction;
        resumeReduction = 0;
        goto ResumeReduction;
    }

    act = state_stack[tos];

    forever {
        if (token == -1 && - TERMINAL_COUNT != action_index[act]) {
            uint cu = getChar();
            token = NOTOKEN;
            token_char = cu;
            if (cu & 0xff0000) {
                token = cu >> 16;
            } else switch (token_char) {
            case 0xfffe:
            case 0xffff:
                token = ERROR;
                break;
            case '\r':
                token = SPACE;
                if (cu == '\r') {
                    if ((token_char = filterCarriageReturn())) {
                        ++lineNumber;
                        lastLineStart = characterOffset + readBufferPos;
                        break;
                    }
                } else {
                    break;
                }
                // fall through
            case '\0': {
                token = EOF_SYMBOL;
                if (!tagsDone && !inParseEntity) {
                    int a = t_action(act, token);
                    if (a < 0) {
                        raiseError(QXmlStreamReader::PrematureEndOfDocumentError);
                        return false;
                    }
                }

            } break;
            case '\n':
                ++lineNumber;
                lastLineStart = characterOffset + readBufferPos;
            case ' ':
            case '\t':
                token = SPACE;
                break;
            case '&':
                token = AMPERSAND;
                break;
            case '#':
                token = HASH;
                break;
            case '\'':
                token = QUOTE;
                break;
            case '\"':
                token = DBLQUOTE;
                break;
            case '<':
                token = LANGLE;
                break;
            case '>':
                token = RANGLE;
                break;
            case '[':
                token = LBRACK;
                break;
            case ']':
                token = RBRACK;
                break;
            case '(':
                token = LPAREN;
                break;
            case ')':
                token = RPAREN;
                break;
            case '|':
                token = PIPE;
                break;
            case '=':
                token = EQ;
                break;
            case '%':
                token = PERCENT;
                break;
            case '/':
                token = SLASH;
                break;
            case ':':
                token = COLON;
                break;
            case ';':
                token = SEMICOLON;
                break;
            case ',':
                token = COMMA;
                break;
            case '-':
                token = DASH;
                break;
            case '+':
                token = PLUS;
                break;
            case '*':
                token = STAR;
                break;
            case '.':
                token = DOT;
                break;
            case '?':
                token = QUESTIONMARK;
                break;
            case '!':
                token = BANG;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                token = DIGIT;
                break;
            default:
                if (cu < 0x20)
                    token = NOTOKEN;
                else
                    token = LETTER;
                break;
            }
        }

        act = t_action (act, token);
        if (act == ACCEPT_STATE) {
            // reset the parser in case someone resumes (process instructions can follow a valid document)
            tos = 0;
            state_stack[tos++] = 0;
            state_stack[tos] = 0;
            return true;
        } else if (act > 0) {
            if (++tos == stack_size-1)
                reallocateStack();

            Value &val = sym_stack[tos];
            val.c = token_char;
            val.pos = textBuffer.size();
            val.prefix = 0;
            val.len = 1;
            if (token_char)
                textBuffer += QChar(token_char);

            state_stack[tos] = act;
            token = -1;


        } else if (act < 0) {
            r = - act - 1;

#if defined (QLALR_DEBUG)
            int ridx = rule_index[r];
            printf ("%3d) %s ::=", r + 1, spell[rule_info[ridx]]);
            ++ridx;
            for (int i = ridx; i < ridx + rhs[r]; ++i) {
                int symbol = rule_info[i];
                if (const char *name = spell[symbol])
                    printf (" %s", name);
                else
                    printf (" #%d", symbol);
            }
            printf ("\n");
#endif

            tos -= rhs[r];
            act = state_stack[tos++];
        ResumeReduction:
            switch (r) {
./

document ::= PARSE_ENTITY content;
/.
        case $rule_number:
            setType(QXmlStreamReader::EndDocument);
        break;
./

document ::= prolog;
/.
        case $rule_number:
            if (type != QXmlStreamReader::Invalid) {
                if (hasSeenTag || inParseEntity) {
                    setType(QXmlStreamReader::EndDocument);
                } else {
                    raiseError(QXmlStreamReader::NotWellFormedError, QXmlStream::tr("Start tag expected."));
                    // reset the parser
                    tos = 0;
                    state_stack[tos++] = 0;
                    state_stack[tos] = 0;
                    return false;
                }
            }
        break;
./


prolog ::= prolog stag content etag;
prolog ::= prolog empty_element_tag;
prolog ::= prolog comment;
prolog ::= prolog xml_decl;
prolog ::= prolog processing_instruction;
prolog ::= prolog doctype_decl;
prolog ::= prolog SPACE;
prolog ::=;

entity_done ::= ENTITY_DONE;
/.
        case $rule_number:
            entityReferenceStack.pop()->isCurrentlyReferenced = false;
            clearSym();
        break;
./


xml_decl_start ::= XML;
/.
        case $rule_number:
            if (!scanString(spell[VERSION], VERSION, false) && atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./

xml_decl ::= xml_decl_start VERSION space_opt EQ space_opt literal attribute_list_opt QUESTIONMARK RANGLE;
/.
        case $rule_number:
            setType(QXmlStreamReader::StartDocument);
            documentVersion = symString(6);
            startDocument();
        break;
./

external_id ::= SYSTEM literal;
/.
        case $rule_number:
            hasExternalDtdSubset = true;
            dtdSystemId = symString(2);
        break;
./
external_id ::= PUBLIC public_literal space literal;
/.
        case $rule_number:
            checkPublicLiteral(symString(2));
            dtdPublicId = symString(2);
            dtdSystemId = symString(4);
            hasExternalDtdSubset = true;
        break;
./
external_id ::=;

doctype_decl_start ::= langle_bang DOCTYPE qname space;
/.
        case $rule_number:
            if (!scanPublicOrSystem() && atEnd) {
                resume($rule_number);
                return false;
            }
            dtdName = symString(3);
        break;
./

doctype_decl ::= langle_bang DOCTYPE qname RANGLE;
/.
        case $rule_number:./
doctype_decl ::= langle_bang DOCTYPE qname markup space_opt RANGLE;
/.
        case $rule_number:
            dtdName = symString(3);
            // fall through
./
doctype_decl ::= doctype_decl_start external_id space_opt markup space_opt RANGLE;
/.
        case $rule_number:./
doctype_decl ::= doctype_decl_start external_id space_opt RANGLE;
/.
        case $rule_number:
            setType(QXmlStreamReader::DTD);
            text = &textBuffer;
        break;
./

markup_start ::= LBRACK;
/.
        case $rule_number:
            scanDtd = true;
        break;
./

markup ::= markup_start markup_list RBRACK;
/.
        case $rule_number:
            scanDtd = false;
        break;
./


markup_list ::= markup_decl | space | pereference;
markup_list ::= markup_list markup_decl | markup_list space | markup_list pereference;
markup_list ::=;

markup_decl ::= element_decl | attlist_decl | entity_decl | entity_done | notation_decl | processing_instruction | comment;


element_decl_start ::= langle_bang ELEMENT qname space;
/.
        case $rule_number:
            if (!scanString(spell[EMPTY], EMPTY, false)
                && !scanString(spell[ANY], ANY, false)
                && atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./

element_decl ::= element_decl_start content_spec space_opt RANGLE;


content_spec ::= EMPTY | ANY | mixed | children;

pcdata_start ::= HASH;
/.
        case $rule_number:
            if (!scanString(spell[PCDATA], PCDATA, false) && atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./

pcdata ::= pcdata_start PCDATA;

questionmark_or_star_or_plus_opt ::= QUESTIONMARK | STAR | PLUS;
questionmark_or_star_or_plus_opt ::=;

cp ::= qname questionmark_or_star_or_plus_opt | choice_or_seq questionmark_or_star_or_plus_opt;

cp_pipe_or_comma_list ::= cp space_opt;
cp_pipe_or_comma_list ::= cp space_opt PIPE space_opt cp_pipe_list space_opt;
cp_pipe_or_comma_list ::= cp space_opt COMMA space_opt cp_comma_list space_opt;
cp_pipe_list ::= cp | cp_pipe_list space_opt PIPE space_opt cp;
cp_comma_list ::= cp | cp_comma_list space_opt COMMA space_opt cp;


name_pipe_list ::= PIPE space_opt qname;
name_pipe_list ::= name_pipe_list space_opt PIPE space_opt qname;

star_opt ::= | STAR;

mixed ::= LPAREN space_opt pcdata space_opt RPAREN star_opt;
mixed ::= LPAREN space_opt pcdata space_opt name_pipe_list space_opt RPAREN STAR;

choice_or_seq ::= LPAREN space_opt cp_pipe_or_comma_list RPAREN;

children ::= choice_or_seq questionmark_or_star_or_plus_opt;


nmtoken_pipe_list ::= nmtoken;
nmtoken_pipe_list ::= nmtoken_pipe_list space_opt PIPE space_opt nmtoken;


att_type ::= CDATA;
/.
        case $rule_number: {
            lastAttributeIsCData = true;
        } break;
./
att_type ::= ID | IDREF | IDREFS | ENTITY | ENTITIES | NMTOKEN | NMTOKENS;
att_type ::= LPAREN space_opt nmtoken_pipe_list space_opt RPAREN space;
att_type ::= NOTATION LPAREN space_opt nmtoken_pipe_list space_opt RPAREN space;


default_declhash ::= HASH;
/.
        case $rule_number:
            if (!scanAfterDefaultDecl() && atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./

default_decl ::= default_declhash REQUIRED;
default_decl ::= default_declhash IMPLIED;
default_decl ::= attribute_value;
default_decl ::= default_declhash FIXED space attribute_value;
attdef_start ::= space qname space;
/.
        case $rule_number:
                sym(1) = sym(2);
                lastAttributeValue.clear();
                lastAttributeIsCData = false;
                if (!scanAttType() && atEnd) {
                    resume($rule_number);
                    return false;
                }
        break;
./

attdef ::= attdef_start att_type default_decl;
/.
        case $rule_number: {
            DtdAttribute &dtdAttribute = dtdAttributes.push();
            dtdAttribute.tagName.clear();
            dtdAttribute.isCDATA = lastAttributeIsCData;
            dtdAttribute.attributePrefix = addToStringStorage(symPrefix(1));
            dtdAttribute.attributeName = addToStringStorage(symString(1));
            dtdAttribute.attributeQualifiedName = addToStringStorage(symName(1));
            dtdAttribute.isNamespaceAttribute = (dtdAttribute.attributePrefix == QLatin1String("xmlns")
                                                 || (dtdAttribute.attributePrefix.isEmpty()
                                                     && dtdAttribute.attributeName == QLatin1String("xmlns")));
            if (lastAttributeValue.isNull()) {
                dtdAttribute.defaultValue.clear();
            } else {
                if (dtdAttribute.isCDATA)
                    dtdAttribute.defaultValue = addToStringStorage(lastAttributeValue);
                else
                    dtdAttribute.defaultValue = addToStringStorage(lastAttributeValue.toString().simplified());

            }
        } break;
./

attdef_list ::= attdef;
attdef_list ::= attdef_list attdef;

attlist_decl ::= langle_bang ATTLIST qname space_opt RANGLE;
attlist_decl ::= langle_bang ATTLIST qname attdef_list space_opt RANGLE;
/.
        case $rule_number: {
            if (referenceToUnparsedEntityDetected && !standalone)
                break;
            int n = dtdAttributes.size();
            QStringRef tagName = addToStringStorage(symName(3));
            while (n--) {
                DtdAttribute &dtdAttribute = dtdAttributes[n];
                if (!dtdAttribute.tagName.isNull())
                    break;
                dtdAttribute.tagName = tagName;
                for (int i = 0; i < n; ++i) {
                    if ((dtdAttributes[i].tagName.isNull() || dtdAttributes[i].tagName == tagName)
                        && dtdAttributes[i].attributeQualifiedName == dtdAttribute.attributeQualifiedName) {
                        dtdAttribute.attributeQualifiedName.clear(); // redefined, delete it
                        break;
                    }
                }
            }
        } break;
./

entity_decl_start ::= langle_bang ENTITY name space;
/.
        case $rule_number: {
            if (!scanPublicOrSystem() && atEnd) {
                resume($rule_number);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.push();
            entityDeclaration.clear();
            entityDeclaration.name = symString(3);
        } break;
./

entity_decl_start ::= langle_bang ENTITY PERCENT space name space;
/.
        case $rule_number: {
            if (!scanPublicOrSystem() && atEnd) {
                resume($rule_number);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.push();
            entityDeclaration.clear();
            entityDeclaration.name = symString(5);
            entityDeclaration.parameter = true;
        } break;
./

entity_decl_external ::= entity_decl_start SYSTEM literal;
/.
        case $rule_number: {
            if (!scanNData() && atEnd) {
                resume($rule_number);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            entityDeclaration.systemId = symString(3);
            entityDeclaration.external = true;
        } break;
./

entity_decl_external ::= entity_decl_start PUBLIC public_literal space literal;
/.
        case $rule_number: {
            if (!scanNData() && atEnd) {
                resume($rule_number);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            checkPublicLiteral((entityDeclaration.publicId = symString(3)));
            entityDeclaration.systemId = symString(5);
            entityDeclaration.external = true;
        } break;
./

entity_decl ::= entity_decl_external NDATA name space_opt RANGLE;
/.
        case $rule_number: {
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            entityDeclaration.notationName = symString(3);
            if (entityDeclaration.parameter)
                raiseWellFormedError(QXmlStream::tr("NDATA in parameter entity declaration."));
        }
        //fall through
./

entity_decl ::= entity_decl_external space_opt RANGLE;
/.
        case $rule_number:./

entity_decl ::= entity_decl_start entity_value space_opt RANGLE;
/.
        case $rule_number: {
            if (referenceToUnparsedEntityDetected && !standalone) {
                entityDeclarations.pop();
                break;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            if (!entityDeclaration.external)
                entityDeclaration.value = symString(2);
            QString entityName = entityDeclaration.name.toString();
            QHash<QString, Entity> &hash = entityDeclaration.parameter ? parameterEntityHash : entityHash;
            if (!hash.contains(entityName)) {
                Entity entity(entityDeclaration.value.toString());
                entity.unparsed = (!entityDeclaration.notationName.isNull());
                entity.external = entityDeclaration.external;
                hash.insert(entityName, entity);
            }
        } break;
./


processing_instruction ::= LANGLE QUESTIONMARK name space;
/.
        case $rule_number: {
            setType(QXmlStreamReader::ProcessingInstruction);
            int pos = sym(4).pos + sym(4).len;
            processingInstructionTarget = symString(3);
            if (scanUntil("?>")) {
                processingInstructionData = QStringRef(&textBuffer, pos, textBuffer.size() - pos - 2);
                const QString piTarget(processingInstructionTarget.toString());
                if (!piTarget.compare(QLatin1String("xml"), Qt::CaseInsensitive)) {
                    raiseWellFormedError(QXmlStream::tr("XML declaration not at start of document."));
                }
                else if(!QXmlUtils::isNCName(piTarget))
                    raiseWellFormedError(QXmlStream::tr("%1 is an invalid processing instruction name.").arg(piTarget));
            } else if (type != QXmlStreamReader::Invalid){
                resume($rule_number);
                return false;
            }
        } break;
./

processing_instruction ::= LANGLE QUESTIONMARK name QUESTIONMARK RANGLE;
/.
        case $rule_number:
            setType(QXmlStreamReader::ProcessingInstruction);
            processingInstructionTarget = symString(3);
            if (!processingInstructionTarget.toString().compare(QLatin1String("xml"), Qt::CaseInsensitive))
                raiseWellFormedError(QXmlStream::tr("Invalid processing instruction name."));
        break;
./


langle_bang ::= LANGLE BANG;
/.
        case $rule_number:
            if (!scanAfterLangleBang() && atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./

comment_start ::= langle_bang DASH DASH;
/.
        case $rule_number:
            if (!scanUntil("--")) {
                resume($rule_number);
                return false;
            }
        break;
./

comment ::= comment_start RANGLE;
/.
        case $rule_number: {
            setType(QXmlStreamReader::Comment);
            int pos = sym(1).pos + 4;
            text = QStringRef(&textBuffer, pos, textBuffer.size() - pos - 3);
        } break;
./


cdata ::= langle_bang CDATA_START;
/.
        case $rule_number: {
            setType(QXmlStreamReader::Characters);
            isCDATA = true;
	    isWhitespace = false;
            int pos = sym(2).pos;
            if (scanUntil("]]>", -1)) {
                text = QStringRef(&textBuffer, pos, textBuffer.size() - pos - 3);
            } else {
                resume($rule_number);
                return false;
            }
        } break;
./

notation_decl_start ::= langle_bang NOTATION name space;
/.
        case $rule_number: {
            if (!scanPublicOrSystem() && atEnd) {
                resume($rule_number);
                return false;
            }
            NotationDeclaration &notationDeclaration = notationDeclarations.push();
            notationDeclaration.name = symString(3);
        } break;
./

notation_decl ::= notation_decl_start SYSTEM literal space_opt RANGLE;
/.
        case $rule_number: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            notationDeclaration.systemId = symString(3);
            notationDeclaration.publicId.clear();
        } break;
./

notation_decl ::= notation_decl_start PUBLIC public_literal space_opt RANGLE;
/.
        case $rule_number: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            notationDeclaration.systemId.clear();
            checkPublicLiteral((notationDeclaration.publicId = symString(3)));
        } break;
./

notation_decl ::= notation_decl_start PUBLIC public_literal space literal space_opt RANGLE;
/.
        case $rule_number: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            checkPublicLiteral((notationDeclaration.publicId = symString(3)));
            notationDeclaration.systemId = symString(5);
        } break;
./



content_char ::= RANGLE | HASH | LBRACK | RBRACK | LPAREN | RPAREN | PIPE | EQ | PERCENT | SLASH | COLON | SEMICOLON | COMMA | DASH | PLUS | STAR | DOT | QUESTIONMARK | BANG | QUOTE | DBLQUOTE | LETTER | DIGIT;

scan_content_char ::= content_char;
/.
        case $rule_number:
            isWhitespace = false;
            // fall through
./

scan_content_char ::= SPACE;
/.
        case $rule_number:
            sym(1).len += fastScanContentCharList();
            if (atEnd && !inParseEntity) {
                resume($rule_number);
                return false;
            }
	break;
./

content_char_list ::= content_char_list char_ref;
content_char_list ::= content_char_list entity_ref;
content_char_list ::= content_char_list entity_done;
content_char_list ::= content_char_list scan_content_char;
content_char_list ::= char_ref;
content_char_list ::= entity_ref;
content_char_list ::= entity_done;
content_char_list ::= scan_content_char;


character_content ::= content_char_list %prec SHIFT_THERE;
/.
        case $rule_number:
	    if (!textBuffer.isEmpty()) {
                setType(QXmlStreamReader::Characters);
                text = &textBuffer;
	    }
	break;
./

literal ::= QUOTE QUOTE;
/.
        case $rule_number:./
literal ::= DBLQUOTE DBLQUOTE;
/.
        case $rule_number:
            clearSym();
        break;
./
literal ::= QUOTE literal_content_with_dblquote QUOTE;
/.
        case $rule_number:./
literal ::= DBLQUOTE literal_content_with_quote DBLQUOTE;
/.
        case $rule_number:
            sym(1) = sym(2);
        break;
./

literal_content_with_dblquote ::= literal_content_with_dblquote literal_content;
/.
        case $rule_number:./
literal_content_with_quote ::= literal_content_with_quote literal_content;
/.
        case $rule_number:./
literal_content_with_dblquote ::= literal_content_with_dblquote DBLQUOTE;
/.
        case $rule_number:./
literal_content_with_quote ::= literal_content_with_quote QUOTE;
/.
        case $rule_number:
            sym(1).len += sym(2).len;
        break;
./
literal_content_with_dblquote ::= literal_content;
literal_content_with_quote ::= literal_content;
literal_content_with_dblquote ::= DBLQUOTE;
literal_content_with_quote ::= QUOTE;

literal_content_start ::= LETTER | DIGIT | RANGLE | HASH | LBRACK | RBRACK | LPAREN | RPAREN | PIPE | EQ | PERCENT | SLASH | COLON | SEMICOLON | COMMA | DASH | PLUS | STAR | DOT | QUESTIONMARK | BANG;

literal_content_start ::= SPACE;
/.
        case $rule_number:
	    if (normalizeLiterals)
                textBuffer.data()[textBuffer.size()-1] = QLatin1Char(' ');
        break;
./

literal_content ::= literal_content_start;
/.
        case $rule_number:
            sym(1).len += fastScanLiteralContent();
            if (atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./


public_literal ::= literal;
/.
        case $rule_number: {
            if (!QXmlUtils::isPublicID(symString(1).toString())) {
                raiseWellFormedError(QXmlStream::tr("%1 is an invalid PUBLIC identifier.").arg(symString(1).toString()));
                resume($rule_number);
                return false;
            }
        } break;
./

entity_value ::= QUOTE QUOTE;
/.
        case $rule_number:./
entity_value ::= DBLQUOTE DBLQUOTE;
/.
        case $rule_number:
            clearSym();
        break;
./

entity_value ::= QUOTE entity_value_content_with_dblquote QUOTE;
/.
        case $rule_number:./
entity_value ::= DBLQUOTE entity_value_content_with_quote DBLQUOTE;
/.
        case $rule_number:
	    sym(1) = sym(2);
        break;
./

entity_value_content_with_dblquote ::= entity_value_content_with_dblquote entity_value_content;
/.
        case $rule_number:./
entity_value_content_with_quote ::= entity_value_content_with_quote entity_value_content;
/.
        case $rule_number:./
entity_value_content_with_dblquote ::= entity_value_content_with_dblquote DBLQUOTE;
/.
        case $rule_number:./
entity_value_content_with_quote ::= entity_value_content_with_quote QUOTE;
/.
        case $rule_number:
            sym(1).len += sym(2).len;
        break;
./
entity_value_content_with_dblquote ::= entity_value_content;
entity_value_content_with_quote ::= entity_value_content;
entity_value_content_with_dblquote ::= DBLQUOTE;
entity_value_content_with_quote ::= QUOTE;

entity_value_content ::= LETTER | DIGIT | LANGLE | RANGLE | HASH | LBRACK | RBRACK | LPAREN | RPAREN | PIPE | EQ | SLASH | COLON | SEMICOLON | COMMA | SPACE | DASH | PLUS | STAR | DOT | QUESTIONMARK | BANG;
entity_value_content ::= char_ref | entity_ref_in_entity_value | entity_done;


attribute_value ::= QUOTE QUOTE;
/.
        case $rule_number:./
attribute_value ::= DBLQUOTE DBLQUOTE;
/.
        case $rule_number:
            clearSym();
        break;
./
attribute_value ::= QUOTE attribute_value_content_with_dblquote QUOTE;
/.
        case $rule_number:./
attribute_value ::= DBLQUOTE attribute_value_content_with_quote DBLQUOTE;
/.
        case $rule_number:
            sym(1) = sym(2);
            lastAttributeValue = symString(1);
        break;
./

attribute_value_content_with_dblquote ::= attribute_value_content_with_dblquote attribute_value_content;
/.
        case $rule_number:./
attribute_value_content_with_quote ::= attribute_value_content_with_quote attribute_value_content;
/.
        case $rule_number:./
attribute_value_content_with_dblquote ::= attribute_value_content_with_dblquote DBLQUOTE;
/.
        case $rule_number:./
attribute_value_content_with_quote ::= attribute_value_content_with_quote QUOTE;
/.
        case $rule_number:
            sym(1).len += sym(2).len;
        break;
./
attribute_value_content_with_dblquote ::= attribute_value_content | DBLQUOTE;
attribute_value_content_with_quote ::= attribute_value_content | QUOTE;

attribute_value_content ::= literal_content | char_ref | entity_ref_in_attribute_value | entity_done;

attribute ::= qname space_opt EQ space_opt attribute_value;
/.
        case $rule_number: {
            QStringRef prefix = symPrefix(1);
            if (prefix.isEmpty() && symString(1) == QLatin1String("xmlns") && namespaceProcessing) {
                NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                namespaceDeclaration.prefix.clear();

                const QStringRef ns(symString(5));
                if(ns == QLatin1String("http://www.w3.org/2000/xmlns/") ||
                   ns == QLatin1String("http://www.w3.org/XML/1998/namespace"))
                    raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));
                else
                    namespaceDeclaration.namespaceUri = addToStringStorage(ns);
            } else {
                Attribute &attribute = attributeStack.push();
                attribute.key = sym(1);
                attribute.value = sym(5);

                QStringRef attributeQualifiedName = symName(1);
                bool normalize = false;
                for (int a = 0; a < dtdAttributes.size(); ++a) {
                    DtdAttribute &dtdAttribute = dtdAttributes[a];
                    if (!dtdAttribute.isCDATA
                        && dtdAttribute.tagName == qualifiedName
                        && dtdAttribute.attributeQualifiedName == attributeQualifiedName
                        ) {
                        normalize = true;
                        break;
                    }
                }
                if (normalize) {
                    // normalize attribute value (simplify and trim)
                    int pos = textBuffer.size();
                    int n = 0;
                    bool wasSpace = true;
                    for (int i = 0; i < attribute.value.len; ++i) {
                        QChar c = textBuffer.at(attribute.value.pos + i);
                        if (c.unicode() == ' ') {
                            if (wasSpace)
                                continue;
                            wasSpace = true;
                        } else {
                            wasSpace = false;
                        }
                        textBuffer += textBuffer.at(attribute.value.pos + i);
                        ++n;
                    }
                    if (wasSpace)
                        while (n && textBuffer.at(pos + n - 1).unicode() == ' ')
                            --n;
                    attribute.value.pos = pos;
                    attribute.value.len = n;
                }
                if (prefix == QLatin1String("xmlns") && namespaceProcessing) {
                    NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                    QStringRef namespacePrefix = symString(attribute.key);
                    QStringRef namespaceUri = symString(attribute.value);
                    attributeStack.pop();
                    if (((namespacePrefix == QLatin1String("xml"))
                         ^ (namespaceUri == QLatin1String("http://www.w3.org/XML/1998/namespace")))
                        || namespaceUri == QLatin1String("http://www.w3.org/2000/xmlns/")
                        || namespaceUri.isEmpty()
                        || namespacePrefix == QLatin1String("xmlns"))
                        raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));

                    namespaceDeclaration.prefix = addToStringStorage(namespacePrefix);
                    namespaceDeclaration.namespaceUri = addToStringStorage(namespaceUri);
                }
            }
        } break;
./



attribute_list_opt ::= | space | space attribute_list space_opt;
attribute_list ::= attribute | attribute_list space attribute;

stag_start ::= LANGLE qname;
/.
        case $rule_number: {
            normalizeLiterals = true;
            Tag &tag = tagStack_push();
            prefix = tag.namespaceDeclaration.prefix  = addToStringStorage(symPrefix(2));
            name = tag.name = addToStringStorage(symString(2));
            qualifiedName = tag.qualifiedName = addToStringStorage(symName(2));
            if ((!prefix.isEmpty() && !QXmlUtils::isNCName(prefix)) || !QXmlUtils::isNCName(name))
                raiseWellFormedError(QXmlStream::tr("Invalid XML name."));
        } break;
./


empty_element_tag ::= stag_start attribute_list_opt SLASH RANGLE;
/.
        case $rule_number:
            isEmptyElement = true;
        // fall through
./


stag ::= stag_start attribute_list_opt RANGLE;
/.
        case $rule_number:
            setType(QXmlStreamReader::StartElement);
            resolveTag();
            if (tagStack.size() == 1 && hasSeenTag && !inParseEntity)
                raiseWellFormedError(QXmlStream::tr("Extra content at end of document."));
            hasSeenTag = true;
        break;
./


etag ::= LANGLE SLASH qname space_opt RANGLE;
/.
        case $rule_number: {
            setType(QXmlStreamReader::EndElement);
            Tag &tag = tagStack_pop();

            namespaceUri = tag.namespaceDeclaration.namespaceUri;
            name = tag.name;
            qualifiedName = tag.qualifiedName;
            if (qualifiedName != symName(3))
                raiseWellFormedError(QXmlStream::tr("Opening and ending tag mismatch."));
        } break;
./


unresolved_entity ::= UNRESOLVED_ENTITY;
/.
        case $rule_number:
            if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(unresolvedEntity));
                break;
            }
            setType(QXmlStreamReader::EntityReference);
            name = &unresolvedEntity;
	break;
./

entity_ref ::= AMPERSAND name SEMICOLON;
/.
        case $rule_number: {
            sym(1).len += sym(2).len + 1;
            QString reference = symString(2).toString();
            if (entityHash.contains(reference)) {
                Entity &entity = entityHash[reference];
                if (entity.unparsed) {
                    raiseWellFormedError(QXmlStream::tr("Reference to unparsed entity '%1'.").arg(reference));
                } else {
                    if (!entity.hasBeenParsed) {
                        parseEntity(entity.value);
                        entity.hasBeenParsed = true;
                    }
                    if (entity.literal)
                        putStringLiteral(entity.value);
                    else if (referenceEntity(entity))
                        putReplacement(entity.value);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                }
                break;
            }

            if (entityResolver) {
                QString replacementText = resolveUndeclaredEntity(reference);
                if (!replacementText.isNull()) {
                    putReplacement(replacementText);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                    break;
                }
            }

	    injectToken(UNRESOLVED_ENTITY);
	    unresolvedEntity = symString(2).toString();
	    textBuffer.chop(2 + sym(2).len);
	    clearSym();

        } break;
./

pereference ::= PERCENT name SEMICOLON;
/.
        case $rule_number: {
            sym(1).len += sym(2).len + 1;
            QString reference = symString(2).toString();
            if (parameterEntityHash.contains(reference)) {
                referenceToParameterEntityDetected = true;
                Entity &entity = parameterEntityHash[reference];
                if (entity.unparsed || entity.external) {
                    referenceToUnparsedEntityDetected = true;
                } else {
                    if (referenceEntity(entity))
                        putString(entity.value);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                }
            } else if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(symString(2).toString()));
            }
        } break;
./



entity_ref_in_entity_value ::= AMPERSAND name SEMICOLON;
/.
        case $rule_number:
            sym(1).len += sym(2).len + 1;
        break;
./

entity_ref_in_attribute_value ::= AMPERSAND name SEMICOLON;
/.
        case $rule_number: {
            sym(1).len += sym(2).len + 1;
            QString reference = symString(2).toString();
            if (entityHash.contains(reference)) {
                Entity &entity = entityHash[reference];
                if (entity.unparsed || entity.value.isNull()) {
                    raiseWellFormedError(QXmlStream::tr("Reference to external entity '%1' in attribute value.").arg(reference));
                    break;
                }
                if (!entity.hasBeenParsed) {
                    parseEntity(entity.value);
                    entity.hasBeenParsed = true;
                }
                if (entity.literal)
                    putStringLiteral(entity.value);
                else if (referenceEntity(entity))
                    putReplacementInAttributeValue(entity.value);
                textBuffer.chop(2 + sym(2).len);
                clearSym();
                break;
            }

            if (entityResolver) {
                QString replacementText = resolveUndeclaredEntity(reference);
                if (!replacementText.isNull()) {
                    putReplacement(replacementText);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                    break;
                }
            }
            if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(reference));
            }
        } break;
./

char_ref ::= AMPERSAND HASH char_ref_value SEMICOLON;
/.
        case $rule_number: {
            if (uint s = resolveCharRef(3)) {
                if (s >= 0xffff)
                    putStringLiteral(QString::fromUcs4(&s, 1));
                else
                    putChar((LETTER << 16) | s);

                textBuffer.chop(3 + sym(3).len);
                clearSym();
            } else {
                raiseWellFormedError(QXmlStream::tr("Invalid character reference."));
            }
        } break;
./


char_ref_value ::= LETTER | DIGIT;
char_ref_value ::= char_ref_value LETTER;
/.
        case $rule_number:./
char_ref_value ::= char_ref_value DIGIT;
/.
        case $rule_number:
            sym(1).len += sym(2).len;
        break;
./


content ::= content character_content;
content ::= content stag content etag;
content ::= content empty_element_tag;
content ::= content comment;
content ::= content cdata;
content ::= content xml_decl;
content ::= content processing_instruction;
content ::= content doctype_decl;
content ::= content unresolved_entity;
content ::=  ;


space ::=  SPACE;
/.
        case $rule_number:
            sym(1).len += fastScanSpace();
            if (atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./


space_opt ::=;
space_opt ::= space;

qname ::= LETTER;
/.
        case $rule_number: {
            sym(1).len += fastScanName(&sym(1).prefix);
            if (atEnd) {
                resume($rule_number);
                return false;
            }
        } break;
./

name ::= LETTER;
/.
        case $rule_number:
            sym(1).len += fastScanName();
            if (atEnd) {
                resume($rule_number);
                return false;
            }
        break;
./

nmtoken ::= LETTER;
/.
        case $rule_number:./
nmtoken ::= DIGIT;
/.
        case $rule_number:./
nmtoken ::= DOT;
/.
        case $rule_number:./
nmtoken ::= DASH;
/.
        case $rule_number:./
nmtoken ::= COLON;
/.
        case $rule_number:
            sym(1).len += fastScanNMTOKEN();
            if (atEnd) {
                resume($rule_number);
                return false;
            }

        break;
./


/.
    default:
        ;
    } // switch
            act = state_stack[tos] = nt_action (act, lhs[r] - TERMINAL_COUNT);
            if (type != QXmlStreamReader::NoToken)
                return true;
        } else {
            parseError();
            break;
        }
    }
    return false;
}
#endif //QT_NO_XMLSTREAMREADER.xml

./

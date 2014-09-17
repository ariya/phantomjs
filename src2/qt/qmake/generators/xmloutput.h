/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef XMLOUTPUT_H
#define XMLOUTPUT_H

#include <qtextstream.h>
#include <qstack.h>

QT_BEGIN_NAMESPACE

class XmlOutput
{
public:
    enum ConverstionType {
        NoConversion,       // No change
        EscapeConversion,   // Use '\"'
        XMLConversion       // Use &quot;
    };
    enum XMLFormat {
        NoNewLine,          // No new lines, unless added manually
        NewLine             // All properties & tags indented on new lines
    };
    enum XMLState {
        Bare,               // Not in tag or attribute
        Tag,                // <tagname attribute1="value"
        Attribute           //  attribute2="value">
    };
    enum XMLType {
        tNothing,           // No XML output, and not state change
        tRaw,               // Raw text (no formating)
        tDeclaration,       // <?xml version="x.x" encoding="xxx"?>
        tTag,               // <tagname attribute1="value"
        tTagValue,          // <tagname>value</tagname>
        tValueTag,          // value</tagname>
        tCloseTag,          // Closes an open tag
        tAttribute,         //  attribute2="value">
        tAttributeTag,      //  attribute on the same line as a tag
        tData,              // Tag data (formating done)
        tImport,            // <import "type"="path" />
        tComment,           // <!-- Comment -->
        tCDATA              // <![CDATA[ ... ]]>
    };

    XmlOutput(QTextStream &file, ConverstionType type = XMLConversion);
    ~XmlOutput();

    // Settings
    void setIndentString(const QString &indentString);
    QString indentString();
    void setIndentLevel(int level);
    int indentLevel();
    void setState(XMLState state);
    void setFormat(XMLFormat newFormat);
    XMLState state();


    struct xml_output {
        XMLType xo_type;    // Type of struct instance
        QString xo_text;    // Tag/Attribute name/xml version
        QString xo_value;   // Value of attributes/xml encoding

        xml_output(XMLType type, const QString &text, const QString &value)
            : xo_type(type), xo_text(text), xo_value(value) {}
        xml_output(const xml_output &xo)
            : xo_type(xo.xo_type), xo_text(xo.xo_text), xo_value(xo.xo_value) {}
    };

    // Streams
    XmlOutput& operator<<(const QString& o);
    XmlOutput& operator<<(const xml_output& o);

private:
    void increaseIndent();
    void decreaseIndent();
    void updateIndent();

    QString doConversion(const QString &text);

    // Output functions
    void newTag(const QString &tag);
    void newTagOpen(const QString &tag);
    void closeOpen();
    void closeTag();
    void closeTo(const QString &tag);
    void closeAll();

    void addDeclaration(const QString &version, const QString &encoding);
    void addRaw(const QString &rawText);
    void addAttribute(const QString &attribute, const QString &value);
    void addAttributeTag(const QString &attribute, const QString &value);
    void addData(const QString &data);

    // Data
    QTextStream &xmlFile;
    QString indent;

    QString currentIndent;
    int currentLevel;
    XMLState currentState;

    XMLFormat format;
    ConverstionType conversion;
    QStack<QString> tagStack;
};

inline XmlOutput::xml_output noxml()
{
    return XmlOutput::xml_output(XmlOutput::tNothing, QString(), QString());
}

inline XmlOutput::xml_output raw(const QString &rawText)
{
    return XmlOutput::xml_output(XmlOutput::tRaw, rawText, QString());
}

inline XmlOutput::xml_output declaration(const QString &version = QString("1.0"),
                                         const QString &encoding = QString())
{
    return XmlOutput::xml_output(XmlOutput::tDeclaration, version, encoding);
}

inline XmlOutput::xml_output decl(const QString &version = QString("1.0"),
                                  const QString &encoding = QString())
{
    return declaration(version, encoding);
}

inline XmlOutput::xml_output tag(const QString &name)
{
    return XmlOutput::xml_output(XmlOutput::tTag, name, QString());
}


inline XmlOutput::xml_output valueTag(const QString &value)
{
    return XmlOutput::xml_output(XmlOutput::tValueTag, value, QString());
}

inline XmlOutput::xml_output tagValue(const QString &tagName, const QString &value)
{
    return XmlOutput::xml_output(XmlOutput::tTagValue, tagName, value);
}

inline XmlOutput::xml_output import(const QString &tagName, const QString &value)
{
    return XmlOutput::xml_output(XmlOutput::tImport, tagName, value);
}

inline XmlOutput::xml_output closetag()
{
    return XmlOutput::xml_output(XmlOutput::tCloseTag, QString(), QString());
}

inline XmlOutput::xml_output closetag(const QString &toTag)
{
    return XmlOutput::xml_output(XmlOutput::tCloseTag, toTag, QString());
}

inline XmlOutput::xml_output closeall()
{
    return XmlOutput::xml_output(XmlOutput::tCloseTag, QString(), QString("all"));
}

inline XmlOutput::xml_output attribute(const QString &name,
                                       const QString &value)
{
    return XmlOutput::xml_output(XmlOutput::tAttribute, name, value);
}

inline XmlOutput::xml_output attributeTag(const QString &name,
                                       const QString &value)
{
    return XmlOutput::xml_output(XmlOutput::tAttributeTag, name, value);
}

inline XmlOutput::xml_output attr(const QString &name,
                                  const QString &value)
{
    return attribute(name, value);
}

inline XmlOutput::xml_output attrTag(const QString &name,
                                  const QString &value)
{
    return attributeTag(name, value);
}

inline XmlOutput::xml_output data(const QString &text = QString())
{
    return XmlOutput::xml_output(XmlOutput::tData, text, QString());
}

inline XmlOutput::xml_output comment(const QString &text)
{
    return XmlOutput::xml_output(XmlOutput::tComment, text, QString());
}

inline XmlOutput::xml_output cdata(const QString &text)
{
    return XmlOutput::xml_output(XmlOutput::tCDATA, text, QString());
}

QT_END_NAMESPACE

#endif // XMLOUTPUT_H

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "xmloutput.h"

QT_BEGIN_NAMESPACE

XmlOutput::XmlOutput(QTextStream &file, ConverstionType type)
    : xmlFile(file), indent("\t"), currentLevel(0), currentState(Bare), format(NewLine),
      conversion(type)
{
    tagStack.clear();
}

XmlOutput::~XmlOutput()
{
    closeAll();
}

// Settings ------------------------------------------------------------------
void XmlOutput::setIndentString(const QString &indentString)
{
    indent = indentString;
}

QString XmlOutput::indentString()
{
    return indent;
}

void XmlOutput::setIndentLevel(int level)
{
    currentLevel = level;
}

int XmlOutput::indentLevel()
{
    return currentLevel;
}

void XmlOutput::setState(XMLState state)
{
    currentState = state;
}

void XmlOutput::setFormat(XMLFormat newFormat)
{
    format = newFormat;
}

XmlOutput::XMLState XmlOutput::state()
{
    return currentState;
}

void XmlOutput::updateIndent()
{
    currentIndent.clear();
    for (int i = 0; i < currentLevel; ++i)
        currentIndent.append(indent);
}

void XmlOutput::increaseIndent()
{
    ++currentLevel;
    updateIndent();
}

void XmlOutput::decreaseIndent()
{
    if (currentLevel)
        --currentLevel;
    updateIndent();
    if (!currentLevel)
        currentState = Bare;
}

QString XmlOutput::doConversion(const QString &text)
{
    if (!text.count())
        return QString();
    else if (conversion == NoConversion)
        return text;

    QString output;
    if (conversion == XMLConversion) {

        // this is a way to escape characters that shouldn't be converted
        for (int i=0; i<text.count(); ++i) {
            if (text.at(i) == QLatin1Char('&')) {
                if ( (i + 7) < text.count() &&
                    text.at(i + 1) == QLatin1Char('#') &&
                    text.at(i + 2) == QLatin1Char('x') &&
                    text.at(i + 7) == QLatin1Char(';') ) {
                    output += text.at(i);
                } else {
                    output += QLatin1String("&amp;");
                }
            } else {
                QChar c = text.at(i);
                if (c.unicode() < 0x20) {
                    output += QString("&#x%1;").arg(c.unicode(), 2, 16, QLatin1Char('0'));
                } else {
                    output += text.at(i);
                }
            }
        }
    } else {
        output = text;
    }

    if (conversion == XMLConversion) {
        output.replace('\"', "&quot;");
        output.replace('\'', "&apos;");
    } else if (conversion == EscapeConversion) {
        output.replace('\"', "\\\"");
        output.replace('\'', "\\\'");
    }
    return output;
}

// Stream functions ----------------------------------------------------------
XmlOutput& XmlOutput::operator<<(const QString& o)
{
    return operator<<(data(o));
}

XmlOutput& XmlOutput::operator<<(const xml_output& o)
{
    switch(o.xo_type) {
    case tNothing:
        break;
    case tRaw:
        addRaw(o.xo_text);
        break;
    case tDeclaration:
        addDeclaration(o.xo_text, o.xo_value);
        break;
    case tTag:
        newTagOpen(o.xo_text);
        break;
    case tTagValue:
        addRaw(QString("\n%1<%2>").arg(currentIndent).arg(o.xo_text));
        addRaw(doConversion(o.xo_value));
        addRaw(QString("</%1>").arg(o.xo_text));
        break;
    case tValueTag:
        addRaw(QString("%1").arg(doConversion(o.xo_text)));
        setFormat(NoNewLine);
        closeTag();
        setFormat(NewLine);
        break;
    case tImport:
        addRaw(QString("\n%1<Import %2=\"%3\" />").arg(currentIndent).arg(o.xo_text).arg(o.xo_value));
        break;
    case tCloseTag:
        if (o.xo_value.count())
            closeAll();
        else if (o.xo_text.count())
            closeTo(o.xo_text);
        else
            closeTag();
        break;
    case tAttribute:
        addAttribute(o.xo_text, o.xo_value);
        break;
    case tAttributeTag:
        addAttributeTag(o.xo_text, o.xo_value);
        break;
    case tData:
        {
            // Special case to be able to close tag in normal
            // way ("</tag>", not "/>") without using addRaw()..
            if (!o.xo_text.count()) {
                closeOpen();
                break;
            }
            QString output = doConversion(o.xo_text);
            output.replace('\n', "\n" + currentIndent);
            addRaw(QString("\n%1%2").arg(currentIndent).arg(output));
        }
        break;
    case tComment:
        {
            QString output("<!--%1-->");
            addRaw(output.arg(o.xo_text));
        }
        break;
    case tCDATA:
        {
            QString output("<![CDATA[\n%1\n]]>");
            addRaw(output.arg(o.xo_text));
        }
        break;
    }
    return *this;
}


// Output functions ----------------------------------------------------------
void XmlOutput::newTag(const QString &tag)
{
    Q_ASSERT_X(tag.count(), "XmlOutput", "Cannot open an empty tag");
    newTagOpen(tag);
    closeOpen();
}

void XmlOutput::newTagOpen(const QString &tag)
{
    Q_ASSERT_X(tag.count(), "XmlOutput", "Cannot open an empty tag");
    closeOpen();

    if (format == NewLine)
        xmlFile << endl << currentIndent;
    xmlFile << '<' << doConversion(tag);
    currentState = Attribute;
    tagStack.append(tag);
    increaseIndent(); // ---> indent
}

void XmlOutput::closeOpen()
{
    switch(currentState) {
        case Bare:
        case Tag:
            return;
        case Attribute:
            break;
    }
    xmlFile << '>';
    currentState = Tag;
}

void XmlOutput::closeTag()
{
    switch(currentState) {
        case Bare:
            if (tagStack.count())
                //warn_msg(WarnLogic, "<Root>: Cannot close tag in Bare state, %d tags on stack", tagStack.count());
                qDebug("<Root>: Cannot close tag in Bare state, %d tags on stack", tagStack.count());
            else
                //warn_msg(WarnLogic, "<Root>: Cannot close tag, no tags on stack");
                qDebug("<Root>: Cannot close tag, no tags on stack");
            return;
        case Tag:
            decreaseIndent(); // <--- Pre-decrease indent
            if (format == NewLine)
                xmlFile << endl << currentIndent;
            xmlFile << "</" << doConversion(tagStack.last()) << '>';
            tagStack.pop_back();
            break;
        case Attribute:
            xmlFile << " />";
            tagStack.pop_back();
            currentState = Tag;
            decreaseIndent(); // <--- Post-decrease indent
            break;
    }
}

void XmlOutput::closeTo(const QString &tag)
{
    bool cont = true;
    if (!tagStack.contains(tag) && !tag.isNull()) {
        //warn_msg(WarnLogic, "<%s>: Cannot close to tag <%s>, not on stack", tagStack.last().latin1(), tag.latin1());
        qDebug("<%s>: Cannot close to tag <%s>, not on stack", tagStack.last().toLatin1().constData(), tag.toLatin1().constData());
        return;
    }
    int left = tagStack.count();
    while (left-- && cont) {
        cont = tagStack.last().compare(tag) != 0;
        closeTag();
    }
}

void XmlOutput::closeAll()
{
    if (!tagStack.count())
        return;
    closeTo(QString());
}

void XmlOutput::addDeclaration(const QString &version, const QString &encoding)
{
    switch(currentState) {
        case Bare:
            break;
        case Tag:
        case Attribute:
            //warn_msg(WarnLogic, "<%s>: Cannot add declaration when not in bare state", tagStack.last().toLatin1().constData());
            qDebug("<%s>: Cannot add declaration when not in bare state", tagStack.last().toLatin1().constData());
            return;
    }
    QString outData = QString("<?xml version=\"%1\" encoding=\"%2\"?>")
                              .arg(doConversion(version))
                              .arg(doConversion(encoding));
    addRaw(outData);
}

void XmlOutput::addRaw(const QString &rawText)
{
    closeOpen();
    xmlFile << rawText;
}

void XmlOutput::addAttribute(const QString &attribute, const QString &value)
{
     switch(currentState) {
        case Bare:
        case Tag:
            //warn_msg(WarnLogic, "<%s>: Cannot add attribute since tags not open", tagStack.last().toLatin1().constData());
            qDebug("<%s>: Cannot add attribute (%s) since tag's not open",
                   (tagStack.count() ? tagStack.last().toLatin1().constData() : "Root"),
                   attribute.toLatin1().constData());
            return;
        case Attribute:
            break;
    }
    if (format == NewLine)
        xmlFile << endl;
    xmlFile << currentIndent << doConversion(attribute) << "=\"" << doConversion(value) << "\"";
}

void XmlOutput::addAttributeTag(const QString &attribute, const QString &value)
{
     switch(currentState) {
        case Bare:
        case Tag:
            //warn_msg(WarnLogic, "<%s>: Cannot add attribute since tags not open", tagStack.last().toLatin1().constData());
            qDebug("<%s>: Cannot add attribute (%s) since tag's not open",
                   (tagStack.count() ? tagStack.last().toLatin1().constData() : "Root"),
                   attribute.toLatin1().constData());
            return;
        case Attribute:
            break;
    }
    xmlFile << " " << doConversion(attribute) << "=\"" << doConversion(value) << "\"";
}

QT_END_NAMESPACE

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
  atom.h
*/

#ifndef ATOM_H
#define ATOM_H

#include <qstringlist.h>

QT_BEGIN_NAMESPACE

class Atom
{
public:
    enum Type {
        AbstractLeft,
        AbstractRight,
        AnnotatedList,
        AutoLink,
        BaseName,
        BR,
        BriefLeft,
        BriefRight,
        C,
        CaptionLeft,
        CaptionRight,
        Code,
        CodeBad,
        CodeNew,
        CodeOld,
        CodeQuoteArgument,
        CodeQuoteCommand,
        DivLeft,
        DivRight,
        EndQmlText,
        FootnoteLeft,
        FootnoteRight,
        FormatElse,
        FormatEndif,
        FormatIf,
        FormattingLeft,
        FormattingRight,
        GeneratedList,
        GuidLink,
        HR,
        Image,
        ImageText,
        ImportantLeft,
        ImportantRight,
        InlineImage,
        JavaScript,
        EndJavaScript,
        LegaleseLeft,
        LegaleseRight,
        LineBreak,
        Link,
        LinkNode,
        ListLeft,
        ListItemNumber,
        ListTagLeft,
        ListTagRight,
        ListItemLeft,
        ListItemRight,
        ListRight,
        Nop,
        NoteLeft,
        NoteRight,
        ParaLeft,
        ParaRight,
        Qml,
        QmlText,
        QuotationLeft,
        QuotationRight,
        RawString,
        SectionLeft,
        SectionRight,
        SectionHeadingLeft,
        SectionHeadingRight,
        SidebarLeft,
        SidebarRight,
        SinceList,
        SnippetCommand,
        SnippetIdentifier,
        SnippetLocation,
        String,
        TableLeft,
        TableRight,
        TableHeaderLeft,
        TableHeaderRight,
        TableRowLeft,
        TableRowRight,
        TableItemLeft,
        TableItemRight,
        TableOfContents,
        Target,
        UnhandledFormat,
        UnknownCommand,
        Last = UnknownCommand
    };

    Atom(Type type, const QString& string = "")
        : next_(0), type_(type)
    {
        strs << string;
    }

    Atom(Type type, const QString& p1, const QString& p2)
        : next_(0), type_(type)
    {
        strs << p1;
        if (!p2.isEmpty())
            strs << p2;
    }

    Atom(Atom* previous, Type type, const QString& string = "")
        : next_(previous->next_), type_(type)
    {
        strs << string;
        previous->next_ = this;
    }

    Atom(Atom* previous, Type type, const QString& p1, const QString& p2)
        : next_(previous->next_), type_(type)
    {
        strs << p1;
        if (!p2.isEmpty())
            strs << p2;
        previous->next_ = this;
    }

    void appendChar(QChar ch) { strs[0] += ch; }
    void appendString(const QString& string) { strs[0] += string; }
    void chopString() { strs[0].chop(1); }
    void setString(const QString& string) { strs[0] = string; }
    Atom* next() { return next_; }
    void setNext(Atom* newNext) { next_ = newNext; }

    const Atom* next() const { return next_; }
    const Atom* next(Type t) const;
    const Atom* next(Type t, const QString& s) const;
    Type type() const { return type_; }
    QString typeString() const;
    const QString& string() const { return strs[0]; }
    const QString& string(int i) const { return strs[i]; }
    int count() const { return strs.size(); }
    void dump() const;

    static QLatin1String BOLD_;
    static QLatin1String INDEX_;
    static QLatin1String ITALIC_;
    static QLatin1String LINK_;
    static QLatin1String PARAMETER_;
    static QLatin1String SPAN_;
    static QLatin1String SUBSCRIPT_;
    static QLatin1String SUPERSCRIPT_;
    static QLatin1String TELETYPE_;
    static QLatin1String UICONTROL_;
    static QLatin1String UNDERLINE_;

    static QLatin1String BULLET_;
    static QLatin1String TAG_;
    static QLatin1String VALUE_;
    static QLatin1String LOWERALPHA_;
    static QLatin1String LOWERROMAN_;
    static QLatin1String NUMERIC_;
    static QLatin1String UPPERALPHA_;
    static QLatin1String UPPERROMAN_;

private:
    Atom* next_;
    Type type_;
    QStringList strs;
};

#define ATOM_FORMATTING_BOLD            "bold"
#define ATOM_FORMATTING_INDEX           "index"
#define ATOM_FORMATTING_ITALIC          "italic"
#define ATOM_FORMATTING_LINK            "link"
#define ATOM_FORMATTING_PARAMETER       "parameter"
#define ATOM_FORMATTING_SPAN            "span "
#define ATOM_FORMATTING_SUBSCRIPT       "subscript"
#define ATOM_FORMATTING_SUPERSCRIPT     "superscript"
#define ATOM_FORMATTING_TELETYPE        "teletype"
#define ATOM_FORMATTING_UICONTROL       "uicontrol"
#define ATOM_FORMATTING_UNDERLINE       "underline"

#define ATOM_LIST_BULLET                "bullet"
#define ATOM_LIST_TAG                   "tag"
#define ATOM_LIST_VALUE                 "value"
#define ATOM_LIST_LOWERALPHA            "loweralpha"
#define ATOM_LIST_LOWERROMAN            "lowerroman"
#define ATOM_LIST_NUMERIC               "numeric"
#define ATOM_LIST_UPPERALPHA            "upperalpha"
#define ATOM_LIST_UPPERROMAN            "upperroman"

QT_END_NAMESPACE

#endif

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

#include <qregexp.h>
#include "atom.h"
#include "location.h"
#include <stdio.h>

QT_BEGIN_NAMESPACE

QLatin1String Atom::BOLD_          ("bold");
QLatin1String Atom::INDEX_         ("index");
QLatin1String Atom::ITALIC_        ("italic");
QLatin1String Atom::LINK_          ("link");
QLatin1String Atom::PARAMETER_     ("parameter");
QLatin1String Atom::SPAN_          ("span");
QLatin1String Atom::SUBSCRIPT_     ("subscript");
QLatin1String Atom::SUPERSCRIPT_   ("superscript");
QLatin1String Atom::TELETYPE_      ("teletype");
QLatin1String Atom::UICONTROL_     ("uicontrol");
QLatin1String Atom::UNDERLINE_     ("underline");

QLatin1String Atom::BULLET_        ("bullet");
QLatin1String Atom::TAG_           ("tag");
QLatin1String Atom::VALUE_         ("value");
QLatin1String Atom::LOWERALPHA_    ("loweralpha");
QLatin1String Atom::LOWERROMAN_    ("lowerroman");
QLatin1String Atom::NUMERIC_       ("numeric");
QLatin1String Atom::UPPERALPHA_    ("upperalpha");
QLatin1String Atom::UPPERROMAN_    ("upperroman");

/*! \class Atom
    \brief The Atom class is the fundamental unit for representing
    documents internally.

  Atoms have a \i type and are completed by a \i string whose
  meaning depends on the \i type. For example, the string
  \quotation
      \i italic text looks nicer than \bold bold text
  \endquotation
  is represented by the following atoms:
  \quotation
      (FormattingLeft, ATOM_FORMATTING_ITALIC)
      (String, "italic")
      (FormattingRight, ATOM_FORMATTING_ITALIC)
      (String, " text is more attractive than ")
      (FormattingLeft, ATOM_FORMATTING_BOLD)
      (String, "bold")
      (FormattingRight, ATOM_FORMATTING_BOLD)
      (String, " text")
  \endquotation

  \also Text
*/

/*! \enum Atom::Type

  \value AbstractLeft
  \value AbstractRight
  \value AnnotatedList
  \value AutoLink
  \value BaseName
  \value BriefLeft
  \value BriefRight
  \value C
  \value CaptionLeft
  \value CaptionRight
  \value Code
  \value CodeBad
  \value CodeNew
  \value CodeOld
  \value CodeQuoteArgument
  \value CodeQuoteCommand
  \value DivLeft
  \value DivRight
  \value EndQmlText
  \value FormatElse
  \value FormatEndif
  \value FormatIf
  \value FootnoteLeft
  \value FootnoteRight
  \value FormattingLeft
  \value FormattingRight
  \value GeneratedList
  \value Image
  \value ImageText
  \value ImportantNote
  \value InlineImage
  \value LineBreak
  \value Link
  \value LinkNode
  \value ListLeft
  \value ListItemNumber
  \value ListTagLeft
  \value ListTagRight
  \value ListItemLeft
  \value ListItemRight
  \value ListRight
  \value Nop
  \value Note
  \value ParaLeft
  \value ParaRight
  \value Qml
  \value QmlText
  \value QuotationLeft
  \value QuotationRight
  \value RawString
  \value SectionLeft
  \value SectionRight
  \value SectionHeadingLeft
  \value SectionHeadingRight
  \value SidebarLeft
  \value SidebarRight
  \value SinceList
  \value String
  \value TableLeft
  \value TableRight
  \value TableHeaderLeft
  \value TableHeaderRight
  \value TableRowLeft
  \value TableRowRight
  \value TableItemLeft
  \value TableItemRight
  \value TableOfContents
  \value Target
  \value UnhandledFormat
  \value UnknownCommand
*/

static const struct {
    const char *english;
    int no;
} atms[] = {
    { "AbstractLeft", Atom::AbstractLeft },
    { "AbstractRight", Atom::AbstractRight },
    { "AnnotatedList", Atom::AnnotatedList },
    { "AutoLink", Atom::AutoLink },
    { "BaseName", Atom::BaseName },
    { "br", Atom::BR},
    { "BriefLeft", Atom::BriefLeft },
    { "BriefRight", Atom::BriefRight },
    { "C", Atom::C },
    { "CaptionLeft", Atom::CaptionLeft },
    { "CaptionRight", Atom::CaptionRight },
    { "Code", Atom::Code },
    { "CodeBad", Atom::CodeBad },
    { "CodeNew", Atom::CodeNew },
    { "CodeOld", Atom::CodeOld },
    { "CodeQuoteArgument", Atom::CodeQuoteArgument },
    { "CodeQuoteCommand", Atom::CodeQuoteCommand },
    { "DivLeft", Atom::DivLeft },
    { "DivRight", Atom::DivRight },
    { "EndQmlText", Atom::EndQmlText },
    { "FootnoteLeft", Atom::FootnoteLeft },
    { "FootnoteRight", Atom::FootnoteRight },
    { "FormatElse", Atom::FormatElse },
    { "FormatEndif", Atom::FormatEndif },
    { "FormatIf", Atom::FormatIf },
    { "FormattingLeft", Atom::FormattingLeft },
    { "FormattingRight", Atom::FormattingRight },
    { "GeneratedList", Atom::GeneratedList },
    { "GuidLink", Atom::GuidLink},
    { "hr", Atom::HR},
    { "Image", Atom::Image },
    { "ImageText", Atom::ImageText },
    { "ImportantLeft", Atom::ImportantLeft },
    { "ImportantRight", Atom::ImportantRight },
    { "InlineImage", Atom::InlineImage },
    { "JavaScript", Atom::JavaScript },
    { "EndJavaScript", Atom::EndJavaScript },
    { "LegaleseLeft", Atom::LegaleseLeft },
    { "LegaleseRight", Atom::LegaleseRight },
    { "LineBreak", Atom::LineBreak },
    { "Link", Atom::Link },
    { "LinkNode", Atom::LinkNode },
    { "ListLeft", Atom::ListLeft },
    { "ListItemNumber", Atom::ListItemNumber },
    { "ListTagLeft", Atom::ListTagLeft },
    { "ListTagRight", Atom::ListTagRight },
    { "ListItemLeft", Atom::ListItemLeft },
    { "ListItemRight", Atom::ListItemRight },
    { "ListRight", Atom::ListRight },
    { "Nop", Atom::Nop },
    { "NoteLeft", Atom::NoteLeft },
    { "NoteRight", Atom::NoteRight },
    { "ParaLeft", Atom::ParaLeft },
    { "ParaRight", Atom::ParaRight },
    { "Qml", Atom::Qml},
    { "QmlText", Atom::QmlText },
    { "QuotationLeft", Atom::QuotationLeft },
    { "QuotationRight", Atom::QuotationRight },
    { "RawString", Atom::RawString },
    { "SectionLeft", Atom::SectionLeft },
    { "SectionRight", Atom::SectionRight },
    { "SectionHeadingLeft", Atom::SectionHeadingLeft },
    { "SectionHeadingRight", Atom::SectionHeadingRight },
    { "SidebarLeft", Atom::SidebarLeft },
    { "SidebarRight", Atom::SidebarRight },
    { "SinceList", Atom::SinceList },
    { "SnippetCommand", Atom::SnippetCommand },
    { "SnippetIdentifier", Atom::SnippetIdentifier },
    { "SnippetLocation", Atom::SnippetLocation },
    { "String", Atom::String },
    { "TableLeft", Atom::TableLeft },
    { "TableRight", Atom::TableRight },
    { "TableHeaderLeft", Atom::TableHeaderLeft },
    { "TableHeaderRight", Atom::TableHeaderRight },
    { "TableRowLeft", Atom::TableRowLeft },
    { "TableRowRight", Atom::TableRowRight },
    { "TableItemLeft", Atom::TableItemLeft },
    { "TableItemRight", Atom::TableItemRight },
    { "TableOfContents", Atom::TableOfContents },
    { "Target", Atom::Target },
    { "UnhandledFormat", Atom::UnhandledFormat },
    { "UnknownCommand", Atom::UnknownCommand },
    { 0, 0 }
};

/*! \fn Atom::Atom(Type type, const QString& string)

  Constructs an atom of the specified \a type with the single
  parameter \a string and does not put the new atom in a list.
*/

/*! \fn Atom::Atom(Type type, const QString& p1, const QString& p2)

  Constructs an atom of the specified \a type with the two
  parameters \a p1 and \a p2 and does not put the new atom
  in a list.
*/

/*! \fn Atom(Atom *previous, Type type, const QString& string)

  Constructs an atom of the specified \a type with the single
  parameter \a string and inserts the new atom into the list
  after the \a previous atom.
*/

/*! \fn Atom::Atom(Atom* previous, Type type, const QString& p1, const QString& p2)

  Constructs an atom of the specified \a type with the two
  parameters \a p1 and \a p2 and inserts the new atom into
  the list after the \a previous atom.
*/

/*! \fn void Atom::appendChar(QChar ch)

  Appends \a ch to the string parameter of this atom.

  \also string()
*/

/*! \fn void Atom::appendString(const QString& string)

  Appends \a string to the string parameter of this atom.

  \also string()
*/

/*! \fn void Atom::chopString()

  \also string()
*/

/*! \fn Atom *Atom::next()
  Return the next atom in the atom list.
  \also type(), string()
*/

/*!
  Return the next Atom in the list if it is of Type \a t.
  Otherwise return 0.
 */
const Atom* Atom::next(Type t) const
{
    return (next_ && (next_->type() == t)) ? next_ : 0;
}

/*!
  Return the next Atom in the list if it is of Type \a t
  and its string part is \a s. Otherwise return 0.
 */
const Atom* Atom::next(Type t, const QString& s) const
{
    return (next_ && (next_->type() == t) && (next_->string() == s)) ? next_ : 0;
}

/*! \fn const Atom *Atom::next() const
  Return the next atom in the atom list.
  \also type(), string()
*/

/*! \fn Type Atom::type() const
  Return the type of this atom.
  \also string(), next()
*/

/*!
  Return the type of this atom as a string. Return "Invalid" if
  type() returns an impossible value.

  This is only useful for debugging.

  \also type()
*/
QString Atom::typeString() const
{
    static bool deja = false;

    if (!deja) {
        int i = 0;
        while (atms[i].english != 0) {
            if (atms[i].no != i)
                Location::internalError(QCoreApplication::translate("QDoc::Atom", "atom %1 missing").arg(i));
            i++;
        }
        deja = true;
    }

    int i = (int) type();
    if (i < 0 || i > (int) Last)
        return QLatin1String("Invalid");
    return QLatin1String(atms[i].english);
}

/*! \fn const QString& Atom::string() const

  Returns the string parameter that together with the type
  characterizes this atom.

  \also type(), next()
*/

/*!
  Dumps this Atom to stderr in printer friendly form.
 */
void Atom::dump() const
{
    QString str = string();
    str.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    str.replace(QLatin1String("\""), QLatin1String("\\\""));
    str.replace(QLatin1String("\n"), QLatin1String("\\n"));
    str.replace(QRegExp(QLatin1String("[^\x20-\x7e]")), QLatin1String("?"));
    if (!str.isEmpty())
        str = QLatin1String(" \"") + str + QLatin1Char('"');
    fprintf(stderr,
            "    %-15s%s\n",
            typeString().toLatin1().data(),
            str.toLatin1().data());
}

QT_END_NAMESPACE

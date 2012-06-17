/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

/* This is the default Qt style sheet.

   IMPORTANT: This style sheet is primarily meant for defining feature
   capablities of styles. Do NOT add default styling rules here. When in
   doubt ask the stylesheet maintainer.

   The stylesheet in here used to be in a CSS file, but was moved here to
   avoid parsing overhead.
*/

#include "private/qcssparser_p.h"
#include "qstylesheetstyle_p.h"

#ifndef QT_NO_STYLE_STYLESHEET

QT_BEGIN_NAMESPACE

using namespace QCss;

// This is the class name of the selector.
// Use an empty string where you would use '*' in CSS.
// Ex. QHeaderView

#define SET_ELEMENT_NAME(x) \
    bSelector.elementName = (x)

// This acts as both pseudo state and sub control. The first parameter is the
// string name, and the second is the PseudoClass_* constant.
// The sub control specifier is always the first, and has the type
// PseudoClass_Unknown.
// If there is no PseudoClass_Unknown as the first pseudo, it is assumed to be
// a pseudo state.
// Ex. QComboBox::drop-down:enabled
//                   ^         ^

#define ADD_PSEUDO(x, y) \
    pseudo.type = (y); \
    pseudo.name = (x); \
    bSelector.pseudos << pseudo

// This is attributes. The third parameter is AttributeSelector::*
// Ex. QComboBox[style="QWindowsXPStyle"]
//                 ^           ^

#define ADD_ATTRIBUTE_SELECTOR(x, y, z) \
    attr.name = (x); \
    attr.value = (y); \
    attr.valueMatchCriterium = (z); \
    bSelector.attributeSelectors << attr

// Adds the current basic selector to the rule.
// Several basic selectors behave as AND (space in CSS).

#define ADD_BASIC_SELECTOR \
    selector.basicSelectors << bSelector; \
    bSelector.ids.clear(); \
    bSelector.pseudos.clear(); \
    bSelector.attributeSelectors.clear()

// Adds the current selector to the rule.
// Several selectors behave as OR (comma in CSS).

#define ADD_SELECTOR \
    styleRule.selectors << selector; \
    selector.basicSelectors.clear()

// Sets the name of a property.
// Ex. background: red;
//         ^

#define SET_PROPERTY(x, y) \
    decl.d->property = (x); \
    decl.d->propertyId = (y)

// Adds a value to the current property.
// The first parameter should be Value::KnownIdentifier if the value can be
// found among the Value_* constants, in which case the second should be that
// constant. Otherwise the first parameter is Value::Identifier and the second
// is a string.
// Adding more values is the same as seperating by spaces in CSS.
// Ex. border: 2px solid black;
//              ^    ^     ^

#define ADD_VALUE(x, y) \
    value.type = (x); \
    value.variant = (y); \
    decl.d->values << value

// Adds the current declaration to the rule.
// Ex. border: 2px solid black;
//     \----------------------/

#define ADD_DECLARATION \
    styleRule.declarations << decl; \
    decl.d.detach(); \
    decl.d->values.clear()

// Adds the rule to the stylesheet.
// Use at the end of every CSS block.

#define ADD_STYLE_RULE \
    sheet.styleRules << styleRule; \
    styleRule.selectors.clear(); \
    styleRule.declarations.clear()

StyleSheet QStyleSheetStyle::getDefaultStyleSheet() const
{
    StyleSheet sheet;
    StyleRule styleRule;
    BasicSelector bSelector;
    Selector selector;
    Declaration decl;
    QCss::Value value;
    Pseudo pseudo;
    AttributeSelector attr;

    // pixmap based style doesn't support any features
    bool styleIsPixmapBased = baseStyle()->inherits("QMacStyle")
                           || baseStyle()->inherits("QWindowsXPStyle")
                           || baseStyle()->inherits("QGtkStyle")
                           || baseStyle()->inherits("QS60Style");


    /*QLineEdit {
        -qt-background-role: base;
        border: native;
        -qt-style-features: background-color;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QLineEdit"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        SET_PROPERTY(QLatin1String("-qt-style-features"), QtStyleFeatures);
        ADD_VALUE(Value::Identifier, QString::fromLatin1("background-color"));
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QLineEdit:no-frame {
        border: none;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QLineEdit"));
        ADD_PSEUDO(QLatin1String("no-frame"), PseudoClass_Frameless);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_None);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QFrame {
        border: native;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QFrame"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QLabel, QToolBox {
        background: none;
        border-image: none;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QLabel"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_ELEMENT_NAME(QLatin1String("QToolBox"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("background"), Background);
        ADD_VALUE(Value::KnownIdentifier, Value_None);
        ADD_DECLARATION;

        SET_PROPERTY(QLatin1String("border-image"), BorderImage);
        ADD_VALUE(Value::KnownIdentifier, Value_None);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QGroupBox {
        border: native;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QGroupBox"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }


    /*QToolTip {
        -qt-background-role: window;
        border: native;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QToolTip"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QPushButton, QToolButton {
        border-style: native;
        -qt-style-features: background-color;  //only for not pixmap based styles
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QPushButton"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_ELEMENT_NAME(QLatin1String("QToolButton"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border-style"), BorderStyles);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        if (!styleIsPixmapBased) {
            SET_PROPERTY(QLatin1String("-qt-style-features"), QtStyleFeatures);
            ADD_VALUE(Value::Identifier, QString::fromLatin1("background-color"));
            ADD_DECLARATION;
        }


        ADD_STYLE_RULE;
    }


    /*QComboBox {
        border: native;
        -qt-style-features: background-color background-gradient;   //only for not pixmap based styles
        -qt-background-role: base;
    }*/

    {
        SET_ELEMENT_NAME(QLatin1String("QComboBox"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        if (!styleIsPixmapBased) {
            SET_PROPERTY(QLatin1String("-qt-style-features"), QtStyleFeatures);
            ADD_VALUE(Value::Identifier, QString::fromLatin1("background-color"));
            ADD_VALUE(Value::Identifier, QString::fromLatin1("background-gradient"));
            ADD_DECLARATION;
        }

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QComboBox[style="QPlastiqueStyle"][readOnly="true"],
    QComboBox[style="QCleanlooksStyle"][readOnly="true"]
    {
        -qt-background-role: button;
    }*/
    if (baseStyle()->inherits("QPlastiqueStyle")  || baseStyle()->inherits("QCleanlooksStyle"))
    {
        SET_ELEMENT_NAME(QLatin1String("QComboBox"));
        ADD_ATTRIBUTE_SELECTOR(QLatin1String("readOnly"), QLatin1String("true"), AttributeSelector::MatchEqual);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Button);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QAbstractSpinBox {
        border: native;
        -qt-style-features: background-color;
        -qt-background-role: base;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QAbstractSpinBox"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        SET_PROPERTY(QLatin1String("-qt-style-features"), QtStyleFeatures);
        ADD_VALUE(Value::Identifier, QString::fromLatin1("background-color"));
        ADD_DECLARATION;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QMenu {
        -qt-background-role: window;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QMenu"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }
    /*QMenu::item {
        -qt-style-features: background-color;
    }*/
    if (!styleIsPixmapBased) {
        SET_ELEMENT_NAME(QLatin1String("QMenu"));
        ADD_PSEUDO(QLatin1String("item"), PseudoClass_Unknown);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-style-features"), QtStyleFeatures);
        ADD_VALUE(Value::Identifier, QString::fromLatin1("background-color"));
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QHeaderView {
        -qt-background-role: window;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QHeaderView"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QTableCornerButton::section, QHeaderView::section {
        -qt-background-role: button;
        -qt-style-features: background-color; //if style is not pixmap based
        border: native;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QTableCornerButton"));
        ADD_PSEUDO(QLatin1String("section"), PseudoClass_Unknown);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_ELEMENT_NAME(QLatin1String("QHeaderView"));
        ADD_PSEUDO(QLatin1String("section"), PseudoClass_Unknown);
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Button);
        ADD_DECLARATION;

        if (!styleIsPixmapBased) {
            SET_PROPERTY(QLatin1String("-qt-style-features"), QtStyleFeatures);
            ADD_VALUE(Value::Identifier, QString::fromLatin1("background-color"));
            ADD_DECLARATION;
        }

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QProgressBar {
        -qt-background-role: base;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QProgressBar"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Base);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QScrollBar {
        -qt-background-role: window;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QScrollBar"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("-qt-background-role"), QtBackgroundRole);
        ADD_VALUE(Value::KnownIdentifier, Value_Window);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    /*QDockWidget {
        border: native;
    }*/
    {
        SET_ELEMENT_NAME(QLatin1String("QDockWidget"));
        ADD_BASIC_SELECTOR;
        ADD_SELECTOR;

        SET_PROPERTY(QLatin1String("border"), Border);
        ADD_VALUE(Value::KnownIdentifier, Value_Native);
        ADD_DECLARATION;

        ADD_STYLE_RULE;
    }

    sheet.origin = StyleSheetOrigin_UserAgent;
    sheet.buildIndexes();
    return sheet;
}

#endif // #ifndef QT_NO_STYLE_STYLESHEET

QT_END_NAMESPACE

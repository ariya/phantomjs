/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "cppwriteinitialization.h"
#include "cppwriteiconinitialization.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"
#include "uic.h"
#include "databaseinfo.h"
#include "globaldefs.h"

#include <QtCore/QTextStream>
#include <QtCore/QDebug>

#include <ctype.h>

QT_BEGIN_NAMESPACE

namespace {
    // Fixup an enumeration name from class Qt.
    // They are currently stored as "BottomToolBarArea" instead of "Qt::BottomToolBarArea".
    // due to MO issues. This might be fixed in the future.
    void fixQtEnumerationName(QString& name) {
        static const QLatin1String prefix("Qt::");
        if (name.indexOf(prefix) != 0)
            name.prepend(prefix);
    }
    // figure out the toolbar area of a DOM attrib list.
    // By legacy, it is stored as an integer. As of 4.3.0, it is the enumeration value.
    QString toolBarAreaStringFromDOMAttributes(const CPP::WriteInitialization::DomPropertyMap &attributes) {
        const DomProperty *pstyle = attributes.value(QLatin1String("toolBarArea"));
        if (!pstyle)
            return QString();

        switch (pstyle->kind()) {
        case DomProperty::Number: {
            QString area = QLatin1String("static_cast<Qt::ToolBarArea>(");
            area += QString::number(pstyle->elementNumber());
            area += QLatin1String("), ");
            return area;
        }
        case DomProperty::Enum: {
            QString area = pstyle->elementEnum();
            fixQtEnumerationName(area);
            area += QLatin1String(", ");
            return area;
        }
        default:
            break;
        }
        return QString();
    }

    // Write a statement to create a spacer item.
    void writeSpacerItem(const DomSpacer *node, QTextStream &output) {
        const QHash<QString, DomProperty *> properties = propertyMap(node->elementProperty());
                output << "new QSpacerItem(";

        if (properties.contains(QLatin1String("sizeHint"))) {
            const DomSize *sizeHint = properties.value(QLatin1String("sizeHint"))->elementSize();
            output << sizeHint->elementWidth() << ", " << sizeHint->elementHeight() << ", ";
        }

        // size type
        QString sizeType = properties.contains(QLatin1String("sizeType"))  ?
                           properties.value(QLatin1String("sizeType"))->elementEnum() :
                           QString::fromLatin1("Expanding");

        if (!sizeType.startsWith(QLatin1String("QSizePolicy::")))
            sizeType.prepend(QLatin1String("QSizePolicy::"));
        // orientation
        bool isVspacer = false;
        if (properties.contains(QLatin1String("orientation"))) {
            const QString orientation = properties.value(QLatin1String("orientation"))->elementEnum();
            if (orientation == QLatin1String("Qt::Vertical") || orientation == QLatin1String("Vertical"))  isVspacer = true;
        }

        if (isVspacer)
            output << "QSizePolicy::Minimum, " << sizeType << ')';
        else
            output << sizeType << ", QSizePolicy::Minimum)";
    }


    // Helper for implementing comparison functions for integers.
    int compareInt(int i1, int i2) {
        if (i1 < i2) return -1;
        if (i1 > i2) return  1;
        return  0;
    }

    // Write object->setFoo(x);
    template <class Value>
        void writeSetter(const QString &indent, const QString &varName,const QString &setter, Value v, QTextStream &str) {
            str << indent << varName << "->" << setter << '(' << v << ");\n";
        }

    void writeSetupUIScriptVariableDeclarations(const QString &indent, QTextStream &str)  {
        str << indent << "ScriptContext scriptContext;\n"
            << indent << "QWidgetList childWidgets;\n";
    }

    static inline bool iconHasStatePixmaps(const DomResourceIcon *i) {
        return i->hasElementNormalOff()   || i->hasElementNormalOn() ||
               i->hasElementDisabledOff() || i->hasElementDisabledOn() ||
               i->hasElementActiveOff()   || i->hasElementActiveOn() ||
               i->hasElementSelectedOff() || i->hasElementSelectedOn();
    }

    static inline bool isIconFormat44(const DomResourceIcon *i) {
        return iconHasStatePixmaps(i) || !i->attributeTheme().isEmpty();
    }

    // Check on properties. Filter out empty legacy pixmap/icon properties
    // as Designer pre 4.4 used to remove missing resource references.
    // This can no longer be handled by the code as we have 'setIcon(QIcon())' as well as 'QIcon icon'
    static bool checkProperty(const QString &fileName, const DomProperty *p) {
        switch (p->kind()) {
        case DomProperty::IconSet:
            if (const DomResourceIcon *dri = p->elementIconSet()) {
                if (!isIconFormat44(dri)) {
                    if (dri->text().isEmpty())  {
                        const QString msg = QString::fromUtf8("%1: Warning: An invalid icon property '%2' was encountered.").arg(fileName).arg(p->attributeName());
                        qWarning("%s", qPrintable(msg));
                        return false;
                    }
                }
            }
            break;
        case DomProperty::Pixmap:
            if (const DomResourcePixmap *drp = p->elementPixmap())
                if (drp->text().isEmpty()) {
                    const QString msg = QString::fromUtf8("%1: Warning: An invalid pixmap property '%2' was encountered.").arg(fileName).arg(p->attributeName());
                    qWarning("%s", qPrintable(msg));
                    return false;
                }
            break;
        default:
            break;
        }
        return  true;
    }

    inline void openIfndef(QTextStream &str, const QString &symbol) { if (!symbol.isEmpty()) str << QLatin1String("#ifndef ") << symbol << endl;  }
    inline void closeIfndef(QTextStream &str, const QString &symbol) { if (!symbol.isEmpty()) str << QLatin1String("#endif // ") << symbol << endl; }

    const char *accessibilityDefineC = "QT_NO_ACCESSIBILITY";
    const char *toolTipDefineC = "QT_NO_TOOLTIP";
    const char *whatsThisDefineC = "QT_NO_WHATSTHIS";
    const char *statusTipDefineC = "QT_NO_STATUSTIP";
    const char *shortcutDefineC = "QT_NO_SHORTCUT";
}

namespace CPP {

FontHandle::FontHandle(const DomFont *domFont) :
      m_domFont(domFont)
{
}

int FontHandle::compare(const FontHandle &rhs) const
{
    const QString family    = m_domFont->hasElementFamily()     ?     m_domFont->elementFamily() : QString();
    const QString rhsFamily = rhs.m_domFont->hasElementFamily() ? rhs.m_domFont->elementFamily() : QString();

    if (const int frc = family.compare(rhsFamily))
        return frc;

    const int pointSize    = m_domFont->hasElementPointSize()     ?     m_domFont->elementPointSize() : -1;
    const int rhsPointSize = rhs.m_domFont->hasElementPointSize() ? rhs.m_domFont->elementPointSize() : -1;

    if (const int crc = compareInt(pointSize, rhsPointSize))
        return crc;

    const int bold    = m_domFont->hasElementBold()     ? (m_domFont->elementBold()     ? 1 : 0) : -1;
    const int rhsBold = rhs.m_domFont->hasElementBold() ? (rhs.m_domFont->elementBold() ? 1 : 0) : -1;
    if (const int crc = compareInt(bold, rhsBold))
        return crc;

    const int italic    = m_domFont->hasElementItalic()     ? (m_domFont->elementItalic()     ? 1 : 0) : -1;
    const int rhsItalic = rhs.m_domFont->hasElementItalic() ? (rhs.m_domFont->elementItalic() ? 1 : 0) : -1;
    if (const int crc = compareInt(italic, rhsItalic))
        return crc;

    const int underline    = m_domFont->hasElementUnderline()     ? (m_domFont->elementUnderline()     ? 1 : 0) : -1;
    const int rhsUnderline = rhs.m_domFont->hasElementUnderline() ? (rhs.m_domFont->elementUnderline() ? 1 : 0) : -1;
    if (const int crc = compareInt(underline, rhsUnderline))
        return crc;

    const int weight    = m_domFont->hasElementWeight()     ?     m_domFont->elementWeight() : -1;
    const int rhsWeight = rhs.m_domFont->hasElementWeight() ? rhs.m_domFont->elementWeight() : -1;
    if (const int crc = compareInt(weight, rhsWeight))
        return crc;

    const int strikeOut    = m_domFont->hasElementStrikeOut()     ? (m_domFont->elementStrikeOut()     ? 1 : 0) : -1;
    const int rhsStrikeOut = rhs.m_domFont->hasElementStrikeOut() ? (rhs.m_domFont->elementStrikeOut() ? 1 : 0) : -1;
    if (const int crc = compareInt(strikeOut, rhsStrikeOut))
        return crc;

    const int kerning    = m_domFont->hasElementKerning()     ? (m_domFont->elementKerning()     ? 1 : 0) : -1;
    const int rhsKerning = rhs.m_domFont->hasElementKerning() ? (rhs.m_domFont->elementKerning() ? 1 : 0) : -1;
    if (const int crc = compareInt(kerning, rhsKerning))
        return crc;

    const int antialiasing    = m_domFont->hasElementAntialiasing()     ? (m_domFont->elementAntialiasing()     ? 1 : 0) : -1;
    const int rhsAntialiasing = rhs.m_domFont->hasElementAntialiasing() ? (rhs.m_domFont->elementAntialiasing() ? 1 : 0) : -1;
    if (const int crc = compareInt(antialiasing, rhsAntialiasing))
        return crc;

    const QString styleStrategy    = m_domFont->hasElementStyleStrategy()     ?     m_domFont->elementStyleStrategy() : QString();
    const QString rhsStyleStrategy = rhs.m_domFont->hasElementStyleStrategy() ? rhs.m_domFont->elementStyleStrategy() : QString();

    if (const int src = styleStrategy.compare(rhsStyleStrategy))
        return src;

    return 0;
}

IconHandle::IconHandle(const DomResourceIcon *domIcon) :
      m_domIcon(domIcon)
{
}

int IconHandle::compare(const IconHandle &rhs) const
{
    if (const int comp = m_domIcon->attributeTheme().compare(rhs.m_domIcon->attributeTheme()))
        return comp;

    const QString normalOff    =     m_domIcon->hasElementNormalOff() ?     m_domIcon->elementNormalOff()->text() : QString();
    const QString rhsNormalOff = rhs.m_domIcon->hasElementNormalOff() ? rhs.m_domIcon->elementNormalOff()->text() : QString();
    if (const int comp = normalOff.compare(rhsNormalOff))
        return comp;

    const QString normalOn    =     m_domIcon->hasElementNormalOn() ?     m_domIcon->elementNormalOn()->text() : QString();
    const QString rhsNormalOn = rhs.m_domIcon->hasElementNormalOn() ? rhs.m_domIcon->elementNormalOn()->text() : QString();
    if (const int comp = normalOn.compare(rhsNormalOn))
        return comp;

    const QString disabledOff    =     m_domIcon->hasElementDisabledOff() ?     m_domIcon->elementDisabledOff()->text() : QString();
    const QString rhsDisabledOff = rhs.m_domIcon->hasElementDisabledOff() ? rhs.m_domIcon->elementDisabledOff()->text() : QString();
    if (const int comp = disabledOff.compare(rhsDisabledOff))
        return comp;

    const QString disabledOn    =     m_domIcon->hasElementDisabledOn() ?     m_domIcon->elementDisabledOn()->text() : QString();
    const QString rhsDisabledOn = rhs.m_domIcon->hasElementDisabledOn() ? rhs.m_domIcon->elementDisabledOn()->text() : QString();
    if (const int comp = disabledOn.compare(rhsDisabledOn))
        return comp;

    const QString activeOff    =     m_domIcon->hasElementActiveOff() ?     m_domIcon->elementActiveOff()->text() : QString();
    const QString rhsActiveOff = rhs.m_domIcon->hasElementActiveOff() ? rhs.m_domIcon->elementActiveOff()->text() : QString();
    if (const int comp = activeOff.compare(rhsActiveOff))
        return comp;

    const QString activeOn    =     m_domIcon->hasElementActiveOn() ?     m_domIcon->elementActiveOn()->text() : QString();
    const QString rhsActiveOn = rhs.m_domIcon->hasElementActiveOn() ? rhs.m_domIcon->elementActiveOn()->text() : QString();
    if (const int comp = activeOn.compare(rhsActiveOn))
        return comp;

    const QString selectedOff    =     m_domIcon->hasElementSelectedOff() ?     m_domIcon->elementSelectedOff()->text() : QString();
    const QString rhsSelectedOff = rhs.m_domIcon->hasElementSelectedOff() ? rhs.m_domIcon->elementSelectedOff()->text() : QString();
    if (const int comp = selectedOff.compare(rhsSelectedOff))
        return comp;

    const QString selectedOn    =     m_domIcon->hasElementSelectedOn() ?     m_domIcon->elementSelectedOn()->text() : QString();
    const QString rhsSelectedOn = rhs.m_domIcon->hasElementSelectedOn() ? rhs.m_domIcon->elementSelectedOn()->text() : QString();
    if (const int comp = selectedOn.compare(rhsSelectedOn))
        return comp;
    // Pre 4.4 Legacy
    if (const int comp = m_domIcon->text().compare(rhs.m_domIcon->text()))
        return comp;

    return 0;
}


#if defined(Q_OS_MAC) && defined(Q_CC_GNU) && (__GNUC__ == 3 && __GNUC_MINOR__ == 3)
inline uint qHash(const SizePolicyHandle &handle) { return qHash(handle.m_domSizePolicy); }
inline uint qHash(const FontHandle &handle) { return qHash(handle.m_domFont); }
inline uint qHash(const IconHandle &handle) { return qHash(handle.m_domIcon); }
#endif

SizePolicyHandle::SizePolicyHandle(const DomSizePolicy *domSizePolicy) :
    m_domSizePolicy(domSizePolicy)
{
}

int SizePolicyHandle::compare(const SizePolicyHandle &rhs) const
{

    const int hSizeType    = m_domSizePolicy->hasElementHSizeType()     ? m_domSizePolicy->elementHSizeType()     : -1;
    const int rhsHSizeType = rhs.m_domSizePolicy->hasElementHSizeType() ? rhs.m_domSizePolicy->elementHSizeType() : -1;
    if (const int crc = compareInt(hSizeType, rhsHSizeType))
        return crc;

    const int vSizeType    = m_domSizePolicy->hasElementVSizeType()     ? m_domSizePolicy->elementVSizeType()     : -1;
    const int rhsVSizeType = rhs.m_domSizePolicy->hasElementVSizeType() ? rhs.m_domSizePolicy->elementVSizeType() : -1;
    if (const int crc = compareInt(vSizeType, rhsVSizeType))
        return crc;

    const int hStretch    =  m_domSizePolicy->hasElementHorStretch()     ? m_domSizePolicy->elementHorStretch()     : -1;
    const int rhsHStretch =  rhs.m_domSizePolicy->hasElementHorStretch() ? rhs.m_domSizePolicy->elementHorStretch() : -1;
    if (const int crc = compareInt(hStretch, rhsHStretch))
        return crc;

    const int vStretch    =  m_domSizePolicy->hasElementVerStretch()     ? m_domSizePolicy->elementVerStretch()     : -1;
    const int rhsVStretch =  rhs.m_domSizePolicy->hasElementVerStretch() ? rhs.m_domSizePolicy->elementVerStretch() : -1;
    if (const int crc = compareInt(vStretch, rhsVStretch))
        return crc;

    const QString attributeHSizeType    = m_domSizePolicy->hasAttributeHSizeType()     ? m_domSizePolicy->attributeHSizeType()     : QString();
    const QString rhsAttributeHSizeType = rhs.m_domSizePolicy->hasAttributeHSizeType() ? rhs.m_domSizePolicy->attributeHSizeType() : QString();

    if (const int hrc = attributeHSizeType.compare(rhsAttributeHSizeType))
        return hrc;

    const QString attributeVSizeType    = m_domSizePolicy->hasAttributeVSizeType()     ? m_domSizePolicy->attributeVSizeType()     : QString();
    const QString rhsAttributeVSizeType = rhs.m_domSizePolicy->hasAttributeVSizeType() ? rhs.m_domSizePolicy->attributeVSizeType() : QString();

    return attributeVSizeType.compare(rhsAttributeVSizeType);
}

// ---  WriteInitialization: LayoutDefaultHandler

WriteInitialization::LayoutDefaultHandler::LayoutDefaultHandler()
{
    qFill(m_state, m_state + NumProperties, 0u);
    qFill(m_defaultValues, m_defaultValues + NumProperties, 0);
}



void WriteInitialization::LayoutDefaultHandler::acceptLayoutDefault(DomLayoutDefault *node)
{
    if (!node)
        return;
    if (node->hasAttributeMargin()) {
        m_state[Margin] |= HasDefaultValue;
        m_defaultValues[Margin] = node->attributeMargin();
    }
    if (node->hasAttributeSpacing()) {
        m_state[Spacing] |= HasDefaultValue;
        m_defaultValues[Spacing]  = node->attributeSpacing();
    }
}

void WriteInitialization::LayoutDefaultHandler::acceptLayoutFunction(DomLayoutFunction *node)
{
    if (!node)
        return;
    if (node->hasAttributeMargin()) {
        m_state[Margin]     |= HasDefaultFunction;
        m_functions[Margin] =  node->attributeMargin();
        m_functions[Margin] += QLatin1String("()");
    }
    if (node->hasAttributeSpacing()) {
        m_state[Spacing]     |= HasDefaultFunction;
        m_functions[Spacing] =  node->attributeSpacing();
        m_functions[Spacing] += QLatin1String("()");
    }
}

static inline void writeContentsMargins(const QString &indent, const QString &objectName, int value, QTextStream &str)
{
     QString contentsMargins;
     QTextStream(&contentsMargins) << value << ", " << value << ", " << value << ", " << value;
     writeSetter(indent, objectName, QLatin1String("setContentsMargins"), contentsMargins, str);
 }

void WriteInitialization::LayoutDefaultHandler::writeProperty(int p, const QString &indent, const QString &objectName,
                                                              const DomPropertyMap &properties, const QString &propertyName, const QString &setter,
                                                              int defaultStyleValue, bool suppressDefault, QTextStream &str) const
{
    // User value
    const DomPropertyMap::const_iterator mit = properties.constFind(propertyName);
    const bool found = mit != properties.constEnd();
    if (found) {
        const int value = mit.value()->elementNumber();
        // Emulate the pre 4.3 behaviour: The value form default value was only used to determine
        // the default value, layout properties were always written
        const bool useLayoutFunctionPre43 = !suppressDefault && (m_state[p] == (HasDefaultFunction|HasDefaultValue)) && value == m_defaultValues[p];
        if (!useLayoutFunctionPre43) {
            bool ifndefMac = (!(m_state[p] & (HasDefaultFunction|HasDefaultValue))
                             && value == defaultStyleValue);
            if (ifndefMac)
                str << "#ifndef Q_OS_MAC\n";
            if (p == Margin) { // Use setContentsMargins for numeric values
                writeContentsMargins(indent, objectName, value, str);
            } else {
                writeSetter(indent, objectName, setter, value, str);
            }
            if (ifndefMac)
                str << "#endif\n";
            return;
        }
    }
    if (suppressDefault)
        return;
    // get default.
    if (m_state[p] & HasDefaultFunction) {
        // Do not use setContentsMargins to avoid repetitive evaluations.
        writeSetter(indent, objectName, setter, m_functions[p], str);
        return;
    }
    if (m_state[p] & HasDefaultValue) {
        if (p == Margin) { // Use setContentsMargins for numeric values
            writeContentsMargins(indent, objectName, m_defaultValues[p], str);
        } else {
            writeSetter(indent, objectName, setter, m_defaultValues[p], str);
        }
    }
    return;
}


void WriteInitialization::LayoutDefaultHandler::writeProperties(const QString &indent, const QString &varName,
                                                                const DomPropertyMap &properties, int marginType,
                                                                bool suppressMarginDefault,
                                                                QTextStream &str) const {
    // Write out properties and ignore the ones found in
    // subsequent writing of the property list.
    int defaultSpacing = marginType == WriteInitialization::Use43UiFile ? -1 : 6;
    writeProperty(Spacing, indent, varName, properties, QLatin1String("spacing"), QLatin1String("setSpacing"),
                  defaultSpacing, false, str);
    // We use 9 as TopLevelMargin, since Designer seem to always use 9.
    static const int layoutmargins[4] = {-1, 9, 9, 0};
    writeProperty(Margin,  indent, varName, properties, QLatin1String("margin"),  QLatin1String("setMargin"),
                  layoutmargins[marginType], suppressMarginDefault, str);
}

static bool needsTranslation(DomString *str)
{
    if (!str)
        return false;
    return !str->hasAttributeNotr() || !toBool(str->attributeNotr());
}

// ---  WriteInitialization
WriteInitialization::WriteInitialization(Uic *uic, bool activateScripts) :
      m_uic(uic),
      m_driver(uic->driver()), m_output(uic->output()), m_option(uic->option()),
      m_indent(m_option.indent + m_option.indent),
      m_dindent(m_indent + m_option.indent),
      m_stdsetdef(true),
      m_layoutMarginType(TopLevelMargin),
      m_mainFormUsedInRetranslateUi(false),
      m_delayedOut(&m_delayedInitialization, QIODevice::WriteOnly),
      m_refreshOut(&m_refreshInitialization, QIODevice::WriteOnly),
      m_actionOut(&m_delayedActionInitialization, QIODevice::WriteOnly),
      m_activateScripts(activateScripts), m_layoutWidget(false),
      m_firstThemeIcon(true)
{
}

void WriteInitialization::acceptUI(DomUI *node)
{
    m_registeredImages.clear();
    m_actionGroupChain.push(0);
    m_widgetChain.push(0);
    m_layoutChain.push(0);

    acceptLayoutDefault(node->elementLayoutDefault());
    acceptLayoutFunction(node->elementLayoutFunction());

    if (node->elementCustomWidgets())
        TreeWalker::acceptCustomWidgets(node->elementCustomWidgets());

    if (node->elementImages())
        TreeWalker::acceptImages(node->elementImages());

    if (m_option.generateImplemetation)
        m_output << "#include <" << m_driver->headerFileName() << ">\n\n";

    m_stdsetdef = true;
    if (node->hasAttributeStdSetDef())
        m_stdsetdef = node->attributeStdSetDef();

    const QString className = node->elementClass() + m_option.postfix;
    m_generatedClass = className;

    const QString varName = m_driver->findOrInsertWidget(node->elementWidget());
    m_mainFormVarName = varName;
    m_registeredWidgets.insert(varName, node->elementWidget()); // register the main widget

    const QString widgetClassName = node->elementWidget()->attributeClass();

    m_output << m_option.indent << "void " << "setupUi(" << widgetClassName << " *" << varName << ")\n"
           << m_option.indent << "{\n";

    if (m_activateScripts)
        writeSetupUIScriptVariableDeclarations(m_indent, m_output);

    const QStringList connections = m_uic->databaseInfo()->connections();
    for (int i=0; i<connections.size(); ++i) {
        QString connection = connections.at(i);

        if (connection == QLatin1String("(default)"))
            continue;

        const QString varConn = connection + QLatin1String("Connection");
        m_output << m_indent << varConn << " = QSqlDatabase::database(" << fixString(connection, m_dindent) << ");\n";
    }

    acceptWidget(node->elementWidget());

    if (m_buddies.size() > 0)
        openIfndef(m_output, QLatin1String(shortcutDefineC));
    for (int i=0; i<m_buddies.size(); ++i) {
        const Buddy &b = m_buddies.at(i);

        if (!m_registeredWidgets.contains(b.objName)) {
            fprintf(stderr, "%s: Warning: Buddy assignment: '%s' is not a valid widget.\n",
                    qPrintable(m_option.messagePrefix()),
                    b.objName.toLatin1().data());
            continue;
        } else if (!m_registeredWidgets.contains(b.buddy)) {
            fprintf(stderr, "%s: Warning: Buddy assignment: '%s' is not a valid widget.\n",
                    qPrintable(m_option.messagePrefix()),
                    b.buddy.toLatin1().data());
            continue;
        }

        m_output << m_indent << b.objName << "->setBuddy(" << b.buddy << ");\n";
    }
    if (m_buddies.size() > 0)
        closeIfndef(m_output, QLatin1String(shortcutDefineC));

    if (node->elementTabStops())
        acceptTabStops(node->elementTabStops());

    if (m_delayedActionInitialization.size())
        m_output << "\n" << m_delayedActionInitialization;

    m_output << "\n" << m_indent << "retranslateUi(" << varName << ");\n";

    if (node->elementConnections())
        acceptConnections(node->elementConnections());

    if (!m_delayedInitialization.isEmpty())
        m_output << "\n" << m_delayedInitialization << "\n";

    if (m_option.autoConnection)
        m_output << "\n" << m_indent << "QMetaObject::connectSlotsByName(" << varName << ");\n";

    m_output << m_option.indent << "} // setupUi\n\n";

    if (!m_mainFormUsedInRetranslateUi) {
        m_refreshInitialization += m_indent;
        m_refreshInitialization += QLatin1String("Q_UNUSED(");
        m_refreshInitialization += varName ;
        m_refreshInitialization += QLatin1String(");\n");
    }

    m_output << m_option.indent << "void " << "retranslateUi(" << widgetClassName << " *" << varName << ")\n"
           << m_option.indent << "{\n"
           << m_refreshInitialization
           << m_option.indent << "} // retranslateUi\n\n";

    m_layoutChain.pop();
    m_widgetChain.pop();
    m_actionGroupChain.pop();
}

void WriteInitialization::addWizardPage(const QString &pageVarName, const DomWidget *page, const QString &parentWidget)
{
    /* If the node has a (free-format) string "pageId" attribute (which could
     * an integer or an enumeration value), use setPage(), else addPage(). */
    QString id;
    const DomPropertyList attributes = page->elementAttribute();
    if (!attributes.empty()) {
        const DomPropertyList::const_iterator acend = attributes.constEnd();
        for (DomPropertyList::const_iterator it = attributes.constBegin(); it != acend; ++it)
            if ((*it)->attributeName() == QLatin1String("pageId")) {
                if (const DomString *ds = (*it)->elementString())
                    id = ds->text();
                break;
            }
    }
    if (id.isEmpty()) {
        m_output << m_indent << parentWidget << "->addPage(" << pageVarName << ");\n";
    } else {
        m_output << m_indent << parentWidget << "->setPage(" << id << ", " << pageVarName << ");\n";
    }
}

void WriteInitialization::acceptWidget(DomWidget *node)
{
    m_layoutMarginType = m_widgetChain.count() == 1 ? TopLevelMargin : ChildMargin;
    const QString className = node->attributeClass();
    const QString varName = m_driver->findOrInsertWidget(node);
    m_registeredWidgets.insert(varName, node); // register the current widget

    QString parentWidget, parentClass;
    if (m_widgetChain.top()) {
        parentWidget = m_driver->findOrInsertWidget(m_widgetChain.top());
        parentClass = m_widgetChain.top()->attributeClass();
    }

    const QString savedParentWidget = parentWidget;

    if (m_uic->isContainer(parentClass) || m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("Q3ToolBar")))
        parentWidget.clear();

    if (m_widgetChain.size() != 1)
        m_output << m_indent << varName << " = new " << m_uic->customWidgetsInfo()->realClassName(className) << '(' << parentWidget << ");\n";

    parentWidget = savedParentWidget;

    if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3ComboBox"))) {
        initializeComboBox3(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QComboBox"))) {
        initializeComboBox(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QListWidget"))) {
        initializeListWidget(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTreeWidget"))) {
        initializeTreeWidget(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTableWidget"))) {
        initializeTableWidget(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3ListBox"))) {
        initializeQ3ListBox(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3ListView"))) {
        initializeQ3ListView(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3IconView"))) {
        initializeQ3IconView(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3Table"))) {
        initializeQ3Table(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3DataTable"))) {
        initializeQ3SqlDataTable(node);
    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3DataBrowser"))) {
        initializeQ3SqlDataBrowser(node);
    }

    if (m_uic->isButton(className))
        addButtonGroup(node, varName);

    writeProperties(varName, className, node->elementProperty());

    if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QMenu")) && parentWidget.size()) {
        initializeMenu(node, parentWidget);
    }

    if (node->elementLayout().isEmpty())
        m_layoutChain.push(0);

    m_layoutWidget = false;
    if (className == QLatin1String("QWidget") && !node->hasAttributeNative()) {
        if (const DomWidget* parentWidget = m_widgetChain.top()) {
            const QString parentClass = parentWidget->attributeClass();
            if (parentClass != QLatin1String("QMainWindow")
                && !m_uic->isCustomWidgetContainer(parentClass)
                && !m_uic->isContainer(parentClass))
            m_layoutWidget = true;
        }
    }
    m_widgetChain.push(node);
    m_layoutChain.push(0);
    TreeWalker::acceptWidget(node);
    m_layoutChain.pop();
    m_widgetChain.pop();
    m_layoutWidget = false;

    const DomPropertyMap attributes = propertyMap(node->elementAttribute());

    const QString pageDefaultString = QLatin1String("Page");

    int id = -1;
    if (const DomProperty *pid = attributes.value(QLatin1String("id"))) {
        id = pid->elementNumber();
    }

    if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QMainWindow"))
            || m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("Q3MainWindow"))) {

        if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QMenuBar"))) {
            if (!m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("Q3MainWindow")))
                m_output << m_indent << parentWidget << "->setMenuBar(" << varName <<");\n";
        } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QToolBar"))) {
            m_output << m_indent << parentWidget << "->addToolBar("
                     << toolBarAreaStringFromDOMAttributes(attributes) << varName << ");\n";

            if (const DomProperty *pbreak = attributes.value(QLatin1String("toolBarBreak"))) {
                if (pbreak->elementBool() == QLatin1String("true")) {
                    m_output << m_indent << parentWidget << "->insertToolBarBreak(" <<  varName << ");\n";
                }
            }

        } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QDockWidget"))) {
            QString area;
            if (DomProperty *pstyle = attributes.value(QLatin1String("dockWidgetArea"))) {
                area += QLatin1String("static_cast<Qt::DockWidgetArea>(");
                area += QString::number(pstyle->elementNumber());
                area += QLatin1String("), ");
            }

            m_output << m_indent << parentWidget << "->addDockWidget(" << area << varName << ");\n";
        } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QStatusBar"))) {
            m_output << m_indent << parentWidget << "->setStatusBar(" << varName << ");\n";
        } else if (!m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3DockWindow"))
                   && !m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3ToolBar"))) {
                m_output << m_indent << parentWidget << "->setCentralWidget(" << varName << ");\n";
        }
    }

    // Check for addPageMethod of a custom plugin first
    const QString addPageMethod = m_uic->customWidgetsInfo()->customWidgetAddPageMethod(parentClass);
    if (!addPageMethod.isEmpty()) {
        m_output << m_indent << parentWidget << "->" << addPageMethod << '(' << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QStackedWidget"))) {
        m_output << m_indent << parentWidget << "->addWidget(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QToolBar"))) {
        m_output << m_indent << parentWidget << "->addWidget(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("Q3WidgetStack"))) {
        m_output << m_indent << parentWidget << "->addWidget(" << varName << ", " << id << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QDockWidget"))) {
        m_output << m_indent << parentWidget << "->setWidget(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QScrollArea"))) {
        m_output << m_indent << parentWidget << "->setWidget(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QSplitter"))) {
        m_output << m_indent << parentWidget << "->addWidget(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QMdiArea"))) {
        m_output << m_indent << parentWidget << "->addSubWindow(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QWorkspace"))) {
        m_output << m_indent << parentWidget << "->addWindow(" << varName << ");\n";
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QWizard"))) {
        addWizardPage(varName, node, parentWidget);
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QToolBox"))) {
        QString icon;
        if (const DomProperty *picon = attributes.value(QLatin1String("icon"))) {
            icon += QLatin1String(", ") ;
            icon += iconCall(picon);
        }

        const DomProperty *plabel = attributes.value(QLatin1String("label"));
        DomString *plabelString = plabel ? plabel->elementString() : 0;

        m_output << m_indent << parentWidget << "->addItem(" << varName << icon << ", " << noTrCall(plabelString, pageDefaultString) << ");\n";

        autoTrOutput(plabelString, pageDefaultString) << m_indent << parentWidget << "->setItemText("
                   << parentWidget << "->indexOf(" << varName << "), " << autoTrCall(plabelString, pageDefaultString) << ");\n";

#ifndef QT_NO_TOOLTIP
        if (DomProperty *ptoolTip = attributes.value(QLatin1String("toolTip"))) {
            autoTrOutput(ptoolTip->elementString()) << m_indent << parentWidget << "->setItemToolTip("
                       << parentWidget << "->indexOf(" << varName << "), " << autoTrCall(ptoolTip->elementString()) << ");\n";
        }
#endif // QT_NO_TOOLTIP
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("QTabWidget"))) {
        QString icon;
        if (const DomProperty *picon = attributes.value(QLatin1String("icon"))) {
            icon += QLatin1String(", ");
            icon += iconCall(picon);
        }

        const DomProperty *ptitle = attributes.value(QLatin1String("title"));
        DomString *ptitleString = ptitle ? ptitle->elementString() : 0;

        m_output << m_indent << parentWidget << "->addTab(" << varName << icon << ", " << "QString());\n";

        autoTrOutput(ptitleString, pageDefaultString) << m_indent << parentWidget << "->setTabText("
                   << parentWidget << "->indexOf(" << varName << "), " << autoTrCall(ptitleString, pageDefaultString) << ");\n";

#ifndef QT_NO_TOOLTIP
        if (const DomProperty *ptoolTip = attributes.value(QLatin1String("toolTip"))) {
            autoTrOutput(ptoolTip->elementString()) << m_indent << parentWidget << "->setTabToolTip("
                       << parentWidget << "->indexOf(" << varName << "), " << autoTrCall(ptoolTip->elementString()) << ");\n";
        }
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        if (const DomProperty *pwhatsThis = attributes.value(QLatin1String("whatsThis"))) {
            autoTrOutput(pwhatsThis->elementString()) << m_indent << parentWidget << "->setTabWhatsThis("
                       << parentWidget << "->indexOf(" << varName << "), " << autoTrCall(pwhatsThis->elementString()) << ");\n";
        }
#endif // QT_NO_WHATSTHIS
    } else if (m_uic->customWidgetsInfo()->extends(parentClass, QLatin1String("Q3Wizard"))) {
        const DomProperty *ptitle = attributes.value(QLatin1String("title"));
        DomString *ptitleString = ptitle ? ptitle->elementString() : 0;

        m_output << m_indent << parentWidget << "->addPage(" << varName << ", " << noTrCall(ptitleString, pageDefaultString) << ");\n";

        autoTrOutput(ptitleString, pageDefaultString) << m_indent << parentWidget << "->setTitle("
                   << varName << ", " << autoTrCall(ptitleString, pageDefaultString) << ");\n";

    }

    //
    // Special handling for qtableview/qtreeview fake header attributes
    //
    static QStringList realPropertyNames =
            (QStringList() << QLatin1String("visible")
                           << QLatin1String("cascadingSectionResizes")
                           << QLatin1String("defaultSectionSize")
                           << QLatin1String("highlightSections")
                           << QLatin1String("minimumSectionSize")
                           << QLatin1String("showSortIndicator")
                           << QLatin1String("stretchLastSection"));

    if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTreeView"))
               || m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTreeWidget"))) {
        DomPropertyList headerProperties;
        foreach (const QString &realPropertyName, realPropertyNames) {
            const QString upperPropertyName = realPropertyName.at(0).toUpper()
                                              + realPropertyName.mid(1);
            const QString fakePropertyName = QLatin1String("header") + upperPropertyName;
            if (DomProperty *fakeProperty = attributes.value(fakePropertyName)) {
                fakeProperty->setAttributeName(realPropertyName);
                headerProperties << fakeProperty;
            }
        }
        writeProperties(varName + QLatin1String("->header()"), QLatin1String("QHeaderView"),
                        headerProperties, WritePropertyIgnoreObjectName);

    } else if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTableView"))
               || m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTableWidget"))) {

        static QStringList headerPrefixes =
                (QStringList() << QLatin1String("horizontalHeader")
                               << QLatin1String("verticalHeader"));

        foreach (const QString &headerPrefix, headerPrefixes) {
            DomPropertyList headerProperties;
            foreach (const QString &realPropertyName, realPropertyNames) {
                const QString upperPropertyName = realPropertyName.at(0).toUpper()
                                                  + realPropertyName.mid(1);
                const QString fakePropertyName = headerPrefix + upperPropertyName;
                if (DomProperty *fakeProperty = attributes.value(fakePropertyName)) {
                    fakeProperty->setAttributeName(realPropertyName);
                    headerProperties << fakeProperty;
                }
            }
            writeProperties(varName + QLatin1String("->") + headerPrefix + QLatin1String("()"),
                            QLatin1String("QHeaderView"),
                            headerProperties, WritePropertyIgnoreObjectName);
        }
    }

    if (node->elementLayout().isEmpty())
        m_layoutChain.pop();

    const QStringList zOrder = node->elementZOrder();
    for (int i = 0; i < zOrder.size(); ++i) {
        const QString name = zOrder.at(i);

        if (!m_registeredWidgets.contains(name)) {
            fprintf(stderr, "%s: Warning: Z-order assignment: '%s' is not a valid widget.\n",
                    qPrintable(m_option.messagePrefix()),
                    name.toLatin1().data());
            continue;
        }

        if (name.isEmpty()) {
            continue;
        }

        m_output << m_indent << name << "->raise();\n";
    }
}

void WriteInitialization::addButtonGroup(const DomWidget *buttonNode, const QString &varName)
{
    const DomPropertyMap attributes = propertyMap(buttonNode->elementAttribute());
    // Look up the button group name as specified in the attribute and find the uniquified name
    const DomProperty *prop = attributes.value(QLatin1String("buttonGroup"));
    if (!prop)
        return;
    const QString attributeName = toString(prop->elementString());
    const DomButtonGroup *group = m_driver->findButtonGroup(attributeName);
    // Legacy feature: Create missing groups on the fly as the UIC button group feature
    // was present before the actual Designer support (4.5)
    const bool createGroupOnTheFly = group == 0;
    if (createGroupOnTheFly) {
        DomButtonGroup *newGroup = new DomButtonGroup;
        newGroup->setAttributeName(attributeName);
        group = newGroup;
        fprintf(stderr, "%s: Warning: Creating button group `%s'\n",
                qPrintable(m_option.messagePrefix()),
                attributeName.toLatin1().data());
    }
    const QString groupName = m_driver->findOrInsertButtonGroup(group);
    // Create on demand
    if (!m_buttonGroups.contains(groupName)) {
        const QString className = QLatin1String("QButtonGroup");
        m_output << m_indent;
        if (createGroupOnTheFly)
            m_output << className << " *";
        m_output << groupName << " = new " << className << '(' << m_mainFormVarName << ");\n";
        m_buttonGroups.insert(groupName);
        writeProperties(groupName, className, group->elementProperty());
    }
    m_output << m_indent << groupName << "->addButton(" << varName << ");\n";
}

void WriteInitialization::acceptLayout(DomLayout *node)
{
    const QString className = node->attributeClass();
    const QString varName = m_driver->findOrInsertLayout(node);

    const DomPropertyMap properties = propertyMap(node->elementProperty());
    const bool oldLayoutProperties = properties.constFind(QLatin1String("margin")) != properties.constEnd();

    bool isGroupBox = false;

    if (m_widgetChain.top()) {
        const QString parentWidget = m_widgetChain.top()->attributeClass();

        if (!m_layoutChain.top() && (m_uic->customWidgetsInfo()->extends(parentWidget, QLatin1String("Q3GroupBox"))
                        || m_uic->customWidgetsInfo()->extends(parentWidget, QLatin1String("Q3ButtonGroup")))) {
            const QString parent = m_driver->findOrInsertWidget(m_widgetChain.top());

            isGroupBox = true;
            // special case for group box

            m_output << m_indent << parent << "->setColumnLayout(0, Qt::Vertical);\n";
            QString objectName = parent;
            objectName += QLatin1String("->layout()");
            int marginType = Use43UiFile;
            if (oldLayoutProperties)
                marginType = m_layoutMarginType;

            m_LayoutDefaultHandler.writeProperties(m_indent,
                                    objectName, properties, marginType, false, m_output);
        }
    }

    m_output << m_indent << varName << " = new " << className << '(';

    if (!m_layoutChain.top() && !isGroupBox)
        m_output << m_driver->findOrInsertWidget(m_widgetChain.top());

    m_output << ");\n";

    if (isGroupBox) {
        const QString tempName = m_driver->unique(QLatin1String("boxlayout"));
        m_output << m_indent << "QBoxLayout *" << tempName << " = qobject_cast<QBoxLayout *>(" <<
                    m_driver->findOrInsertWidget(m_widgetChain.top()) << "->layout());\n";
        m_output << m_indent << "if (" << tempName << ")\n";
        m_output << m_dindent << tempName << "->addLayout(" << varName << ");\n";
    }

    if (isGroupBox) {
        m_output << m_indent << varName << "->setAlignment(Qt::AlignTop);\n";
    }  else {
        // Suppress margin on a read child layout
        const bool suppressMarginDefault = m_layoutChain.top();
        int marginType = Use43UiFile;
        if (oldLayoutProperties)
            marginType = m_layoutMarginType;
        m_LayoutDefaultHandler.writeProperties(m_indent, varName, properties, marginType, suppressMarginDefault, m_output);
    }

    m_layoutMarginType = SubLayoutMargin;

    DomPropertyList propList = node->elementProperty();
    if (m_layoutWidget) {
        bool left, top, right, bottom;
        left = top = right = bottom = false;
        for (int i = 0; i < propList.size(); ++i) {
            const DomProperty *p = propList.at(i);
            const QString propertyName = p->attributeName();
            if (propertyName == QLatin1String("leftMargin") && p->kind() == DomProperty::Number)
                left = true;
            else if (propertyName == QLatin1String("topMargin") && p->kind() == DomProperty::Number)
                top = true;
            else if (propertyName == QLatin1String("rightMargin") && p->kind() == DomProperty::Number)
                right = true;
            else if (propertyName == QLatin1String("bottomMargin") && p->kind() == DomProperty::Number)
                bottom = true;
        }
        if (!left) {
            DomProperty *p = new DomProperty();
            p->setAttributeName(QLatin1String("leftMargin"));
            p->setElementNumber(0);
            propList.append(p);
        }
        if (!top) {
            DomProperty *p = new DomProperty();
            p->setAttributeName(QLatin1String("topMargin"));
            p->setElementNumber(0);
            propList.append(p);
        }
        if (!right) {
            DomProperty *p = new DomProperty();
            p->setAttributeName(QLatin1String("rightMargin"));
            p->setElementNumber(0);
            propList.append(p);
        }
        if (!bottom) {
            DomProperty *p = new DomProperty();
            p->setAttributeName(QLatin1String("bottomMargin"));
            p->setElementNumber(0);
            propList.append(p);
        }
        m_layoutWidget = false;
    }

    writeProperties(varName, className, propList, WritePropertyIgnoreMargin|WritePropertyIgnoreSpacing);

    m_layoutChain.push(node);
    TreeWalker::acceptLayout(node);
    m_layoutChain.pop();

    // Stretch? (Unless we are compiling for UIC3)
    const QString numberNull = QString(QLatin1Char('0'));
    writePropertyList(varName, QLatin1String("setStretch"), node->attributeStretch(), numberNull);
    writePropertyList(varName, QLatin1String("setRowStretch"), node->attributeRowStretch(), numberNull);
    writePropertyList(varName, QLatin1String("setColumnStretch"), node->attributeColumnStretch(), numberNull);
    writePropertyList(varName, QLatin1String("setColumnMinimumWidth"), node->attributeColumnMinimumWidth(), numberNull);
    writePropertyList(varName, QLatin1String("setRowMinimumHeight"), node->attributeRowMinimumHeight(), numberNull);
}

// Apply a comma-separated list of values using a function "setSomething(int idx, value)"
void WriteInitialization::writePropertyList(const QString &varName,
                                            const QString &setFunction,
                                            const QString &value,
                                            const QString &defaultValue)
{
    if (value.isEmpty())
        return;
    const QStringList list = value.split(QLatin1Char(','));
    const int count =  list.count();
    for (int i = 0; i < count; i++)
        if (list.at(i) != defaultValue)
            m_output << m_indent << varName << "->" << setFunction << '(' << i << ", " << list.at(i) << ");\n";
}

void WriteInitialization::acceptSpacer(DomSpacer *node)
{
    m_output << m_indent << m_driver->findOrInsertSpacer(node) << " = ";
    writeSpacerItem(node, m_output);
    m_output << ";\n";
}

static inline QString formLayoutRole(int column, int colspan)
{
    if (colspan > 1)
        return QLatin1String("QFormLayout::SpanningRole");
    return column == 0 ? QLatin1String("QFormLayout::LabelRole") : QLatin1String("QFormLayout::FieldRole");
}

void WriteInitialization::acceptLayoutItem(DomLayoutItem *node)
{
    TreeWalker::acceptLayoutItem(node);

    DomLayout *layout = m_layoutChain.top();

    if (!layout)
        return;

    const QString layoutName = m_driver->findOrInsertLayout(layout);
    const QString itemName = m_driver->findOrInsertLayoutItem(node);

    QString addArgs;
    QString methodPrefix = QLatin1String("add"); //Consistent API-design galore!
    if (layout->attributeClass() == QLatin1String("QGridLayout")) {
        const int row = node->attributeRow();
        const int col = node->attributeColumn();

        const int rowSpan = node->hasAttributeRowSpan() ? node->attributeRowSpan() : 1;
        const int colSpan = node->hasAttributeColSpan() ? node->attributeColSpan() : 1;

        addArgs = QString::fromLatin1("%1, %2, %3, %4, %5").arg(itemName).arg(row).arg(col).arg(rowSpan).arg(colSpan);
        if (!node->attributeAlignment().isEmpty())
            addArgs += QLatin1String(", ") + node->attributeAlignment();
    } else {
        if (layout->attributeClass() == QLatin1String("QFormLayout")) {
            methodPrefix = QLatin1String("set");
            const int row = node->attributeRow();
            const int colSpan = node->hasAttributeColSpan() ? node->attributeColSpan() : 1;
            const QString role = formLayoutRole(node->attributeColumn(), colSpan);
            addArgs = QString::fromLatin1("%1, %2, %3").arg(row).arg(role).arg(itemName);
        } else {
            addArgs = itemName;
            if (layout->attributeClass().contains(QLatin1String("Box")) && !node->attributeAlignment().isEmpty())
                addArgs += QLatin1String(", 0, ") + node->attributeAlignment();
        }
    }

    // figure out "add" method
    m_output << "\n" << m_indent << layoutName << "->";
    switch (node->kind()) {
    case DomLayoutItem::Widget:
        m_output << methodPrefix << "Widget(" <<  addArgs;
        break;
    case DomLayoutItem::Layout:
        m_output <<  methodPrefix << "Layout(" << addArgs;
        break;
    case DomLayoutItem::Spacer:
        m_output << methodPrefix << "Item(" << addArgs;
        break;
    case DomLayoutItem::Unknown:
        Q_ASSERT( 0 );
        break;
    }
    m_output << ");\n\n";
}

void WriteInitialization::acceptActionGroup(DomActionGroup *node)
{
    const QString actionName = m_driver->findOrInsertActionGroup(node);
    QString varName = m_driver->findOrInsertWidget(m_widgetChain.top());

    if (m_actionGroupChain.top())
        varName = m_driver->findOrInsertActionGroup(m_actionGroupChain.top());

    m_output << m_indent << actionName << " = new QActionGroup(" << varName << ");\n";
    writeProperties(actionName, QLatin1String("QActionGroup"), node->elementProperty());

    m_actionGroupChain.push(node);
    TreeWalker::acceptActionGroup(node);
    m_actionGroupChain.pop();
}

void WriteInitialization::acceptAction(DomAction *node)
{
    if (node->hasAttributeMenu())
        return;

    const QString actionName = m_driver->findOrInsertAction(node);
    m_registeredActions.insert(actionName, node);
    QString varName = m_driver->findOrInsertWidget(m_widgetChain.top());

    if (m_actionGroupChain.top())
        varName = m_driver->findOrInsertActionGroup(m_actionGroupChain.top());

    m_output << m_indent << actionName << " = new QAction(" << varName << ");\n";
    writeProperties(actionName, QLatin1String("QAction"), node->elementProperty());
}

void WriteInitialization::acceptActionRef(DomActionRef *node)
{
    QString actionName = node->attributeName();
    const bool isSeparator = actionName == QLatin1String("separator");
    bool isMenu = false;

    QString varName = m_driver->findOrInsertWidget(m_widgetChain.top());

    if (actionName.isEmpty() || !m_widgetChain.top()) {
        return;
    } else if (m_driver->actionGroupByName(actionName)) {
        return;
    } else if (DomWidget *w = m_driver->widgetByName(actionName)) {
        isMenu = m_uic->isMenu(w->attributeClass());
        bool inQ3ToolBar = m_uic->customWidgetsInfo()->extends(m_widgetChain.top()->attributeClass(), QLatin1String("Q3ToolBar"));
        if (!isMenu && inQ3ToolBar) {
            m_actionOut << m_indent << actionName << "->setParent(" << varName << ");\n";
            return;
        }
    } else if (!(m_driver->actionByName(actionName) || isSeparator)) {
        fprintf(stderr, "%s: Warning: action `%s' not declared\n",
                qPrintable(m_option.messagePrefix()),
                actionName.toLatin1().data());
        return;
    }

    if (m_widgetChain.top() && isSeparator) {
        // separator is always reserved!
        m_actionOut << m_indent << varName << "->addSeparator();\n";
        return;
    }

    if (isMenu)
        actionName += QLatin1String("->menuAction()");

    m_actionOut << m_indent << varName << "->addAction(" << actionName << ");\n";
}

void WriteInitialization::writeProperties(const QString &varName,
                                          const QString &className,
                                          const DomPropertyList &lst,
                                          unsigned flags)
{
    const bool isTopLevel = m_widgetChain.count() == 1;

    if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QAxWidget"))) {
        DomPropertyMap properties = propertyMap(lst);
        if (properties.contains(QLatin1String("control"))) {
            DomProperty *p = properties.value(QLatin1String("control"));
            m_output << m_indent << varName << "->setControl(QString::fromUtf8("
                   << fixString(toString(p->elementString()), m_dindent) << "));\n";
        }
    }

    DomWidget *buttonGroupWidget = findWidget(QLatin1String("Q3ButtonGroup"));

    QString indent;
    if (!m_widgetChain.top()) {
        indent = m_option.indent;
        m_output << m_indent << "if (" << varName << "->objectName().isEmpty())\n";
    }
    if (!(flags & WritePropertyIgnoreObjectName))
        m_output << m_indent << indent << varName
                << "->setObjectName(QString::fromUtf8(" << fixString(varName, m_dindent) << "));\n";

    int leftMargin, topMargin, rightMargin, bottomMargin;
    leftMargin = topMargin = rightMargin = bottomMargin = -1;
    bool frameShadowEncountered = false;

    for (int i=0; i<lst.size(); ++i) {
        const DomProperty *p = lst.at(i);
        if (!checkProperty(m_option.inputFile, p))
            continue;
        const QString propertyName = p->attributeName();
        QString propertyValue;

        // special case for the property `geometry': Do not use position
        if (isTopLevel && propertyName == QLatin1String("geometry") && p->elementRect()) {
            const DomRect *r = p->elementRect();
            m_output << m_indent << varName << "->resize(" << r->elementWidth() << ", " << r->elementHeight() << ");\n";
            continue;
        } else if (propertyName == QLatin1String("buttonGroupId")) { // Q3ButtonGroup support
            if (buttonGroupWidget)
                m_output << m_indent << m_driver->findOrInsertWidget(buttonGroupWidget) << "->insert("
                         << varName << ", " << p->elementNumber() << ");\n";
            continue;
        } else if (propertyName == QLatin1String("currentRow") // QListWidget::currentRow
                    && m_uic->customWidgetsInfo()->extends(className, QLatin1String("QListWidget"))) {
            m_delayedOut << m_indent << varName << "->setCurrentRow("
                       << p->elementNumber() << ");\n";
            continue;
        } else if (propertyName == QLatin1String("currentIndex") // set currentIndex later
                    && (m_uic->customWidgetsInfo()->extends(className, QLatin1String("QComboBox"))
                    || m_uic->customWidgetsInfo()->extends(className, QLatin1String("QStackedWidget"))
                    || m_uic->customWidgetsInfo()->extends(className, QLatin1String("QTabWidget"))
                    || m_uic->customWidgetsInfo()->extends(className, QLatin1String("QToolBox")))) {
            m_delayedOut << m_indent << varName << "->setCurrentIndex("
                       << p->elementNumber() << ");\n";
            continue;
        } else if (propertyName == QLatin1String("tabSpacing")
                    && m_uic->customWidgetsInfo()->extends(className, QLatin1String("QToolBox"))) {
            m_delayedOut << m_indent << varName << "->layout()->setSpacing("
                       << p->elementNumber() << ");\n";
            continue;
        } else if (propertyName == QLatin1String("control") // ActiveQt support
                    && m_uic->customWidgetsInfo()->extends(className, QLatin1String("QAxWidget"))) {
            // already done ;)
            continue;
        } else if (propertyName == QLatin1String("database")
                    && p->elementStringList()) {
            // Sql support
            continue;
        } else if (propertyName == QLatin1String("frameworkCode")
                    && p->kind() == DomProperty::Bool) {
            // Sql support
            continue;
        } else if (propertyName == QLatin1String("orientation")
                    && m_uic->customWidgetsInfo()->extends(className, QLatin1String("Line"))) {
            // Line support
            QString shape = QLatin1String("QFrame::HLine");
            if (p->elementEnum() == QLatin1String("Qt::Vertical"))
                shape = QLatin1String("QFrame::VLine");

            m_output << m_indent << varName << "->setFrameShape(" << shape << ");\n";
            // QFrame Default is 'Plain'. Make the line 'Sunken' unless otherwise specified
            if (!frameShadowEncountered)
                m_output << m_indent << varName << "->setFrameShadow(QFrame::Sunken);\n";
            continue;
        } else if ((flags & WritePropertyIgnoreMargin)  && propertyName == QLatin1String("margin")) {
            continue;
        } else if ((flags & WritePropertyIgnoreSpacing) && propertyName == QLatin1String("spacing")) {
            continue;
        } else if (propertyName == QLatin1String("leftMargin") && p->kind() == DomProperty::Number) {
            leftMargin = p->elementNumber();
            continue;
        } else if (propertyName == QLatin1String("topMargin") && p->kind() == DomProperty::Number) {
            topMargin = p->elementNumber();
            continue;
        } else if (propertyName == QLatin1String("rightMargin") && p->kind() == DomProperty::Number) {
            rightMargin = p->elementNumber();
            continue;
        } else if (propertyName == QLatin1String("bottomMargin") && p->kind() == DomProperty::Number) {
            bottomMargin = p->elementNumber();
            continue;
        } else if (propertyName == QLatin1String("frameShadow"))
            frameShadowEncountered = true;

        bool stdset = m_stdsetdef;
        if (p->hasAttributeStdset())
            stdset = p->attributeStdset();

        QString setFunction;

        if (stdset) {
            setFunction = QLatin1String("->set");
            setFunction += propertyName.left(1).toUpper();
            setFunction += propertyName.mid(1);
            setFunction += QLatin1Char('(');
        } else {
            setFunction = QLatin1String("->setProperty(\"");
            setFunction += propertyName;
            setFunction += QLatin1String("\", QVariant(");
        }

        QString varNewName = varName;

        switch (p->kind()) {
        case DomProperty::Bool: {
            propertyValue = p->elementBool();
            break;
        }
        case DomProperty::Color:
            propertyValue = domColor2QString(p->elementColor());
            break;
        case DomProperty::Cstring:
            if (propertyName == QLatin1String("buddy") && m_uic->customWidgetsInfo()->extends(className, QLatin1String("QLabel"))) {
                m_buddies.append(Buddy(varName, p->elementCstring()));
            } else {
                if (stdset)
                    propertyValue = fixString(p->elementCstring(), m_dindent);
                else {
                    propertyValue = QLatin1String("QByteArray(");
                    propertyValue += fixString(p->elementCstring(), m_dindent);
                    propertyValue += QLatin1Char(')');
                }
            }
            break;
        case DomProperty::Cursor:
            propertyValue = QString::fromLatin1("QCursor(static_cast<Qt::CursorShape>(%1))")
                            .arg(p->elementCursor());
            break;
        case DomProperty::CursorShape:
            if (p->hasAttributeStdset() && !p->attributeStdset())
                varNewName += QLatin1String("->viewport()");
            propertyValue = QString::fromLatin1("QCursor(Qt::%1)")
                            .arg(p->elementCursorShape());
            break;
        case DomProperty::Enum:
            propertyValue = p->elementEnum();
            if (!propertyValue.contains(QLatin1String("::"))) {
                QString scope  = className;
                scope += QLatin1String("::");
                propertyValue.prepend(scope);
            }
            break;
        case DomProperty::Set:
            propertyValue = p->elementSet();
            break;
        case DomProperty::Font:
            propertyValue = writeFontProperties(p->elementFont());
            break;
        case DomProperty::IconSet:
            propertyValue = writeIconProperties(p->elementIconSet());
            break;
        case DomProperty::Pixmap:
            propertyValue = pixCall(p);
            break;
        case DomProperty::Palette: {
            const DomPalette *pal = p->elementPalette();
            const QString paletteName = m_driver->unique(QLatin1String("palette"));
            m_output << m_indent << "QPalette " << paletteName << ";\n";

            writeColorGroup(pal->elementActive(), QLatin1String("QPalette::Active"), paletteName);
            writeColorGroup(pal->elementInactive(), QLatin1String("QPalette::Inactive"), paletteName);
            writeColorGroup(pal->elementDisabled(), QLatin1String("QPalette::Disabled"), paletteName);

            propertyValue = paletteName;
            break;
        }
        case DomProperty::Point: {
            const DomPoint *po = p->elementPoint();
            propertyValue = QString::fromLatin1("QPoint(%1, %2)")
                            .arg(po->elementX()).arg(po->elementY());
            break;
        }
        case DomProperty::PointF: {
            const DomPointF *pof = p->elementPointF();
            propertyValue = QString::fromLatin1("QPointF(%1, %2)")
                            .arg(pof->elementX()).arg(pof->elementY());
            break;
        }
        case DomProperty::Rect: {
            const DomRect *r = p->elementRect();
            propertyValue = QString::fromLatin1("QRect(%1, %2, %3, %4)")
                            .arg(r->elementX()).arg(r->elementY())
                            .arg(r->elementWidth()).arg(r->elementHeight());
            break;
        }
        case DomProperty::RectF: {
            const DomRectF *rf = p->elementRectF();
            propertyValue = QString::fromLatin1("QRectF(%1, %2, %3, %4)")
                            .arg(rf->elementX()).arg(rf->elementY())
                            .arg(rf->elementWidth()).arg(rf->elementHeight());
            break;
        }
        case DomProperty::Locale: {
             const DomLocale *locale = p->elementLocale();
             propertyValue = QString::fromLatin1("QLocale(QLocale::%1, QLocale::%2)")
                             .arg(locale->attributeLanguage()).arg(locale->attributeCountry());
            break;
        }
        case DomProperty::SizePolicy: {
            const QString spName = writeSizePolicy( p->elementSizePolicy());
            m_output << m_indent << spName << QString::fromLatin1(
                ".setHeightForWidth(%1->sizePolicy().hasHeightForWidth());\n")
                .arg(varName);

            propertyValue = spName;
            break;
        }
        case DomProperty::Size: {
             const DomSize *s = p->elementSize();
              propertyValue = QString::fromLatin1("QSize(%1, %2)")
                             .arg(s->elementWidth()).arg(s->elementHeight());
            break;
        }
        case DomProperty::SizeF: {
            const DomSizeF *sf = p->elementSizeF();
             propertyValue = QString::fromLatin1("QSizeF(%1, %2)")
                            .arg(sf->elementWidth()).arg(sf->elementHeight());
            break;
        }
        case DomProperty::String: {
            if (propertyName == QLatin1String("objectName")) {
                const QString v = p->elementString()->text();
                if (v == varName)
                    break;

                // ### qWarning("Deprecated: the property `objectName' is different from the variable name");
            }

            propertyValue = autoTrCall(p->elementString());
            break;
        }
        case DomProperty::Number:
            propertyValue = QString::number(p->elementNumber());
            break;
        case DomProperty::UInt:
            propertyValue = QString::number(p->elementUInt());
            propertyValue += QLatin1Char('u');
            break;
        case DomProperty::LongLong:
            propertyValue = QLatin1String("Q_INT64_C(");
            propertyValue += QString::number(p->elementLongLong());
            propertyValue += QLatin1Char(')');;
            break;
        case DomProperty::ULongLong:
            propertyValue = QLatin1String("Q_UINT64_C(");
            propertyValue += QString::number(p->elementULongLong());
            propertyValue += QLatin1Char(')');
            break;
        case DomProperty::Float:
            propertyValue = QString::number(p->elementFloat());
            break;
        case DomProperty::Double:
            propertyValue = QString::number(p->elementDouble());
            break;
        case DomProperty::Char: {
            const DomChar *c = p->elementChar();
            propertyValue = QString::fromLatin1("QChar(%1)")
                            .arg(c->elementUnicode());
            break;
        }
        case DomProperty::Date: {
            const DomDate *d = p->elementDate();
            propertyValue = QString::fromLatin1("QDate(%1, %2, %3)")
                            .arg(d->elementYear())
                            .arg(d->elementMonth())
                            .arg(d->elementDay());
            break;
        }
        case DomProperty::Time: {
            const DomTime *t = p->elementTime();
            propertyValue = QString::fromLatin1("QTime(%1, %2, %3)")
                            .arg(t->elementHour())
                            .arg(t->elementMinute())
                            .arg(t->elementSecond());
            break;
        }
        case DomProperty::DateTime: {
            const DomDateTime *dt = p->elementDateTime();
            propertyValue = QString::fromLatin1("QDateTime(QDate(%1, %2, %3), QTime(%4, %5, %6))")
                            .arg(dt->elementYear())
                            .arg(dt->elementMonth())
                            .arg(dt->elementDay())
                            .arg(dt->elementHour())
                            .arg(dt->elementMinute())
                            .arg(dt->elementSecond());
            break;
        }
        case DomProperty::StringList:
            propertyValue = QLatin1String("QStringList()");
            if (p->elementStringList()->elementString().size()) {
                const QStringList lst = p->elementStringList()->elementString();
                for (int i=0; i<lst.size(); ++i) {
                    propertyValue += QLatin1String(" << QString::fromUtf8(");
                    propertyValue += fixString(lst.at(i), m_dindent);
                    propertyValue += QLatin1Char(')');
                }
            }
            break;

        case DomProperty::Url: {
            const DomUrl* u = p->elementUrl();
            propertyValue = QString::fromLatin1("QUrl(QString::fromUtf8(%1))")
                            .arg(fixString(u->elementString()->text(), m_dindent));
            break;
        }
        case DomProperty::Brush:
            propertyValue = writeBrushInitialization(p->elementBrush());
            break;
        case DomProperty::Unknown:
            break;
        }

        if (propertyValue.size()) {
            const char* defineC = 0;
            if (propertyName == QLatin1String("toolTip"))
                defineC = toolTipDefineC;
            else if (propertyName == QLatin1String("whatsThis"))
                defineC = whatsThisDefineC;
            else if (propertyName == QLatin1String("statusTip"))
                defineC = statusTipDefineC;
            else if (propertyName == QLatin1String("accessibleName") || propertyName == QLatin1String("accessibleDescription"))
                defineC = accessibilityDefineC;

            QTextStream &o = autoTrOutput(p->elementString());

            if (defineC)
                openIfndef(o, QLatin1String(defineC));
            o << m_indent << varNewName << setFunction << propertyValue;
            if (!stdset)
                o << ')';
            o << ");\n";
            if (defineC)
                closeIfndef(o, QLatin1String(defineC));

            if (varName == m_mainFormVarName && &o == &m_refreshOut) {
                // this is the only place (currently) where we output mainForm name to the retranslateUi().
                // Other places output merely instances of a certain class (which cannot be main form, e.g. QListWidget).
                m_mainFormUsedInRetranslateUi = true;
            }
        }
    }
    if (leftMargin != -1 || topMargin != -1 || rightMargin != -1 || bottomMargin != -1) {
        QString objectName = varName;
        if (m_widgetChain.top()) {
            const QString parentWidget = m_widgetChain.top()->attributeClass();

            if (!m_layoutChain.top() && (m_uic->customWidgetsInfo()->extends(parentWidget, QLatin1String("Q3GroupBox"))
                        || m_uic->customWidgetsInfo()->extends(parentWidget, QLatin1String("Q3ButtonGroup")))) {
                objectName = m_driver->findOrInsertWidget(m_widgetChain.top()) + QLatin1String("->layout()");
            }
        }
        m_output << m_indent << objectName << QLatin1String("->setContentsMargins(")
                 << leftMargin << QLatin1String(", ")
                 << topMargin << QLatin1String(", ")
                 << rightMargin << QLatin1String(", ")
                 << bottomMargin << QLatin1String(");\n");
    }
}

QString  WriteInitialization::writeSizePolicy(const DomSizePolicy *sp)
{

    // check cache
    const SizePolicyHandle sizePolicyHandle(sp);
    const SizePolicyNameMap::const_iterator it = m_sizePolicyNameMap.constFind(sizePolicyHandle);
    if ( it != m_sizePolicyNameMap.constEnd()) {
        return it.value();
    }


    // insert with new name
    const QString spName = m_driver->unique(QLatin1String("sizePolicy"));
    m_sizePolicyNameMap.insert(sizePolicyHandle, spName);

    m_output << m_indent << "QSizePolicy " << spName;
    do {
        if (sp->hasElementHSizeType() && sp->hasElementVSizeType()) {
            m_output << "(static_cast<QSizePolicy::Policy>(" << sp->elementHSizeType()
                << "), static_cast<QSizePolicy::Policy>(" << sp->elementVSizeType() << "));\n";
            break;
        }
        if (sp->hasAttributeHSizeType() && sp->hasAttributeVSizeType()) {
                m_output << "(QSizePolicy::" << sp->attributeHSizeType() << ", QSizePolicy::"
                << sp->attributeVSizeType() << ");\n";
            break;
        }
        m_output << ";\n";
    } while (false);

    m_output << m_indent << spName << ".setHorizontalStretch("
        << sp->elementHorStretch() << ");\n";
    m_output << m_indent << spName << ".setVerticalStretch("
        << sp->elementVerStretch() << ");\n";
    return spName;
}
// Check for a font with the given properties in the FontPropertiesNameMap
// or create a new one. Returns the name.

QString WriteInitialization::writeFontProperties(const DomFont *f)
{
    // check cache
    const FontHandle fontHandle(f);
    const FontPropertiesNameMap::const_iterator it = m_fontPropertiesNameMap.constFind(fontHandle);
    if ( it != m_fontPropertiesNameMap.constEnd()) {
        return it.value();
    }

    // insert with new name
    const QString fontName = m_driver->unique(QLatin1String("font"));
    m_fontPropertiesNameMap.insert(FontHandle(f), fontName);

    m_output << m_indent << "QFont " << fontName << ";\n";
    if (f->hasElementFamily() && !f->elementFamily().isEmpty()) {
        m_output << m_indent << fontName << ".setFamily(QString::fromUtf8(" << fixString(f->elementFamily(), m_dindent)
            << "));\n";
    }
    if (f->hasElementPointSize() && f->elementPointSize() > 0) {
         m_output << m_indent << fontName << ".setPointSize(" << f->elementPointSize()
             << ");\n";
    }

    if (f->hasElementBold()) {
        m_output << m_indent << fontName << ".setBold("
            << (f->elementBold() ? "true" : "false") << ");\n";
    }
    if (f->hasElementItalic()) {
        m_output << m_indent << fontName << ".setItalic("
            <<  (f->elementItalic() ? "true" : "false") << ");\n";
    }
    if (f->hasElementUnderline()) {
        m_output << m_indent << fontName << ".setUnderline("
            << (f->elementUnderline() ? "true" : "false") << ");\n";
    }
    if (f->hasElementWeight() && f->elementWeight() > 0) {
        m_output << m_indent << fontName << ".setWeight("
            << f->elementWeight() << ");" << endl;
    }
    if (f->hasElementStrikeOut()) {
         m_output << m_indent << fontName << ".setStrikeOut("
            << (f->elementStrikeOut() ? "true" : "false") << ");\n";
    }
    if (f->hasElementKerning()) {
        m_output << m_indent << fontName << ".setKerning("
            << (f->elementKerning() ? "true" : "false") << ");\n";
    }
    if (f->hasElementAntialiasing()) {
        m_output << m_indent << fontName << ".setStyleStrategy("
            << (f->elementAntialiasing() ? "QFont::PreferDefault" : "QFont::NoAntialias") << ");\n";
    }
    if (f->hasElementStyleStrategy()) {
         m_output << m_indent << fontName << ".setStyleStrategy(QFont::"
            << f->elementStyleStrategy() << ");\n";
    }
    return  fontName;
}

// Post 4.4 write resource icon
static void writeResourceIcon(QTextStream &output,
                              const QString &iconName,
                              const QString &indent,
                              const DomResourceIcon *i)
{
    if (i->hasElementNormalOff())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementNormalOff()->text(), indent) << "), QSize(), QIcon::Normal, QIcon::Off);\n";
    if (i->hasElementNormalOn())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementNormalOn()->text(), indent) << "), QSize(), QIcon::Normal, QIcon::On);\n";
    if (i->hasElementDisabledOff())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementDisabledOff()->text(), indent) << "), QSize(), QIcon::Disabled, QIcon::Off);\n";
    if (i->hasElementDisabledOn())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementDisabledOn()->text(), indent) << "), QSize(), QIcon::Disabled, QIcon::On);\n";
    if (i->hasElementActiveOff())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementActiveOff()->text(), indent) << "), QSize(), QIcon::Active, QIcon::Off);\n";
    if (i->hasElementActiveOn())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementActiveOn()->text(), indent) << "), QSize(), QIcon::Active, QIcon::On);\n";
    if (i->hasElementSelectedOff())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementSelectedOff()->text(), indent) << "), QSize(), QIcon::Selected, QIcon::Off);\n";
    if (i->hasElementSelectedOn())
        output << indent << iconName << ".addFile(QString::fromUtf8(" << fixString(i->elementSelectedOn()->text(), indent) << "), QSize(), QIcon::Selected, QIcon::On);\n";
}

QString WriteInitialization::writeIconProperties(const DomResourceIcon *i)
{
    // check cache
    const IconHandle iconHandle(i);
    const IconPropertiesNameMap::const_iterator it = m_iconPropertiesNameMap.constFind(iconHandle);
    if (it != m_iconPropertiesNameMap.constEnd()) {
        return it.value();
    }

    // insert with new name
    const QString iconName = m_driver->unique(QLatin1String("icon"));
    m_iconPropertiesNameMap.insert(IconHandle(i), iconName);
    if (isIconFormat44(i)) {
        if (i->attributeTheme().isEmpty()) {
            // No theme: Write resource icon as is
            m_output << m_indent << "QIcon " << iconName << ";\n";
            writeResourceIcon(m_output, iconName, m_indent, i);
        } else {
            // Theme: Generate code to check the theme and default to resource
            const QString themeIconName = fixString(i->attributeTheme(), QString());
            if (iconHasStatePixmaps(i)) {
                // Theme + default state pixmaps:
                // Generate code to check the theme and default to state pixmaps
                m_output << m_indent << "QIcon " << iconName << ";\n";
                const char themeNameStringVariableC[] = "iconThemeName";
                // Store theme name in a variable
                m_output << m_indent;
                if (m_firstThemeIcon) { // Declare variable string
                    m_output << "QString ";
                    m_firstThemeIcon = false;
                }
                m_output << themeNameStringVariableC << " = QString::fromUtf8("
                         << themeIconName << ");\n";
                m_output << m_indent << "if (QIcon::hasThemeIcon("
                         << themeNameStringVariableC
                         << ")) {\n"
                         << m_dindent << iconName << " = QIcon::fromTheme(" << themeNameStringVariableC << ");\n"
                         << m_indent << "} else {\n";
                writeResourceIcon(m_output, iconName, m_dindent, i);
                m_output << m_indent << "}\n";
            } else {
                // Theme, but no state pixmaps: Construct from theme directly.
                m_output << m_indent << "QIcon " << iconName
                         << "(QIcon::fromTheme(QString::fromUtf8("
                         << themeIconName << ")));\n";
            } // Theme, but not state
        }     // >= 4.4
    } else {  // pre-4.4 legacy
        m_output <<  m_indent << "const QIcon " << iconName << " = " << pixCall(QLatin1String("QIcon"), i->text())<< ";\n";
    }
    return iconName;
}

QString WriteInitialization::domColor2QString(const DomColor *c)
{
    if (c->hasAttributeAlpha())
        return QString::fromLatin1("QColor(%1, %2, %3, %4)")
            .arg(c->elementRed())
            .arg(c->elementGreen())
            .arg(c->elementBlue())
            .arg(c->attributeAlpha());
    return QString::fromLatin1("QColor(%1, %2, %3)")
        .arg(c->elementRed())
        .arg(c->elementGreen())
        .arg(c->elementBlue());
}

void WriteInitialization::writeColorGroup(DomColorGroup *colorGroup, const QString &group, const QString &paletteName)
{
    if (!colorGroup)
        return;

    // old format
    const QList<DomColor*> colors = colorGroup->elementColor();
    for (int i=0; i<colors.size(); ++i) {
        const DomColor *color = colors.at(i);

        m_output << m_indent << paletteName << ".setColor(" << group
            << ", " << "static_cast<QPalette::ColorRole>(" << QString::number(i) << ')'
            << ", " << domColor2QString(color)
            << ");\n";
    }

    // new format
    const QList<DomColorRole *> colorRoles = colorGroup->elementColorRole();
    QListIterator<DomColorRole *> itRole(colorRoles);
    while (itRole.hasNext()) {
        const DomColorRole *colorRole = itRole.next();
        if (colorRole->hasAttributeRole()) {
            const QString brushName = writeBrushInitialization(colorRole->elementBrush());
            m_output << m_indent << paletteName << ".setBrush(" << group
                << ", " << "QPalette::" << colorRole->attributeRole()
                << ", " << brushName << ");\n";
        }
    }
}

// Write initialization for brush unless it is found in the cache. Returns the name to use
// in an expression.
QString WriteInitialization::writeBrushInitialization(const DomBrush *brush)
{
    // Simple solid, colored  brushes are cached
    const bool solidColoredBrush = !brush->hasAttributeBrushStyle() || brush->attributeBrushStyle() == QLatin1String("SolidPattern");
    uint rgb = 0;
    if (solidColoredBrush) {
        if (const DomColor *color = brush->elementColor()) {
            rgb = ((color->elementRed() & 0xFF) << 24) |
                  ((color->elementGreen() & 0xFF) << 16) |
                  ((color->elementBlue() & 0xFF) << 8) |
                  ((color->attributeAlpha() & 0xFF));
            const ColorBrushHash::const_iterator cit = m_colorBrushHash.constFind(rgb);
            if (cit != m_colorBrushHash.constEnd())
                return cit.value();
        }
    }
    // Create and enter into cache if simple
    const QString brushName = m_driver->unique(QLatin1String("brush"));
    writeBrush(brush, brushName);
    if (solidColoredBrush)
        m_colorBrushHash.insert(rgb, brushName);
    return brushName;
}

void WriteInitialization::writeBrush(const DomBrush *brush, const QString &brushName)
{
    QString style = QLatin1String("SolidPattern");
    if (brush->hasAttributeBrushStyle())
        style = brush->attributeBrushStyle();

    if (style == QLatin1String("LinearGradientPattern") ||
            style == QLatin1String("RadialGradientPattern") ||
            style == QLatin1String("ConicalGradientPattern")) {
        const DomGradient *gradient = brush->elementGradient();
        const QString gradientType = gradient->attributeType();
        const QString gradientName = m_driver->unique(QLatin1String("gradient"));
        if (gradientType == QLatin1String("LinearGradient")) {
            m_output << m_indent << "QLinearGradient " << gradientName
                << '(' << gradient->attributeStartX()
                << ", " << gradient->attributeStartY()
                << ", " << gradient->attributeEndX()
                << ", " << gradient->attributeEndY() << ");\n";
        } else if (gradientType == QLatin1String("RadialGradient")) {
            m_output << m_indent << "QRadialGradient " << gradientName
                << '(' << gradient->attributeCentralX()
                << ", " << gradient->attributeCentralY()
                << ", " << gradient->attributeRadius()
                << ", " << gradient->attributeFocalX()
                << ", " << gradient->attributeFocalY() << ");\n";
        } else if (gradientType == QLatin1String("ConicalGradient")) {
            m_output << m_indent << "QConicalGradient " << gradientName
                << '(' << gradient->attributeCentralX()
                << ", " << gradient->attributeCentralY()
                << ", " << gradient->attributeAngle() << ");\n";
        }

        m_output << m_indent << gradientName << ".setSpread(QGradient::"
            << gradient->attributeSpread() << ");\n";

        if (gradient->hasAttributeCoordinateMode()) {
            m_output << m_indent << gradientName << ".setCoordinateMode(QGradient::"
                << gradient->attributeCoordinateMode() << ");\n";
        }

       const  QList<DomGradientStop *> stops = gradient->elementGradientStop();
        QListIterator<DomGradientStop *> it(stops);
        while (it.hasNext()) {
            const DomGradientStop *stop = it.next();
            const DomColor *color = stop->elementColor();
            m_output << m_indent << gradientName << ".setColorAt("
                << stop->attributePosition() << ", "
                << domColor2QString(color) << ");\n";
        }
        m_output << m_indent << "QBrush " << brushName << '('
            << gradientName << ");\n";
    } else if (style == QLatin1String("TexturePattern")) {
        const DomProperty *property = brush->elementTexture();
        const QString iconValue = iconCall(property);

        m_output << m_indent << "QBrush " << brushName << " = QBrush("
            << iconValue << ");\n";
    } else {
        const DomColor *color = brush->elementColor();
        m_output << m_indent << "QBrush " << brushName << '('
            << domColor2QString(color) << ");\n";

        m_output << m_indent << brushName << ".setStyle("
            << "Qt::" << style << ");\n";
    }
}

void WriteInitialization::acceptCustomWidget(DomCustomWidget *node)
{
    Q_UNUSED(node);
}

void WriteInitialization::acceptCustomWidgets(DomCustomWidgets *node)
{
    Q_UNUSED(node);
}

void WriteInitialization::acceptTabStops(DomTabStops *tabStops)
{
    QString lastName;

    const QStringList l = tabStops->elementTabStop();
    for (int i=0; i<l.size(); ++i) {
        const QString name = l.at(i);

        if (!m_registeredWidgets.contains(name)) {
            fprintf(stderr, "%s: Warning: Tab-stop assignment: '%s' is not a valid widget.\n",
                    qPrintable(m_option.messagePrefix()),
                    name.toLatin1().data());
            continue;
        }

        if (i == 0) {
            lastName = name;
            continue;
        } else if (name.isEmpty() || lastName.isEmpty()) {
            continue;
        }

        m_output << m_indent << "QWidget::setTabOrder(" << lastName << ", " << name << ");\n";

        lastName = name;
    }
}

void WriteInitialization::initializeQ3ListBox(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString className = w->attributeClass();

    const QList<DomItem*> items = w->elementItem();

    if (items.isEmpty())
        return;

    m_refreshOut << m_indent << varName << "->clear();\n";

    for (int i=0; i<items.size(); ++i) {
        const DomItem *item = items.at(i);

        const DomPropertyMap properties = propertyMap(item->elementProperty());
        const DomProperty *text = properties.value(QLatin1String("text"));
        const DomProperty *pixmap = properties.value(QLatin1String("pixmap"));
        if (!(text || pixmap))
            continue;

        m_refreshOut << m_indent << varName << "->insertItem(";
        if (pixmap) {
            m_refreshOut << pixCall(pixmap);

            if (text)
                m_refreshOut << ", ";
        }
        if (text)
            m_refreshOut << trCall(text->elementString());
        m_refreshOut << ");\n";
    }
}

void WriteInitialization::initializeQ3IconView(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString className = w->attributeClass();

    const QList<DomItem*> items = w->elementItem();

    if (items.isEmpty())
        return;

    m_refreshOut << m_indent << varName << "->clear();\n";

    for (int i=0; i<items.size(); ++i) {
        const DomItem *item = items.at(i);

        const DomPropertyMap properties = propertyMap(item->elementProperty());
        const DomProperty *text = properties.value(QLatin1String("text"));
        const DomProperty *pixmap = properties.value(QLatin1String("pixmap"));
        if (!(text || pixmap))
            continue;

        const QString itemName = m_driver->unique(QLatin1String("__item"));
        m_refreshOut << "\n";
        m_refreshOut << m_indent << "Q3IconViewItem *" << itemName << " = new Q3IconViewItem(" << varName << ");\n";

        if (pixmap) {
            m_refreshOut << m_indent << itemName << "->setPixmap(" << pixCall(pixmap) << ");\n";
        }

        if (text) {
            m_refreshOut << m_indent << itemName << "->setText(" << trCall(text->elementString()) << ");\n";
        }
    }
}

void WriteInitialization::initializeQ3ListView(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString className = w->attributeClass();

    // columns
    const QList<DomColumn*> columns = w->elementColumn();
    for (int i=0; i<columns.size(); ++i) {
        const DomColumn *column = columns.at(i);

        const DomPropertyMap properties = propertyMap(column->elementProperty());
        const DomProperty *text = properties.value(QLatin1String("text"));
        const DomProperty *pixmap = properties.value(QLatin1String("pixmap"));
        const DomProperty *clickable = properties.value(QLatin1String("clickable"));
        const DomProperty *resizable = properties.value(QLatin1String("resizable"));

        const QString txt = trCall(text->elementString());
        m_output << m_indent << varName << "->addColumn(" << txt << ");\n";
        m_refreshOut << m_indent << varName << "->header()->setLabel(" << i << ", " << txt << ");\n";

        if (pixmap) {
            m_output << m_indent << varName << "->header()->setLabel("
                   << varName << "->header()->count() - 1, " << pixCall(pixmap) << ", " << txt << ");\n";
        }

        if (clickable != 0) {
            m_output << m_indent << varName << "->header()->setClickEnabled(" << clickable->elementBool() << ", " << varName << "->header()->count() - 1);\n";
        }

        if (resizable != 0) {
            m_output << m_indent << varName << "->header()->setResizeEnabled(" << resizable->elementBool() << ", " << varName << "->header()->count() - 1);\n";
        }
    }

    if (w->elementItem().size()) {
        m_refreshOut << m_indent << varName << "->clear();\n";

        initializeQ3ListViewItems(className, varName, w->elementItem());
    }
}

void WriteInitialization::initializeQ3ListViewItems(const QString &className, const QString &varName, const QList<DomItem *> &items)
{
    if (items.isEmpty())
        return;

    // items
    for (int i=0; i<items.size(); ++i) {
        const DomItem *item = items.at(i);

        const QString itemName = m_driver->unique(QLatin1String("__item"));
        m_refreshOut << "\n";
        m_refreshOut << m_indent << "Q3ListViewItem *" << itemName << " = new Q3ListViewItem(" << varName << ");\n";

        int textCount = 0, pixCount = 0;
        const DomPropertyList properties = item->elementProperty();
        for (int i=0; i<properties.size(); ++i) {
            const DomProperty *p = properties.at(i);
            if (p->attributeName() == QLatin1String("text"))
                m_refreshOut << m_indent << itemName << "->setText(" << textCount++ << ", "
                           << trCall(p->elementString()) << ");\n";

            if (p->attributeName() == QLatin1String("pixmap"))
                m_refreshOut << m_indent << itemName << "->setPixmap(" << pixCount++ << ", "
                           << pixCall(p) << ");\n";
        }

        if (item->elementItem().size()) {
            m_refreshOut << m_indent << itemName << "->setOpen(true);\n";
            initializeQ3ListViewItems(className, itemName, item->elementItem());
        }
    }
}


void WriteInitialization::initializeQ3Table(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString className = w->attributeClass();

    // columns
    const QList<DomColumn*> columns = w->elementColumn();

    for (int i=0; i<columns.size(); ++i) {
        const DomColumn *column = columns.at(i);

        const DomPropertyMap properties = propertyMap(column->elementProperty());
        const DomProperty *text = properties.value(QLatin1String("text"));
        const DomProperty *pixmap = properties.value(QLatin1String("pixmap"));

        m_refreshOut << m_indent << varName << "->horizontalHeader()->setLabel(" << i << ", ";
        if (pixmap) {
            m_refreshOut << pixCall(pixmap) << ", ";
        }
        m_refreshOut << trCall(text->elementString()) << ");\n";
    }

    // rows
    const QList<DomRow*> rows = w->elementRow();
    for (int i=0; i<rows.size(); ++i) {
        const DomRow *row = rows.at(i);

        const DomPropertyMap properties = propertyMap(row->elementProperty());
        const DomProperty *text = properties.value(QLatin1String("text"));
        const DomProperty *pixmap = properties.value(QLatin1String("pixmap"));

        m_refreshOut << m_indent << varName << "->verticalHeader()->setLabel(" << i << ", ";
        if (pixmap) {
            m_refreshOut << pixCall(pixmap) << ", ";
        }
        m_refreshOut << trCall(text->elementString()) << ");\n";
    }


    //initializeQ3TableItems(className, varName, w->elementItem());
}

void WriteInitialization::initializeQ3TableItems(const QString &className, const QString &varName, const QList<DomItem *> &items)
{
    Q_UNUSED(className);
    Q_UNUSED(varName);
    Q_UNUSED(items);
}

QString WriteInitialization::iconCall(const DomProperty *icon)
{
    if (icon->kind() == DomProperty::IconSet)
        return writeIconProperties(icon->elementIconSet());
    return pixCall(icon);
}

QString WriteInitialization::pixCall(const DomProperty *p) const
{
    QString type, s;
    switch (p->kind()) {
    case DomProperty::IconSet:
        type = QLatin1String("QIcon");
        s = p->elementIconSet()->text();
        break;
    case DomProperty::Pixmap:
        type = QLatin1String("QPixmap");
        s = p->elementPixmap()->text();
        break;
    default:
        qWarning("%s: Warning: Unknown icon format encountered. The ui-file was generated with a too-recent version of Designer.",
                 qPrintable(m_option.messagePrefix()));
        return QLatin1String("QIcon()");
        break;
    }
    return pixCall(type, s);
}

QString WriteInitialization::pixCall(const QString &t, const QString &text) const
{
    QString type = t;
    if (text.isEmpty()) {
        type += QLatin1String("()");
        return type;
    }
    if (const DomImage *image = findImage(text)) {
        if (m_option.extractImages) {
            const QString format = image->elementData()->attributeFormat();
            const QString extension = format.left(format.indexOf(QLatin1Char('.'))).toLower();
            QString rc = QLatin1String("QPixmap(QString::fromUtf8(\":/");
            rc += m_generatedClass;
            rc += QLatin1String("/images/");
            rc += text;
            rc += QLatin1Char('.');
            rc += extension;
            rc += QLatin1String("\"))");
            return rc;
        }
        QString rc = WriteIconInitialization::iconFromDataFunction();
        rc += QLatin1Char('(');
        rc += text;
        rc += QLatin1String("_ID)");
        return rc;
    }

    QString pixFunc = m_uic->pixmapFunction();
    if (pixFunc.isEmpty())
        pixFunc = QLatin1String("QString::fromUtf8");

    type += QLatin1Char('(');
    type += pixFunc;
    type += QLatin1Char('(');
    type += fixString(text, m_dindent);
    type += QLatin1String("))");
    return type;
}

void WriteInitialization::initializeComboBox3(DomWidget *w)
{
    const QList<DomItem*> items = w->elementItem();
    if (items.empty())
        return;
    // Basic legacy Qt3 support, write out translatable text items, ignore pixmaps
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString textProperty = QLatin1String("text");

    m_refreshOut << m_indent << varName << "->clear();\n";
    m_refreshOut << m_indent << varName << "->insertStringList(QStringList()" << '\n';
    const int itemCount = items.size();
    for (int i = 0; i< itemCount; ++i) {
        const DomItem *item = items.at(i);
        if (const DomProperty *text = propertyMap(item->elementProperty()).value(textProperty))
            m_refreshOut << m_indent << " << " << autoTrCall(text->elementString()) << "\n";
    }
    m_refreshOut << m_indent << ", 0);\n";
}

void WriteInitialization::initializeComboBox(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString className = w->attributeClass();

    const QList<DomItem*> items = w->elementItem();

    if (items.isEmpty())
        return;

    // If possible use qcombobox's addItems() which is much faster then a bunch of addItem() calls
    bool makeStringListCall = true;
    bool translatable = false;
    QStringList list;
    for (int i=0; i<items.size(); ++i) {
        const DomItem *item = items.at(i);
        const DomPropertyMap properties = propertyMap(item->elementProperty());
        const DomProperty *text = properties.value(QLatin1String("text"));
        const DomProperty *pixmap = properties.value(QLatin1String("icon"));
        bool needsTr = needsTranslation(text->elementString());
        if (pixmap != 0 || (i > 0 && translatable != needsTr)) {
            makeStringListCall = false;
            break;
        }
        translatable = needsTr;
        list.append(autoTrCall(text->elementString()));  // fix me here
    }

    if (makeStringListCall) {
        QTextStream &o = translatable ? m_refreshOut : m_output;
        if (translatable)
            o << m_indent << varName << "->clear();\n";
        o << m_indent << varName << "->insertItems(0, QStringList()" << '\n';
        for (int i = 0; i < list.size(); ++i)
            o << m_indent << " << " << list.at(i) << "\n";
        o << m_indent << ");\n";
    } else {
        for (int i = 0; i < items.size(); ++i) {
            const DomItem *item = items.at(i);
            const DomPropertyMap properties = propertyMap(item->elementProperty());
            const DomProperty *text = properties.value(QLatin1String("text"));
            const DomProperty *icon = properties.value(QLatin1String("icon"));

            QString iconValue;
            if (icon)
                iconValue = iconCall(icon);

            m_output << m_indent << varName << "->addItem(";
            if (icon)
                m_output << iconValue << ", ";

            if (needsTranslation(text->elementString())) {
                m_output << "QString());\n";
                m_refreshOut << m_indent << varName << "->setItemText(" << i << ", " << trCall(text->elementString()) << ");\n";
            } else {
                m_output << noTrCall(text->elementString()) << ");\n";
            }
        }
        m_refreshOut << "\n";
    }
}

QString WriteInitialization::disableSorting(DomWidget *w, const QString &varName)
{
    // turn off sortingEnabled to force programmatic item order (setItem())
    QString tempName;
    if (!w->elementItem().isEmpty()) {
        tempName = m_driver->unique(QLatin1String("__sortingEnabled"));
        m_refreshOut << "\n";
        m_refreshOut << m_indent << "const bool " << tempName
            << " = " << varName << "->isSortingEnabled();\n";
        m_refreshOut << m_indent << varName << "->setSortingEnabled(false);\n";
    }
    return tempName;
}

void WriteInitialization::enableSorting(DomWidget *w, const QString &varName, const QString &tempName)
{
    if (!w->elementItem().isEmpty()) {
        m_refreshOut << m_indent << varName << "->setSortingEnabled(" << tempName << ");\n\n";
    }
}

/*
 * Initializers are just strings containing the function call and need to be prepended
 * the line indentation and the object they are supposed to initialize.
 * String initializers come with a preprocessor conditional (ifdef), so the code
 * compiles with QT_NO_xxx. A null pointer means no conditional. String initializers
 * are written to the retranslateUi() function, others to setupUi().
 */


/*!
    Create non-string inititializer.
    \param value the value to initialize the attribute with. May be empty, in which case
        the initializer is omitted.
    See above for other parameters.
*/
void WriteInitialization::addInitializer(Item *item,
        const QString &name, int column, const QString &value, const QString &directive, bool translatable) const
{
    if (!value.isEmpty())
        item->addSetter(QLatin1String("->set") + name.at(0).toUpper() + name.mid(1) +
                    QLatin1Char('(') + (column < 0 ? QString() : QString::number(column) +
                    QLatin1String(", ")) + value + QLatin1String(");"), directive, translatable);
}

/*!
    Create string inititializer.
    \param initializers in/out list of inializers
    \param properties map property name -> property to extract data from
    \param name the property to extract
    \param col the item column to generate the initializer for. This is relevant for
        tree widgets only. If it is -1, no column index will be generated.
    \param ifdef preprocessor symbol for disabling compilation of this initializer
*/
void WriteInitialization::addStringInitializer(Item *item,
        const DomPropertyMap &properties, const QString &name, int column, const QString &directive) const
{
    if (const DomProperty *p = properties.value(name)) {
        DomString *str = p->elementString();
        QString text = toString(str);
        if (!text.isEmpty()) {
            bool translatable = needsTranslation(str);
            QString value = autoTrCall(str);
            addInitializer(item, name, column, value, directive, translatable);
        }
    }
}

void WriteInitialization::addBrushInitializer(Item *item,
        const DomPropertyMap &properties, const QString &name, int column)
{
    if (const DomProperty *p = properties.value(name)) {
        if (p->elementBrush())
            addInitializer(item, name, column, writeBrushInitialization(p->elementBrush()));
        else if (p->elementColor())
            addInitializer(item, name, column, domColor2QString(p->elementColor()));
    }
}

/*!
    Create inititializer for a flag value in the Qt namespace.
    If the named property is not in the map, the initializer is omitted.
*/
void WriteInitialization::addQtFlagsInitializer(Item *item,
        const DomPropertyMap &properties, const QString &name, int column) const
{
    if (const DomProperty *p = properties.value(name)) {
        QString v = p->elementSet();
        if (!v.isEmpty()) {
            v.replace(QLatin1Char('|'), QLatin1String("|Qt::"));
            addInitializer(item, name, column, QLatin1String("Qt::") + v);
        }
    }
}

/*!
    Create inititializer for an enum value in the Qt namespace.
    If the named property is not in the map, the initializer is omitted.
*/
void WriteInitialization::addQtEnumInitializer(Item *item,
        const DomPropertyMap &properties, const QString &name, int column) const
{
    if (const DomProperty *p = properties.value(name)) {
        QString v = p->elementEnum();
        if (!v.isEmpty())
            addInitializer(item, name, column, QLatin1String("Qt::") + v);
    }
}

/*!
    Create inititializers for all common properties that may be bound to a column.
*/
void WriteInitialization::addCommonInitializers(Item *item,
        const DomPropertyMap &properties, int column)
{
    if (const DomProperty *icon = properties.value(QLatin1String("icon")))
        addInitializer(item, QLatin1String("icon"), column, iconCall(icon));
    addBrushInitializer(item, properties, QLatin1String("foreground"), column);
    addBrushInitializer(item, properties, QLatin1String("background"), column);
    if (const DomProperty *font = properties.value(QLatin1String("font")))
        addInitializer(item, QLatin1String("font"), column, writeFontProperties(font->elementFont()));
    addQtFlagsInitializer(item, properties, QLatin1String("textAlignment"), column);
    addQtEnumInitializer(item, properties, QLatin1String("checkState"), column);
    addStringInitializer(item, properties, QLatin1String("text"), column);
    addStringInitializer(item, properties, QLatin1String("toolTip"), column, QLatin1String(toolTipDefineC));
    addStringInitializer(item, properties, QLatin1String("whatsThis"), column, QLatin1String(whatsThisDefineC));
    addStringInitializer(item, properties, QLatin1String("statusTip"), column, QLatin1String(statusTipDefineC));
}

void WriteInitialization::initializeListWidget(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);
    const QString className = w->attributeClass();

    const QList<DomItem*> items = w->elementItem();

    if (items.isEmpty())
        return;

    QString tempName = disableSorting(w, varName);
    // items
    // TODO: the generated code should be data-driven to reduce its size
    for (int i = 0; i < items.size(); ++i) {
        const DomItem *domItem = items.at(i);

        const DomPropertyMap properties = propertyMap(domItem->elementProperty());

        Item item(QLatin1String("QListWidgetItem"), m_indent, m_output, m_refreshOut, m_driver);
        addQtFlagsInitializer(&item, properties, QLatin1String("flags"));
        addCommonInitializers(&item, properties);

        item.writeSetupUi(varName);
        item.writeRetranslateUi(varName + QLatin1String("->item(") + QString::number(i) + QLatin1Char(')'));
    }
    enableSorting(w, varName, tempName);
}

void WriteInitialization::initializeTreeWidget(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);

    // columns
    Item item(QLatin1String("QTreeWidgetItem"), m_indent, m_output, m_refreshOut, m_driver);

    const QList<DomColumn*> columns = w->elementColumn();
    for (int i = 0; i < columns.size(); ++i) {
        const DomColumn *column = columns.at(i);

        const DomPropertyMap properties = propertyMap(column->elementProperty());
        addCommonInitializers(&item, properties, i);
    }
    const QString itemName = item.writeSetupUi(QString(), Item::DontConstruct);
    item.writeRetranslateUi(varName + QLatin1String("->headerItem()"));
    if (!itemName.isNull())
        m_output << m_indent << varName << "->setHeaderItem(" << itemName << ");\n";

    if (w->elementItem().size() == 0)
        return;

    QString tempName = disableSorting(w, varName);

    QList<Item *> items = initializeTreeWidgetItems(w->elementItem());
    for (int i = 0; i < items.count(); i++) {
        Item *itm = items[i];
        itm->writeSetupUi(varName);
        itm->writeRetranslateUi(varName + QLatin1String("->topLevelItem(") + QString::number(i) + QLatin1Char(')'));
        delete itm;
    }

    enableSorting(w, varName, tempName);
}

/*!
    Create and write out initializers for tree widget items.
    This function makes sure that only needed items are fetched (subject to preprocessor
    conditionals), that each item is fetched from its parent widget/item exactly once
    and that no temporary variables are created for items that are needed only once. As
    fetches are built top-down from the root, but determining how often and under which
    conditions an item is needed needs to be done bottom-up, the whole process makes
    two passes, storing the intermediate result in a recursive StringInitializerListMap.
*/
QList<WriteInitialization::Item *> WriteInitialization::initializeTreeWidgetItems(const QList<DomItem *> &domItems)
{
    // items
    QList<Item *> items;

    for (int i = 0; i < domItems.size(); ++i) {
        const DomItem *domItem = domItems.at(i);

        Item *item = new Item(QLatin1String("QTreeWidgetItem"), m_indent, m_output, m_refreshOut, m_driver);
        items << item;

        QHash<QString, DomProperty *> map;

        int col = -1;
        const DomPropertyList properties = domItem->elementProperty();
        for (int j = 0; j < properties.size(); ++j) {
            DomProperty *p = properties.at(j);
            if (p->attributeName() == QLatin1String("text")) {
                if (!map.isEmpty()) {
                    addCommonInitializers(item, map, col);
                    map.clear();
                }
                col++;
            }
            map.insert(p->attributeName(), p);
        }
        addCommonInitializers(item, map, col);
        // AbstractFromBuilder saves flags last, so they always end up in the last column's map.
        addQtFlagsInitializer(item, map, QLatin1String("flags"));

        QList<Item *> subItems = initializeTreeWidgetItems(domItem->elementItem());
        foreach (Item *subItem, subItems)
            item->addChild(subItem);
    }
    return items;
}

void WriteInitialization::initializeTableWidget(DomWidget *w)
{
    const QString varName = m_driver->findOrInsertWidget(w);

    // columns
    const QList<DomColumn *> columns = w->elementColumn();

    if (columns.size() != 0) {
        m_output << m_indent << "if (" << varName << "->columnCount() < " << columns.size() << ")\n"
            << m_dindent << varName << "->setColumnCount(" << columns.size() << ");\n";
    }

    for (int i = 0; i < columns.size(); ++i) {
        const DomColumn *column = columns.at(i);
        if (!column->elementProperty().isEmpty()) {
            const DomPropertyMap properties = propertyMap(column->elementProperty());

            Item item(QLatin1String("QTableWidgetItem"), m_indent, m_output, m_refreshOut, m_driver);
            addCommonInitializers(&item, properties);

            QString itemName = item.writeSetupUi(QString(), Item::ConstructItemAndVariable);
            item.writeRetranslateUi(varName + QLatin1String("->horizontalHeaderItem(") + QString::number(i) + QLatin1Char(')'));
            m_output << m_indent << varName << "->setHorizontalHeaderItem(" << QString::number(i) << ", " << itemName << ");\n";
        }
    }

    // rows
    const QList<DomRow *> rows = w->elementRow();

    if (rows.size() != 0) {
        m_output << m_indent << "if (" << varName << "->rowCount() < " << rows.size() << ")\n"
            << m_dindent << varName << "->setRowCount(" << rows.size() << ");\n";
    }

    for (int i = 0; i < rows.size(); ++i) {
        const DomRow *row = rows.at(i);
        if (!row->elementProperty().isEmpty()) {
            const DomPropertyMap properties = propertyMap(row->elementProperty());

            Item item(QLatin1String("QTableWidgetItem"), m_indent, m_output, m_refreshOut, m_driver);
            addCommonInitializers(&item, properties);

            QString itemName = item.writeSetupUi(QString(), Item::ConstructItemAndVariable);
            item.writeRetranslateUi(varName + QLatin1String("->verticalHeaderItem(") + QString::number(i) + QLatin1Char(')'));
            m_output << m_indent << varName << "->setVerticalHeaderItem(" << QString::number(i) << ", " << itemName << ");\n";
        }
    }

    // items
    QString tempName = disableSorting(w, varName);

    const QList<DomItem *> items = w->elementItem();

    for (int i = 0; i < items.size(); ++i) {
        const DomItem *cell = items.at(i);
        if (cell->hasAttributeRow() && cell->hasAttributeColumn() && !cell->elementProperty().isEmpty()) {
            const int r = cell->attributeRow();
            const int c = cell->attributeColumn();
            const DomPropertyMap properties = propertyMap(cell->elementProperty());

            Item item(QLatin1String("QTableWidgetItem"), m_indent, m_output, m_refreshOut, m_driver);
            addQtFlagsInitializer(&item, properties, QLatin1String("flags"));
            addCommonInitializers(&item, properties);

            QString itemName = item.writeSetupUi(QString(), Item::ConstructItemAndVariable);
            item.writeRetranslateUi(varName + QLatin1String("->item(") + QString::number(r) + QLatin1String(", ") + QString::number(c) + QLatin1Char(')'));
            m_output << m_indent << varName << "->setItem(" << QString::number(r) << ", " << QString::number(c) << ", " << itemName << ");\n";
        }
    }
    enableSorting(w, varName, tempName);
}

QString WriteInitialization::trCall(const QString &str, const QString &commentHint) const
{
    if (str.isEmpty())
        return QLatin1String("QString()");

    QString result;
    const QString comment = commentHint.isEmpty() ? QString(QLatin1Char('0')) : fixString(commentHint, m_dindent);

    if (m_option.translateFunction.isEmpty()) {
        result = QLatin1String("QApplication::translate(\"");
        result += m_generatedClass;
        result += QLatin1Char('"');
        result += QLatin1String(", ");
    } else {
        result = m_option.translateFunction;
        result += QLatin1Char('(');
    }

    result += fixString(str, m_dindent);
    result += QLatin1String(", ");
    result += comment;

    if (m_option.translateFunction.isEmpty()) {
        result += QLatin1String(", ");
        result += QLatin1String("QApplication::UnicodeUTF8");
    }

    result += QLatin1Char(')');
    return result;
}

void WriteInitialization::initializeQ3SqlDataTable(DomWidget *w)
{
    const DomPropertyMap properties = propertyMap(w->elementProperty());

    const DomProperty *frameworkCode = properties.value(QLatin1String("frameworkCode"), 0);
    if (frameworkCode && toBool(frameworkCode->elementBool()) == false)
        return;

    QString connection;
    QString table;
    QString field;

    const DomProperty *db = properties.value(QLatin1String("database"), 0);
    if (db && db->elementStringList()) {
        const QStringList info = db->elementStringList()->elementString();
        connection = info.size() > 0 ? info.at(0) : QString();
        table = info.size() > 1 ? info.at(1) : QString();
        field = info.size() > 2 ? info.at(2) : QString();
    }

    if (table.isEmpty() || connection.isEmpty()) {
        fprintf(stderr, "%s: Warning: Invalid database connection\n", qPrintable(m_option.messagePrefix()));
        return;
    }

   const QString varName = m_driver->findOrInsertWidget(w);

    m_output << m_indent << "if (!" << varName << "->sqlCursor()) {\n";

    m_output << m_dindent << varName << "->setSqlCursor(";

    if (connection == QLatin1String("(default)")) {
        m_output << "new Q3SqlCursor(" << fixString(table, m_dindent) << "), false, true);\n";
    } else {
        m_output << "new Q3SqlCursor(" << fixString(table, m_dindent) << ", true, " << connection << "Connection" << "), false, true);\n";
    }
    m_output << m_dindent << varName << "->refresh(Q3DataTable::RefreshAll);\n";
    m_output << m_indent << "}\n";
}

void WriteInitialization::initializeQ3SqlDataBrowser(DomWidget *w)
{
    const DomPropertyMap properties = propertyMap(w->elementProperty());

    const DomProperty *frameworkCode = properties.value(QLatin1String("frameworkCode"), 0);
    if (frameworkCode && toBool(frameworkCode->elementBool()) == false)
        return;

    QString connection;
    QString table;
    QString field;

    const DomProperty *db = properties.value(QLatin1String("database"), 0);
    if (db && db->elementStringList()) {
        const QStringList info = db->elementStringList()->elementString();
        connection = info.size() > 0 ? info.at(0) : QString();
        table = info.size() > 1 ? info.at(1) : QString();
        field = info.size() > 2 ? info.at(2) : QString();
    }

    if (table.isEmpty() || connection.isEmpty()) {
        fprintf(stderr, "%s: Warning: Invalid database connection\n", qPrintable(m_option.messagePrefix()));
        return;
    }

    const QString varName = m_driver->findOrInsertWidget(w);

    m_output << m_indent << "if (!" << varName << "->sqlCursor()) {\n";

    m_output << m_dindent << varName << "->setSqlCursor(";

    if (connection == QLatin1String("(default)")) {
        m_output << "new Q3SqlCursor(" << fixString(table, m_dindent) << "), true);\n";
    } else {
        m_output << "new Q3SqlCursor(" << fixString(table, m_dindent) << ", true, " << connection << "Connection" << "), false, true);\n";
    }
    m_output << m_dindent << varName << "->refresh();\n";
    m_output << m_indent << "}\n";
}

void WriteInitialization::initializeMenu(DomWidget *w, const QString &/*parentWidget*/)
{
    const QString menuName = m_driver->findOrInsertWidget(w);
    const QString menuAction = menuName + QLatin1String("Action");

    const DomAction *action = m_driver->actionByName(menuAction);
    if (action && action->hasAttributeMenu()) {
        m_output << m_indent << menuAction << " = " << menuName << "->menuAction();\n";
    }
}

QString WriteInitialization::trCall(DomString *str, const QString &defaultString) const
{
    QString value = defaultString;
    QString comment;
    if (str) {
        value = toString(str);
        comment = str->attributeComment();
    }
    return trCall(value, comment);
}

QString WriteInitialization::noTrCall(DomString *str, const QString &defaultString) const
{
    QString value = defaultString;
    if (!str && defaultString.isEmpty())
        return QString();
    if (str)
        value = str->text();
    QString ret = QLatin1String("QString::fromUtf8(");
    ret += fixString(value, m_dindent);
    ret += QLatin1Char(')');
    return ret;
}

QString WriteInitialization::autoTrCall(DomString *str, const QString &defaultString) const
{
    if ((!str && !defaultString.isEmpty()) || needsTranslation(str))
        return trCall(str, defaultString);
    return noTrCall(str, defaultString);
}

QTextStream &WriteInitialization::autoTrOutput(DomString *str, const QString &defaultString)
{
    if ((!str && !defaultString.isEmpty()) || needsTranslation(str))
        return m_refreshOut;
    return m_output;
}

bool WriteInitialization::isValidObject(const QString &name) const
{
    return m_registeredWidgets.contains(name)
        || m_registeredActions.contains(name);
}

QString WriteInitialization::findDeclaration(const QString &name)
{
    const QString normalized = Driver::normalizedName(name);

    if (DomWidget *widget = m_driver->widgetByName(normalized))
        return m_driver->findOrInsertWidget(widget);
    if (DomAction *action = m_driver->actionByName(normalized))
        return m_driver->findOrInsertAction(action);
    if (const DomButtonGroup *group = m_driver->findButtonGroup(normalized))
        return m_driver->findOrInsertButtonGroup(group);
    return QString();
}

void WriteInitialization::acceptConnection(DomConnection *connection)
{
    const QString sender = findDeclaration(connection->elementSender());
    const QString receiver = findDeclaration(connection->elementReceiver());

    if (sender.isEmpty() || receiver.isEmpty())
        return;

    m_output << m_indent << "QObject::connect("
        << sender
        << ", "
        << "SIGNAL("<<connection->elementSignal()<<')'
        << ", "
        << receiver
        << ", "
        << "SLOT("<<connection->elementSlot()<<')'
        << ");\n";
}

DomImage *WriteInitialization::findImage(const QString &name) const
{
    return m_registeredImages.value(name);
}

DomWidget *WriteInitialization::findWidget(const QLatin1String &widgetClass)
{
    for (int i = m_widgetChain.count() - 1; i >= 0; --i) {
        DomWidget *widget = m_widgetChain.at(i);

        if (widget && m_uic->customWidgetsInfo()->extends(widget->attributeClass(), widgetClass))
            return widget;
    }

    return 0;
}

void WriteInitialization::acceptImage(DomImage *image)
{
    if (!image->hasAttributeName())
        return;

    m_registeredImages.insert(image->attributeName(), image);
}

void WriteInitialization::acceptWidgetScripts(const DomScripts &widgetScripts, DomWidget *node, const  DomWidgets &childWidgets)
{
    // Add the per-class custom scripts to the per-widget ones.
    DomScripts scripts(widgetScripts);

    if (DomScript *customWidgetScript = m_uic->customWidgetsInfo()->customWidgetScript(node->attributeClass()))
        scripts.push_front(customWidgetScript);

    if (scripts.empty())
        return;

    // concatenate script snippets
    QString script;
    foreach (const DomScript *domScript, scripts) {
        const QString snippet = domScript->text();
        if (!snippet.isEmpty()) {
            script += snippet.trimmed();
            script += QLatin1Char('\n');
        }
    }
    if (script.isEmpty())
        return;

    // Build the list of children and insert call
    m_output << m_indent << "childWidgets.clear();\n";
    if (!childWidgets.empty()) {
        m_output << m_indent <<  "childWidgets";
        foreach (DomWidget *child, childWidgets) {
            m_output << " << " << m_driver->findOrInsertWidget(child);
        }
        m_output << ";\n";
    }
    m_output << m_indent << "scriptContext.run(QString::fromUtf8("
             << fixString(script, m_dindent) << "), "
             << m_driver->findOrInsertWidget(node) << ", childWidgets);\n";
}


static void generateMultiDirectiveBegin(QTextStream &outputStream, const QSet<QString> &directives)
{
    if (directives.isEmpty())
        return;

    QMap<QString, bool> map; // bool is dummy. The idea is to sort that (always generate in the same order) by putting a set into a map
    foreach (const QString &str, directives)
        map.insert(str, true);

    if (map.size() == 1) {
        outputStream << "#ifndef " << map.constBegin().key() << endl;
        return;
    }

    outputStream << "#if";
    bool doOr = false;
    foreach (const QString &str, map.keys()) {
        if (doOr)
            outputStream << " ||";
        outputStream << " !defined(" << str << ')';
        doOr = true;
    }
    outputStream << endl;
}

static void generateMultiDirectiveEnd(QTextStream &outputStream, const QSet<QString> &directives)
{
    if (directives.isEmpty())
        return;

    outputStream << "#endif" << endl;
}

WriteInitialization::Item::Item(const QString &itemClassName, const QString &indent, QTextStream &setupUiStream, QTextStream &retranslateUiStream, Driver *driver)
    :
    m_parent(0),
    m_itemClassName(itemClassName),
    m_indent(indent),
    m_setupUiStream(setupUiStream),
    m_retranslateUiStream(retranslateUiStream),
    m_driver(driver)
{

}

WriteInitialization::Item::~Item()
{
    foreach (Item *child, m_children)
        delete child;
}

QString WriteInitialization::Item::writeSetupUi(const QString &parent, Item::EmptyItemPolicy emptyItemPolicy)
{
    if (emptyItemPolicy == Item::DontConstruct && m_setupUiData.policy == ItemData::DontGenerate)
        return QString();

    bool generateMultiDirective = false;
    if (emptyItemPolicy == Item::ConstructItemOnly && m_children.size() == 0) {
        if (m_setupUiData.policy == ItemData::DontGenerate) {
            m_setupUiStream << m_indent << "new " << m_itemClassName << '(' << parent << ");\n";
                return QString();
        } else if (m_setupUiData.policy == ItemData::GenerateWithMultiDirective) {
            generateMultiDirective = true;
        }
    }

    if (generateMultiDirective)
        generateMultiDirectiveBegin(m_setupUiStream, m_setupUiData.directives);

    const QString uniqueName = m_driver->unique(QLatin1String("__") + m_itemClassName.toLower());
    m_setupUiStream << m_indent << m_itemClassName << " *" << uniqueName << " = new " << m_itemClassName << '(' << parent << ");\n";

    if (generateMultiDirective) {
        m_setupUiStream << "#else\n";
        m_setupUiStream << m_indent << "new " << m_itemClassName << '(' << parent << ");\n";
        generateMultiDirectiveEnd(m_setupUiStream, m_setupUiData.directives);
    }

    QMultiMap<QString, QString>::ConstIterator it = m_setupUiData.setters.constBegin();
    while (it != m_setupUiData.setters.constEnd()) {
        openIfndef(m_setupUiStream, it.key());
        m_setupUiStream << m_indent << uniqueName << it.value() << endl;
        closeIfndef(m_setupUiStream, it.key());
        ++it;
    }
    foreach (Item *child, m_children)
        child->writeSetupUi(uniqueName);
    return uniqueName;
}

void WriteInitialization::Item::writeRetranslateUi(const QString &parentPath)
{
    if (m_retranslateUiData.policy == ItemData::DontGenerate)
        return;

    if (m_retranslateUiData.policy == ItemData::GenerateWithMultiDirective)
        generateMultiDirectiveBegin(m_retranslateUiStream, m_retranslateUiData.directives);

    const QString uniqueName = m_driver->unique(QLatin1String("___") + m_itemClassName.toLower());
    m_retranslateUiStream << m_indent << m_itemClassName << " *" << uniqueName << " = " << parentPath << ";\n";

    if (m_retranslateUiData.policy == ItemData::GenerateWithMultiDirective)
        generateMultiDirectiveEnd(m_retranslateUiStream, m_retranslateUiData.directives);

    QString oldDirective;
    QMultiMap<QString, QString>::ConstIterator it = m_retranslateUiData.setters.constBegin();
    while (it != m_retranslateUiData.setters.constEnd()) {
        const QString newDirective = it.key();
        if (oldDirective != newDirective) {
            closeIfndef(m_retranslateUiStream, oldDirective);
            openIfndef(m_retranslateUiStream, newDirective);
            oldDirective = newDirective;
        }
        m_retranslateUiStream << m_indent << uniqueName << it.value() << endl;
        ++it;
    }
    closeIfndef(m_retranslateUiStream, oldDirective);

    for (int i = 0; i < m_children.size(); i++)
        m_children[i]->writeRetranslateUi(uniqueName + QLatin1String("->child(") + QString::number(i) + QLatin1Char(')'));
}

void WriteInitialization::Item::addSetter(const QString &setter, const QString &directive, bool translatable)
{
    const ItemData::TemporaryVariableGeneratorPolicy newPolicy = directive.isNull() ? ItemData::Generate : ItemData::GenerateWithMultiDirective;
    if (translatable) {
        m_retranslateUiData.setters.insert(directive, setter);
        if (ItemData::GenerateWithMultiDirective == newPolicy)
            m_retranslateUiData.directives << directive;
        if (m_retranslateUiData.policy < newPolicy)
            m_retranslateUiData.policy = newPolicy;
    } else {
        m_setupUiData.setters.insert(directive, setter);
        if (ItemData::GenerateWithMultiDirective == newPolicy)
            m_setupUiData.directives << directive;
        if (m_setupUiData.policy < newPolicy)
            m_setupUiData.policy = newPolicy;
    }
}

void WriteInitialization::Item::addChild(Item *child)
{
    m_children << child;
    child->m_parent = this;

    Item *c = child;
    Item *p = this;
    while (p) {
        p->m_setupUiData.directives |= c->m_setupUiData.directives;
        p->m_retranslateUiData.directives |= c->m_retranslateUiData.directives;
        if (p->m_setupUiData.policy < c->m_setupUiData.policy)
            p->m_setupUiData.policy = c->m_setupUiData.policy;
        if (p->m_retranslateUiData.policy < c->m_retranslateUiData.policy)
            p->m_retranslateUiData.policy = c->m_retranslateUiData.policy;
        c = p;
        p = p->m_parent;
    }
}


} // namespace CPP

QT_END_NAMESPACE

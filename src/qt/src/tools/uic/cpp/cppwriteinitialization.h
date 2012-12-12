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

#ifndef CPPWRITEINITIALIZATION_H
#define CPPWRITEINITIALIZATION_H

#include "treewalker.h"
#include <QtCore/QPair>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QMap>
#include <QtCore/QStack>
#include <QtCore/QTextStream>

QT_BEGIN_NAMESPACE

class Driver;
class Uic;
class DomBrush;
class DomFont;
class DomResourceIcon;
class DomSizePolicy;
struct Option;

namespace CPP {
    // Handle for a flat DOM font to get comparison functionality required for maps
    class FontHandle {
    public:
        FontHandle(const DomFont *domFont);
        int compare(const FontHandle &) const;
    private:
        const DomFont *m_domFont;
#if defined(Q_OS_MAC) && defined(Q_CC_GNU) && (__GNUC__ == 3 && __GNUC_MINOR__ == 3)
        friend uint qHash(const FontHandle &);
#endif
    };
    inline bool operator ==(const FontHandle &f1, const FontHandle &f2) { return f1.compare(f2) == 0; }
    inline bool operator  <(const FontHandle &f1, const FontHandle &f2) { return f1.compare(f2) < 0; }

    // Handle for a flat DOM icon to get comparison functionality required for maps
    class IconHandle {
    public:
        IconHandle(const DomResourceIcon *domIcon);
        int compare(const IconHandle &) const;
    private:
        const DomResourceIcon *m_domIcon;
#if defined(Q_OS_MAC) && defined(Q_CC_GNU) && (__GNUC__ == 3 && __GNUC_MINOR__ == 3)
        friend uint qHash(const IconHandle &);
#endif
    };
    inline bool operator ==(const IconHandle &i1, const IconHandle &i2) { return i1.compare(i2) == 0; }
    inline bool operator  <(const IconHandle &i1, const IconHandle &i2) { return i1.compare(i2) < 0; }

    // Handle for a flat DOM size policy to get comparison functionality required for maps
    class SizePolicyHandle {
    public:
        SizePolicyHandle(const DomSizePolicy *domSizePolicy);
        int compare(const SizePolicyHandle &) const;
    private:
        const DomSizePolicy *m_domSizePolicy;
#if defined(Q_OS_MAC) && defined(Q_CC_GNU) && (__GNUC__ == 3 && __GNUC_MINOR__ == 3)
        friend uint qHash(const SizePolicyHandle &);
#endif
    };
    inline bool operator ==(const SizePolicyHandle &f1, const SizePolicyHandle &f2) { return f1.compare(f2) == 0; }
#if !(defined(Q_OS_MAC) && defined(Q_CC_GNU) && (__GNUC__ == 3 && __GNUC_MINOR__ == 3))
    inline bool operator  <(const SizePolicyHandle &f1, const SizePolicyHandle &f2) { return f1.compare(f2) < 0; }
#endif



struct WriteInitialization : public TreeWalker
{
    typedef QList<DomProperty*> DomPropertyList;
    typedef QHash<QString, DomProperty*> DomPropertyMap;

    WriteInitialization(Uic *uic, bool activateScripts);

//
// widgets
//
    void acceptUI(DomUI *node);
    void acceptWidget(DomWidget *node);
    void acceptWidgetScripts(const DomScripts &, DomWidget *node, const  DomWidgets &childWidgets);

    void acceptLayout(DomLayout *node);
    void acceptSpacer(DomSpacer *node);
    void acceptLayoutItem(DomLayoutItem *node);

//
// actions
//
    void acceptActionGroup(DomActionGroup *node);
    void acceptAction(DomAction *node);
    void acceptActionRef(DomActionRef *node);

//
// tab stops
//
    void acceptTabStops(DomTabStops *tabStops);

//
// custom widgets
//
    void acceptCustomWidgets(DomCustomWidgets *node);
    void acceptCustomWidget(DomCustomWidget *node);

//
// layout defaults/functions
//
    void acceptLayoutDefault(DomLayoutDefault *node)   { m_LayoutDefaultHandler.acceptLayoutDefault(node); }
    void acceptLayoutFunction(DomLayoutFunction *node) { m_LayoutDefaultHandler.acceptLayoutFunction(node); }

//
// signal/slot connections
//
    void acceptConnection(DomConnection *connection);

//
// images
//
    void acceptImage(DomImage *image);

    enum {
        Use43UiFile = 0,
        TopLevelMargin,
        ChildMargin,
        SubLayoutMargin
    };

private:
    static QString domColor2QString(const DomColor *c);

    QString iconCall(const DomProperty *prop);
    QString pixCall(const DomProperty *prop) const;
    QString pixCall(const QString &type, const QString &text) const;
    QString trCall(const QString &str, const QString &comment = QString()) const;
    QString trCall(DomString *str, const QString &defaultString = QString()) const;
    QString noTrCall(DomString *str, const QString &defaultString = QString()) const;
    QString autoTrCall(DomString *str, const QString &defaultString = QString()) const;
    QTextStream &autoTrOutput(DomString *str, const QString &defaultString = QString());
    // Apply a comma-separated list of values using a function "setSomething(int idx, value)"
    void writePropertyList(const QString &varName, const QString &setFunction, const QString &value, const QString &defaultValue);

    enum { WritePropertyIgnoreMargin = 1, WritePropertyIgnoreSpacing = 2, WritePropertyIgnoreObjectName = 4 };
    void writeProperties(const QString &varName, const QString &className, const DomPropertyList &lst, unsigned flags = 0);
    void writeColorGroup(DomColorGroup *colorGroup, const QString &group, const QString &paletteName);
    void writeBrush(const DomBrush *brush, const QString &brushName);

//
// special initialization
//
    class Item {
    public:
        Item(const QString &itemClassName, const QString &indent, QTextStream &setupUiStream, QTextStream &retranslateUiStream, Driver *driver);
        ~Item();
        enum EmptyItemPolicy {
            DontConstruct,
            ConstructItemOnly,
            ConstructItemAndVariable
        };
        QString writeSetupUi(const QString &parent, EmptyItemPolicy emptyItemPolicy = ConstructItemOnly);
        void writeRetranslateUi(const QString &parentPath);
        void addSetter(const QString &setter, const QString &directive = QString(), bool translatable = false); // don't call it if you already added *this as a child of another Item
        void addChild(Item *child); // all setters should already been added
        int setupUiCount() const { return m_setupUiData.setters.count(); }
        int retranslateUiCount() const { return m_retranslateUiData.setters.count(); }
    private:
        struct ItemData {
            ItemData() : policy(DontGenerate) {}
            QMultiMap<QString, QString> setters; // directive to setter
            QSet<QString> directives;
            enum TemporaryVariableGeneratorPolicy { // policies with priority, number describes the priority
                DontGenerate = 1,
                GenerateWithMultiDirective = 2,
                Generate = 3
            } policy;
        };
        ItemData m_setupUiData;
        ItemData m_retranslateUiData;
        QList<Item *> m_children;
        Item *m_parent;

        const QString m_itemClassName;
        const QString m_indent;
        QTextStream &m_setupUiStream;
        QTextStream &m_retranslateUiStream;
        Driver *m_driver;
    };

    void addInitializer(Item *item,
            const QString &name, int column, const QString &value, const QString &directive = QString(), bool translatable = false) const;
    void addQtFlagsInitializer(Item *item, const DomPropertyMap &properties,
            const QString &name, int column = -1) const;
    void addQtEnumInitializer(Item *item,
                    const DomPropertyMap &properties, const QString &name, int column = -1) const;
    void addBrushInitializer(Item *item,
                    const DomPropertyMap &properties, const QString &name, int column = -1);
    void addStringInitializer(Item *item,
            const DomPropertyMap &properties, const QString &name, int column = -1, const QString &directive = QString()) const;
    void addCommonInitializers(Item *item,
                    const DomPropertyMap &properties, int column = -1);

    void initializeMenu(DomWidget *w, const QString &parentWidget);
    void initializeComboBox(DomWidget *w);
    void initializeComboBox3(DomWidget *w);
    void initializeListWidget(DomWidget *w);
    void initializeTreeWidget(DomWidget *w);
    QList<Item *> initializeTreeWidgetItems(const QList<DomItem *> &domItems);
    void initializeTableWidget(DomWidget *w);

    QString disableSorting(DomWidget *w, const QString &varName);
    void enableSorting(DomWidget *w, const QString &varName, const QString &tempName);

//
// special initialization for the Q3 support classes
//
    void initializeQ3ListBox(DomWidget *w);
    void initializeQ3IconView(DomWidget *w);
    void initializeQ3ListView(DomWidget *w);
    void initializeQ3ListViewItems(const QString &className, const QString &varName, const QList<DomItem*> &items);
    void initializeQ3Table(DomWidget *w);
    void initializeQ3TableItems(const QString &className, const QString &varName, const QList<DomItem*> &items);

//
// Sql
//
    void initializeQ3SqlDataTable(DomWidget *w);
    void initializeQ3SqlDataBrowser(DomWidget *w);

    QString findDeclaration(const QString &name);
    DomWidget *findWidget(const QLatin1String &widgetClass);
    DomImage *findImage(const QString &name) const;

    bool isValidObject(const QString &name) const;

private:
    QString writeFontProperties(const DomFont *f);
    QString writeIconProperties(const DomResourceIcon *i);
    QString writeSizePolicy(const DomSizePolicy *sp);
    QString writeBrushInitialization(const DomBrush *brush);
    void addButtonGroup(const DomWidget *node, const QString &varName);
    void addWizardPage(const QString &pageVarName, const DomWidget *page, const QString &parentWidget);

    const Uic *m_uic;
    Driver *m_driver;
    QTextStream &m_output;
    const Option &m_option;
    QString m_indent;
    QString m_dindent;
    bool m_stdsetdef;

    struct Buddy
    {
        Buddy(const QString &oN, const QString &b)
            : objName(oN), buddy(b) {}
        QString objName;
        QString buddy;
    };

    QStack<DomWidget*> m_widgetChain;
    QStack<DomLayout*> m_layoutChain;
    QStack<DomActionGroup*> m_actionGroupChain;
    QList<Buddy> m_buddies;

    QSet<QString> m_buttonGroups;
    QHash<QString, DomWidget*> m_registeredWidgets;
    QHash<QString, DomImage*> m_registeredImages;
    QHash<QString, DomAction*> m_registeredActions;
    typedef QHash<uint, QString> ColorBrushHash;
    ColorBrushHash m_colorBrushHash;
    // Map from font properties to  font variable name for reuse
    // Map from size policy to  variable for reuse
#if defined(Q_OS_MAC) && defined(Q_CC_GNU) && (__GNUC__ == 3 && __GNUC_MINOR__ == 3)
    typedef QHash<FontHandle, QString> FontPropertiesNameMap;
    typedef QHash<IconHandle, QString> IconPropertiesNameMap;
    typedef QHash<SizePolicyHandle, QString> SizePolicyNameMap;
#else
    typedef QMap<FontHandle, QString> FontPropertiesNameMap;
    typedef QMap<IconHandle, QString> IconPropertiesNameMap;
    typedef QMap<SizePolicyHandle, QString> SizePolicyNameMap;
#endif
    FontPropertiesNameMap m_fontPropertiesNameMap;
    IconPropertiesNameMap m_iconPropertiesNameMap;
    SizePolicyNameMap     m_sizePolicyNameMap;

    class LayoutDefaultHandler {
    public:
        LayoutDefaultHandler();
        void acceptLayoutDefault(DomLayoutDefault *node);
        void acceptLayoutFunction(DomLayoutFunction *node);

        // Write out the layout margin and spacing properties applying the defaults.
        void writeProperties(const QString &indent, const QString &varName,
                             const DomPropertyMap &pm, int marginType,
                             bool suppressMarginDefault, QTextStream &str) const;
    private:
        void writeProperty(int p, const QString &indent, const QString &objectName, const DomPropertyMap &pm,
                           const QString &propertyName, const QString &setter, int defaultStyleValue,
                           bool suppressDefault, QTextStream &str) const;

        enum Properties { Margin, Spacing, NumProperties };
        enum StateFlags { HasDefaultValue = 1, HasDefaultFunction = 2};
        unsigned m_state[NumProperties];
        int m_defaultValues[NumProperties];
        QString m_functions[NumProperties];
    };

    // layout defaults
    LayoutDefaultHandler m_LayoutDefaultHandler;
    int m_layoutMarginType;

    QString m_generatedClass;
    QString m_mainFormVarName;
    bool m_mainFormUsedInRetranslateUi;

    QString m_delayedInitialization;
    QTextStream m_delayedOut;

    QString m_refreshInitialization;
    QTextStream m_refreshOut;

    QString m_delayedActionInitialization;
    QTextStream m_actionOut;
    const bool m_activateScripts;

    bool m_layoutWidget;
    bool m_firstThemeIcon;
};

} // namespace CPP

QT_END_NAMESPACE

#endif // CPPWRITEINITIALIZATION_H

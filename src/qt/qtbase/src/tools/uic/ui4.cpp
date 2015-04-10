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
#include "ui4.h"


QT_BEGIN_NAMESPACE
#ifdef QFORMINTERNAL_NAMESPACE
using namespace QFormInternal;
#endif

/*******************************************************************************
** Implementations
*/

void DomUI::clear(bool clear_all)
{
    delete m_widget;
    delete m_layoutDefault;
    delete m_layoutFunction;
    delete m_customWidgets;
    delete m_tabStops;
    delete m_images;
    delete m_includes;
    delete m_resources;
    delete m_connections;
    delete m_designerdata;
    delete m_slots;
    delete m_buttonGroups;

    if (clear_all) {
    m_text.clear();
    m_has_attr_version = false;
    m_has_attr_language = false;
    m_has_attr_displayname = false;
    m_has_attr_stdsetdef = false;
    m_attr_stdsetdef = 0;
    m_has_attr_stdSetDef = false;
    m_attr_stdSetDef = 0;
    }

    m_children = 0;
    m_widget = 0;
    m_layoutDefault = 0;
    m_layoutFunction = 0;
    m_customWidgets = 0;
    m_tabStops = 0;
    m_images = 0;
    m_includes = 0;
    m_resources = 0;
    m_connections = 0;
    m_designerdata = 0;
    m_slots = 0;
    m_buttonGroups = 0;
}

DomUI::DomUI()
{
    m_children = 0;
    m_has_attr_version = false;
    m_has_attr_language = false;
    m_has_attr_displayname = false;
    m_has_attr_stdsetdef = false;
    m_attr_stdsetdef = 0;
    m_has_attr_stdSetDef = false;
    m_attr_stdSetDef = 0;
    m_widget = 0;
    m_layoutDefault = 0;
    m_layoutFunction = 0;
    m_customWidgets = 0;
    m_tabStops = 0;
    m_images = 0;
    m_includes = 0;
    m_resources = 0;
    m_connections = 0;
    m_designerdata = 0;
    m_slots = 0;
    m_buttonGroups = 0;
}

DomUI::~DomUI()
{
    delete m_widget;
    delete m_layoutDefault;
    delete m_layoutFunction;
    delete m_customWidgets;
    delete m_tabStops;
    delete m_images;
    delete m_includes;
    delete m_resources;
    delete m_connections;
    delete m_designerdata;
    delete m_slots;
    delete m_buttonGroups;
}

void DomUI::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("version")) {
            setAttributeVersion(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("language")) {
            setAttributeLanguage(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("displayname")) {
            setAttributeDisplayname(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("stdsetdef")) {
            setAttributeStdsetdef(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("stdSetDef")) {
            setAttributeStdSetDef(attribute.value().toString().toInt());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("author")) {
                setElementAuthor(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("comment")) {
                setElementComment(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("exportmacro")) {
                setElementExportMacro(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("class")) {
                setElementClass(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("widget")) {
                DomWidget *v = new DomWidget();
                v->read(reader);
                setElementWidget(v);
                continue;
            }
            if (tag == QStringLiteral("layoutdefault")) {
                DomLayoutDefault *v = new DomLayoutDefault();
                v->read(reader);
                setElementLayoutDefault(v);
                continue;
            }
            if (tag == QStringLiteral("layoutfunction")) {
                DomLayoutFunction *v = new DomLayoutFunction();
                v->read(reader);
                setElementLayoutFunction(v);
                continue;
            }
            if (tag == QStringLiteral("pixmapfunction")) {
                setElementPixmapFunction(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("customwidgets")) {
                DomCustomWidgets *v = new DomCustomWidgets();
                v->read(reader);
                setElementCustomWidgets(v);
                continue;
            }
            if (tag == QStringLiteral("tabstops")) {
                DomTabStops *v = new DomTabStops();
                v->read(reader);
                setElementTabStops(v);
                continue;
            }
            if (tag == QStringLiteral("images")) {
                DomImages *v = new DomImages();
                v->read(reader);
                setElementImages(v);
                continue;
            }
            if (tag == QStringLiteral("includes")) {
                DomIncludes *v = new DomIncludes();
                v->read(reader);
                setElementIncludes(v);
                continue;
            }
            if (tag == QStringLiteral("resources")) {
                DomResources *v = new DomResources();
                v->read(reader);
                setElementResources(v);
                continue;
            }
            if (tag == QStringLiteral("connections")) {
                DomConnections *v = new DomConnections();
                v->read(reader);
                setElementConnections(v);
                continue;
            }
            if (tag == QStringLiteral("designerdata")) {
                DomDesignerData *v = new DomDesignerData();
                v->read(reader);
                setElementDesignerdata(v);
                continue;
            }
            if (tag == QStringLiteral("slots")) {
                DomSlots *v = new DomSlots();
                v->read(reader);
                setElementSlots(v);
                continue;
            }
            if (tag == QStringLiteral("buttongroups")) {
                DomButtonGroups *v = new DomButtonGroups();
                v->read(reader);
                setElementButtonGroups(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomUI::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("ui") : tagName.toLower());

    if (hasAttributeVersion())
        writer.writeAttribute(QStringLiteral("version"), attributeVersion());

    if (hasAttributeLanguage())
        writer.writeAttribute(QStringLiteral("language"), attributeLanguage());

    if (hasAttributeDisplayname())
        writer.writeAttribute(QStringLiteral("displayname"), attributeDisplayname());

    if (hasAttributeStdsetdef())
        writer.writeAttribute(QStringLiteral("stdsetdef"), QString::number(attributeStdsetdef()));

    if (hasAttributeStdSetDef())
        writer.writeAttribute(QStringLiteral("stdsetdef"), QString::number(attributeStdSetDef()));

    if (m_children & Author) {
        writer.writeTextElement(QStringLiteral("author"), m_author);
    }

    if (m_children & Comment) {
        writer.writeTextElement(QStringLiteral("comment"), m_comment);
    }

    if (m_children & ExportMacro) {
        writer.writeTextElement(QStringLiteral("exportmacro"), m_exportMacro);
    }

    if (m_children & Class) {
        writer.writeTextElement(QStringLiteral("class"), m_class);
    }

    if (m_children & Widget) {
        m_widget->write(writer, QStringLiteral("widget"));
    }

    if (m_children & LayoutDefault) {
        m_layoutDefault->write(writer, QStringLiteral("layoutdefault"));
    }

    if (m_children & LayoutFunction) {
        m_layoutFunction->write(writer, QStringLiteral("layoutfunction"));
    }

    if (m_children & PixmapFunction) {
        writer.writeTextElement(QStringLiteral("pixmapfunction"), m_pixmapFunction);
    }

    if (m_children & CustomWidgets) {
        m_customWidgets->write(writer, QStringLiteral("customwidgets"));
    }

    if (m_children & TabStops) {
        m_tabStops->write(writer, QStringLiteral("tabstops"));
    }

    if (m_children & Images) {
        m_images->write(writer, QStringLiteral("images"));
    }

    if (m_children & Includes) {
        m_includes->write(writer, QStringLiteral("includes"));
    }

    if (m_children & Resources) {
        m_resources->write(writer, QStringLiteral("resources"));
    }

    if (m_children & Connections) {
        m_connections->write(writer, QStringLiteral("connections"));
    }

    if (m_children & Designerdata) {
        m_designerdata->write(writer, QStringLiteral("designerdata"));
    }

    if (m_children & Slots) {
        m_slots->write(writer, QStringLiteral("slots"));
    }

    if (m_children & ButtonGroups) {
        m_buttonGroups->write(writer, QStringLiteral("buttongroups"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomUI::setElementAuthor(const QString& a)
{
    m_children |= Author;
    m_author = a;
}

void DomUI::setElementComment(const QString& a)
{
    m_children |= Comment;
    m_comment = a;
}

void DomUI::setElementExportMacro(const QString& a)
{
    m_children |= ExportMacro;
    m_exportMacro = a;
}

void DomUI::setElementClass(const QString& a)
{
    m_children |= Class;
    m_class = a;
}

DomWidget* DomUI::takeElementWidget() 
{
    DomWidget* a = m_widget;
    m_widget = 0;
    m_children ^= Widget;
    return a;
}

void DomUI::setElementWidget(DomWidget* a)
{
    delete m_widget;
    m_children |= Widget;
    m_widget = a;
}

DomLayoutDefault* DomUI::takeElementLayoutDefault() 
{
    DomLayoutDefault* a = m_layoutDefault;
    m_layoutDefault = 0;
    m_children ^= LayoutDefault;
    return a;
}

void DomUI::setElementLayoutDefault(DomLayoutDefault* a)
{
    delete m_layoutDefault;
    m_children |= LayoutDefault;
    m_layoutDefault = a;
}

DomLayoutFunction* DomUI::takeElementLayoutFunction() 
{
    DomLayoutFunction* a = m_layoutFunction;
    m_layoutFunction = 0;
    m_children ^= LayoutFunction;
    return a;
}

void DomUI::setElementLayoutFunction(DomLayoutFunction* a)
{
    delete m_layoutFunction;
    m_children |= LayoutFunction;
    m_layoutFunction = a;
}

void DomUI::setElementPixmapFunction(const QString& a)
{
    m_children |= PixmapFunction;
    m_pixmapFunction = a;
}

DomCustomWidgets* DomUI::takeElementCustomWidgets() 
{
    DomCustomWidgets* a = m_customWidgets;
    m_customWidgets = 0;
    m_children ^= CustomWidgets;
    return a;
}

void DomUI::setElementCustomWidgets(DomCustomWidgets* a)
{
    delete m_customWidgets;
    m_children |= CustomWidgets;
    m_customWidgets = a;
}

DomTabStops* DomUI::takeElementTabStops() 
{
    DomTabStops* a = m_tabStops;
    m_tabStops = 0;
    m_children ^= TabStops;
    return a;
}

void DomUI::setElementTabStops(DomTabStops* a)
{
    delete m_tabStops;
    m_children |= TabStops;
    m_tabStops = a;
}

DomImages* DomUI::takeElementImages() 
{
    DomImages* a = m_images;
    m_images = 0;
    m_children ^= Images;
    return a;
}

void DomUI::setElementImages(DomImages* a)
{
    delete m_images;
    m_children |= Images;
    m_images = a;
}

DomIncludes* DomUI::takeElementIncludes() 
{
    DomIncludes* a = m_includes;
    m_includes = 0;
    m_children ^= Includes;
    return a;
}

void DomUI::setElementIncludes(DomIncludes* a)
{
    delete m_includes;
    m_children |= Includes;
    m_includes = a;
}

DomResources* DomUI::takeElementResources() 
{
    DomResources* a = m_resources;
    m_resources = 0;
    m_children ^= Resources;
    return a;
}

void DomUI::setElementResources(DomResources* a)
{
    delete m_resources;
    m_children |= Resources;
    m_resources = a;
}

DomConnections* DomUI::takeElementConnections() 
{
    DomConnections* a = m_connections;
    m_connections = 0;
    m_children ^= Connections;
    return a;
}

void DomUI::setElementConnections(DomConnections* a)
{
    delete m_connections;
    m_children |= Connections;
    m_connections = a;
}

DomDesignerData* DomUI::takeElementDesignerdata() 
{
    DomDesignerData* a = m_designerdata;
    m_designerdata = 0;
    m_children ^= Designerdata;
    return a;
}

void DomUI::setElementDesignerdata(DomDesignerData* a)
{
    delete m_designerdata;
    m_children |= Designerdata;
    m_designerdata = a;
}

DomSlots* DomUI::takeElementSlots() 
{
    DomSlots* a = m_slots;
    m_slots = 0;
    m_children ^= Slots;
    return a;
}

void DomUI::setElementSlots(DomSlots* a)
{
    delete m_slots;
    m_children |= Slots;
    m_slots = a;
}

DomButtonGroups* DomUI::takeElementButtonGroups() 
{
    DomButtonGroups* a = m_buttonGroups;
    m_buttonGroups = 0;
    m_children ^= ButtonGroups;
    return a;
}

void DomUI::setElementButtonGroups(DomButtonGroups* a)
{
    delete m_buttonGroups;
    m_children |= ButtonGroups;
    m_buttonGroups = a;
}

void DomUI::clearElementAuthor()
{
    m_children &= ~Author;
}

void DomUI::clearElementComment()
{
    m_children &= ~Comment;
}

void DomUI::clearElementExportMacro()
{
    m_children &= ~ExportMacro;
}

void DomUI::clearElementClass()
{
    m_children &= ~Class;
}

void DomUI::clearElementWidget()
{
    delete m_widget;
    m_widget = 0;
    m_children &= ~Widget;
}

void DomUI::clearElementLayoutDefault()
{
    delete m_layoutDefault;
    m_layoutDefault = 0;
    m_children &= ~LayoutDefault;
}

void DomUI::clearElementLayoutFunction()
{
    delete m_layoutFunction;
    m_layoutFunction = 0;
    m_children &= ~LayoutFunction;
}

void DomUI::clearElementPixmapFunction()
{
    m_children &= ~PixmapFunction;
}

void DomUI::clearElementCustomWidgets()
{
    delete m_customWidgets;
    m_customWidgets = 0;
    m_children &= ~CustomWidgets;
}

void DomUI::clearElementTabStops()
{
    delete m_tabStops;
    m_tabStops = 0;
    m_children &= ~TabStops;
}

void DomUI::clearElementImages()
{
    delete m_images;
    m_images = 0;
    m_children &= ~Images;
}

void DomUI::clearElementIncludes()
{
    delete m_includes;
    m_includes = 0;
    m_children &= ~Includes;
}

void DomUI::clearElementResources()
{
    delete m_resources;
    m_resources = 0;
    m_children &= ~Resources;
}

void DomUI::clearElementConnections()
{
    delete m_connections;
    m_connections = 0;
    m_children &= ~Connections;
}

void DomUI::clearElementDesignerdata()
{
    delete m_designerdata;
    m_designerdata = 0;
    m_children &= ~Designerdata;
}

void DomUI::clearElementSlots()
{
    delete m_slots;
    m_slots = 0;
    m_children &= ~Slots;
}

void DomUI::clearElementButtonGroups()
{
    delete m_buttonGroups;
    m_buttonGroups = 0;
    m_children &= ~ButtonGroups;
}

void DomIncludes::clear(bool clear_all)
{
    qDeleteAll(m_include);
    m_include.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomIncludes::DomIncludes()
{
    m_children = 0;
}

DomIncludes::~DomIncludes()
{
    qDeleteAll(m_include);
    m_include.clear();
}

void DomIncludes::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("include")) {
                DomInclude *v = new DomInclude();
                v->read(reader);
                m_include.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomIncludes::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("includes") : tagName.toLower());

    for (int i = 0; i < m_include.size(); ++i) {
        DomInclude* v = m_include[i];
        v->write(writer, QStringLiteral("include"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomIncludes::setElementInclude(const QList<DomInclude*>& a)
{
    m_children |= Include;
    m_include = a;
}

void DomInclude::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_location = false;
    m_has_attr_impldecl = false;
    }

    m_children = 0;
}

DomInclude::DomInclude()
{
    m_children = 0;
    m_has_attr_location = false;
    m_has_attr_impldecl = false;
    m_text.clear();
}

DomInclude::~DomInclude()
{
}

void DomInclude::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("location")) {
            setAttributeLocation(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("impldecl")) {
            setAttributeImpldecl(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomInclude::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("include") : tagName.toLower());

    if (hasAttributeLocation())
        writer.writeAttribute(QStringLiteral("location"), attributeLocation());

    if (hasAttributeImpldecl())
        writer.writeAttribute(QStringLiteral("impldecl"), attributeImpldecl());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomResources::clear(bool clear_all)
{
    qDeleteAll(m_include);
    m_include.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    }

    m_children = 0;
}

DomResources::DomResources()
{
    m_children = 0;
    m_has_attr_name = false;
}

DomResources::~DomResources()
{
    qDeleteAll(m_include);
    m_include.clear();
}

void DomResources::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("include")) {
                DomResource *v = new DomResource();
                v->read(reader);
                m_include.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomResources::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("resources") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    for (int i = 0; i < m_include.size(); ++i) {
        DomResource* v = m_include[i];
        v->write(writer, QStringLiteral("include"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomResources::setElementInclude(const QList<DomResource*>& a)
{
    m_children |= Include;
    m_include = a;
}

void DomResource::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_location = false;
    }

    m_children = 0;
}

DomResource::DomResource()
{
    m_children = 0;
    m_has_attr_location = false;
}

DomResource::~DomResource()
{
}

void DomResource::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("location")) {
            setAttributeLocation(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomResource::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("resource") : tagName.toLower());

    if (hasAttributeLocation())
        writer.writeAttribute(QStringLiteral("location"), attributeLocation());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomActionGroup::clear(bool clear_all)
{
    qDeleteAll(m_action);
    m_action.clear();
    qDeleteAll(m_actionGroup);
    m_actionGroup.clear();
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    }

    m_children = 0;
}

DomActionGroup::DomActionGroup()
{
    m_children = 0;
    m_has_attr_name = false;
}

DomActionGroup::~DomActionGroup()
{
    qDeleteAll(m_action);
    m_action.clear();
    qDeleteAll(m_actionGroup);
    m_actionGroup.clear();
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
}

void DomActionGroup::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("action")) {
                DomAction *v = new DomAction();
                v->read(reader);
                m_action.append(v);
                continue;
            }
            if (tag == QStringLiteral("actiongroup")) {
                DomActionGroup *v = new DomActionGroup();
                v->read(reader);
                m_actionGroup.append(v);
                continue;
            }
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            if (tag == QStringLiteral("attribute")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_attribute.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomActionGroup::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("actiongroup") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    for (int i = 0; i < m_action.size(); ++i) {
        DomAction* v = m_action[i];
        v->write(writer, QStringLiteral("action"));
    }
    for (int i = 0; i < m_actionGroup.size(); ++i) {
        DomActionGroup* v = m_actionGroup[i];
        v->write(writer, QStringLiteral("actiongroup"));
    }
    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    for (int i = 0; i < m_attribute.size(); ++i) {
        DomProperty* v = m_attribute[i];
        v->write(writer, QStringLiteral("attribute"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomActionGroup::setElementAction(const QList<DomAction*>& a)
{
    m_children |= Action;
    m_action = a;
}

void DomActionGroup::setElementActionGroup(const QList<DomActionGroup*>& a)
{
    m_children |= ActionGroup;
    m_actionGroup = a;
}

void DomActionGroup::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomActionGroup::setElementAttribute(const QList<DomProperty*>& a)
{
    m_children |= Attribute;
    m_attribute = a;
}

void DomAction::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    m_has_attr_menu = false;
    }

    m_children = 0;
}

DomAction::DomAction()
{
    m_children = 0;
    m_has_attr_name = false;
    m_has_attr_menu = false;
}

DomAction::~DomAction()
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
}

void DomAction::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("menu")) {
            setAttributeMenu(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            if (tag == QStringLiteral("attribute")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_attribute.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomAction::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("action") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (hasAttributeMenu())
        writer.writeAttribute(QStringLiteral("menu"), attributeMenu());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    for (int i = 0; i < m_attribute.size(); ++i) {
        DomProperty* v = m_attribute[i];
        v->write(writer, QStringLiteral("attribute"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomAction::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomAction::setElementAttribute(const QList<DomProperty*>& a)
{
    m_children |= Attribute;
    m_attribute = a;
}

void DomActionRef::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    }

    m_children = 0;
}

DomActionRef::DomActionRef()
{
    m_children = 0;
    m_has_attr_name = false;
}

DomActionRef::~DomActionRef()
{
}

void DomActionRef::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomActionRef::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("actionref") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomButtonGroup::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    }

    m_children = 0;
}

DomButtonGroup::DomButtonGroup()
{
    m_children = 0;
    m_has_attr_name = false;
}

DomButtonGroup::~DomButtonGroup()
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
}

void DomButtonGroup::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            if (tag == QStringLiteral("attribute")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_attribute.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomButtonGroup::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("buttongroup") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    for (int i = 0; i < m_attribute.size(); ++i) {
        DomProperty* v = m_attribute[i];
        v->write(writer, QStringLiteral("attribute"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomButtonGroup::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomButtonGroup::setElementAttribute(const QList<DomProperty*>& a)
{
    m_children |= Attribute;
    m_attribute = a;
}

void DomButtonGroups::clear(bool clear_all)
{
    qDeleteAll(m_buttonGroup);
    m_buttonGroup.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomButtonGroups::DomButtonGroups()
{
    m_children = 0;
}

DomButtonGroups::~DomButtonGroups()
{
    qDeleteAll(m_buttonGroup);
    m_buttonGroup.clear();
}

void DomButtonGroups::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("buttongroup")) {
                DomButtonGroup *v = new DomButtonGroup();
                v->read(reader);
                m_buttonGroup.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomButtonGroups::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("buttongroups") : tagName.toLower());

    for (int i = 0; i < m_buttonGroup.size(); ++i) {
        DomButtonGroup* v = m_buttonGroup[i];
        v->write(writer, QStringLiteral("buttongroup"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomButtonGroups::setElementButtonGroup(const QList<DomButtonGroup*>& a)
{
    m_children |= ButtonGroup;
    m_buttonGroup = a;
}

void DomImages::clear(bool clear_all)
{
    qDeleteAll(m_image);
    m_image.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomImages::DomImages()
{
    m_children = 0;
}

DomImages::~DomImages()
{
    qDeleteAll(m_image);
    m_image.clear();
}

void DomImages::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("image")) {
                DomImage *v = new DomImage();
                v->read(reader);
                m_image.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomImages::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("images") : tagName.toLower());

    for (int i = 0; i < m_image.size(); ++i) {
        DomImage* v = m_image[i];
        v->write(writer, QStringLiteral("image"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomImages::setElementImage(const QList<DomImage*>& a)
{
    m_children |= Image;
    m_image = a;
}

void DomImage::clear(bool clear_all)
{
    delete m_data;

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    }

    m_children = 0;
    m_data = 0;
}

DomImage::DomImage()
{
    m_children = 0;
    m_has_attr_name = false;
    m_data = 0;
}

DomImage::~DomImage()
{
    delete m_data;
}

void DomImage::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("data")) {
                DomImageData *v = new DomImageData();
                v->read(reader);
                setElementData(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomImage::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("image") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (m_children & Data) {
        m_data->write(writer, QStringLiteral("data"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomImageData* DomImage::takeElementData() 
{
    DomImageData* a = m_data;
    m_data = 0;
    m_children ^= Data;
    return a;
}

void DomImage::setElementData(DomImageData* a)
{
    delete m_data;
    m_children |= Data;
    m_data = a;
}

void DomImage::clearElementData()
{
    delete m_data;
    m_data = 0;
    m_children &= ~Data;
}

void DomImageData::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_format = false;
    m_has_attr_length = false;
    m_attr_length = 0;
    }

    m_children = 0;
}

DomImageData::DomImageData()
{
    m_children = 0;
    m_has_attr_format = false;
    m_has_attr_length = false;
    m_attr_length = 0;
    m_text.clear();
}

DomImageData::~DomImageData()
{
}

void DomImageData::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("format")) {
            setAttributeFormat(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("length")) {
            setAttributeLength(attribute.value().toString().toInt());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomImageData::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("imagedata") : tagName.toLower());

    if (hasAttributeFormat())
        writer.writeAttribute(QStringLiteral("format"), attributeFormat());

    if (hasAttributeLength())
        writer.writeAttribute(QStringLiteral("length"), QString::number(attributeLength()));

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomCustomWidgets::clear(bool clear_all)
{
    qDeleteAll(m_customWidget);
    m_customWidget.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomCustomWidgets::DomCustomWidgets()
{
    m_children = 0;
}

DomCustomWidgets::~DomCustomWidgets()
{
    qDeleteAll(m_customWidget);
    m_customWidget.clear();
}

void DomCustomWidgets::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("customwidget")) {
                DomCustomWidget *v = new DomCustomWidget();
                v->read(reader);
                m_customWidget.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomCustomWidgets::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("customwidgets") : tagName.toLower());

    for (int i = 0; i < m_customWidget.size(); ++i) {
        DomCustomWidget* v = m_customWidget[i];
        v->write(writer, QStringLiteral("customwidget"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomCustomWidgets::setElementCustomWidget(const QList<DomCustomWidget*>& a)
{
    m_children |= CustomWidget;
    m_customWidget = a;
}

void DomHeader::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_location = false;
    }

    m_children = 0;
}

DomHeader::DomHeader()
{
    m_children = 0;
    m_has_attr_location = false;
    m_text.clear();
}

DomHeader::~DomHeader()
{
}

void DomHeader::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("location")) {
            setAttributeLocation(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomHeader::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("header") : tagName.toLower());

    if (hasAttributeLocation())
        writer.writeAttribute(QStringLiteral("location"), attributeLocation());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomCustomWidget::clear(bool clear_all)
{
    delete m_header;
    delete m_sizeHint;
    delete m_sizePolicy;
    delete m_script;
    delete m_properties;
    delete m_slots;
    delete m_propertyspecifications;

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_header = 0;
    m_sizeHint = 0;
    m_container = 0;
    m_sizePolicy = 0;
    m_script = 0;
    m_properties = 0;
    m_slots = 0;
    m_propertyspecifications = 0;
}

DomCustomWidget::DomCustomWidget()
{
    m_children = 0;
    m_header = 0;
    m_sizeHint = 0;
    m_container = 0;
    m_sizePolicy = 0;
    m_script = 0;
    m_properties = 0;
    m_slots = 0;
    m_propertyspecifications = 0;
}

DomCustomWidget::~DomCustomWidget()
{
    delete m_header;
    delete m_sizeHint;
    delete m_sizePolicy;
    delete m_script;
    delete m_properties;
    delete m_slots;
    delete m_propertyspecifications;
}

void DomCustomWidget::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("class")) {
                setElementClass(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("extends")) {
                setElementExtends(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("header")) {
                DomHeader *v = new DomHeader();
                v->read(reader);
                setElementHeader(v);
                continue;
            }
            if (tag == QStringLiteral("sizehint")) {
                DomSize *v = new DomSize();
                v->read(reader);
                setElementSizeHint(v);
                continue;
            }
            if (tag == QStringLiteral("addpagemethod")) {
                setElementAddPageMethod(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("container")) {
                setElementContainer(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("sizepolicy")) {
                DomSizePolicyData *v = new DomSizePolicyData();
                v->read(reader);
                setElementSizePolicy(v);
                continue;
            }
            if (tag == QStringLiteral("pixmap")) {
                setElementPixmap(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("script")) {
                DomScript *v = new DomScript();
                v->read(reader);
                setElementScript(v);
                continue;
            }
            if (tag == QStringLiteral("properties")) {
                DomProperties *v = new DomProperties();
                v->read(reader);
                setElementProperties(v);
                continue;
            }
            if (tag == QStringLiteral("slots")) {
                DomSlots *v = new DomSlots();
                v->read(reader);
                setElementSlots(v);
                continue;
            }
            if (tag == QStringLiteral("propertyspecifications")) {
                DomPropertySpecifications *v = new DomPropertySpecifications();
                v->read(reader);
                setElementPropertyspecifications(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomCustomWidget::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("customwidget") : tagName.toLower());

    if (m_children & Class) {
        writer.writeTextElement(QStringLiteral("class"), m_class);
    }

    if (m_children & Extends) {
        writer.writeTextElement(QStringLiteral("extends"), m_extends);
    }

    if (m_children & Header) {
        m_header->write(writer, QStringLiteral("header"));
    }

    if (m_children & SizeHint) {
        m_sizeHint->write(writer, QStringLiteral("sizehint"));
    }

    if (m_children & AddPageMethod) {
        writer.writeTextElement(QStringLiteral("addpagemethod"), m_addPageMethod);
    }

    if (m_children & Container) {
        writer.writeTextElement(QStringLiteral("container"), QString::number(m_container));
    }

    if (m_children & SizePolicy) {
        m_sizePolicy->write(writer, QStringLiteral("sizepolicy"));
    }

    if (m_children & Pixmap) {
        writer.writeTextElement(QStringLiteral("pixmap"), m_pixmap);
    }

    if (m_children & Script) {
        m_script->write(writer, QStringLiteral("script"));
    }

    if (m_children & Properties) {
        m_properties->write(writer, QStringLiteral("properties"));
    }

    if (m_children & Slots) {
        m_slots->write(writer, QStringLiteral("slots"));
    }

    if (m_children & Propertyspecifications) {
        m_propertyspecifications->write(writer, QStringLiteral("propertyspecifications"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomCustomWidget::setElementClass(const QString& a)
{
    m_children |= Class;
    m_class = a;
}

void DomCustomWidget::setElementExtends(const QString& a)
{
    m_children |= Extends;
    m_extends = a;
}

DomHeader* DomCustomWidget::takeElementHeader() 
{
    DomHeader* a = m_header;
    m_header = 0;
    m_children ^= Header;
    return a;
}

void DomCustomWidget::setElementHeader(DomHeader* a)
{
    delete m_header;
    m_children |= Header;
    m_header = a;
}

DomSize* DomCustomWidget::takeElementSizeHint() 
{
    DomSize* a = m_sizeHint;
    m_sizeHint = 0;
    m_children ^= SizeHint;
    return a;
}

void DomCustomWidget::setElementSizeHint(DomSize* a)
{
    delete m_sizeHint;
    m_children |= SizeHint;
    m_sizeHint = a;
}

void DomCustomWidget::setElementAddPageMethod(const QString& a)
{
    m_children |= AddPageMethod;
    m_addPageMethod = a;
}

void DomCustomWidget::setElementContainer(int a)
{
    m_children |= Container;
    m_container = a;
}

DomSizePolicyData* DomCustomWidget::takeElementSizePolicy() 
{
    DomSizePolicyData* a = m_sizePolicy;
    m_sizePolicy = 0;
    m_children ^= SizePolicy;
    return a;
}

void DomCustomWidget::setElementSizePolicy(DomSizePolicyData* a)
{
    delete m_sizePolicy;
    m_children |= SizePolicy;
    m_sizePolicy = a;
}

void DomCustomWidget::setElementPixmap(const QString& a)
{
    m_children |= Pixmap;
    m_pixmap = a;
}

DomScript* DomCustomWidget::takeElementScript() 
{
    DomScript* a = m_script;
    m_script = 0;
    m_children ^= Script;
    return a;
}

void DomCustomWidget::setElementScript(DomScript* a)
{
    delete m_script;
    m_children |= Script;
    m_script = a;
}

DomProperties* DomCustomWidget::takeElementProperties() 
{
    DomProperties* a = m_properties;
    m_properties = 0;
    m_children ^= Properties;
    return a;
}

void DomCustomWidget::setElementProperties(DomProperties* a)
{
    delete m_properties;
    m_children |= Properties;
    m_properties = a;
}

DomSlots* DomCustomWidget::takeElementSlots() 
{
    DomSlots* a = m_slots;
    m_slots = 0;
    m_children ^= Slots;
    return a;
}

void DomCustomWidget::setElementSlots(DomSlots* a)
{
    delete m_slots;
    m_children |= Slots;
    m_slots = a;
}

DomPropertySpecifications* DomCustomWidget::takeElementPropertyspecifications() 
{
    DomPropertySpecifications* a = m_propertyspecifications;
    m_propertyspecifications = 0;
    m_children ^= Propertyspecifications;
    return a;
}

void DomCustomWidget::setElementPropertyspecifications(DomPropertySpecifications* a)
{
    delete m_propertyspecifications;
    m_children |= Propertyspecifications;
    m_propertyspecifications = a;
}

void DomCustomWidget::clearElementClass()
{
    m_children &= ~Class;
}

void DomCustomWidget::clearElementExtends()
{
    m_children &= ~Extends;
}

void DomCustomWidget::clearElementHeader()
{
    delete m_header;
    m_header = 0;
    m_children &= ~Header;
}

void DomCustomWidget::clearElementSizeHint()
{
    delete m_sizeHint;
    m_sizeHint = 0;
    m_children &= ~SizeHint;
}

void DomCustomWidget::clearElementAddPageMethod()
{
    m_children &= ~AddPageMethod;
}

void DomCustomWidget::clearElementContainer()
{
    m_children &= ~Container;
}

void DomCustomWidget::clearElementSizePolicy()
{
    delete m_sizePolicy;
    m_sizePolicy = 0;
    m_children &= ~SizePolicy;
}

void DomCustomWidget::clearElementPixmap()
{
    m_children &= ~Pixmap;
}

void DomCustomWidget::clearElementScript()
{
    delete m_script;
    m_script = 0;
    m_children &= ~Script;
}

void DomCustomWidget::clearElementProperties()
{
    delete m_properties;
    m_properties = 0;
    m_children &= ~Properties;
}

void DomCustomWidget::clearElementSlots()
{
    delete m_slots;
    m_slots = 0;
    m_children &= ~Slots;
}

void DomCustomWidget::clearElementPropertyspecifications()
{
    delete m_propertyspecifications;
    m_propertyspecifications = 0;
    m_children &= ~Propertyspecifications;
}

void DomProperties::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomProperties::DomProperties()
{
    m_children = 0;
}

DomProperties::~DomProperties()
{
    qDeleteAll(m_property);
    m_property.clear();
}

void DomProperties::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomPropertyData *v = new DomPropertyData();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomProperties::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("properties") : tagName.toLower());

    for (int i = 0; i < m_property.size(); ++i) {
        DomPropertyData* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomProperties::setElementProperty(const QList<DomPropertyData*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomPropertyData::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_type = false;
    }

    m_children = 0;
}

DomPropertyData::DomPropertyData()
{
    m_children = 0;
    m_has_attr_type = false;
}

DomPropertyData::~DomPropertyData()
{
}

void DomPropertyData::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("type")) {
            setAttributeType(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomPropertyData::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("propertydata") : tagName.toLower());

    if (hasAttributeType())
        writer.writeAttribute(QStringLiteral("type"), attributeType());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSizePolicyData::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_horData = 0;
    m_verData = 0;
}

DomSizePolicyData::DomSizePolicyData()
{
    m_children = 0;
    m_horData = 0;
    m_verData = 0;
}

DomSizePolicyData::~DomSizePolicyData()
{
}

void DomSizePolicyData::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("hordata")) {
                setElementHorData(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("verdata")) {
                setElementVerData(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomSizePolicyData::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("sizepolicydata") : tagName.toLower());

    if (m_children & HorData) {
        writer.writeTextElement(QStringLiteral("hordata"), QString::number(m_horData));
    }

    if (m_children & VerData) {
        writer.writeTextElement(QStringLiteral("verdata"), QString::number(m_verData));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSizePolicyData::setElementHorData(int a)
{
    m_children |= HorData;
    m_horData = a;
}

void DomSizePolicyData::setElementVerData(int a)
{
    m_children |= VerData;
    m_verData = a;
}

void DomSizePolicyData::clearElementHorData()
{
    m_children &= ~HorData;
}

void DomSizePolicyData::clearElementVerData()
{
    m_children &= ~VerData;
}

void DomLayoutDefault::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_spacing = false;
    m_attr_spacing = 0;
    m_has_attr_margin = false;
    m_attr_margin = 0;
    }

    m_children = 0;
}

DomLayoutDefault::DomLayoutDefault()
{
    m_children = 0;
    m_has_attr_spacing = false;
    m_attr_spacing = 0;
    m_has_attr_margin = false;
    m_attr_margin = 0;
}

DomLayoutDefault::~DomLayoutDefault()
{
}

void DomLayoutDefault::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("spacing")) {
            setAttributeSpacing(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("margin")) {
            setAttributeMargin(attribute.value().toString().toInt());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomLayoutDefault::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("layoutdefault") : tagName.toLower());

    if (hasAttributeSpacing())
        writer.writeAttribute(QStringLiteral("spacing"), QString::number(attributeSpacing()));

    if (hasAttributeMargin())
        writer.writeAttribute(QStringLiteral("margin"), QString::number(attributeMargin()));

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomLayoutFunction::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_spacing = false;
    m_has_attr_margin = false;
    }

    m_children = 0;
}

DomLayoutFunction::DomLayoutFunction()
{
    m_children = 0;
    m_has_attr_spacing = false;
    m_has_attr_margin = false;
}

DomLayoutFunction::~DomLayoutFunction()
{
}

void DomLayoutFunction::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("spacing")) {
            setAttributeSpacing(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("margin")) {
            setAttributeMargin(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomLayoutFunction::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("layoutfunction") : tagName.toLower());

    if (hasAttributeSpacing())
        writer.writeAttribute(QStringLiteral("spacing"), attributeSpacing());

    if (hasAttributeMargin())
        writer.writeAttribute(QStringLiteral("margin"), attributeMargin());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomTabStops::clear(bool clear_all)
{
    m_tabStop.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomTabStops::DomTabStops()
{
    m_children = 0;
}

DomTabStops::~DomTabStops()
{
    m_tabStop.clear();
}

void DomTabStops::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("tabstop")) {
                m_tabStop.append(reader.readElementText());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomTabStops::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("tabstops") : tagName.toLower());

    for (int i = 0; i < m_tabStop.size(); ++i) {
        QString v = m_tabStop[i];
        writer.writeTextElement(QStringLiteral("tabstop"), v);
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomTabStops::setElementTabStop(const QStringList& a)
{
    m_children |= TabStop;
    m_tabStop = a;
}

void DomLayout::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
    qDeleteAll(m_item);
    m_item.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_class = false;
    m_has_attr_name = false;
    m_has_attr_stretch = false;
    m_has_attr_rowStretch = false;
    m_has_attr_columnStretch = false;
    m_has_attr_rowMinimumHeight = false;
    m_has_attr_columnMinimumWidth = false;
    }

    m_children = 0;
}

DomLayout::DomLayout()
{
    m_children = 0;
    m_has_attr_class = false;
    m_has_attr_name = false;
    m_has_attr_stretch = false;
    m_has_attr_rowStretch = false;
    m_has_attr_columnStretch = false;
    m_has_attr_rowMinimumHeight = false;
    m_has_attr_columnMinimumWidth = false;
}

DomLayout::~DomLayout()
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
    qDeleteAll(m_item);
    m_item.clear();
}

void DomLayout::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("class")) {
            setAttributeClass(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("stretch")) {
            setAttributeStretch(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("rowstretch")) {
            setAttributeRowStretch(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("columnstretch")) {
            setAttributeColumnStretch(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("rowminimumheight")) {
            setAttributeRowMinimumHeight(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("columnminimumwidth")) {
            setAttributeColumnMinimumWidth(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            if (tag == QStringLiteral("attribute")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_attribute.append(v);
                continue;
            }
            if (tag == QStringLiteral("item")) {
                DomLayoutItem *v = new DomLayoutItem();
                v->read(reader);
                m_item.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomLayout::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("layout") : tagName.toLower());

    if (hasAttributeClass())
        writer.writeAttribute(QStringLiteral("class"), attributeClass());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (hasAttributeStretch())
        writer.writeAttribute(QStringLiteral("stretch"), attributeStretch());

    if (hasAttributeRowStretch())
        writer.writeAttribute(QStringLiteral("rowstretch"), attributeRowStretch());

    if (hasAttributeColumnStretch())
        writer.writeAttribute(QStringLiteral("columnstretch"), attributeColumnStretch());

    if (hasAttributeRowMinimumHeight())
        writer.writeAttribute(QStringLiteral("rowminimumheight"), attributeRowMinimumHeight());

    if (hasAttributeColumnMinimumWidth())
        writer.writeAttribute(QStringLiteral("columnminimumwidth"), attributeColumnMinimumWidth());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    for (int i = 0; i < m_attribute.size(); ++i) {
        DomProperty* v = m_attribute[i];
        v->write(writer, QStringLiteral("attribute"));
    }
    for (int i = 0; i < m_item.size(); ++i) {
        DomLayoutItem* v = m_item[i];
        v->write(writer, QStringLiteral("item"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomLayout::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomLayout::setElementAttribute(const QList<DomProperty*>& a)
{
    m_children |= Attribute;
    m_attribute = a;
}

void DomLayout::setElementItem(const QList<DomLayoutItem*>& a)
{
    m_children |= Item;
    m_item = a;
}

void DomLayoutItem::clear(bool clear_all)
{
    delete m_widget;
    delete m_layout;
    delete m_spacer;

    if (clear_all) {
    m_text.clear();
    m_has_attr_row = false;
    m_attr_row = 0;
    m_has_attr_column = false;
    m_attr_column = 0;
    m_has_attr_rowSpan = false;
    m_attr_rowSpan = 0;
    m_has_attr_colSpan = false;
    m_attr_colSpan = 0;
    m_has_attr_alignment = false;
    }

    m_kind = Unknown;

    m_widget = 0;
    m_layout = 0;
    m_spacer = 0;
}

DomLayoutItem::DomLayoutItem()
{
    m_kind = Unknown;

    m_has_attr_row = false;
    m_attr_row = 0;
    m_has_attr_column = false;
    m_attr_column = 0;
    m_has_attr_rowSpan = false;
    m_attr_rowSpan = 0;
    m_has_attr_colSpan = false;
    m_attr_colSpan = 0;
    m_has_attr_alignment = false;
    m_widget = 0;
    m_layout = 0;
    m_spacer = 0;
}

DomLayoutItem::~DomLayoutItem()
{
    delete m_widget;
    delete m_layout;
    delete m_spacer;
}

void DomLayoutItem::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("row")) {
            setAttributeRow(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("column")) {
            setAttributeColumn(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("rowspan")) {
            setAttributeRowSpan(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("colspan")) {
            setAttributeColSpan(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("alignment")) {
            setAttributeAlignment(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("widget")) {
                DomWidget *v = new DomWidget();
                v->read(reader);
                setElementWidget(v);
                continue;
            }
            if (tag == QStringLiteral("layout")) {
                DomLayout *v = new DomLayout();
                v->read(reader);
                setElementLayout(v);
                continue;
            }
            if (tag == QStringLiteral("spacer")) {
                DomSpacer *v = new DomSpacer();
                v->read(reader);
                setElementSpacer(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomLayoutItem::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("layoutitem") : tagName.toLower());

    if (hasAttributeRow())
        writer.writeAttribute(QStringLiteral("row"), QString::number(attributeRow()));

    if (hasAttributeColumn())
        writer.writeAttribute(QStringLiteral("column"), QString::number(attributeColumn()));

    if (hasAttributeRowSpan())
        writer.writeAttribute(QStringLiteral("rowspan"), QString::number(attributeRowSpan()));

    if (hasAttributeColSpan())
        writer.writeAttribute(QStringLiteral("colspan"), QString::number(attributeColSpan()));

    if (hasAttributeAlignment())
        writer.writeAttribute(QStringLiteral("alignment"), attributeAlignment());

    switch (kind()) {
        case Widget: {
            DomWidget* v = elementWidget();
            if (v != 0) {
                v->write(writer, QStringLiteral("widget"));
            }
            break;
        }
        case Layout: {
            DomLayout* v = elementLayout();
            if (v != 0) {
                v->write(writer, QStringLiteral("layout"));
            }
            break;
        }
        case Spacer: {
            DomSpacer* v = elementSpacer();
            if (v != 0) {
                v->write(writer, QStringLiteral("spacer"));
            }
            break;
        }
        default:
            break;
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomWidget* DomLayoutItem::takeElementWidget() 
{
    DomWidget* a = m_widget;
    m_widget = 0;
    return a;
}

void DomLayoutItem::setElementWidget(DomWidget* a)
{
    clear(false);
    m_kind = Widget;
    m_widget = a;
}

DomLayout* DomLayoutItem::takeElementLayout() 
{
    DomLayout* a = m_layout;
    m_layout = 0;
    return a;
}

void DomLayoutItem::setElementLayout(DomLayout* a)
{
    clear(false);
    m_kind = Layout;
    m_layout = a;
}

DomSpacer* DomLayoutItem::takeElementSpacer() 
{
    DomSpacer* a = m_spacer;
    m_spacer = 0;
    return a;
}

void DomLayoutItem::setElementSpacer(DomSpacer* a)
{
    clear(false);
    m_kind = Spacer;
    m_spacer = a;
}

void DomRow::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomRow::DomRow()
{
    m_children = 0;
}

DomRow::~DomRow()
{
    qDeleteAll(m_property);
    m_property.clear();
}

void DomRow::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomRow::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("row") : tagName.toLower());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomRow::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomColumn::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomColumn::DomColumn()
{
    m_children = 0;
}

DomColumn::~DomColumn()
{
    qDeleteAll(m_property);
    m_property.clear();
}

void DomColumn::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomColumn::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("column") : tagName.toLower());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomColumn::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomItem::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_item);
    m_item.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_row = false;
    m_attr_row = 0;
    m_has_attr_column = false;
    m_attr_column = 0;
    }

    m_children = 0;
}

DomItem::DomItem()
{
    m_children = 0;
    m_has_attr_row = false;
    m_attr_row = 0;
    m_has_attr_column = false;
    m_attr_column = 0;
}

DomItem::~DomItem()
{
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_item);
    m_item.clear();
}

void DomItem::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("row")) {
            setAttributeRow(attribute.value().toString().toInt());
            continue;
        }
        if (name == QStringLiteral("column")) {
            setAttributeColumn(attribute.value().toString().toInt());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            if (tag == QStringLiteral("item")) {
                DomItem *v = new DomItem();
                v->read(reader);
                m_item.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomItem::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("item") : tagName.toLower());

    if (hasAttributeRow())
        writer.writeAttribute(QStringLiteral("row"), QString::number(attributeRow()));

    if (hasAttributeColumn())
        writer.writeAttribute(QStringLiteral("column"), QString::number(attributeColumn()));

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    for (int i = 0; i < m_item.size(); ++i) {
        DomItem* v = m_item[i];
        v->write(writer, QStringLiteral("item"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomItem::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomItem::setElementItem(const QList<DomItem*>& a)
{
    m_children |= Item;
    m_item = a;
}

void DomWidget::clear(bool clear_all)
{
    m_class.clear();
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_script);
    m_script.clear();
    qDeleteAll(m_widgetData);
    m_widgetData.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
    qDeleteAll(m_row);
    m_row.clear();
    qDeleteAll(m_column);
    m_column.clear();
    qDeleteAll(m_item);
    m_item.clear();
    qDeleteAll(m_layout);
    m_layout.clear();
    qDeleteAll(m_widget);
    m_widget.clear();
    qDeleteAll(m_action);
    m_action.clear();
    qDeleteAll(m_actionGroup);
    m_actionGroup.clear();
    qDeleteAll(m_addAction);
    m_addAction.clear();
    m_zOrder.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_class = false;
    m_has_attr_name = false;
    m_has_attr_native = false;
    m_attr_native = false;
    }

    m_children = 0;
}

DomWidget::DomWidget()
{
    m_children = 0;
    m_has_attr_class = false;
    m_has_attr_name = false;
    m_has_attr_native = false;
    m_attr_native = false;
}

DomWidget::~DomWidget()
{
    m_class.clear();
    qDeleteAll(m_property);
    m_property.clear();
    qDeleteAll(m_script);
    m_script.clear();
    qDeleteAll(m_widgetData);
    m_widgetData.clear();
    qDeleteAll(m_attribute);
    m_attribute.clear();
    qDeleteAll(m_row);
    m_row.clear();
    qDeleteAll(m_column);
    m_column.clear();
    qDeleteAll(m_item);
    m_item.clear();
    qDeleteAll(m_layout);
    m_layout.clear();
    qDeleteAll(m_widget);
    m_widget.clear();
    qDeleteAll(m_action);
    m_action.clear();
    qDeleteAll(m_actionGroup);
    m_actionGroup.clear();
    qDeleteAll(m_addAction);
    m_addAction.clear();
    m_zOrder.clear();
}

void DomWidget::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("class")) {
            setAttributeClass(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("native")) {
            setAttributeNative((attribute.value().toString() == QStringLiteral("true") ? true : false));
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("class")) {
                m_class.append(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            if (tag == QStringLiteral("script")) {
                DomScript *v = new DomScript();
                v->read(reader);
                m_script.append(v);
                continue;
            }
            if (tag == QStringLiteral("widgetdata")) {
                DomWidgetData *v = new DomWidgetData();
                v->read(reader);
                m_widgetData.append(v);
                continue;
            }
            if (tag == QStringLiteral("attribute")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_attribute.append(v);
                continue;
            }
            if (tag == QStringLiteral("row")) {
                DomRow *v = new DomRow();
                v->read(reader);
                m_row.append(v);
                continue;
            }
            if (tag == QStringLiteral("column")) {
                DomColumn *v = new DomColumn();
                v->read(reader);
                m_column.append(v);
                continue;
            }
            if (tag == QStringLiteral("item")) {
                DomItem *v = new DomItem();
                v->read(reader);
                m_item.append(v);
                continue;
            }
            if (tag == QStringLiteral("layout")) {
                DomLayout *v = new DomLayout();
                v->read(reader);
                m_layout.append(v);
                continue;
            }
            if (tag == QStringLiteral("widget")) {
                DomWidget *v = new DomWidget();
                v->read(reader);
                m_widget.append(v);
                continue;
            }
            if (tag == QStringLiteral("action")) {
                DomAction *v = new DomAction();
                v->read(reader);
                m_action.append(v);
                continue;
            }
            if (tag == QStringLiteral("actiongroup")) {
                DomActionGroup *v = new DomActionGroup();
                v->read(reader);
                m_actionGroup.append(v);
                continue;
            }
            if (tag == QStringLiteral("addaction")) {
                DomActionRef *v = new DomActionRef();
                v->read(reader);
                m_addAction.append(v);
                continue;
            }
            if (tag == QStringLiteral("zorder")) {
                m_zOrder.append(reader.readElementText());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomWidget::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("widget") : tagName.toLower());

    if (hasAttributeClass())
        writer.writeAttribute(QStringLiteral("class"), attributeClass());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (hasAttributeNative())
        writer.writeAttribute(QStringLiteral("native"), (attributeNative() ? QLatin1String("true") : QLatin1String("false")));

    for (int i = 0; i < m_class.size(); ++i) {
        QString v = m_class[i];
        writer.writeTextElement(QStringLiteral("class"), v);
    }
    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    for (int i = 0; i < m_script.size(); ++i) {
        DomScript* v = m_script[i];
        v->write(writer, QStringLiteral("script"));
    }
    for (int i = 0; i < m_widgetData.size(); ++i) {
        DomWidgetData* v = m_widgetData[i];
        v->write(writer, QStringLiteral("widgetdata"));
    }
    for (int i = 0; i < m_attribute.size(); ++i) {
        DomProperty* v = m_attribute[i];
        v->write(writer, QStringLiteral("attribute"));
    }
    for (int i = 0; i < m_row.size(); ++i) {
        DomRow* v = m_row[i];
        v->write(writer, QStringLiteral("row"));
    }
    for (int i = 0; i < m_column.size(); ++i) {
        DomColumn* v = m_column[i];
        v->write(writer, QStringLiteral("column"));
    }
    for (int i = 0; i < m_item.size(); ++i) {
        DomItem* v = m_item[i];
        v->write(writer, QStringLiteral("item"));
    }
    for (int i = 0; i < m_layout.size(); ++i) {
        DomLayout* v = m_layout[i];
        v->write(writer, QStringLiteral("layout"));
    }
    for (int i = 0; i < m_widget.size(); ++i) {
        DomWidget* v = m_widget[i];
        v->write(writer, QStringLiteral("widget"));
    }
    for (int i = 0; i < m_action.size(); ++i) {
        DomAction* v = m_action[i];
        v->write(writer, QStringLiteral("action"));
    }
    for (int i = 0; i < m_actionGroup.size(); ++i) {
        DomActionGroup* v = m_actionGroup[i];
        v->write(writer, QStringLiteral("actiongroup"));
    }
    for (int i = 0; i < m_addAction.size(); ++i) {
        DomActionRef* v = m_addAction[i];
        v->write(writer, QStringLiteral("addaction"));
    }
    for (int i = 0; i < m_zOrder.size(); ++i) {
        QString v = m_zOrder[i];
        writer.writeTextElement(QStringLiteral("zorder"), v);
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomWidget::setElementClass(const QStringList& a)
{
    m_children |= Class;
    m_class = a;
}

void DomWidget::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomWidget::setElementScript(const QList<DomScript*>& a)
{
    m_children |= Script;
    m_script = a;
}

void DomWidget::setElementWidgetData(const QList<DomWidgetData*>& a)
{
    m_children |= WidgetData;
    m_widgetData = a;
}

void DomWidget::setElementAttribute(const QList<DomProperty*>& a)
{
    m_children |= Attribute;
    m_attribute = a;
}

void DomWidget::setElementRow(const QList<DomRow*>& a)
{
    m_children |= Row;
    m_row = a;
}

void DomWidget::setElementColumn(const QList<DomColumn*>& a)
{
    m_children |= Column;
    m_column = a;
}

void DomWidget::setElementItem(const QList<DomItem*>& a)
{
    m_children |= Item;
    m_item = a;
}

void DomWidget::setElementLayout(const QList<DomLayout*>& a)
{
    m_children |= Layout;
    m_layout = a;
}

void DomWidget::setElementWidget(const QList<DomWidget*>& a)
{
    m_children |= Widget;
    m_widget = a;
}

void DomWidget::setElementAction(const QList<DomAction*>& a)
{
    m_children |= Action;
    m_action = a;
}

void DomWidget::setElementActionGroup(const QList<DomActionGroup*>& a)
{
    m_children |= ActionGroup;
    m_actionGroup = a;
}

void DomWidget::setElementAddAction(const QList<DomActionRef*>& a)
{
    m_children |= AddAction;
    m_addAction = a;
}

void DomWidget::setElementZOrder(const QStringList& a)
{
    m_children |= ZOrder;
    m_zOrder = a;
}

void DomSpacer::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    }

    m_children = 0;
}

DomSpacer::DomSpacer()
{
    m_children = 0;
    m_has_attr_name = false;
}

DomSpacer::~DomSpacer()
{
    qDeleteAll(m_property);
    m_property.clear();
}

void DomSpacer::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomSpacer::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("spacer") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSpacer::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomColor::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_alpha = false;
    m_attr_alpha = 0;
    }

    m_children = 0;
    m_red = 0;
    m_green = 0;
    m_blue = 0;
}

DomColor::DomColor()
{
    m_children = 0;
    m_has_attr_alpha = false;
    m_attr_alpha = 0;
    m_red = 0;
    m_green = 0;
    m_blue = 0;
}

DomColor::~DomColor()
{
}

void DomColor::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("alpha")) {
            setAttributeAlpha(attribute.value().toString().toInt());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("red")) {
                setElementRed(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("green")) {
                setElementGreen(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("blue")) {
                setElementBlue(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomColor::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("color") : tagName.toLower());

    if (hasAttributeAlpha())
        writer.writeAttribute(QStringLiteral("alpha"), QString::number(attributeAlpha()));

    if (m_children & Red) {
        writer.writeTextElement(QStringLiteral("red"), QString::number(m_red));
    }

    if (m_children & Green) {
        writer.writeTextElement(QStringLiteral("green"), QString::number(m_green));
    }

    if (m_children & Blue) {
        writer.writeTextElement(QStringLiteral("blue"), QString::number(m_blue));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomColor::setElementRed(int a)
{
    m_children |= Red;
    m_red = a;
}

void DomColor::setElementGreen(int a)
{
    m_children |= Green;
    m_green = a;
}

void DomColor::setElementBlue(int a)
{
    m_children |= Blue;
    m_blue = a;
}

void DomColor::clearElementRed()
{
    m_children &= ~Red;
}

void DomColor::clearElementGreen()
{
    m_children &= ~Green;
}

void DomColor::clearElementBlue()
{
    m_children &= ~Blue;
}

void DomGradientStop::clear(bool clear_all)
{
    delete m_color;

    if (clear_all) {
    m_text.clear();
    m_has_attr_position = false;
    m_attr_position = 0.0;
    }

    m_children = 0;
    m_color = 0;
}

DomGradientStop::DomGradientStop()
{
    m_children = 0;
    m_has_attr_position = false;
    m_attr_position = 0.0;
    m_color = 0;
}

DomGradientStop::~DomGradientStop()
{
    delete m_color;
}

void DomGradientStop::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("position")) {
            setAttributePosition(attribute.value().toString().toDouble());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("color")) {
                DomColor *v = new DomColor();
                v->read(reader);
                setElementColor(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomGradientStop::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("gradientstop") : tagName.toLower());

    if (hasAttributePosition())
        writer.writeAttribute(QStringLiteral("position"), QString::number(attributePosition(), 'f', 15));

    if (m_children & Color) {
        m_color->write(writer, QStringLiteral("color"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomColor* DomGradientStop::takeElementColor() 
{
    DomColor* a = m_color;
    m_color = 0;
    m_children ^= Color;
    return a;
}

void DomGradientStop::setElementColor(DomColor* a)
{
    delete m_color;
    m_children |= Color;
    m_color = a;
}

void DomGradientStop::clearElementColor()
{
    delete m_color;
    m_color = 0;
    m_children &= ~Color;
}

void DomGradient::clear(bool clear_all)
{
    qDeleteAll(m_gradientStop);
    m_gradientStop.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_startX = false;
    m_attr_startX = 0.0;
    m_has_attr_startY = false;
    m_attr_startY = 0.0;
    m_has_attr_endX = false;
    m_attr_endX = 0.0;
    m_has_attr_endY = false;
    m_attr_endY = 0.0;
    m_has_attr_centralX = false;
    m_attr_centralX = 0.0;
    m_has_attr_centralY = false;
    m_attr_centralY = 0.0;
    m_has_attr_focalX = false;
    m_attr_focalX = 0.0;
    m_has_attr_focalY = false;
    m_attr_focalY = 0.0;
    m_has_attr_radius = false;
    m_attr_radius = 0.0;
    m_has_attr_angle = false;
    m_attr_angle = 0.0;
    m_has_attr_type = false;
    m_has_attr_spread = false;
    m_has_attr_coordinateMode = false;
    }

    m_children = 0;
}

DomGradient::DomGradient()
{
    m_children = 0;
    m_has_attr_startX = false;
    m_attr_startX = 0.0;
    m_has_attr_startY = false;
    m_attr_startY = 0.0;
    m_has_attr_endX = false;
    m_attr_endX = 0.0;
    m_has_attr_endY = false;
    m_attr_endY = 0.0;
    m_has_attr_centralX = false;
    m_attr_centralX = 0.0;
    m_has_attr_centralY = false;
    m_attr_centralY = 0.0;
    m_has_attr_focalX = false;
    m_attr_focalX = 0.0;
    m_has_attr_focalY = false;
    m_attr_focalY = 0.0;
    m_has_attr_radius = false;
    m_attr_radius = 0.0;
    m_has_attr_angle = false;
    m_attr_angle = 0.0;
    m_has_attr_type = false;
    m_has_attr_spread = false;
    m_has_attr_coordinateMode = false;
}

DomGradient::~DomGradient()
{
    qDeleteAll(m_gradientStop);
    m_gradientStop.clear();
}

void DomGradient::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("startx")) {
            setAttributeStartX(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("starty")) {
            setAttributeStartY(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("endx")) {
            setAttributeEndX(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("endy")) {
            setAttributeEndY(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("centralx")) {
            setAttributeCentralX(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("centraly")) {
            setAttributeCentralY(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("focalx")) {
            setAttributeFocalX(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("focaly")) {
            setAttributeFocalY(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("radius")) {
            setAttributeRadius(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("angle")) {
            setAttributeAngle(attribute.value().toString().toDouble());
            continue;
        }
        if (name == QStringLiteral("type")) {
            setAttributeType(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("spread")) {
            setAttributeSpread(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("coordinatemode")) {
            setAttributeCoordinateMode(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("gradientstop")) {
                DomGradientStop *v = new DomGradientStop();
                v->read(reader);
                m_gradientStop.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomGradient::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("gradient") : tagName.toLower());

    if (hasAttributeStartX())
        writer.writeAttribute(QStringLiteral("startx"), QString::number(attributeStartX(), 'f', 15));

    if (hasAttributeStartY())
        writer.writeAttribute(QStringLiteral("starty"), QString::number(attributeStartY(), 'f', 15));

    if (hasAttributeEndX())
        writer.writeAttribute(QStringLiteral("endx"), QString::number(attributeEndX(), 'f', 15));

    if (hasAttributeEndY())
        writer.writeAttribute(QStringLiteral("endy"), QString::number(attributeEndY(), 'f', 15));

    if (hasAttributeCentralX())
        writer.writeAttribute(QStringLiteral("centralx"), QString::number(attributeCentralX(), 'f', 15));

    if (hasAttributeCentralY())
        writer.writeAttribute(QStringLiteral("centraly"), QString::number(attributeCentralY(), 'f', 15));

    if (hasAttributeFocalX())
        writer.writeAttribute(QStringLiteral("focalx"), QString::number(attributeFocalX(), 'f', 15));

    if (hasAttributeFocalY())
        writer.writeAttribute(QStringLiteral("focaly"), QString::number(attributeFocalY(), 'f', 15));

    if (hasAttributeRadius())
        writer.writeAttribute(QStringLiteral("radius"), QString::number(attributeRadius(), 'f', 15));

    if (hasAttributeAngle())
        writer.writeAttribute(QStringLiteral("angle"), QString::number(attributeAngle(), 'f', 15));

    if (hasAttributeType())
        writer.writeAttribute(QStringLiteral("type"), attributeType());

    if (hasAttributeSpread())
        writer.writeAttribute(QStringLiteral("spread"), attributeSpread());

    if (hasAttributeCoordinateMode())
        writer.writeAttribute(QStringLiteral("coordinatemode"), attributeCoordinateMode());

    for (int i = 0; i < m_gradientStop.size(); ++i) {
        DomGradientStop* v = m_gradientStop[i];
        v->write(writer, QStringLiteral("gradientstop"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomGradient::setElementGradientStop(const QList<DomGradientStop*>& a)
{
    m_children |= GradientStop;
    m_gradientStop = a;
}

void DomBrush::clear(bool clear_all)
{
    delete m_color;
    delete m_texture;
    delete m_gradient;

    if (clear_all) {
    m_text.clear();
    m_has_attr_brushStyle = false;
    }

    m_kind = Unknown;

    m_color = 0;
    m_texture = 0;
    m_gradient = 0;
}

DomBrush::DomBrush()
{
    m_kind = Unknown;

    m_has_attr_brushStyle = false;
    m_color = 0;
    m_texture = 0;
    m_gradient = 0;
}

DomBrush::~DomBrush()
{
    delete m_color;
    delete m_texture;
    delete m_gradient;
}

void DomBrush::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("brushstyle")) {
            setAttributeBrushStyle(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("color")) {
                DomColor *v = new DomColor();
                v->read(reader);
                setElementColor(v);
                continue;
            }
            if (tag == QStringLiteral("texture")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                setElementTexture(v);
                continue;
            }
            if (tag == QStringLiteral("gradient")) {
                DomGradient *v = new DomGradient();
                v->read(reader);
                setElementGradient(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomBrush::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("brush") : tagName.toLower());

    if (hasAttributeBrushStyle())
        writer.writeAttribute(QStringLiteral("brushstyle"), attributeBrushStyle());

    switch (kind()) {
        case Color: {
            DomColor* v = elementColor();
            if (v != 0) {
                v->write(writer, QStringLiteral("color"));
            }
            break;
        }
        case Texture: {
            DomProperty* v = elementTexture();
            if (v != 0) {
                v->write(writer, QStringLiteral("texture"));
            }
            break;
        }
        case Gradient: {
            DomGradient* v = elementGradient();
            if (v != 0) {
                v->write(writer, QStringLiteral("gradient"));
            }
            break;
        }
        default:
            break;
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomColor* DomBrush::takeElementColor() 
{
    DomColor* a = m_color;
    m_color = 0;
    return a;
}

void DomBrush::setElementColor(DomColor* a)
{
    clear(false);
    m_kind = Color;
    m_color = a;
}

DomProperty* DomBrush::takeElementTexture() 
{
    DomProperty* a = m_texture;
    m_texture = 0;
    return a;
}

void DomBrush::setElementTexture(DomProperty* a)
{
    clear(false);
    m_kind = Texture;
    m_texture = a;
}

DomGradient* DomBrush::takeElementGradient() 
{
    DomGradient* a = m_gradient;
    m_gradient = 0;
    return a;
}

void DomBrush::setElementGradient(DomGradient* a)
{
    clear(false);
    m_kind = Gradient;
    m_gradient = a;
}

void DomColorRole::clear(bool clear_all)
{
    delete m_brush;

    if (clear_all) {
    m_text.clear();
    m_has_attr_role = false;
    }

    m_children = 0;
    m_brush = 0;
}

DomColorRole::DomColorRole()
{
    m_children = 0;
    m_has_attr_role = false;
    m_brush = 0;
}

DomColorRole::~DomColorRole()
{
    delete m_brush;
}

void DomColorRole::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("role")) {
            setAttributeRole(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("brush")) {
                DomBrush *v = new DomBrush();
                v->read(reader);
                setElementBrush(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomColorRole::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("colorrole") : tagName.toLower());

    if (hasAttributeRole())
        writer.writeAttribute(QStringLiteral("role"), attributeRole());

    if (m_children & Brush) {
        m_brush->write(writer, QStringLiteral("brush"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomBrush* DomColorRole::takeElementBrush() 
{
    DomBrush* a = m_brush;
    m_brush = 0;
    m_children ^= Brush;
    return a;
}

void DomColorRole::setElementBrush(DomBrush* a)
{
    delete m_brush;
    m_children |= Brush;
    m_brush = a;
}

void DomColorRole::clearElementBrush()
{
    delete m_brush;
    m_brush = 0;
    m_children &= ~Brush;
}

void DomColorGroup::clear(bool clear_all)
{
    qDeleteAll(m_colorRole);
    m_colorRole.clear();
    qDeleteAll(m_color);
    m_color.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomColorGroup::DomColorGroup()
{
    m_children = 0;
}

DomColorGroup::~DomColorGroup()
{
    qDeleteAll(m_colorRole);
    m_colorRole.clear();
    qDeleteAll(m_color);
    m_color.clear();
}

void DomColorGroup::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("colorrole")) {
                DomColorRole *v = new DomColorRole();
                v->read(reader);
                m_colorRole.append(v);
                continue;
            }
            if (tag == QStringLiteral("color")) {
                DomColor *v = new DomColor();
                v->read(reader);
                m_color.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomColorGroup::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("colorgroup") : tagName.toLower());

    for (int i = 0; i < m_colorRole.size(); ++i) {
        DomColorRole* v = m_colorRole[i];
        v->write(writer, QStringLiteral("colorrole"));
    }
    for (int i = 0; i < m_color.size(); ++i) {
        DomColor* v = m_color[i];
        v->write(writer, QStringLiteral("color"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomColorGroup::setElementColorRole(const QList<DomColorRole*>& a)
{
    m_children |= ColorRole;
    m_colorRole = a;
}

void DomColorGroup::setElementColor(const QList<DomColor*>& a)
{
    m_children |= Color;
    m_color = a;
}

void DomPalette::clear(bool clear_all)
{
    delete m_active;
    delete m_inactive;
    delete m_disabled;

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_active = 0;
    m_inactive = 0;
    m_disabled = 0;
}

DomPalette::DomPalette()
{
    m_children = 0;
    m_active = 0;
    m_inactive = 0;
    m_disabled = 0;
}

DomPalette::~DomPalette()
{
    delete m_active;
    delete m_inactive;
    delete m_disabled;
}

void DomPalette::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("active")) {
                DomColorGroup *v = new DomColorGroup();
                v->read(reader);
                setElementActive(v);
                continue;
            }
            if (tag == QStringLiteral("inactive")) {
                DomColorGroup *v = new DomColorGroup();
                v->read(reader);
                setElementInactive(v);
                continue;
            }
            if (tag == QStringLiteral("disabled")) {
                DomColorGroup *v = new DomColorGroup();
                v->read(reader);
                setElementDisabled(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomPalette::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("palette") : tagName.toLower());

    if (m_children & Active) {
        m_active->write(writer, QStringLiteral("active"));
    }

    if (m_children & Inactive) {
        m_inactive->write(writer, QStringLiteral("inactive"));
    }

    if (m_children & Disabled) {
        m_disabled->write(writer, QStringLiteral("disabled"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomColorGroup* DomPalette::takeElementActive() 
{
    DomColorGroup* a = m_active;
    m_active = 0;
    m_children ^= Active;
    return a;
}

void DomPalette::setElementActive(DomColorGroup* a)
{
    delete m_active;
    m_children |= Active;
    m_active = a;
}

DomColorGroup* DomPalette::takeElementInactive() 
{
    DomColorGroup* a = m_inactive;
    m_inactive = 0;
    m_children ^= Inactive;
    return a;
}

void DomPalette::setElementInactive(DomColorGroup* a)
{
    delete m_inactive;
    m_children |= Inactive;
    m_inactive = a;
}

DomColorGroup* DomPalette::takeElementDisabled() 
{
    DomColorGroup* a = m_disabled;
    m_disabled = 0;
    m_children ^= Disabled;
    return a;
}

void DomPalette::setElementDisabled(DomColorGroup* a)
{
    delete m_disabled;
    m_children |= Disabled;
    m_disabled = a;
}

void DomPalette::clearElementActive()
{
    delete m_active;
    m_active = 0;
    m_children &= ~Active;
}

void DomPalette::clearElementInactive()
{
    delete m_inactive;
    m_inactive = 0;
    m_children &= ~Inactive;
}

void DomPalette::clearElementDisabled()
{
    delete m_disabled;
    m_disabled = 0;
    m_children &= ~Disabled;
}

void DomFont::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_pointSize = 0;
    m_weight = 0;
    m_italic = false;
    m_bold = false;
    m_underline = false;
    m_strikeOut = false;
    m_antialiasing = false;
    m_kerning = false;
}

DomFont::DomFont()
{
    m_children = 0;
    m_pointSize = 0;
    m_weight = 0;
    m_italic = false;
    m_bold = false;
    m_underline = false;
    m_strikeOut = false;
    m_antialiasing = false;
    m_kerning = false;
}

DomFont::~DomFont()
{
}

void DomFont::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("family")) {
                setElementFamily(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("pointsize")) {
                setElementPointSize(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("weight")) {
                setElementWeight(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("italic")) {
                setElementItalic((reader.readElementText() == QStringLiteral("true") ? true : false));
                continue;
            }
            if (tag == QStringLiteral("bold")) {
                setElementBold((reader.readElementText() == QStringLiteral("true") ? true : false));
                continue;
            }
            if (tag == QStringLiteral("underline")) {
                setElementUnderline((reader.readElementText() == QStringLiteral("true") ? true : false));
                continue;
            }
            if (tag == QStringLiteral("strikeout")) {
                setElementStrikeOut((reader.readElementText() == QStringLiteral("true") ? true : false));
                continue;
            }
            if (tag == QStringLiteral("antialiasing")) {
                setElementAntialiasing((reader.readElementText() == QStringLiteral("true") ? true : false));
                continue;
            }
            if (tag == QStringLiteral("stylestrategy")) {
                setElementStyleStrategy(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("kerning")) {
                setElementKerning((reader.readElementText() == QStringLiteral("true") ? true : false));
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomFont::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("font") : tagName.toLower());

    if (m_children & Family) {
        writer.writeTextElement(QStringLiteral("family"), m_family);
    }

    if (m_children & PointSize) {
        writer.writeTextElement(QStringLiteral("pointsize"), QString::number(m_pointSize));
    }

    if (m_children & Weight) {
        writer.writeTextElement(QStringLiteral("weight"), QString::number(m_weight));
    }

    if (m_children & Italic) {
        writer.writeTextElement(QStringLiteral("italic"), (m_italic ? QLatin1String("true") : QLatin1String("false")));
    }

    if (m_children & Bold) {
        writer.writeTextElement(QStringLiteral("bold"), (m_bold ? QLatin1String("true") : QLatin1String("false")));
    }

    if (m_children & Underline) {
        writer.writeTextElement(QStringLiteral("underline"), (m_underline ? QLatin1String("true") : QLatin1String("false")));
    }

    if (m_children & StrikeOut) {
        writer.writeTextElement(QStringLiteral("strikeout"), (m_strikeOut ? QLatin1String("true") : QLatin1String("false")));
    }

    if (m_children & Antialiasing) {
        writer.writeTextElement(QStringLiteral("antialiasing"), (m_antialiasing ? QLatin1String("true") : QLatin1String("false")));
    }

    if (m_children & StyleStrategy) {
        writer.writeTextElement(QStringLiteral("stylestrategy"), m_styleStrategy);
    }

    if (m_children & Kerning) {
        writer.writeTextElement(QStringLiteral("kerning"), (m_kerning ? QLatin1String("true") : QLatin1String("false")));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomFont::setElementFamily(const QString& a)
{
    m_children |= Family;
    m_family = a;
}

void DomFont::setElementPointSize(int a)
{
    m_children |= PointSize;
    m_pointSize = a;
}

void DomFont::setElementWeight(int a)
{
    m_children |= Weight;
    m_weight = a;
}

void DomFont::setElementItalic(bool a)
{
    m_children |= Italic;
    m_italic = a;
}

void DomFont::setElementBold(bool a)
{
    m_children |= Bold;
    m_bold = a;
}

void DomFont::setElementUnderline(bool a)
{
    m_children |= Underline;
    m_underline = a;
}

void DomFont::setElementStrikeOut(bool a)
{
    m_children |= StrikeOut;
    m_strikeOut = a;
}

void DomFont::setElementAntialiasing(bool a)
{
    m_children |= Antialiasing;
    m_antialiasing = a;
}

void DomFont::setElementStyleStrategy(const QString& a)
{
    m_children |= StyleStrategy;
    m_styleStrategy = a;
}

void DomFont::setElementKerning(bool a)
{
    m_children |= Kerning;
    m_kerning = a;
}

void DomFont::clearElementFamily()
{
    m_children &= ~Family;
}

void DomFont::clearElementPointSize()
{
    m_children &= ~PointSize;
}

void DomFont::clearElementWeight()
{
    m_children &= ~Weight;
}

void DomFont::clearElementItalic()
{
    m_children &= ~Italic;
}

void DomFont::clearElementBold()
{
    m_children &= ~Bold;
}

void DomFont::clearElementUnderline()
{
    m_children &= ~Underline;
}

void DomFont::clearElementStrikeOut()
{
    m_children &= ~StrikeOut;
}

void DomFont::clearElementAntialiasing()
{
    m_children &= ~Antialiasing;
}

void DomFont::clearElementStyleStrategy()
{
    m_children &= ~StyleStrategy;
}

void DomFont::clearElementKerning()
{
    m_children &= ~Kerning;
}

void DomPoint::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_x = 0;
    m_y = 0;
}

DomPoint::DomPoint()
{
    m_children = 0;
    m_x = 0;
    m_y = 0;
}

DomPoint::~DomPoint()
{
}

void DomPoint::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QString(QLatin1Char('x'))) {
                setElementX(reader.readElementText().toInt());
                continue;
            }
            if (tag == QString(QLatin1Char('y'))) {
                setElementY(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomPoint::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("point") : tagName.toLower());

    if (m_children & X) {
        writer.writeTextElement(QString(QLatin1Char('x')), QString::number(m_x));
    }

    if (m_children & Y) {
        writer.writeTextElement(QString(QLatin1Char('y')), QString::number(m_y));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomPoint::setElementX(int a)
{
    m_children |= X;
    m_x = a;
}

void DomPoint::setElementY(int a)
{
    m_children |= Y;
    m_y = a;
}

void DomPoint::clearElementX()
{
    m_children &= ~X;
}

void DomPoint::clearElementY()
{
    m_children &= ~Y;
}

void DomRect::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
}

DomRect::DomRect()
{
    m_children = 0;
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
}

DomRect::~DomRect()
{
}

void DomRect::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QString(QLatin1Char('x'))) {
                setElementX(reader.readElementText().toInt());
                continue;
            }
            if (tag == QString(QLatin1Char('y'))) {
                setElementY(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("width")) {
                setElementWidth(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("height")) {
                setElementHeight(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomRect::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("rect") : tagName.toLower());

    if (m_children & X) {
        writer.writeTextElement(QString(QLatin1Char('x')), QString::number(m_x));
    }

    if (m_children & Y) {
        writer.writeTextElement(QString(QLatin1Char('y')), QString::number(m_y));
    }

    if (m_children & Width) {
        writer.writeTextElement(QStringLiteral("width"), QString::number(m_width));
    }

    if (m_children & Height) {
        writer.writeTextElement(QStringLiteral("height"), QString::number(m_height));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomRect::setElementX(int a)
{
    m_children |= X;
    m_x = a;
}

void DomRect::setElementY(int a)
{
    m_children |= Y;
    m_y = a;
}

void DomRect::setElementWidth(int a)
{
    m_children |= Width;
    m_width = a;
}

void DomRect::setElementHeight(int a)
{
    m_children |= Height;
    m_height = a;
}

void DomRect::clearElementX()
{
    m_children &= ~X;
}

void DomRect::clearElementY()
{
    m_children &= ~Y;
}

void DomRect::clearElementWidth()
{
    m_children &= ~Width;
}

void DomRect::clearElementHeight()
{
    m_children &= ~Height;
}

void DomLocale::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_language = false;
    m_has_attr_country = false;
    }

    m_children = 0;
}

DomLocale::DomLocale()
{
    m_children = 0;
    m_has_attr_language = false;
    m_has_attr_country = false;
}

DomLocale::~DomLocale()
{
}

void DomLocale::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("language")) {
            setAttributeLanguage(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("country")) {
            setAttributeCountry(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomLocale::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("locale") : tagName.toLower());

    if (hasAttributeLanguage())
        writer.writeAttribute(QStringLiteral("language"), attributeLanguage());

    if (hasAttributeCountry())
        writer.writeAttribute(QStringLiteral("country"), attributeCountry());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSizePolicy::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_hSizeType = false;
    m_has_attr_vSizeType = false;
    }

    m_children = 0;
    m_hSizeType = 0;
    m_vSizeType = 0;
    m_horStretch = 0;
    m_verStretch = 0;
}

DomSizePolicy::DomSizePolicy()
{
    m_children = 0;
    m_has_attr_hSizeType = false;
    m_has_attr_vSizeType = false;
    m_hSizeType = 0;
    m_vSizeType = 0;
    m_horStretch = 0;
    m_verStretch = 0;
}

DomSizePolicy::~DomSizePolicy()
{
}

void DomSizePolicy::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("hsizetype")) {
            setAttributeHSizeType(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("vsizetype")) {
            setAttributeVSizeType(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("hsizetype")) {
                setElementHSizeType(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("vsizetype")) {
                setElementVSizeType(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("horstretch")) {
                setElementHorStretch(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("verstretch")) {
                setElementVerStretch(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomSizePolicy::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("sizepolicy") : tagName.toLower());

    if (hasAttributeHSizeType())
        writer.writeAttribute(QStringLiteral("hsizetype"), attributeHSizeType());

    if (hasAttributeVSizeType())
        writer.writeAttribute(QStringLiteral("vsizetype"), attributeVSizeType());

    if (m_children & HSizeType) {
        writer.writeTextElement(QStringLiteral("hsizetype"), QString::number(m_hSizeType));
    }

    if (m_children & VSizeType) {
        writer.writeTextElement(QStringLiteral("vsizetype"), QString::number(m_vSizeType));
    }

    if (m_children & HorStretch) {
        writer.writeTextElement(QStringLiteral("horstretch"), QString::number(m_horStretch));
    }

    if (m_children & VerStretch) {
        writer.writeTextElement(QStringLiteral("verstretch"), QString::number(m_verStretch));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSizePolicy::setElementHSizeType(int a)
{
    m_children |= HSizeType;
    m_hSizeType = a;
}

void DomSizePolicy::setElementVSizeType(int a)
{
    m_children |= VSizeType;
    m_vSizeType = a;
}

void DomSizePolicy::setElementHorStretch(int a)
{
    m_children |= HorStretch;
    m_horStretch = a;
}

void DomSizePolicy::setElementVerStretch(int a)
{
    m_children |= VerStretch;
    m_verStretch = a;
}

void DomSizePolicy::clearElementHSizeType()
{
    m_children &= ~HSizeType;
}

void DomSizePolicy::clearElementVSizeType()
{
    m_children &= ~VSizeType;
}

void DomSizePolicy::clearElementHorStretch()
{
    m_children &= ~HorStretch;
}

void DomSizePolicy::clearElementVerStretch()
{
    m_children &= ~VerStretch;
}

void DomSize::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_width = 0;
    m_height = 0;
}

DomSize::DomSize()
{
    m_children = 0;
    m_width = 0;
    m_height = 0;
}

DomSize::~DomSize()
{
}

void DomSize::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("width")) {
                setElementWidth(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("height")) {
                setElementHeight(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomSize::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("size") : tagName.toLower());

    if (m_children & Width) {
        writer.writeTextElement(QStringLiteral("width"), QString::number(m_width));
    }

    if (m_children & Height) {
        writer.writeTextElement(QStringLiteral("height"), QString::number(m_height));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSize::setElementWidth(int a)
{
    m_children |= Width;
    m_width = a;
}

void DomSize::setElementHeight(int a)
{
    m_children |= Height;
    m_height = a;
}

void DomSize::clearElementWidth()
{
    m_children &= ~Width;
}

void DomSize::clearElementHeight()
{
    m_children &= ~Height;
}

void DomDate::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_year = 0;
    m_month = 0;
    m_day = 0;
}

DomDate::DomDate()
{
    m_children = 0;
    m_year = 0;
    m_month = 0;
    m_day = 0;
}

DomDate::~DomDate()
{
}

void DomDate::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("year")) {
                setElementYear(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("month")) {
                setElementMonth(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("day")) {
                setElementDay(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomDate::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("date") : tagName.toLower());

    if (m_children & Year) {
        writer.writeTextElement(QStringLiteral("year"), QString::number(m_year));
    }

    if (m_children & Month) {
        writer.writeTextElement(QStringLiteral("month"), QString::number(m_month));
    }

    if (m_children & Day) {
        writer.writeTextElement(QStringLiteral("day"), QString::number(m_day));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomDate::setElementYear(int a)
{
    m_children |= Year;
    m_year = a;
}

void DomDate::setElementMonth(int a)
{
    m_children |= Month;
    m_month = a;
}

void DomDate::setElementDay(int a)
{
    m_children |= Day;
    m_day = a;
}

void DomDate::clearElementYear()
{
    m_children &= ~Year;
}

void DomDate::clearElementMonth()
{
    m_children &= ~Month;
}

void DomDate::clearElementDay()
{
    m_children &= ~Day;
}

void DomTime::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
}

DomTime::DomTime()
{
    m_children = 0;
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
}

DomTime::~DomTime()
{
}

void DomTime::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("hour")) {
                setElementHour(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("minute")) {
                setElementMinute(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("second")) {
                setElementSecond(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomTime::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("time") : tagName.toLower());

    if (m_children & Hour) {
        writer.writeTextElement(QStringLiteral("hour"), QString::number(m_hour));
    }

    if (m_children & Minute) {
        writer.writeTextElement(QStringLiteral("minute"), QString::number(m_minute));
    }

    if (m_children & Second) {
        writer.writeTextElement(QStringLiteral("second"), QString::number(m_second));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomTime::setElementHour(int a)
{
    m_children |= Hour;
    m_hour = a;
}

void DomTime::setElementMinute(int a)
{
    m_children |= Minute;
    m_minute = a;
}

void DomTime::setElementSecond(int a)
{
    m_children |= Second;
    m_second = a;
}

void DomTime::clearElementHour()
{
    m_children &= ~Hour;
}

void DomTime::clearElementMinute()
{
    m_children &= ~Minute;
}

void DomTime::clearElementSecond()
{
    m_children &= ~Second;
}

void DomDateTime::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
    m_year = 0;
    m_month = 0;
    m_day = 0;
}

DomDateTime::DomDateTime()
{
    m_children = 0;
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
    m_year = 0;
    m_month = 0;
    m_day = 0;
}

DomDateTime::~DomDateTime()
{
}

void DomDateTime::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("hour")) {
                setElementHour(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("minute")) {
                setElementMinute(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("second")) {
                setElementSecond(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("year")) {
                setElementYear(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("month")) {
                setElementMonth(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("day")) {
                setElementDay(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomDateTime::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("datetime") : tagName.toLower());

    if (m_children & Hour) {
        writer.writeTextElement(QStringLiteral("hour"), QString::number(m_hour));
    }

    if (m_children & Minute) {
        writer.writeTextElement(QStringLiteral("minute"), QString::number(m_minute));
    }

    if (m_children & Second) {
        writer.writeTextElement(QStringLiteral("second"), QString::number(m_second));
    }

    if (m_children & Year) {
        writer.writeTextElement(QStringLiteral("year"), QString::number(m_year));
    }

    if (m_children & Month) {
        writer.writeTextElement(QStringLiteral("month"), QString::number(m_month));
    }

    if (m_children & Day) {
        writer.writeTextElement(QStringLiteral("day"), QString::number(m_day));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomDateTime::setElementHour(int a)
{
    m_children |= Hour;
    m_hour = a;
}

void DomDateTime::setElementMinute(int a)
{
    m_children |= Minute;
    m_minute = a;
}

void DomDateTime::setElementSecond(int a)
{
    m_children |= Second;
    m_second = a;
}

void DomDateTime::setElementYear(int a)
{
    m_children |= Year;
    m_year = a;
}

void DomDateTime::setElementMonth(int a)
{
    m_children |= Month;
    m_month = a;
}

void DomDateTime::setElementDay(int a)
{
    m_children |= Day;
    m_day = a;
}

void DomDateTime::clearElementHour()
{
    m_children &= ~Hour;
}

void DomDateTime::clearElementMinute()
{
    m_children &= ~Minute;
}

void DomDateTime::clearElementSecond()
{
    m_children &= ~Second;
}

void DomDateTime::clearElementYear()
{
    m_children &= ~Year;
}

void DomDateTime::clearElementMonth()
{
    m_children &= ~Month;
}

void DomDateTime::clearElementDay()
{
    m_children &= ~Day;
}

void DomStringList::clear(bool clear_all)
{
    m_string.clear();

    if (clear_all) {
    m_text.clear();
    m_has_attr_notr = false;
    m_has_attr_comment = false;
    m_has_attr_extraComment = false;
    }

    m_children = 0;
}

DomStringList::DomStringList()
{
    m_children = 0;
    m_has_attr_notr = false;
    m_has_attr_comment = false;
    m_has_attr_extraComment = false;
}

DomStringList::~DomStringList()
{
    m_string.clear();
}

void DomStringList::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("notr")) {
            setAttributeNotr(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("comment")) {
            setAttributeComment(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("extracomment")) {
            setAttributeExtraComment(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("string")) {
                m_string.append(reader.readElementText());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomStringList::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("stringlist") : tagName.toLower());

    if (hasAttributeNotr())
        writer.writeAttribute(QStringLiteral("notr"), attributeNotr());

    if (hasAttributeComment())
        writer.writeAttribute(QStringLiteral("comment"), attributeComment());

    if (hasAttributeExtraComment())
        writer.writeAttribute(QStringLiteral("extracomment"), attributeExtraComment());

    for (int i = 0; i < m_string.size(); ++i) {
        QString v = m_string[i];
        writer.writeTextElement(QStringLiteral("string"), v);
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomStringList::setElementString(const QStringList& a)
{
    m_children |= String;
    m_string = a;
}

void DomResourcePixmap::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_resource = false;
    m_has_attr_alias = false;
    }

    m_children = 0;
}

DomResourcePixmap::DomResourcePixmap()
{
    m_children = 0;
    m_has_attr_resource = false;
    m_has_attr_alias = false;
    m_text.clear();
}

DomResourcePixmap::~DomResourcePixmap()
{
}

void DomResourcePixmap::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("resource")) {
            setAttributeResource(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("alias")) {
            setAttributeAlias(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomResourcePixmap::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("resourcepixmap") : tagName.toLower());

    if (hasAttributeResource())
        writer.writeAttribute(QStringLiteral("resource"), attributeResource());

    if (hasAttributeAlias())
        writer.writeAttribute(QStringLiteral("alias"), attributeAlias());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomResourceIcon::clear(bool clear_all)
{
    delete m_normalOff;
    delete m_normalOn;
    delete m_disabledOff;
    delete m_disabledOn;
    delete m_activeOff;
    delete m_activeOn;
    delete m_selectedOff;
    delete m_selectedOn;

    if (clear_all) {
    m_text.clear();
    m_has_attr_theme = false;
    m_has_attr_resource = false;
    }

    m_children = 0;
    m_normalOff = 0;
    m_normalOn = 0;
    m_disabledOff = 0;
    m_disabledOn = 0;
    m_activeOff = 0;
    m_activeOn = 0;
    m_selectedOff = 0;
    m_selectedOn = 0;
}

DomResourceIcon::DomResourceIcon()
{
    m_children = 0;
    m_has_attr_theme = false;
    m_has_attr_resource = false;
    m_text.clear();
    m_normalOff = 0;
    m_normalOn = 0;
    m_disabledOff = 0;
    m_disabledOn = 0;
    m_activeOff = 0;
    m_activeOn = 0;
    m_selectedOff = 0;
    m_selectedOn = 0;
}

DomResourceIcon::~DomResourceIcon()
{
    delete m_normalOff;
    delete m_normalOn;
    delete m_disabledOff;
    delete m_disabledOn;
    delete m_activeOff;
    delete m_activeOn;
    delete m_selectedOff;
    delete m_selectedOn;
}

void DomResourceIcon::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("theme")) {
            setAttributeTheme(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("resource")) {
            setAttributeResource(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("normaloff")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementNormalOff(v);
                continue;
            }
            if (tag == QStringLiteral("normalon")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementNormalOn(v);
                continue;
            }
            if (tag == QStringLiteral("disabledoff")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementDisabledOff(v);
                continue;
            }
            if (tag == QStringLiteral("disabledon")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementDisabledOn(v);
                continue;
            }
            if (tag == QStringLiteral("activeoff")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementActiveOff(v);
                continue;
            }
            if (tag == QStringLiteral("activeon")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementActiveOn(v);
                continue;
            }
            if (tag == QStringLiteral("selectedoff")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementSelectedOff(v);
                continue;
            }
            if (tag == QStringLiteral("selectedon")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementSelectedOn(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomResourceIcon::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("resourceicon") : tagName.toLower());

    if (hasAttributeTheme())
        writer.writeAttribute(QStringLiteral("theme"), attributeTheme());

    if (hasAttributeResource())
        writer.writeAttribute(QStringLiteral("resource"), attributeResource());

    if (m_children & NormalOff) {
        m_normalOff->write(writer, QStringLiteral("normaloff"));
    }

    if (m_children & NormalOn) {
        m_normalOn->write(writer, QStringLiteral("normalon"));
    }

    if (m_children & DisabledOff) {
        m_disabledOff->write(writer, QStringLiteral("disabledoff"));
    }

    if (m_children & DisabledOn) {
        m_disabledOn->write(writer, QStringLiteral("disabledon"));
    }

    if (m_children & ActiveOff) {
        m_activeOff->write(writer, QStringLiteral("activeoff"));
    }

    if (m_children & ActiveOn) {
        m_activeOn->write(writer, QStringLiteral("activeon"));
    }

    if (m_children & SelectedOff) {
        m_selectedOff->write(writer, QStringLiteral("selectedoff"));
    }

    if (m_children & SelectedOn) {
        m_selectedOn->write(writer, QStringLiteral("selectedon"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomResourcePixmap* DomResourceIcon::takeElementNormalOff() 
{
    DomResourcePixmap* a = m_normalOff;
    m_normalOff = 0;
    m_children ^= NormalOff;
    return a;
}

void DomResourceIcon::setElementNormalOff(DomResourcePixmap* a)
{
    delete m_normalOff;
    m_children |= NormalOff;
    m_normalOff = a;
}

DomResourcePixmap* DomResourceIcon::takeElementNormalOn() 
{
    DomResourcePixmap* a = m_normalOn;
    m_normalOn = 0;
    m_children ^= NormalOn;
    return a;
}

void DomResourceIcon::setElementNormalOn(DomResourcePixmap* a)
{
    delete m_normalOn;
    m_children |= NormalOn;
    m_normalOn = a;
}

DomResourcePixmap* DomResourceIcon::takeElementDisabledOff() 
{
    DomResourcePixmap* a = m_disabledOff;
    m_disabledOff = 0;
    m_children ^= DisabledOff;
    return a;
}

void DomResourceIcon::setElementDisabledOff(DomResourcePixmap* a)
{
    delete m_disabledOff;
    m_children |= DisabledOff;
    m_disabledOff = a;
}

DomResourcePixmap* DomResourceIcon::takeElementDisabledOn() 
{
    DomResourcePixmap* a = m_disabledOn;
    m_disabledOn = 0;
    m_children ^= DisabledOn;
    return a;
}

void DomResourceIcon::setElementDisabledOn(DomResourcePixmap* a)
{
    delete m_disabledOn;
    m_children |= DisabledOn;
    m_disabledOn = a;
}

DomResourcePixmap* DomResourceIcon::takeElementActiveOff() 
{
    DomResourcePixmap* a = m_activeOff;
    m_activeOff = 0;
    m_children ^= ActiveOff;
    return a;
}

void DomResourceIcon::setElementActiveOff(DomResourcePixmap* a)
{
    delete m_activeOff;
    m_children |= ActiveOff;
    m_activeOff = a;
}

DomResourcePixmap* DomResourceIcon::takeElementActiveOn() 
{
    DomResourcePixmap* a = m_activeOn;
    m_activeOn = 0;
    m_children ^= ActiveOn;
    return a;
}

void DomResourceIcon::setElementActiveOn(DomResourcePixmap* a)
{
    delete m_activeOn;
    m_children |= ActiveOn;
    m_activeOn = a;
}

DomResourcePixmap* DomResourceIcon::takeElementSelectedOff() 
{
    DomResourcePixmap* a = m_selectedOff;
    m_selectedOff = 0;
    m_children ^= SelectedOff;
    return a;
}

void DomResourceIcon::setElementSelectedOff(DomResourcePixmap* a)
{
    delete m_selectedOff;
    m_children |= SelectedOff;
    m_selectedOff = a;
}

DomResourcePixmap* DomResourceIcon::takeElementSelectedOn() 
{
    DomResourcePixmap* a = m_selectedOn;
    m_selectedOn = 0;
    m_children ^= SelectedOn;
    return a;
}

void DomResourceIcon::setElementSelectedOn(DomResourcePixmap* a)
{
    delete m_selectedOn;
    m_children |= SelectedOn;
    m_selectedOn = a;
}

void DomResourceIcon::clearElementNormalOff()
{
    delete m_normalOff;
    m_normalOff = 0;
    m_children &= ~NormalOff;
}

void DomResourceIcon::clearElementNormalOn()
{
    delete m_normalOn;
    m_normalOn = 0;
    m_children &= ~NormalOn;
}

void DomResourceIcon::clearElementDisabledOff()
{
    delete m_disabledOff;
    m_disabledOff = 0;
    m_children &= ~DisabledOff;
}

void DomResourceIcon::clearElementDisabledOn()
{
    delete m_disabledOn;
    m_disabledOn = 0;
    m_children &= ~DisabledOn;
}

void DomResourceIcon::clearElementActiveOff()
{
    delete m_activeOff;
    m_activeOff = 0;
    m_children &= ~ActiveOff;
}

void DomResourceIcon::clearElementActiveOn()
{
    delete m_activeOn;
    m_activeOn = 0;
    m_children &= ~ActiveOn;
}

void DomResourceIcon::clearElementSelectedOff()
{
    delete m_selectedOff;
    m_selectedOff = 0;
    m_children &= ~SelectedOff;
}

void DomResourceIcon::clearElementSelectedOn()
{
    delete m_selectedOn;
    m_selectedOn = 0;
    m_children &= ~SelectedOn;
}

void DomString::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_notr = false;
    m_has_attr_comment = false;
    m_has_attr_extraComment = false;
    }

    m_children = 0;
}

DomString::DomString()
{
    m_children = 0;
    m_has_attr_notr = false;
    m_has_attr_comment = false;
    m_has_attr_extraComment = false;
    m_text.clear();
}

DomString::~DomString()
{
}

void DomString::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("notr")) {
            setAttributeNotr(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("comment")) {
            setAttributeComment(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("extracomment")) {
            setAttributeExtraComment(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomString::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("string") : tagName.toLower());

    if (hasAttributeNotr())
        writer.writeAttribute(QStringLiteral("notr"), attributeNotr());

    if (hasAttributeComment())
        writer.writeAttribute(QStringLiteral("comment"), attributeComment());

    if (hasAttributeExtraComment())
        writer.writeAttribute(QStringLiteral("extracomment"), attributeExtraComment());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomPointF::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_x = 0;
    m_y = 0;
}

DomPointF::DomPointF()
{
    m_children = 0;
    m_x = 0;
    m_y = 0;
}

DomPointF::~DomPointF()
{
}

void DomPointF::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QString(QLatin1Char('x'))) {
                setElementX(reader.readElementText().toDouble());
                continue;
            }
            if (tag == QString(QLatin1Char('y'))) {
                setElementY(reader.readElementText().toDouble());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomPointF::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("pointf") : tagName.toLower());

    if (m_children & X) {
        writer.writeTextElement(QString(QLatin1Char('x')), QString::number(m_x, 'f', 15));
    }

    if (m_children & Y) {
        writer.writeTextElement(QString(QLatin1Char('y')), QString::number(m_y, 'f', 15));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomPointF::setElementX(double a)
{
    m_children |= X;
    m_x = a;
}

void DomPointF::setElementY(double a)
{
    m_children |= Y;
    m_y = a;
}

void DomPointF::clearElementX()
{
    m_children &= ~X;
}

void DomPointF::clearElementY()
{
    m_children &= ~Y;
}

void DomRectF::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
}

DomRectF::DomRectF()
{
    m_children = 0;
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
}

DomRectF::~DomRectF()
{
}

void DomRectF::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QString(QLatin1Char('x'))) {
                setElementX(reader.readElementText().toDouble());
                continue;
            }
            if (tag == QString(QLatin1Char('y'))) {
                setElementY(reader.readElementText().toDouble());
                continue;
            }
            if (tag == QStringLiteral("width")) {
                setElementWidth(reader.readElementText().toDouble());
                continue;
            }
            if (tag == QStringLiteral("height")) {
                setElementHeight(reader.readElementText().toDouble());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomRectF::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("rectf") : tagName.toLower());

    if (m_children & X) {
        writer.writeTextElement(QString(QLatin1Char('x')), QString::number(m_x, 'f', 15));
    }

    if (m_children & Y) {
        writer.writeTextElement(QString(QLatin1Char('y')), QString::number(m_y, 'f', 15));
    }

    if (m_children & Width) {
        writer.writeTextElement(QStringLiteral("width"), QString::number(m_width, 'f', 15));
    }

    if (m_children & Height) {
        writer.writeTextElement(QStringLiteral("height"), QString::number(m_height, 'f', 15));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomRectF::setElementX(double a)
{
    m_children |= X;
    m_x = a;
}

void DomRectF::setElementY(double a)
{
    m_children |= Y;
    m_y = a;
}

void DomRectF::setElementWidth(double a)
{
    m_children |= Width;
    m_width = a;
}

void DomRectF::setElementHeight(double a)
{
    m_children |= Height;
    m_height = a;
}

void DomRectF::clearElementX()
{
    m_children &= ~X;
}

void DomRectF::clearElementY()
{
    m_children &= ~Y;
}

void DomRectF::clearElementWidth()
{
    m_children &= ~Width;
}

void DomRectF::clearElementHeight()
{
    m_children &= ~Height;
}

void DomSizeF::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_width = 0;
    m_height = 0;
}

DomSizeF::DomSizeF()
{
    m_children = 0;
    m_width = 0;
    m_height = 0;
}

DomSizeF::~DomSizeF()
{
}

void DomSizeF::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("width")) {
                setElementWidth(reader.readElementText().toDouble());
                continue;
            }
            if (tag == QStringLiteral("height")) {
                setElementHeight(reader.readElementText().toDouble());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomSizeF::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("sizef") : tagName.toLower());

    if (m_children & Width) {
        writer.writeTextElement(QStringLiteral("width"), QString::number(m_width, 'f', 15));
    }

    if (m_children & Height) {
        writer.writeTextElement(QStringLiteral("height"), QString::number(m_height, 'f', 15));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSizeF::setElementWidth(double a)
{
    m_children |= Width;
    m_width = a;
}

void DomSizeF::setElementHeight(double a)
{
    m_children |= Height;
    m_height = a;
}

void DomSizeF::clearElementWidth()
{
    m_children &= ~Width;
}

void DomSizeF::clearElementHeight()
{
    m_children &= ~Height;
}

void DomChar::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_unicode = 0;
}

DomChar::DomChar()
{
    m_children = 0;
    m_unicode = 0;
}

DomChar::~DomChar()
{
}

void DomChar::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("unicode")) {
                setElementUnicode(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomChar::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("char") : tagName.toLower());

    if (m_children & Unicode) {
        writer.writeTextElement(QStringLiteral("unicode"), QString::number(m_unicode));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomChar::setElementUnicode(int a)
{
    m_children |= Unicode;
    m_unicode = a;
}

void DomChar::clearElementUnicode()
{
    m_children &= ~Unicode;
}

void DomUrl::clear(bool clear_all)
{
    delete m_string;

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_string = 0;
}

DomUrl::DomUrl()
{
    m_children = 0;
    m_string = 0;
}

DomUrl::~DomUrl()
{
    delete m_string;
}

void DomUrl::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("string")) {
                DomString *v = new DomString();
                v->read(reader);
                setElementString(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomUrl::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("url") : tagName.toLower());

    if (m_children & String) {
        m_string->write(writer, QStringLiteral("string"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

DomString* DomUrl::takeElementString() 
{
    DomString* a = m_string;
    m_string = 0;
    m_children ^= String;
    return a;
}

void DomUrl::setElementString(DomString* a)
{
    delete m_string;
    m_children |= String;
    m_string = a;
}

void DomUrl::clearElementString()
{
    delete m_string;
    m_string = 0;
    m_children &= ~String;
}

void DomProperty::clear(bool clear_all)
{
    delete m_color;
    delete m_font;
    delete m_iconSet;
    delete m_pixmap;
    delete m_palette;
    delete m_point;
    delete m_rect;
    delete m_locale;
    delete m_sizePolicy;
    delete m_size;
    delete m_string;
    delete m_stringList;
    delete m_date;
    delete m_time;
    delete m_dateTime;
    delete m_pointF;
    delete m_rectF;
    delete m_sizeF;
    delete m_char;
    delete m_url;
    delete m_brush;

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    m_has_attr_stdset = false;
    m_attr_stdset = 0;
    }

    m_kind = Unknown;

    m_color = 0;
    m_cursor = 0;
    m_font = 0;
    m_iconSet = 0;
    m_pixmap = 0;
    m_palette = 0;
    m_point = 0;
    m_rect = 0;
    m_locale = 0;
    m_sizePolicy = 0;
    m_size = 0;
    m_string = 0;
    m_stringList = 0;
    m_number = 0;
    m_float = 0.0;
    m_double = 0;
    m_date = 0;
    m_time = 0;
    m_dateTime = 0;
    m_pointF = 0;
    m_rectF = 0;
    m_sizeF = 0;
    m_longLong = 0;
    m_char = 0;
    m_url = 0;
    m_UInt = 0;
    m_uLongLong = 0;
    m_brush = 0;
}

DomProperty::DomProperty()
{
    m_kind = Unknown;

    m_has_attr_name = false;
    m_has_attr_stdset = false;
    m_attr_stdset = 0;
    m_color = 0;
    m_cursor = 0;
    m_font = 0;
    m_iconSet = 0;
    m_pixmap = 0;
    m_palette = 0;
    m_point = 0;
    m_rect = 0;
    m_locale = 0;
    m_sizePolicy = 0;
    m_size = 0;
    m_string = 0;
    m_stringList = 0;
    m_number = 0;
    m_float = 0.0;
    m_double = 0;
    m_date = 0;
    m_time = 0;
    m_dateTime = 0;
    m_pointF = 0;
    m_rectF = 0;
    m_sizeF = 0;
    m_longLong = 0;
    m_char = 0;
    m_url = 0;
    m_UInt = 0;
    m_uLongLong = 0;
    m_brush = 0;
}

DomProperty::~DomProperty()
{
    delete m_color;
    delete m_font;
    delete m_iconSet;
    delete m_pixmap;
    delete m_palette;
    delete m_point;
    delete m_rect;
    delete m_locale;
    delete m_sizePolicy;
    delete m_size;
    delete m_string;
    delete m_stringList;
    delete m_date;
    delete m_time;
    delete m_dateTime;
    delete m_pointF;
    delete m_rectF;
    delete m_sizeF;
    delete m_char;
    delete m_url;
    delete m_brush;
}

void DomProperty::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("stdset")) {
            setAttributeStdset(attribute.value().toString().toInt());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("bool")) {
                setElementBool(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("color")) {
                DomColor *v = new DomColor();
                v->read(reader);
                setElementColor(v);
                continue;
            }
            if (tag == QStringLiteral("cstring")) {
                setElementCstring(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("cursor")) {
                setElementCursor(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("cursorshape")) {
                setElementCursorShape(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("enum")) {
                setElementEnum(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("font")) {
                DomFont *v = new DomFont();
                v->read(reader);
                setElementFont(v);
                continue;
            }
            if (tag == QStringLiteral("iconset")) {
                DomResourceIcon *v = new DomResourceIcon();
                v->read(reader);
                setElementIconSet(v);
                continue;
            }
            if (tag == QStringLiteral("pixmap")) {
                DomResourcePixmap *v = new DomResourcePixmap();
                v->read(reader);
                setElementPixmap(v);
                continue;
            }
            if (tag == QStringLiteral("palette")) {
                DomPalette *v = new DomPalette();
                v->read(reader);
                setElementPalette(v);
                continue;
            }
            if (tag == QStringLiteral("point")) {
                DomPoint *v = new DomPoint();
                v->read(reader);
                setElementPoint(v);
                continue;
            }
            if (tag == QStringLiteral("rect")) {
                DomRect *v = new DomRect();
                v->read(reader);
                setElementRect(v);
                continue;
            }
            if (tag == QStringLiteral("set")) {
                setElementSet(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("locale")) {
                DomLocale *v = new DomLocale();
                v->read(reader);
                setElementLocale(v);
                continue;
            }
            if (tag == QStringLiteral("sizepolicy")) {
                DomSizePolicy *v = new DomSizePolicy();
                v->read(reader);
                setElementSizePolicy(v);
                continue;
            }
            if (tag == QStringLiteral("size")) {
                DomSize *v = new DomSize();
                v->read(reader);
                setElementSize(v);
                continue;
            }
            if (tag == QStringLiteral("string")) {
                DomString *v = new DomString();
                v->read(reader);
                setElementString(v);
                continue;
            }
            if (tag == QStringLiteral("stringlist")) {
                DomStringList *v = new DomStringList();
                v->read(reader);
                setElementStringList(v);
                continue;
            }
            if (tag == QStringLiteral("number")) {
                setElementNumber(reader.readElementText().toInt());
                continue;
            }
            if (tag == QStringLiteral("float")) {
                setElementFloat(reader.readElementText().toFloat());
                continue;
            }
            if (tag == QStringLiteral("double")) {
                setElementDouble(reader.readElementText().toDouble());
                continue;
            }
            if (tag == QStringLiteral("date")) {
                DomDate *v = new DomDate();
                v->read(reader);
                setElementDate(v);
                continue;
            }
            if (tag == QStringLiteral("time")) {
                DomTime *v = new DomTime();
                v->read(reader);
                setElementTime(v);
                continue;
            }
            if (tag == QStringLiteral("datetime")) {
                DomDateTime *v = new DomDateTime();
                v->read(reader);
                setElementDateTime(v);
                continue;
            }
            if (tag == QStringLiteral("pointf")) {
                DomPointF *v = new DomPointF();
                v->read(reader);
                setElementPointF(v);
                continue;
            }
            if (tag == QStringLiteral("rectf")) {
                DomRectF *v = new DomRectF();
                v->read(reader);
                setElementRectF(v);
                continue;
            }
            if (tag == QStringLiteral("sizef")) {
                DomSizeF *v = new DomSizeF();
                v->read(reader);
                setElementSizeF(v);
                continue;
            }
            if (tag == QStringLiteral("longlong")) {
                setElementLongLong(reader.readElementText().toLongLong());
                continue;
            }
            if (tag == QStringLiteral("char")) {
                DomChar *v = new DomChar();
                v->read(reader);
                setElementChar(v);
                continue;
            }
            if (tag == QStringLiteral("url")) {
                DomUrl *v = new DomUrl();
                v->read(reader);
                setElementUrl(v);
                continue;
            }
            if (tag == QStringLiteral("uint")) {
                setElementUInt(reader.readElementText().toUInt());
                continue;
            }
            if (tag == QStringLiteral("ulonglong")) {
                setElementULongLong(reader.readElementText().toULongLong());
                continue;
            }
            if (tag == QStringLiteral("brush")) {
                DomBrush *v = new DomBrush();
                v->read(reader);
                setElementBrush(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomProperty::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("property") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (hasAttributeStdset())
        writer.writeAttribute(QStringLiteral("stdset"), QString::number(attributeStdset()));

    switch (kind()) {
        case Bool: {
            writer.writeTextElement(QStringLiteral("bool"), elementBool());
            break;
        }
        case Color: {
            DomColor* v = elementColor();
            if (v != 0) {
                v->write(writer, QStringLiteral("color"));
            }
            break;
        }
        case Cstring: {
            writer.writeTextElement(QStringLiteral("cstring"), elementCstring());
            break;
        }
        case Cursor: {
            writer.writeTextElement(QStringLiteral("cursor"), QString::number(elementCursor()));
            break;
        }
        case CursorShape: {
            writer.writeTextElement(QStringLiteral("cursorShape"), elementCursorShape());
            break;
        }
        case Enum: {
            writer.writeTextElement(QStringLiteral("enum"), elementEnum());
            break;
        }
        case Font: {
            DomFont* v = elementFont();
            if (v != 0) {
                v->write(writer, QStringLiteral("font"));
            }
            break;
        }
        case IconSet: {
            DomResourceIcon* v = elementIconSet();
            if (v != 0) {
                v->write(writer, QStringLiteral("iconset"));
            }
            break;
        }
        case Pixmap: {
            DomResourcePixmap* v = elementPixmap();
            if (v != 0) {
                v->write(writer, QStringLiteral("pixmap"));
            }
            break;
        }
        case Palette: {
            DomPalette* v = elementPalette();
            if (v != 0) {
                v->write(writer, QStringLiteral("palette"));
            }
            break;
        }
        case Point: {
            DomPoint* v = elementPoint();
            if (v != 0) {
                v->write(writer, QStringLiteral("point"));
            }
            break;
        }
        case Rect: {
            DomRect* v = elementRect();
            if (v != 0) {
                v->write(writer, QStringLiteral("rect"));
            }
            break;
        }
        case Set: {
            writer.writeTextElement(QStringLiteral("set"), elementSet());
            break;
        }
        case Locale: {
            DomLocale* v = elementLocale();
            if (v != 0) {
                v->write(writer, QStringLiteral("locale"));
            }
            break;
        }
        case SizePolicy: {
            DomSizePolicy* v = elementSizePolicy();
            if (v != 0) {
                v->write(writer, QStringLiteral("sizepolicy"));
            }
            break;
        }
        case Size: {
            DomSize* v = elementSize();
            if (v != 0) {
                v->write(writer, QStringLiteral("size"));
            }
            break;
        }
        case String: {
            DomString* v = elementString();
            if (v != 0) {
                v->write(writer, QStringLiteral("string"));
            }
            break;
        }
        case StringList: {
            DomStringList* v = elementStringList();
            if (v != 0) {
                v->write(writer, QStringLiteral("stringlist"));
            }
            break;
        }
        case Number: {
            writer.writeTextElement(QStringLiteral("number"), QString::number(elementNumber()));
            break;
        }
        case Float: {
            writer.writeTextElement(QStringLiteral("float"), QString::number(elementFloat(), 'f', 8));
            break;
        }
        case Double: {
            writer.writeTextElement(QStringLiteral("double"), QString::number(elementDouble(), 'f', 15));
            break;
        }
        case Date: {
            DomDate* v = elementDate();
            if (v != 0) {
                v->write(writer, QStringLiteral("date"));
            }
            break;
        }
        case Time: {
            DomTime* v = elementTime();
            if (v != 0) {
                v->write(writer, QStringLiteral("time"));
            }
            break;
        }
        case DateTime: {
            DomDateTime* v = elementDateTime();
            if (v != 0) {
                v->write(writer, QStringLiteral("datetime"));
            }
            break;
        }
        case PointF: {
            DomPointF* v = elementPointF();
            if (v != 0) {
                v->write(writer, QStringLiteral("pointf"));
            }
            break;
        }
        case RectF: {
            DomRectF* v = elementRectF();
            if (v != 0) {
                v->write(writer, QStringLiteral("rectf"));
            }
            break;
        }
        case SizeF: {
            DomSizeF* v = elementSizeF();
            if (v != 0) {
                v->write(writer, QStringLiteral("sizef"));
            }
            break;
        }
        case LongLong: {
            writer.writeTextElement(QStringLiteral("longLong"), QString::number(elementLongLong()));
            break;
        }
        case Char: {
            DomChar* v = elementChar();
            if (v != 0) {
                v->write(writer, QStringLiteral("char"));
            }
            break;
        }
        case Url: {
            DomUrl* v = elementUrl();
            if (v != 0) {
                v->write(writer, QStringLiteral("url"));
            }
            break;
        }
        case UInt: {
            writer.writeTextElement(QStringLiteral("UInt"), QString::number(elementUInt()));
            break;
        }
        case ULongLong: {
            writer.writeTextElement(QStringLiteral("uLongLong"), QString::number(elementULongLong()));
            break;
        }
        case Brush: {
            DomBrush* v = elementBrush();
            if (v != 0) {
                v->write(writer, QStringLiteral("brush"));
            }
            break;
        }
        default:
            break;
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomProperty::setElementBool(const QString& a)
{
    clear(false);
    m_kind = Bool;
    m_bool = a;
}

DomColor* DomProperty::takeElementColor() 
{
    DomColor* a = m_color;
    m_color = 0;
    return a;
}

void DomProperty::setElementColor(DomColor* a)
{
    clear(false);
    m_kind = Color;
    m_color = a;
}

void DomProperty::setElementCstring(const QString& a)
{
    clear(false);
    m_kind = Cstring;
    m_cstring = a;
}

void DomProperty::setElementCursor(int a)
{
    clear(false);
    m_kind = Cursor;
    m_cursor = a;
}

void DomProperty::setElementCursorShape(const QString& a)
{
    clear(false);
    m_kind = CursorShape;
    m_cursorShape = a;
}

void DomProperty::setElementEnum(const QString& a)
{
    clear(false);
    m_kind = Enum;
    m_enum = a;
}

DomFont* DomProperty::takeElementFont() 
{
    DomFont* a = m_font;
    m_font = 0;
    return a;
}

void DomProperty::setElementFont(DomFont* a)
{
    clear(false);
    m_kind = Font;
    m_font = a;
}

DomResourceIcon* DomProperty::takeElementIconSet() 
{
    DomResourceIcon* a = m_iconSet;
    m_iconSet = 0;
    return a;
}

void DomProperty::setElementIconSet(DomResourceIcon* a)
{
    clear(false);
    m_kind = IconSet;
    m_iconSet = a;
}

DomResourcePixmap* DomProperty::takeElementPixmap() 
{
    DomResourcePixmap* a = m_pixmap;
    m_pixmap = 0;
    return a;
}

void DomProperty::setElementPixmap(DomResourcePixmap* a)
{
    clear(false);
    m_kind = Pixmap;
    m_pixmap = a;
}

DomPalette* DomProperty::takeElementPalette() 
{
    DomPalette* a = m_palette;
    m_palette = 0;
    return a;
}

void DomProperty::setElementPalette(DomPalette* a)
{
    clear(false);
    m_kind = Palette;
    m_palette = a;
}

DomPoint* DomProperty::takeElementPoint() 
{
    DomPoint* a = m_point;
    m_point = 0;
    return a;
}

void DomProperty::setElementPoint(DomPoint* a)
{
    clear(false);
    m_kind = Point;
    m_point = a;
}

DomRect* DomProperty::takeElementRect() 
{
    DomRect* a = m_rect;
    m_rect = 0;
    return a;
}

void DomProperty::setElementRect(DomRect* a)
{
    clear(false);
    m_kind = Rect;
    m_rect = a;
}

void DomProperty::setElementSet(const QString& a)
{
    clear(false);
    m_kind = Set;
    m_set = a;
}

DomLocale* DomProperty::takeElementLocale() 
{
    DomLocale* a = m_locale;
    m_locale = 0;
    return a;
}

void DomProperty::setElementLocale(DomLocale* a)
{
    clear(false);
    m_kind = Locale;
    m_locale = a;
}

DomSizePolicy* DomProperty::takeElementSizePolicy() 
{
    DomSizePolicy* a = m_sizePolicy;
    m_sizePolicy = 0;
    return a;
}

void DomProperty::setElementSizePolicy(DomSizePolicy* a)
{
    clear(false);
    m_kind = SizePolicy;
    m_sizePolicy = a;
}

DomSize* DomProperty::takeElementSize() 
{
    DomSize* a = m_size;
    m_size = 0;
    return a;
}

void DomProperty::setElementSize(DomSize* a)
{
    clear(false);
    m_kind = Size;
    m_size = a;
}

DomString* DomProperty::takeElementString() 
{
    DomString* a = m_string;
    m_string = 0;
    return a;
}

void DomProperty::setElementString(DomString* a)
{
    clear(false);
    m_kind = String;
    m_string = a;
}

DomStringList* DomProperty::takeElementStringList() 
{
    DomStringList* a = m_stringList;
    m_stringList = 0;
    return a;
}

void DomProperty::setElementStringList(DomStringList* a)
{
    clear(false);
    m_kind = StringList;
    m_stringList = a;
}

void DomProperty::setElementNumber(int a)
{
    clear(false);
    m_kind = Number;
    m_number = a;
}

void DomProperty::setElementFloat(float a)
{
    clear(false);
    m_kind = Float;
    m_float = a;
}

void DomProperty::setElementDouble(double a)
{
    clear(false);
    m_kind = Double;
    m_double = a;
}

DomDate* DomProperty::takeElementDate() 
{
    DomDate* a = m_date;
    m_date = 0;
    return a;
}

void DomProperty::setElementDate(DomDate* a)
{
    clear(false);
    m_kind = Date;
    m_date = a;
}

DomTime* DomProperty::takeElementTime() 
{
    DomTime* a = m_time;
    m_time = 0;
    return a;
}

void DomProperty::setElementTime(DomTime* a)
{
    clear(false);
    m_kind = Time;
    m_time = a;
}

DomDateTime* DomProperty::takeElementDateTime() 
{
    DomDateTime* a = m_dateTime;
    m_dateTime = 0;
    return a;
}

void DomProperty::setElementDateTime(DomDateTime* a)
{
    clear(false);
    m_kind = DateTime;
    m_dateTime = a;
}

DomPointF* DomProperty::takeElementPointF() 
{
    DomPointF* a = m_pointF;
    m_pointF = 0;
    return a;
}

void DomProperty::setElementPointF(DomPointF* a)
{
    clear(false);
    m_kind = PointF;
    m_pointF = a;
}

DomRectF* DomProperty::takeElementRectF() 
{
    DomRectF* a = m_rectF;
    m_rectF = 0;
    return a;
}

void DomProperty::setElementRectF(DomRectF* a)
{
    clear(false);
    m_kind = RectF;
    m_rectF = a;
}

DomSizeF* DomProperty::takeElementSizeF() 
{
    DomSizeF* a = m_sizeF;
    m_sizeF = 0;
    return a;
}

void DomProperty::setElementSizeF(DomSizeF* a)
{
    clear(false);
    m_kind = SizeF;
    m_sizeF = a;
}

void DomProperty::setElementLongLong(qlonglong a)
{
    clear(false);
    m_kind = LongLong;
    m_longLong = a;
}

DomChar* DomProperty::takeElementChar() 
{
    DomChar* a = m_char;
    m_char = 0;
    return a;
}

void DomProperty::setElementChar(DomChar* a)
{
    clear(false);
    m_kind = Char;
    m_char = a;
}

DomUrl* DomProperty::takeElementUrl() 
{
    DomUrl* a = m_url;
    m_url = 0;
    return a;
}

void DomProperty::setElementUrl(DomUrl* a)
{
    clear(false);
    m_kind = Url;
    m_url = a;
}

void DomProperty::setElementUInt(uint a)
{
    clear(false);
    m_kind = UInt;
    m_UInt = a;
}

void DomProperty::setElementULongLong(qulonglong a)
{
    clear(false);
    m_kind = ULongLong;
    m_uLongLong = a;
}

DomBrush* DomProperty::takeElementBrush() 
{
    DomBrush* a = m_brush;
    m_brush = 0;
    return a;
}

void DomProperty::setElementBrush(DomBrush* a)
{
    clear(false);
    m_kind = Brush;
    m_brush = a;
}

void DomConnections::clear(bool clear_all)
{
    qDeleteAll(m_connection);
    m_connection.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomConnections::DomConnections()
{
    m_children = 0;
}

DomConnections::~DomConnections()
{
    qDeleteAll(m_connection);
    m_connection.clear();
}

void DomConnections::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("connection")) {
                DomConnection *v = new DomConnection();
                v->read(reader);
                m_connection.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomConnections::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("connections") : tagName.toLower());

    for (int i = 0; i < m_connection.size(); ++i) {
        DomConnection* v = m_connection[i];
        v->write(writer, QStringLiteral("connection"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomConnections::setElementConnection(const QList<DomConnection*>& a)
{
    m_children |= Connection;
    m_connection = a;
}

void DomConnection::clear(bool clear_all)
{
    delete m_hints;

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
    m_hints = 0;
}

DomConnection::DomConnection()
{
    m_children = 0;
    m_hints = 0;
}

DomConnection::~DomConnection()
{
    delete m_hints;
}

void DomConnection::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("sender")) {
                setElementSender(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("signal")) {
                setElementSignal(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("receiver")) {
                setElementReceiver(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("slot")) {
                setElementSlot(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("hints")) {
                DomConnectionHints *v = new DomConnectionHints();
                v->read(reader);
                setElementHints(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomConnection::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("connection") : tagName.toLower());

    if (m_children & Sender) {
        writer.writeTextElement(QStringLiteral("sender"), m_sender);
    }

    if (m_children & Signal) {
        writer.writeTextElement(QStringLiteral("signal"), m_signal);
    }

    if (m_children & Receiver) {
        writer.writeTextElement(QStringLiteral("receiver"), m_receiver);
    }

    if (m_children & Slot) {
        writer.writeTextElement(QStringLiteral("slot"), m_slot);
    }

    if (m_children & Hints) {
        m_hints->write(writer, QStringLiteral("hints"));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomConnection::setElementSender(const QString& a)
{
    m_children |= Sender;
    m_sender = a;
}

void DomConnection::setElementSignal(const QString& a)
{
    m_children |= Signal;
    m_signal = a;
}

void DomConnection::setElementReceiver(const QString& a)
{
    m_children |= Receiver;
    m_receiver = a;
}

void DomConnection::setElementSlot(const QString& a)
{
    m_children |= Slot;
    m_slot = a;
}

DomConnectionHints* DomConnection::takeElementHints() 
{
    DomConnectionHints* a = m_hints;
    m_hints = 0;
    m_children ^= Hints;
    return a;
}

void DomConnection::setElementHints(DomConnectionHints* a)
{
    delete m_hints;
    m_children |= Hints;
    m_hints = a;
}

void DomConnection::clearElementSender()
{
    m_children &= ~Sender;
}

void DomConnection::clearElementSignal()
{
    m_children &= ~Signal;
}

void DomConnection::clearElementReceiver()
{
    m_children &= ~Receiver;
}

void DomConnection::clearElementSlot()
{
    m_children &= ~Slot;
}

void DomConnection::clearElementHints()
{
    delete m_hints;
    m_hints = 0;
    m_children &= ~Hints;
}

void DomConnectionHints::clear(bool clear_all)
{
    qDeleteAll(m_hint);
    m_hint.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomConnectionHints::DomConnectionHints()
{
    m_children = 0;
}

DomConnectionHints::~DomConnectionHints()
{
    qDeleteAll(m_hint);
    m_hint.clear();
}

void DomConnectionHints::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("hint")) {
                DomConnectionHint *v = new DomConnectionHint();
                v->read(reader);
                m_hint.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomConnectionHints::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("connectionhints") : tagName.toLower());

    for (int i = 0; i < m_hint.size(); ++i) {
        DomConnectionHint* v = m_hint[i];
        v->write(writer, QStringLiteral("hint"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomConnectionHints::setElementHint(const QList<DomConnectionHint*>& a)
{
    m_children |= Hint;
    m_hint = a;
}

void DomConnectionHint::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_type = false;
    }

    m_children = 0;
    m_x = 0;
    m_y = 0;
}

DomConnectionHint::DomConnectionHint()
{
    m_children = 0;
    m_has_attr_type = false;
    m_x = 0;
    m_y = 0;
}

DomConnectionHint::~DomConnectionHint()
{
}

void DomConnectionHint::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("type")) {
            setAttributeType(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QString(QLatin1Char('x'))) {
                setElementX(reader.readElementText().toInt());
                continue;
            }
            if (tag == QString(QLatin1Char('y'))) {
                setElementY(reader.readElementText().toInt());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomConnectionHint::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("connectionhint") : tagName.toLower());

    if (hasAttributeType())
        writer.writeAttribute(QStringLiteral("type"), attributeType());

    if (m_children & X) {
        writer.writeTextElement(QString(QLatin1Char('x')), QString::number(m_x));
    }

    if (m_children & Y) {
        writer.writeTextElement(QString(QLatin1Char('y')), QString::number(m_y));
    }

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomConnectionHint::setElementX(int a)
{
    m_children |= X;
    m_x = a;
}

void DomConnectionHint::setElementY(int a)
{
    m_children |= Y;
    m_y = a;
}

void DomConnectionHint::clearElementX()
{
    m_children &= ~X;
}

void DomConnectionHint::clearElementY()
{
    m_children &= ~Y;
}

void DomScript::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_source = false;
    m_has_attr_language = false;
    }

    m_children = 0;
}

DomScript::DomScript()
{
    m_children = 0;
    m_has_attr_source = false;
    m_has_attr_language = false;
}

DomScript::~DomScript()
{
}

void DomScript::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("source")) {
            setAttributeSource(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("language")) {
            setAttributeLanguage(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomScript::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("script") : tagName.toLower());

    if (hasAttributeSource())
        writer.writeAttribute(QStringLiteral("source"), attributeSource());

    if (hasAttributeLanguage())
        writer.writeAttribute(QStringLiteral("language"), attributeLanguage());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomWidgetData::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomWidgetData::DomWidgetData()
{
    m_children = 0;
}

DomWidgetData::~DomWidgetData()
{
    qDeleteAll(m_property);
    m_property.clear();
}

void DomWidgetData::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomWidgetData::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("widgetdata") : tagName.toLower());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomWidgetData::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomDesignerData::clear(bool clear_all)
{
    qDeleteAll(m_property);
    m_property.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomDesignerData::DomDesignerData()
{
    m_children = 0;
}

DomDesignerData::~DomDesignerData()
{
    qDeleteAll(m_property);
    m_property.clear();
}

void DomDesignerData::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("property")) {
                DomProperty *v = new DomProperty();
                v->read(reader);
                m_property.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomDesignerData::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("designerdata") : tagName.toLower());

    for (int i = 0; i < m_property.size(); ++i) {
        DomProperty* v = m_property[i];
        v->write(writer, QStringLiteral("property"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomDesignerData::setElementProperty(const QList<DomProperty*>& a)
{
    m_children |= Property;
    m_property = a;
}

void DomSlots::clear(bool clear_all)
{
    m_signal.clear();
    m_slot.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomSlots::DomSlots()
{
    m_children = 0;
}

DomSlots::~DomSlots()
{
    m_signal.clear();
    m_slot.clear();
}

void DomSlots::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("signal")) {
                m_signal.append(reader.readElementText());
                continue;
            }
            if (tag == QStringLiteral("slot")) {
                m_slot.append(reader.readElementText());
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomSlots::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("slots") : tagName.toLower());

    for (int i = 0; i < m_signal.size(); ++i) {
        QString v = m_signal[i];
        writer.writeTextElement(QStringLiteral("signal"), v);
    }
    for (int i = 0; i < m_slot.size(); ++i) {
        QString v = m_slot[i];
        writer.writeTextElement(QStringLiteral("slot"), v);
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomSlots::setElementSignal(const QStringList& a)
{
    m_children |= Signal;
    m_signal = a;
}

void DomSlots::setElementSlot(const QStringList& a)
{
    m_children |= Slot;
    m_slot = a;
}

void DomPropertySpecifications::clear(bool clear_all)
{
    qDeleteAll(m_stringpropertyspecification);
    m_stringpropertyspecification.clear();

    if (clear_all) {
    m_text.clear();
    }

    m_children = 0;
}

DomPropertySpecifications::DomPropertySpecifications()
{
    m_children = 0;
}

DomPropertySpecifications::~DomPropertySpecifications()
{
    qDeleteAll(m_stringpropertyspecification);
    m_stringpropertyspecification.clear();
}

void DomPropertySpecifications::read(QXmlStreamReader &reader)
{

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            if (tag == QStringLiteral("stringpropertyspecification")) {
                DomStringPropertySpecification *v = new DomStringPropertySpecification();
                v->read(reader);
                m_stringpropertyspecification.append(v);
                continue;
            }
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomPropertySpecifications::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("propertyspecifications") : tagName.toLower());

    for (int i = 0; i < m_stringpropertyspecification.size(); ++i) {
        DomStringPropertySpecification* v = m_stringpropertyspecification[i];
        v->write(writer, QStringLiteral("stringpropertyspecification"));
    }
    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

void DomPropertySpecifications::setElementStringpropertyspecification(const QList<DomStringPropertySpecification*>& a)
{
    m_children |= Stringpropertyspecification;
    m_stringpropertyspecification = a;
}

void DomStringPropertySpecification::clear(bool clear_all)
{

    if (clear_all) {
    m_text.clear();
    m_has_attr_name = false;
    m_has_attr_type = false;
    m_has_attr_notr = false;
    }

    m_children = 0;
}

DomStringPropertySpecification::DomStringPropertySpecification()
{
    m_children = 0;
    m_has_attr_name = false;
    m_has_attr_type = false;
    m_has_attr_notr = false;
}

DomStringPropertySpecification::~DomStringPropertySpecification()
{
}

void DomStringPropertySpecification::read(QXmlStreamReader &reader)
{

    foreach (const QXmlStreamAttribute &attribute, reader.attributes()) {
        QStringRef name = attribute.name();
        if (name == QStringLiteral("name")) {
            setAttributeName(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("type")) {
            setAttributeType(attribute.value().toString());
            continue;
        }
        if (name == QStringLiteral("notr")) {
            setAttributeNotr(attribute.value().toString());
            continue;
        }
        reader.raiseError(QStringLiteral("Unexpected attribute ") + name.toString());
    }

    for (bool finished = false; !finished && !reader.hasError();) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement : {
            const QString tag = reader.name().toString().toLower();
            reader.raiseError(QStringLiteral("Unexpected element ") + tag);
        }
            break;
        case QXmlStreamReader::EndElement :
            finished = true;
            break;
        case QXmlStreamReader::Characters :
            if (!reader.isWhitespace())
                m_text.append(reader.text().toString());
            break;
        default :
            break;
        }
    }
}

void DomStringPropertySpecification::write(QXmlStreamWriter &writer, const QString &tagName) const
{
    writer.writeStartElement(tagName.isEmpty() ? QString::fromUtf8("stringpropertyspecification") : tagName.toLower());

    if (hasAttributeName())
        writer.writeAttribute(QStringLiteral("name"), attributeName());

    if (hasAttributeType())
        writer.writeAttribute(QStringLiteral("type"), attributeType());

    if (hasAttributeNotr())
        writer.writeAttribute(QStringLiteral("notr"), attributeNotr());

    if (!m_text.isEmpty())
        writer.writeCharacters(m_text);

    writer.writeEndElement();
}

QT_END_NAMESPACE


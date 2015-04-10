/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "atspiadaptor_p.h"

#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <qdbusmessage.h>
#include <qdbusreply.h>
#include <qclipboard.h>

#include <qdebug.h>

#include "socket_interface.h"
#include "constant_mappings_p.h"

#include "application_p.h"
/*!
    \class AtSpiAdaptor
    \internal

    \brief AtSpiAdaptor is the main class to forward between QAccessibleInterface and AT-SPI DBus

    AtSpiAdaptor implements the functions specified in all at-spi interfaces.
    It sends notifications coming from Qt via dbus and listens to incoming dbus requests.
*/

QT_BEGIN_NAMESPACE

static bool isDebugging = false;
#define qAtspiDebug              if (!::isDebugging); else qDebug

AtSpiAdaptor::AtSpiAdaptor(DBusConnection *connection, QObject *parent)
    : QDBusVirtualObject(parent), m_dbus(connection)
    , sendFocus(0)
    , sendObject(0)
    , sendObject_active_descendant_changed(0)
    , sendObject_attributes_changed(0)
    , sendObject_bounds_changed(0)
    , sendObject_children_changed(0)
//    , sendObject_children_changed_add(0)
//    , sendObject_children_changed_remove(0)
    , sendObject_column_deleted(0)
    , sendObject_column_inserted(0)
    , sendObject_column_reordered(0)
    , sendObject_link_selected(0)
    , sendObject_model_changed(0)
    , sendObject_property_change(0)
    , sendObject_property_change_accessible_description(0)
    , sendObject_property_change_accessible_name(0)
    , sendObject_property_change_accessible_parent(0)
    , sendObject_property_change_accessible_role(0)
    , sendObject_property_change_accessible_table_caption(0)
    , sendObject_property_change_accessible_table_column_description(0)
    , sendObject_property_change_accessible_table_column_header(0)
    , sendObject_property_change_accessible_table_row_description(0)
    , sendObject_property_change_accessible_table_row_header(0)
    , sendObject_property_change_accessible_table_summary(0)
    , sendObject_property_change_accessible_value(0)
    , sendObject_row_deleted(0)
    , sendObject_row_inserted(0)
    , sendObject_row_reordered(0)
    , sendObject_selection_changed(0)
    , sendObject_state_changed(0)
    , sendObject_text_attributes_changed(0)
    , sendObject_text_bounds_changed(0)
    , sendObject_text_caret_moved(0)
    , sendObject_text_changed(0)
//    , sendObject_text_changed_delete(0)
//    , sendObject_text_changed_insert(0)
    , sendObject_text_selection_changed(0)
    , sendObject_value_changed(0)
    , sendObject_visible_data_changed(0)
    , sendWindow(0)
    , sendWindow_activate(0)
    , sendWindow_close(0)
    , sendWindow_create(0)
    , sendWindow_deactivate(0)
//    , sendWindow_desktop_create(0)
//    , sendWindow_desktop_destroy(0)
    , sendWindow_lower(0)
    , sendWindow_maximize(0)
    , sendWindow_minimize(0)
    , sendWindow_move(0)
    , sendWindow_raise(0)
    , sendWindow_reparent(0)
    , sendWindow_resize(0)
    , sendWindow_restore(0)
    , sendWindow_restyle(0)
    , sendWindow_shade(0)
    , sendWindow_unshade(0)
{
    ::isDebugging = qEnvironmentVariableIsSet("QT_DEBUG_ACCESSIBILITY");

    m_applicationAdaptor = new QSpiApplicationAdaptor(m_dbus->connection(), this);
    connect(m_applicationAdaptor, SIGNAL(windowActivated(QObject*,bool)), this, SLOT(windowActivated(QObject*,bool)));

    updateEventListeners();
    bool success = m_dbus->connection().connect(QLatin1String("org.a11y.atspi.Registry"), QLatin1String("/org/a11y/atspi/registry"),
                                                QLatin1String("org.a11y.atspi.Registry"), QLatin1String("EventListenerRegistered"), this,
                                                SLOT(eventListenerRegistered(QString,QString)));
    success = success && m_dbus->connection().connect(QLatin1String("org.a11y.atspi.Registry"), QLatin1String("/org/a11y/atspi/registry"),
                                                      QLatin1String("org.a11y.atspi.Registry"), QLatin1String("EventListenerDeregistered"), this,
                                                      SLOT(eventListenerDeregistered(QString,QString)));
#ifdef QT_ATSPI_DEBUG
    qAtspiDebug() << "Registered event listener change listener: " << success;
#endif
}

AtSpiAdaptor::~AtSpiAdaptor()
{
}

/*!
  Provide DBus introspection.
  */
QString AtSpiAdaptor::introspect(const QString &path) const
{
    static const QLatin1String accessibleIntrospection(
                "  <interface name=\"org.a11y.atspi.Accessible\">\n"
                "    <property access=\"read\" type=\"s\" name=\"Name\"/>\n"
                "    <property access=\"read\" type=\"s\" name=\"Description\"/>\n"
                "    <property access=\"read\" type=\"(so)\" name=\"Parent\">\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" type=\"i\" name=\"ChildCount\"/>\n"
                "    <method name=\"GetChildAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetChildren\">\n"
                "      <arg direction=\"out\" type=\"a(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReferenceArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetIndexInParent\">\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRelationSet\">\n"
                "      <arg direction=\"out\" type=\"a(ua(so))\"/>\n"
                "      <annotation value=\"QSpiRelationArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRole\">\n"
                "      <arg direction=\"out\" type=\"u\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRoleName\">\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetLocalizedRoleName\">\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetState\">\n"
                "      <arg direction=\"out\" type=\"au\"/>\n"
                "      <annotation value=\"QSpiUIntList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributes\">\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetApplication\">\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String actionIntrospection(
                "  <interface name=\"org.a11y.atspi.Action\">\n"
                "    <property access=\"read\" type=\"i\" name=\"NActions\"/>\n"
                "    <method name=\"GetDescription\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetName\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetKeyBinding\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetActions\">\n"
                "      <arg direction=\"out\" type=\"a(sss)\" name=\"index\"/>\n"
                "      <annotation value=\"QSpiActionArray\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"DoAction\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String applicationIntrospection(
                "  <interface name=\"org.a11y.atspi.Application\">\n"
                "    <property access=\"read\" type=\"s\" name=\"ToolkitName\"/>\n"
                "    <property access=\"read\" type=\"s\" name=\"Version\"/>\n"
                "    <property access=\"readwrite\" type=\"i\" name=\"Id\"/>\n"
                "    <method name=\"GetLocale\">\n"
                "      <arg direction=\"in\" type=\"u\" name=\"lctype\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetApplicationBusAddress\">\n"
                "      <arg direction=\"out\" type=\"s\" name=\"address\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String componentIntrospection(
                "  <interface name=\"org.a11y.atspi.Component\">\n"
                "    <method name=\"Contains\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAccessibleAtPoint\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetExtents\">\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"(iiii)\"/>\n"
                "      <annotation value=\"QSpiRect\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetPosition\">\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"y\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSize\">\n"
                "      <arg direction=\"out\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"height\"/>\n"
                "    </method>\n"
                "    <method name=\"GetLayer\">\n"
                "      <arg direction=\"out\" type=\"u\"/>\n"
                "    </method>\n"
                "    <method name=\"GetMDIZOrder\">\n"
                "      <arg direction=\"out\" type=\"n\"/>\n"
                "    </method>\n"
                "    <method name=\"GrabFocus\">\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAlpha\">\n"
                "      <arg direction=\"out\" type=\"d\"/>\n"
                "    </method>\n"
                "    <method name=\"SetExtents\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SetPosition\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coord_type\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SetSize\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String editableTextIntrospection(
                "  <interface name=\"org.a11y.atspi.EditableText\">\n"
                "    <method name=\"SetTextContents\">\n"
                "      <arg direction=\"in\" type=\"s\" name=\"newContents\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"InsertText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"position\"/>\n"
                "      <arg direction=\"in\" type=\"s\" name=\"text\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"length\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"CopyText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startPos\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endPos\"/>\n"
                "    </method>\n"
                "    <method name=\"CutText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startPos\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endPos\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"DeleteText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startPos\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endPos\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"PasteText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"position\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String tableIntrospection(
                "  <interface name=\"org.a11y.atspi.Table\">\n"
                "    <property access=\"read\" type=\"i\" name=\"NRows\"/>\n"
                "    <property access=\"read\" type=\"i\" name=\"NColumns\"/>\n"
                "    <property access=\"read\" type=\"(so)\" name=\"Caption\">\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" type=\"(so)\" name=\"Summary\">\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                "    </property>\n"
                "    <property access=\"read\" type=\"i\" name=\"NSelectedRows\"/>\n"
                "    <property access=\"read\" type=\"i\" name=\"NSelectedColumns\"/>\n"
                "    <method name=\"GetAccessibleAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetIndexAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowDescription\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnDescription\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowExtentAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnExtentAt\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowHeader\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetColumnHeader\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"(so)\"/>\n"
                "      <annotation value=\"QSpiObjectReference\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSelectedRows\">\n"
                "      <arg direction=\"out\" type=\"ai\"/>\n"
                "      <annotation value=\"QSpiIntList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetSelectedColumns\">\n"
                "      <arg direction=\"out\" type=\"ai\"/>\n"
                "      <annotation value=\"QSpiIntList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"IsRowSelected\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"IsColumnSelected\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"IsSelected\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"AddRowSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"AddColumnSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"RemoveRowSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"RemoveColumnSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"column\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRowColumnExtentsAtIndex\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"row\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"col\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"row_extents\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"col_extents\"/>\n"
                "      <arg direction=\"out\" type=\"b\" name=\"is_selected\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String textIntrospection(
                "  <interface name=\"org.a11y.atspi.Text\">\n"
                "    <property access=\"read\" type=\"i\" name=\"CharacterCount\"/>\n"
                "    <property access=\"read\" type=\"i\" name=\"CaretOffset\"/>\n"
                "    <method name=\"GetText\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "    </method>\n"
                "    <method name=\"SetCaretOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetTextBeforeOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"type\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"GetTextAtOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"type\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"GetTextAfterOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"type\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"GetCharacterAtOffset\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributeValue\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"s\" name=\"attributeName\"/>\n"
                "      <arg direction=\"out\" type=\"s\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"b\" name=\"defined\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributes\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetDefaultAttributes\">\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetCharacterExtents\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "    </method>\n"
                "    <method name=\"GetOffsetAtPoint\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    </method>\n"
                "    <method name=\"GetNSelections\">\n"
                "      <arg direction=\"out\" type=\"i\"/>\n"
                "    <method name=\"GetSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"selectionNum\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "    </method>\n"
                "    <method name=\"AddSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"RemoveSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"selectionNum\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"SetSelection\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"selectionNum\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "    </method>\n"
                "    <method name=\"GetRangeExtents\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"endOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "    </method>\n"
                "    <method name=\"GetBoundedRanges\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
                "      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"coordType\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"xClipType\"/>\n"
                "      <arg direction=\"in\" type=\"u\" name=\"yClipType\"/>\n"
                "      <arg direction=\"out\" type=\"a(iisv)\"/>\n"
                "      <annotation value=\"QSpiRangeList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetAttributeRun\">\n"
                "      <arg direction=\"in\" type=\"i\" name=\"offset\"/>\n"
                "      <arg direction=\"in\" type=\"b\" name=\"includeDefaults\"/>\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"startOffset\"/>\n"
                "      <arg direction=\"out\" type=\"i\" name=\"endOffset\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "    <method name=\"GetDefaultAttributeSet\">\n"
                "      <arg direction=\"out\" type=\"a{ss}\"/>\n"
                "      <annotation value=\"QSpiAttributeSet\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    static const QLatin1String valueIntrospection(
                "  <interface name=\"org.a11y.atspi.Value\">\n"
                "    <property access=\"read\" type=\"d\" name=\"MinimumValue\"/>\n"
                "    <property access=\"read\" type=\"d\" name=\"MaximumValue\"/>\n"
                "    <property access=\"read\" type=\"d\" name=\"MinimumIncrement\"/>\n"
                "    <property access=\"readwrite\" type=\"d\" name=\"CurrentValue\"/>\n"
                "    <method name=\"SetCurrentValue\">\n"
                "      <arg direction=\"in\" type=\"d\" name=\"value\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                );

    QAccessibleInterface * interface = interfaceFromPath(path);
    if (!interface) {
        qAtspiDebug() << "WARNING Qt AtSpiAdaptor: Could not find accessible on path: " << path;
        return QString();
    }

    QStringList interfaces = accessibleInterfaces(interface);

    QString xml;
    xml.append(accessibleIntrospection);

    if (interfaces.contains(QLatin1String(ATSPI_DBUS_INTERFACE_COMPONENT)))
        xml.append(componentIntrospection);
    if (interfaces.contains(QLatin1String(ATSPI_DBUS_INTERFACE_TEXT)))
        xml.append(textIntrospection);
    if (interfaces.contains(QLatin1String(ATSPI_DBUS_INTERFACE_EDITABLE_TEXT)))
        xml.append(editableTextIntrospection);
    if (interfaces.contains(QLatin1String(ATSPI_DBUS_INTERFACE_ACTION)))
        xml.append(actionIntrospection);
    if (interfaces.contains(QLatin1String(ATSPI_DBUS_INTERFACE_TABLE)))
        xml.append(tableIntrospection);
    if (interfaces.contains(QLatin1String(ATSPI_DBUS_INTERFACE_VALUE)))
        xml.append(valueIntrospection);
    if (path == QLatin1String(QSPI_OBJECT_PATH_ROOT))
        xml.append(applicationIntrospection);

    return xml;
}

void AtSpiAdaptor::setBitFlag(const QString &flag)
{
    Q_ASSERT(flag.size());

    // assume we don't get nonsense - look at first letter only
    switch (flag.at(0).toLower().toLatin1()) {
    case 'o': {
        if (flag.size() <= 8) { // Object::
            sendObject = 1;
        } else { // Object:Foo:Bar
            QString right = flag.mid(7);
            if (false) {
            } else if (right.startsWith(QLatin1String("ActiveDescendantChanged"))) {
                sendObject_active_descendant_changed = 1;
            } else if (right.startsWith(QLatin1String("AttributesChanged"))) {
                sendObject_attributes_changed = 1;
            } else if (right.startsWith(QLatin1String("BoundsChanged"))) {
                sendObject_bounds_changed = 1;
            } else if (right.startsWith(QLatin1String("ChildrenChanged"))) {
                sendObject_children_changed = 1;
            } else if (right.startsWith(QLatin1String("ColumnDeleted"))) {
                sendObject_column_deleted = 1;
            } else if (right.startsWith(QLatin1String("ColumnInserted"))) {
                sendObject_column_inserted = 1;
            } else if (right.startsWith(QLatin1String("ColumnReordered"))) {
                sendObject_column_reordered = 1;
            } else if (right.startsWith(QLatin1String("LinkSelected"))) {
                sendObject_link_selected = 1;
            } else if (right.startsWith(QLatin1String("ModelChanged"))) {
                sendObject_model_changed = 1;
            } else if (right.startsWith(QLatin1String("PropertyChange"))) {
                if (right == QLatin1String("PropertyChange:AccessibleDescription")) {
                    sendObject_property_change_accessible_description = 1;
                } else if (right == QLatin1String("PropertyChange:AccessibleName")) {
                    sendObject_property_change_accessible_name = 1;
                } else if (right == QLatin1String("PropertyChange:AccessibleParent")) {
                    sendObject_property_change_accessible_parent = 1;
                } else if (right == QLatin1String("PropertyChange:AccessibleRole")) {
                    sendObject_property_change_accessible_role = 1;
                } else if (right == QLatin1String("PropertyChange:TableCaption")) {
                    sendObject_property_change_accessible_table_caption = 1;
                } else if (right == QLatin1String("PropertyChange:TableColumnDescription")) {
                    sendObject_property_change_accessible_table_column_description = 1;
                } else if (right == QLatin1String("PropertyChange:TableColumnHeader")) {
                    sendObject_property_change_accessible_table_column_header = 1;
                } else if (right == QLatin1String("PropertyChange:TableRowDescription")) {
                    sendObject_property_change_accessible_table_row_description = 1;
                } else if (right == QLatin1String("PropertyChange:TableRowHeader")) {
                    sendObject_property_change_accessible_table_row_header = 1;
                } else if (right == QLatin1String("PropertyChange:TableSummary")) {
                    sendObject_property_change_accessible_table_summary = 1;
                } else if (right == QLatin1String("PropertyChange:AccessibleValue")) {
                    sendObject_property_change_accessible_value = 1;
                } else {
                    sendObject_property_change = 1;
                }
            } else if (right.startsWith(QLatin1String("RowDeleted"))) {
                sendObject_row_deleted = 1;
            } else if (right.startsWith(QLatin1String("RowInserted"))) {
                sendObject_row_inserted = 1;
            } else if (right.startsWith(QLatin1String("RowReordered"))) {
                sendObject_row_reordered = 1;
            } else if (right.startsWith(QLatin1String("SelectionChanged"))) {
                sendObject_selection_changed = 1;
            } else if (right.startsWith(QLatin1String("StateChanged"))) {
                sendObject_state_changed = 1;
            } else if (right.startsWith(QLatin1String("TextAttributesChanged"))) {
                sendObject_text_attributes_changed = 1;
            } else if (right.startsWith(QLatin1String("TextBoundsChanged"))) {
                sendObject_text_bounds_changed = 1;
            } else if (right.startsWith(QLatin1String("TextCaretMoved"))) {
                sendObject_text_caret_moved = 1;
            } else if (right.startsWith(QLatin1String("TextChanged"))) {
                sendObject_text_changed = 1;
            } else if (right.startsWith(QLatin1String("TextSelectionChanged"))) {
                sendObject_text_selection_changed = 1;
            } else if (right.startsWith(QLatin1String("ValueChanged"))) {
                sendObject_value_changed = 1;
            } else if (right.startsWith(QLatin1String("VisibleDataChanged"))
                    || right.startsWith(QLatin1String("VisibledataChanged"))) { // typo in libatspi
                sendObject_visible_data_changed = 1;
            } else {
                qAtspiDebug() << "WARNING: subscription string not handled:" << flag;
            }
        }
        break;
    }
    case 'w': { // window
        if (flag.size() <= 8) {
            sendWindow = 1;
        } else { // object:Foo:Bar
            QString right = flag.mid(7);
            if (false) {
            } else if (right.startsWith(QLatin1String("Activate"))) {
                sendWindow_activate = 1;
            } else if (right.startsWith(QLatin1String("Close"))) {
                sendWindow_close= 1;
            } else if (right.startsWith(QLatin1String("Create"))) {
                sendWindow_create = 1;
            } else if (right.startsWith(QLatin1String("Deactivate"))) {
                sendWindow_deactivate = 1;
            } else if (right.startsWith(QLatin1String("Lower"))) {
                sendWindow_lower = 1;
            } else if (right.startsWith(QLatin1String("Maximize"))) {
                sendWindow_maximize = 1;
            } else if (right.startsWith(QLatin1String("Minimize"))) {
                sendWindow_minimize = 1;
            } else if (right.startsWith(QLatin1String("Move"))) {
                sendWindow_move = 1;
            } else if (right.startsWith(QLatin1String("Raise"))) {
                sendWindow_raise = 1;
            } else if (right.startsWith(QLatin1String("Reparent"))) {
                sendWindow_reparent = 1;
            } else if (right.startsWith(QLatin1String("Resize"))) {
                sendWindow_resize = 1;
            } else if (right.startsWith(QLatin1String("Restore"))) {
                sendWindow_restore = 1;
            } else if (right.startsWith(QLatin1String("Restyle"))) {
                sendWindow_restyle = 1;
            } else if (right.startsWith(QLatin1String("Shade"))) {
                sendWindow_shade = 1;
            } else if (right.startsWith(QLatin1String("Unshade"))) {
                sendWindow_unshade = 1;
            } else if (right.startsWith(QLatin1String("DesktopCreate"))) {
                // ignore this one
            } else if (right.startsWith(QLatin1String("DesktopDestroy"))) {
                // ignore this one
            } else {
                qAtspiDebug() << "WARNING: subscription string not handled:" << flag;
            }
        }
        break;
    }
    case 'f': {
        sendFocus = 1;
        break;
    }
    case 'd': { // document is not implemented
        break;
    }
    case 't': { // terminal is not implemented
        break;
    }
    case 'm': { // mouse* is handled in a different way by the gnome atspi stack
        break;
    }
    default:
        qAtspiDebug() << "WARNING: subscription string not handled:" << flag;
    }
}

/*!
  Checks via dbus which events should be sent.
  */
void AtSpiAdaptor::updateEventListeners()
{
    QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.a11y.atspi.Registry"),
                                                    QLatin1String("/org/a11y/atspi/registry"),
                                                    QLatin1String("org.a11y.atspi.Registry"), QLatin1String("GetRegisteredEvents"));
    QDBusReply<QSpiEventListenerArray> listenersReply = m_dbus->connection().call(m);
    if (listenersReply.isValid()) {
        const QSpiEventListenerArray evList = listenersReply.value();
        Q_FOREACH (const QSpiEventListener &ev, evList) {
            setBitFlag(ev.eventName);
        }
        m_applicationAdaptor->sendEvents(!evList.isEmpty());
    } else {
        qAtspiDebug() << "Could not query active accessibility event listeners.";
    }
}

void AtSpiAdaptor::eventListenerDeregistered(const QString &/*bus*/, const QString &/*path*/)
{
//    qAtspiDebug() << "AtSpiAdaptor::eventListenerDeregistered: " << bus << path;
    updateEventListeners();
}

void AtSpiAdaptor::eventListenerRegistered(const QString &/*bus*/, const QString &/*path*/)
{
//    qAtspiDebug() << "AtSpiAdaptor::eventListenerRegistered: " << bus << path;
    updateEventListeners();
}

/*!
  This slot needs to get called when a \a window has be activated or deactivated (become focused).
  When \a active is true, the window just received focus, otherwise it lost the focus.
  */
void AtSpiAdaptor::windowActivated(QObject* window, bool active)
{
    if (!(sendWindow || sendWindow_activate))
        return;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window);
    Q_ASSERT(iface);
    Q_ASSERT(!active || iface->isValid());

    QString windowTitle;
    // in dtor it may be invalid
    if (iface->isValid())
        windowTitle = iface->text(QAccessible::Name);

    QDBusVariant data;
    data.setVariant(windowTitle);

    QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(data));

    QString status = active ? QLatin1String("Activate") : QLatin1String("Deactivate");
    QString path = pathForObject(window);
    sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_WINDOW), status, args);

    QVariantList stateArgs = packDBusSignalArguments(QLatin1String("active"), active ? 1 : 0, 0, variantForPath(path));
    sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                   QLatin1String("StateChanged"), stateArgs);
}

QVariantList AtSpiAdaptor::packDBusSignalArguments(const QString &type, int data1, int data2, const QVariant &variantData) const
{
    QVariantList arguments;
    arguments << type << data1 << data2 << variantData
              << QVariant::fromValue(QSpiObjectReference(m_dbus->connection(), QDBusObjectPath(QSPI_OBJECT_PATH_ROOT)));
    return arguments;
}

QVariant AtSpiAdaptor::variantForPath(const QString &path) const
{
    QDBusVariant data;
    data.setVariant(QVariant::fromValue(QSpiObjectReference(m_dbus->connection(), QDBusObjectPath(path))));
    return QVariant::fromValue(data);
}

bool AtSpiAdaptor::sendDBusSignal(const QString &path, const QString &interface, const QString &signalName, const QVariantList &arguments) const
{
    QDBusMessage message = QDBusMessage::createSignal(path, interface, signalName);
    message.setArguments(arguments);
    return m_dbus->connection().send(message);
}

QAccessibleInterface *AtSpiAdaptor::interfaceFromPath(const QString& dbusPath) const
{
    if (dbusPath == QLatin1String(QSPI_OBJECT_PATH_ROOT))
        return QAccessible::queryAccessibleInterface(qApp);

    QStringList parts = dbusPath.split(QLatin1Char('/'));
    if (parts.size() != 6) {
        qAtspiDebug() << "invalid path: " << dbusPath;
        return 0;
    }

    QString objectString = parts.at(5);
    QAccessible::Id id = objectString.toUInt();

    // The id is always in the range [INT_MAX+1, UINT_MAX]
    if ((int)id >= 0)
        qWarning() << "No accessible object found for id: " << id;

    return QAccessible::accessibleInterface(id);
}

void AtSpiAdaptor::notifyStateChange(QAccessibleInterface *interface, const QString &state, int value)
{
    QString path = pathForInterface(interface);
    QVariantList stateArgs = packDBusSignalArguments(state, value, 0, variantForPath(path));
    sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                    QLatin1String("StateChanged"), stateArgs);
}


/*!
    This function gets called when Qt notifies about accessibility updates.
*/
void AtSpiAdaptor::notify(QAccessibleEvent *event)
{
    switch (event->type()) {
    case QAccessible::ObjectCreated:
        if (sendObject || sendObject_children_changed)
            notifyAboutCreation(event->accessibleInterface());
        break;
    case QAccessible::ObjectShow: {
        if (sendObject || sendObject_state_changed) {
            notifyStateChange(event->accessibleInterface(), QLatin1String("showing"), 1);
        }
        break;
    }
    case QAccessible::ObjectHide: {
        if (sendObject || sendObject_state_changed) {
            notifyStateChange(event->accessibleInterface(), QLatin1String("showing"), 0);
        }
        break;
    }
    case QAccessible::ObjectDestroyed: {
        if (sendObject || sendObject_state_changed)
            notifyAboutDestruction(event->accessibleInterface());
        break;
    }
    case QAccessible::NameChanged: {
        if (sendObject || sendObject_property_change || sendObject_property_change_accessible_name) {
            QString path = pathForInterface(event->accessibleInterface());
            QVariantList args = packDBusSignalArguments(QLatin1String("accessible-name"), 0, 0, variantForPath(path));
            sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                           QLatin1String("PropertyChange"), args);
        }
        break;
    }
    case QAccessible::DescriptionChanged: {
        if (sendObject || sendObject_property_change || sendObject_property_change_accessible_description) {
            QString path = pathForInterface(event->accessibleInterface());
            QVariantList args = packDBusSignalArguments(QLatin1String("accessible-description"), 0, 0, variantForPath(path));
            sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                           QLatin1String("PropertyChange"), args);
        }
        break;
    }
    case QAccessible::Focus: {
        if (sendFocus || sendObject || sendObject_state_changed)
            sendFocusChanged(event->accessibleInterface());
        break;
    }
    case QAccessible::TextInserted:
    case QAccessible::TextRemoved:
    case QAccessible::TextUpdated: {
        if (sendObject || sendObject_text_changed) {
            QAccessibleInterface * iface = event->accessibleInterface();
            if (!iface || !iface->textInterface()) {
                qAtspiDebug() << "Received text event for invalid interface.";
                return;
            }
            QString path = pathForInterface(iface);

            int changePosition = 0;
            int cursorPosition = 0;
            QString textRemoved;
            QString textInserted;

            if (event->type() == QAccessible::TextInserted) {
                QAccessibleTextInsertEvent *textEvent = static_cast<QAccessibleTextInsertEvent*>(event);
                textInserted = textEvent->textInserted();
                changePosition = textEvent->changePosition();
                cursorPosition = textEvent->cursorPosition();
            } else if (event->type() == QAccessible::TextRemoved) {
                QAccessibleTextRemoveEvent *textEvent = static_cast<QAccessibleTextRemoveEvent*>(event);
                textRemoved = textEvent->textRemoved();
                changePosition = textEvent->changePosition();
                cursorPosition = textEvent->cursorPosition();
            } else if (event->type() == QAccessible::TextInserted) {
                QAccessibleTextUpdateEvent *textEvent = static_cast<QAccessibleTextUpdateEvent*>(event);
                textInserted = textEvent->textInserted();
                textRemoved = textEvent->textRemoved();
                changePosition = textEvent->changePosition();
                cursorPosition = textEvent->cursorPosition();
            }

            QDBusVariant data;

            if (!textRemoved.isEmpty()) {
                data.setVariant(QVariant::fromValue(textRemoved));
                QVariantList args = packDBusSignalArguments(QLatin1String("delete"), changePosition, textRemoved.length(), QVariant::fromValue(data));
                sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                               QLatin1String("TextChanged"), args);
            }

            if (!textInserted.isEmpty()) {
                data.setVariant(QVariant::fromValue(textInserted));
                QVariantList args = packDBusSignalArguments(QLatin1String("insert"), changePosition, textInserted.length(), QVariant::fromValue(data));
                sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                               QLatin1String("TextChanged"), args);
            }

            // send a cursor update
            Q_UNUSED(cursorPosition)
//            QDBusVariant cursorData;
//            cursorData.setVariant(QVariant::fromValue(cursorPosition));
//            QVariantList args = packDBusSignalArguments(QString(), cursorPosition, 0, QVariant::fromValue(cursorData));
//            sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
//                           QLatin1String("TextCaretMoved"), args);
        }
        break;
    }
    case QAccessible::TextCaretMoved: {
        if (sendObject || sendObject_text_caret_moved) {
            QAccessibleInterface * iface = event->accessibleInterface();
            if (!iface || !iface->textInterface()) {
                qWarning() << "Sending TextCaretMoved from object that does not implement text interface: " << iface;
                return;
            }

            QString path = pathForInterface(iface);
            QDBusVariant cursorData;
            int pos = iface->textInterface()->cursorPosition();
            cursorData.setVariant(QVariant::fromValue(pos));
            QVariantList args = packDBusSignalArguments(QString(), pos, 0, QVariant::fromValue(cursorData));
            sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                           QLatin1String("TextCaretMoved"), args);
        }
        break;
    }
    case QAccessible::TextSelectionChanged: {
        if (sendObject || sendObject_text_selection_changed) {
            QAccessibleInterface * iface = event->accessibleInterface();
            QString path = pathForInterface(iface);
            QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(QDBusVariant(QVariant(QString()))));
            sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                           QLatin1String("TextSelectionChanged"), args);
        }
        break;
    }
    case QAccessible::ValueChanged: {
        if (sendObject || sendObject_value_changed || sendObject_property_change_accessible_value) {
            QAccessibleInterface * iface = event->accessibleInterface();
            if (!iface || !iface->valueInterface()) {
                qWarning() << "ValueChanged event from invalid accessible: " << iface;
                return;
            }

            QString path = pathForInterface(iface);
            QVariantList args = packDBusSignalArguments(QLatin1String("accessible-value"), 0, 0, variantForPath(path));
            sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                           QLatin1String("PropertyChange"), args);
        }
        break;
    }
    case QAccessible::Selection: {
        QAccessibleInterface * iface = event->accessibleInterface();
        if (!iface) {
            qWarning() << "Selection event from invalid accessible.";
            return;
        }
        QString path = pathForInterface(iface);
        int selected = iface->state().selected ? 1 : 0;
        QVariantList stateArgs = packDBusSignalArguments(QLatin1String("selected"), selected, 0, variantForPath(path));
        sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                       QLatin1String("StateChanged"), stateArgs);
        break;
    }

    case QAccessible::StateChanged: {
        if (sendObject || sendObject_state_changed || sendWindow || sendWindow_activate) {
            QAccessible::State stateChange = static_cast<QAccessibleStateChangeEvent*>(event)->changedStates();
            if (stateChange.checked) {
                QAccessibleInterface * iface = event->accessibleInterface();
                if (!iface) {
                    qWarning() << "StateChanged event from invalid accessible.";
                    return;
                }
                int checked = iface->state().checked;
                notifyStateChange(iface, QLatin1String("checked"), checked);
            } else if (stateChange.active) {
                QAccessibleInterface * iface = event->accessibleInterface();
                if (!iface || !(iface->role() == QAccessible::Window && (sendWindow || sendWindow_activate)))
                    return;
                QString windowTitle = iface->text(QAccessible::Name);
                QDBusVariant data;
                data.setVariant(windowTitle);
                QVariantList args = packDBusSignalArguments(QString(), 0, 0, QVariant::fromValue(data));

                QString status = iface->state().active ? QLatin1String("Activate") : QLatin1String("Deactivate");
                QString path = pathForInterface(iface);
                sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_WINDOW), status, args);

                int isActive = iface->state().active;
                notifyStateChange(iface, QLatin1String("active"), isActive);
            } else if (stateChange.disabled) {
                QAccessibleInterface *iface = event->accessibleInterface();
                QAccessible::State state = iface->state();
                bool enabled = !state.disabled;

                notifyStateChange(iface, QLatin1String("enabled"), enabled);
                notifyStateChange(iface, QLatin1String("sensitive"), enabled);
            }
        }
        break;
    }
        // For now we ignore these events
    case QAccessible::TableModelChanged:
        // For tables, setting manages_descendants should
        // indicate to the client that it cannot cache these
        // interfaces.
    case QAccessible::ParentChanged:
    case QAccessible::DialogStart:
    case QAccessible::DialogEnd:
    case QAccessible::SelectionRemove:
    case QAccessible::PopupMenuStart:
    case QAccessible::PopupMenuEnd:
    case QAccessible::SoundPlayed:
    case QAccessible::Alert:
    case QAccessible::ForegroundChanged:
    case QAccessible::MenuStart:
    case QAccessible::MenuEnd:
    case QAccessible::ContextHelpStart:
    case QAccessible::ContextHelpEnd:
    case QAccessible::DragDropStart:
    case QAccessible::DragDropEnd:
    case QAccessible::ScrollingStart:
    case QAccessible::ScrollingEnd:
    case QAccessible::MenuCommand:
    case QAccessible::ActionChanged:
    case QAccessible::ActiveDescendantChanged:
    case QAccessible::AttributeChanged:
    case QAccessible::DocumentContentChanged:
    case QAccessible::DocumentLoadComplete:
    case QAccessible::DocumentLoadStopped:
    case QAccessible::DocumentReload:
    case QAccessible::HyperlinkEndIndexChanged:
    case QAccessible::HyperlinkNumberOfAnchorsChanged:
    case QAccessible::HyperlinkSelectedLinkChanged:
    case QAccessible::HypertextLinkActivated:
    case QAccessible::HypertextLinkSelected:
    case QAccessible::HyperlinkStartIndexChanged:
    case QAccessible::HypertextChanged:
    case QAccessible::HypertextNLinksChanged:
    case QAccessible::ObjectAttributeChanged:
    case QAccessible::PageChanged:
    case QAccessible::SectionChanged:
    case QAccessible::TableCaptionChanged:
    case QAccessible::TableColumnDescriptionChanged:
    case QAccessible::TableColumnHeaderChanged:
    case QAccessible::TableRowDescriptionChanged:
    case QAccessible::TableRowHeaderChanged:
    case QAccessible::TableSummaryChanged:
    case QAccessible::TextAttributeChanged:
    case QAccessible::TextColumnChanged:
    case QAccessible::VisibleDataChanged:
    case QAccessible::ObjectReorder:
    case QAccessible::SelectionAdd:
    case QAccessible::SelectionWithin:
    case QAccessible::LocationChanged:
    case QAccessible::HelpChanged:
    case QAccessible::DefaultActionChanged:
    case QAccessible::AcceleratorChanged:
    case QAccessible::InvalidEvent:
        break;
    }
}

void AtSpiAdaptor::sendFocusChanged(QAccessibleInterface *interface) const
{
    static QString lastFocusPath;
    // "remove" old focus
    if (!lastFocusPath.isEmpty()) {
        QVariantList stateArgs = packDBusSignalArguments(QLatin1String("focused"), 0, 0, variantForPath(lastFocusPath));
        sendDBusSignal(lastFocusPath, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                       QLatin1String("StateChanged"), stateArgs);
    }
    // send new focus
    {
        QString path = pathForInterface(interface);

        QVariantList stateArgs = packDBusSignalArguments(QLatin1String("focused"), 1, 0, variantForPath(path));
        sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT),
                       QLatin1String("StateChanged"), stateArgs);

        QVariantList focusArgs = packDBusSignalArguments(QString(), 0, 0, variantForPath(path));
        sendDBusSignal(path, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_FOCUS),
                       QLatin1String("Focus"), focusArgs);
        lastFocusPath = path;
    }
}

void AtSpiAdaptor::notifyAboutCreation(QAccessibleInterface *interface) const
{
//    // say hello to d-bus
//    cache->emitAddAccessible(accessible->getCacheItem());

    // notify about the new child of our parent
    QAccessibleInterface * parent = interface->parent();
    if (!parent) {
        qAtspiDebug() << "AtSpiAdaptor::notifyAboutCreation: Could not find parent for " << interface->object();
        return;
    }
    QString path = pathForInterface(interface);
    int childCount = parent->childCount();
    QString parentPath = pathForInterface(parent);
    QVariantList args = packDBusSignalArguments(QLatin1String("add"), childCount, 0, variantForPath(path));
    sendDBusSignal(parentPath, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT), QLatin1String("ChildrenChanged"), args);
}

void AtSpiAdaptor::notifyAboutDestruction(QAccessibleInterface *interface) const
{
    if (!interface || !interface->isValid())
        return;

    QAccessibleInterface * parent = interface->parent();
    if (!parent) {
        qAtspiDebug() << "AtSpiAdaptor::notifyAboutDestruction: Could not find parent for " << interface->object();
        return;
    }
    QString path = pathForInterface(interface);

    // this is in the destructor. we have no clue which child we used to be.
    // FIXME
    int childIndex = -1;
    //    if (child) {
    //        childIndex = child;
    //    } else {
    //        childIndex = parent->indexOfChild(interface);
    //    }

    QString parentPath = pathForInterface(parent);
    QVariantList args = packDBusSignalArguments(QLatin1String("remove"), childIndex, 0, variantForPath(path));
    sendDBusSignal(parentPath, QLatin1String(ATSPI_DBUS_INTERFACE_EVENT_OBJECT), QLatin1String("ChildrenChanged"), args);
}

/*!
  Handle incoming DBus message.
  This function dispatches the dbus message to the right interface handler.
  */
bool AtSpiAdaptor::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    // get accessible interface
    QAccessibleInterface * accessible = interfaceFromPath(message.path());
    if (!accessible) {
        qAtspiDebug() << "WARNING Qt AtSpiAdaptor: Could not find accessible on path: " << message.path();
        return false;
    }
    if (!accessible->isValid()) {
        qWarning() << "WARNING Qt AtSpiAdaptor: Accessible invalid: " << accessible << message.path();
        return false;
    }

    QString interface = message.interface();
    QString function = message.member();

    // qAtspiDebug() << "AtSpiAdaptor::handleMessage: " << interface << function;

    if (function == QLatin1String("Introspect")) {
        //introspect(message.path());
        return false;
    }

    // handle properties like regular functions
    if (interface == QLatin1String("org.freedesktop.DBus.Properties")) {
        interface = message.arguments().at(0).toString();
        // Get/Set + Name
        function = message.member() + message.arguments().at(1).toString();
    }

    // switch interface to call
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_ACCESSIBLE))
        return accessibleInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_APPLICATION))
        return applicationInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_COMPONENT))
        return componentInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_ACTION))
        return actionInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_TEXT))
        return textInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_EDITABLE_TEXT))
        return editableTextInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_VALUE))
        return valueInterface(accessible, function, message, connection);
    if (interface == QLatin1String(ATSPI_DBUS_INTERFACE_TABLE))
        return tableInterface(accessible, function, message, connection);

    qAtspiDebug() << "AtSpiAdaptor::handleMessage with unknown interface: " << message.path() << interface << function;
    return false;
}

// Application
bool AtSpiAdaptor::applicationInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (message.path() != QLatin1String(ATSPI_DBUS_PATH_ROOT)) {
        qAtspiDebug() << "WARNING Qt AtSpiAdaptor: Could not find application interface for: " << message.path() << interface;
        return false;
    }

    if (function == QLatin1String("SetId")) {
        Q_ASSERT(message.signature() == QLatin1String("ssv"));
        QVariant value = qvariant_cast<QDBusVariant>(message.arguments().at(2)).variant();

        m_applicationId = value.toInt();
        return true;
    }
    if (function == QLatin1String("GetId")) {
        Q_ASSERT(message.signature() == QLatin1String("ss"));
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant(m_applicationId)));
        return connection.send(reply);
    }
    if (function == QLatin1String("GetToolkitName")) {
        Q_ASSERT(message.signature() == QLatin1String("ss"));
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant(QLatin1String("Qt"))));
        return connection.send(reply);
    }
    if (function == QLatin1String("GetVersion")) {
        Q_ASSERT(message.signature() == QLatin1String("ss"));
        QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant(QLatin1String(qVersion()))));
        return connection.send(reply);
    }
    if (function == QLatin1String("GetLocale")) {
        Q_ASSERT(message.signature() == QLatin1String("u"));
        QDBusMessage reply = message.createReply(QVariant::fromValue(QLocale().name()));
        return connection.send(reply);
    }
    qAtspiDebug() << "AtSpiAdaptor::applicationInterface " << message.path() << interface << function;
    return false;
}

/*!
  Register this application as accessible on the accessibility DBus.
  */
void AtSpiAdaptor::registerApplication()
{
    OrgA11yAtspiSocketInterface *registry;
    registry = new OrgA11yAtspiSocketInterface(QLatin1String(QSPI_REGISTRY_NAME),
                               QLatin1String(QSPI_OBJECT_PATH_ROOT), m_dbus->connection());

    QDBusPendingReply<QSpiObjectReference> reply;
    QSpiObjectReference ref = QSpiObjectReference(m_dbus->connection(), QDBusObjectPath(QSPI_OBJECT_PATH_ROOT));
    reply = registry->Embed(ref);
    reply.waitForFinished(); // TODO: make this async
    if (reply.isValid ()) {
        const QSpiObjectReference &socket = reply.value();
        accessibilityRegistry = QSpiObjectReference(socket);
    } else {
        qAtspiDebug() << "Error in contacting registry: "
                   << reply.error().name()
                   << reply.error().message();
    }
    delete registry;
}

// Accessible
bool AtSpiAdaptor::accessibleInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == QLatin1String("GetRole")) {
        sendReply(connection, message, (uint) getRole(interface));
    } else if (function == QLatin1String("GetName")) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->text(QAccessible::Name))));
    } else if (function == QLatin1String("GetRoleName")) {
        sendReply(connection, message, qSpiRoleMapping[interface->role()].name());
    } else if (function == QLatin1String("GetLocalizedRoleName")) {
        sendReply(connection, message, QVariant::fromValue(qSpiRoleMapping[interface->role()].localizedName()));
    } else if (function == QLatin1String("GetChildCount")) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->childCount())));
    } else if (function == QLatin1String("GetIndexInParent")) {
        int childIndex = -1;
        QAccessibleInterface * parent = interface->parent();
        if (parent) {
            childIndex = parent->indexOfChild(interface);
            if (childIndex < 0) {
                qAtspiDebug() <<  "GetIndexInParent get invalid index: " << childIndex << interface;
            }
        }
        sendReply(connection, message, childIndex);
    } else if (function == QLatin1String("GetParent")) {
        QString path;
        QAccessibleInterface * parent = interface->parent();
        if (!parent) {
            path = QLatin1String(ATSPI_DBUS_PATH_NULL);
        } else if (parent->role() == QAccessible::Application) {
            path = QLatin1String(ATSPI_DBUS_PATH_ROOT);
        } else {
            path = pathForInterface(parent);
        }
        // Parent is a property, so it needs to be wrapped inside an extra variant.
        sendReply(connection, message, QVariant::fromValue(
                      QDBusVariant(QVariant::fromValue(QSpiObjectReference(connection, QDBusObjectPath(path))))));
    } else if (function == QLatin1String("GetChildAtIndex")) {
        int index = message.arguments().first().toInt();
        if (index < 0) {
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_NULL))));
        } else {
            QAccessibleInterface * childInterface = interface->child(index);
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(childInterface)))));
        }
    } else if (function == QLatin1String("GetInterfaces")) {
        sendReply(connection, message, accessibleInterfaces(interface));
    } else if (function == QLatin1String("GetDescription")) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(interface->text(QAccessible::Description))));
    } else if (function == QLatin1String("GetState")) {
        quint64 spiState = spiStatesFromQState(interface->state());
        if (interface->tableInterface()) {
            setSpiStateBit(&spiState, ATSPI_STATE_MANAGES_DESCENDANTS);
        }
        QAccessible::Role role = interface->role();
        if (role == QAccessible::TreeItem ||
            role == QAccessible::ListItem) {
            /* Transient means libatspi2 will not cache items.
               This is important because when adding/removing an item
               the cache becomes outdated and we don't change the paths of
               items in lists/trees/tables. */
            setSpiStateBit(&spiState, ATSPI_STATE_TRANSIENT);
        }
        sendReply(connection, message,
                  QVariant::fromValue(spiStateSetFromSpiStates(spiState)));
    } else if (function == QLatin1String("GetAttributes")) {
        sendReply(connection, message, QVariant::fromValue(QSpiAttributeSet()));
    } else if (function == QLatin1String("GetRelationSet")) {
        sendReply(connection, message, QVariant::fromValue(relationSet(interface, connection)));
    } else if (function == QLatin1String("GetApplication")) {
        sendReply(connection, message, QVariant::fromValue(
                      QSpiObjectReference(connection, QDBusObjectPath(QSPI_OBJECT_PATH_ROOT))));
    } else if (function == QLatin1String("GetChildren")) {
        QSpiObjectReferenceArray children;
        for (int i = 0; i < interface->childCount(); ++i) {
            QString childPath = pathForInterface(interface->child(i));
            QSpiObjectReference ref(connection, QDBusObjectPath(childPath));
            children << ref;
        }
        connection.send(message.createReply(QVariant::fromValue(children)));
    } else {
        qAtspiDebug() << "WARNING: AtSpiAdaptor::accessibleInterface does not implement " << function << message.path();
        return false;
    }
    return true;
}

AtspiRole AtSpiAdaptor::getRole(QAccessibleInterface *interface) const
{
    if ((interface->role() == QAccessible::EditableText) && interface->state().passwordEdit)
        return ATSPI_ROLE_PASSWORD_TEXT;
    return qSpiRoleMapping[interface->role()].spiRole();
}

//#define ACCESSIBLE_CREATION_DEBUG

QStringList AtSpiAdaptor::accessibleInterfaces(QAccessibleInterface *interface) const
{
    QStringList ifaces;
#ifdef ACCESSIBLE_CREATION_DEBUG
    qAtspiDebug() << "AtSpiAdaptor::accessibleInterfaces create: " << interface->object();
#endif
    ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_ACCESSIBLE);

    if (    (!interface->rect().isEmpty()) ||
            (interface->object() && interface->object()->isWidgetType()) ||
            (interface->role() == QAccessible::ListItem) ||
            (interface->role() == QAccessible::Cell) ||
            (interface->role() == QAccessible::TreeItem) ||
            (interface->role() == QAccessible::Row) ||
            (interface->object() && interface->object()->inherits("QSGItem"))
            ) {
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_COMPONENT);
        }
#ifdef ACCESSIBLE_CREATION_DEBUG
    else {
        qAtspiDebug() << " IS NOT a component";
    }
#endif
    if (interface->role() == QAccessible::Application)
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_APPLICATION);

    if (interface->actionInterface())
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_ACTION);

    if (interface->textInterface())
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_TEXT);

    if (interface->editableTextInterface())
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_EDITABLE_TEXT);

    if (interface->valueInterface())
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_VALUE);

    if (interface->tableInterface())
        ifaces << QLatin1String(ATSPI_DBUS_INTERFACE_TABLE);

    return ifaces;
}

QSpiRelationArray AtSpiAdaptor::relationSet(QAccessibleInterface *interface, const QDBusConnection &connection) const
{
    typedef QPair<QAccessibleInterface*, QAccessible::Relation> RelationPair;
    QVector<RelationPair> relationInterfaces;
    relationInterfaces = interface->relations();

    QSpiRelationArray relations;
    Q_FOREACH (const RelationPair &pair, relationInterfaces) {
// FIXME: this loop seems a bit strange... "related" always have one item when we check.
//And why is it a list, when it always have one item? And it seems to assume that the QAccessible::Relation enum maps directly to AtSpi
        QList<QSpiObjectReference> related;

        QDBusObjectPath path = QDBusObjectPath(pathForInterface(pair.first));
        related.append(QSpiObjectReference(connection, path));

        if (!related.isEmpty())
            relations.append(QSpiRelationArrayEntry(qAccessibleRelationToAtSpiRelation(pair.second), related));
    }
    return relations;
}

void AtSpiAdaptor::sendReply(const QDBusConnection &connection, const QDBusMessage &message, const QVariant &argument) const
{
    QDBusMessage reply = message.createReply(argument);
    connection.send(reply);
}


QString AtSpiAdaptor::pathForObject(QObject *object) const
{
    Q_ASSERT(object);

    if (inheritsQAction(object)) {
        qAtspiDebug() << "AtSpiAdaptor::pathForObject: warning: creating path with QAction as object.";
    }

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(object);
    return pathForInterface(iface);
}

QString AtSpiAdaptor::pathForInterface(QAccessibleInterface *interface) const
{
    if (!interface || !interface->isValid())
        return QLatin1String(ATSPI_DBUS_PATH_NULL);
    if (interface->role() == QAccessible::Application)
        return QLatin1String(QSPI_OBJECT_PATH_ROOT);

    QAccessible::Id id = QAccessible::uniqueId(interface);
    Q_ASSERT((int)id < 0);
    return QLatin1String(QSPI_OBJECT_PATH_PREFIX) + QString::number(id);
}

bool AtSpiAdaptor::inheritsQAction(QObject *object)
{
    const QMetaObject *mo = object->metaObject();
    while (mo) {
        const QLatin1String cn(mo->className());
        if (cn == QLatin1String("QAction"))
            return true;
        mo = mo->superClass();
    }
    return false;
}

// Component
static QAccessibleInterface * getWindow(QAccessibleInterface * interface)
{
    if (interface->role() == QAccessible::Window)
        return interface;

    QAccessibleInterface * parent = interface->parent();
    while (parent && parent->role() != QAccessible::Window)
        parent = parent->parent();

    return parent;
}

static QRect getRelativeRect(QAccessibleInterface *interface)
{
    QAccessibleInterface * window;
    QRect wr, cr;

    cr = interface->rect();

    window = getWindow(interface);
    if (window) {
        wr = window->rect();

        cr.setX(cr.x() - wr.x());
        cr.setY(cr.x() - wr.y());
    }
    return cr;
}

bool AtSpiAdaptor::componentInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == QLatin1String("Contains")) {
        bool ret = false;
        int x = message.arguments().at(0).toInt();
        int y = message.arguments().at(1).toInt();
        uint coordType = message.arguments().at(2).toUInt();
        if (coordType == ATSPI_COORD_TYPE_SCREEN)
            ret = interface->rect().contains(x, y);
        else
            ret = getRelativeRect(interface).contains(x, y);
        sendReply(connection, message, ret);
    } else if (function == QLatin1String("GetAccessibleAtPoint")) {
        int x = message.arguments().at(0).toInt();
        int y = message.arguments().at(1).toInt();
        uint coordType = message.arguments().at(2).toUInt();
        Q_UNUSED (coordType) // FIXME

        QAccessibleInterface * childInterface(interface->childAt(x, y));
        QAccessibleInterface * iface = 0;
        while (childInterface) {
            iface = childInterface;
            childInterface = iface->childAt(x, y);
        }
        if (iface) {
            QString path = pathForInterface(iface);
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(path))));
        } else {
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_NULL))));
        }
    } else if (function == QLatin1String("GetAlpha")) {
        sendReply(connection, message, (double) 1.0);
    } else if (function == QLatin1String("GetExtents")) {
        uint coordType = message.arguments().at(0).toUInt();
        sendReply(connection, message, QVariant::fromValue(getExtents(interface, coordType)));
    } else if (function == QLatin1String("GetLayer")) {
        sendReply(connection, message, QVariant::fromValue((uint)1));
    } else if (function == QLatin1String("GetMDIZOrder")) {
        sendReply(connection, message, QVariant::fromValue((short)0));
    } else if (function == QLatin1String("GetPosition")) {
        uint coordType = message.arguments().at(0).toUInt();
        QRect rect;
        if (coordType == ATSPI_COORD_TYPE_SCREEN)
            rect = interface->rect();
        else
            rect = getRelativeRect(interface);
        QVariantList pos;
        pos << rect.x() << rect.y();
        connection.send(message.createReply(pos));
    } else if (function == QLatin1String("GetSize")) {
        QRect rect = interface->rect();
        QVariantList size;
        size << rect.width() << rect.height();
        connection.send(message.createReply(size));
    } else if (function == QLatin1String("GrabFocus")) {
// FIXME: implement focus grabbing
//        if (interface->object() && interface->object()->isWidgetType()) {
//            QWidget* w = static_cast<QWidget*>(interface->object());
//            w->setFocus(Qt::OtherFocusReason);
//            sendReply(connection, message, true);
//        }
        sendReply(connection, message, false);
    } else if (function == QLatin1String("SetExtents")) {
//        int x = message.arguments().at(0).toInt();
//        int y = message.arguments().at(1).toInt();
//        int width = message.arguments().at(2).toInt();
//        int height = message.arguments().at(3).toInt();
//        uint coordinateType = message.arguments().at(4).toUInt();
        qAtspiDebug() << "SetExtents is not implemented.";
        sendReply(connection, message, false);
    } else if (function == QLatin1String("SetPosition")) {
//        int x = message.arguments().at(0).toInt();
//        int y = message.arguments().at(1).toInt();
//        uint coordinateType = message.arguments().at(2).toUInt();
        qAtspiDebug() << "SetPosition is not implemented.";
        sendReply(connection, message, false);
    } else if (function == QLatin1String("SetSize")) {
//        int width = message.arguments().at(0).toInt();
//        int height = message.arguments().at(1).toInt();
        qAtspiDebug() << "SetSize is not implemented.";
        sendReply(connection, message, false);
    } else {
        qAtspiDebug() << "WARNING: AtSpiAdaptor::componentInterface does not implement " << function << message.path();
        return false;
    }
    return true;
}

QRect AtSpiAdaptor::getExtents(QAccessibleInterface *interface, uint coordType)
{
    return (coordType == ATSPI_COORD_TYPE_SCREEN) ? interface->rect() : getRelativeRect(interface);
}

// Action interface
bool AtSpiAdaptor::actionInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    QAccessibleActionInterface *actionIface = interface->actionInterface();
    if (!actionIface)
        return false;

    if (function == QLatin1String("GetNActions")) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(actionIface->actionNames().count()))));
    } else if (function == QLatin1String("DoAction")) {
        int index = message.arguments().at(0).toInt();
        if (index < 0 || index >= actionIface->actionNames().count())
            return false;
        interface->actionInterface()->doAction(actionIface->actionNames().at(index));
        sendReply(connection, message, true);
    } else if (function == QLatin1String("GetActions")) {
        sendReply(connection, message, QVariant::fromValue(getActions(actionIface)));
    } else if (function == QLatin1String("GetName")) {
        int index = message.arguments().at(0).toInt();
        if (index < 0 || index >= actionIface->actionNames().count())
            return false;
        sendReply(connection, message, actionIface->actionNames().at(index));
    } else if (function == QLatin1String("GetDescription")) {
        int index = message.arguments().at(0).toInt();
        if (index < 0 || index >= actionIface->actionNames().count())
            return false;
        sendReply(connection, message, actionIface->localizedActionDescription(actionIface->actionNames().at(index)));
    } else if (function == QLatin1String("GetKeyBinding")) {
        int index = message.arguments().at(0).toInt();
        if (index < 0 || index >= actionIface->actionNames().count())
            return false;
        QStringList keyBindings;
        keyBindings = actionIface->keyBindingsForAction(actionIface->actionNames().value(index));
        if (keyBindings.isEmpty()) {
            QString acc = interface->text(QAccessible::Accelerator);
            if (!acc.isEmpty())
                keyBindings.append(acc);
        }
        if (keyBindings.length() > 0)
            sendReply(connection, message, keyBindings.join(QLatin1Char(';')));
        else
            sendReply(connection, message, QString());
    } else {
        qAtspiDebug() << "WARNING: AtSpiAdaptor::actionInterface does not implement " << function << message.path();
        return false;
    }
    return true;
}

QSpiActionArray AtSpiAdaptor::getActions(QAccessibleActionInterface *actionInterface) const
{
    QSpiActionArray actions;
    Q_FOREACH (const QString &actionName, actionInterface->actionNames()) {
        QSpiAction action;
        QStringList keyBindings;

        action.name = actionName;
        action.description = actionInterface->localizedActionDescription(actionName);

        keyBindings = actionInterface->keyBindingsForAction(actionName);

        if (keyBindings.length() > 0)
                action.keyBinding = keyBindings[0];
        else
            action.keyBinding = QString();

        actions << action;
    }
    return actions;
}

// Text interface
bool AtSpiAdaptor::textInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (!interface->textInterface())
        return false;

    // properties
    if (function == QLatin1String("GetCaretOffset")) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(interface->textInterface()->cursorPosition()))));
    } else if (function == QLatin1String("GetCharacterCount")) {
        sendReply(connection, message, QVariant::fromValue(QDBusVariant(QVariant::fromValue(interface->textInterface()->characterCount()))));

    // functions
    } else if (function == QLatin1String("AddSelection")) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        int lastSelection = interface->textInterface()->selectionCount();
        interface->textInterface()->setSelection(lastSelection, startOffset, endOffset);
        sendReply(connection, message, (interface->textInterface()->selectionCount() > lastSelection));
    } else if (function == QLatin1String("GetAttributeRun")) {
        int offset = message.arguments().at(0).toInt();
        bool includeDefaults = message.arguments().at(1).toBool();
        Q_UNUSED(includeDefaults)
        connection.send(message.createReply(getAttributes(interface, offset, includeDefaults)));
    } else if (function == QLatin1String("GetAttributeValue")) {
        int offset = message.arguments().at(0).toInt();
        QString attributeName = message.arguments().at(1).toString();
        connection.send(message.createReply(getAttributeValue(interface, offset, attributeName)));
    } else if (function == QLatin1String("GetAttributes")) {
        int offset = message.arguments().at(0).toInt();
        connection.send(message.createReply(getAttributes(interface, offset, true)));
    } else if (function == QLatin1String("GetBoundedRanges")) {
        int x = message.arguments().at(0).toInt();
        int y = message.arguments().at(1).toInt();
        int width = message.arguments().at(2).toInt();
        int height = message.arguments().at(3).toInt();
        uint coordType = message.arguments().at(4).toUInt();
        uint xClipType = message.arguments().at(5).toUInt();
        uint yClipType = message.arguments().at(6).toUInt();
        Q_UNUSED(x) Q_UNUSED (y) Q_UNUSED(width)
        Q_UNUSED(height) Q_UNUSED(coordType)
        Q_UNUSED(xClipType) Q_UNUSED(yClipType)
        qAtspiDebug("Not implemented: QSpiAdaptor::GetBoundedRanges");
        sendReply(connection, message, QVariant::fromValue(QSpiTextRangeList()));
    } else if (function == QLatin1String("GetCharacterAtOffset")) {
        int offset = message.arguments().at(0).toInt();
        int start;
        int end;
        QString result = interface->textInterface()->textAtOffset(offset, QAccessible::CharBoundary, &start, &end);
        sendReply(connection, message, (int) *(qPrintable (result)));
    } else if (function == QLatin1String("GetCharacterExtents")) {
        int offset = message.arguments().at(0).toInt();
        int coordType = message.arguments().at(1).toUInt();
        connection.send(message.createReply(getCharacterExtents(interface, offset, coordType)));
    } else if (function == QLatin1String("GetDefaultAttributeSet") || function == QLatin1String("GetDefaultAttributes")) {
        // GetDefaultAttributes is deprecated in favour of GetDefaultAttributeSet.
        // Empty set seems reasonable. There is no default attribute set.
        sendReply(connection, message, QVariant::fromValue(QSpiAttributeSet()));
    } else if (function == QLatin1String("GetNSelections")) {
        sendReply(connection, message, interface->textInterface()->selectionCount());
    } else if (function == QLatin1String("GetOffsetAtPoint")) {
        qAtspiDebug() << message.signature();
        Q_ASSERT(!message.signature().isEmpty());
        QPoint point(message.arguments().at(0).toInt(), message.arguments().at(1).toInt());
        uint coordType = message.arguments().at(2).toUInt();
        if (coordType == ATSPI_COORD_TYPE_WINDOW) {
            QWindow *win = interface->window();
            point -= QPoint(win->x(), win->y());
        }
        int offset = interface->textInterface()->offsetAtPoint(point);
        sendReply(connection, message, offset);
    } else if (function == QLatin1String("GetRangeExtents")) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        uint coordType = message.arguments().at(2).toUInt();
        connection.send(message.createReply(getRangeExtents(interface, startOffset, endOffset, coordType)));
    } else if (function == QLatin1String("GetSelection")) {
        int selectionNum = message.arguments().at(0).toInt();
        int start, end;
        interface->textInterface()->selection(selectionNum, &start, &end);
        if (start < 0)
            start = end = interface->textInterface()->cursorPosition();
        QVariantList sel;
        sel << start << end;
        connection.send(message.createReply(sel));
    } else if (function == QLatin1String("GetText")) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        if (endOffset == -1) // AT-SPI uses -1 to signal all characters
            endOffset = interface->textInterface()->characterCount();
        sendReply(connection, message, interface->textInterface()->text(startOffset, endOffset));
    } else if (function == QLatin1String("GetTextAfterOffset")) {
        int offset = message.arguments().at(0).toInt();
        int type = message.arguments().at(1).toUInt();
        int startOffset, endOffset;
        QString text = interface->textInterface()->textAfterOffset(offset, qAccessibleBoundaryType(type), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == QLatin1String("GetTextAtOffset")) {
        int offset = message.arguments().at(0).toInt();
        int type = message.arguments().at(1).toUInt();
        int startOffset, endOffset;
        QString text = interface->textInterface()->textAtOffset(offset, qAccessibleBoundaryType(type), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == QLatin1String("GetTextBeforeOffset")) {
        int offset = message.arguments().at(0).toInt();
        int type = message.arguments().at(1).toUInt();
        int startOffset, endOffset;
        QString text = interface->textInterface()->textBeforeOffset(offset, qAccessibleBoundaryType(type), &startOffset, &endOffset);
        QVariantList ret;
        ret << text << startOffset << endOffset;
        connection.send(message.createReply(ret));
    } else if (function == QLatin1String("RemoveSelection")) {
        int selectionNum = message.arguments().at(0).toInt();
        interface->textInterface()->removeSelection(selectionNum);
        sendReply(connection, message, true);
    } else if (function == QLatin1String("SetCaretOffset")) {
        int offset = message.arguments().at(0).toInt();
        interface->textInterface()->setCursorPosition(offset);
        sendReply(connection, message, true);
    } else if (function == QLatin1String("SetSelection")) {
        int selectionNum = message.arguments().at(0).toInt();
        int startOffset = message.arguments().at(1).toInt();
        int endOffset = message.arguments().at(2).toInt();
        interface->textInterface()->setSelection(selectionNum, startOffset, endOffset);
        sendReply(connection, message, true);
    } else {
        qAtspiDebug() << "WARNING: AtSpiAdaptor::textInterface does not implement " << function << message.path();
        return false;
    }
    return true;
}

QAccessible::TextBoundaryType AtSpiAdaptor::qAccessibleBoundaryType(int atspiTextBoundaryType) const
{
    switch (atspiTextBoundaryType) {
    case ATSPI_TEXT_BOUNDARY_CHAR:
        return QAccessible::CharBoundary;
    case ATSPI_TEXT_BOUNDARY_WORD_START:
    case ATSPI_TEXT_BOUNDARY_WORD_END:
        return QAccessible::WordBoundary;
    case ATSPI_TEXT_BOUNDARY_SENTENCE_START:
    case ATSPI_TEXT_BOUNDARY_SENTENCE_END:
        return QAccessible::SentenceBoundary;
    case ATSPI_TEXT_BOUNDARY_LINE_START:
    case ATSPI_TEXT_BOUNDARY_LINE_END:
        return QAccessible::LineBoundary;
    }
    Q_ASSERT_X(0, "", "Requested invalid boundary type.");
    return QAccessible::CharBoundary;
}

// FIXME all attribute methods below should share code
QVariantList AtSpiAdaptor::getAttributes(QAccessibleInterface *interface, int offset, bool includeDefaults) const
{
    Q_UNUSED(includeDefaults);

    QSpiAttributeSet set;
    int startOffset;
    int endOffset;

    QString joined = interface->textInterface()->attributes(offset, &startOffset, &endOffset);
    QStringList attributes = joined.split (QLatin1Char(';'), QString::SkipEmptyParts, Qt::CaseSensitive);
    foreach (const QString &attr, attributes) {
        QStringList items;
        items = attr.split(QLatin1Char(':'), QString::SkipEmptyParts, Qt::CaseSensitive);
        set[items[0]] = items[1];
    }

    QVariantList list;
    list << QVariant::fromValue(set) << startOffset << endOffset;

    return list;
}

QVariantList AtSpiAdaptor::getAttributeValue(QAccessibleInterface *interface, int offset, const QString &attributeName) const
{
    QString mapped;
    QString joined;
    QStringList attributes;
    QSpiAttributeSet map;
    int startOffset;
    int endOffset;
    bool defined;

    joined = interface->textInterface()->attributes(offset, &startOffset, &endOffset);
    attributes = joined.split (QLatin1Char(';'), QString::SkipEmptyParts, Qt::CaseSensitive);
    foreach (const QString& attr, attributes) {
        QStringList items;
        items = attr.split(QLatin1Char(':'), QString::SkipEmptyParts, Qt::CaseSensitive);
        map[items[0]] = items[1];
    }
    mapped = map[attributeName];
    defined = mapped.isEmpty();
    QVariantList list;
    list << mapped << startOffset << endOffset << defined;
    return list;
}

QRect AtSpiAdaptor::getCharacterExtents(QAccessibleInterface *interface, int offset, uint coordType) const
{
    QRect rect = interface->textInterface()->characterRect(offset);

    if (coordType == ATSPI_COORD_TYPE_WINDOW)
        rect = translateRectToWindowCoordinates(interface, rect);

    return rect;
}

QRect AtSpiAdaptor::getRangeExtents(QAccessibleInterface *interface,
                                            int startOffset, int endOffset, uint coordType) const
{
    if (endOffset == -1)
        endOffset = interface->textInterface()->characterCount();

    QAccessibleTextInterface *textInterface = interface->textInterface();
    if (endOffset <= startOffset || !textInterface)
        return QRect();

    QRect rect = textInterface->characterRect(startOffset);
    for (int i=startOffset + 1; i <= endOffset; i++)
        rect = rect | textInterface->characterRect(i);

    // relative to window
    if (coordType == ATSPI_COORD_TYPE_WINDOW)
        rect = translateRectToWindowCoordinates(interface, rect);

    return rect;
}

QRect AtSpiAdaptor::translateRectToWindowCoordinates(QAccessibleInterface *interface, const QRect &rect)
{
    QAccessibleInterface * window = getWindow(interface);
    if (window)
        return rect.translated(-window->rect().x(), -window->rect().y());

    return rect;
}


// Editable Text interface
static QString textForRange(QAccessibleInterface *accessible, int startOffset, int endOffset)
{
    if (QAccessibleTextInterface *textIface = accessible->textInterface()) {
        if (endOffset == -1)
            endOffset = textIface->characterCount();
        return textIface->text(startOffset, endOffset);
    }
    QString txt = accessible->text(QAccessible::Value);
    if (endOffset == -1)
        endOffset = txt.length();
    return txt.mid(startOffset, endOffset - startOffset);
}

static void replaceTextFallback(QAccessibleInterface *accessible, long startOffset, long endOffset, const QString &txt)
{
    QString t = textForRange(accessible, 0, -1);
    if (endOffset == -1)
        endOffset = t.length();
    if (endOffset - startOffset == 0)
        t.insert(startOffset, txt);
    else
        t.replace(startOffset, endOffset - startOffset, txt);
    accessible->setText(QAccessible::Value, t);
}

bool AtSpiAdaptor::editableTextInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == QLatin1String("CopyText")) {
#ifndef QT_NO_CLIPBOARD
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        const QString t = textForRange(interface, startOffset, endOffset);
        QGuiApplication::clipboard()->setText(t);
#endif
        connection.send(message.createReply(true));
    } else if (function == QLatin1String("CutText")) {
#ifndef QT_NO_CLIPBOARD
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        const QString t = textForRange(interface, startOffset, endOffset);
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->deleteText(startOffset, endOffset);
        else
            replaceTextFallback(interface, startOffset, endOffset, QString());
        QGuiApplication::clipboard()->setText(t);
#endif
        connection.send(message.createReply(true));
    } else if (function == QLatin1String("DeleteText")) {
        int startOffset = message.arguments().at(0).toInt();
        int endOffset = message.arguments().at(1).toInt();
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->deleteText(startOffset, endOffset);
        else
            replaceTextFallback(interface, startOffset, endOffset, QString());
        connection.send(message.createReply(true));
    } else if (function == QLatin1String("InsertText")) {
        int position = message.arguments().at(0).toInt();
        QString text = message.arguments().at(1).toString();
        int length = message.arguments().at(2).toInt();
        text.resize(length);
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->insertText(position, text);
        else
            replaceTextFallback(interface, position, position, text);
        connection.send(message.createReply(true));
    } else if (function == QLatin1String("PasteText")) {
#ifndef QT_NO_CLIPBOARD
        int position = message.arguments().at(0).toInt();
        const QString txt = QGuiApplication::clipboard()->text();
        if (QAccessibleEditableTextInterface *editableTextIface = interface->editableTextInterface())
            editableTextIface->insertText(position, txt);
        else
            replaceTextFallback(interface, position, position, txt);
#endif
        connection.send(message.createReply(true));
    } else if (function == QLatin1String("SetTextContents")) {
        QString newContents = message.arguments().at(0).toString();
        interface->editableTextInterface()->replaceText(0, interface->textInterface()->characterCount(), newContents);
        connection.send(message.createReply(true));
    } else if (function == QLatin1String("")) {
        connection.send(message.createReply());
    } else {
        qAtspiDebug() << "WARNING: AtSpiAdaptor::editableTextInterface does not implement " << function << message.path();
        return false;
    }
    return true;
}

// Value interface
bool AtSpiAdaptor::valueInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (function == QLatin1String("SetCurrentValue")) {
        QDBusVariant v = message.arguments().at(2).value<QDBusVariant>();
        double value = v.variant().toDouble();
        //Temporary fix
        //See https://bugzilla.gnome.org/show_bug.cgi?id=652596
        interface->valueInterface()->setCurrentValue(value);
        connection.send(message.createReply()); // FIXME is the reply needed?
    } else {
        QVariant value;
        if (function == QLatin1String("GetCurrentValue"))
            value = interface->valueInterface()->currentValue();
        else if (function == QLatin1String("GetMaximumValue"))
            value = interface->valueInterface()->maximumValue();
        else if (function == QLatin1String("GetMinimumIncrement"))
            value = interface->valueInterface()->minimumStepSize();
        else if (function == QLatin1String("GetMinimumValue"))
            value = interface->valueInterface()->minimumValue();
        else {
            qAtspiDebug() << "WARNING: AtSpiAdaptor::valueInterface does not implement " << function << message.path();
            return false;
        }
        if (!value.canConvert(QVariant::Double)) {
            qAtspiDebug() << "AtSpiAdaptor::valueInterface: Could not convert to double: " << function;
        }

        // explicitly convert to dbus-variant containing one double since atspi expects that
        // everything else might fail to convert back on the other end
        connection.send(message.createReply(
                            QVariant::fromValue(QDBusVariant(QVariant::fromValue(value.toDouble())))));
    }
    return true;
}

// Table interface
bool AtSpiAdaptor::tableInterface(QAccessibleInterface *interface, const QString &function, const QDBusMessage &message, const QDBusConnection &connection)
{
    if (!(interface->tableInterface() || interface->tableCellInterface())) {
        qAtspiDebug() << "WARNING Qt AtSpiAdaptor: Could not find table interface for: " << message.path() << interface;
        return false;
    }

    if (0) {
    // properties
    } else if (function == QLatin1String("GetCaption")) {
        QAccessibleInterface * captionInterface= interface->tableInterface()->caption();
        if (captionInterface) {
            QSpiObjectReference ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(captionInterface)));
            sendReply(connection, message, QVariant::fromValue(ref));
        } else {
            sendReply(connection, message, QVariant::fromValue(
                          QSpiObjectReference(connection, QDBusObjectPath(ATSPI_DBUS_PATH_NULL))));
        }
    } else if (function == QLatin1String("GetNColumns")) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->columnCount())))));
    } else if (function == QLatin1String("GetNRows")) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->rowCount())))));
    } else if (function == QLatin1String("GetNSelectedColumns")) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->selectedColumnCount())))));
    } else if (function == QLatin1String("GetNSelectedRows")) {
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(
            QVariant::fromValue(interface->tableInterface()->selectedRowCount())))));
    } else if (function == QLatin1String("GetSummary")) {
        QAccessibleInterface * summary = interface->tableInterface() ? interface->tableInterface()->summary() : 0;
        QSpiObjectReference ref(connection, QDBusObjectPath(pathForInterface(summary)));
        connection.send(message.createReply(QVariant::fromValue(QDBusVariant(QVariant::fromValue(ref)))));
    } else if (function == QLatin1String("GetAccessibleAt")) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        if ((row < 0) ||
            (column < 0) ||
            (row >= interface->tableInterface()->rowCount()) ||
            (column >= interface->tableInterface()->columnCount())) {
            qAtspiDebug() << "WARNING: invalid index for tableInterface GetAccessibleAt (" << row << ", " << column << ")";
            return false;
        }

        QSpiObjectReference ref;
        QAccessibleInterface * cell(interface->tableInterface()->cellAt(row, column));
        if (cell) {
            ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(cell)));
        } else {
            qAtspiDebug() << "WARNING: no cell interface returned for " << interface->object() << row << column;
            ref = QSpiObjectReference();
        }
        connection.send(message.createReply(QVariant::fromValue(ref)));

    } else if (function == QLatin1String("GetIndexAt")) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        QAccessibleInterface *cell = interface->tableInterface()->cellAt(row, column);
        if (!cell) {
            qAtspiDebug() << "WARNING: AtSpiAdaptor::GetIndexAt(" << row << "," << column << ") did not find a cell. " << interface;
            return false;
        }
        int index = interface->indexOfChild(cell);
        qAtspiDebug() << "QSpiAdaptor::GetIndexAt row:" << row << " col:" << column << " logical index:" << index;
        Q_ASSERT(index > 0);
        connection.send(message.createReply(index));
    } else if ((function == QLatin1String("GetColumnAtIndex")) || (function == QLatin1String("GetRowAtIndex"))) {
        int index = message.arguments().at(0).toInt();
        int ret = -1;
        if (index >= 0) {
            QAccessibleInterface * cell = interface->child(index);
            if (cell) {
                if (function == QLatin1String("GetColumnAtIndex")) {
                    if (cell->role() == QAccessible::ColumnHeader) {
                        ret = index;
                    } else if (cell->role() == QAccessible::RowHeader) {
                        ret = -1;
                    } else {
                        if (!cell->tableCellInterface()) {
                            qAtspiDebug() << "WARNING: AtSpiAdaptor::" << function << " No table cell interface: " << cell;
                            return false;
                        }
                        ret = cell->tableCellInterface()->columnIndex();
                    }
                } else {
                    if (cell->role() == QAccessible::ColumnHeader) {
                        ret = -1;
                    } else if (cell->role() == QAccessible::RowHeader) {
                        ret = index % interface->tableInterface()->columnCount();
                    } else {
                        if (!cell->tableCellInterface()) {
                            qAtspiDebug() << "WARNING: AtSpiAdaptor::" << function << " No table cell interface: " << cell;
                            return false;
                        }
                        ret = cell->tableCellInterface()->rowIndex();
                    }
                }
            } else {
                qAtspiDebug() << "WARNING: AtSpiAdaptor::" << function << " No cell at index: " << index << interface;
                return false;
            }
        }
        connection.send(message.createReply(ret));

    } else if (function == QLatin1String("GetColumnDescription")) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->columnDescription(column)));
    } else if (function == QLatin1String("GetRowDescription")) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->rowDescription(row)));



    } else if (function == QLatin1String("GetRowColumnExtentsAtIndex")) {
        int index = message.arguments().at(0).toInt();
        bool success = false;

        int row = -1;
        int col = -1;
        int rowExtents = -1;
        int colExtents = -1;
        bool isSelected = false;

        int cols = interface->tableInterface()->columnCount();
        if (cols > 0) {
            row = index / cols;
            col = index % cols;
            QAccessibleTableCellInterface *cell = interface->tableInterface()->cellAt(row, col)->tableCellInterface();
            if (cell) {
                row = cell->rowIndex();
                col = cell->columnIndex();
                rowExtents = cell->rowExtent();
                colExtents = cell->columnExtent();
                isSelected = cell->isSelected();
                success = true;
            }
        }
        QVariantList list;
        list << success << row << col << rowExtents << colExtents << isSelected;
        connection.send(message.createReply(list));

    } else if (function == QLatin1String("GetColumnExtentAt")) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        connection.send(message.createReply(interface->tableInterface()->cellAt(row, column)->tableCellInterface()->columnExtent()));

    } else if (function == QLatin1String("GetRowExtentAt")) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        connection.send(message.createReply(interface->tableInterface()->cellAt(row, column)->tableCellInterface()->rowExtent()));

    } else if (function == QLatin1String("GetColumnHeader")) {
        int column = message.arguments().at(0).toInt();
        QSpiObjectReference ref;

        QAccessibleInterface * cell(interface->tableInterface()->cellAt(0, column));
        if (cell && cell->tableCellInterface()) {
            QList<QAccessibleInterface*> header = cell->tableCellInterface()->columnHeaderCells();
            if (header.size() > 0) {
                ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(header.takeAt(0))));
            }
        }
        connection.send(message.createReply(QVariant::fromValue(ref)));

    } else if (function == QLatin1String("GetRowHeader")) {
        int row = message.arguments().at(0).toInt();
        QSpiObjectReference ref;
        QAccessibleTableCellInterface *cell = interface->tableInterface()->cellAt(row, 0)->tableCellInterface();
        if (cell) {
            QList<QAccessibleInterface*> header = cell->rowHeaderCells();
            if (header.size() > 0) {
                ref = QSpiObjectReference(connection, QDBusObjectPath(pathForInterface(header.takeAt(0))));
            }
        }
        connection.send(message.createReply(QVariant::fromValue(ref)));

    } else if (function == QLatin1String("GetSelectedColumns")) {
        connection.send(message.createReply(QVariant::fromValue(interface->tableInterface()->selectedColumns())));
    } else if (function == QLatin1String("GetSelectedRows")) {
        connection.send(message.createReply(QVariant::fromValue(interface->tableInterface()->selectedRows())));
    } else if (function == QLatin1String("IsColumnSelected")) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->isColumnSelected(column)));
    } else if (function == QLatin1String("IsRowSelected")) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->isRowSelected(row)));
    } else if (function == QLatin1String("IsSelected")) {
        int row = message.arguments().at(0).toInt();
        int column = message.arguments().at(1).toInt();
        QAccessibleTableCellInterface* cell = interface->tableInterface()->cellAt(row, column)->tableCellInterface();
        connection.send(message.createReply(cell->isSelected()));
    } else if (function == QLatin1String("AddColumnSelection")) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->selectColumn(column)));
    } else if (function == QLatin1String("AddRowSelection")) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->selectRow(row)));
    } else if (function == QLatin1String("RemoveColumnSelection")) {
        int column = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->unselectColumn(column)));
    } else if (function ==  QLatin1String("RemoveRowSelection")) {
        int row = message.arguments().at(0).toInt();
        connection.send(message.createReply(interface->tableInterface()->unselectRow(row)));
    } else {
        qAtspiDebug() << "WARNING: AtSpiAdaptor::tableInterface does not implement " << function << message.path();
        return false;
    }
    return true;
}

QT_END_NAMESPACE

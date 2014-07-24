/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPopupMenuProxyQt.h"

#include "PlatformPopupMenuData.h"
#include "WebPopupItem.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

using namespace WebCore;

namespace WebKit {

static QHash<int, QByteArray> createRoleNamesHash();

class PopupMenuItemModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        GroupRole = Qt::UserRole,
        EnabledRole = Qt::UserRole + 1,
        SelectedRole = Qt::UserRole + 2,
        IsSeparatorRole = Qt::UserRole + 3
    };

    PopupMenuItemModel(const Vector<WebPopupItem>&, bool multiple);
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const { return m_items.size(); }
    virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void select(int);

    int selectedOriginalIndex() const;
    bool multiple() const { return m_allowMultiples; }
    void toggleItem(int);

Q_SIGNALS:
    void indexUpdated();

private:
    struct Item {
        Item(const WebPopupItem& webPopupItem, const QString& group, int originalIndex)
            : text(webPopupItem.m_text)
            , toolTip(webPopupItem.m_toolTip)
            , group(group)
            , originalIndex(originalIndex)
            , enabled(webPopupItem.m_isEnabled)
            , selected(webPopupItem.m_isSelected)
            , isSeparator(webPopupItem.m_type == WebPopupItem::Separator)
        { }

        QString text;
        QString toolTip;
        QString group;
        // Keep track of originalIndex because we don't add the label (group) items to our vector.
        int originalIndex;
        bool enabled;
        bool selected;
        bool isSeparator;
    };

    void buildItems(const Vector<WebPopupItem>& webPopupItems);

    Vector<Item> m_items;
    int m_selectedModelIndex;
    bool m_allowMultiples;
};

class ItemSelectorContextObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QRectF elementRect READ elementRect CONSTANT FINAL)
    Q_PROPERTY(QObject* items READ items CONSTANT FINAL)
    Q_PROPERTY(bool allowMultiSelect READ allowMultiSelect CONSTANT FINAL)

public:
    ItemSelectorContextObject(const QRectF& elementRect, const Vector<WebPopupItem>&, bool multiple);

    QRectF elementRect() const { return m_elementRect; }
    PopupMenuItemModel* items() { return &m_items; }
    bool allowMultiSelect() { return m_items.multiple(); }

    Q_INVOKABLE void accept(int index = -1);
    Q_INVOKABLE void reject() { emit done(); }
    Q_INVOKABLE void dismiss() { emit done(); }

Q_SIGNALS:
    void acceptedWithOriginalIndex(int);
    void done();

private Q_SLOTS:
    void onIndexUpdate();

private:
    QRectF m_elementRect;
    PopupMenuItemModel m_items;
};

ItemSelectorContextObject::ItemSelectorContextObject(const QRectF& elementRect, const Vector<WebPopupItem>& webPopupItems, bool multiple)
    : m_elementRect(elementRect)
    , m_items(webPopupItems, multiple)
{
    connect(&m_items, SIGNAL(indexUpdated()), SLOT(onIndexUpdate()));
}

void ItemSelectorContextObject::onIndexUpdate()
{
    // Send the update for multi-select list.
    if (m_items.multiple())
        emit acceptedWithOriginalIndex(m_items.selectedOriginalIndex());
}


void ItemSelectorContextObject::accept(int index)
{
    // If the index is not valid for multi-select lists, just hide the pop up as the selected indices have
    // already been sent.
    if ((index == -1) && m_items.multiple())
        emit done();
    else {
        if (index != -1)
            m_items.toggleItem(index);
        emit acceptedWithOriginalIndex(m_items.selectedOriginalIndex());
    }
}

static QHash<int, QByteArray> createRoleNamesHash()
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "text";
    roles[Qt::ToolTipRole] = "tooltip";
    roles[PopupMenuItemModel::GroupRole] = "group";
    roles[PopupMenuItemModel::EnabledRole] = "enabled";
    roles[PopupMenuItemModel::SelectedRole] = "selected";
    roles[PopupMenuItemModel::IsSeparatorRole] = "isSeparator";
    return roles;
}

PopupMenuItemModel::PopupMenuItemModel(const Vector<WebPopupItem>& webPopupItems, bool multiple)
    : m_selectedModelIndex(-1)
    , m_allowMultiples(multiple)
{
    buildItems(webPopupItems);
}

QHash<int, QByteArray> PopupMenuItemModel::roleNames() const
{
    static QHash<int, QByteArray> roles = createRoleNamesHash();
    return roles;
}

QVariant PopupMenuItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const Item& item = m_items[index.row()];
    if (item.isSeparator) {
        if (role == IsSeparatorRole)
            return true;
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return item.text;
    case Qt::ToolTipRole:
        return item.toolTip;
    case GroupRole:
        return item.group;
    case EnabledRole:
        return item.enabled;
    case SelectedRole:
        return item.selected;
    case IsSeparatorRole:
        return false;
    }

    return QVariant();
}

void PopupMenuItemModel::select(int index)
{
    toggleItem(index);
    emit indexUpdated();
}

void PopupMenuItemModel::toggleItem(int index)
{
    int oldIndex = m_selectedModelIndex;
    if (index < 0 || index >= m_items.size())
        return;
    Item& item = m_items[index];
    if (!item.enabled)
        return;

    m_selectedModelIndex = index;
    if (m_allowMultiples)
        item.selected = !item.selected;
    else {
        if (index == oldIndex)
            return;
        item.selected = true;
        if (oldIndex != -1) {
            Item& oldItem = m_items[oldIndex];
            oldItem.selected = false;
            emit dataChanged(this->index(oldIndex), this->index(oldIndex));
        }
    }

    emit dataChanged(this->index(index), this->index(index));
}

int PopupMenuItemModel::selectedOriginalIndex() const
{
    if (m_selectedModelIndex == -1)
        return -1;
    return m_items[m_selectedModelIndex].originalIndex;
}

void PopupMenuItemModel::buildItems(const Vector<WebPopupItem>& webPopupItems)
{
    QString currentGroup;
    m_items.reserveInitialCapacity(webPopupItems.size());
    for (size_t i = 0; i < webPopupItems.size(); i++) {
        const WebPopupItem& webPopupItem = webPopupItems[i];
        if (webPopupItem.m_isLabel) {
            currentGroup = webPopupItem.m_text;
            continue;
        }
        if (webPopupItem.m_isSelected && !m_allowMultiples)
            m_selectedModelIndex = m_items.size();
        m_items.append(Item(webPopupItem, currentGroup, i));
    }
}

WebPopupMenuProxyQt::WebPopupMenuProxyQt(WebPopupMenuProxy::Client* client, QQuickWebView* webView)
    : WebPopupMenuProxy(client)
    , m_webView(webView)
{
}

WebPopupMenuProxyQt::~WebPopupMenuProxyQt()
{
}

void WebPopupMenuProxyQt::showPopupMenu(const IntRect& rect, WebCore::TextDirection, double, const Vector<WebPopupItem>& items, const PlatformPopupMenuData& data, int32_t)
{
    m_selectionType = (data.multipleSelections) ? WebPopupMenuProxyQt::MultipleSelection : WebPopupMenuProxyQt::SingleSelection;

    const QRectF mappedRect= m_webView->mapRectFromWebContent(QRect(rect));
    ItemSelectorContextObject* contextObject = new ItemSelectorContextObject(mappedRect, items, (m_selectionType == WebPopupMenuProxyQt::MultipleSelection));
    createItem(contextObject);
    if (!m_itemSelector) {
        hidePopupMenu();
        return;
    }
}

void WebPopupMenuProxyQt::hidePopupMenu()
{
    m_itemSelector.clear();
    m_context.clear();

    if (m_client) {
        m_client->closePopupMenu();
        invalidate();
    }
}

void WebPopupMenuProxyQt::selectIndex(int index)
{
    m_client->changeSelectedIndex(index);
}

void WebPopupMenuProxyQt::createItem(QObject* contextObject)
{
    QQmlComponent* component = m_webView->experimental()->itemSelector();
    if (!component) {
        delete contextObject;
        return;
    }

    createContext(component, contextObject);
    QObject* object = component->beginCreate(m_context.get());
    if (!object)
        return;

    m_itemSelector = adoptPtr(qobject_cast<QQuickItem*>(object));
    if (!m_itemSelector)
        return;

    connect(contextObject, SIGNAL(acceptedWithOriginalIndex(int)), SLOT(selectIndex(int)));

    // We enqueue these because they are triggered by m_itemSelector and will lead to its destruction.
    connect(contextObject, SIGNAL(done()), SLOT(hidePopupMenu()), Qt::QueuedConnection);
    if (m_selectionType == WebPopupMenuProxyQt::SingleSelection)
        connect(contextObject, SIGNAL(acceptedWithOriginalIndex(int)), SLOT(hidePopupMenu()), Qt::QueuedConnection);

    QQuickWebViewPrivate::get(m_webView)->addAttachedPropertyTo(m_itemSelector.get());
    m_itemSelector->setParentItem(m_webView);

    // Only fully create the component once we've set both a parent
    // and the needed context and attached properties, so that the
    // dialog can do useful stuff in Component.onCompleted().
    component->completeCreate();
}

void WebPopupMenuProxyQt::createContext(QQmlComponent* component, QObject* contextObject)
{
    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = QQmlEngine::contextForObject(m_webView);
    m_context = adoptPtr(new QQmlContext(baseContext));

    contextObject->setParent(m_context.get());
    m_context->setContextProperty(QLatin1String("model"), contextObject);
    m_context->setContextObject(contextObject);
}

} // namespace WebKit

// Since we define QObjects in WebPopupMenuProxyQt.cpp, this will trigger moc to run on .cpp.
#include "WebPopupMenuProxyQt.moc"

// And we can't compile the moc for WebPopupMenuProxyQt.h by itself, since it doesn't include "config.h"
#include "moc_WebPopupMenuProxyQt.cpp"

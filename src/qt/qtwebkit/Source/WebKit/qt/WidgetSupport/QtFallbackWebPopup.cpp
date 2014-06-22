/*
 * Copyright (C) 2010 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "config.h"
#include "QtFallbackWebPopup.h"

#ifndef QT_NO_COMBOBOX

#include "QWebPageAdapter.h"
#include "QWebPageClient.h"
#include "QtWebComboBox.h"
#include "qgraphicswebview.h"
#include <QGraphicsProxyWidget>
#include <QtGui/QStandardItemModel>

namespace WebCore {

QtFallbackWebPopup::QtFallbackWebPopup(const QWebPageAdapter* page)
    : m_combo(0)
    , m_page(page)
{
}

QtFallbackWebPopup::~QtFallbackWebPopup()
{
    deleteComboBox();
}

void QtFallbackWebPopup::show(const QWebSelectData& data)
{
    if (!pageClient())
        return;

    deleteComboBox();

    m_combo = new QtWebComboBox();
    connect(m_combo, SIGNAL(activated(int)), SLOT(activeChanged(int)), Qt::QueuedConnection);
    connect(m_combo, SIGNAL(didHide()), SLOT(deleteComboBox()));
    connect(m_combo, SIGNAL(didHide()), SIGNAL(didHide()));

    populate(data);

    QRect rect = geometry();
#ifdef QT_NO_GRAPHICSVIEW
    if (false) {
#else
    if (QGraphicsWebView *webView = qobject_cast<QGraphicsWebView*>(pageClient()->pluginParent())) {
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(webView);
        proxy->setWidget(m_combo);
        proxy->setGeometry(rect);
#endif
    } else {
        m_combo->setParent(qobject_cast<QWidget*>(pageClient()->ownerWidget()));
        m_combo->setGeometry(QRect(rect.left(), rect.top(), rect.width(), m_combo->sizeHint().height()));
    }

    m_combo->showPopupAtCursorPosition();
}

void QtFallbackWebPopup::hide()
{
    // Destroying the QComboBox here cause problems if the popup is in the
    // middle of its show animation. Instead we rely on the fact that the
    // Qt::Popup window will hide itself on mouse events outside its window.
}

void QtFallbackWebPopup::populate(const QWebSelectData& data)
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(m_combo->model());
    Q_ASSERT(model);

    m_combo->setFont(font());

    int currentIndex = -1;
    for (int i = 0; i < data.itemCount(); ++i) {
        switch (data.itemType(i)) {
        case QWebSelectData::Separator:
            m_combo->insertSeparator(i);
            break;
        case QWebSelectData::Group:
            m_combo->insertItem(i, data.itemText(i));
            model->item(i)->setEnabled(false);
            break;
        case QWebSelectData::Option:
            m_combo->insertItem(i, data.itemText(i));
            model->item(i)->setEnabled(data.itemIsEnabled(i));
#ifndef QT_NO_TOOLTIP
            model->item(i)->setToolTip(data.itemToolTip(i));
#endif
            model->item(i)->setBackground(data.itemBackgroundColor(i));
            model->item(i)->setForeground(data.itemForegroundColor(i));
            if (data.itemIsSelected(i))
                currentIndex = i;
            break;
        }
    }

    if (currentIndex >= 0)
        m_combo->setCurrentIndex(currentIndex);
}

void QtFallbackWebPopup::activeChanged(int index)
{
    if (index < 0)
        return;

    emit selectItem(index, false, false);
}

void QtFallbackWebPopup::deleteComboBox()
{
    if (!m_combo)
        return;
    m_combo->deleteComboBox();
    m_combo = 0;
}

QWebPageClient* QtFallbackWebPopup::pageClient() const
{
    return m_page->client.data();
}

}

#endif // QT_NO_COMBOBOX

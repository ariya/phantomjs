/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
#include "QtMaemoWebPopup.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include <libintl.h>


namespace WebCore {

static const int gMaemoListItemSize = 70;
static const int gMaemoListPadding = 38;
static const int gMaemoMaxVisibleItems = 5;

void Maemo5Popup::populateList()
{
    QListWidgetItem* listItem;
    for (int i = 0; i < m_data.itemCount(); ++i) {
        if (m_data.itemType(i) == QWebSelectData::Option) {
            listItem = new QListWidgetItem(m_data.itemText(i));
            m_list->addItem(listItem);
            listItem->setSelected(m_data.itemIsSelected(i));
        } else if (m_data.itemType(i) == QWebSelectData::Group) {
            listItem = new QListWidgetItem(m_data.itemText(i));
            m_list->addItem(listItem);
            listItem->setSelected(false);
            listItem->setFlags(Qt::NoItemFlags);
        }
    }
    connect(m_list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onItemSelected(QListWidgetItem*)));
}

void Maemo5Popup::onItemSelected(QListWidgetItem* item)
{
    if (item->flags() != Qt::NoItemFlags)
        emit itemClicked(m_list->row(item));
}

QtMaemoWebPopup::QtMaemoWebPopup()
    : m_popup(0)
{
}

QtMaemoWebPopup::~QtMaemoWebPopup()
{
    if (m_popup)
        m_popup->deleteLater();
}

Maemo5Popup* QtMaemoWebPopup::createSingleSelectionPopup(const QWebSelectData& data)
{
    return new Maemo5SingleSelectionPopup(data);
}

Maemo5Popup* QtMaemoWebPopup::createMultipleSelectionPopup(const QWebSelectData& data)
{
    return new Maemo5MultipleSelectionPopup(data);
}

Maemo5Popup* QtMaemoWebPopup::createPopup(const QWebSelectData& data)
{
    Maemo5Popup* result = data.multiple() ? createMultipleSelectionPopup(data) : createSingleSelectionPopup(data);
    connect(result, SIGNAL(finished(int)), this, SLOT(popupClosed()));
    connect(result, SIGNAL(itemClicked(int)), this, SLOT(itemClicked(int)));
    return result;
}

void QtMaemoWebPopup::show(const QWebSelectData& data)
{
    if (m_popup)
        return;

    m_popup = createPopup(data);
    m_popup->show();
}

void QtMaemoWebPopup::hide()
{
    if (!m_popup)
        return;

    m_popup->accept();
}

void QtMaemoWebPopup::popupClosed()
{
    if (!m_popup)
        return;

    m_popup->deleteLater();
    m_popup = 0;
    emit didHide();
}

void QtMaemoWebPopup::itemClicked(int idx)
{
    emit selectItem(idx, true, false);
}

Maemo5SingleSelectionPopup::Maemo5SingleSelectionPopup(const QWebSelectData& data)
    : Maemo5Popup(data)
{
    // we try to get the standard list title the web browser is using
    const char* title = ::dgettext("osso-browser-ui", "weba_ti_texlist_single");
    if (qstrcmp(title, "weba_ti_texlist_single"))
        setWindowTitle(QString::fromUtf8(title));
    else
        setWindowTitle("Select item");

    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 0, 0);

    m_list = new QListWidget(this);
    populateList();

    hLayout->addSpacing(gMaemoListPadding);
    hLayout->addWidget(m_list);
    hLayout->addSpacing(gMaemoListPadding);

    connect(m_list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(accept()));

    const int visibleItemCount = (m_list->count() > gMaemoMaxVisibleItems) ? gMaemoMaxVisibleItems : m_list->count();
    resize(size().width(), visibleItemCount * gMaemoListItemSize);
}


class MultipleItemListDelegate : public QStyledItemDelegate {
public:
    MultipleItemListDelegate(QObject* parent = 0)
           : QStyledItemDelegate(parent)
    {
        tickMark = QIcon::fromTheme("widgets_tickmark_list").pixmap(48, 48);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyledItemDelegate::paint(painter, option, index);

        if (option.state & QStyle::State_Selected)
            painter->drawPixmap(option.rect.width() - tickMark.rect().width(), option.rect.y() + (option.rect.height() / 2 - tickMark.rect().height() / 2), tickMark);
    }

private:
    QPixmap tickMark;
};

Maemo5MultipleSelectionPopup::Maemo5MultipleSelectionPopup(const QWebSelectData& data)
    : Maemo5Popup(data)
{
    // we try to get the standard list title the web browser is using
    const char* title = ::dgettext("osso-browser-ui", "weba_ti_textlist_multi");
    if (qstrcmp(title, "weba_ti_textlist_multi"))
        setWindowTitle(QString::fromUtf8(title));
    else
        setWindowTitle("Select items");

    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 0, 0);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::MultiSelection);
    populateList();

    MultipleItemListDelegate* delegate = new MultipleItemListDelegate(this);
    m_list->setItemDelegate(delegate);

    hLayout->addSpacing(gMaemoListPadding);
    hLayout->addWidget(m_list);

    QVBoxLayout* vLayout = new QVBoxLayout();

    const int visibleItemCount = (m_list->count() > gMaemoMaxVisibleItems) ? gMaemoMaxVisibleItems : m_list->count();
    vLayout->addSpacing((visibleItemCount - 1) * gMaemoListItemSize);

    // we try to get the standard Done button title
    QPushButton* done = new QPushButton(this);
    title = ::dgettext("hildon-libs", "wdgt_bd_done");
    if (qstrcmp(title, "wdgt_bd_done"))
        done->setText(QString::fromUtf8(title));
    else
        done->setText("Done");

    done->setMinimumWidth(178);
    vLayout->addWidget(done);

    hLayout->addSpacing(8);
    hLayout->addLayout(vLayout);
    hLayout->addSpacing(18);

    connect(done, SIGNAL(clicked()), this, SLOT(accept()));
    resize(size().width(), visibleItemCount * gMaemoListItemSize);
}

}

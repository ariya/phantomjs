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
#include "WebPlugin.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QtPlugin>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

static const int gMaemoListItemSize = 70;
static const int gMaemoListPadding = 38;
static const int gMaemoMaxVisibleItems = 5;

void Popup::populateList()
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

void Popup::onItemSelected(QListWidgetItem* item)
{
    if (item->flags() != Qt::NoItemFlags)
        emit itemClicked(m_list->row(item));
}

WebPopup::WebPopup()
    : m_popup(0)
{
}

WebPopup::~WebPopup()
{
    if (m_popup)
        m_popup->deleteLater();
}

Popup* WebPopup::createSingleSelectionPopup(const QWebSelectData& data)
{
    return new SingleSelectionPopup(data);
}

Popup* WebPopup::createMultipleSelectionPopup(const QWebSelectData& data)
{
    return new MultipleSelectionPopup(data);
}

Popup* WebPopup::createPopup(const QWebSelectData& data)
{
    Popup* result = data.multiple() ? createMultipleSelectionPopup(data) : createSingleSelectionPopup(data);
    connect(result, SIGNAL(finished(int)), this, SLOT(popupClosed()));
    connect(result, SIGNAL(itemClicked(int)), this, SLOT(itemClicked(int)));
    return result;
}

void WebPopup::show(const QWebSelectData& data)
{
    if (m_popup)
        return;

    m_popup = createPopup(data);
    m_popup->show();
}

void WebPopup::hide()
{
    if (!m_popup)
        return;

    m_popup->accept();
}

void WebPopup::popupClosed()
{
    if (!m_popup)
        return;

    m_popup->deleteLater();
    m_popup = 0;
    emit didHide();
}

void WebPopup::itemClicked(int idx)
{
    emit selectItem(idx, true, false);
}

SingleSelectionPopup::SingleSelectionPopup(const QWebSelectData& data)
    : Popup(data)
{
    const char* title = "select";
    if (qstrcmp(title, "weba_ti_texlist_single"))
        setWindowTitle(QString::fromUtf8(title));
    else
        setWindowTitle(QLatin1String("Select item"));

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
        tickMark = QIcon::fromTheme(QLatin1String("widgets_tickmark_list")).pixmap(48, 48);
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

MultipleSelectionPopup::MultipleSelectionPopup(const QWebSelectData& data)
    : Popup(data)
{
    const char* title = "select";
    if (qstrcmp(title, "weba_ti_textlist_multi"))
        setWindowTitle(QString::fromUtf8(title));
    else
        setWindowTitle(QLatin1String("Select items"));

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

    QPushButton* done = new QPushButton(this);
    title = "done";
    if (qstrcmp(title, "wdgt_bd_done"))
        done->setText(QString::fromUtf8(title));
    else
        done->setText(QLatin1String("Done"));

    done->setMinimumWidth(178);
    vLayout->addWidget(done);

    hLayout->addSpacing(8);
    hLayout->addLayout(vLayout);
    hLayout->addSpacing(18);

    connect(done, SIGNAL(clicked()), this, SLOT(accept()));
    resize(size().width(), visibleItemCount * gMaemoListItemSize);
}

#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
FullScreenVideoWidget::FullScreenVideoWidget(QMediaPlayer* player)
    : QVideoWidget()
    , m_mediaPlayer(player)
{
    Q_ASSERT(m_mediaPlayer);

    setFullScreen(true);
    m_mediaPlayer->setVideoOutput(this);
}

bool FullScreenVideoWidget::event(QEvent* ev)
{
    if (ev->type() ==  QEvent::MouseButtonDblClick) {
        emit fullScreenClosed();
        ev->accept();
        return true;
    } 
    return QWidget::event(ev);
}

void FullScreenVideoWidget::keyPressEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Space) {
        if (m_mediaPlayer->state() == QMediaPlayer::PlayingState)
            m_mediaPlayer->pause();
        else
            m_mediaPlayer->play();
        ev->accept();
        return;
    }
}

FullScreenVideoHandler::FullScreenVideoHandler()
    : m_mediaWidget(0)
{
}

FullScreenVideoHandler::~FullScreenVideoHandler()
{
    delete m_mediaWidget;
}

bool FullScreenVideoHandler::requiresFullScreenForVideoPlayback() const
{
    return true;
}

void FullScreenVideoHandler::enterFullScreen(QMediaPlayer* player)
{
    Q_ASSERT(player);

    m_mediaWidget = new FullScreenVideoWidget(player);
    connect(m_mediaWidget, SIGNAL(fullScreenClosed()), this, SIGNAL(fullScreenClosed()));
    m_mediaWidget->showFullScreen();
}

void FullScreenVideoHandler::exitFullScreen()
{
    m_mediaWidget->hide();
    delete m_mediaWidget;
    m_mediaWidget = 0;
}
#endif

bool WebPlugin::supportsExtension(Extension extension) const
{
    switch (extension) {
    case MultipleSelections:
        return true;
#if ENABLE_NOTIFICATIONS
    case Notifications:
        return true;
#endif
    case TouchInteraction:
        return true;
#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
    case FullScreenVideoPlayer:
        return true;
#endif
    default:
        return false;
    }
}

QObject* WebPlugin::createExtension(Extension extension) const
{
    switch (extension) {
    case MultipleSelections:
        return new WebPopup();
#if ENABLE_NOTIFICATIONS
    case Notifications:
        return new WebNotificationPresenter();
#endif
    case TouchInteraction:
        return new TouchModifier();
#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
    case FullScreenVideoPlayer:
        return new FullScreenVideoHandler();
#endif
    default:
        return 0;
    }
}

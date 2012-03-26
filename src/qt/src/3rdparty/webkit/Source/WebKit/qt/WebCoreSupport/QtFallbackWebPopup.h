/*
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
#ifndef QtFallbackWebPopup_h
#define QtFallbackWebPopup_h

#include "Platform.h"
#include "qwebkitplatformplugin.h"

#include <QComboBox>

#ifndef QT_NO_COMBOBOX

QT_BEGIN_NAMESPACE
class QGraphicsProxyWidget;
QT_END_NAMESPACE

class QWebPageClient;

namespace WebCore {

class ChromeClientQt;
class QtFallbackWebPopupCombo;

class QtFallbackWebPopup : public QWebSelectMethod {
    Q_OBJECT
public:
    QtFallbackWebPopup(const ChromeClientQt*);
    ~QtFallbackWebPopup();

    virtual void show(const QWebSelectData&);
    virtual void hide();

    void destroyPopup();

    void setGeometry(const QRect& rect) { m_geometry = rect; }
    QRect geometry() const { return m_geometry; }

    void setFont(const QFont& font) { m_font = font; }
    QFont font() const { return m_font; }

private slots:
    void activeChanged(int);

private:
    friend class QtFallbackWebPopupCombo;
    bool m_popupVisible;
    QtFallbackWebPopupCombo* m_combo;
    const ChromeClientQt* m_chromeClient;
    QRect m_geometry;
    QFont m_font;

    QWebPageClient* pageClient() const;

    void populate(const QWebSelectData&);
};

class QtFallbackWebPopupCombo : public QComboBox {
public:
    QtFallbackWebPopupCombo(QtFallbackWebPopup& ownerPopup);
    virtual void showPopup();
    virtual void hidePopup();
    virtual bool eventFilter(QObject* watched, QEvent* event);

private:
    QtFallbackWebPopup& m_ownerPopup;
};

}

#endif // QT_NO_COMBOBOX

#endif // QtFallbackWebPopup_h

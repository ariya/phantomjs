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
#ifndef QtMaemoWebPopup_h
#define QtMaemoWebPopup_h

#include "qwebkitplatformplugin.h"

#include <QDialog>

class QListWidgetItem;
class QListWidget;


namespace WebCore {

class Maemo5Popup : public QDialog {
    Q_OBJECT
public:
    Maemo5Popup(const QWebSelectData& data) : m_data(data) {}

signals:
    void itemClicked(int idx);

protected slots:
    void onItemSelected(QListWidgetItem* item);

protected:
    void populateList();

    const QWebSelectData& m_data;
    QListWidget* m_list;
};


class QtMaemoWebPopup : public QWebSelectMethod {
    Q_OBJECT
public:
    QtMaemoWebPopup();
    ~QtMaemoWebPopup();

    virtual void show(const QWebSelectData& data);
    virtual void hide();

private slots:
    void popupClosed();
    void itemClicked(int idx);

private:
    Maemo5Popup* m_popup;

    Maemo5Popup* createPopup(const QWebSelectData& data);
    Maemo5Popup* createSingleSelectionPopup(const QWebSelectData& data);
    Maemo5Popup* createMultipleSelectionPopup(const QWebSelectData& data);
};


class Maemo5SingleSelectionPopup : public Maemo5Popup {
    Q_OBJECT
public:
    Maemo5SingleSelectionPopup(const QWebSelectData& data);
};


class Maemo5MultipleSelectionPopup : public Maemo5Popup {
    Q_OBJECT
public:
    Maemo5MultipleSelectionPopup(const QWebSelectData& data);
};

}

#endif // QtMaemoWebPopup_h

/*
 * Copyright (C) 2010 Juha Savolainen (juha.savolainen@weego.fi)
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef qwebnavigationhistory_p_h
#define qwebnavigationhistory_p_h

#include "qwebkitglobal.h"
#include <QAbstractListModel>
#include <QObject>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE
class QAbstractListModel;
class QString;
class QUrl;
QT_END_NAMESPACE
class QWebNavigationHistoryPrivate;
class QWebNavigationListModelPrivate;

class QWEBKIT_EXPORT QWebNavigationListModel : public QAbstractListModel {
    Q_OBJECT
public:
    virtual ~QWebNavigationListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;
    QHash<int, QByteArray> roleNames() const;
    void reset();

private:
    QWebNavigationListModel()
        : QAbstractListModel()
    { }

    QWebNavigationListModelPrivate* d;
    friend class QWebNavigationListModelPrivate;
    friend class QWebNavigationHistory;
    friend class QWebNavigationHistoryPrivate;
};

QML_DECLARE_TYPE(QWebNavigationListModel)

class QWEBKIT_EXPORT QWebNavigationHistory : public QObject {
    Q_OBJECT
    Q_PROPERTY(QWebNavigationListModel* backItems READ backItems CONSTANT FINAL)
    Q_PROPERTY(QWebNavigationListModel* forwardItems READ forwardItems CONSTANT FINAL)
public:
    enum NavigationHistoryRoles {
        UrlRole = Qt::UserRole + 1,
        TitleRole = Qt::UserRole + 2
    };

    QWebNavigationListModel* backItems() const;
    QWebNavigationListModel* forwardItems() const;

    virtual ~QWebNavigationHistory();

private:
    QWebNavigationHistory();

    QWebNavigationHistoryPrivate* d;
    friend class QWebNavigationHistoryPrivate;
    friend class QQuickWebViewPrivate;
    friend class QQuickWebViewExperimental;
};

QML_DECLARE_TYPE(QWebNavigationHistory)

#endif /* qwebnavigationhistory_p_h */

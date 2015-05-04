/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWIDGETBACKINGSTORE_P_H
#define QWIDGETBACKINGSTORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QDebug>
#include <QtWidgets/qwidget.h>
#include <private/qwidget_p.h>
#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

class QPlatformTextureList;
class QPlatformTextureListWatcher;
class QWidgetBackingStore;

struct BeginPaintInfo {
    inline BeginPaintInfo() : wasFlushed(0), nothingToPaint(0), backingStoreRecreated(0) {}
    uint wasFlushed : 1;
    uint nothingToPaint : 1;
    uint backingStoreRecreated : 1;
};

#ifndef QT_NO_OPENGL
class QPlatformTextureListWatcher : public QObject
{
    Q_OBJECT

public:
    QPlatformTextureListWatcher(QWidgetBackingStore *backingStore);
    void watch(QPlatformTextureList *textureList);
    bool isLocked() const { return m_locked; }

private slots:
     void onLockStatusChanged(bool locked);

private:
     bool m_locked;
     QWidgetBackingStore *m_backingStore;
};
#endif

class Q_AUTOTEST_EXPORT QWidgetBackingStore
{
public:
    enum UpdateTime {
        UpdateNow,
        UpdateLater
    };

    enum BufferState{
        BufferValid,
        BufferInvalid
    };

    QWidgetBackingStore(QWidget *t);
    ~QWidgetBackingStore();

    static void showYellowThing(QWidget *widget, const QRegion &rgn, int msec, bool);

    void sync(QWidget *exposedWidget, const QRegion &exposedRegion);
    void sync();
    void flush(QWidget *widget = 0);

    inline QPoint topLevelOffset() const { return tlwOffset; }

    QBackingStore *backingStore() const { return store; }

    inline bool isDirty() const
    {
        return !(dirtyWidgets.isEmpty() && dirty.isEmpty() && !fullUpdatePending);
    }

    // ### Qt 4.6: Merge into a template function (after MSVC isn't supported anymore).
    void markDirty(const QRegion &rgn, QWidget *widget, UpdateTime updateTime = UpdateLater,
                   BufferState bufferState = BufferValid);
    void markDirty(const QRect &rect, QWidget *widget, UpdateTime updateTime = UpdateLater,
                   BufferState bufferState = BufferValid);

private:
    QWidget *tlw;
    QRegion dirtyOnScreen; // needsFlush
    QRegion dirty; // needsRepaint
    QRegion dirtyFromPreviousSync;
    QVector<QWidget *> dirtyWidgets;
    QVector<QWidget *> dirtyRenderToTextureWidgets;
    QVector<QWidget *> *dirtyOnScreenWidgets;
    QList<QWidget *> staticWidgets;
    QPlatformTextureList *widgetTextures;
    QBackingStore *store;
    uint fullUpdatePending : 1;
    uint updateRequestSent : 1;

    QPoint tlwOffset;

    QPlatformTextureListWatcher *textureListWatcher;
    QElapsedTimer perfTime;
    int perfFrames;

    void sendUpdateRequest(QWidget *widget, UpdateTime updateTime);

    static bool flushPaint(QWidget *widget, const QRegion &rgn);
    static void unflushPaint(QWidget *widget, const QRegion &rgn);
    static void qt_flush(QWidget *widget, const QRegion &region, QBackingStore *backingStore,
                         QWidget *tlw, const QPoint &tlwOffset,
                         QPlatformTextureList *widgetTextures,
                         QWidgetBackingStore *widgetBackingStore);

    void doSync();
    bool bltRect(const QRect &rect, int dx, int dy, QWidget *widget);
    void releaseBuffer();

    void beginPaint(QRegion &toClean, QWidget *widget, QBackingStore *backingStore,
                    BeginPaintInfo *returnInfo, bool toCleanIsInTopLevelCoordinates = true);
    void endPaint(const QRegion &cleaned, QBackingStore *backingStore, BeginPaintInfo *beginPaintInfo);

    QRegion dirtyRegion(QWidget *widget = 0) const;
    QRegion staticContents(QWidget *widget = 0, const QRect &withinClipRect = QRect()) const;

    void markDirtyOnScreen(const QRegion &dirtyOnScreen, QWidget *widget, const QPoint &topLevelOffset);

    void removeDirtyWidget(QWidget *w);

    void updateLists(QWidget *widget);

    inline void addDirtyWidget(QWidget *widget, const QRegion &rgn)
    {
        if (widget && !widget->d_func()->inDirtyList && !widget->data->in_destructor) {
            QWidgetPrivate *widgetPrivate = widget->d_func();
#ifndef QT_NO_GRAPHICSEFFECT
            if (widgetPrivate->graphicsEffect)
                widgetPrivate->dirty = widgetPrivate->effectiveRectFor(rgn.boundingRect());
            else
#endif //QT_NO_GRAPHICSEFFECT
                widgetPrivate->dirty = rgn;
            dirtyWidgets.append(widget);
            widgetPrivate->inDirtyList = true;
        }
    }

    inline void addDirtyRenderToTextureWidget(QWidget *widget)
    {
        if (widget && !widget->d_func()->inDirtyList && !widget->data->in_destructor) {
            QWidgetPrivate *widgetPrivate = widget->d_func();
            Q_ASSERT(widgetPrivate->renderToTexture);
            dirtyRenderToTextureWidgets.append(widget);
            widgetPrivate->inDirtyList = true;
        }
    }

    inline void dirtyWidgetsRemoveAll(QWidget *widget)
    {
        int i = 0;
        while (i < dirtyWidgets.size()) {
            if (dirtyWidgets.at(i) == widget)
                dirtyWidgets.remove(i);
            else
                ++i;
        }
    }

    inline void addStaticWidget(QWidget *widget)
    {
        if (!widget)
            return;

        Q_ASSERT(widget->testAttribute(Qt::WA_StaticContents));
        if (!staticWidgets.contains(widget))
            staticWidgets.append(widget);
    }

    inline void removeStaticWidget(QWidget *widget)
    { staticWidgets.removeAll(widget); }

    // Move the reparented widget and all its static children from this backing store
    // to the new backing store if reparented into another top-level / backing store.
    inline void moveStaticWidgets(QWidget *reparented)
    {
        Q_ASSERT(reparented);
        QWidgetBackingStore *newBs = reparented->d_func()->maybeBackingStore();
        if (newBs == this)
            return;

        int i = 0;
        while (i < staticWidgets.size()) {
            QWidget *w = staticWidgets.at(i);
            if (reparented == w || reparented->isAncestorOf(w)) {
                staticWidgets.removeAt(i);
                if (newBs)
                    newBs->addStaticWidget(w);
            } else {
                ++i;
            }
        }
    }

    inline QRect topLevelRect() const
    {
        return tlw->data->crect;
    }

    inline void appendDirtyOnScreenWidget(QWidget *widget)
    {
        if (!widget)
            return;

        if (!dirtyOnScreenWidgets) {
            dirtyOnScreenWidgets = new QVector<QWidget *>;
            dirtyOnScreenWidgets->append(widget);
        } else if (!dirtyOnScreenWidgets->contains(widget)) {
            dirtyOnScreenWidgets->append(widget);
        }
    }

    inline void dirtyOnScreenWidgetsRemoveAll(QWidget *widget)
    {
        if (!widget || !dirtyOnScreenWidgets)
            return;

        int i = 0;
        while (i < dirtyOnScreenWidgets->size()) {
            if (dirtyOnScreenWidgets->at(i) == widget)
                dirtyOnScreenWidgets->remove(i);
            else
                ++i;
        }
    }

    inline void resetWidget(QWidget *widget)
    {
        if (widget) {
            widget->d_func()->inDirtyList = false;
            widget->d_func()->isScrolled = false;
            widget->d_func()->isMoved = false;
            widget->d_func()->dirty = QRegion();
        }
    }

    inline void updateStaticContentsSize()
    {
        for (int i = 0; i < staticWidgets.size(); ++i) {
            QWidgetPrivate *wd = staticWidgets.at(i)->d_func();
            if (!wd->extra)
                wd->createExtra();
            wd->extra->staticContentsSize = wd->data.crect.size();
        }
    }

    inline bool hasStaticContents() const
    {
#if defined(Q_OS_WIN)
        return !staticWidgets.isEmpty();
#else
        return !staticWidgets.isEmpty() && false;
#endif
    }

    friend QRegion qt_dirtyRegion(QWidget *);
    friend class QWidgetPrivate;
    friend class QWidget;
    friend class QBackingStore;
};

QT_END_NAMESPACE

#endif // QBACKINGSTORE_P_H

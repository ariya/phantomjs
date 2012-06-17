/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qshortcutmap_p.h"
#include "private/qobject_p.h"
#include "qkeysequence.h"
#include "qgraphicsscene.h"
#include "qgraphicsview.h"
#include "qdebug.h"
#include "qevent.h"
#include "qwidget.h"
#include "qapplication.h"
#include "qvector.h"
#include "qmenu.h"
#include "qmenubar.h"
#include "qshortcut.h"
#include "qapplication_p.h"
#include <private/qaction_p.h>
#include <private/qkeymapper_p.h>
#include <private/qwidget_p.h>

#ifndef QT_NO_SHORTCUT

QT_BEGIN_NAMESPACE

// To enable verbose output uncomment below
//#define DEBUG_QSHORTCUTMAP

/* \internal
    Entry data for QShortcutMap
    Contains:
        Keysequence for entry
        Pointer to parent owning the sequence
*/
struct QShortcutEntry
{
    QShortcutEntry()
        : keyseq(0), context(Qt::WindowShortcut), enabled(false), autorepeat(1), id(0), owner(0)
    {}

    QShortcutEntry(const QKeySequence &k)
        : keyseq(k), context(Qt::WindowShortcut), enabled(false), autorepeat(1), id(0), owner(0)
    {}

    QShortcutEntry(QObject *o, const QKeySequence &k, Qt::ShortcutContext c, int i)
        : keyseq(k), context(c), enabled(true), autorepeat(1), id(i), owner(o)
    {}

    QShortcutEntry(QObject *o, const QKeySequence &k, Qt::ShortcutContext c, int i, bool a)
        : keyseq(k), context(c), enabled(true), autorepeat(a), id(i), owner(o)
    {}

    bool operator<(const QShortcutEntry &f) const
    { return keyseq < f.keyseq; }

    QKeySequence keyseq;
    Qt::ShortcutContext context;
    bool enabled : 1;
    bool autorepeat : 1;
    signed int id;
    QObject *owner;
};

#if 0 //ndef QT_NO_DEBUG_STREAM
/*! \internal
    QDebug operator<< for easy debug output of the shortcut entries.
*/
static QDebug &operator<<(QDebug &dbg, const QShortcutEntry *se) {
    if (!se)
        return dbg << "QShortcutEntry(0x0)";
    dbg.nospace()
        << "QShortcutEntry(" << se->keyseq
        << "), id(" << se->id << "), enabled(" << se->enabled << "), autorepeat(" << se->autorepeat
        << "), owner(" << se->owner << ')';
    return dbg.space();
}
#endif // QT_NO_DEBUGSTREAM

/* \internal
    Private data for QShortcutMap
*/
class QShortcutMapPrivate
{
    Q_DECLARE_PUBLIC(QShortcutMap)

public:
    QShortcutMapPrivate(QShortcutMap* parent)
        : q_ptr(parent), currentId(0), ambigCount(0), currentState(QKeySequence::NoMatch)
    {
        identicals.reserve(10);
        currentSequences.reserve(10);
    }
    QShortcutMap *q_ptr;                        // Private's parent

    QList<QShortcutEntry> sequences;            // All sequences!

    int currentId;                              // Global shortcut ID number
    int ambigCount;                             // Index of last enabled ambiguous dispatch
    QKeySequence::SequenceMatch currentState;
    QVector<QKeySequence> currentSequences;     // Sequence for the current state
    QVector<QKeySequence> newEntries;
    QKeySequence prevSequence;                  // Sequence for the previous identical match
    QVector<const QShortcutEntry*> identicals;  // Last identical matches
};


/*! \internal
    QShortcutMap constructor.
*/
QShortcutMap::QShortcutMap()
    : d_ptr(new QShortcutMapPrivate(this))
{
    resetState();
}

/*! \internal
    QShortcutMap destructor.
*/
QShortcutMap::~QShortcutMap()
{
}

/*! \internal
    Adds a shortcut to the global map.
    Returns the id of the newly added shortcut.
*/
int QShortcutMap::addShortcut(QObject *owner, const QKeySequence &key, Qt::ShortcutContext context)
{
    Q_ASSERT_X(owner, "QShortcutMap::addShortcut", "All shortcuts need an owner");
    Q_ASSERT_X(!key.isEmpty(), "QShortcutMap::addShortcut", "Cannot add keyless shortcuts to map");
    Q_D(QShortcutMap);

    QShortcutEntry newEntry(owner, key, context, --(d->currentId), true);
    QList<QShortcutEntry>::iterator it = qUpperBound(d->sequences.begin(), d->sequences.end(), newEntry);
    d->sequences.insert(it, newEntry); // Insert sorted
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace()
        << "QShortcutMap::addShortcut(" << owner << ", "
        << key << ", " << context << ") = " << d->currentId;
#endif
    return d->currentId;
}

/*! \internal
    Removes a shortcut from the global map.
    If \a owner is 0, all entries in the map with the key sequence specified
    is removed. If \a key is null, all sequences for \a owner is removed from
    the map. If \a id is 0, any identical \a key sequences owned by \a owner
    are removed.
    Returns the number of sequences removed from the map.
*/

int QShortcutMap::removeShortcut(int id, QObject *owner, const QKeySequence &key)
{
    Q_D(QShortcutMap);
    int itemsRemoved = 0;
    bool allOwners = (owner == 0);
    bool allKeys = key.isEmpty();
    bool allIds = id == 0;

    // Special case, remove everything
    if (allOwners && allKeys && id == 0) {
        itemsRemoved = d->sequences.size();
        d->sequences.clear();
        return itemsRemoved;
    }

    int i = d->sequences.size()-1;
    while (i>=0)
    {
        const QShortcutEntry &entry = d->sequences.at(i);
        int entryId = entry.id;
        if ((allOwners || entry.owner == owner)
            && (allIds || entry.id == id)
            && (allKeys || entry.keyseq == key)) {
            d->sequences.removeAt(i);
            ++itemsRemoved;
        }
        if (id == entryId)
            return itemsRemoved;
        --i;
    }
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace()
        << "QShortcutMap::removeShortcut(" << id << ", " << owner << ", "
        << key << ") = " << itemsRemoved;
#endif
    return itemsRemoved;
}

/*! \internal
    Changes the enable state of a shortcut to \a enable.
    If \a owner is 0, all entries in the map with the key sequence specified
    is removed. If \a key is null, all sequences for \a owner is removed from
    the map. If \a id is 0, any identical \a key sequences owned by \a owner
    are changed.
    Returns the number of sequences which are matched in the map.
*/
int QShortcutMap::setShortcutEnabled(bool enable, int id, QObject *owner, const QKeySequence &key)
{
    Q_D(QShortcutMap);
    int itemsChanged = 0;
    bool allOwners = (owner == 0);
    bool allKeys = key.isEmpty();
    bool allIds = id == 0;

    int i = d->sequences.size()-1;
    while (i>=0)
    {
        QShortcutEntry entry = d->sequences.at(i);
        if ((allOwners || entry.owner == owner)
            && (allIds || entry.id == id)
            && (allKeys || entry.keyseq == key)) {
            d->sequences[i].enabled = enable;
            ++itemsChanged;
        }
        if (id == entry.id)
            return itemsChanged;
        --i;
    }
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace()
        << "QShortcutMap::setShortcutEnabled(" << enable << ", " << id << ", "
        << owner << ", " << key << ") = " << itemsChanged;
#endif
    return itemsChanged;
}

/*! \internal
    Changes the auto repeat state of a shortcut to \a enable.
    If \a owner is 0, all entries in the map with the key sequence specified
    is removed. If \a key is null, all sequences for \a owner is removed from
    the map. If \a id is 0, any identical \a key sequences owned by \a owner
    are changed.
    Returns the number of sequences which are matched in the map.
*/
int QShortcutMap::setShortcutAutoRepeat(bool on, int id, QObject *owner, const QKeySequence &key)
{
    Q_D(QShortcutMap);
    int itemsChanged = 0;
    bool allOwners = (owner == 0);
    bool allKeys = key.isEmpty();
    bool allIds = id == 0;

    int i = d->sequences.size()-1;
    while (i>=0)
    {
        QShortcutEntry entry = d->sequences.at(i);
        if ((allOwners || entry.owner == owner)
            && (allIds || entry.id == id)
            && (allKeys || entry.keyseq == key)) {
                d->sequences[i].autorepeat = on;
                ++itemsChanged;
        }
        if (id == entry.id)
            return itemsChanged;
        --i;
    }
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace()
        << "QShortcutMap::setShortcutAutoRepeat(" << on << ", " << id << ", "
        << owner << ", " << key << ") = " << itemsChanged;
#endif
    return itemsChanged;
}

/*! \internal
    Resets the state of the statemachine to NoMatch
*/
void QShortcutMap::resetState()
{
    Q_D(QShortcutMap);
    d->currentState = QKeySequence::NoMatch;
    clearSequence(d->currentSequences);
}

/*! \internal
    Returns the current state of the statemachine
*/
QKeySequence::SequenceMatch QShortcutMap::state()
{
    Q_D(QShortcutMap);
    return d->currentState;
}

/*! \internal
    Uses ShortcutOverride event to see if any widgets want to override
    the event. If not, uses nextState(QKeyEvent) to check for a grabbed
    Shortcut, and dispatchEvent() is found an identical.
    \sa nextState dispatchEvent
*/
bool QShortcutMap::tryShortcutEvent(QObject *o, QKeyEvent *e)
{
    Q_D(QShortcutMap);

    bool wasAccepted = e->isAccepted();
    bool wasSpontaneous = e->spont;
    if (d->currentState == QKeySequence::NoMatch) {
        ushort orgType = e->t;
        e->t = QEvent::ShortcutOverride;
        e->ignore();
        QApplication::sendEvent(o, e);
        e->t = orgType;
        e->spont = wasSpontaneous;
        if (e->isAccepted()) {
            if (!wasAccepted)
                e->ignore();
            return false;
        }
    }

    QKeySequence::SequenceMatch result = nextState(e);
    bool stateWasAccepted = e->isAccepted();
    if (wasAccepted)
        e->accept();
    else
        e->ignore();

    int identicalMatches = d->identicals.count();

    switch(result) {
    case QKeySequence::NoMatch:
        return stateWasAccepted;
    case QKeySequence::ExactMatch:
        resetState();
        dispatchEvent(e);
    default:
	break;
    }
    // If nextState is QKeySequence::ExactMatch && identicals.count == 0
    // we've only found disabled shortcuts
    return identicalMatches > 0 || result == QKeySequence::PartialMatch;
}

/*! \internal
    Returns the next state of the statemachine
    If return value is SequenceMatch::ExactMatch, then a call to matches()
    will return a QObjects* list of all matching objects for the last matching
    sequence.
*/
QKeySequence::SequenceMatch QShortcutMap::nextState(QKeyEvent *e)
{
    Q_D(QShortcutMap);
    // Modifiers can NOT be shortcuts...
    if (e->key() >= Qt::Key_Shift &&
        e->key() <= Qt::Key_Alt)
        return d->currentState;

    QKeySequence::SequenceMatch result = QKeySequence::NoMatch;

    // We start fresh each time..
    d->identicals.resize(0);

    result = find(e);
    if (result == QKeySequence::NoMatch && e->modifiers() & Qt::ShiftModifier) {
        // If Shift + Key_Backtab, also try Shift + Qt::Key_Tab
        if (e->key() == Qt::Key_Backtab) {
            QKeyEvent pe = QKeyEvent(e->type(), Qt::Key_Tab, e->modifiers(), e->text());
            result = find(&pe);
        }
    }

    // Should we eat this key press?
    if (d->currentState == QKeySequence::PartialMatch
        || (d->currentState == QKeySequence::ExactMatch && d->identicals.count()))
        e->accept();
    // Does the new state require us to clean up?
    if (result == QKeySequence::NoMatch)
        clearSequence(d->currentSequences);
    d->currentState = result;

#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace() << "QShortcutMap::nextState(" << e << ") = " << result;
#endif
    return result;
}


/*! \internal
    Determines if an enabled shortcut has a matcing key sequence.
*/
bool QShortcutMap::hasShortcutForKeySequence(const QKeySequence &seq) const
{
    Q_D(const QShortcutMap);
    QShortcutEntry entry(seq); // needed for searching
    QList<QShortcutEntry>::ConstIterator itEnd = d->sequences.constEnd();
    QList<QShortcutEntry>::ConstIterator it = qLowerBound(d->sequences.constBegin(), itEnd, entry);

    for (;it != itEnd; ++it) {
        if (matches(entry.keyseq, (*it).keyseq) == QKeySequence::ExactMatch && correctContext(*it) && (*it).enabled) {
            return true;
        }
    }

    //end of the loop: we didn't find anything
    return false;
}

/*! \internal
    Returns the next state of the statemachine, based
    on the new key event \a e.
    Matches are appended to the vector of identicals,
    which can be access through matches().
    \sa matches
*/
QKeySequence::SequenceMatch QShortcutMap::find(QKeyEvent *e)
{
    Q_D(QShortcutMap);
    if (!d->sequences.count())
        return QKeySequence::NoMatch;

    createNewSequences(e, d->newEntries);
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug() << "Possible shortcut key sequences:" << d->newEntries;
#endif

    // Should never happen
    if (d->newEntries == d->currentSequences) {
        Q_ASSERT_X(e->key() != Qt::Key_unknown || e->text().length(),
                   "QShortcutMap::find", "New sequence to find identical to previous");
        return QKeySequence::NoMatch;
    }

    // Looking for new identicals, scrap old
    d->identicals.resize(0);

    bool partialFound = false;
    bool identicalDisabledFound = false;
    QVector<QKeySequence> okEntries;
    int result = QKeySequence::NoMatch;
    for (int i = d->newEntries.count()-1; i >= 0 ; --i) {
        QShortcutEntry entry(d->newEntries.at(i)); // needed for searching
        QList<QShortcutEntry>::ConstIterator itEnd = d->sequences.constEnd();
        QList<QShortcutEntry>::ConstIterator it =
             qLowerBound(d->sequences.constBegin(), itEnd, entry);

        int oneKSResult = QKeySequence::NoMatch;
        int tempRes = QKeySequence::NoMatch;
        do {
            if (it == itEnd)
                break;
            tempRes = matches(entry.keyseq, (*it).keyseq);
            oneKSResult = qMax(oneKSResult, tempRes);
            if (tempRes != QKeySequence::NoMatch && correctContext(*it)) {
                if (tempRes == QKeySequence::ExactMatch) {
                    if ((*it).enabled)
                        d->identicals.append(&*it);
                    else
                        identicalDisabledFound = true;
                } else if (tempRes == QKeySequence::PartialMatch) {
                    // We don't need partials, if we have identicals
                    if (d->identicals.size())
                        break;
                    // We only care about enabled partials, so we don't consume
                    // key events when all partials are disabled!
                    partialFound |= (*it).enabled;
                }
            }
            ++it;
            // If we got a valid match on this run, there might still be more keys to check against,
            // so we'll loop once more. If we get NoMatch, there's guaranteed no more possible
            // matches in the shortcutmap.
        } while (tempRes != QKeySequence::NoMatch);

        // If the type of match improves (ergo, NoMatch->Partial, or Partial->Exact), clear the
        // previous list. If this match is equal or better than the last match, append to the list
        if (oneKSResult > result) {
            okEntries.clear();
#if defined(DEBUG_QSHORTCUTMAP)
            qDebug() << "Found better match (" << d->newEntries << "), clearing key sequence list";
#endif
        }
        if (oneKSResult && oneKSResult >= result) {
            okEntries << d->newEntries.at(i);
#if defined(DEBUG_QSHORTCUTMAP)
            qDebug() << "Added ok key sequence" << d->newEntries;
#endif
        }
    }

    if (d->identicals.size()) {
        result = QKeySequence::ExactMatch;
    } else if (partialFound) {
        result = QKeySequence::PartialMatch;
    } else if (identicalDisabledFound) {
        result = QKeySequence::ExactMatch;
    } else {
        clearSequence(d->currentSequences);
        result = QKeySequence::NoMatch;
    }
    if (result != QKeySequence::NoMatch)
        d->currentSequences = okEntries;
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug() << "Returning shortcut match == " << result;
#endif
    return QKeySequence::SequenceMatch(result);
}

/*! \internal
    Clears \a seq to an empty QKeySequence.
    Same as doing (the slower)
    \snippet doc/src/snippets/code/src_gui_kernel_qshortcutmap.cpp 0
*/
void QShortcutMap::clearSequence(QVector<QKeySequence> &ksl)
{
    ksl.clear();
    d_func()->newEntries.clear();
}

/*! \internal
    Alters \a seq to the new sequence state, based on the
    current sequence state, and the new key event \a e.
*/
void QShortcutMap::createNewSequences(QKeyEvent *e, QVector<QKeySequence> &ksl)
{
    Q_D(QShortcutMap);
    QList<int> possibleKeys = QKeyMapper::possibleKeys(e);
    int pkTotal = possibleKeys.count();
    if (!pkTotal)
        return;

    int ssActual = d->currentSequences.count();
    int ssTotal = qMax(1, ssActual);
    // Resize to possible permutations of the current sequence(s).
    ksl.resize(pkTotal * ssTotal);

    int index = ssActual ? d->currentSequences.at(0).count() : 0;
    for (int pkNum = 0; pkNum < pkTotal; ++pkNum) {
        for (int ssNum = 0; ssNum < ssTotal; ++ssNum) {
            int i = (pkNum * ssTotal) + ssNum;
            QKeySequence &curKsl = ksl[i];
            if (ssActual) {
                const QKeySequence &curSeq = d->currentSequences.at(ssNum);
                curKsl.setKey(curSeq[0], 0);
                curKsl.setKey(curSeq[1], 1);
                curKsl.setKey(curSeq[2], 2);
                curKsl.setKey(curSeq[3], 3);
            } else {
                curKsl.setKey(0, 0);
                curKsl.setKey(0, 1);
                curKsl.setKey(0, 2);
                curKsl.setKey(0, 3);
            }
            // Filtering keycode here with 0xdfffffff to ignore the Keypad modifier
            curKsl.setKey(possibleKeys.at(pkNum) & 0xdfffffff, index);
        }
    }
}

/*! \internal
    Basically the same function as QKeySequence::matches(const QKeySequence &seq) const
    only that is specially handles Key_hyphen as Key_Minus, as people mix these up all the time and
    they conceptually the same.
*/
QKeySequence::SequenceMatch QShortcutMap::matches(const QKeySequence &seq1,
                                                  const QKeySequence &seq2) const
{
    uint userN = seq1.count(),
        seqN = seq2.count();

    if (userN > seqN)
        return QKeySequence::NoMatch;

    // If equal in length, we have a potential ExactMatch sequence,
    // else we already know it can only be partial.
    QKeySequence::SequenceMatch match = (userN == seqN
                                            ? QKeySequence::ExactMatch
                                            : QKeySequence::PartialMatch);

    for (uint i = 0; i < userN; ++i) {
        int userKey = seq1[i],
            sequenceKey = seq2[i];
        if ((userKey & Qt::Key_unknown) == Qt::Key_hyphen)
            userKey = (userKey & Qt::KeyboardModifierMask) | Qt::Key_Minus;
        if ((sequenceKey & Qt::Key_unknown) == Qt::Key_hyphen)
            sequenceKey = (sequenceKey & Qt::KeyboardModifierMask) | Qt::Key_Minus;
        if (userKey != sequenceKey)
            return QKeySequence::NoMatch;
    }
    return match;
}

/*! \internal
    Returns true if the widget \a w is a logical sub window of the current
    top-level widget.
*/
bool QShortcutMap::correctContext(const QShortcutEntry &item) const {
    Q_ASSERT_X(item.owner, "QShortcutMap", "Shortcut has no owner. Illegal map state!");

    QWidget *active_window = QApplication::activeWindow();

    // popups do not become the active window,
    // so we fake it here to get the correct context
    // for the shortcut system.
    if (QApplication::activePopupWidget())
        active_window = QApplication::activePopupWidget();

    if (!active_window)
        return false;
#ifndef QT_NO_ACTION
    if (QAction *a = qobject_cast<QAction *>(item.owner))
        return correctContext(item.context, a, active_window);
#endif
#ifndef QT_NO_GRAPHICSVIEW
    if (QGraphicsWidget *gw = qobject_cast<QGraphicsWidget *>(item.owner))
        return correctGraphicsWidgetContext(item.context, gw, active_window);
#endif
    QWidget *w = qobject_cast<QWidget *>(item.owner);
    if (!w) {
        QShortcut *s = qobject_cast<QShortcut *>(item.owner);
        w = s->parentWidget();
    }
    return correctWidgetContext(item.context, w, active_window);
}

bool QShortcutMap::correctWidgetContext(Qt::ShortcutContext context, QWidget *w, QWidget *active_window) const
{
    bool visible = w->isVisible();    
#ifdef Q_WS_MAC
    if (!qApp->testAttribute(Qt::AA_DontUseNativeMenuBar) && qobject_cast<QMenuBar *>(w))
        visible = true;
#endif

    if (!visible || !w->isEnabled())
        return false;

    if (context == Qt::ApplicationShortcut)
        return QApplicationPrivate::tryModalHelper(w, 0); // true, unless w is shadowed by a modal dialog

    if (context == Qt::WidgetShortcut)
        return w == QApplication::focusWidget();

    if (context == Qt::WidgetWithChildrenShortcut) {
        const QWidget *tw = QApplication::focusWidget();
        while (tw && tw != w && (tw->windowType() == Qt::Widget || tw->windowType() == Qt::Popup))
            tw = tw->parentWidget();
        return tw == w;
    }

    // Below is Qt::WindowShortcut context
    QWidget *tlw = w->window();
#ifndef QT_NO_GRAPHICSVIEW
    if (QWExtra *topData = tlw->d_func()->extra) {
        if (topData->proxyWidget) {
            bool res = correctGraphicsWidgetContext(context, (QGraphicsWidget *)topData->proxyWidget, active_window);
            return res;
        }
    }
#endif

    /* if a floating tool window is active, keep shortcuts on the
     * parent working */
    if (active_window != tlw && active_window && active_window->windowType() == Qt::Tool && active_window->parentWidget()) {
        active_window = active_window->parentWidget()->window();
    }

    if (active_window  != tlw)
        return false;

    /* if we live in a MDI subwindow, ignore the event if we are
       not the active document window */
    const QWidget* sw = w;
    while (sw && !(sw->windowType() == Qt::SubWindow) && !sw->isWindow())
        sw = sw->parentWidget();
    if (sw && (sw->windowType() == Qt::SubWindow)) {
        QWidget *focus_widget = QApplication::focusWidget();
        while (focus_widget && focus_widget != sw)
            focus_widget = focus_widget->parentWidget();
        return sw == focus_widget;
    }

#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace() << "..true [Pass-through]";
#endif
    return true;
}

#ifndef QT_NO_GRAPHICSVIEW
bool QShortcutMap::correctGraphicsWidgetContext(Qt::ShortcutContext context, QGraphicsWidget *w, QWidget *active_window) const
{
    bool visible = w->isVisible();
#ifdef Q_WS_MAC
    if (!qApp->testAttribute(Qt::AA_DontUseNativeMenuBar) && qobject_cast<QMenuBar *>(w))
        visible = true;
#endif

    if (!visible || !w->isEnabled() || !w->scene())
        return false;

    if (context == Qt::ApplicationShortcut) {
        // Applicationwide shortcuts are always reachable unless their owner
        // is shadowed by modality. In QGV there's no modality concept, but we
        // must still check if all views are shadowed.
        QList<QGraphicsView *> views = w->scene()->views();
        for (int i = 0; i < views.size(); ++i) {
            if (QApplicationPrivate::tryModalHelper(views.at(i), 0))
                return true;
        }
        return false;
    }

    if (context == Qt::WidgetShortcut)
        return static_cast<QGraphicsItem *>(w) == w->scene()->focusItem();

    if (context == Qt::WidgetWithChildrenShortcut) {
        const QGraphicsItem *ti = w->scene()->focusItem();
        if (ti && ti->isWidget()) {
            const QGraphicsWidget *tw = static_cast<const QGraphicsWidget *>(ti);
            while (tw && tw != w && (tw->windowType() == Qt::Widget || tw->windowType() == Qt::Popup))
                tw = tw->parentWidget();
            return tw == w;
        }
        return false;
    }

    // Below is Qt::WindowShortcut context

    // Find the active view (if any).
    QList<QGraphicsView *> views = w->scene()->views();
    QGraphicsView *activeView = 0;
    for (int i = 0; i < views.size(); ++i) {
        QGraphicsView *view = views.at(i);
        if (view->window() == active_window) {
            activeView = view;
            break;
        }
    }
    if (!activeView)
        return false;

    // The shortcut is reachable if owned by a windowless widget, or if the
    // widget's window is the same as the focus item's window.
    QGraphicsWidget *a = w->scene()->activeWindow();
    return !w->window() || a == w->window();
}
#endif

#ifndef QT_NO_ACTION
bool QShortcutMap::correctContext(Qt::ShortcutContext context, QAction *a, QWidget *active_window) const
{
    const QList<QWidget *> &widgets = a->d_func()->widgets;
#if defined(DEBUG_QSHORTCUTMAP)
    if (widgets.isEmpty())
        qDebug() << a << "not connected to any widgets; won't trigger";
#endif
    for (int i = 0; i < widgets.size(); ++i) {
        QWidget *w = widgets.at(i);
#ifndef QT_NO_MENU
        if (QMenu *menu = qobject_cast<QMenu *>(w)) {
            QAction *a = menu->menuAction();
            if (correctContext(context, a, active_window))
                return true;
        } else
#endif
            if (correctWidgetContext(context, w, active_window))
                return true;
    }

#ifndef QT_NO_GRAPHICSVIEW
    const QList<QGraphicsWidget *> &graphicsWidgets = a->d_func()->graphicsWidgets;
#if defined(DEBUG_QSHORTCUTMAP)
    if (graphicsWidgets.isEmpty())
        qDebug() << a << "not connected to any widgets; won't trigger";
#endif
    for (int i = 0; i < graphicsWidgets.size(); ++i) {
        QGraphicsWidget *w = graphicsWidgets.at(i);
        if (correctGraphicsWidgetContext(context, w, active_window))
            return true;
    }
#endif
    return false;
}
#endif // QT_NO_ACTION

/*! \internal
    Converts keyboard button states into modifier states
*/
int QShortcutMap::translateModifiers(Qt::KeyboardModifiers modifiers)
{
    int result = 0;
    if (modifiers & Qt::ShiftModifier)
        result |= Qt::SHIFT;
    if (modifiers & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (modifiers & Qt::MetaModifier)
        result |= Qt::META;
    if (modifiers & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

/*! \internal
    Returns the vector of QShortcutEntry's matching the last Identical state.
*/
QVector<const QShortcutEntry*> QShortcutMap::matches() const
{
    Q_D(const QShortcutMap);
    return d->identicals;
}

/*! \internal
    Dispatches QShortcutEvents to widgets who grabbed the matched key sequence.
*/
void QShortcutMap::dispatchEvent(QKeyEvent *e)
{
    Q_D(QShortcutMap);
    if (!d->identicals.size())
        return;

    const QKeySequence &curKey = d->identicals.at(0)->keyseq;
    if (d->prevSequence != curKey) {
        d->ambigCount = 0;
        d->prevSequence = curKey;
    }
    // Find next
    const QShortcutEntry *current = 0, *next = 0;
    int i = 0, enabledShortcuts = 0;
    while(i < d->identicals.size()) {
        current = d->identicals.at(i);
        if (current->enabled || !next){
            ++enabledShortcuts;
            if (enabledShortcuts > d->ambigCount + 1)
                break;
            next = current;
        }
        ++i;
    }
    d->ambigCount = (d->identicals.size() == i ? 0 : d->ambigCount + 1);
    // Don't trigger shortcut if we're autorepeating and the shortcut is
    // grabbed with not accepting autorepeats.
    if (!next || (e->isAutoRepeat() && !next->autorepeat))
        return;
    // Dispatch next enabled
#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace()
        << "QShortcutMap::dispatchEvent(): Sending QShortcutEvent(\""
        << (QString)next->keyseq << "\", " << next->id << ", "
        << (bool)(enabledShortcuts>1) << ") to object(" << next->owner << ')';
#endif
    QShortcutEvent se(next->keyseq, next->id, enabledShortcuts>1);
    QApplication::sendEvent(const_cast<QObject *>(next->owner), &se);
}

/* \internal
    QShortcutMap dump function, only available when DEBUG_QSHORTCUTMAP is
    defined.
*/
#if defined(Dump_QShortcutMap)
void QShortcutMap::dumpMap() const
{
    Q_D(const QShortcutMap);
    for (int i = 0; i < d->sequences.size(); ++i)
        qDebug().nospace() << &(d->sequences.at(i));
}
#endif

QT_END_NAMESPACE

#endif // QT_NO_SHORTCUT

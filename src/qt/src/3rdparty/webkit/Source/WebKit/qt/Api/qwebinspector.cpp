/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qwebinspector.h"

#include "Element.h"
#include "InspectorController.h"
#include "qwebelement.h"
#include "qwebinspector_p.h"
#include "qwebpage_p.h"

#include <QResizeEvent>

/*!
    \class QWebInspector
    \since 4.6
    \inmodule QtWebKit
    \brief The QWebInspector class allows the placement and control of a
    QWebPage's inspector.
    The inspector can display a page's hierarchy, its loading statistics and
    the current state of its individual elements. It is mostly used by web
    developers.

    The QWebPage to be inspected must be specified using the setPage() method.

    A typical use of QWebInspector follows:

    \snippet webkitsnippets/qtwebkit_qwebinspector_snippet.cpp 0

    A QWebInspector can be made visible either programmatically using
    setVisible(), or by the user through the attached QWebPage's context
    menu.

    \note A QWebInspector will display a blank widget if either:
    \list
        \o page() is null
        \o QWebSettings::DeveloperExtrasEnabled is false
    \endlist

    \section1 Resources

    This class acts mostly as a container and a controller for the inspector.
    Most of the resources needed by the inspector are owned by the associated
    QWebPage and are allocated the first time that:
    \list
        \o an element is inspected
        \o the QWebInspector is shown.
    \endlist

    \section1 Inspector configuration persistence

    The inspector allows the user to configure some options through its
    user interface (e.g. the resource tracking "Always enable" option).
    These settings will be persisted automatically by QtWebKit only if
    your application previously called QCoreApplication::setOrganizationName()
    and QCoreApplication::setApplicationName().
    See QSettings's default constructor documentation for an explanation
    of why this is necessary.
*/

/*!
    Constructs an unbound QWebInspector with \a parent as its parent.
*/
QWebInspector::QWebInspector(QWidget* parent)
    : QWidget(parent)
    , d(new QWebInspectorPrivate(this))
{
}

/*!
    Destroys the inspector.
*/
QWebInspector::~QWebInspector()
{
    // Remove association principally to prevent deleting a child frontend
    setPage(0);
    delete d;
    d = 0;
}

/*!
    Bind this inspector to the QWebPage to be inspected.

    \bold {Notes:}
    \list
        \o There can only be one QWebInspector associated with a QWebPage
           and vice versa.
        \o Calling this method with a null \a page will break the current association, if any.
        \o If \a page is already associated to another QWebInspector, the association
           will be replaced and the previous QWebInspector will become unbound
    \endlist

    \sa page()
*/
void QWebInspector::setPage(QWebPage* page)
{
    if (d->page) {
        // Break currentPage-->this
        d->page->d->setInspector(0);
    }
    if (page && page->d->inspector && page->d->inspector != this) {
        // Break newPage<->newPageCurrentInspector
        page->d->inspector->setPage(0);
    }

    d->page = page;

    if (page) {
        // Setup the reciprocal association
        page->d->setInspector(this);
    }
}

/*!
    Returns the inspected QWebPage.
    If no web page is currently associated, a null pointer is returned.
*/
QWebPage* QWebInspector::page() const
{
    return d->page;
}

/*! \reimp */
QSize QWebInspector::sizeHint() const
{
    return QSize(450, 300);
}

/*! \reimp */
bool QWebInspector::event(QEvent* ev)
{
    return QWidget::event(ev);
}

/*! \reimp */
void QWebInspector::resizeEvent(QResizeEvent* event)
{
    d->adjustFrontendSize(event->size());
}

/*! \reimp */
void QWebInspector::showEvent(QShowEvent* event)
{
#if ENABLE(INSPECTOR)
    // Allows QWebInspector::show() to init the inspector.
    if (d->page)
        d->page->d->inspectorController()->showAndEnableDebugger();
#endif
}

/*! \reimp */
void QWebInspector::hideEvent(QHideEvent* event)
{
#if ENABLE(INSPECTOR)
    if (d->page)
        d->page->d->inspectorController()->close();
#endif
}

/*! \reimp */
void QWebInspector::closeEvent(QCloseEvent* event)
{
#if ENABLE(INSPECTOR)
    if (d->page)
        d->page->d->inspectorController()->close();
#endif
}

/*! \internal */
void QWebInspectorPrivate::setFrontend(QWidget* newFrontend)
{
    if (frontend)
        frontend->setParent(0);

    frontend = newFrontend;

    if (frontend) {
        frontend->setParent(q);
        frontend->show();
        adjustFrontendSize(q->size());
    }
}

/*! 
 * \internal 
 */
void QWebInspectorPrivate::attachAndReplaceRemoteFrontend(QObject* newRemoteFrontend)
{
    if (remoteFrontend)
        remoteFrontend->setParent(0);

    remoteFrontend = newRemoteFrontend;

    if (remoteFrontend)
        remoteFrontend->setParent(q);
}

/*! 
 * \internal 
 */
void QWebInspectorPrivate::detachRemoteFrontend()
{
    if (remoteFrontend) {
        remoteFrontend->deleteLater();
        remoteFrontend = 0;
    }
}

void QWebInspectorPrivate::adjustFrontendSize(const QSize& size)
{
    if (frontend)
        frontend->resize(size);
}


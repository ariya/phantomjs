/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiconengine.h"
#include "qpainter.h"

QT_BEGIN_NAMESPACE

/*!
  \class QIconEngine

  \brief The QIconEngine class provides an abstract base class for QIcon renderers.

  \ingroup painting
  \inmodule QtGui

  An icon engine provides the rendering functions for a QIcon. Each icon has a
  corresponding icon engine that is responsible for drawing the icon with a
  requested size, mode and state.

  The icon is rendered by the paint() function, and the icon can additionally be
  obtained as a pixmap with the pixmap() function (the default implementation
  simply uses paint() to achieve this). The addPixmap() function can be used to
  add new pixmaps to the icon engine, and is used by QIcon to add specialized
  custom pixmaps.

  The paint(), pixmap(), and addPixmap() functions are all virtual, and can
  therefore be reimplemented in subclasses of QIconEngine.

  \sa QIconEnginePlugin

*/

/*!
  \fn virtual void QIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) = 0;

  Uses the given \a painter to paint the icon with the required \a mode and
  \a state into the rectangle \a rect.
*/

/*!  Returns the actual size of the icon the engine provides for the
  requested \a size, \a mode and \a state. The default implementation
  returns the given \a size.
 */
QSize QIconEngine::actualSize(const QSize &size, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
    return size;
}


/*!
  Destroys the icon engine.
 */
QIconEngine::~QIconEngine()
{
}


/*!
  Returns the icon as a pixmap with the required \a size, \a mode,
  and \a state. The default implementation creates a new pixmap and
  calls paint() to fill it.
*/
QPixmap QIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm(size);
    {
        QPainter p(&pm);
        paint(&p, QRect(QPoint(0,0),size), mode, state);
    }
    return pm;
}

/*!
  Called by QIcon::addPixmap(). Adds a specialized \a pixmap for the given
  \a mode and \a state. The default pixmap-based engine stores any supplied
  pixmaps, and it uses them instead of scaled pixmaps if the size of a pixmap
  matches the size of icon requested. Custom icon engines that implement
  scalable vector formats are free to ignores any extra pixmaps.
 */
void QIconEngine::addPixmap(const QPixmap &/*pixmap*/, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
}


/*!  Called by QIcon::addFile(). Adds a specialized pixmap from the
  file with the given \a fileName, \a size, \a mode and \a state. The
  default pixmap-based engine stores any supplied file names, and it
  loads the pixmaps on demand instead of using scaled pixmaps if the
  size of a pixmap matches the size of icon requested. Custom icon
  engines that implement scalable vector formats are free to ignores
  any extra files.
 */
void QIconEngine::addFile(const QString &/*fileName*/, const QSize &/*size*/, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
}


/*!
    \enum QIconEngine::IconEngineHook
    \since 4.5

    These enum values are used for virtual_hook() to allow additional
    queries to icon engine without breaking binary compatibility.

    \value AvailableSizesHook Allows to query the sizes of the
    contained pixmaps for pixmap-based engines. The \a data argument
    of the virtual_hook() function is a AvailableSizesArgument pointer
    that should be filled with icon sizes. Engines that work in terms
    of a scalable, vectorial format normally return an empty list.

    \value IconNameHook Allows to query the name used to create the
    icon, for example when instantiating an icon using
    QIcon::fromTheme().

    \sa virtual_hook()
 */

/*!
    \class QIconEngine::AvailableSizesArgument
    \since 4.5

    \inmodule QtGui

    This struct represents arguments to virtual_hook() function when
    \a id parameter is QIconEngine::AvailableSizesHook.

    \sa virtual_hook(), QIconEngine::IconEngineHook
 */

/*!
    \variable QIconEngine::AvailableSizesArgument::mode
    \brief the requested mode of an image.

    \sa QIcon::Mode
*/

/*!
    \variable QIconEngine::AvailableSizesArgument::state
    \brief the requested state of an image.

    \sa QIcon::State
*/

/*!
    \variable QIconEngine::AvailableSizesArgument::sizes

    \brief image sizes that are available with specified \a mode and
    \a state. This is an output parameter and is filled after call to
    virtual_hook(). Engines that work in terms of a scalable,
    vectorial format normally return an empty list.
*/


/*!
    Returns a key that identifies this icon engine.
 */
QString QIconEngine::key() const
{
    return QString();
}

/*! \fn QIconEngine *QIconEngine::clone() const

    Reimplement this method to return a clone of this icon engine.
 */

/*!
    Reads icon engine contents from the QDataStream \a in. Returns
    true if the contents were read; otherwise returns \c false.

    QIconEngine's default implementation always return false.
 */
bool QIconEngine::read(QDataStream &)
{
    return false;
}

/*!
    Writes the contents of this engine to the QDataStream \a out.
    Returns \c true if the contents were written; otherwise returns \c false.

    QIconEngine's default implementation always return false.
 */
bool QIconEngine::write(QDataStream &) const
{
    return false;
}

/*!
    \since 4.5

    Additional method to allow extending QIconEngine without
    adding new virtual methods (and without breaking binary compatibility).
    The actual action and format of \a data depends on \a id argument
    which is in fact a constant from IconEngineHook enum.

    \sa IconEngineHook
*/
void QIconEngine::virtual_hook(int id, void *data)
{
    switch (id) {
    case QIconEngine::AvailableSizesHook: {
        QIconEngine::AvailableSizesArgument &arg =
            *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
        arg.sizes.clear();
        break;
    }
    default:
        break;
    }
}

/*!
    \since 4.5

    Returns sizes of all images that are contained in the engine for the
    specific \a mode and \a state.

    \note This is a helper method and the actual work is done by
    virtual_hook() method, hence this method depends on icon engine support
    and may not work with all icon engines.
 */
QList<QSize> QIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) const
{
    AvailableSizesArgument arg;
    arg.mode = mode;
    arg.state = state;
    const_cast<QIconEngine *>(this)->virtual_hook(QIconEngine::AvailableSizesHook, reinterpret_cast<void*>(&arg));
    return arg.sizes;
}

/*!
    \since 4.7

    Returns the name used to create the engine, if available.

    \note This is a helper method and the actual work is done by
    virtual_hook() method, hence this method depends on icon engine support
    and may not work with all icon engines.
 */
QString QIconEngine::iconName() const
{
    QString name;
    const_cast<QIconEngine *>(this)->virtual_hook(QIconEngine::IconNameHook, reinterpret_cast<void*>(&name));
    return name;
}

QT_END_NAMESPACE

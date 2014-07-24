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

#include "qicon.h"
#include "qicon_p.h"
#include "qiconengine.h"
#include "qiconengineplugin.h"
#include "qimagereader.h"
#include "private/qfactoryloader_p.h"
#include "private/qiconloader_p.h"
#include "qpainter.h"
#include "qfileinfo.h"
#include "qpixmapcache.h"
#include "qvariant.h"
#include "qcache.h"
#include "qdebug.h"
#include "qpalette.h"

#ifdef Q_WS_MAC
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#endif

#include "private/qhexstring_p.h"
#include "private/qguiapplication_p.h"
#include "qpa/qplatformtheme.h"

#ifndef QT_NO_ICON
QT_BEGIN_NAMESPACE

/*!
    \enum QIcon::Mode

    This enum type describes the mode for which a pixmap is intended
    to be used. The currently defined modes are:

    \value Normal
         Display the pixmap when the user is
        not interacting with the icon, but the
        functionality represented by the icon is available.
    \value Disabled
         Display the pixmap when the
        functionality represented by the icon is not available.
    \value Active
         Display the pixmap when the
        functionality represented by the icon is available and
        the user is interacting with the icon, for example, moving the
        mouse over it or clicking it.
   \value Selected
        Display the pixmap when the item represented by the icon is
        selected.
*/

/*!
  \enum QIcon::State

  This enum describes the state for which a pixmap is intended to be
  used. The \e state can be:

  \value Off  Display the pixmap when the widget is in an "off" state
  \value On  Display the pixmap when the widget is in an "on" state
*/

static QBasicAtomicInt serialNumCounter = Q_BASIC_ATOMIC_INITIALIZER(1);

static void qt_cleanup_icon_cache();
namespace {
    struct IconCache : public QCache<QString, QIcon>
    {
        IconCache()
        {
            // ### note: won't readd if QApplication is re-created!
            qAddPostRoutine(qt_cleanup_icon_cache);
        }
    };
}

Q_GLOBAL_STATIC(IconCache, qtIconCache)

static void qt_cleanup_icon_cache()
{
    qtIconCache()->clear();
}

/*! \internal

    Returns the effective device pixel ratio, using
    the provided window pointer if possible.

    if Qt::AA_UseHighDpiPixmaps is not set this function
    returns 1.0 to keep non-hihdpi aware code working.
*/
static qreal qt_effective_device_pixel_ratio(QWindow *window = 0)
{
    if (!qApp->testAttribute(Qt::AA_UseHighDpiPixmaps))
        return qreal(1.0);

    if (window)
        return window->devicePixelRatio();

    return qApp->devicePixelRatio(); // Don't know which window to target.
}

QIconPrivate::QIconPrivate()
    : engine(0), ref(1),
    serialNum(serialNumCounter.fetchAndAddRelaxed(1)),
    detach_no(0)
{
}

/*! \internal
    Computes the displayDevicePixelRatio for a pixmap.

    If displayDevicePixelRatio is 1.0 the reurned value is 1.0, always.

    For a displayDevicePixelRatio of 2.0 the returned value will be between
    1.0 and 2.0, depending on requestedSize and actualsize:
    * If actualsize < requestedSize        : 1.0 (not enough pixels for a normal-dpi pixmap)
    * If actualsize == requestedSize * 2.0 : 2.0 (enough pixels for a high-dpi pixmap)
    * else : a scaled value between 1.0 and 2.0. (pixel count is between normal-dpi and high-dpi)
*/
qreal QIconPrivate::pixmapDevicePixelRatio(qreal displayDevicePixelRatio, const QSize &requestedSize, const QSize &actualSize)
{
    QSize targetSize = requestedSize * displayDevicePixelRatio;
    qreal scale = 0.5 * (qreal(actualSize.width()) / qreal(targetSize.width()) +
                         qreal(actualSize.height() / qreal(targetSize.height())));
    return qMax(qreal(1.0), displayDevicePixelRatio *scale);
}

QPixmapIconEngine::QPixmapIconEngine()
{
}

QPixmapIconEngine::QPixmapIconEngine(const QPixmapIconEngine &other)
    : QIconEngine(other), pixmaps(other.pixmaps)
{
}

QPixmapIconEngine::~QPixmapIconEngine()
{
}

void QPixmapIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size() * qt_effective_device_pixel_ratio(0);
    QPixmap px = pixmap(pixmapSize, mode, state);
    painter->drawPixmap(rect, px);
}

static inline int area(const QSize &s) { return s.width() * s.height(); }

// returns the smallest of the two that is still larger than or equal to size.
static QPixmapIconEngineEntry *bestSizeMatch( const QSize &size, QPixmapIconEngineEntry *pa, QPixmapIconEngineEntry *pb)
{
    int s = area(size);
    if (pa->size == QSize() && pa->pixmap.isNull()) {
        pa->pixmap = QPixmap(pa->fileName);
        pa->size = pa->pixmap.size();
    }
    int a = area(pa->size);
    if (pb->size == QSize() && pb->pixmap.isNull()) {
        pb->pixmap = QPixmap(pb->fileName);
        pb->size = pb->pixmap.size();
    }
    int b = area(pb->size);
    int res = a;
    if (qMin(a,b) >= s)
        res = qMin(a,b);
    else
        res = qMax(a,b);
    if (res == a)
        return pa;
    return pb;
}

QPixmapIconEngineEntry *QPixmapIconEngine::tryMatch(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmapIconEngineEntry *pe = 0;
    for (int i = 0; i < pixmaps.count(); ++i)
        if (pixmaps.at(i).mode == mode && pixmaps.at(i).state == state) {
            if (pe)
                pe = bestSizeMatch(size, &pixmaps[i], pe);
            else
                pe = &pixmaps[i];
        }
    return pe;
}


QPixmapIconEngineEntry *QPixmapIconEngine::bestMatch(const QSize &size, QIcon::Mode mode, QIcon::State state, bool sizeOnly)
{
    QPixmapIconEngineEntry *pe = tryMatch(size, mode, state);
    while (!pe){
        QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off : QIcon::On;
        if (mode == QIcon::Disabled || mode == QIcon::Selected) {
            QIcon::Mode oppositeMode = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;
            if ((pe = tryMatch(size, QIcon::Normal, state)))
                break;
            if ((pe = tryMatch(size, QIcon::Active, state)))
                break;
            if ((pe = tryMatch(size, mode, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Normal, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Active, oppositeState)))
                break;
            if ((pe = tryMatch(size, oppositeMode, state)))
                break;
            if ((pe = tryMatch(size, oppositeMode, oppositeState)))
                break;
        } else {
            QIcon::Mode oppositeMode = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
            if ((pe = tryMatch(size, oppositeMode, state)))
                break;
            if ((pe = tryMatch(size, mode, oppositeState)))
                break;
            if ((pe = tryMatch(size, oppositeMode, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Disabled, state)))
                break;
            if ((pe = tryMatch(size, QIcon::Selected, state)))
                break;
            if ((pe = tryMatch(size, QIcon::Disabled, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Selected, oppositeState)))
                break;
        }

        if (!pe)
            return pe;
    }

    if (sizeOnly ? (pe->size.isNull() || !pe->size.isValid()) : pe->pixmap.isNull()) {
        pe->pixmap = QPixmap(pe->fileName);
        if (!pe->pixmap.isNull())
            pe->size = pe->pixmap.size();
    }

    return pe;
}

QPixmap QPixmapIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm;
    QPixmapIconEngineEntry *pe = bestMatch(size, mode, state, false);
    if (pe)
        pm = pe->pixmap;

    if (pm.isNull()) {
        int idx = pixmaps.count();
        while (--idx >= 0) {
            if (pe == &pixmaps[idx]) {
                pixmaps.remove(idx);
                break;
            }
        }
        if (pixmaps.isEmpty())
            return pm;
        else
            return pixmap(size, mode, state);
    }

    QSize actualSize = pm.size();
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

    QString key = QLatin1String("qt_")
                  % HexString<quint64>(pm.cacheKey())
                  % HexString<uint>(pe->mode)
                  % HexString<quint64>(QGuiApplication::palette().cacheKey())
                  % HexString<uint>(actualSize.width())
                  % HexString<uint>(actualSize.height());

    if (mode == QIcon::Active) {
        if (QPixmapCache::find(key % HexString<uint>(mode), pm))
            return pm; // horray
        if (QPixmapCache::find(key % HexString<uint>(QIcon::Normal), pm)) {
            QPixmap active = pm;
            if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
                active = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(QIcon::Active, pm);
            if (pm.cacheKey() == active.cacheKey())
                return pm;
        }
    }

    if (!QPixmapCache::find(key % HexString<uint>(mode), pm)) {
        if (pm.size() != actualSize)
            pm = pm.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if (pe->mode != mode && mode != QIcon::Normal) {
            QPixmap generated = pm;
            if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
                generated = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, pm);
            if (!generated.isNull())
                pm = generated;
        }
        QPixmapCache::insert(key % HexString<uint>(mode), pm);
    }
    return pm;
}

QSize QPixmapIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QSize actualSize;
    if (QPixmapIconEngineEntry *pe = bestMatch(size, mode, state, true))
        actualSize = pe->size;

    if (actualSize.isNull())
        return actualSize;

    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);
    return actualSize;
}

void QPixmapIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    if (!pixmap.isNull()) {
        QPixmapIconEngineEntry *pe = tryMatch(pixmap.size(), mode, state);
        if(pe && pe->size == pixmap.size()) {
            pe->pixmap = pixmap;
            pe->fileName.clear();
        } else {
            pixmaps += QPixmapIconEngineEntry(pixmap, mode, state);
        }
    }
}

void QPixmapIconEngine::addFile(const QString &fileName, const QSize &_size, QIcon::Mode mode, QIcon::State state)
{
    if (!fileName.isEmpty()) {
        QString abs = fileName;
        if (fileName.at(0) != QLatin1Char(':'))
            abs = QFileInfo(fileName).absoluteFilePath();
        QImageReader reader(abs);

        do {
            QSize size = _size;
            QPixmap pixmap;

            for (int i = 0; i < pixmaps.count(); ++i) {
                if (pixmaps.at(i).mode == mode && pixmaps.at(i).state == state) {
                    QPixmapIconEngineEntry *pe = &pixmaps[i];
                    if (size == QSize()) {
                        pixmap.convertFromImage(reader.read());
                        size = pixmap.size();
                    }
                    if (pe->size == QSize() && pe->pixmap.isNull()) {
                        pe->pixmap = QPixmap(pe->fileName);
                        // Reset the devicePixelRatio. The pixmap may be loaded from a @2x file,
                        // but be used as a 1x pixmap by QIcon.
                        pe->pixmap.setDevicePixelRatio(1.0);
                        pe->size = pe->pixmap.size();
                    }
                    if (pe->size == size) {
                        pe->pixmap = pixmap;
                        pe->fileName = abs;
                        return;
                    }
                }
            }
            QPixmapIconEngineEntry e(abs, size, mode, state);
            e.pixmap = pixmap;
            pixmaps += e;
        } while (reader.jumpToNextImage());
    }
}

QString QPixmapIconEngine::key() const
{
    return QLatin1String("QPixmapIconEngine");
}

QIconEngine *QPixmapIconEngine::clone() const
{
    return new QPixmapIconEngine(*this);
}

bool QPixmapIconEngine::read(QDataStream &in)
{
    int num_entries;
    QPixmap pm;
    QString fileName;
    QSize sz;
    uint mode;
    uint state;

    in >> num_entries;
    for (int i=0; i < num_entries; ++i) {
        if (in.atEnd()) {
            pixmaps.clear();
            return false;
        }
        in >> pm;
        in >> fileName;
        in >> sz;
        in >> mode;
        in >> state;
        if (pm.isNull()) {
            addFile(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
        } else {
            QPixmapIconEngineEntry pe(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
            pe.pixmap = pm;
            pixmaps += pe;
        }
    }
    return true;
}

bool QPixmapIconEngine::write(QDataStream &out) const
{
    int num_entries = pixmaps.size();
    out << num_entries;
    for (int i=0; i < num_entries; ++i) {
        if (pixmaps.at(i).pixmap.isNull())
            out << QPixmap(pixmaps.at(i).fileName);
        else
            out << pixmaps.at(i).pixmap;
        out << pixmaps.at(i).fileName;
        out << pixmaps.at(i).size;
        out << (uint) pixmaps.at(i).mode;
        out << (uint) pixmaps.at(i).state;
    }
    return true;
}

void QPixmapIconEngine::virtual_hook(int id, void *data)
{
    switch (id) {
    case QIconEngine::AvailableSizesHook: {
        QIconEngine::AvailableSizesArgument &arg =
            *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
        arg.sizes.clear();
        for (int i = 0; i < pixmaps.size(); ++i) {
            QPixmapIconEngineEntry &pe = pixmaps[i];
            if (pe.size == QSize() && pe.pixmap.isNull()) {
                pe.pixmap = QPixmap(pe.fileName);
                pe.size = pe.pixmap.size();
            }
            if (pe.mode == arg.mode && pe.state == arg.state && !pe.size.isEmpty())
                arg.sizes.push_back(pe.size);
        }
        break;
    }
    default:
        QIconEngine::virtual_hook(id, data);
    }
}

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QIconEngineFactoryInterface_iid, QLatin1String("/iconengines"), Qt::CaseInsensitive))

QFactoryLoader *qt_iconEngineFactoryLoader()
{
    return loader();
}
#endif



/*!
  \class QIcon

  \brief The QIcon class provides scalable icons in different modes
  and states.

  \ingroup painting
  \ingroup shared
  \inmodule QtGui

  A QIcon can generate smaller, larger, active, and disabled pixmaps
  from the set of pixmaps it is given. Such pixmaps are used by Qt
  widgets to show an icon representing a particular action.

  The simplest use of QIcon is to create one from a QPixmap file or
  resource, and then use it, allowing Qt to work out all the required
  icon styles and sizes. For example:

  \snippet code/src_gui_image_qicon.cpp 0

  To undo a QIcon, simply set a null icon in its place:

  \snippet code/src_gui_image_qicon.cpp 1

  Use the QImageReader::supportedImageFormats() and
  QImageWriter::supportedImageFormats() functions to retrieve a
  complete list of the supported file formats.

  When you retrieve a pixmap using pixmap(QSize, Mode, State), and no
  pixmap for this given size, mode and state has been added with
  addFile() or addPixmap(), then QIcon will generate one on the
  fly. This pixmap generation happens in a QIconEngineV2. The default
  engine scales pixmaps down if required, but never up, and it uses
  the current style to calculate a disabled appearance. By using
  custom icon engines, you can customize every aspect of generated
  icons. With QIconEnginePluginV2 it is possible to register different
  icon engines for different file suffixes, making it possible for
  third parties to provide additional icon engines to those included
  with Qt.

  \note Since Qt 4.2, an icon engine that supports SVG is included.

  \section1 Making Classes that Use QIcon

  If you write your own widgets that have an option to set a small
  pixmap, consider allowing a QIcon to be set for that pixmap.  The
  Qt class QToolButton is an example of such a widget.

  Provide a method to set a QIcon, and when you draw the icon, choose
  whichever pixmap is appropriate for the current state of your widget.
  For example:
  \snippet code/src_gui_image_qicon.cpp 2

  You might also make use of the \c Active mode, perhaps making your
  widget \c Active when the mouse is over the widget (see \l
  QWidget::enterEvent()), while the mouse is pressed pending the
  release that will activate the function, or when it is the currently
  selected item. If the widget can be toggled, the "On" mode might be
  used to draw a different icon.

  \image icon.png QIcon

  \sa {fowler}{GUI Design Handbook: Iconic Label}, {Icons Example}
*/


/*!
  Constructs a null icon.
*/
QIcon::QIcon()
    : d(0)
{
}

/*!
  Constructs an icon from a \a pixmap.
 */
QIcon::QIcon(const QPixmap &pixmap)
    :d(0)
{
    addPixmap(pixmap);
}

/*!
  Constructs a copy of \a other. This is very fast.
*/
QIcon::QIcon(const QIcon &other)
    :d(other.d)
{
    if (d)
        d->ref.ref();
}

/*!
    Constructs an icon from the file with the given \a fileName. The
    file will be loaded on demand.

    If \a fileName contains a relative path (e.g. the filename only)
    the relevant file must be found relative to the runtime working
    directory.

    The file name can be either refer to an actual file on disk or to
    one of the application's embedded resources.  See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    Use the QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormats() functions to retrieve a
    complete list of the supported file formats.
*/
QIcon::QIcon(const QString &fileName)
    : d(0)
{
    addFile(fileName);
}


/*!
    Creates an icon with a specific icon \a engine. The icon takes
    ownership of the engine.
*/
QIcon::QIcon(QIconEngine *engine)
    :d(new QIconPrivate)
{
    d->engine = engine;
}

/*!
    Destroys the icon.
*/
QIcon::~QIcon()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Assigns the \a other icon to this icon and returns a reference to
    this icon.
*/
QIcon &QIcon::operator=(const QIcon &other)
{
    if (other.d)
        other.d->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = other.d;
    return *this;
}

/*!
    \fn QIcon &QIcon::operator=(QIcon &&other)

    Move-assigns \a other to this QIcon instance.

    \since 5.2
*/

/*!
    \fn void QIcon::swap(QIcon &other)
    \since 4.8

    Swaps icon \a other with this icon. This operation is very
    fast and never fails.
*/

/*!
   Returns the icon as a QVariant.
*/
QIcon::operator QVariant() const
{
    return QVariant(QVariant::Icon, this);
}

/*! \fn int QIcon::serialNumber() const
    \obsolete

    Returns a number that identifies the contents of this
    QIcon object. Distinct QIcon objects can have
    the same serial number if they refer to the same contents
    (but they don't have to). Also, the serial number of
    a QIcon object may change during its lifetime.

    Use cacheKey() instead.

    A null icon always has a serial number of 0.

    Serial numbers are mostly useful in conjunction with caching.

    \sa QPixmap::serialNumber()
*/

/*!
    Returns a number that identifies the contents of this QIcon
    object. Distinct QIcon objects can have the same key if
    they refer to the same contents.
    \since 4.3

    The cacheKey() will change when the icon is altered via
    addPixmap() or addFile().

    Cache keys are mostly useful in conjunction with caching.

    \sa QPixmap::cacheKey()
*/
qint64 QIcon::cacheKey() const
{
    if (!d)
        return 0;
    return (((qint64) d->serialNum) << 32) | ((qint64) (d->detach_no));
}

/*!
  Returns a pixmap with the requested \a size, \a mode, and \a
  state, generating one if necessary. The pixmap might be smaller than
  requested, but never larger.

  Setting the Qt::AA_UseHighDpiPixmaps application attribute enables this
  function to return pixmaps that are larger than the requested size. Such
  images will have a devicePixelRatio larger than 1.

  \sa actualSize(), paint()
*/
QPixmap QIcon::pixmap(const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QPixmap();
    return pixmap(0, size, mode, state);
}

/*!
    \fn QPixmap QIcon::pixmap(int w, int h, Mode mode = Normal, State state = Off) const

    \overload

    Returns a pixmap of size QSize(\a w, \a h). The pixmap might be smaller than
    requested, but never larger.

    Setting the Qt::AA_UseHighDpiPixmaps application attribute enables this
    function to return pixmaps that are larger than the requested size. Such
    images will have a devicePixelRatio larger than 1.
*/

/*!
    \fn QPixmap QIcon::pixmap(int extent, Mode mode = Normal, State state = Off) const

    \overload

    Returns a pixmap of size QSize(\a extent, \a extent). The pixmap might be smaller
    than requested, but never larger.

    Setting the Qt::AA_UseHighDpiPixmaps application attribute enables this
    function to return pixmaps that are larger than the requested size. Such
    images will have a devicePixelRatio larger than 1.
*/

/*!  Returns the actual size of the icon for the requested \a size, \a
  mode, and \a state. The result might be smaller than requested, but
  never larger. The returned size is in device-independent pixels (This
  is relevant for high-dpi pixmaps.)

  \sa pixmap(), paint()
*/
QSize QIcon::actualSize(const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QSize();
    return actualSize(0, size, mode, state);
}

/*!
  \since 5.1

  Returns a pixmap with the requested \a window \a size, \a mode, and \a
  state, generating one if necessary.

  The pixmap can be smaller than the requested size. If \a window is on
  a high-dpi display the pixmap can be larger. In that case it will have
  a devicePixelRatio larger than 1.

  \sa  actualSize(), paint()
*/
QPixmap QIcon::pixmap(QWindow *window, const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QPixmap();

    qreal devicePixelRatio = qt_effective_device_pixel_ratio(window);

    // Handle the simple normal-dpi case:
    if (!(devicePixelRatio > 1.0))
        return d->engine->pixmap(size, mode, state);

    // Try get a pixmap that is big enough to be displayed at device pixel resolution.
    QPixmap pixmap = d->engine->pixmap(size * devicePixelRatio, mode, state);
    pixmap.setDevicePixelRatio(d->pixmapDevicePixelRatio(devicePixelRatio, size, pixmap.size()));
    return pixmap;
}

/*!
  \since 5.1

  Returns the actual size of the icon for the requested \a window  \a size, \a
  mode, and \a state.

  The pixmap can be smaller than the requested size. The returned size
  is in device-independent pixels (This is relevant for high-dpi pixmaps.)

  \sa actualSize(), pixmap(), paint()
*/
QSize QIcon::actualSize(QWindow *window, const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QSize();

    qreal devicePixelRatio = qt_effective_device_pixel_ratio(window);

    // Handle the simple normal-dpi case:
    if (!(devicePixelRatio > 1.0))
        return d->engine->actualSize(size, mode, state);

    QSize actualSize = d->engine->actualSize(size * devicePixelRatio, mode, state);
    return actualSize / d->pixmapDevicePixelRatio(devicePixelRatio, size, actualSize);
}

/*!
    Uses the \a painter to paint the icon with specified \a alignment,
    required \a mode, and \a state into the rectangle \a rect.

    \sa actualSize(), pixmap()
*/
void QIcon::paint(QPainter *painter, const QRect &rect, Qt::Alignment alignment, Mode mode, State state) const
{
    if (!d || !painter)
        return;

    // Copy of QStyle::alignedRect
    const QSize size = d->engine->actualSize(rect.size(), mode, state);
    alignment = QGuiApplicationPrivate::visualAlignment(painter->layoutDirection(), alignment);
    int x = rect.x();
    int y = rect.y();
    int w = size.width();
    int h = size.height();
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += rect.size().height()/2 - h/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += rect.size().height() - h;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += rect.size().width() - w;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += rect.size().width()/2 - w/2;
    QRect alignedRect(x, y, w, h);

    d->engine->paint(painter, alignedRect, mode, state);
}

/*!
    \fn void QIcon::paint(QPainter *painter, int x, int y, int w, int h, Qt::Alignment alignment,
                          Mode mode, State state) const

    \overload

    Paints the icon into the rectangle QRect(\a x, \a y, \a w, \a h).
*/

/*!
    Returns \c true if the icon is empty; otherwise returns \c false.

    An icon is empty if it has neither a pixmap nor a filename.

    Note: Even a non-null icon might not be able to create valid
    pixmaps, eg. if the file does not exist or cannot be read.
*/
bool QIcon::isNull() const
{
    return !d;
}

/*!\internal
 */
bool QIcon::isDetached() const
{
    return !d || d->ref.load() == 1;
}

/*! \internal
 */
void QIcon::detach()
{
    if (d) {
        if (d->ref.load() != 1) {
            QIconPrivate *x = new QIconPrivate;
            x->engine = d->engine->clone();
            if (!d->ref.deref())
                delete d;
            d = x;
        }
        ++d->detach_no;
    }
}

/*!
    Adds \a pixmap to the icon, as a specialization for \a mode and
    \a state.

    Custom icon engines are free to ignore additionally added
    pixmaps.

    \sa addFile()
*/
void QIcon::addPixmap(const QPixmap &pixmap, Mode mode, State state)
{
    if (pixmap.isNull())
        return;
    if (!d) {
        d = new QIconPrivate;
        d->engine = new QPixmapIconEngine;
    } else {
        detach();
    }
    d->engine->addPixmap(pixmap, mode, state);
}


/*!  Adds an image from the file with the given \a fileName to the
     icon, as a specialization for \a size, \a mode and \a state. The
     file will be loaded on demand. Note: custom icon engines are free
     to ignore additionally added pixmaps.

     If \a fileName contains a relative path (e.g. the filename only)
     the relevant file must be found relative to the runtime working
     directory.

    The file name can be either refer to an actual file on disk or to
    one of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    Use the QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormats() functions to retrieve a
    complete list of the supported file formats.

    Note: When you add a non-empty filename to a QIcon, the icon becomes
    non-null, even if the file doesn't exist or points to a corrupt file.

    \sa addPixmap()
 */
void QIcon::addFile(const QString &fileName, const QSize &size, Mode mode, State state)
{
    if (fileName.isEmpty())
        return;
    if (!d) {
#ifndef QT_NO_LIBRARY
        QFileInfo info(fileName);
        QString suffix = info.suffix();
        if (!suffix.isEmpty()) {
            // first try version 2 engines..
            const int index = loader()->indexOf(suffix);
            if (index != -1) {
                if (QIconEnginePlugin *factory = qobject_cast<QIconEnginePlugin*>(loader()->instance(index))) {
                    if (QIconEngine *engine = factory->create(fileName)) {
                        d = new QIconPrivate;
                        d->engine = engine;
                    }
                }
            }
        }
#endif
        // ...then fall back to the default engine
        if (!d) {
            d = new QIconPrivate;
            d->engine = new QPixmapIconEngine;
        }
    } else {
        detach();
    }
    d->engine->addFile(fileName, size, mode, state);

    // Check if a "@2x" file exists and add it.
    if (qApp->devicePixelRatio() > 1.0) {
        int dotIndex = fileName.lastIndexOf(QLatin1Char('.'));
        if (dotIndex != -1) {
            QString at2xfileName = fileName;
            at2xfileName.insert(dotIndex, QStringLiteral("@2x"));
            if (QFile::exists(at2xfileName))
                d->engine->addFile(at2xfileName, size, mode, state);
        }
    }
}

/*!
    \since 4.5

    Returns a list of available icon sizes for the specified \a mode and
    \a state.
*/
QList<QSize> QIcon::availableSizes(Mode mode, State state) const
{
    if (!d || !d->engine)
        return QList<QSize>();
    return d->engine->availableSizes(mode, state);
}

/*!
    \since 4.7

    Returns the name used to create the icon, if available.

    Depending on the way the icon was created, it may have an associated
    name. This is the case for icons created with fromTheme() or icons
    using a QIconEngine which supports the QIconEngineV2::IconNameHook.

    \sa fromTheme(), QIconEngine
*/
QString QIcon::name() const
{
    if (!d || !d->engine)
        return QString();
    return d->engine->iconName();
}

/*!
    \since 4.6

    Sets the search paths for icon themes to \a paths.
    \sa themeSearchPaths(), fromTheme(), setThemeName()
*/
void QIcon::setThemeSearchPaths(const QStringList &paths)
{
    QIconLoader::instance()->setThemeSearchPath(paths);
}

/*!
  \since 4.6

  Returns the search paths for icon themes.

  The default value will depend on the platform:

  On X11, the search path will use the XDG_DATA_DIRS environment
  variable if available.

  By default all platforms will have the resource directory
  \c{:\icons} as a fallback. You can use "rcc -project" to generate a
  resource file from your icon theme.

  \sa setThemeSearchPaths(), fromTheme(), setThemeName()
*/
QStringList QIcon::themeSearchPaths()
{
    return QIconLoader::instance()->themeSearchPaths();
}

/*!
    \since 4.6

    Sets the current icon theme to \a name.

    The \a name should correspond to a directory name in the
    themeSearchPath() containing an index.theme
    file describing it's contents.

    \sa themeSearchPaths(), themeName()
*/
void QIcon::setThemeName(const QString &name)
{
    QIconLoader::instance()->setThemeName(name);
}

/*!
    \since 4.6

    Returns the name of the current icon theme.

    On X11, the current icon theme depends on your desktop
    settings. On other platforms it is not set by default.

    \sa setThemeName(), themeSearchPaths(), fromTheme(),
    hasThemeIcon()
*/
QString QIcon::themeName()
{
    return QIconLoader::instance()->themeName();
}

/*!
    \since 4.6

    Returns the QIcon corresponding to \a name in the current
    icon theme. If no such icon is found in the current theme
    \a fallback is returned instead.

    The latest version of the freedesktop icon specification and naming
    specification can be obtained here:

    \list
    \li \l{http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html}
    \li \l{http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html}
    \endlist

    To fetch an icon from the current icon theme:

    \snippet code/src_gui_image_qicon.cpp 3

    Or if you want to provide a guaranteed fallback for platforms that
    do not support theme icons, you can use the second argument:

    \snippet code/src_gui_image_qicon.cpp 4

    \note By default, only X11 will support themed icons. In order to
    use themed icons on Mac and Windows, you will have to bundle a
    compliant theme in one of your themeSearchPaths() and set the
    appropriate themeName().

    \sa themeName(), setThemeName(), themeSearchPaths()
*/
QIcon QIcon::fromTheme(const QString &name, const QIcon &fallback)
{
    QIcon icon;

    if (qtIconCache()->contains(name)) {
        icon = *qtIconCache()->object(name);
    } else {
        QPlatformTheme * const platformTheme = QGuiApplicationPrivate::platformTheme();
        QIconEngine * const engine = platformTheme ? platformTheme->createIconEngine(name)
                                                   : new QIconLoaderEngine(name);
        QIcon *cachedIcon  = new QIcon(engine);
        qtIconCache()->insert(name, cachedIcon);
        icon = *cachedIcon;
    }

    // Note the qapp check is to allow lazy loading of static icons
    // Supporting fallbacks will not work for this case.
    if (qApp && icon.availableSizes().isEmpty())
        return fallback;

    return icon;
}

/*!
    \since 4.6

    Returns \c true if there is an icon available for \a name in the
    current icon theme, otherwise returns \c false.

    \sa themeSearchPaths(), fromTheme(), setThemeName()
*/
bool QIcon::hasThemeIcon(const QString &name)
{
    QIcon icon = fromTheme(name);

    return !icon.isNull();
}


/*****************************************************************************
  QIcon stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QIcon &icon)
    \relates QIcon
    \since 4.2

    Writes the given \a icon to the given \a stream as a PNG
    image. If the icon contains more than one image, all images will
    be written to the stream. Note that writing the stream to a file
    will not produce a valid image file.
*/

QDataStream &operator<<(QDataStream &s, const QIcon &icon)
{
    if (s.version() >= QDataStream::Qt_4_3) {
        if (icon.isNull()) {
            s << QString();
        } else {
            s << icon.d->engine->key();
            icon.d->engine->write(s);
        }
    } else if (s.version() == QDataStream::Qt_4_2) {
        if (icon.isNull()) {
            s << 0;
        } else {
            QPixmapIconEngine *engine = static_cast<QPixmapIconEngine *>(icon.d->engine);
            int num_entries = engine->pixmaps.size();
            s << num_entries;
            for (int i=0; i < num_entries; ++i) {
                s << engine->pixmaps.at(i).pixmap;
                s << engine->pixmaps.at(i).fileName;
                s << engine->pixmaps.at(i).size;
                s << (uint) engine->pixmaps.at(i).mode;
                s << (uint) engine->pixmaps.at(i).state;
            }
        }
    } else {
        s << QPixmap(icon.pixmap(22,22));
    }
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QIcon &icon)
    \relates QIcon
    \since 4.2

    Reads an image, or a set of images, from the given \a stream into
    the given \a icon.
*/

QDataStream &operator>>(QDataStream &s, QIcon &icon)
{
    if (s.version() >= QDataStream::Qt_4_3) {
        icon = QIcon();
        QString key;
        s >> key;
        if (key == QLatin1String("QPixmapIconEngine")) {
            icon.d = new QIconPrivate;
            QIconEngine *engine = new QPixmapIconEngine;
            icon.d->engine = engine;
            engine->read(s);
        } else if (key == QLatin1String("QIconLoaderEngine")) {
            icon.d = new QIconPrivate;
            QIconEngine *engine = new QIconLoaderEngine();
            icon.d->engine = engine;
            engine->read(s);
#ifndef QT_NO_LIBRARY
        } else {
            const int index = loader()->indexOf(key);
            if (index != -1) {
                if (QIconEnginePlugin *factory = qobject_cast<QIconEnginePlugin*>(loader()->instance(index))) {
                    if (QIconEngine *engine= factory->create()) {
                        icon.d = new QIconPrivate;
                        icon.d->engine = engine;
                        engine->read(s);
                    } // factory
                } // instance
            } // index
#endif
        }
    } else if (s.version() == QDataStream::Qt_4_2) {
        icon = QIcon();
        int num_entries;
        QPixmap pm;
        QString fileName;
        QSize sz;
        uint mode;
        uint state;

        s >> num_entries;
        for (int i=0; i < num_entries; ++i) {
            s >> pm;
            s >> fileName;
            s >> sz;
            s >> mode;
            s >> state;
            if (pm.isNull())
                icon.addFile(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
            else
                icon.addPixmap(pm, QIcon::Mode(mode), QIcon::State(state));
        }
    } else {
        QPixmap pm;
        s >> pm;
        icon.addPixmap(pm);
    }
    return s;
}

#endif //QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QIcon &i)
{
    dbg.nospace() << "QIcon(" << i.name() << ')';
    return dbg.space();
}
#endif

/*!
    \fn DataPtr &QIcon::data_ptr()
    \internal
*/

/*!
    \typedef QIcon::DataPtr
    \internal
*/

QT_END_NAMESPACE
#endif //QT_NO_ICON

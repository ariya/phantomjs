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

#include "qsound.h"

#ifndef QT_NO_SOUND

#include "qlist.h"
#include <private/qobject_p.h>
#include "qsound_p.h"

QT_BEGIN_NAMESPACE

static QList<QAuServer*> *servers=0;

QAuServer::QAuServer(QObject* parent)
    : QObject(parent)
{
    if (!servers)
        servers = new QList<QAuServer*>;
    servers->prepend(this);
}

QAuServer::~QAuServer()
{
    servers->removeAll(this);
    if (servers->count() == 0) {
        delete servers;
        servers = 0;
    }
}

void QAuServer::play(const QString& filename)
{
    QSound s(filename);
    play(&s);
}

extern QAuServer* qt_new_audio_server();

static QAuServer& server()
{
    if (!servers) qt_new_audio_server();
    return *servers->first();
}

class QSoundPrivate : public QObjectPrivate
{
public:
    QSoundPrivate(const QString& fname)
        : filename(fname), bucket(0), looprem(0), looptotal(1)
    {
    }

    ~QSoundPrivate()
    {
        delete bucket;
    }

    QString filename;
    QAuBucket* bucket;
    int looprem;
    int looptotal;
};

/*!
    \class QSound
    \brief The QSound class provides access to the platform audio facilities.

    \ingroup multimedia


    Qt provides the most commonly required audio operation in GUI
    applications: asynchronously playing a sound file. This is most
    easily accomplished using the static play() function:

    \snippet doc/src/snippets/code/src_gui_kernel_qsound.cpp 0

    Alternatively, create a QSound object from the sound file first
    and then call the play() slot:

    \snippet doc/src/snippets/code/src_gui_kernel_qsound.cpp 1

    Once created a QSound object can be queried for its fileName() and
    total number of loops() (i.e. the number of times the sound will
    play). The number of repetitions can be altered using the
    setLoops() function. While playing the sound, the loopsRemaining()
    function returns the remaining number of repetitions. Use the
    isFinished() function to determine whether the sound has finished
    playing.

    Sounds played using a QSound object may use more memory than the
    static play() function, but it may also play more immediately
    (depending on the underlying platform audio facilities). Use the
    static isAvailable() function to determine whether sound
    facilities exist on the platform. Which facilities that are
    actually used varies:

    \table
    \header \o Platform \o Audio Facility
    \row
    \o Microsoft Windows
    \o The underlying multimedia system is used; only WAVE format sound files
    are supported.
    \row
    \o X11
    \o The \l{ftp://ftp.x.org/contrib/audio/nas/}{Network Audio System}
    is used if available, otherwise all operations work silently. NAS
    supports WAVE and AU files.
    \row
    \o Mac OS X
    \o NSSound is used. All formats that NSSound supports, including QuickTime formats,
    are supported by Qt for Mac OS X.
    \row
    \o Qt for Embedded Linux
    \o A built-in mixing sound server is used, accessing \c /dev/dsp
    directly. Only the WAVE format is supported.
    \row
    \o Symbian
    \o CMdaAudioPlayerUtility is used. All formats that Symbian OS or devices support
    are supported also by Qt.
    \endtable

    Note that QSound does not support \l{resources.html}{resources}.
    This might be fixed in a future Qt version.
*/

/*!
    Plays the sound stored in the file specified by the given \a filename.

    \sa stop(), loopsRemaining(), isFinished()
*/
void QSound::play(const QString& filename)
{
    server().play(filename);
}

/*!
    Constructs a QSound object from the file specified by the given \a
    filename and with the given \a parent.

    This may use more memory than the static play() function, but it
    may also play more immediately (depending on the underlying
    platform audio facilities).

    \sa play()
*/
QSound::QSound(const QString& filename, QObject* parent)
    : QObject(*new QSoundPrivate(filename), parent)
{
    server().init(this);
}

#ifdef QT3_SUPPORT
/*!
    \obsolete

    Constructs a QSound object from the file specified by the given \a
    filename and with the given \a parent and \a name. Use the
    QSound() construcor and QObject::setObjectName() instead.

    \oldcode
        QSound *mySound = new QSound(filename, parent, name);
    \newcode
        QSounc *mySound = new QSound(filename, parent);
        mySound->setObjectName(name);
    \endcode
*/
QSound::QSound(const QString& filename, QObject* parent, const char* name)
    : QObject(*new QSoundPrivate(filename), parent)
{
    setObjectName(QString::fromAscii(name));
    server().init(this);
}
#endif

/*!
    Destroys this sound object. If the sound is not finished playing,
    the stop() function is called before the sound object is
    destructed.

    \sa stop(), isFinished()
*/
QSound::~QSound()
{
    if (!isFinished())
        stop();
}

/*!
    Returns true if the sound has finished playing; otherwise returns false.

    \warning On Windows this function always returns true for unlooped sounds.
*/
bool QSound::isFinished() const
{
    Q_D(const QSound);
    return d->looprem == 0;
}

/*!
    \overload

    Starts playing the sound specified by this QSound object.

    The function returns immediately.  Depending on the platform audio
    facilities, other sounds may stop or be mixed with the new
    sound. The sound can be played again at any time, possibly mixing
    or replacing previous plays of the sound.

    \sa fileName()
*/
void QSound::play()
{
    Q_D(QSound);
    d->looprem = d->looptotal;
    server().play(this);
}

/*!
    Returns the number of times the sound will play.

    \sa loopsRemaining(), setLoops()
*/
int QSound::loops() const
{
    Q_D(const QSound);
    return d->looptotal;
}

/*!
    Returns the remaining number of times the sound will loop (this
    value decreases each time the sound is played).

    \sa loops(), isFinished()
*/
int QSound::loopsRemaining() const
{
    Q_D(const QSound);
    return d->looprem;
}

/*!
    \fn void QSound::setLoops(int number)

    Sets the sound to repeat the given \a number of times when it is
    played.

    Note that passing the value -1 will cause the sound to loop
    indefinitely.

    \sa loops()
*/
void QSound::setLoops(int n)
{
    Q_D(QSound);
    d->looptotal = n;
}

/*!
    Returns the filename associated with this QSound object.

    \sa QSound()
*/
QString QSound::fileName() const
{
    Q_D(const QSound);
    return d->filename;
}

/*!
    Stops the sound playing.

    Note that on Windows the current loop will finish if a sound is
    played in a loop.

    \sa play()
*/
void QSound::stop()
{
    Q_D(QSound);
    server().stop(this);
    d->looprem = 0;
}


/*!
    Returns true if sound facilities exist on the platform; otherwise
    returns false.

    If no sound is available, all QSound operations work silently and
    quickly. An application may choose either to notify the user if
    sound is crucial to the application or to operate silently without
    bothering the user.

    Note: On Windows this always returns true because some sound card
    drivers do not implement a way to find out whether it is available
    or not.
*/
bool QSound::isAvailable()
{
    return server().okay();
}

/*!
    Sets the internal bucket record of sound \a s to \a b, deleting
    any previous setting.
*/
void QAuServer::setBucket(QSound* s, QAuBucket* b)
{
    delete s->d_func()->bucket;
    s->d_func()->bucket = b;
}

/*!
    Returns the internal bucket record of sound \a s.
*/
QAuBucket* QAuServer::bucket(QSound* s)
{
    return s->d_func()->bucket;
}

/*!
    Decrements the QSound::loopRemaining() value for sound \a s,
    returning the result.
*/
int QAuServer::decLoop(QSound* s)
{
    if (s->d_func()->looprem > 0)
        --s->d_func()->looprem;
    return s->d_func()->looprem;
}

/*!
    Initializes the sound. The default implementation does nothing.
*/
void QAuServer::init(QSound*)
{
}

QAuBucket::~QAuBucket()
{
}
/*!
    \fn bool QSound::available()

    Use the isAvailable() function instead.
*/

QT_END_NAMESPACE

#endif // QT_NO_SOUND

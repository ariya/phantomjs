/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qsound.h"

#ifndef QT_NO_SOUND

#include "qhash.h"
#include "qsocketnotifier.h"
#include "qapplication.h"
#include "qsound_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_NAS

QT_BEGIN_INCLUDE_NAMESPACE
#include <audio/audiolib.h>
#include <audio/soundlib.h>
QT_END_INCLUDE_NAMESPACE

static AuServer *nas=0;

static AuBool eventPred(AuServer *, AuEvent *e, AuPointer p)
{
    if (e && (e->type == AuEventTypeElementNotify)) {
        if (e->auelementnotify.flow == *((AuFlowID *)p))
            return true;
    }
    return false;
}

class QAuBucketNAS : public QAuBucket {
public:
    QAuBucketNAS(AuBucketID b, AuFlowID f = 0) : id(b), flow(f), stopped(true), numplaying(0) { }
    ~QAuBucketNAS()
    {
        if (nas) {
            AuSync(nas, false);
            AuDestroyBucket(nas, id, NULL);

            AuEvent ev;
            while (AuScanEvents(nas, AuEventsQueuedAfterFlush, true, eventPred, &flow, &ev))
                ;
        }
    }

    AuBucketID id;
    AuFlowID flow;
    bool     stopped;
    int      numplaying;
};

class QAuServerNAS : public QAuServer {
    Q_OBJECT

    QSocketNotifier* sn;

public:
    QAuServerNAS(QObject* parent);
    ~QAuServerNAS();

    void init(QSound*);
    void play(const QString& filename);
    void play(QSound*);
    void stop(QSound*);
    bool okay();
    void setDone(QSound*);

public slots:
    void dataReceived();
    void soundDestroyed(QObject *o);

private:
    QAuBucketNAS* bucket(QSound* s)
    {
        return (QAuBucketNAS*)QAuServer::bucket(s);
    }
};

QAuServerNAS::QAuServerNAS(QObject* parent) :
    QAuServer(parent)
{
    setObjectName(QLatin1String("Network Audio System"));
    nas = AuOpenServer(NULL, 0, NULL, 0, NULL, NULL);
    if (nas) {
        AuSetCloseDownMode(nas, AuCloseDownDestroy, NULL);
        // Ask Qt for async messages...
        sn=new QSocketNotifier(AuServerConnectionNumber(nas),
                QSocketNotifier::Read);
        QObject::connect(sn, SIGNAL(activated(int)),
                this, SLOT(dataReceived()));
    } else {
        sn = 0;
    }
}

QAuServerNAS::~QAuServerNAS()
{
    if (nas)
        AuCloseServer(nas);
    delete sn;
    nas = 0;
}

typedef QHash<void*,QAuServerNAS*> AuServerHash;
static AuServerHash *inprogress=0;

void QAuServerNAS::soundDestroyed(QObject *o)
{
    if (inprogress) {
        QSound *so = static_cast<QSound *>(o);
        while (inprogress->remove(so))
            ; // Loop while remove returns true
    }
}

void QAuServerNAS::play(const QString& filename)
{
    if (nas) {
        int iv=100;
        AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
        AuSoundPlayFromFile(nas, filename.toLocal8Bit().constData(), AuNone, volume,
                            NULL, NULL, NULL, NULL, NULL, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flush();
    }
}

static void callback(AuServer*, AuEventHandlerRec*, AuEvent* e, AuPointer p)
{
    if (inprogress->contains(p) && e) {
        if (e->type==AuEventTypeElementNotify &&
                    e->auelementnotify.kind==AuElementNotifyKindState) {
            if (e->auelementnotify.cur_state == AuStateStop) {
                AuServerHash::Iterator it = inprogress->find(p);
                if (it != inprogress->end())
                    (*it)->setDone((QSound*)p);
            }
        }
    }
}

void QAuServerNAS::setDone(QSound* s)
{
    if (nas) {
        decLoop(s);
        if (s->loopsRemaining() && !bucket(s)->stopped) {
            bucket(s)->stopped = true;
            play(s);
        } else {
            if (--(bucket(s)->numplaying) == 0)
                bucket(s)->stopped = true;
            inprogress->remove(s);
        }
    }
}

void QAuServerNAS::play(QSound* s)
{
    if (nas) {
        ++(bucket(s)->numplaying);
        if (!bucket(s)->stopped) {
            stop(s);
        }

        bucket(s)->stopped = false;
        if (!inprogress)
            inprogress = new AuServerHash;
        inprogress->insert(s,this);
        int iv=100;
        AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
        QAuBucketNAS *b = bucket(s);
        AuSoundPlayFromBucket(nas, b->id, AuNone, volume,
                              callback, s, 0, &b->flow, NULL, NULL, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flush();
    }
}

void QAuServerNAS::stop(QSound* s)
{
    if (nas && !bucket(s)->stopped) {
        bucket(s)->stopped = true;
        AuStopFlow(nas, bucket(s)->flow, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flush();
    }
}

void QAuServerNAS::init(QSound* s)
{
    connect(s, SIGNAL(destroyed(QObject*)),
            this, SLOT(soundDestroyed(QObject*)));

    if (nas) {
        AuBucketID b_id =
            AuSoundCreateBucketFromFile(nas, s->fileName().toLocal8Bit().constData(),
                                        0 /*AuAccessAllMasks*/, NULL, NULL);
        setBucket(s, new QAuBucketNAS(b_id));
    }
}

bool QAuServerNAS::okay()
{
    return !!nas;
}

void QAuServerNAS::dataReceived()
{
    AuHandleEvents(nas);
}

QT_BEGIN_INCLUDE_NAMESPACE
#include "qsound_x11.moc"
QT_END_INCLUDE_NAMESPACE

#endif


class QAuServerNull : public QAuServer
{
public:
    QAuServerNull(QObject* parent);

    void play(const QString&) { }
    void play(QSound*s) { while(decLoop(s) > 0) /* nothing */ ; }
    void stop(QSound*) { }
    bool okay() { return false; }
};

QAuServerNull::QAuServerNull(QObject* parent)
    : QAuServer(parent)
{
}


QAuServer* qt_new_audio_server()
{
#ifndef QT_NO_NAS
    QAuServer* s = new QAuServerNAS(qApp);
    if (s->okay())
        return s;
    else
        delete s;
#endif
    return new QAuServerNull(qApp);
}

QT_END_NAMESPACE

#endif // QT_NO_SOUND

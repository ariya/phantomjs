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



#ifndef QT_NO_SOUND

#include "qdir.h"
#include "qapplication.h"
#include "qsound.h"
#include "qsound_p.h"
#include "qfileinfo.h"
#include <private/qcore_symbian_p.h>

#include <e32std.h>
#include <mdaaudiosampleplayer.h>

QT_BEGIN_NAMESPACE

class QAuServerS60;

class QAuBucketS60 : public QAuBucket, public MMdaAudioPlayerCallback
{
public:
    QAuBucketS60(QAuServerS60 *server, QSound *sound);
    ~QAuBucketS60();

    void play();
    void stop();

    inline QSound *sound() const { return m_sound; }

public: // from MMdaAudioPlayerCallback
    void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration);
    void MapcPlayComplete(TInt aError);

private:
    QSound *m_sound;
    QAuServerS60 *m_server;
    bool m_prepared;
    bool m_playCalled;
    CMdaAudioPlayerUtility *m_playUtility;
};


class QAuServerS60 : public QAuServer
{
public:
    QAuServerS60(QObject *parent);

    void init(QSound *s)
    {
        QAuBucketS60 *bucket = new QAuBucketS60(this, s);
        setBucket(s, bucket);
    }

    void play(QSound *s)
    {
        bucket(s)->play();
    }

    void stop(QSound *s)
    {
        bucket(s)->stop();
    }

    bool okay() { return true; }

    void play(const QString& filename);

protected:
    void playCompleted(QAuBucketS60 *bucket, int error);

protected:
    QAuBucketS60 *bucket(QSound *s)
    {
        return (QAuBucketS60 *)QAuServer::bucket( s );
    }

    friend class QAuBucketS60;

    // static QSound::play(filename) cannot be stopped, meaning that playCompleted
    // will get always called and QSound gets removed form this list.
    QList<QSound *> staticPlayingSounds;
};

QAuServerS60::QAuServerS60(QObject *parent) :
    QAuServer(parent)
{
    setObjectName(QLatin1String("QAuServerS60"));
}

void QAuServerS60::play(const QString& filename)
{
    QSound *s = new QSound(filename);
    staticPlayingSounds.append(s);
    play(s);
}

void QAuServerS60::playCompleted(QAuBucketS60 *bucket, int error)
{
    QSound *sound = bucket->sound();
    if (!error) {
        // We need to handle repeats by ourselves, since with Symbian API we don't
        // know how many loops have been played when user asks it
        if (decLoop(sound)) {
            play(sound);
        } else {
            if (staticPlayingSounds.removeAll(sound))
                delete sound;
        }
    } else {
        // We don't have a way to inform about errors -> just decrement loops
        // in order that QSound::isFinished will return true;
        while (decLoop(sound) > 0) {}
        if (staticPlayingSounds.removeAll(sound))
            delete sound;
    }
}

QAuServer *qt_new_audio_server()
{
    return new QAuServerS60(qApp);
}

QAuBucketS60::QAuBucketS60(QAuServerS60 *server, QSound *sound)
    : m_sound(sound), m_server(server), m_prepared(false), m_playCalled(false)
{
    QString filepath = QFileInfo(m_sound->fileName()).absoluteFilePath();
    filepath = QDir::toNativeSeparators(filepath);
    TPtrC filepathPtr(qt_QString2TPtrC(filepath));
    TRAPD(err, m_playUtility = CMdaAudioPlayerUtility::NewL(*this);
               m_playUtility->OpenFileL(filepathPtr));
    if (err) {
        m_server->playCompleted(this, err);
    }
}

void QAuBucketS60::play()
{
    if (m_prepared) {
        // OpenFileL call is completed we can start playing immediately
        m_playUtility->Play();
    } else {
        m_playCalled = true;
    }

}

void QAuBucketS60::stop()
{
    m_playCalled = false;
    m_playUtility->Stop();
}

void QAuBucketS60::MapcPlayComplete(TInt aError)
{
    m_server->playCompleted(this, aError);
}

void QAuBucketS60::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& /*aDuration*/)
{
    if (aError) {
        m_server->playCompleted(this, aError);
    } else {
        m_prepared = true;
        if (m_playCalled){
            play();
        }
    }
}

QAuBucketS60::~QAuBucketS60()
{
    if (m_playUtility){
        m_playUtility->Stop();
        m_playUtility->Close();
    }

    delete m_playUtility;
}


#endif // QT_NO_SOUND

QT_END_NAMESPACE

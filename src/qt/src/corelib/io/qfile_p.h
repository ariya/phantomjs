/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QFILE_P_H
#define QFILE_P_H

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

#include "QtCore/qabstractfileengine.h"
#include "private/qiodevice_p.h"
#include "private/qringbuffer_p.h"

QT_BEGIN_NAMESPACE

class QFilePrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QFile)

protected:
    QFilePrivate();
    ~QFilePrivate();

    bool openExternalFile(int flags, int fd, QFile::FileHandleFlags handleFlags);
    bool openExternalFile(int flags, FILE *fh, QFile::FileHandleFlags handleFlags);
#ifdef Q_OS_SYMBIAN
    bool openExternalFile(int flags, const RFile& f, QFile::FileHandleFlags handleFlags);
#endif

    QString fileName;
    mutable QAbstractFileEngine *fileEngine;

    bool lastWasWrite;
    QRingBuffer writeBuffer;
    inline bool ensureFlushed() const;

    bool putCharHelper(char c);
    
    QFile::FileError error;
    void setError(QFile::FileError err);
    void setError(QFile::FileError err, const QString &errorString);
    void setError(QFile::FileError err, int errNum);

    mutable qint64 cachedSize;

private:
    static QFile::EncoderFn encoder;
    static QFile::DecoderFn decoder;
};

QT_END_NAMESPACE

#endif // QFILE_P_H

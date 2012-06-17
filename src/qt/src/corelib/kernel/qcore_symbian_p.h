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

#ifndef QCORE_SYMBIAN_P_H
#define QCORE_SYMBIAN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <e32std.h>
#include <QtCore/qglobal.h>
#include <QtCore/qmutex.h>
#include <qstring.h>
#include <qrect.h>
#include <qhash.h>
#include <qscopedpointer.h>
#include <f32file.h>
#include <es_sock.h>

#define QT_LSTRING2(x) L##x
#define QT_LSTRING(x) QT_LSTRING2(x)

#if defined(QT_LIBINFIX)
#  define QT_LIBINFIX_UNICODE QT_LSTRING(QT_LIBINFIX)
#else
#  define QT_LIBINFIX_UNICODE L""
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT HBufC* qt_QString2HBufC(const QString& aString);

Q_CORE_EXPORT QString qt_TDesC2QString(const TDesC& aDescriptor);
inline QString qt_TDes2QString(const TDes& aDescriptor) { return qt_TDesC2QString(aDescriptor); }

static inline QSize qt_TSize2QSize(const TSize& ts)
{
    return QSize(ts.iWidth, ts.iHeight);
}

static inline TSize qt_QSize2TSize(const QSize& qs)
{
    return TSize(qs.width(), qs.height());
}

static inline QRect qt_TRect2QRect(const TRect& tr)
{
    return QRect(tr.iTl.iX, tr.iTl.iY, tr.Width(), tr.Height());
}

static inline TRect qt_QRect2TRect(const QRect& qr)
{
    return TRect(TPoint(qr.left(), qr.top()), TSize(qr.width(), qr.height()));
}

// Returned TPtrC is valid as long as the given parameter is valid and unmodified
static inline TPtrC qt_QString2TPtrC( const QString& string )
{
    return TPtrC16(static_cast<const TUint16*>(string.utf16()), string.length());
}

/*!
    \internal
    This class is a wrapper around the Symbian HBufC descriptor class.
    It makes sure that the heap allocated HBufC class is freed when it is
    destroyed.
*/
class Q_CORE_EXPORT QHBufC
{
public:
    QHBufC();
    QHBufC(const QHBufC &src);
    QHBufC(HBufC *src);
    QHBufC(const QString &src);
    ~QHBufC();

    inline operator HBufC *() { return m_hBufC; }
    inline operator const HBufC *() const { return m_hBufC; }
    inline HBufC *data() { return m_hBufC; }
    inline const HBufC *data() const { return m_hBufC; }
    inline HBufC & operator*() { return *m_hBufC; }
    inline const HBufC & operator*() const { return *m_hBufC; }
    inline HBufC * operator->() { return m_hBufC; }
    inline const HBufC * operator->() const { return m_hBufC; }

    inline bool operator==(const QHBufC &param) const { return data() == param.data(); }
    inline bool operator!=(const QHBufC &param) const { return data() != param.data(); }

private:
    HBufC *m_hBufC;
};

inline uint qHash(TUid uid)
{
    return qHash(uid.iUid);
}

Q_CORE_EXPORT RFs& qt_s60GetRFs();
Q_CORE_EXPORT RSocketServ& qt_symbianGetSocketServer();

// Defined in qlocale_symbian.cpp.
Q_CORE_EXPORT QByteArray qt_symbianLocaleName(int code);

template <typename R>
struct QScopedPointerRCloser
{
    static inline void cleanup(R *rPointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(R) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        if (rPointer)
            rPointer->Close();
    }
};

//Wrapper for RSocket so it can be used as a key in QHash or QMap
class QHashableSocket : public RSocket
{
public:
    bool operator==(const QHashableSocket &other) const
    {
        return SubSessionHandle() == other.SubSessionHandle()
            && Session().Handle() == other.Session().Handle();
    }
    bool operator<(const QHashableSocket &other) const
    {
        if (Session().Handle() == other.Session().Handle())
            return SubSessionHandle() < other.SubSessionHandle();
        return Session().Handle() < other.Session().Handle();
    }
};

uint qHash(const RSubSessionBase& key);

/*!
  \internal
  This class exists in QtCore for the benefit of QSocketNotifier, which uses integer
  file descriptors in its public API.
  So we need a way to map between int and RSocket.
  Additionally, it is used to host the global RSocketServ session
*/
class Q_CORE_EXPORT QSymbianSocketManager
{
public:
    QSymbianSocketManager();
    ~QSymbianSocketManager();

    /*!
      \internal
      \return handle to the socket server
    */
    RSocketServ& getSocketServer();
    /*!
      \internal
      Adds a symbian socket to the global map
      \param an open socket
      \return pseudo file descriptor, -1 if out of resources
    */
    int addSocket(const RSocket &sock);
    /*!
      \internal
      Removes a symbian socket from the global map
      \param an open socket
      \return true if the socket was in the map
    */
    bool removeSocket(const RSocket &sock);
    /*!
      \internal
      Get pseudo file descriptor for a socket
      \param an open socket
      \return integer handle, or -1 if not in map
    */
    int lookupSocket(const RSocket &sock) const;
    /*!
      \internal
      Get socket for a pseudo file descriptor
      \param an open socket fd
      \param sock (out) socket handle
      \return true on success or false if not in map
    */
    bool lookupSocket(int fd, RSocket& sock) const;

    /*!
      \internal
      Set the default connection to use for new sockets
      \param an open connection
    */
    void setDefaultConnection(RConnection* con);
    /*!
      \internal
      Get the default connection to use for new sockets
      \return the connection, or null pointer if there is none set
    */
    RConnection *defaultConnection() const;

    /*!
      \internal
      Add an opened connection to the active list
      \param an open connection
    */
    void addActiveConnection(TUint32 identifier);

    /*!
      \internal
      Remove a connection from the active list
      \param a closed connection
    */
    void removeActiveConnection(TUint32 identifier);

    /*!
      \internal
      Add an opened connection to the active list
      \param an open connection
    */
    QList<TUint32> activeConnections() const;

    /*!
      \internal
      Gets a reference to the singleton socket manager
    */
    static QSymbianSocketManager& instance();
private:
    int allocateSocket();

    const static int max_sockets = 0x20000; //covers all TCP and UDP ports, probably run out of memory first
    const static int socket_offset = 0x40000000; //hacky way of separating sockets from file descriptors
    int iNextSocket;
    QHash<QHashableSocket, int> socketMap;
    QHash<int, RSocket> reverseSocketMap;
    QHash<TUint32, int> activeConnectionsMap;
    mutable QMutex iMutex;
    RSocketServ iSocketServ;
    RConnection *iDefaultConnection;
};

template <typename T> class QScopedPointerResourceCloser
{
public:
    static inline void cleanup(T* pointer)
    {
        if (pointer)
            pointer->Close();
    }
};

/*typical use:
    RFile file;
    file.Open(...);
    QScopedResource<RFile> ptr(file);
    container.append(file); //this may throw std::bad_alloc, in which case file.Close() is called by destructor
    ptr.take(); //if we reach this line, ownership is transferred to the container
 */
template <typename T> class QScopedResource : public QScopedPointer<T, QScopedPointerResourceCloser<T> >
{
public:
    inline QScopedResource(T& resource) : QScopedPointer<T, QScopedPointerResourceCloser<T> >(&resource) {}
};

QT_END_NAMESPACE

QT_END_HEADER

#endif //QCORE_SYMBIAN_P_H

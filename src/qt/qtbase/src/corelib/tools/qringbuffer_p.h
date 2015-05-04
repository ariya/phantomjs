/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QRINGBUFFER_P_H
#define QRINGBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QRingBuffer
{
public:
    explicit inline QRingBuffer(int growth = 4096) :
        head(0), tail(0), tailBuffer(0), basicBlockSize(growth), bufferSize(0) {
        buffers.append(QByteArray());
    }

    inline int nextDataBlockSize() const {
        return (tailBuffer == 0 ? tail : buffers.first().size()) - head;
    }

    inline const char *readPointer() const {
        return buffers.isEmpty() ? 0 : (buffers.first().constData() + head);
    }

    // access the bytes at a specified position
    // the out-variable length will contain the amount of bytes readable
    // from there, e.g. the amount still the same QByteArray
    inline const char *readPointerAtPosition(qint64 pos, qint64 &length) const {
        if (pos >= 0) {
            pos += head;
            for (int i = 0; i < buffers.size(); ++i) {
                length = (i == tailBuffer ? tail : buffers[i].size());
                if (length > pos) {
                    length -= pos;
                    return buffers[i].constData() + pos;
                }
                pos -= length;
            }
        }

        length = 0;
        return 0;
    }

    inline void free(int bytes) {
        while (bytes > 0) {
            int blockSize = buffers.first().size() - head;

            if (tailBuffer == 0 || blockSize > bytes) {
                bufferSize -= bytes;
                if (bufferSize <= 0)
                    clear(); // try to minify/squeeze us
                else
                    head += bytes;
                return;
            }

            bufferSize -= blockSize;
            bytes -= blockSize;
            buffers.removeFirst();
            --tailBuffer;
            head = 0;
        }
    }

    inline char *reserve(int bytes) {
        if (bytes <= 0)
            return 0;

        // if need buffer reallocation
        if (tail + bytes > buffers.last().size()) {
            if (tail >= basicBlockSize) {
                // shrink this buffer to its current size
                buffers.last().resize(tail);

                // create a new QByteArray
                buffers.append(QByteArray());
                ++tailBuffer;
                tail = 0;
            }
            buffers.last().resize(qMax(basicBlockSize, tail + bytes));
        }

        char *writePtr = buffers.last().data() + tail;
        bufferSize += bytes;
        tail += bytes;
        return writePtr;
    }

    inline void truncate(int pos) {
        if (pos < size())
            chop(size() - pos);
    }

    inline void chop(int bytes) {
        while (bytes > 0) {
            if (tailBuffer == 0 || tail > bytes) {
                bufferSize -= bytes;
                if (bufferSize <= 0)
                    clear(); // try to minify/squeeze us
                else
                    tail -= bytes;
                return;
            }

            bufferSize -= tail;
            bytes -= tail;
            buffers.removeLast();
            --tailBuffer;
            tail = buffers.last().size();
        }
    }

    inline bool isEmpty() const {
        return tailBuffer == 0 && tail == 0;
    }

    inline int getChar() {
        if (isEmpty())
            return -1;
        char c = *readPointer();
        free(1);
        return int(uchar(c));
    }

    inline void putChar(char c) {
        char *ptr = reserve(1);
        *ptr = c;
    }

    inline void ungetChar(char c) {
        --head;
        if (head < 0) {
            buffers.prepend(QByteArray());
            buffers.first().resize(basicBlockSize);
            head = basicBlockSize - 1;
            ++tailBuffer;
        }
        buffers.first()[head] = c;
        ++bufferSize;
    }

    inline int size() const {
        return bufferSize;
    }

    inline void clear() {
        buffers.erase(buffers.begin() + 1, buffers.end());
        buffers.first().clear();

        head = tail = 0;
        tailBuffer = 0;
        bufferSize = 0;
    }

    inline int indexOf(char c) const {
        int index = 0;
        int j = head;
        for (int i = 0; i < buffers.size(); ++i) {
            const char *ptr = buffers[i].constData() + j;
            j = index + (i == tailBuffer ? tail : buffers[i].size()) - j;

            while (index < j) {
                if (*ptr++ == c)
                    return index;
                ++index;
            }
            j = 0;
        }
        return -1;
    }

    inline int indexOf(char c, int maxLength) const {
        int index = 0;
        int j = head;
        for (int i = 0; index < maxLength && i < buffers.size(); ++i) {
            const char *ptr = buffers[i].constData() + j;
            j = qMin(index + (i == tailBuffer ? tail : buffers[i].size()) - j, maxLength);

            while (index < j) {
                if (*ptr++ == c)
                    return index;
                ++index;
            }
            j = 0;
        }
        return -1;
    }

    inline int read(char *data, int maxLength) {
        int bytesToRead = qMin(size(), maxLength);
        int readSoFar = 0;
        while (readSoFar < bytesToRead) {
            int bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar, nextDataBlockSize());
            if (data)
                memcpy(data + readSoFar, readPointer(), bytesToReadFromThisBlock);
            readSoFar += bytesToReadFromThisBlock;
            free(bytesToReadFromThisBlock);
        }
        return readSoFar;
    }

    // read an unspecified amount (will read the first buffer)
    inline QByteArray read() {
        if (bufferSize == 0)
            return QByteArray();

        QByteArray qba(buffers.takeFirst());

        qba.reserve(0); // avoid that resizing needlessly reallocates
        if (tailBuffer == 0) {
            qba.resize(tail);
            tail = 0;
            buffers.append(QByteArray());
        } else {
            --tailBuffer;
        }
        qba.remove(0, head); // does nothing if head is 0
        head = 0;
        bufferSize -= qba.size();
        return qba;
    }

    // append a new buffer to the end
    inline void append(const QByteArray &qba) {
        if (tail == 0) {
            buffers.last() = qba;
        } else {
            buffers.last().resize(tail);
            buffers.append(qba);
            ++tailBuffer;
        }
        tail = qba.size();
        bufferSize += tail;
    }

    inline int skip(int length) {
        return read(0, length);
    }

    inline int readLine(char *data, int maxLength) {
        if (!data || --maxLength <= 0)
            return -1;

        int i = indexOf('\n', maxLength);
        i = read(data, i >= 0 ? (i + 1) : maxLength);

        // Terminate it.
        data[i] = '\0';
        return i;
    }

    inline bool canReadLine() const {
        return indexOf('\n') >= 0;
    }

private:
    QList<QByteArray> buffers;
    int head, tail;
    int tailBuffer; // always buffers.size() - 1
    const int basicBlockSize;
    int bufferSize;
};

QT_END_NAMESPACE

#endif // QRINGBUFFER_P_H

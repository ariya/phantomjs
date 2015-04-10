/*
    Copyright (C) 2014 Digia Plc. and/or its subsidiary(-ies)

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

#ifndef STORAGE_LEVELDB_PORT_PORT_QT_H_
#define STORAGE_LEVELDB_PORT_PORT_QT_H_

#include <QAtomicInteger>
#include <QAtomicPointer>
#include <QMutex>
#include <QWaitCondition>

#include <string>

#ifdef SNAPPY
#include <snappy.h>
#endif

#ifdef Q_CC_MSVC
#include "win/stdint.h"
#define snprintf _snprintf
#endif

namespace leveldb {
namespace port {

static const bool kLittleEndian = (Q_BYTE_ORDER == Q_LITTLE_ENDIAN);

class Mutex : public QMutex {
public:
    Mutex() { }

    void Lock() { lock(); }
    void Unlock() { unlock(); }
    void AssertHeld() { Q_ASSERT(!tryLock()); }
};

class CondVar : public QWaitCondition {
public:
    explicit CondVar(Mutex* mu) : m_mutex(mu) { }

    void Wait() { wait(m_mutex); }
    void Signal() { wakeOne(); }
    void SignalAll() { wakeAll(); }
private:
    Mutex* m_mutex;
};

typedef QAtomicInteger<int> OnceType;
#define LEVELDB_ONCE_INIT 0
inline void InitOnce(port::OnceType* once, void (*initializer)())
{
    if (once->testAndSetAcquire(0, 1))
        initializer();
}

class AtomicPointer : public QAtomicPointer<void> {
public:
    AtomicPointer() { }
    AtomicPointer(void *p) : QAtomicPointer<void>(p) { }
    void* Acquire_Load() const { return loadAcquire(); }
    void Release_Store(void* p) { storeRelease(p); }
    void* NoBarrier_Load() const { return load(); }
    void NoBarrier_Store(void* p) { store(p); }
};

// SNAPPY, non Qt specific:

inline bool Snappy_Compress(const char* input, size_t length,
                            ::std::string* output) {
#ifdef SNAPPY
    output->resize(snappy::MaxCompressedLength(length));
    size_t outlen;
    snappy::RawCompress(input, length, &(*output)[0], &outlen);
    output->resize(outlen);
    return true;
#endif

  return false;
}

inline bool Snappy_GetUncompressedLength(const char* input, size_t length,
                                         size_t* result) {
#ifdef SNAPPY
    return snappy::GetUncompressedLength(input, length, result);
#else
    return false;
#endif
}

inline bool Snappy_Uncompress(const char* input, size_t length,
                              char* output) {
#ifdef SNAPPY
    return snappy::RawUncompress(input, length, output);
#else
    return false;
#endif
}


inline bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg) {
    return false;
}

}  // namespace port
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_PORT_PORT_QT_H_

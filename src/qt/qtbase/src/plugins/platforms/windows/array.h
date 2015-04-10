/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef ARRAY_H
#define ARRAY_H

#include <QtCore/QtGlobal>
#include <algorithm>

QT_BEGIN_NAMESPACE

/* A simple, non-shared array. */

template <class T>
class Array
{
    Q_DISABLE_COPY(Array)
public:
    enum { initialSize = 5 };

    typedef T* const_iterator;

    explicit Array(size_t size= 0) : data(0), m_capacity(0), m_size(0)
        { if (size) resize(size); }
    ~Array() { delete [] data; }

    T *data;
    inline size_t size() const          { return m_size; }
    inline const_iterator begin() const { return data; }
    inline const_iterator end() const   { return data + m_size; }

    inline void append(const T &value)
    {
        const size_t oldSize = m_size;
        resize(m_size + 1);
        data[oldSize] = value;
    }

    inline void resize(size_t size)
    {
        if (size > m_size)
            reserve(size > 1 ? size + size / 2 : size_t(initialSize));
        m_size = size;
    }

    void reserve(size_t capacity)
    {
        if (capacity > m_capacity) {
            const T *oldData = data;
            data = new T[capacity];
            if (oldData) {
                std::copy(oldData, oldData + m_size, data);
                delete [] oldData;
            }
            m_capacity = capacity;
        }
    }

private:
    size_t m_capacity;
    size_t m_size;
};

QT_END_NAMESPACE

#endif // ARRAY_H

/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSTACK_H
#define QSTACK_H

#include <QtCore/qvector.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

template<class T>
class QStack : public QVector<T>
{
public:
    inline QStack() {}
    inline ~QStack() {}
    inline void swap(QStack<T> &other) { QVector<T>::swap(other); } // prevent QVector<->QStack swaps
    inline void push(const T &t) { QVector<T>::append(t); }
    T pop();
    T &top();
    const T &top() const;
};

template<class T>
inline T QStack<T>::pop()
{ Q_ASSERT(!this->isEmpty()); T t = this->data()[this->size() -1];
  this->resize(this->size()-1); return t; }

template<class T>
inline T &QStack<T>::top()
{ Q_ASSERT(!this->isEmpty()); this->detach(); return this->data()[this->size()-1]; }

template<class T>
inline const T &QStack<T>::top() const
{ Q_ASSERT(!this->isEmpty()); return this->data()[this->size()-1]; }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTACK_H

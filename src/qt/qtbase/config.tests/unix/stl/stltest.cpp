/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the config.tests of the Qt Toolkit.
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

/* Sample program for configure to test STL support on target
platforms.  We are mainly concerned with being able to instantiate
templates for common STL container classes.
*/

#include <iterator>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstddef>

// something mean to see if the compiler and C++ standard lib are good enough
template<class K, class T>
class DummyClass
{
    // everything in std namespace ?
    typedef std::bidirectional_iterator_tag i;
    typedef std::ptrdiff_t d;
    // typename implemented ?
    typedef typename std::map<K,T>::iterator MyIterator;
};

// extracted from QVector's strict iterator
template<class T>
class DummyIterator
{
    typedef DummyIterator<int> iterator;
public:
        T *i;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef std::ptrdiff_t difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        inline DummyIterator() : i(0) {}
        inline DummyIterator(T *n) : i(n) {}
        inline DummyIterator(const DummyIterator &o): i(o.i){}
        inline T &operator*() const { return *i; }
        inline T *operator->() const { return i; }
        inline T &operator[](int j) const { return *(i + j); }
        inline bool operator==(const DummyIterator &o) const { return i == o.i; }
        inline bool operator!=(const DummyIterator &o) const { return i != o.i; }
        inline bool operator<(const DummyIterator& other) const { return i < other.i; }
        inline bool operator<=(const DummyIterator& other) const { return i <= other.i; }
        inline bool operator>(const DummyIterator& other) const { return i > other.i; }
        inline bool operator>=(const DummyIterator& other) const { return i >= other.i; }
        inline DummyIterator &operator++() { ++i; return *this; }
        inline DummyIterator operator++(int) { T *n = i; ++i; return n; }
        inline DummyIterator &operator--() { i--; return *this; }
        inline DummyIterator operator--(int) { T *n = i; i--; return n; }
        inline DummyIterator &operator+=(int j) { i+=j; return *this; }
        inline DummyIterator &operator-=(int j) { i-=j; return *this; }
        inline DummyIterator operator+(int j) const { return DummyIterator(i+j); }
        inline DummyIterator operator-(int j) const { return DummyIterator(i-j); }
        inline int operator-(DummyIterator j) const { return i - j.i; }
};

int main()
{
    std::vector<int> v1;
    v1.push_back( 0 );
    v1.push_back( 1 );
    v1.push_back( 2 );
    v1.push_back( 3 );
    v1.push_back( 4 );
    int v1size = v1.size();
    v1size = 0;
    int v1capacity = v1.capacity();
    v1capacity = 0;

    std::vector<int>::iterator v1it = std::find( v1.begin(), v1.end(), 99 );
    bool v1notfound = (v1it == v1.end());
    v1notfound = false;

    v1it = std::find( v1.begin(), v1.end(), 3 );
    bool v1found = (v1it != v1.end());
    v1found = false;

    std::vector<int> v2;
    std::copy( v1.begin(), v1it, std::back_inserter( v2 ) );
    int v2size = v2.size();
    v2size = 0;

    std::map<int, double> m1;
    m1.insert( std::make_pair( 1, 2.0 ) );
    m1.insert( std::make_pair( 3, 2.0 ) );
    m1.insert( std::make_pair( 5, 2.0 ) );
    m1.insert( std::make_pair( 7, 2.0 ) );
    int m1size = m1.size();
    m1size = 0;
    std::map<int,double>::iterator m1it = m1.begin();
    for ( ; m1it != m1.end(); ++m1it ) {
        int first = (*m1it).first;
        first = 0;
        double second = (*m1it).second;
        second = 0.0;
    }
    std::map< int, double > m2( m1 );
    int m2size = m2.size();
    m2size = 0;

    DummyIterator<int> it1, it2;
    int n = std::distance(it1, it2);
    std::advance(it1, 3);

    return 0;
}


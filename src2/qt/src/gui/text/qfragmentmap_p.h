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

#ifndef QFRAGMENTMAP_P_H
#define QFRAGMENTMAP_P_H

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

#include "QtCore/qglobal.h"
#include <stdlib.h>
#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE


template <int N = 1>
class QFragment
{
public:
    quint32 parent;
    quint32 left;
    quint32 right;
    quint32 color;
    quint32 size_left_array[N];
    quint32 size_array[N];
    enum {size_array_max = N };
};

template <class Fragment>
class QFragmentMapData
{
    enum Color { Red, Black };
public:
    QFragmentMapData();
    ~QFragmentMapData();

    void init();

    class Header
    {
    public:
        quint32 root; // this relies on being at the same position as parent in the fragment struct
        quint32 tag;
        quint32 freelist;
        quint32 node_count;
        quint32 allocated;
    };


    enum {fragmentSize = sizeof(Fragment) };


    int length(uint field = 0) const;


    inline Fragment *fragment(uint index) {
        return (fragments + index);
    }
    inline const Fragment *fragment(uint index) const {
        return (fragments + index);
    }


    inline Fragment &F(uint index) { return fragments[index] ; }
    inline const Fragment &F(uint index) const { return fragments[index] ; }

    inline bool isRoot(uint index) const {
        return !fragment(index)->parent;
    }

    inline uint position(uint node, uint field = 0) const {
        Q_ASSERT(field < Fragment::size_array_max);
        const Fragment *f = fragment(node);
        uint offset = f->size_left_array[field];
        while (f->parent) {
            uint p = f->parent;
            f = fragment(p);
            if (f->right == node)
                offset += f->size_left_array[field] + f->size_array[field];
            node = p;
        }
        return offset;
    }
    inline uint sizeRight(uint node, uint field = 0) const {
        Q_ASSERT(field < Fragment::size_array_max);
        uint sr = 0;
        const Fragment *f = fragment(node);
        node = f->right;
        while (node) {
            f = fragment(node);
            sr += f->size_left_array[field] + f->size_array[field];
            node = f->right;
        }
        return sr;
    }
    inline uint sizeLeft(uint node, uint field = 0) const {
        Q_ASSERT(field < Fragment::size_array_max);
        return fragment(node)->size_left_array[field];
    }


    inline uint size(uint node, uint field = 0) const {
        Q_ASSERT(field < Fragment::size_array_max);
        return fragment(node)->size_array[field];
    }

    inline void setSize(uint node, int new_size, uint field = 0) {
        Q_ASSERT(field < Fragment::size_array_max);
        Fragment *f = fragment(node);
        int diff = new_size - f->size_array[field];
        f->size_array[field] = new_size;
        while (f->parent) {
            uint p = f->parent;
            f = fragment(p);
            if (f->left == node)
                f->size_left_array[field] += diff;
            node = p;
        }
    }


    uint findNode(int k, uint field = 0) const;

    uint insert_single(int key, uint length);
    uint erase_single(uint f);

    uint minimum(uint n) const {
        while (n && fragment(n)->left)
            n = fragment(n)->left;
        return n;
    }

    uint maximum(uint n) const {
        while (n && fragment(n)->right)
            n = fragment(n)->right;
        return n;
    }

    uint next(uint n) const;
    uint previous(uint n) const;

    inline uint root() const {
        Q_ASSERT(!head->root || !fragment(head->root)->parent);
        return head->root;
    }
    inline void setRoot(uint new_root) {
        Q_ASSERT(!head->root || !fragment(new_root)->parent);
        head->root = new_root;
    }

    inline bool isValid(uint n) const {
        return n > 0 && n != head->freelist;
    }

    union {
        Header *head;
        Fragment *fragments;
    };

private:

    void rotateLeft(uint x);
    void rotateRight(uint x);
    void rebalance(uint x);
    void removeAndRebalance(uint z);

    uint createFragment();
    void freeFragment(uint f);

};

template <class Fragment>
QFragmentMapData<Fragment>::QFragmentMapData()
    : fragments(0)
{
    init();
}

template <class Fragment>
void QFragmentMapData<Fragment>::init()
{
    // the following code will realloc an existing fragment or create a new one.
    // it will also ignore errors when shrinking an existing fragment.
    Fragment *newFragments = (Fragment *)realloc(fragments, 64*fragmentSize);
    if (newFragments) {
        fragments = newFragments;
        head->allocated = 64;
    }
    Q_CHECK_PTR(fragments);

    head->tag = (((quint32)'p') << 24) | (((quint32)'m') << 16) | (((quint32)'a') << 8) | 'p'; //TAG('p', 'm', 'a', 'p');
    head->root = 0;
    head->freelist = 1;
    head->node_count = 0;
    // mark all items to the right as unused
    F(head->freelist).right = 0;
}

template <class Fragment>
QFragmentMapData<Fragment>::~QFragmentMapData()
{
    free(fragments);
}

template <class Fragment>
uint QFragmentMapData<Fragment>::createFragment()
{
    Q_ASSERT(head->freelist <= head->allocated);

    uint freePos = head->freelist;
    if (freePos == head->allocated) {
        // need to create some free space
        uint needed = qAllocMore((freePos+1)*fragmentSize, 0);
        Q_ASSERT(needed/fragmentSize > head->allocated);
        Fragment *newFragments = (Fragment *)realloc(fragments, needed);
        Q_CHECK_PTR(newFragments);
        fragments = newFragments;
        head->allocated = needed/fragmentSize;
        F(freePos).right = 0;
    }

    uint nextPos = F(freePos).right;
    if (!nextPos) {
        nextPos = freePos+1;
        if (nextPos < head->allocated)
            F(nextPos).right = 0;
    }

    head->freelist = nextPos;

    ++head->node_count;

    return freePos;
}

template <class Fragment>
void QFragmentMapData<Fragment>::freeFragment(uint i)
{
    F(i).right = head->freelist;
    head->freelist = i;

    --head->node_count;
}


template <class Fragment>
uint QFragmentMapData<Fragment>::next(uint n) const {
    Q_ASSERT(n);
    if (F(n).right) {
        n = F(n).right;
        while (F(n).left)
            n = F(n).left;
    } else {
        uint y = F(n).parent;
        while (F(n).parent && n == F(y).right) {
            n = y;
            y = F(y).parent;
        }
        n = y;
    }
    return n;
}

template <class Fragment>
uint QFragmentMapData<Fragment>::previous(uint n) const {
    if (!n)
        return maximum(root());

    if (F(n).left) {
        n = F(n).left;
        while (F(n).right)
            n = F(n).right;
    } else {
        uint y = F(n).parent;
        while (F(n).parent && n == F(y).left) {
            n = y;
            y = F(y).parent;
        }
        n = y;
    }
    return n;
}


/*
     x              y
      \            / \
       y    -->   x   b
      / \          \
     a   b          a
*/
template <class Fragment>
void QFragmentMapData<Fragment>::rotateLeft(uint x)
{
    uint p = F(x).parent;
    uint y = F(x).right;


    if (y) {
        F(x).right = F(y).left;
        if (F(y).left)
            F(F(y).left).parent = x;
        F(y).left = x;
        F(y).parent = p;
    } else {
        F(x).right = 0;
    }
    if (!p) {
        Q_ASSERT(head->root == x);
        head->root = y;
    }
    else if (x == F(p).left)
        F(p).left = y;
    else
        F(p).right = y;
    F(x).parent = y;
    for (uint field = 0; field < Fragment::size_array_max; ++field)
        F(y).size_left_array[field] += F(x).size_left_array[field] + F(x).size_array[field];
}


/*
         x          y
        /          / \
       y    -->   a   x
      / \            /
     a   b          b
*/
template <class Fragment>
void QFragmentMapData<Fragment>::rotateRight(uint x)
{
    uint y = F(x).left;
    uint p = F(x).parent;

    if (y) {
        F(x).left = F(y).right;
        if (F(y).right)
            F(F(y).right).parent = x;
        F(y).right = x;
        F(y).parent = p;
    } else {
        F(x).left = 0;
    }
    if (!p) {
        Q_ASSERT(head->root == x);
        head->root = y;
    }
    else if (x == F(p).right)
        F(p).right = y;
    else
        F(p).left = y;
    F(x).parent = y;
    for (uint field = 0; field < Fragment::size_array_max; ++field)
        F(x).size_left_array[field] -= F(y).size_left_array[field] + F(y).size_array[field];
}


template <class Fragment>
void QFragmentMapData<Fragment>::rebalance(uint x)
{
    F(x).color = Red;

    while (F(x).parent && F(F(x).parent).color == Red) {
        uint p = F(x).parent;
        uint pp = F(p).parent;
        Q_ASSERT(pp);
        if (p == F(pp).left) {
            uint y = F(pp).right;
            if (y && F(y).color == Red) {
                F(p).color = Black;
                F(y).color = Black;
                F(pp).color = Red;
                x = pp;
            } else {
                if (x == F(p).right) {
                    x = p;
                    rotateLeft(x);
                    p = F(x).parent;
                    pp = F(p).parent;
                }
                F(p).color = Black;
                if (pp) {
                    F(pp).color = Red;
                    rotateRight(pp);
                }
            }
        } else {
            uint y = F(pp).left;
            if (y && F(y).color == Red) {
                F(p).color = Black;
                F(y).color = Black;
                F(pp).color = Red;
                x = pp;
            } else {
                if (x == F(p).left) {
                    x = p;
                    rotateRight(x);
                    p = F(x).parent;
                    pp = F(p).parent;
                }
                F(p).color = Black;
                if (pp) {
                    F(pp).color = Red;
                    rotateLeft(pp);
                }
            }
        }
    }
    F(root()).color = Black;
}


template <class Fragment>
uint QFragmentMapData<Fragment>::erase_single(uint z)
{
    uint w = previous(z);
    uint y = z;
    uint x;
    uint p;

    if (!F(y).left) {
        x = F(y).right;
    } else if (!F(y).right) {
        x = F(y).left;
    } else {
        y = F(y).right;
        while (F(y).left)
            y = F(y).left;
        x = F(y).right;
    }

    if (y != z) {
        F(F(z).left).parent = y;
        F(y).left = F(z).left;
        for (uint field = 0; field < Fragment::size_array_max; ++field)
            F(y).size_left_array[field] = F(z).size_left_array[field];
        if (y != F(z).right) {
            /*
                     z                y
                    / \              / \
                   a   b            a   b
                      /                /
                    ...     -->      ...
                    /                /
                   y                x
                  / \
                 0   x
             */
            p = F(y).parent;
            if (x)
                F(x).parent = p;
            F(p).left = x;
            F(y).right = F(z).right;
            F(F(z).right).parent = y;
            uint n = p;
            while (n != y) {
                for (uint field = 0; field < Fragment::size_array_max; ++field)
                    F(n).size_left_array[field] -= F(y).size_array[field];
                n = F(n).parent;
            }
        } else {
            /*
                     z                y
                    / \              / \
                   a   y     -->    a   x
                      / \
                     0   x
             */
            p = y;
        }
        uint zp = F(z).parent;
        if (!zp) {
            Q_ASSERT(head->root == z);
            head->root = y;
        } else if (F(zp).left == z) {
            F(zp).left = y;
            for (uint field = 0; field < Fragment::size_array_max; ++field)
                F(zp).size_left_array[field] -= F(z).size_array[field];
        } else {
            F(zp).right = y;
        }
        F(y).parent = zp;
        // Swap the colors
        uint c = F(y).color;
        F(y).color = F(z).color;
        F(z).color = c;
        y = z;
    } else {
        /*
                p          p            p          p
               /          /              \          \
              z    -->   x                z  -->     x
              |                           |
              x                           x
         */
        p = F(z).parent;
        if (x)
            F(x).parent = p;
        if (!p) {
            Q_ASSERT(head->root == z);
            head->root = x;
        } else if (F(p).left == z) {
            F(p).left = x;
            for (uint field = 0; field < Fragment::size_array_max; ++field)
                F(p).size_left_array[field] -= F(z).size_array[field];
        } else {
            F(p).right = x;
        }
    }
    uint n = z;
    while (F(n).parent) {
        uint p = F(n).parent;
        if (F(p).left == n) {
            for (uint field = 0; field < Fragment::size_array_max; ++field)
                F(p).size_left_array[field] -= F(z).size_array[field];
        }
        n = p;
    }

    freeFragment(z);


    if (F(y).color != Red) {
        while (F(x).parent && (x == 0 || F(x).color == Black)) {
            if (x == F(p).left) {
                uint w = F(p).right;
                if (F(w).color == Red) {
                    F(w).color = Black;
                    F(p).color = Red;
                    rotateLeft(p);
                    w = F(p).right;
                }
                if ((F(w).left == 0 || F(F(w).left).color == Black) &&
                    (F(w).right == 0 || F(F(w).right).color == Black)) {
                    F(w).color = Red;
                    x = p;
                    p = F(x).parent;
                } else {
                    if (F(w).right == 0 || F(F(w).right).color == Black) {
                        if (F(w).left)
                            F(F(w).left).color = Black;
                        F(w).color = Red;
                        rotateRight(F(p).right);
                        w = F(p).right;
                    }
                    F(w).color = F(p).color;
                    F(p).color = Black;
                    if (F(w).right)
                        F(F(w).right).color = Black;
                    rotateLeft(p);
                    break;
                }
            } else {
                uint w = F(p).left;
                if (F(w).color == Red) {
                    F(w).color = Black;
                    F(p).color = Red;
                    rotateRight(p);
                    w = F(p).left;
                }
                if ((F(w).right == 0 || F(F(w).right).color == Black) &&
                    (F(w).left == 0 || F(F(w).left).color == Black)) {
                    F(w).color = Red;
                    x = p;
                    p = F(x).parent;
                } else {
                    if (F(w).left == 0 || F(F(w).left).color == Black) {
                        if (F(w).right)
                            F(F(w).right).color = Black;
                        F(w).color = Red;
                        rotateLeft(F(p).left);
                        w = F(p).left;
                    }
                    F(w).color = F(p).color;
                    F(p).color = Black;
                    if (F(w).left)
                        F(F(w).left).color = Black;
                    rotateRight(p);
                    break;
                }
            }
        }
        if (x)
            F(x).color = Black;
    }

    return w;
}

template <class Fragment>
uint QFragmentMapData<Fragment>::findNode(int k, uint field) const
{
    Q_ASSERT(field < Fragment::size_array_max);
    uint x = root();

    uint s = k;
    while (x) {
        if (sizeLeft(x, field) <= s) {
            if (s < sizeLeft(x, field) + size(x, field))
                return x;
            s -= sizeLeft(x, field) + size(x, field);
            x = F(x).right;
        } else {
            x = F(x).left;
        }
    }
    return 0;
}

template <class Fragment>
uint QFragmentMapData<Fragment>::insert_single(int key, uint length)
{
    Q_ASSERT(!findNode(key) || (int)this->position(findNode(key)) == key);

    uint z = createFragment();

    F(z).left = 0;
    F(z).right = 0;
    F(z).size_array[0] = length;
    for (uint field = 1; field < Fragment::size_array_max; ++field)
        F(z).size_array[field] = 1;
    for (uint field = 0; field < Fragment::size_array_max; ++field)
        F(z).size_left_array[field] = 0;

    uint y = 0;
    uint x = root();

    Q_ASSERT(!x || F(x).parent == 0);

    uint s = key;
    bool right = false;
    while (x) {
        y = x;
        if (s <= F(x).size_left_array[0]) {
            x = F(x).left;
            right = false;
        } else {
            s -= F(x).size_left_array[0] + F(x).size_array[0];
            x = F(x).right;
            right = true;
        }
    }

    F(z).parent = y;
    if (!y) {
        head->root = z;
    } else if (!right) {
        F(y).left = z;
        for (uint field = 0; field < Fragment::size_array_max; ++field)
            F(y).size_left_array[field] = F(z).size_array[field];
    } else {
        F(y).right = z;
    }
    while (y && F(y).parent) {
        uint p = F(y).parent;
        if (F(p).left == y) {
            for (uint field = 0; field < Fragment::size_array_max; ++field)
                F(p).size_left_array[field] += F(z).size_array[field];
        }
        y = p;
    }
    rebalance(z);

    return z;
}


template <class Fragment>
int QFragmentMapData<Fragment>::length(uint field) const {
    uint root = this->root();
    return root ? sizeLeft(root, field) + size(root, field) + sizeRight(root, field) : 0;
}


template <class Fragment> // NOTE: must inherit QFragment
class QFragmentMap
{
public:
    class Iterator
    {
    public:
        QFragmentMap *pt;
        quint32 n;

        Iterator() : pt(0), n(0) {}
        Iterator(QFragmentMap *p, int node) : pt(p), n(node) {}
        Iterator(const Iterator& it) : pt(it.pt), n(it.n) {}

        inline bool atEnd() const { return !n; }

        bool operator==(const Iterator& it) const { return pt == it.pt && n == it.n; }
        bool operator!=(const Iterator& it) const { return pt != it.pt || n != it.n; }
        bool operator<(const Iterator &it) const { return position() < it.position(); }

        Fragment *operator*() { Q_ASSERT(!atEnd()); return pt->fragment(n); }
        const Fragment *operator*() const { Q_ASSERT(!atEnd()); return pt->fragment(n); }
        Fragment *operator->() { Q_ASSERT(!atEnd()); return pt->fragment(n); }
        const Fragment *operator->() const { Q_ASSERT(!atEnd()); return pt->fragment(n); }

        int position() const { Q_ASSERT(!atEnd()); return pt->data.position(n); }
        const Fragment *value() const { Q_ASSERT(!atEnd()); return pt->fragment(n); }
        Fragment *value() { Q_ASSERT(!atEnd()); return pt->fragment(n); }

        Iterator& operator++() {
            n = pt->data.next(n);
            return *this;
        }
        Iterator& operator--() {
            n = pt->data.previous(n);
            return *this;
        }

    };


    class ConstIterator
    {
    public:
        const QFragmentMap *pt;
        quint32 n;

        /**
         * Functions
         */
        ConstIterator() : pt(0), n(0) {}
        ConstIterator(const QFragmentMap *p, int node) : pt(p), n(node) {}
        ConstIterator(const ConstIterator& it) : pt(it.pt), n(it.n) {}
        ConstIterator(const Iterator& it) : pt(it.pt), n(it.n) {}

        inline bool atEnd() const { return !n; }

        bool operator==(const ConstIterator& it) const { return pt == it.pt && n == it.n; }
        bool operator!=(const ConstIterator& it) const { return pt != it.pt || n != it.n; }
        bool operator<(const ConstIterator &it) const { return position() < it.position(); }

        const Fragment *operator*()  const { Q_ASSERT(!atEnd()); return pt->fragment(n); }
        const Fragment *operator->()  const { Q_ASSERT(!atEnd()); return pt->fragment(n); }

        int position() const { Q_ASSERT(!atEnd()); return pt->data.position(n); }
        int size() const { Q_ASSERT(!atEnd()); return pt->data.size(n); }
        const Fragment *value() const { Q_ASSERT(!atEnd()); return pt->fragment(n); }

        ConstIterator& operator++() {
            n = pt->data.next(n);
            return *this;
        }
        ConstIterator& operator--() {
            n = pt->data.previous(n);
            return *this;
        }
    };


    QFragmentMap() {}
    ~QFragmentMap()
    {
        if (!data.fragments)
            return; // in case of out-of-memory, we won't have fragments
        for (Iterator it = begin(); !it.atEnd(); ++it)
            it.value()->free();
    }

    inline void clear() {
        for (Iterator it = begin(); !it.atEnd(); ++it)
            it.value()->free();
        data.init();
    }

    inline Iterator begin() { return Iterator(this, data.minimum(data.root())); }
    inline Iterator end() { return Iterator(this, 0); }
    inline ConstIterator begin() const { return ConstIterator(this, data.minimum(data.root())); }
    inline ConstIterator end() const { return ConstIterator(this, 0); }

    inline ConstIterator last() const { return ConstIterator(this, data.maximum(data.root())); }

    inline bool isEmpty() const { return data.head->node_count == 0; }
    inline int numNodes() const { return data.head->node_count; }
    int length(uint field = 0) const { return data.length(field); }

    Iterator find(int k, uint field = 0) { return Iterator(this, data.findNode(k, field)); }
    ConstIterator find(int k, uint field = 0) const { return ConstIterator(this, data.findNode(k, field)); }

    uint findNode(int k, uint field = 0) const { return data.findNode(k, field); }

    uint insert_single(int key, uint length)
    {
        uint f = data.insert_single(key, length);
        if (f != 0) {
            Fragment *frag = fragment(f);
            Q_ASSERT(frag);
            frag->initialize();
        }
        return f;
    }
    uint erase_single(uint f)
    {
      if (f != 0) {
          Fragment *frag = fragment(f);
          Q_ASSERT(frag);
          frag->free();
      }
      return data.erase_single(f);
    }

    inline Fragment *fragment(uint index) {
        Q_ASSERT(index != 0);
        return data.fragment(index);
    }
    inline const Fragment *fragment(uint index) const {
        Q_ASSERT(index != 0);
        return data.fragment(index);
    }
    inline uint position(uint node, uint field = 0) const { return data.position(node, field); }
    inline bool isValid(uint n) const { return data.isValid(n); }
    inline uint next(uint n) const { return data.next(n); }
    inline uint previous(uint n) const { return data.previous(n); }
    inline uint size(uint node, uint field = 0) const { return data.size(node, field); }
    inline void setSize(uint node, int new_size, uint field = 0)
        { data.setSize(node, new_size, field);
      if (node != 0 && field == 0) {
          Fragment *frag = fragment(node);
          Q_ASSERT(frag);
          frag->invalidate();
      }
    }

    inline int firstNode() const { return data.minimum(data.root()); }

private:
    friend class Iterator;
    friend class ConstIterator;

    QFragmentMapData<Fragment> data;

    QFragmentMap(const QFragmentMap& m);
    QFragmentMap& operator= (const QFragmentMap& m);
};

QT_END_NAMESPACE

#endif // QFRAGMENTMAP_P_H

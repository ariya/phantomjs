/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QRBTREE_P_H
#define QRBTREE_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

template <class T>
struct QRBTree
{
    struct Node
    {
        inline Node() : parent(0), left(0), right(0), red(true) { }
        inline ~Node() {if (left) delete left; if (right) delete right;}
        T data;
        Node *parent;
        Node *left;
        Node *right;
        bool red;
    };

    inline QRBTree() : root(0), freeList(0) { }
    inline ~QRBTree();

    inline void clear();

    void attachBefore(Node *parent, Node *child);
    void attachAfter(Node *parent, Node *child);

    inline Node *front(Node *node) const;
    inline Node *back(Node *node) const;
    Node *next(Node *node) const;
    Node *previous(Node *node) const;

    inline void deleteNode(Node *&node);
    inline Node *newNode();

    // Return 1 if 'left' comes after 'right', 0 if equal, and -1 otherwise.
    // 'left' and 'right' cannot be null.
    int order(Node *left, Node *right);
    inline bool validate() const;

private:
    void rotateLeft(Node *node);
    void rotateRight(Node *node);
    void update(Node *node);

    inline void attachLeft(Node *parent, Node *child);
    inline void attachRight(Node *parent, Node *child);

    int blackDepth(Node *top) const;
    bool checkRedBlackProperty(Node *top) const;

    void swapNodes(Node *n1, Node *n2);
    void detach(Node *node);

    // 'node' must be black. rebalance will reduce the depth of black nodes by one in the sibling tree.
    void rebalance(Node *node);

public:
    Node *root;
private:
    Node *freeList;
};

template <class T>
inline QRBTree<T>::~QRBTree()
{
    clear();
    while (freeList) {
        // Avoid recursively calling the destructor, as this list may become large.
        Node *next = freeList->right;
        freeList->right = 0;
        delete freeList;
        freeList = next;
    }
}

template <class T>
inline void QRBTree<T>::clear()
{
    if (root)
        delete root;
    root = 0;
}

template <class T>
void QRBTree<T>::rotateLeft(Node *node)
{
    //   |            |      //
    //   N            B      //
    //  / \          / \     //
    // A   B  --->  N   D    //
    //    / \      / \       //
    //   C   D    A   C      //

    Node *&ref = (node->parent ? (node == node->parent->left ? node->parent->left : node->parent->right) : root);
    ref = node->right;
    node->right->parent = node->parent;

    //   :        //
    //   N        //
    //  / :|      //
    // A   B      //
    //    / \     //
    //   C   D    //

    node->right = ref->left;
    if (ref->left)
        ref->left->parent = node;

    //   :   |     //
    //   N   B     //
    //  / \ : \    //
    // A   C   D   //

    ref->left = node;
    node->parent = ref;

    //     |       //
    //     B       //
    //    / \      //
    //   N   D     //
    //  / \        //
    // A   C       //
}

template <class T>
void QRBTree<T>::rotateRight(Node *node)
{
    //     |            |        //
    //     N            A        //
    //    / \          / \       //
    //   A   B  --->  C   N      //
    //  / \              / \     //
    // C   D            D   B    //

    Node *&ref = (node->parent ? (node == node->parent->left ? node->parent->left : node->parent->right) : root);
    ref = node->left;
    node->left->parent = node->parent;

    node->left = ref->right;
    if (ref->right)
        ref->right->parent = node;

    ref->right = node;
    node->parent = ref;
}

template <class T>
void QRBTree<T>::update(Node *node) // call this after inserting a node
{
    for (;;) {
        Node *parent = node->parent;

        // if the node is the root, color it black
        if (!parent) {
            node->red = false;
            return;
        }

        // if the parent is black, the node can be left red
        if (!parent->red)
            return;

        // at this point, the parent is red and cannot be the root
        Node *grandpa = parent->parent;
        Q_ASSERT(grandpa);

        Node *uncle = (parent == grandpa->left ? grandpa->right : grandpa->left);
        if (uncle && uncle->red) {
            // grandpa's black, parent and uncle are red.
            // let parent and uncle be black, grandpa red and recursively update grandpa.
            Q_ASSERT(!grandpa->red);
            parent->red = false;
            uncle->red = false;
            grandpa->red = true;
            node = grandpa;
            continue;
        }

        // at this point, uncle is black
        if (node == parent->right && parent == grandpa->left)
            rotateLeft(node = parent);
        else if (node == parent->left && parent == grandpa->right)
            rotateRight(node = parent);
        parent = node->parent;

        if (parent == grandpa->left) {
            rotateRight(grandpa);
            parent->red = false;
            grandpa->red = true;
        } else {
            rotateLeft(grandpa);
            parent->red = false;
            grandpa->red = true;
        }
        return;
    }
}

template <class T>
inline void QRBTree<T>::attachLeft(Node *parent, Node *child)
{
    Q_ASSERT(!parent->left);
    parent->left = child;
    child->parent = parent;
    update(child);
}

template <class T>
inline void QRBTree<T>::attachRight(Node *parent, Node *child)
{
    Q_ASSERT(!parent->right);
    parent->right = child;
    child->parent = parent;
    update(child);
}

template <class T>
void QRBTree<T>::attachBefore(Node *parent, Node *child)
{
    if (!root)
        update(root = child);
    else if (!parent)
        attachRight(back(root), child);
    else if (parent->left)
        attachRight(back(parent->left), child);
    else
        attachLeft(parent, child);
}

template <class T>
void QRBTree<T>::attachAfter(Node *parent, Node *child)
{
    if (!root)
        update(root = child);
    else if (!parent)
        attachLeft(front(root), child);
    else if (parent->right)
        attachLeft(front(parent->right), child);
    else
        attachRight(parent, child);
}

template <class T>
void QRBTree<T>::swapNodes(Node *n1, Node *n2)
{
    // Since iterators must not be invalidated, it is not sufficient to only swap the data.
    if (n1->parent == n2) {
        n1->parent = n2->parent;
        n2->parent = n1;
    } else if (n2->parent == n1) {
        n2->parent = n1->parent;
        n1->parent = n2;
    } else {
        qSwap(n1->parent, n2->parent);
    }

    qSwap(n1->left, n2->left);
    qSwap(n1->right, n2->right);
    qSwap(n1->red, n2->red);

    if (n1->parent) {
        if (n1->parent->left == n2)
            n1->parent->left = n1;
        else
            n1->parent->right = n1;
    } else {
        root = n1;
    }

    if (n2->parent) {
        if (n2->parent->left == n1)
            n2->parent->left = n2;
        else
            n2->parent->right = n2;
    } else {
        root = n2;
    }

    if (n1->left)
        n1->left->parent = n1;
    if (n1->right)
        n1->right->parent = n1;

    if (n2->left)
        n2->left->parent = n2;
    if (n2->right)
        n2->right->parent = n2;
}

template <class T>
void QRBTree<T>::detach(Node *node) // call this before removing a node.
{
    if (node->right)
        swapNodes(node, front(node->right));

    Node *child = (node->left ? node->left : node->right);

    if (!node->red) {
        if (child && child->red)
            child->red = false;
        else
            rebalance(node);
    }

    Node *&ref = (node->parent ? (node == node->parent->left ? node->parent->left : node->parent->right) : root);
    ref = child;
    if (child)
        child->parent = node->parent;
    node->left = node->right = node->parent = 0;
}

// 'node' must be black. rebalance will reduce the depth of black nodes by one in the sibling tree.
template <class T>
void QRBTree<T>::rebalance(Node *node)
{
    Q_ASSERT(!node->red);
    for (;;) {
        if (!node->parent)
            return;

        // at this point, node is not a parent, it is black, thus it must have a sibling.
        Node *sibling = (node == node->parent->left ? node->parent->right : node->parent->left);
        Q_ASSERT(sibling);

        if (sibling->red) {
            sibling->red = false;
            node->parent->red = true;
            if (node == node->parent->left)
                rotateLeft(node->parent);
            else
                rotateRight(node->parent);
            sibling = (node == node->parent->left ? node->parent->right : node->parent->left);
            Q_ASSERT(sibling);
        }

        // at this point, the sibling is black.
        Q_ASSERT(!sibling->red);

        if ((!sibling->left || !sibling->left->red) && (!sibling->right || !sibling->right->red)) {
            bool parentWasRed = node->parent->red;
            sibling->red = true;
            node->parent->red = false;
            if (parentWasRed)
                return;
            node = node->parent;
            continue;
        }

        // at this point, at least one of the sibling's children is red.

        if (node == node->parent->left) {
            if (!sibling->right || !sibling->right->red) {
                Q_ASSERT(sibling->left);
                sibling->red = true;
                sibling->left->red = false;
                rotateRight(sibling);

                sibling = sibling->parent;
                Q_ASSERT(sibling);
            }
            sibling->red = node->parent->red;
            node->parent->red = false;

            Q_ASSERT(sibling->right->red);
            sibling->right->red = false;
            rotateLeft(node->parent);
        } else {
            if (!sibling->left || !sibling->left->red) {
                Q_ASSERT(sibling->right);
                sibling->red = true;
                sibling->right->red = false;
                rotateLeft(sibling);

                sibling = sibling->parent;
                Q_ASSERT(sibling);
            }
            sibling->red = node->parent->red;
            node->parent->red = false;

            Q_ASSERT(sibling->left->red);
            sibling->left->red = false;
            rotateRight(node->parent);
        }
        return;
    }
}

template <class T>
inline typename QRBTree<T>::Node *QRBTree<T>::front(Node *node) const
{
    while (node->left)
        node = node->left;
    return node;
}

template <class T>
inline typename QRBTree<T>::Node *QRBTree<T>::back(Node *node) const
{
    while (node->right)
        node = node->right;
    return node;
}

template <class T>
typename QRBTree<T>::Node *QRBTree<T>::next(Node *node) const
{
    if (node->right)
        return front(node->right);
    while (node->parent && node == node->parent->right)
        node = node->parent;
    return node->parent;
}

template <class T>
typename QRBTree<T>::Node *QRBTree<T>::previous(Node *node) const
{
    if (node->left)
        return back(node->left);
    while (node->parent && node == node->parent->left)
        node = node->parent;
    return node->parent;
}

template <class T>
int QRBTree<T>::blackDepth(Node *top) const
{
    if (!top)
        return 0;
    int leftDepth = blackDepth(top->left);
    int rightDepth = blackDepth(top->right);
    if (leftDepth != rightDepth)
        return -1;
    if (!top->red)
        ++leftDepth;
    return leftDepth;
}

template <class T>
bool QRBTree<T>::checkRedBlackProperty(Node *top) const
{
    if (!top)
        return true;
    if (top->left && !checkRedBlackProperty(top->left))
        return false;
    if (top->right && !checkRedBlackProperty(top->right))
        return false;
    return !(top->red && ((top->left && top->left->red) || (top->right && top->right->red)));
}

template <class T>
inline bool QRBTree<T>::validate() const
{
    return checkRedBlackProperty(root) && blackDepth(root) != -1;
}

template <class T>
inline void QRBTree<T>::deleteNode(Node *&node)
{
    Q_ASSERT(node);
    detach(node);
    node->right = freeList;
    freeList = node;
    node = 0;
}

template <class T>
inline typename QRBTree<T>::Node *QRBTree<T>::newNode()
{
    if (freeList) {
        Node *node = freeList;
        freeList = freeList->right;
        node->parent = node->left = node->right = 0;
        node->red = true;
        return node;
    }
    return new Node;
}

// Return 1 if 'left' comes after 'right', 0 if equal, and -1 otherwise.
// 'left' and 'right' cannot be null.
template <class T>
int QRBTree<T>::order(Node *left, Node *right)
{
    Q_ASSERT(left && right);
    if (left == right)
        return 0;

    QVector<Node *> leftAncestors;
    QVector<Node *> rightAncestors;
    while (left) {
        leftAncestors.push_back(left);
        left = left->parent;
    }
    while (right) {
        rightAncestors.push_back(right);
        right = right->parent;
    }
    Q_ASSERT(leftAncestors.back() == root && rightAncestors.back() == root);

    while (!leftAncestors.empty() && !rightAncestors.empty() && leftAncestors.back() == rightAncestors.back()) {
        leftAncestors.pop_back();
        rightAncestors.pop_back();
    }

    if (!leftAncestors.empty())
        return (leftAncestors.back() == leftAncestors.back()->parent->left ? -1 : 1);

    if (!rightAncestors.empty())
        return (rightAncestors.back() == rightAncestors.back()->parent->right ? -1 : 1);

    // The code should never reach this point.
    Q_ASSERT(!leftAncestors.empty() || !rightAncestors.empty());
    return 0;
}

QT_END_NAMESPACE

#endif

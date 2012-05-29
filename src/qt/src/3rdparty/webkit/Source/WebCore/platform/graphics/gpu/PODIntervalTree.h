/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PODIntervalTree_h
#define PODIntervalTree_h

#include "PODArena.h"
#include "PODInterval.h"
#include "PODRedBlackTree.h"
#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {

#ifndef NDEBUG
template<class T>
struct ValueToString;
#endif

// An interval tree, which is a form of augmented red-black tree. It
// supports efficient (O(lg n)) insertion, removal and querying of
// intervals in the tree.
template<class T, class UserData = void*>
class PODIntervalTree : public PODRedBlackTree<PODInterval<T, UserData> > {
    WTF_MAKE_NONCOPYABLE(PODIntervalTree);
public:
    // Typedef to reduce typing when declaring intervals to be stored in
    // this tree.
    typedef PODInterval<T, UserData> IntervalType;

    PODIntervalTree()
        : PODRedBlackTree<IntervalType>()
    {
        init();
    }

    explicit PODIntervalTree(PassRefPtr<PODArena> arena)
        : PODRedBlackTree<IntervalType>(arena)
    {
        init();
    }

    // Returns all intervals in the tree which overlap the given query
    // interval. The returned intervals are sorted by increasing low
    // endpoint.
    Vector<IntervalType> allOverlaps(const IntervalType& interval) const
    {
        Vector<IntervalType> result;
        allOverlaps(interval, result);
        return result;
    }

    // Returns all intervals in the tree which overlap the given query
    // interval. The returned intervals are sorted by increasing low
    // endpoint.
    void allOverlaps(const IntervalType& interval, Vector<IntervalType>& result) const
    {
        // Explicit dereference of "this" required because of
        // inheritance rules in template classes.
        searchForOverlapsFrom(this->root(), interval, result);
    }

    // Helper to create interval objects.
    static IntervalType createInterval(const T& low, const T& high, const UserData data = 0)
    {
        return IntervalType(low, high, data);
    }

    virtual bool checkInvariants() const
    {
        if (!PODRedBlackTree<IntervalType>::checkInvariants())
            return false;
        if (!this->root())
            return true;
        return checkInvariantsFromNode(this->root(), 0);
    }

private:
    typedef typename PODRedBlackTree<IntervalType>::Node IntervalNode;

    // Initializes the tree.
    void init()
    {
        // Explicit dereference of "this" required because of
        // inheritance rules in template classes.
        this->setNeedsFullOrderingComparisons(true);
    }

    // Starting from the given node, adds all overlaps with the given
    // interval to the result vector. The intervals are sorted by
    // increasing low endpoint.
    void searchForOverlapsFrom(IntervalNode* node, const IntervalType& interval, Vector<IntervalType>& res) const
    {
        if (!node)
            return;

        // Because the intervals are sorted by left endpoint, inorder
        // traversal produces results sorted as desired.

        // See whether we need to traverse the left subtree.
        IntervalNode* left = node->left();
        if (left
            // This is phrased this way to avoid the need for operator
            // <= on type T.
            && !(left->data().maxHigh() < interval.low()))
            searchForOverlapsFrom(left, interval, res);

        // Check for overlap with current node.
        if (node->data().overlaps(interval))
            res.append(node->data());

        // See whether we need to traverse the right subtree.
        // This is phrased this way to avoid the need for operator <=
        // on type T.
        if (!(interval.high() < node->data().low()))
            searchForOverlapsFrom(node->right(), interval, res);
    }

    virtual bool updateNode(IntervalNode* node)
    {
        // Would use const T&, but need to reassign this reference in this
        // function.
        const T* curMax = &node->data().high();
        IntervalNode* left = node->left();
        if (left) {
            if (*curMax < left->data().maxHigh())
                curMax = &left->data().maxHigh();
        }
        IntervalNode* right = node->right();
        if (right) {
            if (*curMax < right->data().maxHigh())
                curMax = &right->data().maxHigh();
        }
        // This is phrased like this to avoid needing operator!= on type T.
        if (!(*curMax == node->data().maxHigh())) {
            node->data().setMaxHigh(*curMax);
            return true;
        }
        return false;
    }

    bool checkInvariantsFromNode(IntervalNode* node, T* currentMaxValue) const
    {
        // These assignments are only done in order to avoid requiring
        // a default constructor on type T.
        T leftMaxValue(node->data().maxHigh());
        T rightMaxValue(node->data().maxHigh());
        IntervalNode* left = node->left();
        IntervalNode* right = node->right();
        if (left) {
            if (!checkInvariantsFromNode(left, &leftMaxValue))
                return false;
        }
        if (right) {
            if (!checkInvariantsFromNode(right, &rightMaxValue))
                return false;
        }
        if (!left && !right) {
            // Base case.
            if (currentMaxValue)
                *currentMaxValue = node->data().high();
            return (node->data().high() == node->data().maxHigh());
        }
        T localMaxValue(node->data().maxHigh());
        if (!left || !right) {
            if (left)
                localMaxValue = leftMaxValue;
            else
                localMaxValue = rightMaxValue;
        } else
            localMaxValue = (leftMaxValue < rightMaxValue) ? rightMaxValue : leftMaxValue;
        if (localMaxValue < node->data().high())
            localMaxValue = node->data().high();
        if (!(localMaxValue == node->data().maxHigh())) {
#ifndef NDEBUG
            String localMaxValueString = ValueToString<T>::string(localMaxValue);
            LOG_ERROR("PODIntervalTree verification failed at node 0x%p: localMaxValue=%s and data=%s",
                      node, localMaxValueString.utf8().data(), node->data().toString().utf8().data());
#endif
            return false;
        }
        if (currentMaxValue)
            *currentMaxValue = localMaxValue;
        return true;
    }
};

#ifndef NDEBUG
// Support for printing PODIntervals at the PODRedBlackTree level.
template<class T, class UserData>
struct ValueToString<PODInterval<T, UserData> > {
    static String string(const PODInterval<T, UserData>& interval)
    {
        return interval.toString();
    }
};
#endif

} // namespace WebCore

#endif // PODIntervalTree_h

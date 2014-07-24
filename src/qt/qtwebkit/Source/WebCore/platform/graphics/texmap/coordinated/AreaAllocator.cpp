/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"

#if USE(COORDINATED_GRAPHICS)

#include "AreaAllocator.h"

namespace WebCore {

AreaAllocator::AreaAllocator(const IntSize& size)
    : m_size(size)
    , m_minAlloc(1, 1)
    , m_margin(0, 0)
{
}

AreaAllocator::~AreaAllocator()
{
}

void AreaAllocator::expand(const IntSize& size)
{
    m_size = m_size.expandedTo(size);
}

void AreaAllocator::expandBy(const IntSize& size)
{
    m_size += size;
}

void AreaAllocator::release(const IntRect&)
{
}

int AreaAllocator::overhead() const
{
    return 0;
}

IntSize AreaAllocator::roundAllocation(const IntSize& size) const
{
    int width = size.width() + m_margin.width();
    int height = size.height() + m_margin.height();
    int extra = width % m_minAlloc.width();
    if (extra)
        width += m_minAlloc.width() - extra;
    extra = height % m_minAlloc.height();
    if (extra)
        height += m_minAlloc.height() - extra;

    return IntSize(width, height);
}

GeneralAreaAllocator::GeneralAreaAllocator(const IntSize& size)
    : AreaAllocator(nextPowerOfTwo(size))
{
    m_root = new Node();
    m_root->rect = IntRect(0, 0, m_size.width(), m_size.height());
    m_root->largestFree = m_size;
    m_root->parent = 0;
    m_root->left = 0;
    m_root->right = 0;
    m_nodeCount = 1;
    setMinimumAllocation(IntSize(8, 8));
}

GeneralAreaAllocator::~GeneralAreaAllocator()
{
    freeNode(m_root);
}

void GeneralAreaAllocator::freeNode(Node* node)
{
    if (node) {
        freeNode(node->left);
        freeNode(node->right);
    }
    delete node;
}

void GeneralAreaAllocator::expand(const IntSize& size)
{
    AreaAllocator::expand(nextPowerOfTwo(size));

    if (m_root->rect.size() == m_size)
        return; // No change.

    if (!m_root->left && m_root->largestFree.width() > 0) {
        // No allocations have occurred, so just adjust the root size.
        m_root->rect = IntRect(0, 0, m_size.width(), m_size.height());
        m_root->largestFree = m_size;
        return;
    }

    // Add extra nodes above the current root to expand the tree.
    Node* oldRoot = m_root;
    Split split;
    if (m_size.width() >= m_size.height())
        split = SplitOnX;
    else
        split = SplitOnY;

    while (m_root->rect.size() != m_size) {
        if (m_root->rect.width() == m_size.width())
            split = SplitOnY;
        else if (m_root->rect.height() == m_size.height())
            split = SplitOnX;
        Node* parent = new Node();
        Node* right = new Node();
        m_nodeCount += 2;
        m_root->parent = parent;
        parent->parent = 0;
        parent->left = m_root;
        parent->right = right;
        parent->largestFree = m_root->rect.size();
        right->parent = parent;
        right->left = 0;
        right->right = 0;
        right->largestFree = m_root->rect.size();
        if (split == SplitOnX) {
            parent->rect = IntRect(m_root->rect.x(), m_root->rect.y(),
                m_root->rect.width() * 2, m_root->rect.height());
            right->rect = IntRect(m_root->rect.x() + m_root->rect.width(), m_root->rect.y(),
                m_root->rect.width(), m_root->rect.height());
        } else {
            parent->rect = IntRect(m_root->rect.x(), m_root->rect.y(),
                m_root->rect.width(), m_root->rect.height() * 2);
            right->rect = IntRect(m_root->rect.x(), m_root->rect.y() + m_root->rect.width(),
                m_root->rect.width(), m_root->rect.height());
        }
        split = (split == SplitOnX ? SplitOnY : SplitOnX);
        m_root = parent;
    }
    updateLargestFree(oldRoot);
}

static inline bool fitsWithin(const IntSize& size1, const IntSize& size2)
{
    return size1.width() <= size2.width() && size1.height() <= size2.height();
}

IntRect GeneralAreaAllocator::allocate(const IntSize& size)
{
    IntSize rounded = roundAllocation(size);
    rounded = nextPowerOfTwo(rounded);
    if (rounded.width() <= 0 || rounded.width() > m_size.width()
        || rounded.height() <= 0 || rounded.height() > m_size.height())
        return IntRect();

    IntPoint point = allocateFromNode(rounded, m_root);
    if (point.x() >= 0)
        return IntRect(point, size);
    return IntRect();
}

IntPoint GeneralAreaAllocator::allocateFromNode(const IntSize& size, Node* node)
{
    // Find the best node to insert into, which should be
    // a node with the least amount of unused space that is
    // big enough to contain the requested size.
    while (node) {
        // Go down a level and determine if the left or right
        // sub-tree contains the best chance of allocation.
        Node* left = node->left;
        Node* right = node->right;
        if (left && fitsWithin(size, left->largestFree)) {
            if (right && fitsWithin(size, right->largestFree)) {
                if (left->largestFree.width() < right->largestFree.width()
                    || left->largestFree.height() < right->largestFree.height()) {
                    // The largestFree values may be a little oversized,
                    // so try the left sub-tree and then the right sub-tree.
                    IntPoint point = allocateFromNode(size, left);
                    if (point.x() >= 0)
                        return point;
                    return allocateFromNode(size, right);
                }
                node = right;
            } else
                node = left;
        } else if (right && fitsWithin(size, right->largestFree))
            node = right;
        else if (left || right) {
            // Neither sub-node has enough space to allocate from.
            return IntPoint(-1, -1);
        } else if (fitsWithin(size, node->largestFree)) {
            // Do we need to split this node into smaller pieces?
            Split split;
            if (fitsWithin(IntSize(size.width() * 2, size.height() * 2), node->largestFree)) {
                // Split in either direction: choose the inverse of
                // the parent node's split direction to try to balance
                // out the wasted space as further subdivisions happen.
                if (node->parent
                    && node->parent->left->rect.x() == node->parent->right->rect.x())
                    split = SplitOnX;
                else if (node->parent)
                    split = SplitOnY;
                else if (node->rect.width() >= node->rect.height())
                    split = SplitOnX;
                else
                    split = SplitOnY;
            } else if (fitsWithin(IntSize(size.width() * 2, size.height()), node->largestFree)) {
                // Split along the X direction.
                split = SplitOnX;
            } else if (fitsWithin(IntSize(size.width(), size.height() * 2), node->largestFree)) {
                // Split along the Y direction.
                split = SplitOnY;
            } else {
                // Cannot split further - allocate this node.
                node->largestFree = IntSize(0, 0);
                updateLargestFree(node);
                return node->rect.location();
            }

            // Split the node, then go around again using the left sub-tree.
            node = splitNode(node, split);
        } else {
            // Cannot possibly fit into this node.
            break;
        }
    }
    return IntPoint(-1, -1);
}

GeneralAreaAllocator::Node* GeneralAreaAllocator::splitNode
    (Node* node, Split split)
{
    Node* left = new Node();
    Node* right = new Node();
    m_nodeCount += 2;
    left->parent = node;
    left->left = 0;
    left->right = 0;
    right->parent = node;
    right->left = 0;
    right->right = 0;
    node->left = left;
    node->right = right;

    if (split == SplitOnX) {
        left->rect = IntRect(node->rect.x(), node->rect.y(),
            node->rect.width() / 2, node->rect.height());
        right->rect = IntRect(left->rect.maxX(), node->rect.y(),
            node->rect.width() / 2, node->rect.height());
    } else {
        left->rect = IntRect(node->rect.x(), node->rect.y(),
            node->rect.width(), node->rect.height() / 2);
        right->rect = IntRect(node->rect.x(), left->rect.maxY(),
            node->rect.width(), node->rect.height() / 2);
    }

    left->largestFree = left->rect.size();
    right->largestFree = right->rect.size();
    node->largestFree = right->largestFree;
    return left;
}

void GeneralAreaAllocator::updateLargestFree(Node* node)
{
    while ((node = node->parent)) {
        node->largestFree = IntSize(
            std::max(node->left->largestFree.width(), node->right->largestFree.width()),
            std::max(node->left->largestFree.height(), node->right->largestFree.height())
            );
    }
}

void GeneralAreaAllocator::release(const IntRect& rect)
{
    // Locate the node that contains the allocated region.
    Node* node = m_root;
    IntPoint point = rect.location();
    while (node) {
        if (node->left && node->left->rect.contains(point))
            node = node->left;
        else if (node->right && node->right->rect.contains(point))
            node = node->right;
        else if (node->rect.contains(point))
            break;
        else
            return; // Point is completely outside the tree.
    }
    if (!node)
        return;

    // Mark the node as free and then work upwards through the tree
    // recombining and deleting nodes until we reach a sibling
    // that is still allocated.
    node->largestFree = node->rect.size();
    while (node->parent) {
        if (node->parent->left == node) {
            if (node->parent->right->largestFree != node->parent->right->rect.size())
                break;
        } else {
            if (node->parent->left->largestFree != node->parent->left->rect.size())
                break;
        }
        node = node->parent;
        freeNode(node->left);
        freeNode(node->right);
        m_nodeCount -= 2;
        node->left = 0;
        node->right = 0;
        node->largestFree = node->rect.size();
    }

    // Make the rest of our ancestors have the correct "largest free size".
    updateLargestFree(node);
}

int GeneralAreaAllocator::overhead() const
{
    return m_nodeCount * sizeof(Node);
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

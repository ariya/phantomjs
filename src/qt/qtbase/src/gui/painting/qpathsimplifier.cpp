/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "qpathsimplifier_p.h"

#include <QtCore/qvarlengtharray.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>
#include <QtCore/qalgorithms.h>

#include <math.h>

#include <private/qopengl_p.h>
#include <private/qrbtree_p.h>

QT_BEGIN_NAMESPACE

#define Q_FIXED_POINT_SCALE 256
#define Q_TRIANGULATE_END_OF_POLYGON quint32(-1)



//============================================================================//
//                                   QPoint                                   //
//============================================================================//

inline bool operator < (const QPoint &a, const QPoint &b)
{
    return a.y() < b.y() || (a.y() == b.y() && a.x() < b.x());
}

inline bool operator > (const QPoint &a, const QPoint &b)
{
    return b < a;
}

inline bool operator <= (const QPoint &a, const QPoint &b)
{
    return !(a > b);
}

inline bool operator >= (const QPoint &a, const QPoint &b)
{
    return !(a < b);
}

namespace {

inline int cross(const QPoint &u, const QPoint &v)
{
    return u.x() * v.y() - u.y() * v.x();
}

inline int dot(const QPoint &u, const QPoint &v)
{
    return u.x() * v.x() + u.y() * v.y();
}

//============================================================================//
//                                  Fraction                                  //
//============================================================================//

// Fraction must be in the range [0, 1)
struct Fraction
{
    bool isValid() const { return denominator != 0; }

    // numerator and denominator must not have common denominators.
    unsigned int numerator, denominator;
};

inline unsigned int gcd(unsigned int x, unsigned int y)
{
    while (y != 0) {
        unsigned int z = y;
        y = x % y;
        x = z;
    }
    return x;
}

// Fraction must be in the range [0, 1)
// Assume input is valid.
Fraction fraction(unsigned int n, unsigned int d) {
    Fraction result;
    if (n == 0) {
        result.numerator = 0;
        result.denominator = 1;
    } else {
        unsigned int g = gcd(n, d);
        result.numerator = n / g;
        result.denominator = d / g;
    }
    return result;
}

//============================================================================//
//                                  Rational                                  //
//============================================================================//

struct Rational
{
    int integer;
    Fraction fraction;
};

//============================================================================//
//                             IntersectionPoint                              //
//============================================================================//

struct IntersectionPoint
{
    bool isValid() const { return x.fraction.isValid() && y.fraction.isValid(); }
    QPoint round() const;
    bool isAccurate() const { return x.fraction.numerator == 0 && y.fraction.numerator == 0; }

    Rational x; // 8:8 signed, 32/32
    Rational y; // 8:8 signed, 32/32
};

QPoint IntersectionPoint::round() const
{
    QPoint result(x.integer, y.integer);
    if (2 * x.fraction.numerator >= x.fraction.denominator)
        ++result.rx();
    if (2 * y.fraction.numerator >= y.fraction.denominator)
        ++result.ry();
    return result;
}

// Return positive value if 'p' is to the right of the line 'v1'->'v2', negative if left of the
// line and zero if exactly on the line.
// The returned value is the z-component of the qCross product between 'v2-v1' and 'p-v1',
// which is twice the signed area of the triangle 'p'->'v1'->'v2' (positive for CW order).
inline int pointDistanceFromLine(const QPoint &p, const QPoint &v1, const QPoint &v2)
{
    return cross(v2 - v1, p - v1);
}

IntersectionPoint intersectionPoint(const QPoint &u1, const QPoint &u2,
                                    const QPoint &v1, const QPoint &v2)
{
    IntersectionPoint result = {{0, {0, 0}}, {0, {0, 0}}};

    QPoint u = u2 - u1;
    QPoint v = v2 - v1;
    int d1 = cross(u, v1 - u1);
    int d2 = cross(u, v2 - u1);
    int det = d2 - d1;
    int d3 = cross(v, u1 - v1);
    int d4 = d3 - det; //qCross(v, u2 - v1);

    // Check that the math is correct.
    Q_ASSERT(d4 == cross(v, u2 - v1));

    // The intersection point can be expressed as:
    // v1 - v * d1/det
    // v2 - v * d2/det
    // u1 + u * d3/det
    // u2 + u * d4/det

    // I'm only interested in lines that are crossing, so ignore parallel lines even if they overlap.
    if (det == 0)
        return result;

    if (det < 0) {
        det = -det;
        d1 = -d1;
        d2 = -d2;
        d3 = -d3;
        d4 = -d4;
    }

    // I'm only interested in lines intersecting at their interior, not at their end points.
    // The lines intersect at their interior if and only if 'd1 < 0', 'd2 > 0', 'd3 < 0' and 'd4 > 0'.
    if (d1 >= 0 || d2 <= 0 || d3 <= 0 || d4 >= 0)
        return result;

    // Calculate the intersection point as follows:
    // v1 - v * d1/det | v1 <= v2 (component-wise)
    // v2 - v * d2/det | v2 < v1 (component-wise)

    // Assuming 16 bits per vector component.
    if (v.x() >= 0) {
        result.x.integer = v1.x() + int(qint64(-v.x()) * d1 / det);
        result.x.fraction = fraction((unsigned int)(qint64(-v.x()) * d1 % det), (unsigned int)det);
    } else {
        result.x.integer = v2.x() + int(qint64(-v.x()) * d2 / det);
        result.x.fraction = fraction((unsigned int)(qint64(-v.x()) * d2 % det), (unsigned int)det);
    }

    if (v.y() >= 0) {
        result.y.integer = v1.y() + int(qint64(-v.y()) * d1 / det);
        result.y.fraction = fraction((unsigned int)(qint64(-v.y()) * d1 % det), (unsigned int)det);
    } else {
        result.y.integer = v2.y() + int(qint64(-v.y()) * d2 / det);
        result.y.fraction = fraction((unsigned int)(qint64(-v.y()) * d2 % det), (unsigned int)det);
    }

    Q_ASSERT(result.x.fraction.isValid());
    Q_ASSERT(result.y.fraction.isValid());
    return result;
}

//============================================================================//
//                               PathSimplifier                               //
//============================================================================//

class PathSimplifier
{
public:
    PathSimplifier(const QVectorPath &path, QDataBuffer<QPoint> &vertices,
                   QDataBuffer<quint32> &indices, const QTransform &matrix);

private:
    struct Element;

    class BoundingVolumeHierarchy
    {
    public:
        struct Node
        {
            enum Type
            {
                Leaf,
                Split
            };
            Type type;
            QPoint minimum;
            QPoint maximum;
            union {
                Element *element; // type == Leaf
                Node *left; // type == Split
            };
            Node *right;
        };

        BoundingVolumeHierarchy();
        ~BoundingVolumeHierarchy();
        void allocate(int nodeCount);
        void free();
        Node *newNode();

        Node *root;
    private:
        void freeNode(Node *n);

        Node *nodeBlock;
        int blockSize;
        int firstFree;
    };

    struct Element
    {
        enum Degree
        {
            Line = 1,
            Quadratic = 2,
            Cubic = 3
        };

        quint32 &upperIndex() { return indices[pointingUp ? degree : 0]; }
        quint32 &lowerIndex() { return indices[pointingUp ? 0 : degree]; }
        quint32 upperIndex() const { return indices[pointingUp ? degree : 0]; }
        quint32 lowerIndex() const { return indices[pointingUp ? 0 : degree]; }
        void flip();

        QPoint middle;
        quint32 indices[4]; // index to points
        Element *next, *previous; // used in connectElements()
        int winding; // used in connectElements()
        union {
            QRBTree<Element *>::Node *edgeNode; // used in connectElements()
            BoundingVolumeHierarchy::Node *bvhNode;
        };
        Degree degree : 8;
        uint processed : 1; // initially false, true when the element has been checked for intersections.
        uint pointingUp : 1; // used in connectElements()
        uint originallyPointingUp : 1; // used in connectElements()
    };

    class ElementAllocator
    {
    public:
        ElementAllocator();
        ~ElementAllocator();
        void allocate(int count);
        Element *newElement();
    private:
        struct ElementBlock
        {
            ElementBlock *next;
            int blockSize;
            int firstFree;
            Element elements[1];
        } *blocks;
    };

    struct Event
    {
        enum Type { Upper, Lower };
        bool operator < (const Event &other) const;

        QPoint point;
        Type type;
        Element *element;
    };

    typedef QRBTree<Element *>::Node RBNode;
    typedef BoundingVolumeHierarchy::Node BVHNode;

    void initElements(const QVectorPath &path, const QTransform &matrix);
    void removeIntersections();
    void connectElements();
    void fillIndices();
    BVHNode *buildTree(Element **elements, int elementCount);
    bool intersectNodes(QDataBuffer<Element *> &elements, BVHNode *elementNode, BVHNode *treeNode);
    bool equalElements(const Element *e1, const Element *e2);
    bool splitLineAt(QDataBuffer<Element *> &elements, BVHNode *node, quint32 pointIndex, bool processAgain);
    void appendSeparatingAxes(QVarLengthArray<QPoint, 12> &axes, Element *element);
    QPair<int, int> calculateSeparatingAxisRange(const QPoint &axis, Element *element);
    void splitCurve(QDataBuffer<Element *> &elements, BVHNode *node);
    bool setElementToQuadratic(Element *element, quint32 pointIndex1, const QPoint &ctrl, quint32 pointIndex2);
    bool setElementToCubic(Element *element, quint32 pointIndex1, const QPoint &ctrl1, const QPoint &ctrl2, quint32 pointIndex2);
    void setElementToCubicAndSimplify(Element *element, quint32 pointIndex1, const QPoint &ctrl1, const QPoint &ctrl2, quint32 pointIndex2);
    RBNode *findElementLeftOf(const Element *element, const QPair<RBNode *, RBNode *> &bounds);
    bool elementIsLeftOf(const Element *left, const Element *right);
    QPair<RBNode *, RBNode *> outerBounds(const QPoint &point);
    static bool flattenQuadratic(const QPoint &u, const QPoint &v, const QPoint &w);
    static bool flattenCubic(const QPoint &u, const QPoint &v, const QPoint &w, const QPoint &q);
    static bool splitQuadratic(const QPoint &u, const QPoint &v, const QPoint &w, QPoint *result);
    static bool splitCubic(const QPoint &u, const QPoint &v, const QPoint &w, const QPoint &q, QPoint *result);
    void subDivQuadratic(const QPoint &u, const QPoint &v, const QPoint &w);
    void subDivCubic(const QPoint &u, const QPoint &v, const QPoint &w, const QPoint &q);
    static void sortEvents(Event *events, int count);

    ElementAllocator m_elementAllocator;
    QDataBuffer<Element *> m_elements;
    QDataBuffer<QPoint> *m_points;
    BoundingVolumeHierarchy m_bvh;
    QDataBuffer<quint32> *m_indices;
    QRBTree<Element *> m_elementList;
    uint m_hints;
};

inline PathSimplifier::BoundingVolumeHierarchy::BoundingVolumeHierarchy()
    : root(0)
    , nodeBlock(0)
    , blockSize(0)
    , firstFree(0)
{
}

inline PathSimplifier::BoundingVolumeHierarchy::~BoundingVolumeHierarchy()
{
    free();
}

inline void PathSimplifier::BoundingVolumeHierarchy::allocate(int nodeCount)
{
    Q_ASSERT(nodeBlock == 0);
    Q_ASSERT(firstFree == 0);
    nodeBlock = new Node[blockSize = nodeCount];
}

inline void PathSimplifier::BoundingVolumeHierarchy::free()
{
    freeNode(root);
    delete[] nodeBlock;
    nodeBlock = 0;
    firstFree = blockSize = 0;
    root = 0;
}

inline PathSimplifier::BVHNode *PathSimplifier::BoundingVolumeHierarchy::newNode()
{
    if (firstFree < blockSize)
        return &nodeBlock[firstFree++];
    return new Node;
}

inline void PathSimplifier::BoundingVolumeHierarchy::freeNode(Node *n)
{
    if (!n)
        return;
    Q_ASSERT(n->type == Node::Split || n->type == Node::Leaf);
    if (n->type == Node::Split) {
        freeNode(n->left);
        freeNode(n->right);
    }
    if (!(n >= nodeBlock && n < nodeBlock + blockSize))
        delete n;
}

inline PathSimplifier::ElementAllocator::ElementAllocator()
    : blocks(0)
{
}

inline PathSimplifier::ElementAllocator::~ElementAllocator()
{
    while (blocks) {
        ElementBlock *block = blocks;
        blocks = blocks->next;
        free(block);
    }
}

inline void PathSimplifier::ElementAllocator::allocate(int count)
{
    Q_ASSERT(blocks == 0);
    Q_ASSERT(count > 0);
    blocks = (ElementBlock *)malloc(sizeof(ElementBlock) + (count - 1) * sizeof(Element));
    blocks->blockSize = count;
    blocks->next = 0;
    blocks->firstFree = 0;
}

inline PathSimplifier::Element *PathSimplifier::ElementAllocator::newElement()
{
    Q_ASSERT(blocks);
    if (blocks->firstFree < blocks->blockSize)
        return &blocks->elements[blocks->firstFree++];
    ElementBlock *oldBlock = blocks;
    blocks = (ElementBlock *)malloc(sizeof(ElementBlock) + (oldBlock->blockSize - 1) * sizeof(Element));
    blocks->blockSize = oldBlock->blockSize;
    blocks->next = oldBlock;
    blocks->firstFree = 0;
    return &blocks->elements[blocks->firstFree++];
}


inline bool PathSimplifier::Event::operator < (const Event &other) const
{
    if (point == other.point)
        return type < other.type;
    return other.point < point;
}

inline void PathSimplifier::Element::flip()
{
    for (int i = 0; i < (degree + 1) >> 1; ++i) {
        Q_ASSERT(degree >= Line && degree <= Cubic);
        Q_ASSERT(i >= 0 && i < degree);
        qSwap(indices[i], indices[degree - i]);
    }
    pointingUp = !pointingUp;
    Q_ASSERT(next == 0 && previous == 0);
}

PathSimplifier::PathSimplifier(const QVectorPath &path, QDataBuffer<QPoint> &vertices,
                               QDataBuffer<quint32> &indices, const QTransform &matrix)
    : m_elements(0)
    , m_points(&vertices)
    , m_indices(&indices)
{
    m_points->reset();
    m_indices->reset();
    initElements(path, matrix);
    if (!m_elements.isEmpty()) {
        removeIntersections();
        connectElements();
        fillIndices();
    }
}

void PathSimplifier::initElements(const QVectorPath &path, const QTransform &matrix)
{
    m_hints = path.hints();
    int pathElementCount = path.elementCount();
    if (pathElementCount == 0)
        return;
    m_elements.reserve(2 * pathElementCount);
    m_elementAllocator.allocate(2 * pathElementCount);
    m_points->reserve(2 * pathElementCount);
    const QPainterPath::ElementType *e = path.elements();
    const qreal *p = path.points();
    if (e) {
        qreal x, y;
        quint32 moveToIndex = 0;
        quint32 previousIndex = 0;
        for (int i = 0; i < pathElementCount; ++i, ++e, p += 2) {
            switch (*e) {
            case QPainterPath::MoveToElement:
                {
                    if (!m_points->isEmpty()) {
                        const QPoint &from = m_points->at(previousIndex);
                        const QPoint &to = m_points->at(moveToIndex);
                        if (from != to) {
                            Element *element = m_elementAllocator.newElement();
                            element->degree = Element::Line;
                            element->indices[0] = previousIndex;
                            element->indices[1] = moveToIndex;
                            element->middle.rx() = (from.x() + to.x()) >> 1;
                            element->middle.ry() = (from.y() + to.y()) >> 1;
                            m_elements.add(element);
                        }
                    }
                    previousIndex = moveToIndex = m_points->size();
                    matrix.map(p[0], p[1], &x, &y);
                    QPoint to(qRound(x * Q_FIXED_POINT_SCALE), qRound(y * Q_FIXED_POINT_SCALE));
                    m_points->add(to);
                }
                break;
            case QPainterPath::LineToElement:
                Q_ASSERT(!m_points->isEmpty());
                {
                    matrix.map(p[0], p[1], &x, &y);
                    QPoint to(qRound(x * Q_FIXED_POINT_SCALE), qRound(y * Q_FIXED_POINT_SCALE));
                    const QPoint &from = m_points->last();
                    if (to != from) {
                        Element *element = m_elementAllocator.newElement();
                        element->degree = Element::Line;
                        element->indices[0] = previousIndex;
                        element->indices[1] = quint32(m_points->size());
                        element->middle.rx() = (from.x() + to.x()) >> 1;
                        element->middle.ry() = (from.y() + to.y()) >> 1;
                        m_elements.add(element);
                        previousIndex = m_points->size();
                        m_points->add(to);
                    }
                }
                break;
            case QPainterPath::CurveToElement:
                Q_ASSERT(i + 2 < pathElementCount);
                Q_ASSERT(!m_points->isEmpty());
                Q_ASSERT(e[1] == QPainterPath::CurveToDataElement);
                Q_ASSERT(e[2] == QPainterPath::CurveToDataElement);
                {
                    quint32 startPointIndex = previousIndex;
                    matrix.map(p[4], p[5], &x, &y);
                    QPoint end(qRound(x * Q_FIXED_POINT_SCALE), qRound(y * Q_FIXED_POINT_SCALE));
                    previousIndex = m_points->size();
                    m_points->add(end);

                    // See if this cubic bezier is really quadratic.
                    qreal x1 = p[-2] + qreal(1.5) * (p[0] - p[-2]);
                    qreal y1 = p[-1] + qreal(1.5) * (p[1] - p[-1]);
                    qreal x2 = p[4] + qreal(1.5) * (p[2] - p[4]);
                    qreal y2 = p[5] + qreal(1.5) * (p[3] - p[5]);

                    Element *element = m_elementAllocator.newElement();
                    if (qAbs(x1 - x2) < qreal(1e-3) && qAbs(y1 - y2) < qreal(1e-3)) {
                        // The bezier curve is quadratic.
                        matrix.map(x1, y1, &x, &y);
                        QPoint ctrl(qRound(x * Q_FIXED_POINT_SCALE),
                                    qRound(y * Q_FIXED_POINT_SCALE));
                        setElementToQuadratic(element, startPointIndex, ctrl, previousIndex);
                    } else {
                        // The bezier curve is cubic.
                        matrix.map(p[0], p[1], &x, &y);
                        QPoint ctrl1(qRound(x * Q_FIXED_POINT_SCALE),
                                     qRound(y * Q_FIXED_POINT_SCALE));
                        matrix.map(p[2], p[3], &x, &y);
                        QPoint ctrl2(qRound(x * Q_FIXED_POINT_SCALE),
                                     qRound(y * Q_FIXED_POINT_SCALE));
                        setElementToCubicAndSimplify(element, startPointIndex, ctrl1, ctrl2,
                                                     previousIndex);
                    }
                    m_elements.add(element);
                }
                i += 2;
                e += 2;
                p += 4;

                break;
            default:
                Q_ASSERT_X(0, "QSGPathSimplifier::initialize", "Unexpected element type.");
                break;
            }
        }
        if (!m_points->isEmpty()) {
            const QPoint &from = m_points->at(previousIndex);
            const QPoint &to = m_points->at(moveToIndex);
            if (from != to) {
                Element *element = m_elementAllocator.newElement();
                element->degree = Element::Line;
                element->indices[0] = previousIndex;
                element->indices[1] = moveToIndex;
                element->middle.rx() = (from.x() + to.x()) >> 1;
                element->middle.ry() = (from.y() + to.y()) >> 1;
                m_elements.add(element);
            }
        }
    } else {
        qreal x, y;

        for (int i = 0; i < pathElementCount; ++i, p += 2) {
            matrix.map(p[0], p[1], &x, &y);
            QPoint to(qRound(x * Q_FIXED_POINT_SCALE), qRound(y * Q_FIXED_POINT_SCALE));
            if (to != m_points->last())
                m_points->add(to);
        }

        while (!m_points->isEmpty() && m_points->last() == m_points->first())
            m_points->pop_back();

        if (m_points->isEmpty())
            return;

        quint32 prev = quint32(m_points->size() - 1);
        for (int i = 0; i < m_points->size(); ++i) {
            QPoint &to = m_points->at(i);
            QPoint &from = m_points->at(prev);
            Element *element = m_elementAllocator.newElement();
            element->degree = Element::Line;
            element->indices[0] = prev;
            element->indices[1] = quint32(i);
            element->middle.rx() = (from.x() + to.x()) >> 1;
            element->middle.ry() = (from.y() + to.y()) >> 1;
            m_elements.add(element);
            prev = i;
        }
    }

    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->processed = false;
}

void PathSimplifier::removeIntersections()
{
    Q_ASSERT(!m_elements.isEmpty());
    QDataBuffer<Element *> elements(m_elements.size());
    for (int i = 0; i < m_elements.size(); ++i)
        elements.add(m_elements.at(i));
    m_bvh.allocate(2 * m_elements.size());
    m_bvh.root = buildTree(elements.data(), elements.size());

    elements.reset();
    for (int i = 0; i < m_elements.size(); ++i)
        elements.add(m_elements.at(i));

    while (!elements.isEmpty()) {
        Element *element = elements.last();
        elements.pop_back();
        BVHNode *node = element->bvhNode;
        Q_ASSERT(node->type == BVHNode::Leaf);
        Q_ASSERT(node->element == element);
        if (!element->processed) {
            if (!intersectNodes(elements, node, m_bvh.root))
                element->processed = true;
        }
    }

    m_bvh.free(); // The bounding volume hierarchy is not needed anymore.
}

void PathSimplifier::connectElements()
{
    Q_ASSERT(!m_elements.isEmpty());
    QDataBuffer<Event> events(m_elements.size() * 2);
    for (int i = 0; i < m_elements.size(); ++i) {
        Element *element = m_elements.at(i);
        element->next = element->previous = 0;
        element->winding = 0;
        element->edgeNode = 0;
        const QPoint &u = m_points->at(element->indices[0]);
        const QPoint &v = m_points->at(element->indices[element->degree]);
        if (u != v) {
            element->pointingUp = element->originallyPointingUp = v < u;

            Event event;
            event.element = element;
            event.point = u;
            event.type = element->pointingUp ? Event::Lower : Event::Upper;
            events.add(event);
            event.point = v;
            event.type = element->pointingUp ? Event::Upper : Event::Lower;
            events.add(event);
        }
    }
    QVarLengthArray<Element *, 8> orderedElements;
    if (!events.isEmpty())
        sortEvents(events.data(), events.size());
    while (!events.isEmpty()) {
        const Event *event = &events.last();
        QPoint eventPoint = event->point;

        // Find all elements passing through the event point.
        QPair<RBNode *, RBNode *> bounds = outerBounds(eventPoint);

        // Special case: single element above and single element below event point.
        int eventCount = events.size();
        if (event->type == Event::Lower && eventCount > 2) {
            QPair<RBNode *, RBNode *> range;
            range.first = bounds.first ? m_elementList.next(bounds.first)
                                       : m_elementList.front(m_elementList.root);
            range.second = bounds.second ? m_elementList.previous(bounds.second)
                                         : m_elementList.back(m_elementList.root);

            const Event *event2 = &events.at(eventCount - 2);
            const Event *event3 = &events.at(eventCount - 3);
            Q_ASSERT(event2->point == eventPoint); // There are always at least two events at a point.
            if (range.first == range.second && event2->type == Event::Upper && event3->point != eventPoint) {
                Element *element = event->element;
                Element *element2 = event2->element;
                element->edgeNode->data = event2->element;
                element2->edgeNode = element->edgeNode;
                element->edgeNode = 0;

                events.pop_back();
                events.pop_back();

                if (element2->pointingUp != element->pointingUp)
                    element2->flip();
                element2->winding = element->winding;
                int winding = element->winding;
                if (element->originallyPointingUp)
                    ++winding;
                if (winding == 0 || winding == 1) {
                    if (element->pointingUp) {
                        element->previous = event2->element;
                        element2->next = event->element;
                    } else {
                        element->next = event2->element;
                        element2->previous = event->element;
                    }
                }
                continue;
            }
        }
        orderedElements.clear();

        // First, find the ones above the event point.
        if (m_elementList.root) {
            RBNode *current = bounds.first ? m_elementList.next(bounds.first)
                                           : m_elementList.front(m_elementList.root);
            while (current != bounds.second) {
                Element *element = current->data;
                Q_ASSERT(element->edgeNode == current);
                int winding = element->winding;
                if (element->originallyPointingUp)
                    ++winding;
                const QPoint &lower = m_points->at(element->lowerIndex());
                if (lower == eventPoint) {
                    if (winding == 0 || winding == 1)
                        orderedElements.append(current->data);
                } else {
                    // The element is passing through 'event.point'.
                    Q_ASSERT(m_points->at(element->upperIndex()) != eventPoint);
                    Q_ASSERT(element->degree == Element::Line);
                    // Split the line.
                    Element *eventElement = event->element;
                    int indexIndex = (event->type == Event::Upper) == eventElement->pointingUp
                                     ? eventElement->degree : 0;
                    quint32 pointIndex = eventElement->indices[indexIndex];
                    Q_ASSERT(eventPoint == m_points->at(pointIndex));

                    Element *upperElement = m_elementAllocator.newElement();
                    *upperElement = *element;
                    upperElement->lowerIndex() = element->upperIndex() = pointIndex;
                    upperElement->edgeNode = 0;
                    element->next = element->previous = 0;
                    if (upperElement->next)
                        upperElement->next->previous = upperElement;
                    else if (upperElement->previous)
                        upperElement->previous->next = upperElement;
                    if (element->pointingUp != element->originallyPointingUp)
                        element->flip();
                    if (winding == 0 || winding == 1)
                        orderedElements.append(upperElement);
                    m_elements.add(upperElement);
                }
                current = m_elementList.next(current);
            }
        }
        while (!events.isEmpty() && events.last().point == eventPoint) {
            event = &events.last();
            if (event->type == Event::Upper) {
                Q_ASSERT(event->point == m_points->at(event->element->upperIndex()));
                RBNode *left = findElementLeftOf(event->element, bounds);
                RBNode *node = m_elementList.newNode();
                node->data = event->element;
                Q_ASSERT(event->element->edgeNode == 0);
                event->element->edgeNode = node;
                m_elementList.attachAfter(left, node);
            } else {
                Q_ASSERT(event->type == Event::Lower);
                Q_ASSERT(event->point == m_points->at(event->element->lowerIndex()));
                Element *element = event->element;
                Q_ASSERT(element->edgeNode);
                m_elementList.deleteNode(element->edgeNode);
                Q_ASSERT(element->edgeNode == 0);
            }
            events.pop_back();
        }

        if (m_elementList.root) {
            RBNode *current = bounds.first ? m_elementList.next(bounds.first)
                                           : m_elementList.front(m_elementList.root);
            int winding = bounds.first ? bounds.first->data->winding : 0;

            // Calculate winding numbers and flip elements if necessary.
            while (current != bounds.second) {
                Element *element = current->data;
                Q_ASSERT(element->edgeNode == current);
                int ccw = winding & 1;
                Q_ASSERT(element->pointingUp == element->originallyPointingUp);
                if (element->originallyPointingUp) {
                    --winding;
                } else {
                    ++winding;
                    ccw ^= 1;
                }
                element->winding = winding;
                if (ccw == 0)
                    element->flip();
                current = m_elementList.next(current);
            }

            // Pick elements with correct winding number.
            current = bounds.second ? m_elementList.previous(bounds.second)
                                    : m_elementList.back(m_elementList.root);
            while (current != bounds.first) {
                Element *element = current->data;
                Q_ASSERT(element->edgeNode == current);
                Q_ASSERT(m_points->at(element->upperIndex()) == eventPoint);
                int winding = element->winding;
                if (element->originallyPointingUp)
                    ++winding;
                if (winding == 0 || winding == 1)
                    orderedElements.append(current->data);
                current = m_elementList.previous(current);
            }
        }

        if (!orderedElements.isEmpty()) {
            Q_ASSERT((orderedElements.size() & 1) == 0);
            int i = 0;
            Element *firstElement = orderedElements.at(0);
            if (m_points->at(firstElement->indices[0]) != eventPoint) {
                orderedElements.append(firstElement);
                i = 1;
            }
            for (; i < orderedElements.size(); i += 2) {
                Q_ASSERT(i + 1 < orderedElements.size());
                Element *next = orderedElements.at(i);
                Element *previous = orderedElements.at(i + 1);
                Q_ASSERT(next->previous == 0);
                Q_ASSERT(previous->next == 0);
                next->previous = previous;
                previous->next = next;
            }
        }
    }
#ifndef QT_NO_DEBUG
    for (int i = 0; i < m_elements.size(); ++i) {
        const Element *element = m_elements.at(i);
        Q_ASSERT(element->next == 0 || element->next->previous == element);
        Q_ASSERT(element->previous == 0 || element->previous->next == element);
        Q_ASSERT((element->next == 0) == (element->previous == 0));
    }
#endif
}

void PathSimplifier::fillIndices()
{
    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->processed = false;
    for (int i = 0; i < m_elements.size(); ++i) {
        Element *element = m_elements.at(i);
        if (element->processed || element->next == 0)
            continue;
        do {
            m_indices->add(element->indices[0]);
            switch (element->degree) {
            case Element::Quadratic:
                {
                    QPoint pts[] = {
                        m_points->at(element->indices[0]),
                        m_points->at(element->indices[1]),
                        m_points->at(element->indices[2])
                    };
                    subDivQuadratic(pts[0], pts[1], pts[2]);
                }
                break;
            case Element::Cubic:
                {
                    QPoint pts[] = {
                        m_points->at(element->indices[0]),
                        m_points->at(element->indices[1]),
                        m_points->at(element->indices[2]),
                        m_points->at(element->indices[3])
                    };
                    subDivCubic(pts[0], pts[1], pts[2], pts[3]);
                }
                break;
            default:
                break;
            }
            Q_ASSERT(element->next);
            element->processed = true;
            element = element->next;
        } while (element != m_elements.at(i));
        m_indices->add(Q_TRIANGULATE_END_OF_POLYGON);
    }
}

PathSimplifier::BVHNode *PathSimplifier::buildTree(Element **elements, int elementCount)
{
    Q_ASSERT(elementCount > 0);
    BVHNode *node = m_bvh.newNode();
    if (elementCount == 1) {
        Element *element = *elements;
        element->bvhNode = node;
        node->type = BVHNode::Leaf;
        node->element = element;
        node->minimum = node->maximum = m_points->at(element->indices[0]);
        for (int i = 1; i <= element->degree; ++i) {
            const QPoint &p = m_points->at(element->indices[i]);
            node->minimum.rx() = qMin(node->minimum.x(), p.x());
            node->minimum.ry() = qMin(node->minimum.y(), p.y());
            node->maximum.rx() = qMax(node->maximum.x(), p.x());
            node->maximum.ry() = qMax(node->maximum.y(), p.y());
        }
        return node;
    }

    node->type = BVHNode::Split;

    QPoint minimum, maximum;
    minimum = maximum = elements[0]->middle;

    for (int i = 1; i < elementCount; ++i) {
        const QPoint &p = elements[i]->middle;
        minimum.rx() = qMin(minimum.x(), p.x());
        minimum.ry() = qMin(minimum.y(), p.y());
        maximum.rx() = qMax(maximum.x(), p.x());
        maximum.ry() = qMax(maximum.y(), p.y());
    }

    int comp, pivot;
    if (maximum.x() - minimum.x() > maximum.y() - minimum.y()) {
        comp = 0;
        pivot = (maximum.x() + minimum.x()) >> 1;
    } else {
        comp = 1;
        pivot = (maximum.y() + minimum.y()) >> 1;
    }

    int lo = 0;
    int hi = elementCount - 1;
    while (lo < hi) {
        while (lo < hi && (&elements[lo]->middle.rx())[comp] <= pivot)
            ++lo;
        while (lo < hi && (&elements[hi]->middle.rx())[comp] > pivot)
            --hi;
        if (lo < hi)
            qSwap(elements[lo], elements[hi]);
    }

    if (lo == elementCount) {
        // All points are the same.
        Q_ASSERT(minimum.x() == maximum.x() && minimum.y() == maximum.y());
        lo = elementCount >> 1;
    }

    node->left = buildTree(elements, lo);
    node->right = buildTree(elements + lo, elementCount - lo);

    const BVHNode *left = node->left;
    const BVHNode *right = node->right;
    node->minimum.rx() = qMin(left->minimum.x(), right->minimum.x());
    node->minimum.ry() = qMin(left->minimum.y(), right->minimum.y());
    node->maximum.rx() = qMax(left->maximum.x(), right->maximum.x());
    node->maximum.ry() = qMax(left->maximum.y(), right->maximum.y());

    return node;
}

bool PathSimplifier::intersectNodes(QDataBuffer<Element *> &elements, BVHNode *elementNode,
                                    BVHNode *treeNode)
{
    if (elementNode->minimum.x() >= treeNode->maximum.x()
        || elementNode->minimum.y() >= treeNode->maximum.y()
        || elementNode->maximum.x() <= treeNode->minimum.x()
        || elementNode->maximum.y() <= treeNode->minimum.y())
    {
        return false;
    }

    Q_ASSERT(elementNode->type == BVHNode::Leaf);
    Element *element = elementNode->element;
    Q_ASSERT(!element->processed);

    if (treeNode->type == BVHNode::Leaf) {
        Element *nodeElement = treeNode->element;
        if (!nodeElement->processed)
            return false;

        if (treeNode->element == elementNode->element)
            return false;

        if (equalElements(treeNode->element, elementNode->element))
            return false; // element doesn't split itself.

        if (element->degree == Element::Line && nodeElement->degree == Element::Line) {
            const QPoint &u1 = m_points->at(element->indices[0]);
            const QPoint &u2 = m_points->at(element->indices[1]);
            const QPoint &v1 = m_points->at(nodeElement->indices[0]);
            const QPoint &v2 = m_points->at(nodeElement->indices[1]);
            IntersectionPoint intersection = intersectionPoint(u1, u2, v1, v2);
            if (!intersection.isValid())
                return false;

            Q_ASSERT(intersection.x.integer >= qMin(u1.x(), u2.x()));
            Q_ASSERT(intersection.y.integer >= qMin(u1.y(), u2.y()));
            Q_ASSERT(intersection.x.integer >= qMin(v1.x(), v2.x()));
            Q_ASSERT(intersection.y.integer >= qMin(v1.y(), v2.y()));

            Q_ASSERT(intersection.x.integer <= qMax(u1.x(), u2.x()));
            Q_ASSERT(intersection.y.integer <= qMax(u1.y(), u2.y()));
            Q_ASSERT(intersection.x.integer <= qMax(v1.x(), v2.x()));
            Q_ASSERT(intersection.y.integer <= qMax(v1.y(), v2.y()));

            m_points->add(intersection.round());
            splitLineAt(elements, treeNode, m_points->size() - 1, !intersection.isAccurate());
            return splitLineAt(elements, elementNode, m_points->size() - 1, false);
        } else {
            QVarLengthArray<QPoint, 12> axes;
            appendSeparatingAxes(axes, elementNode->element);
            appendSeparatingAxes(axes, treeNode->element);
            for (int i = 0; i < axes.size(); ++i) {
                QPair<int, int> range1 = calculateSeparatingAxisRange(axes.at(i), elementNode->element);
                QPair<int, int> range2 = calculateSeparatingAxisRange(axes.at(i), treeNode->element);
                if (range1.first >= range2.second || range1.second <= range2.first) {
                    return false; // Separating axis found.
                }
            }
            // Bounding areas overlap.
            if (nodeElement->degree > Element::Line)
                splitCurve(elements, treeNode);
            if (element->degree > Element::Line) {
                splitCurve(elements, elementNode);
            } else {
                // The element was not split, so it can be processed further.
                if (intersectNodes(elements, elementNode, treeNode->left))
                    return true;
                if (intersectNodes(elements, elementNode, treeNode->right))
                    return true;
                return false;
            }
            return true;
        }
    } else {
        if (intersectNodes(elements, elementNode, treeNode->left))
            return true;
        if (intersectNodes(elements, elementNode, treeNode->right))
            return true;
        return false;
    }
}

bool PathSimplifier::equalElements(const Element *e1, const Element *e2)
{
    Q_ASSERT(e1 != e2);
    if (e1->degree != e2->degree)
        return false;

    // Possibly equal and in the same direction.
    bool equalSame = true;
    for (int i = 0; i <= e1->degree; ++i)
        equalSame &= m_points->at(e1->indices[i]) == m_points->at(e2->indices[i]);

    // Possibly equal and in opposite directions.
    bool equalOpposite = true;
    for (int i = 0; i <= e1->degree; ++i)
        equalOpposite &= m_points->at(e1->indices[e1->degree - i]) == m_points->at(e2->indices[i]);

    return equalSame || equalOpposite;
}

bool PathSimplifier::splitLineAt(QDataBuffer<Element *> &elements, BVHNode *node,
                                 quint32 pointIndex, bool processAgain)
{
    Q_ASSERT(node->type == BVHNode::Leaf);
    Element *element = node->element;
    Q_ASSERT(element->degree == Element::Line);
    const QPoint &u = m_points->at(element->indices[0]);
    const QPoint &v = m_points->at(element->indices[1]);
    const QPoint &p = m_points->at(pointIndex);
    if (u == p || v == p)
        return false; // No split needed.

    if (processAgain)
        element->processed = false; // Needs to be processed again.

    Element *first = node->element;
    Element *second = m_elementAllocator.newElement();
    *second = *first;
    first->indices[1] = second->indices[0] = pointIndex;
    first->middle.rx() = (u.x() + p.x()) >> 1;
    first->middle.ry() = (u.y() + p.y()) >> 1;
    second->middle.rx() = (v.x() + p.x()) >> 1;
    second->middle.ry() = (v.y() + p.y()) >> 1;
    m_elements.add(second);

    BVHNode *left = m_bvh.newNode();
    BVHNode *right = m_bvh.newNode();
    left->type = right->type = BVHNode::Leaf;
    left->element = first;
    right->element = second;
    left->minimum = right->minimum = node->minimum;
    left->maximum = right->maximum = node->maximum;
    if (u.x() < v.x())
        left->maximum.rx() = right->minimum.rx() = p.x();
    else
        left->minimum.rx() = right->maximum.rx() = p.x();
    if (u.y() < v.y())
        left->maximum.ry() = right->minimum.ry() = p.y();
    else
        left->minimum.ry() = right->maximum.ry() = p.y();
    left->element->bvhNode = left;
    right->element->bvhNode = right;

    node->type = BVHNode::Split;
    node->left = left;
    node->right = right;

    if (!first->processed) {
        elements.add(left->element);
        elements.add(right->element);
    }
    return true;
}

void PathSimplifier::appendSeparatingAxes(QVarLengthArray<QPoint, 12> &axes, Element *element)
{
    switch (element->degree) {
    case Element::Cubic:
        {
            const QPoint &u = m_points->at(element->indices[0]);
            const QPoint &v = m_points->at(element->indices[1]);
            const QPoint &w = m_points->at(element->indices[2]);
            const QPoint &q = m_points->at(element->indices[3]);
            QPoint ns[] = {
                QPoint(u.y() - v.y(), v.x() - u.x()),
                QPoint(v.y() - w.y(), w.x() - v.x()),
                QPoint(w.y() - q.y(), q.x() - w.x()),
                QPoint(q.y() - u.y(), u.x() - q.x()),
                QPoint(u.y() - w.y(), w.x() - u.x()),
                QPoint(v.y() - q.y(), q.x() - v.x())
            };
            for (int i = 0; i < 6; ++i) {
                if (ns[i].x() || ns[i].y())
                    axes.append(ns[i]);
            }
        }
        break;
    case Element::Quadratic:
        {
            const QPoint &u = m_points->at(element->indices[0]);
            const QPoint &v = m_points->at(element->indices[1]);
            const QPoint &w = m_points->at(element->indices[2]);
            QPoint ns[] = {
                QPoint(u.y() - v.y(), v.x() - u.x()),
                QPoint(v.y() - w.y(), w.x() - v.x()),
                QPoint(w.y() - u.y(), u.x() - w.x())
            };
            for (int i = 0; i < 3; ++i) {
                if (ns[i].x() || ns[i].y())
                    axes.append(ns[i]);
            }
        }
        break;
    case Element::Line:
        {
            const QPoint &u = m_points->at(element->indices[0]);
            const QPoint &v = m_points->at(element->indices[1]);
            QPoint n(u.y() - v.y(), v.x() - u.x());
            if (n.x() || n.y())
                axes.append(n);
        }
        break;
    default:
        Q_ASSERT_X(0, "QSGPathSimplifier::appendSeparatingAxes", "Unexpected element type.");
        break;
    }
}

QPair<int, int> PathSimplifier::calculateSeparatingAxisRange(const QPoint &axis, Element *element)
{
    QPair<int, int> range(0x7fffffff, -0x7fffffff);
    for (int i = 0; i <= element->degree; ++i) {
        const QPoint &p = m_points->at(element->indices[i]);
        int dist = dot(axis, p);
        range.first = qMin(range.first, dist);
        range.second = qMax(range.second, dist);
    }
    return range;
}

void PathSimplifier::splitCurve(QDataBuffer<Element *> &elements, BVHNode *node)
{
    Q_ASSERT(node->type == BVHNode::Leaf);

    Element *first = node->element;
    Element *second = m_elementAllocator.newElement();
    *second = *first;
    m_elements.add(second);
    Q_ASSERT(first->degree > Element::Line);

    bool accurate = true;
    const QPoint &u = m_points->at(first->indices[0]);
    const QPoint &v = m_points->at(first->indices[1]);
    const QPoint &w = m_points->at(first->indices[2]);

    if (first->degree == Element::Quadratic) {
        QPoint pts[3];
        accurate = splitQuadratic(u, v, w, pts);
        int pointIndex = m_points->size();
        m_points->add(pts[1]);
        accurate &= setElementToQuadratic(first, first->indices[0], pts[0], pointIndex);
        accurate &= setElementToQuadratic(second, pointIndex, pts[2], second->indices[2]);
    } else {
        Q_ASSERT(first->degree == Element::Cubic);
        const QPoint &q = m_points->at(first->indices[3]);
        QPoint pts[5];
        accurate = splitCubic(u, v, w, q, pts);
        int pointIndex = m_points->size();
        m_points->add(pts[2]);
        accurate &= setElementToCubic(first, first->indices[0], pts[0], pts[1], pointIndex);
        accurate &= setElementToCubic(second, pointIndex, pts[3], pts[4], second->indices[3]);
    }

    if (!accurate)
        first->processed = second->processed = false; // Needs to be processed again.

    BVHNode *left = m_bvh.newNode();
    BVHNode *right = m_bvh.newNode();
    left->type = right->type = BVHNode::Leaf;
    left->element = first;
    right->element = second;

    left->minimum.rx() = left->minimum.ry() = right->minimum.rx() = right->minimum.ry() = INT_MAX;
    left->maximum.rx() = left->maximum.ry() = right->maximum.rx() = right->maximum.ry() = INT_MIN;

    for (int i = 0; i <= first->degree; ++i) {
        QPoint &p = m_points->at(first->indices[i]);
        left->minimum.rx() = qMin(left->minimum.x(), p.x());
        left->minimum.ry() = qMin(left->minimum.y(), p.y());
        left->maximum.rx() = qMax(left->maximum.x(), p.x());
        left->maximum.ry() = qMax(left->maximum.y(), p.y());
    }
    for (int i = 0; i <= second->degree; ++i) {
        QPoint &p = m_points->at(second->indices[i]);
        right->minimum.rx() = qMin(right->minimum.x(), p.x());
        right->minimum.ry() = qMin(right->minimum.y(), p.y());
        right->maximum.rx() = qMax(right->maximum.x(), p.x());
        right->maximum.ry() = qMax(right->maximum.y(), p.y());
    }
    left->element->bvhNode = left;
    right->element->bvhNode = right;

    node->type = BVHNode::Split;
    node->left = left;
    node->right = right;

    if (!first->processed) {
        elements.add(left->element);
        elements.add(right->element);
    }
}

bool PathSimplifier::setElementToQuadratic(Element *element, quint32 pointIndex1,
                                           const QPoint &ctrl, quint32 pointIndex2)
{
    const QPoint &p1 = m_points->at(pointIndex1);
    const QPoint &p2 = m_points->at(pointIndex2);
    if (flattenQuadratic(p1, ctrl, p2)) {
        // Insert line.
        element->degree = Element::Line;
        element->indices[0] = pointIndex1;
        element->indices[1] = pointIndex2;
        element->middle.rx() = (p1.x() + p2.x()) >> 1;
        element->middle.ry() = (p1.y() + p2.y()) >> 1;
        return false;
    } else {
        // Insert bezier.
        element->degree = Element::Quadratic;
        element->indices[0] = pointIndex1;
        element->indices[1] = m_points->size();
        element->indices[2] = pointIndex2;
        element->middle.rx() = (p1.x() + ctrl.x() + p2.x()) / 3;
        element->middle.ry() = (p1.y() + ctrl.y() + p2.y()) / 3;
        m_points->add(ctrl);
        return true;
    }
}

bool PathSimplifier::setElementToCubic(Element *element, quint32 pointIndex1, const QPoint &v,
                                       const QPoint &w, quint32 pointIndex2)
{
    const QPoint &u = m_points->at(pointIndex1);
    const QPoint &q = m_points->at(pointIndex2);
    if (flattenCubic(u, v, w, q)) {
        // Insert line.
        element->degree = Element::Line;
        element->indices[0] = pointIndex1;
        element->indices[1] = pointIndex2;
        element->middle.rx() = (u.x() + q.x()) >> 1;
        element->middle.ry() = (u.y() + q.y()) >> 1;
        return false;
    } else {
        // Insert bezier.
        element->degree = Element::Cubic;
        element->indices[0] = pointIndex1;
        element->indices[1] = m_points->size();
        element->indices[2] = m_points->size() + 1;
        element->indices[3] = pointIndex2;
        element->middle.rx() = (u.x() + v.x() + w.x() + q.x()) >> 2;
        element->middle.ry() = (u.y() + v.y() + w.y() + q.y()) >> 2;
        m_points->add(v);
        m_points->add(w);
        return true;
    }
}

void PathSimplifier::setElementToCubicAndSimplify(Element *element, quint32 pointIndex1,
                                                  const QPoint &v, const QPoint &w,
                                                  quint32 pointIndex2)
{
    const QPoint &u = m_points->at(pointIndex1);
    const QPoint &q = m_points->at(pointIndex2);
    if (flattenCubic(u, v, w, q)) {
        // Insert line.
        element->degree = Element::Line;
        element->indices[0] = pointIndex1;
        element->indices[1] = pointIndex2;
        element->middle.rx() = (u.x() + q.x()) >> 1;
        element->middle.ry() = (u.y() + q.y()) >> 1;
        return;
    }

    bool intersecting = (u == q) || intersectionPoint(u, v, w, q).isValid();
    if (!intersecting) {
        // Insert bezier.
        element->degree = Element::Cubic;
        element->indices[0] = pointIndex1;
        element->indices[1] = m_points->size();
        element->indices[2] = m_points->size() + 1;
        element->indices[3] = pointIndex2;
        element->middle.rx() = (u.x() + v.x() + w.x() + q.x()) >> 2;
        element->middle.ry() = (u.y() + v.y() + w.y() + q.y()) >> 2;
        m_points->add(v);
        m_points->add(w);
        return;
    }

    QPoint pts[5];
    splitCubic(u, v, w, q, pts);
    int pointIndex = m_points->size();
    m_points->add(pts[2]);
    Element *element2 = m_elementAllocator.newElement();
    m_elements.add(element2);
    setElementToCubicAndSimplify(element, pointIndex1, pts[0], pts[1], pointIndex);
    setElementToCubicAndSimplify(element2, pointIndex, pts[3], pts[4], pointIndex2);
}

PathSimplifier::RBNode *PathSimplifier::findElementLeftOf(const Element *element,
                                                          const QPair<RBNode *, RBNode *> &bounds)
{
    if (!m_elementList.root)
        return 0;
    RBNode *current = bounds.first;
    Q_ASSERT(!current || !elementIsLeftOf(element, current->data));
    if (!current)
        current = m_elementList.front(m_elementList.root);
    Q_ASSERT(current);
    RBNode *result = 0;
    while (current != bounds.second && !elementIsLeftOf(element, current->data)) {
        result = current;
        current = m_elementList.next(current);
    }
    return result;
}

bool PathSimplifier::elementIsLeftOf(const Element *left, const Element *right)
{
    const QPoint &leftU = m_points->at(left->upperIndex());
    const QPoint &leftL = m_points->at(left->lowerIndex());
    const QPoint &rightU = m_points->at(right->upperIndex());
    const QPoint &rightL = m_points->at(right->lowerIndex());
    Q_ASSERT(leftL >= rightU && rightL >= leftU);
    if (leftU.x() < qMin(rightL.x(), rightU.x()))
        return true;
    if (leftU.x() > qMax(rightL.x(), rightU.x()))
        return false;
    int d = pointDistanceFromLine(leftU, rightL, rightU);
    // d < 0: left, d > 0: right, d == 0: on top
    if (d == 0) {
        d = pointDistanceFromLine(leftL, rightL, rightU);
        if (d == 0) {
            if (right->degree > Element::Line) {
                d = pointDistanceFromLine(leftL, rightL, m_points->at(right->indices[1]));
                if (d == 0)
                    d = pointDistanceFromLine(leftL, rightL, m_points->at(right->indices[2]));
            } else if (left->degree > Element::Line) {
                d = pointDistanceFromLine(m_points->at(left->indices[1]), rightL, rightU);
                if (d == 0)
                    d = pointDistanceFromLine(m_points->at(left->indices[2]), rightL, rightU);
            }
        }
    }
    return d < 0;
}

QPair<PathSimplifier::RBNode *, PathSimplifier::RBNode *> PathSimplifier::outerBounds(const QPoint &point)
{
    RBNode *current = m_elementList.root;
    QPair<RBNode *, RBNode *> result(0, 0);

    while (current) {
        const Element *element = current->data;
        Q_ASSERT(element->edgeNode == current);
        const QPoint &v1 = m_points->at(element->lowerIndex());
        const QPoint &v2 = m_points->at(element->upperIndex());
        Q_ASSERT(point >= v2 && point <= v1);
        if (point == v1 || point == v2)
            break;
        int d = pointDistanceFromLine(point, v1, v2);
        if (d == 0) {
            if (element->degree == Element::Line)
                break;
            d = pointDistanceFromLine(point, v1, m_points->at(element->indices[1]));
            if (d == 0)
                d = pointDistanceFromLine(point, v1, m_points->at(element->indices[2]));
            Q_ASSERT(d != 0);
        }
        if (d < 0) {
            result.second = current;
            current = current->left;
        } else {
            result.first = current;
            current = current->right;
        }
    }

    if (!current)
        return result;

    RBNode *mid = current;

    current = mid->left;
    while (current) {
        const Element *element = current->data;
        Q_ASSERT(element->edgeNode == current);
        const QPoint &v1 = m_points->at(element->lowerIndex());
        const QPoint &v2 = m_points->at(element->upperIndex());
        Q_ASSERT(point >= v2 && point <= v1);
        bool equal = (point == v1 || point == v2);
        if (!equal) {
            int d = pointDistanceFromLine(point, v1, v2);
            Q_ASSERT(d >= 0);
            equal = (d == 0 && element->degree == Element::Line);
        }
        if (equal) {
            current = current->left;
        } else {
            result.first = current;
            current = current->right;
        }
    }

    current = mid->right;
    while (current) {
        const Element *element = current->data;
        Q_ASSERT(element->edgeNode == current);
        const QPoint &v1 = m_points->at(element->lowerIndex());
        const QPoint &v2 = m_points->at(element->upperIndex());
        Q_ASSERT(point >= v2 && point <= v1);
        bool equal = (point == v1 || point == v2);
        if (!equal) {
            int d = pointDistanceFromLine(point, v1, v2);
            Q_ASSERT(d <= 0);
            equal = (d == 0 && element->degree == Element::Line);
        }
        if (equal) {
            current = current->right;
        } else {
            result.second = current;
            current = current->left;
        }
    }

    return result;
}

inline bool PathSimplifier::flattenQuadratic(const QPoint &u, const QPoint &v, const QPoint &w)
{
    QPoint deltas[2] = { v - u, w - v };
    int d = qAbs(cross(deltas[0], deltas[1]));
    int l = qAbs(deltas[0].x()) + qAbs(deltas[0].y()) + qAbs(deltas[1].x()) + qAbs(deltas[1].y());
    return d < (Q_FIXED_POINT_SCALE * Q_FIXED_POINT_SCALE * 3 / 2) || l <= Q_FIXED_POINT_SCALE * 2;
}

inline bool PathSimplifier::flattenCubic(const QPoint &u, const QPoint &v,
                                         const QPoint &w, const QPoint &q)
{
    QPoint deltas[] = { v - u, w - v, q - w, q - u };
    int d = qAbs(cross(deltas[0], deltas[1])) + qAbs(cross(deltas[1], deltas[2]))
            + qAbs(cross(deltas[0], deltas[3])) + qAbs(cross(deltas[3], deltas[2]));
    int l = qAbs(deltas[0].x()) + qAbs(deltas[0].y()) + qAbs(deltas[1].x()) + qAbs(deltas[1].y())
            + qAbs(deltas[2].x()) + qAbs(deltas[2].y());
    return d < (Q_FIXED_POINT_SCALE * Q_FIXED_POINT_SCALE * 3) || l <= Q_FIXED_POINT_SCALE * 2;
}

inline bool PathSimplifier::splitQuadratic(const QPoint &u, const QPoint &v,
                                           const QPoint &w, QPoint *result)
{
    result[0] = u + v;
    result[2] = v + w;
    result[1] = result[0] + result[2];
    bool accurate = ((result[0].x() | result[0].y() | result[2].x() | result[2].y()) & 1) == 0
                    && ((result[1].x() | result[1].y()) & 3) == 0;
    result[0].rx() >>= 1;
    result[0].ry() >>= 1;
    result[1].rx() >>= 2;
    result[1].ry() >>= 2;
    result[2].rx() >>= 1;
    result[2].ry() >>= 1;
    return accurate;
}

inline bool PathSimplifier::splitCubic(const QPoint &u, const QPoint &v,
                                       const QPoint &w, const QPoint &q, QPoint *result)
{
    result[0] = u + v;
    result[2] = v + w;
    result[4] = w + q;
    result[1] = result[0] + result[2];
    result[3] = result[2] + result[4];
    result[2] = result[1] + result[3];
    bool accurate = ((result[0].x() | result[0].y() | result[4].x() | result[4].y()) & 1) == 0
                    && ((result[1].x() | result[1].y() | result[3].x() | result[3].y()) & 3) == 0
                    && ((result[2].x() | result[2].y()) & 7) == 0;
    result[0].rx() >>= 1;
    result[0].ry() >>= 1;
    result[1].rx() >>= 2;
    result[1].ry() >>= 2;
    result[2].rx() >>= 3;
    result[2].ry() >>= 3;
    result[3].rx() >>= 2;
    result[3].ry() >>= 2;
    result[4].rx() >>= 1;
    result[4].ry() >>= 1;
    return accurate;
}

inline void PathSimplifier::subDivQuadratic(const QPoint &u, const QPoint &v, const QPoint &w)
{
    if (flattenQuadratic(u, v, w))
        return;
    QPoint pts[3];
    splitQuadratic(u, v, w, pts);
    subDivQuadratic(u, pts[0], pts[1]);
    m_indices->add(m_points->size());
    m_points->add(pts[1]);
    subDivQuadratic(pts[1], pts[2], w);
}

inline void PathSimplifier::subDivCubic(const QPoint &u, const QPoint &v,
                                        const QPoint &w, const QPoint &q)
{
    if (flattenCubic(u, v, w, q))
        return;
    QPoint pts[5];
    splitCubic(u, v, w, q, pts);
    subDivCubic(u, pts[0], pts[1], pts[2]);
    m_indices->add(m_points->size());
    m_points->add(pts[2]);
    subDivCubic(pts[2], pts[3], pts[4], q);
}

void PathSimplifier::sortEvents(Event *events, int count)
{
    // Bucket sort + insertion sort.
    Q_ASSERT(count > 0);
    QDataBuffer<Event> buffer(count);
    buffer.resize(count);
    QScopedArrayPointer<int> bins(new int[count]);
    int counts[0x101];
    memset(counts, 0, sizeof(counts));

    int minimum, maximum;
    minimum = maximum = events[0].point.y();
    for (int i = 1; i < count; ++i) {
        minimum = qMin(minimum, events[i].point.y());
        maximum = qMax(maximum, events[i].point.y());
    }

    for (int i = 0; i < count; ++i) {
        bins[i] = ((maximum - events[i].point.y()) << 8) / (maximum - minimum + 1);
        Q_ASSERT(bins[i] >= 0 && bins[i] < 0x100);
        ++counts[bins[i]];
    }

    for (int i = 1; i < 0x100; ++i)
        counts[i] += counts[i - 1];
    counts[0x100] = counts[0xff];
    Q_ASSERT(counts[0x100] == count);

    for (int i = 0; i < count; ++i)
        buffer.at(--counts[bins[i]]) = events[i];

    int j = 0;
    for (int i = 0; i < 0x100; ++i) {
        for (; j < counts[i + 1]; ++j) {
            int k = j;
            while (k > 0 && (buffer.at(j) < events[k - 1])) {
                events[k] = events[k - 1];
                --k;
            }
            events[k] = buffer.at(j);
        }
    }
}

} // end anonymous namespace


void qSimplifyPath(const QVectorPath &path, QDataBuffer<QPoint> &vertices,
                   QDataBuffer<quint32> &indices, const QTransform &matrix)
{
    PathSimplifier(path, vertices, indices, matrix);
}

void qSimplifyPath(const QPainterPath &path, QDataBuffer<QPoint> &vertices,
                   QDataBuffer<quint32> &indices, const QTransform &matrix)
{
    qSimplifyPath(qtVectorPathForPath(path), vertices, indices, matrix);
}


QT_END_NAMESPACE

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

#include "qtriangulator_p.h"

#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/private/qbezier_p.h>
#include <QtGui/private/qdatabuffer_p.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qqueue.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>
#include <QtCore/qalgorithms.h>

#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>
#include <private/qrbtree_p.h>

#include <math.h>

QT_BEGIN_NAMESPACE

//#define Q_TRIANGULATOR_DEBUG

#define Q_FIXED_POINT_SCALE 32

template<typename T>
struct QVertexSet
{
    inline QVertexSet() { }
    inline QVertexSet(const QVertexSet<T> &other) : vertices(other.vertices), indices(other.indices) { }
    QVertexSet<T> &operator = (const QVertexSet<T> &other) {vertices = other.vertices; indices = other.indices; return *this;}

    // The vertices of a triangle are given by: (x[i[n]], y[i[n]]), (x[j[n]], y[j[n]]), (x[k[n]], y[k[n]]), n = 0, 1, ...
    QVector<qreal> vertices; // [x[0], y[0], x[1], y[1], x[2], ...]
    QVector<T> indices; // [i[0], j[0], k[0], i[1], j[1], k[1], i[2], ...]
};

//============================================================================//
//                                 QFraction                                  //
//============================================================================//

// Fraction must be in the range [0, 1)
struct QFraction
{
    // Comparison operators must not be called on invalid fractions.
    inline bool operator < (const QFraction &other) const;
    inline bool operator == (const QFraction &other) const;
    inline bool operator != (const QFraction &other) const {return !(*this == other);}
    inline bool operator > (const QFraction &other) const {return other < *this;}
    inline bool operator >= (const QFraction &other) const {return !(*this < other);}
    inline bool operator <= (const QFraction &other) const {return !(*this > other);}

    inline bool isValid() const {return denominator != 0;}

    // numerator and denominator must not have common denominators.
    quint64 numerator, denominator;
};

static inline quint64 gcd(quint64 x, quint64 y)
{
    while (y != 0) {
        quint64 z = y;
        y = x % y;
        x = z;
    }
    return x;
}

static inline int compare(quint64 a, quint64 b)
{
    return (a > b) - (a < b);
}

// Compare a/b with c/d.
// Return negative if less, 0 if equal, positive if greater.
// a < b, c < d
static int qCompareFractions(quint64 a, quint64 b, quint64 c, quint64 d)
{
    const quint64 LIMIT = Q_UINT64_C(0x100000000);
    for (;;) {
        // If the products 'ad' and 'bc' fit into 64 bits, they can be directly compared.
        if (b < LIMIT && d < LIMIT)
            return compare(a * d, b * c);

        if (a == 0 || c == 0)
            return compare(a, c);

        // a/b < c/d  <=>  d/c < b/a
        quint64 b_div_a = b / a;
        quint64 d_div_c = d / c;
        if (b_div_a != d_div_c)
            return compare(d_div_c, b_div_a);

        // floor(d/c) == floor(b/a)
        // frac(d/c) < frac(b/a) ?
        // frac(x/y) = (x%y)/y
        d -= d_div_c * c; //d %= c;
        b -= b_div_a * a; //b %= a;
        qSwap(a, d);
        qSwap(b, c);
    }
}

// Fraction must be in the range [0, 1)
// Assume input is valid.
static QFraction qFraction(quint64 n, quint64 d) {
    QFraction result;
    if (n == 0) {
        result.numerator = 0;
        result.denominator = 1;
    } else {
        quint64 g = gcd(n, d);
        result.numerator = n / g;
        result.denominator = d / g;
    }
    return result;
}

inline bool QFraction::operator < (const QFraction &other) const
{
    return qCompareFractions(numerator, denominator, other.numerator, other.denominator) < 0;
}

inline bool QFraction::operator == (const QFraction &other) const
{
    return numerator == other.numerator && denominator == other.denominator;
}

//============================================================================//
//                                 QPodPoint                                  //
//============================================================================//

struct QPodPoint
{
    inline bool operator < (const QPodPoint &other) const
    {
        if (y != other.y)
            return y < other.y;
        return x < other.x;
    }

    inline bool operator > (const QPodPoint &other) const {return other < *this;}
    inline bool operator <= (const QPodPoint &other) const {return !(*this > other);}
    inline bool operator >= (const QPodPoint &other) const {return !(*this < other);}
    inline bool operator == (const QPodPoint &other) const {return x == other.x && y == other.y;}
    inline bool operator != (const QPodPoint &other) const {return x != other.x || y != other.y;}

    inline QPodPoint &operator += (const QPodPoint &other) {x += other.x; y += other.y; return *this;}
    inline QPodPoint &operator -= (const QPodPoint &other) {x -= other.x; y -= other.y; return *this;}
    inline QPodPoint operator + (const QPodPoint &other) const {QPodPoint result = {x + other.x, y + other.y}; return result;}
    inline QPodPoint operator - (const QPodPoint &other) const {QPodPoint result = {x - other.x, y - other.y}; return result;}

    int x;
    int y;
};

static inline qint64 qCross(const QPodPoint &u, const QPodPoint &v)
{
    return qint64(u.x) * qint64(v.y) - qint64(u.y) * qint64(v.x);
}

#ifdef Q_TRIANGULATOR_DEBUG
static inline qint64 qDot(const QPodPoint &u, const QPodPoint &v)
{
    return qint64(u.x) * qint64(v.x) + qint64(u.y) * qint64(v.y);
}
#endif

// Return positive value if 'p' is to the right of the line 'v1'->'v2', negative if left of the
// line and zero if exactly on the line.
// The returned value is the z-component of the qCross product between 'v2-v1' and 'p-v1',
// which is twice the signed area of the triangle 'p'->'v1'->'v2' (positive for CW order).
static inline qint64 qPointDistanceFromLine(const QPodPoint &p, const QPodPoint &v1, const QPodPoint &v2)
{
    return qCross(v2 - v1, p - v1);
}

static inline bool qPointIsLeftOfLine(const QPodPoint &p, const QPodPoint &v1, const QPodPoint &v2)
{
    return QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(p, v1, v2) < 0;
}

//============================================================================//
//                             QIntersectionPoint                             //
//============================================================================//

struct QIntersectionPoint
{
    inline bool isValid() const {return xOffset.isValid() && yOffset.isValid();}
    QPodPoint round() const;
    inline bool isAccurate() const {return xOffset.numerator == 0 && yOffset.numerator == 0;}
    bool operator < (const QIntersectionPoint &other) const;
    bool operator == (const QIntersectionPoint &other) const;
    inline bool operator != (const QIntersectionPoint &other) const {return !(*this == other);}
    inline bool operator > (const QIntersectionPoint &other) const {return other < *this;}
    inline bool operator >= (const QIntersectionPoint &other) const {return !(*this < other);}
    inline bool operator <= (const QIntersectionPoint &other) const {return !(*this > other);}
    bool isOnLine(const QPodPoint &u, const QPodPoint &v) const;

    QPodPoint upperLeft;
    QFraction xOffset;
    QFraction yOffset;
};

static inline QIntersectionPoint qIntersectionPoint(const QPodPoint &point)
{
    // upperLeft = point, xOffset = 0/1, yOffset = 0/1.
    QIntersectionPoint p = {{point.x, point.y}, {0, 1}, {0, 1}};
    return p;
}

static QIntersectionPoint qIntersectionPoint(const QPodPoint &u1, const QPodPoint &u2, const QPodPoint &v1, const QPodPoint &v2)
{
    QIntersectionPoint result = {{0, 0}, {0, 0}, {0, 0}};

    QPodPoint u = u2 - u1;
    QPodPoint v = v2 - v1;
    qint64 d1 = qCross(u, v1 - u1);
    qint64 d2 = qCross(u, v2 - u1);
    qint64 det = d2 - d1;
    qint64 d3 = qCross(v, u1 - v1);
    qint64 d4 = d3 - det; //qCross(v, u2 - v1);

    // Check that the math is correct.
    Q_ASSERT(d4 == qCross(v, u2 - v1));

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

    // Assuming 21 bits per vector component.
    // TODO: Make code path for 31 bits per vector component.
    if (v.x >= 0) {
        result.upperLeft.x = v1.x + (-v.x * d1) / det;
        result.xOffset = qFraction(quint64(-v.x * d1) % quint64(det), quint64(det));
    } else {
        result.upperLeft.x = v2.x + (-v.x * d2) / det;
        result.xOffset = qFraction(quint64(-v.x * d2) % quint64(det), quint64(det));
    }

    if (v.y >= 0) {
        result.upperLeft.y = v1.y + (-v.y * d1) / det;
        result.yOffset = qFraction(quint64(-v.y * d1) % quint64(det), quint64(det));
    } else {
        result.upperLeft.y = v2.y + (-v.y * d2) / det;
        result.yOffset = qFraction(quint64(-v.y * d2) % quint64(det), quint64(det));
    }

    Q_ASSERT(result.xOffset.isValid());
    Q_ASSERT(result.yOffset.isValid());
    return result;
}

QPodPoint QIntersectionPoint::round() const
{
    QPodPoint result = upperLeft;
    if (2 * xOffset.numerator >= xOffset.denominator)
        ++result.x;
    if (2 * yOffset.numerator >= yOffset.denominator)
        ++result.y;
    return result;
}

bool QIntersectionPoint::operator < (const QIntersectionPoint &other) const
{
    if (upperLeft.y != other.upperLeft.y)
        return upperLeft.y < other.upperLeft.y;
    if (yOffset != other.yOffset)
        return yOffset < other.yOffset;
    if (upperLeft.x != other.upperLeft.x)
        return upperLeft.x < other.upperLeft.x;
    return xOffset < other.xOffset;
}

bool QIntersectionPoint::operator == (const QIntersectionPoint &other) const
{
    return upperLeft == other.upperLeft && xOffset == other.xOffset && yOffset == other.yOffset;
}

// Returns \c true if this point is on the infinite line passing through 'u' and 'v'.
bool QIntersectionPoint::isOnLine(const QPodPoint &u, const QPodPoint &v) const
{
    // TODO: Make code path for coordinates with more than 21 bits.
    const QPodPoint p = upperLeft - u;
    const QPodPoint q = v - u;
    bool isHorizontal = p.y == 0 && yOffset.numerator == 0;
    bool isVertical = p.x == 0 && xOffset.numerator == 0;
    if (isHorizontal && isVertical)
        return true;
    if (isHorizontal)
        return q.y == 0;
    if (q.y == 0)
        return false;
    if (isVertical)
        return q.x == 0;
    if (q.x == 0)
        return false;

    // At this point, 'p+offset' and 'q' cannot lie on the x or y axis.

    if (((q.x < 0) == (q.y < 0)) != ((p.x < 0) == (p.y < 0)))
        return false; // 'p + offset' and 'q' pass through different quadrants.

    // Move all coordinates into the first quadrant.
    quint64 nx, ny;
    if (p.x < 0)
        nx = quint64(-p.x) * xOffset.denominator - xOffset.numerator;
    else
        nx = quint64(p.x) * xOffset.denominator + xOffset.numerator;
    if (p.y < 0)
        ny = quint64(-p.y) * yOffset.denominator - yOffset.numerator;
    else
        ny = quint64(p.y) * yOffset.denominator + yOffset.numerator;

    return qFraction(quint64(qAbs(q.x)) * xOffset.denominator, quint64(qAbs(q.y)) * yOffset.denominator) == qFraction(nx, ny);
}

//============================================================================//
//                                  QMaxHeap                                  //
//============================================================================//

template <class T>
class QMaxHeap
{
public:
    QMaxHeap() : m_data(0) {}
    inline int size() const {return m_data.size();}
    inline bool empty() const {return m_data.isEmpty();}
    inline bool isEmpty() const {return m_data.isEmpty();}
    void push(const T &x);
    T pop();
    inline const T &top() const {return m_data.first();}
private:
    static inline int parent(int i) {return (i - 1) / 2;}
    static inline int left(int i) {return 2 * i + 1;}
    static inline int right(int i) {return 2 * i + 2;}

    QDataBuffer<T> m_data;
};

template <class T>
void QMaxHeap<T>::push(const T &x)
{
    int current = m_data.size();
    int parent = QMaxHeap::parent(current);
    m_data.add(x);
    while (current != 0 && m_data.at(parent) < x) {
        m_data.at(current) = m_data.at(parent);
        current = parent;
        parent = QMaxHeap::parent(current);
    }
    m_data.at(current) = x;
}

template <class T>
T QMaxHeap<T>::pop()
{
    T result = m_data.first();
    T back = m_data.last();
    m_data.pop_back();
    if (!m_data.isEmpty()) {
        int current = 0;
        for (;;) {
            int left = QMaxHeap::left(current);
            int right = QMaxHeap::right(current);
            if (left >= m_data.size())
                break;
            int greater = left;
            if (right < m_data.size() && m_data.at(left) < m_data.at(right))
                greater = right;
            if (m_data.at(greater) < back)
                break;
            m_data.at(current) = m_data.at(greater);
            current = greater;
        }
        m_data.at(current) = back;
    }
    return result;
}

//============================================================================//
//                                 QInt64Hash                                 //
//============================================================================//

// Copied from qhash.cpp
static const uchar prime_deltas[] = {
    0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3,  9, 25,  3,
    1, 21,  3, 21,  7, 15,  9,  5,  3, 29, 15,  0,  0,  0,  0,  0
};

// Copied from qhash.cpp
static inline int primeForNumBits(int numBits)
{
    return (1 << numBits) + prime_deltas[numBits];
}

static inline int primeForCount(int count)
{
    int low = 0;
    int high = 32;
    for (int i = 0; i < 5; ++i) {
        int mid = (high + low) / 2;
        if (count >= 1 << mid)
            low = mid;
        else
            high = mid;
    }
    return primeForNumBits(high);
}

// Hash set of quint64s. Elements cannot be removed without clearing the
// entire set. A value of -1 is used to mark unused entries.
class QInt64Set
{
public:
    inline QInt64Set(int capacity = 64);
    inline ~QInt64Set() {if (m_array) delete[] m_array;}
    inline bool isValid() const {return m_array;}
    void insert(quint64 key);
    bool contains(quint64 key) const;
    inline void clear();
private:
    bool rehash(int capacity);

    static const quint64 UNUSED;

    quint64 *m_array;
    int m_capacity;
    int m_count;
};

const quint64 QInt64Set::UNUSED = quint64(-1);

inline QInt64Set::QInt64Set(int capacity)
{
    m_capacity = primeForCount(capacity);
    m_array = new quint64[m_capacity];
    if (m_array)
        clear();
    else
        m_capacity = 0;
}

bool QInt64Set::rehash(int capacity)
{
    quint64 *oldArray = m_array;
    int oldCapacity = m_capacity;

    m_capacity = capacity;
    m_array = new quint64[m_capacity];
    if (m_array) {
        clear();
        if (oldArray) {
            for (int i = 0; i < oldCapacity; ++i) {
                if (oldArray[i] != UNUSED)
                    insert(oldArray[i]);
            }
            delete[] oldArray;
        }
        return true;
    } else {
        m_capacity = oldCapacity;
        m_array = oldArray;
        return false;
    }
}

void QInt64Set::insert(quint64 key)
{
    if (m_count > 3 * m_capacity / 4)
        rehash(primeForCount(2 * m_capacity));
    Q_ASSERT_X(m_array, "QInt64Hash<T>::insert", "Hash set not allocated.");
    int index = int(key % m_capacity);
    for (int i = 0; i < m_capacity; ++i) {
        index += i;
        if (index >= m_capacity)
            index -= m_capacity;
        if (m_array[index] == key)
            return;
        if (m_array[index] == UNUSED) {
            ++m_count;
            m_array[index] = key;
            return;
        }
    }
    Q_ASSERT_X(0, "QInt64Hash<T>::insert", "Hash set full.");
}

bool QInt64Set::contains(quint64 key) const
{
    Q_ASSERT_X(m_array, "QInt64Hash<T>::contains", "Hash set not allocated.");
    int index = int(key % m_capacity);
    for (int i = 0; i < m_capacity; ++i) {
        index += i;
        if (index >= m_capacity)
            index -= m_capacity;
        if (m_array[index] == key)
            return true;
        if (m_array[index] == UNUSED)
            return false;
    }
    return false;
}

inline void QInt64Set::clear()
{
    Q_ASSERT_X(m_array, "QInt64Hash<T>::clear", "Hash set not allocated.");
    for (int i = 0; i < m_capacity; ++i)
        m_array[i] = UNUSED;
    m_count = 0;
}

//============================================================================//
//                               QTriangulator                                //
//============================================================================//
template<typename T>
class QTriangulator
{
public:
    typedef QVarLengthArray<int, 6> ShortArray;

    //================================//
    // QTriangulator::ComplexToSimple //
    //================================//
    friend class ComplexToSimple;
    class ComplexToSimple
    {
    public:
        inline ComplexToSimple(QTriangulator<T> *parent) : m_parent(parent),
            m_edges(0), m_events(0), m_splits(0) { }
        void decompose();
    private:
        struct Edge
        {
            inline int &upper() {return pointingUp ? to : from;}
            inline int &lower() {return pointingUp ? from : to;}
            inline int upper() const {return pointingUp ? to : from;}
            inline int lower() const {return pointingUp ? from : to;}

            QRBTree<int>::Node *node;
            int from, to; // vertex
            int next, previous; // edge
            int winding;
            bool mayIntersect;
            bool pointingUp, originallyPointingUp;
        };

        struct Intersection
        {
            bool operator < (const Intersection &other) const {return other.intersectionPoint < intersectionPoint;}

            QIntersectionPoint intersectionPoint;
            int vertex;
            int leftEdge;
            int rightEdge;
        };

        struct Split
        {
            int vertex;
            int edge;
            bool accurate;
        };

        struct Event
        {
            enum Type {Upper, Lower};
            inline bool operator < (const Event &other) const;

            QPodPoint point;
            Type type;
            int edge;
        };

#ifdef Q_TRIANGULATOR_DEBUG
        friend class DebugDialog;
        friend class QTriangulator;
        class DebugDialog : public QDialog
        {
        public:
            DebugDialog(ComplexToSimple *parent, int currentVertex);
        protected:
            void paintEvent(QPaintEvent *);
            void wheelEvent(QWheelEvent *);
            void mouseMoveEvent(QMouseEvent *);
            void mousePressEvent(QMouseEvent *);
        private:
            ComplexToSimple *m_parent;
            QRectF m_window;
            QPoint m_lastMousePos;
            int m_vertex;
        };
#endif

        void initEdges();
        bool calculateIntersection(int left, int right);
        bool edgeIsLeftOfEdge(int leftEdgeIndex, int rightEdgeIndex) const;
        QRBTree<int>::Node *searchEdgeLeftOf(int edgeIndex) const;
        QRBTree<int>::Node *searchEdgeLeftOf(int edgeIndex, QRBTree<int>::Node *after) const;
        QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> bounds(const QPodPoint &point) const;
        QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> outerBounds(const QPodPoint &point) const;
        void splitEdgeListRange(QRBTree<int>::Node *leftmost, QRBTree<int>::Node *rightmost, int vertex, const QIntersectionPoint &intersectionPoint);
        void reorderEdgeListRange(QRBTree<int>::Node *leftmost, QRBTree<int>::Node *rightmost);
        void sortEdgeList(const QPodPoint eventPoint);
        void fillPriorityQueue();
        void calculateIntersections();
        int splitEdge(int splitIndex);
        bool splitEdgesAtIntersections();
        void insertEdgeIntoVectorIfWanted(ShortArray &orderedEdges, int i);
        void removeUnwantedEdgesAndConnect();
        void removeUnusedPoints();

        QTriangulator *m_parent;
        QDataBuffer<Edge> m_edges;
        QRBTree<int> m_edgeList;
        QDataBuffer<Event> m_events;
        QDataBuffer<Split> m_splits;
        QMaxHeap<Intersection> m_topIntersection;
        QInt64Set m_processedEdgePairs;
        int m_initialPointCount;
    };
#ifdef Q_TRIANGULATOR_DEBUG
    friend class ComplexToSimple::DebugDialog;
#endif

    //=================================//
    // QTriangulator::SimpleToMonotone //
    //=================================//
    friend class SimpleToMonotone;
    class SimpleToMonotone
    {
    public:
        inline SimpleToMonotone(QTriangulator<T> *parent) : m_parent(parent), m_edges(0), m_upperVertex(0) { }
        void decompose();
    private:
        enum VertexType {MergeVertex, EndVertex, RegularVertex, StartVertex, SplitVertex};

        struct Edge
        {
            QRBTree<int>::Node *node;
            int helper, twin, next, previous;
            T from, to;
            VertexType type;
            bool pointingUp;
            int upper() const {return (pointingUp ? to : from);}
            int lower() const {return (pointingUp ? from : to);}
        };

        friend class CompareVertices;
        class CompareVertices
        {
        public:
            CompareVertices(SimpleToMonotone *parent) : m_parent(parent) { }
            bool operator () (int i, int j) const;
        private:
            SimpleToMonotone *m_parent;
        };

        void setupDataStructures();
        void removeZeroLengthEdges();
        void fillPriorityQueue();
        bool edgeIsLeftOfEdge(int leftEdgeIndex, int rightEdgeIndex) const;
        // Returns the rightmost edge not to the right of the given edge.
        QRBTree<int>::Node *searchEdgeLeftOfEdge(int edgeIndex) const;
        // Returns the rightmost edge left of the given point.
        QRBTree<int>::Node *searchEdgeLeftOfPoint(int pointIndex) const;
        void classifyVertex(int i);
        void classifyVertices();
        bool pointIsInSector(const QPodPoint &p, const QPodPoint &v1, const QPodPoint &v2, const QPodPoint &v3);
        bool pointIsInSector(int vertex, int sector);
        int findSector(int edge, int vertex);
        void createDiagonal(int lower, int upper);
        void monotoneDecomposition();

        QTriangulator *m_parent;
        QRBTree<int> m_edgeList;
        QDataBuffer<Edge> m_edges;
        QDataBuffer<int> m_upperVertex;
        bool m_clockwiseOrder;
    };

    //====================================//
    // QTriangulator::MonotoneToTriangles //
    //====================================//
    friend class MonotoneToTriangles;
    class MonotoneToTriangles
    {
    public:
        inline MonotoneToTriangles(QTriangulator<T> *parent) : m_parent(parent) { }
        void decompose();
    private:
        inline T indices(int index) const {return m_parent->m_indices.at(index + m_first);}
        inline int next(int index) const {return (index + 1) % m_length;}
        inline int previous(int index) const {return (index + m_length - 1) % m_length;}
        inline bool less(int i, int j) const {return m_parent->m_vertices.at((qint32)indices(i)) < m_parent->m_vertices.at(indices(j));}
        inline bool leftOfEdge(int i, int j, int k) const
        {
            return qPointIsLeftOfLine(m_parent->m_vertices.at((qint32)indices(i)),
                m_parent->m_vertices.at((qint32)indices(j)), m_parent->m_vertices.at((qint32)indices(k)));
        }

        QTriangulator<T> *m_parent;
        int m_first;
        int m_length;
    };

    inline QTriangulator() : m_vertices(0) { }

    // Call this only once.
    void initialize(const qreal *polygon, int count, uint hint, const QTransform &matrix);
    // Call this only once.
    void initialize(const QVectorPath &path, const QTransform &matrix, qreal lod);
    // Call this only once.
    void initialize(const QPainterPath &path, const QTransform &matrix, qreal lod);
    // Call either triangulate() or polyline() only once.
    QVertexSet<T> triangulate();
    QVertexSet<T> polyline();
private:
    QDataBuffer<QPodPoint> m_vertices;
    QVector<T> m_indices;
    uint m_hint;
};

//============================================================================//
//                               QTriangulator                                //
//============================================================================//

template <typename T>
QVertexSet<T> QTriangulator<T>::triangulate()
{
    for (int i = 0; i < m_vertices.size(); ++i) {
        Q_ASSERT(qAbs(m_vertices.at(i).x) < (1 << 21));
        Q_ASSERT(qAbs(m_vertices.at(i).y) < (1 << 21));
    }

    if (!(m_hint & (QVectorPath::OddEvenFill | QVectorPath::WindingFill)))
        m_hint |= QVectorPath::OddEvenFill;

    if (m_hint & QVectorPath::NonConvexShapeMask) {
        ComplexToSimple c2s(this);
        c2s.decompose();
        SimpleToMonotone s2m(this);
        s2m.decompose();
    }
    MonotoneToTriangles m2t(this);
    m2t.decompose();

    QVertexSet<T> result;
    result.indices = m_indices;
    result.vertices.resize(2 * m_vertices.size());
    for (int i = 0; i < m_vertices.size(); ++i) {
        result.vertices[2 * i + 0] = qreal(m_vertices.at(i).x) / Q_FIXED_POINT_SCALE;
        result.vertices[2 * i + 1] = qreal(m_vertices.at(i).y) / Q_FIXED_POINT_SCALE;
    }
    return result;
}

template <typename T>
QVertexSet<T> QTriangulator<T>::polyline()
{
    for (int i = 0; i < m_vertices.size(); ++i) {
        Q_ASSERT(qAbs(m_vertices.at(i).x) < (1 << 21));
        Q_ASSERT(qAbs(m_vertices.at(i).y) < (1 << 21));
    }

    if (!(m_hint & (QVectorPath::OddEvenFill | QVectorPath::WindingFill)))
        m_hint |= QVectorPath::OddEvenFill;

    if (m_hint & QVectorPath::NonConvexShapeMask) {
        ComplexToSimple c2s(this);
        c2s.decompose();
    }

    QVertexSet<T> result;
    result.indices = m_indices;
    result.vertices.resize(2 * m_vertices.size());
    for (int i = 0; i < m_vertices.size(); ++i) {
        result.vertices[2 * i + 0] = qreal(m_vertices.at(i).x) / Q_FIXED_POINT_SCALE;
        result.vertices[2 * i + 1] = qreal(m_vertices.at(i).y) / Q_FIXED_POINT_SCALE;
    }
    return result;
}

template <typename T>
void QTriangulator<T>::initialize(const qreal *polygon, int count, uint hint, const QTransform &matrix)
{
    m_hint = hint;
    m_vertices.resize(count);
    m_indices.resize(count + 1);
    for (int i = 0; i < count; ++i) {
        qreal x, y;
        matrix.map(polygon[2 * i + 0], polygon[2 * i + 1], &x, &y);
        m_vertices.at(i).x = qRound(x * Q_FIXED_POINT_SCALE);
        m_vertices.at(i).y = qRound(y * Q_FIXED_POINT_SCALE);
        m_indices[i] = i;
    }
    m_indices[count] = T(-1); //Q_TRIANGULATE_END_OF_POLYGON
}

template <typename T>
void QTriangulator<T>::initialize(const QVectorPath &path, const QTransform &matrix, qreal lod)
{
    m_hint = path.hints();
    // Curved paths will be converted to complex polygons.
    m_hint &= ~QVectorPath::CurvedShapeMask;

    const qreal *p = path.points();
    const QPainterPath::ElementType *e = path.elements();
    if (e) {
        for (int i = 0; i < path.elementCount(); ++i, ++e, p += 2) {
            switch (*e) {
            case QPainterPath::MoveToElement:
                if (!m_indices.isEmpty())
                    m_indices.push_back(T(-1)); // Q_TRIANGULATE_END_OF_POLYGON
                // Fall through.
            case QPainterPath::LineToElement:
                m_indices.push_back(T(m_vertices.size()));
                m_vertices.resize(m_vertices.size() + 1);
                qreal x, y;
                matrix.map(p[0], p[1], &x, &y);
                m_vertices.last().x = qRound(x * Q_FIXED_POINT_SCALE);
                m_vertices.last().y = qRound(y * Q_FIXED_POINT_SCALE);
                break;
            case QPainterPath::CurveToElement:
                {
                    qreal pts[8];
                    for (int i = 0; i < 4; ++i)
                        matrix.map(p[2 * i - 2], p[2 * i - 1], &pts[2 * i + 0], &pts[2 * i + 1]);
                    for (int i = 0; i < 8; ++i)
                        pts[i] *= lod;
                    QBezier bezier = QBezier::fromPoints(QPointF(pts[0], pts[1]), QPointF(pts[2], pts[3]), QPointF(pts[4], pts[5]), QPointF(pts[6], pts[7]));
                    QPolygonF poly = bezier.toPolygon();
                    // Skip first point, it already exists in 'm_vertices'.
                    for (int j = 1; j < poly.size(); ++j) {
                        m_indices.push_back(T(m_vertices.size()));
                        m_vertices.resize(m_vertices.size() + 1);
                        m_vertices.last().x = qRound(poly.at(j).x() * Q_FIXED_POINT_SCALE / lod);
                        m_vertices.last().y = qRound(poly.at(j).y() * Q_FIXED_POINT_SCALE / lod);
                    }
                }
                i += 2;
                e += 2;
                p += 4;
                break;
            default:
                Q_ASSERT_X(0, "QTriangulator::triangulate", "Unexpected element type.");
                break;
            }
        }
    } else {
        for (int i = 0; i < path.elementCount(); ++i, p += 2) {
            m_indices.push_back(T(m_vertices.size()));
            m_vertices.resize(m_vertices.size() + 1);
            qreal x, y;
            matrix.map(p[0], p[1], &x, &y);
            m_vertices.last().x = qRound(x * Q_FIXED_POINT_SCALE);
            m_vertices.last().y = qRound(y * Q_FIXED_POINT_SCALE);
        }
    }
    m_indices.push_back(T(-1)); // Q_TRIANGULATE_END_OF_POLYGON
}

template <typename T>
void QTriangulator<T>::initialize(const QPainterPath &path, const QTransform &matrix, qreal lod)
{
    initialize(qtVectorPathForPath(path), matrix, lod);
}

//============================================================================//
//                       QTriangulator::ComplexToSimple                       //
//============================================================================//
template <typename T>
void QTriangulator<T>::ComplexToSimple::decompose()
{
    m_initialPointCount = m_parent->m_vertices.size();
    initEdges();
    do {
        calculateIntersections();
    } while (splitEdgesAtIntersections());

    removeUnwantedEdgesAndConnect();
    removeUnusedPoints();

    m_parent->m_indices.clear();
    QBitArray processed(m_edges.size(), false);
    for (int first = 0; first < m_edges.size(); ++first) {
        // If already processed, or if unused path, skip.
        if (processed.at(first) || m_edges.at(first).next == -1)
            continue;

        int i = first;
        do {
            Q_ASSERT(!processed.at(i));
            Q_ASSERT(m_edges.at(m_edges.at(i).next).previous == i);
            m_parent->m_indices.push_back(m_edges.at(i).from);
            processed.setBit(i);
            i = m_edges.at(i).next; // CCW order
        } while (i != first);
        m_parent->m_indices.push_back(T(-1)); // Q_TRIANGULATE_END_OF_POLYGON
    }
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::initEdges()
{
    // Initialize edge structure.
    // 'next' and 'previous' are not being initialized at this point.
    int first = 0;
    for (int i = 0; i < m_parent->m_indices.size(); ++i) {
        if (m_parent->m_indices.at(i) == T(-1)) { // Q_TRIANGULATE_END_OF_POLYGON
            if (m_edges.size() != first)
                m_edges.last().to = m_edges.at(first).from;
            first = m_edges.size();
        } else {
            Q_ASSERT(i + 1 < m_parent->m_indices.size());
            // {node, from, to, next, previous, winding, mayIntersect, pointingUp, originallyPointingUp}
            Edge edge = {0, int(m_parent->m_indices.at(i)), int(m_parent->m_indices.at(i + 1)), -1, -1, 0, true, false, false};
            m_edges.add(edge);
        }
    }
    if (first != m_edges.size())
        m_edges.last().to = m_edges.at(first).from;
    for (int i = 0; i < m_edges.size(); ++i) {
        m_edges.at(i).originallyPointingUp = m_edges.at(i).pointingUp =
            m_parent->m_vertices.at(m_edges.at(i).to) < m_parent->m_vertices.at(m_edges.at(i).from);
    }
}

// Return true if new intersection was found
template <typename T>
bool QTriangulator<T>::ComplexToSimple::calculateIntersection(int left, int right)
{
    const Edge &e1 = m_edges.at(left);
    const Edge &e2 = m_edges.at(right);

    const QPodPoint &u1 = m_parent->m_vertices.at((qint32)e1.from);
    const QPodPoint &u2 = m_parent->m_vertices.at((qint32)e1.to);
    const QPodPoint &v1 = m_parent->m_vertices.at((qint32)e2.from);
    const QPodPoint &v2 = m_parent->m_vertices.at((qint32)e2.to);
    if (qMax(u1.x, u2.x) <= qMin(v1.x, v2.x))
        return false;

    quint64 key = (left > right ? (quint64(right) << 32) | quint64(left) : (quint64(left) << 32) | quint64(right));
    if (m_processedEdgePairs.contains(key))
        return false;
    m_processedEdgePairs.insert(key);

    Intersection intersection;
    intersection.leftEdge = left;
    intersection.rightEdge = right;
    intersection.intersectionPoint = QT_PREPEND_NAMESPACE(qIntersectionPoint)(u1, u2, v1, v2);

    if (!intersection.intersectionPoint.isValid())
        return false;

    Q_ASSERT(intersection.intersectionPoint.isOnLine(u1, u2));
    Q_ASSERT(intersection.intersectionPoint.isOnLine(v1, v2));

    intersection.vertex = m_parent->m_vertices.size();
    m_topIntersection.push(intersection);
    m_parent->m_vertices.add(intersection.intersectionPoint.round());
    return true;
}

template <typename T>
bool QTriangulator<T>::ComplexToSimple::edgeIsLeftOfEdge(int leftEdgeIndex, int rightEdgeIndex) const
{
    const Edge &leftEdge = m_edges.at(leftEdgeIndex);
    const Edge &rightEdge = m_edges.at(rightEdgeIndex);
    const QPodPoint &u = m_parent->m_vertices.at(rightEdge.upper());
    const QPodPoint &l = m_parent->m_vertices.at(rightEdge.lower());
    const QPodPoint &upper = m_parent->m_vertices.at(leftEdge.upper());
    if (upper.x < qMin(l.x, u.x))
        return true;
    if (upper.x > qMax(l.x, u.x))
        return false;
    qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(upper, l, u);
    // d < 0: left, d > 0: right, d == 0: on top
    if (d == 0)
        d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(m_parent->m_vertices.at(leftEdge.lower()), l, u);
    return d < 0;
}

template <typename T>
QRBTree<int>::Node *QTriangulator<T>::ComplexToSimple::searchEdgeLeftOf(int edgeIndex) const
{
    QRBTree<int>::Node *current = m_edgeList.root;
    QRBTree<int>::Node *result = 0;
    while (current) {
        if (edgeIsLeftOfEdge(edgeIndex, current->data)) {
            current = current->left;
        } else {
            result = current;
            current = current->right;
        }
    }
    return result;
}

template <typename T>
QRBTree<int>::Node *QTriangulator<T>::ComplexToSimple::searchEdgeLeftOf(int edgeIndex, QRBTree<int>::Node *after) const
{
    if (!m_edgeList.root)
        return after;
    QRBTree<int>::Node *result = after;
    QRBTree<int>::Node *current = (after ? m_edgeList.next(after) : m_edgeList.front(m_edgeList.root));
    while (current) {
        if (edgeIsLeftOfEdge(edgeIndex, current->data))
            return result;
        result = current;
        current = m_edgeList.next(current);
    }
    return result;
}

template <typename T>
QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> QTriangulator<T>::ComplexToSimple::bounds(const QPodPoint &point) const
{
    QRBTree<int>::Node *current = m_edgeList.root;
    QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> result(0, 0);
    while (current) {
        const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &v2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(point, v1, v2);
        if (d == 0) {
            result.first = result.second = current;
            break;
        }
        current = (d < 0 ? current->left : current->right);
    }
    if (current == 0)
        return result;

    current = result.first->left;
    while (current) {
        const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &v2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(point, v1, v2);
        Q_ASSERT(d >= 0);
        if (d == 0) {
            result.first = current;
            current = current->left;
        } else {
            current = current->right;
        }
    }

    current = result.second->right;
    while (current) {
        const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &v2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(point, v1, v2);
        Q_ASSERT(d <= 0);
        if (d == 0) {
            result.second = current;
            current = current->right;
        } else {
            current = current->left;
        }
    }

    return result;
}

template <typename T>
QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> QTriangulator<T>::ComplexToSimple::outerBounds(const QPodPoint &point) const
{
    QRBTree<int>::Node *current = m_edgeList.root;
    QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> result(0, 0);

    while (current) {
        const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &v2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(point, v1, v2);
        if (d == 0)
            break;
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

    QRBTree<int>::Node *mid = current;

    current = mid->left;
    while (current) {
        const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &v2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(point, v1, v2);
        Q_ASSERT(d >= 0);
        if (d == 0) {
            current = current->left;
        } else {
            result.first = current;
            current = current->right;
        }
    }

    current = mid->right;
    while (current) {
        const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &v2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(point, v1, v2);
        Q_ASSERT(d <= 0);
        if (d == 0) {
            current = current->right;
        } else {
            result.second = current;
            current = current->left;
        }
    }

    return result;
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::splitEdgeListRange(QRBTree<int>::Node *leftmost, QRBTree<int>::Node *rightmost, int vertex, const QIntersectionPoint &intersectionPoint)
{
    Q_ASSERT(leftmost && rightmost);

    // Split.
    for (;;) {
        const QPodPoint &u = m_parent->m_vertices.at(m_edges.at(leftmost->data).from);
        const QPodPoint &v = m_parent->m_vertices.at(m_edges.at(leftmost->data).to);
        Q_ASSERT(intersectionPoint.isOnLine(u, v));
        const Split split = {vertex, leftmost->data, intersectionPoint.isAccurate()};
        if (intersectionPoint.xOffset.numerator != 0 || intersectionPoint.yOffset.numerator != 0 || (intersectionPoint.upperLeft != u && intersectionPoint.upperLeft != v))
            m_splits.add(split);
        if (leftmost == rightmost)
            break;
        leftmost = m_edgeList.next(leftmost);
    }
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::reorderEdgeListRange(QRBTree<int>::Node *leftmost, QRBTree<int>::Node *rightmost)
{
    Q_ASSERT(leftmost && rightmost);

    QRBTree<int>::Node *storeLeftmost = leftmost;
    QRBTree<int>::Node *storeRightmost = rightmost;

    // Reorder.
    while (leftmost != rightmost) {
        Edge &left = m_edges.at(leftmost->data);
        Edge &right = m_edges.at(rightmost->data);
        qSwap(left.node, right.node);
        qSwap(leftmost->data, rightmost->data);
        leftmost = m_edgeList.next(leftmost);
        if (leftmost == rightmost)
            break;
        rightmost = m_edgeList.previous(rightmost);
    }

    rightmost = m_edgeList.next(storeRightmost);
    leftmost = m_edgeList.previous(storeLeftmost);
    if (leftmost)
        calculateIntersection(leftmost->data, storeLeftmost->data);
    if (rightmost)
        calculateIntersection(storeRightmost->data, rightmost->data);
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::sortEdgeList(const QPodPoint eventPoint)
{
    QIntersectionPoint eventPoint2 = QT_PREPEND_NAMESPACE(qIntersectionPoint)(eventPoint);
    while (!m_topIntersection.isEmpty() && m_topIntersection.top().intersectionPoint < eventPoint2) {
        Intersection intersection = m_topIntersection.pop();

        QIntersectionPoint currentIntersectionPoint = intersection.intersectionPoint;
        int currentVertex = intersection.vertex;

        QRBTree<int>::Node *leftmost = m_edges.at(intersection.leftEdge).node;
        QRBTree<int>::Node *rightmost = m_edges.at(intersection.rightEdge).node;

        for (;;) {
            QRBTree<int>::Node *previous = m_edgeList.previous(leftmost);
            if (!previous)
                break;
            const Edge &edge = m_edges.at(previous->data);
            const QPodPoint &u = m_parent->m_vertices.at((qint32)edge.from);
            const QPodPoint &v = m_parent->m_vertices.at((qint32)edge.to);
            if (!currentIntersectionPoint.isOnLine(u, v)) {
                Q_ASSERT(!currentIntersectionPoint.isAccurate() || qCross(currentIntersectionPoint.upperLeft - u, v - u) != 0);
                break;
            }
            leftmost = previous;
        }

        for (;;) {
            QRBTree<int>::Node *next = m_edgeList.next(rightmost);
            if (!next)
                break;
            const Edge &edge = m_edges.at(next->data);
            const QPodPoint &u = m_parent->m_vertices.at((qint32)edge.from);
            const QPodPoint &v = m_parent->m_vertices.at((qint32)edge.to);
            if (!currentIntersectionPoint.isOnLine(u, v)) {
                Q_ASSERT(!currentIntersectionPoint.isAccurate() || qCross(currentIntersectionPoint.upperLeft - u, v - u) != 0);
                break;
            }
            rightmost = next;
        }

        Q_ASSERT(leftmost && rightmost);
        splitEdgeListRange(leftmost, rightmost, currentVertex, currentIntersectionPoint);
        reorderEdgeListRange(leftmost, rightmost);

        while (!m_topIntersection.isEmpty() && m_topIntersection.top().intersectionPoint <= currentIntersectionPoint)
            m_topIntersection.pop();

#ifdef Q_TRIANGULATOR_DEBUG
        DebugDialog dialog(this, intersection.vertex);
        dialog.exec();
#endif

    }
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::fillPriorityQueue()
{
    m_events.reset();
    m_events.reserve(m_edges.size() * 2);
    for (int i = 0; i < m_edges.size(); ++i) {
        Q_ASSERT(m_edges.at(i).previous == -1 && m_edges.at(i).next == -1);
        Q_ASSERT(m_edges.at(i).node == 0);
        Q_ASSERT(m_edges.at(i).pointingUp == m_edges.at(i).originallyPointingUp);
        Q_ASSERT(m_edges.at(i).pointingUp == (m_parent->m_vertices.at(m_edges.at(i).to) < m_parent->m_vertices.at(m_edges.at(i).from)));
        // Ignore zero-length edges.
        if (m_parent->m_vertices.at(m_edges.at(i).to) != m_parent->m_vertices.at(m_edges.at(i).from)) {
            QPodPoint upper = m_parent->m_vertices.at(m_edges.at(i).upper());
            QPodPoint lower = m_parent->m_vertices.at(m_edges.at(i).lower());
            Event upperEvent = {{upper.x, upper.y}, Event::Upper, i};
            Event lowerEvent = {{lower.x, lower.y}, Event::Lower, i};
            m_events.add(upperEvent);
            m_events.add(lowerEvent);
        }
    }

    std::sort(m_events.data(), m_events.data() + m_events.size());
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::calculateIntersections()
{
    fillPriorityQueue();

    Q_ASSERT(m_topIntersection.empty());
    Q_ASSERT(m_edgeList.root == 0);

    // Find all intersection points.
    while (!m_events.isEmpty()) {
        Event event = m_events.last();
        sortEdgeList(event.point);

        // Find all edges in the edge list that contain the current vertex and mark them to be split later.
        QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> range = bounds(event.point);
        QRBTree<int>::Node *leftNode = range.first ? m_edgeList.previous(range.first) : 0;
        int vertex = (event.type == Event::Upper ? m_edges.at(event.edge).upper() : m_edges.at(event.edge).lower());
        QIntersectionPoint eventPoint = QT_PREPEND_NAMESPACE(qIntersectionPoint)(event.point);

        if (range.first != 0) {
            splitEdgeListRange(range.first, range.second, vertex, eventPoint);
            reorderEdgeListRange(range.first, range.second);
        }

        // Handle the edges with start or end point in the current vertex.
        while (!m_events.isEmpty() && m_events.last().point == event.point) {
            event = m_events.last();
            m_events.pop_back();
            int i = event.edge;

            if (m_edges.at(i).node) {
                // Remove edge from edge list.
                Q_ASSERT(event.type == Event::Lower);
                QRBTree<int>::Node *left = m_edgeList.previous(m_edges.at(i).node);
                QRBTree<int>::Node *right = m_edgeList.next(m_edges.at(i).node);
                m_edgeList.deleteNode(m_edges.at(i).node);
                if (!left || !right)
                    continue;
                calculateIntersection(left->data, right->data);
            } else {
                // Insert edge into edge list.
                Q_ASSERT(event.type == Event::Upper);
                QRBTree<int>::Node *left = searchEdgeLeftOf(i, leftNode);
                m_edgeList.attachAfter(left, m_edges.at(i).node = m_edgeList.newNode());
                m_edges.at(i).node->data = i;
                QRBTree<int>::Node *right = m_edgeList.next(m_edges.at(i).node);
                if (left)
                    calculateIntersection(left->data, i);
                if (right)
                    calculateIntersection(i, right->data);
            }
        }
        while (!m_topIntersection.isEmpty() && m_topIntersection.top().intersectionPoint <= eventPoint)
            m_topIntersection.pop();
#ifdef Q_TRIANGULATOR_DEBUG
        DebugDialog dialog(this, vertex);
        dialog.exec();
#endif
    }
    m_processedEdgePairs.clear();
}

// Split an edge into two pieces at the given point.
// The upper piece is pushed to the end of the 'm_edges' vector.
// The lower piece replaces the old edge.
// Return the edge whose 'from' is 'pointIndex'.
template <typename T>
int QTriangulator<T>::ComplexToSimple::splitEdge(int splitIndex)
{
    const Split &split = m_splits.at(splitIndex);
    Edge &lowerEdge = m_edges.at(split.edge);
    Q_ASSERT(lowerEdge.node == 0);
    Q_ASSERT(lowerEdge.previous == -1 && lowerEdge.next == -1);

    if (lowerEdge.from == split.vertex)
        return split.edge;
    if (lowerEdge.to == split.vertex)
        return lowerEdge.next;

    // Check that angle >= 90 degrees.
    //Q_ASSERT(qDot(m_points.at(m_edges.at(edgeIndex).from) - m_points.at(pointIndex),
    //    m_points.at(m_edges.at(edgeIndex).to) - m_points.at(pointIndex)) <= 0);

    Edge upperEdge = lowerEdge;
    upperEdge.mayIntersect |= !split.accurate; // The edge may have been split before at an inaccurate split point.
    lowerEdge.mayIntersect = !split.accurate;
    if (lowerEdge.pointingUp) {
        lowerEdge.to = upperEdge.from = split.vertex;
        m_edges.add(upperEdge);
        return m_edges.size() - 1;
    } else {
        lowerEdge.from = upperEdge.to = split.vertex;
        m_edges.add(upperEdge);
        return split.edge;
    }
}

template <typename T>
bool QTriangulator<T>::ComplexToSimple::splitEdgesAtIntersections()
{
    for (int i = 0; i < m_edges.size(); ++i)
        m_edges.at(i).mayIntersect = false;
    bool checkForNewIntersections = false;
    for (int i = 0; i < m_splits.size(); ++i) {
        splitEdge(i);
        checkForNewIntersections |= !m_splits.at(i).accurate;
    }
    for (int i = 0; i < m_edges.size(); ++i) {
        m_edges.at(i).originallyPointingUp = m_edges.at(i).pointingUp =
            m_parent->m_vertices.at(m_edges.at(i).to) < m_parent->m_vertices.at(m_edges.at(i).from);
    }
    m_splits.reset();
    return checkForNewIntersections;
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::insertEdgeIntoVectorIfWanted(ShortArray &orderedEdges, int i)
{
    // Edges with zero length should not reach this part.
    Q_ASSERT(m_parent->m_vertices.at(m_edges.at(i).from) != m_parent->m_vertices.at(m_edges.at(i).to));

    // Skip edges with unwanted winding number.
    int windingNumber = m_edges.at(i).winding;
    if (m_edges.at(i).originallyPointingUp)
        ++windingNumber;

    // Make sure exactly one fill rule is specified.
    Q_ASSERT(((m_parent->m_hint & QVectorPath::WindingFill) != 0) != ((m_parent->m_hint & QVectorPath::OddEvenFill) != 0));

    if ((m_parent->m_hint & QVectorPath::WindingFill) && windingNumber != 0 && windingNumber != 1)
        return;

    // Skip cancelling edges.
    if (!orderedEdges.isEmpty()) {
        int j = orderedEdges[orderedEdges.size() - 1];
        // If the last edge is already connected in one end, it should not be cancelled.
        if (m_edges.at(j).next == -1 && m_edges.at(j).previous == -1
            && (m_parent->m_vertices.at(m_edges.at(i).from) == m_parent->m_vertices.at(m_edges.at(j).to))
            && (m_parent->m_vertices.at(m_edges.at(i).to) == m_parent->m_vertices.at(m_edges.at(j).from))) {
            orderedEdges.removeLast();
            return;
        }
    }
    orderedEdges.append(i);
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::removeUnwantedEdgesAndConnect()
{
    Q_ASSERT(m_edgeList.root == 0);
    // Initialize priority queue.
    fillPriorityQueue();

    ShortArray orderedEdges;

    while (!m_events.isEmpty()) {
        Event event = m_events.last();
        int edgeIndex = event.edge;

        // Check that all the edges in the list crosses the current scanline
        //if (m_edgeList.root) {
        //    for (QRBTree<int>::Node *node = m_edgeList.front(m_edgeList.root); node; node = m_edgeList.next(node)) {
        //        Q_ASSERT(event.point <= m_points.at(m_edges.at(node->data).lower()));
        //    }
        //}

        orderedEdges.clear();
        QPair<QRBTree<int>::Node *, QRBTree<int>::Node *> b = outerBounds(event.point);
        if (m_edgeList.root) {
            QRBTree<int>::Node *current = (b.first ? m_edgeList.next(b.first) : m_edgeList.front(m_edgeList.root));
            // Process edges that are going to be removed from the edge list at the current event point.
            while (current != b.second) {
                Q_ASSERT(current);
                Q_ASSERT(m_edges.at(current->data).node == current);
                Q_ASSERT(QT_PREPEND_NAMESPACE(qIntersectionPoint)(event.point).isOnLine(m_parent->m_vertices.at(m_edges.at(current->data).from), m_parent->m_vertices.at(m_edges.at(current->data).to)));
                Q_ASSERT(m_parent->m_vertices.at(m_edges.at(current->data).from) == event.point || m_parent->m_vertices.at(m_edges.at(current->data).to) == event.point);
                insertEdgeIntoVectorIfWanted(orderedEdges, current->data);
                current = m_edgeList.next(current);
            }
        }

        // Remove edges above the event point, insert edges below the event point.
        do {
            event = m_events.last();
            m_events.pop_back();
            edgeIndex = event.edge;

            // Edges with zero length should not reach this part.
            Q_ASSERT(m_parent->m_vertices.at(m_edges.at(edgeIndex).from) != m_parent->m_vertices.at(m_edges.at(edgeIndex).to));

            if (m_edges.at(edgeIndex).node) {
                Q_ASSERT(event.type == Event::Lower);
                Q_ASSERT(event.point == m_parent->m_vertices.at(m_edges.at(event.edge).lower()));
                m_edgeList.deleteNode(m_edges.at(edgeIndex).node);
            } else {
                Q_ASSERT(event.type == Event::Upper);
                Q_ASSERT(event.point == m_parent->m_vertices.at(m_edges.at(event.edge).upper()));
                QRBTree<int>::Node *left = searchEdgeLeftOf(edgeIndex, b.first);
                m_edgeList.attachAfter(left, m_edges.at(edgeIndex).node = m_edgeList.newNode());
                m_edges.at(edgeIndex).node->data = edgeIndex;
            }
        } while (!m_events.isEmpty() && m_events.last().point == event.point);

        if (m_edgeList.root) {
            QRBTree<int>::Node *current = (b.first ? m_edgeList.next(b.first) : m_edgeList.front(m_edgeList.root));

            // Calculate winding number and turn counter-clockwise.
            int currentWindingNumber = (b.first ? m_edges.at(b.first->data).winding : 0);
            while (current != b.second) {
                Q_ASSERT(current);
                //Q_ASSERT(b.second == 0 || m_edgeList.order(current, b.second) < 0);
                int i = current->data;
                Q_ASSERT(m_edges.at(i).node == current);

                // Winding number.
                int ccwWindingNumber = m_edges.at(i).winding = currentWindingNumber;
                if (m_edges.at(i).originallyPointingUp) {
                    --m_edges.at(i).winding;
                } else {
                    ++m_edges.at(i).winding;
                    ++ccwWindingNumber;
                }
                currentWindingNumber = m_edges.at(i).winding;

                // Turn counter-clockwise.
                if ((ccwWindingNumber & 1) == 0) {
                    Q_ASSERT(m_edges.at(i).previous == -1 && m_edges.at(i).next == -1);
                    qSwap(m_edges.at(i).from, m_edges.at(i).to);
                    m_edges.at(i).pointingUp = !m_edges.at(i).pointingUp;
                }

                current = m_edgeList.next(current);
            }

            // Process edges that were inserted into the edge list at the current event point.
            current = (b.second ? m_edgeList.previous(b.second) : m_edgeList.back(m_edgeList.root));
            while (current != b.first) {
                Q_ASSERT(current);
                Q_ASSERT(m_edges.at(current->data).node == current);
                insertEdgeIntoVectorIfWanted(orderedEdges, current->data);
                current = m_edgeList.previous(current);
            }
        }
        if (orderedEdges.isEmpty())
            continue;

        Q_ASSERT((orderedEdges.size() & 1) == 0);

        // Connect edges.
        // First make sure the first edge point towards the current point.
        int i;
        if (m_parent->m_vertices.at(m_edges.at(orderedEdges[0]).from) == event.point) {
            i = 1;
            int copy = orderedEdges[0]; // Make copy in case the append() will cause a reallocation.
            orderedEdges.append(copy);
        } else {
            Q_ASSERT(m_parent->m_vertices.at(m_edges.at(orderedEdges[0]).to) == event.point);
            i = 0;
        }

        // Remove references to duplicate points. First find the point with lowest index.
        int pointIndex = INT_MAX;
        for (int j = i; j < orderedEdges.size(); j += 2) {
            Q_ASSERT(j + 1 < orderedEdges.size());
            Q_ASSERT(m_parent->m_vertices.at(m_edges.at(orderedEdges[j]).to) == event.point);
            Q_ASSERT(m_parent->m_vertices.at(m_edges.at(orderedEdges[j + 1]).from) == event.point);
            if (m_edges.at(orderedEdges[j]).to < pointIndex)
                pointIndex = m_edges.at(orderedEdges[j]).to;
            if (m_edges.at(orderedEdges[j + 1]).from < pointIndex)
                pointIndex = m_edges.at(orderedEdges[j + 1]).from;
        }

        for (; i < orderedEdges.size(); i += 2) {
            // Remove references to duplicate points by making all edges reference one common point.
            m_edges.at(orderedEdges[i]).to = m_edges.at(orderedEdges[i + 1]).from = pointIndex;

            Q_ASSERT(m_edges.at(orderedEdges[i]).pointingUp || m_edges.at(orderedEdges[i]).previous != -1);
            Q_ASSERT(!m_edges.at(orderedEdges[i + 1]).pointingUp || m_edges.at(orderedEdges[i + 1]).next != -1);

            m_edges.at(orderedEdges[i]).next = orderedEdges[i + 1];
            m_edges.at(orderedEdges[i + 1]).previous = orderedEdges[i];
        }
    } // end while
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::removeUnusedPoints() {
    QBitArray used(m_parent->m_vertices.size(), false);
    for (int i = 0; i < m_edges.size(); ++i) {
        Q_ASSERT((m_edges.at(i).previous == -1) == (m_edges.at(i).next == -1));
        if (m_edges.at(i).next != -1)
            used.setBit(m_edges.at(i).from);
    }
    QDataBuffer<quint32> newMapping(m_parent->m_vertices.size());
    newMapping.resize(m_parent->m_vertices.size());
    int count = 0;
    for (int i = 0; i < m_parent->m_vertices.size(); ++i) {
        if (used.at(i)) {
            m_parent->m_vertices.at(count) = m_parent->m_vertices.at(i);
            newMapping.at(i) = count;
            ++count;
        }
    }
    m_parent->m_vertices.resize(count);
    for (int i = 0; i < m_edges.size(); ++i) {
        m_edges.at(i).from = newMapping.at(m_edges.at(i).from);
        m_edges.at(i).to = newMapping.at(m_edges.at(i).to);
    }
}

template <typename T>
inline bool QTriangulator<T>::ComplexToSimple::Event::operator < (const Event &other) const
{
    if (point == other.point)
        return type < other.type; // 'Lower' has higher priority than 'Upper'.
    return other.point < point;
}

//============================================================================//
//                QTriangulator::ComplexToSimple::DebugDialog                 //
//============================================================================//

#ifdef Q_TRIANGULATOR_DEBUG
template <typename T>
QTriangulator<T>::ComplexToSimple::DebugDialog::DebugDialog(ComplexToSimple *parent, int currentVertex)
    : m_parent(parent), m_vertex(currentVertex)
{
    QDataBuffer<QPodPoint> &vertices = m_parent->m_parent->m_vertices;
    if (vertices.isEmpty())
        return;

    int minX, maxX, minY, maxY;
    minX = maxX = vertices.at(0).x;
    minY = maxY = vertices.at(0).y;
    for (int i = 1; i < vertices.size(); ++i) {
        minX = qMin(minX, vertices.at(i).x);
        maxX = qMax(maxX, vertices.at(i).x);
        minY = qMin(minY, vertices.at(i).y);
        maxY = qMax(maxY, vertices.at(i).y);
    }
    int w = maxX - minX;
    int h = maxY - minY;
    qreal border = qMin(w, h) / 10.0;
    m_window = QRectF(minX - border, minY - border, (maxX - minX + 2 * border), (maxY - minY + 2 * border));
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::DebugDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::black);
    QDataBuffer<QPodPoint> &vertices = m_parent->m_parent->m_vertices;
    if (vertices.isEmpty())
        return;

    qreal halfPointSize = qMin(m_window.width(), m_window.height()) / 300.0;
    p.setWindow(m_window.toRect());

    p.setPen(Qt::white);

    QDataBuffer<Edge> &edges = m_parent->m_edges;
    for (int i = 0; i < edges.size(); ++i) {
        QPodPoint u = vertices.at(edges.at(i).from);
        QPodPoint v = vertices.at(edges.at(i).to);
        p.drawLine(u.x, u.y, v.x, v.y);
    }

    for (int i = 0; i < vertices.size(); ++i) {
        QPodPoint q = vertices.at(i);
        p.fillRect(QRectF(q.x - halfPointSize, q.y - halfPointSize, 2 * halfPointSize, 2 * halfPointSize), Qt::red);
    }

    Qt::GlobalColor colors[6] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow};
    p.setOpacity(0.5);
    int count = 0;
    if (m_parent->m_edgeList.root) {
        QRBTree<int>::Node *current = m_parent->m_edgeList.front(m_parent->m_edgeList.root);
        while (current) {
            p.setPen(colors[count++ % 6]);
            QPodPoint u = vertices.at(edges.at(current->data).from);
            QPodPoint v = vertices.at(edges.at(current->data).to);
            p.drawLine(u.x, u.y, v.x, v.y);
            current = m_parent->m_edgeList.next(current);
        }
    }

    p.setOpacity(1.0);
    QPodPoint q = vertices.at(m_vertex);
    p.fillRect(QRectF(q.x - halfPointSize, q.y - halfPointSize, 2 * halfPointSize, 2 * halfPointSize), Qt::green);

    p.setPen(Qt::gray);
    QDataBuffer<Split> &splits = m_parent->m_splits;
    for (int i = 0; i < splits.size(); ++i) {
        QPodPoint q = vertices.at(splits.at(i).vertex);
        QPodPoint u = vertices.at(edges.at(splits.at(i).edge).from) - q;
        QPodPoint v = vertices.at(edges.at(splits.at(i).edge).to) - q;
        qreal uLen = sqrt(qreal(qDot(u, u)));
        qreal vLen = sqrt(qreal(qDot(v, v)));
        if (uLen) {
            u.x *= 2 * halfPointSize / uLen;
            u.y *= 2 * halfPointSize / uLen;
        }
        if (vLen) {
            v.x *= 2 * halfPointSize / vLen;
            v.y *= 2 * halfPointSize / vLen;
        }
        u += q;
        v += q;
        p.drawLine(u.x, u.y, v.x, v.y);
    }
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::DebugDialog::wheelEvent(QWheelEvent *event)
{
    qreal scale = exp(-0.001 * event->delta());
    QPointF center = m_window.center();
    QPointF delta = scale * (m_window.bottomRight() - center);
    m_window = QRectF(center - delta, center + delta);
    event->accept();
    update();
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::DebugDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = event->pos() - m_lastMousePos;
        delta.setX(delta.x() * m_window.width() / width());
        delta.setY(delta.y() * m_window.height() / height());
        m_window.translate(-delta.x(), -delta.y());
        m_lastMousePos = event->pos();
        event->accept();
        update();
    }
}

template <typename T>
void QTriangulator<T>::ComplexToSimple::DebugDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_lastMousePos = event->pos();
    event->accept();
}


#endif

//============================================================================//
//                      QTriangulator::SimpleToMonotone                       //
//============================================================================//
template <typename T>
void QTriangulator<T>::SimpleToMonotone::decompose()
{
    setupDataStructures();
    removeZeroLengthEdges();
    monotoneDecomposition();

    m_parent->m_indices.clear();
    QBitArray processed(m_edges.size(), false);
    for (int first = 0; first < m_edges.size(); ++first) {
        if (processed.at(first))
            continue;
        int i = first;
        do {
            Q_ASSERT(!processed.at(i));
            Q_ASSERT(m_edges.at(m_edges.at(i).next).previous == i);
            m_parent->m_indices.push_back(m_edges.at(i).from);
            processed.setBit(i);
            i = m_edges.at(i).next;
        } while (i != first);
        if (m_parent->m_indices.size() > 0 && m_parent->m_indices.back() != T(-1)) // Q_TRIANGULATE_END_OF_POLYGON
            m_parent->m_indices.push_back(T(-1)); // Q_TRIANGULATE_END_OF_POLYGON
    }
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::setupDataStructures()
{
    int i = 0;
    Edge e;
    e.node = 0;
    e.twin = -1;

    while (i + 3 <= m_parent->m_indices.size()) {
        int start = m_edges.size();

        do {
            e.from = m_parent->m_indices.at(i);
            e.type = RegularVertex;
            e.next = m_edges.size() + 1;
            e.previous = m_edges.size() - 1;
            m_edges.add(e);
            ++i;
            Q_ASSERT(i < m_parent->m_indices.size());
        } while (m_parent->m_indices.at(i) != T(-1)); // Q_TRIANGULATE_END_OF_POLYGON

        m_edges.last().next = start;
        m_edges.at(start).previous = m_edges.size() - 1;
        ++i; // Skip Q_TRIANGULATE_END_OF_POLYGON.
    }

    for (i = 0; i < m_edges.size(); ++i) {
        m_edges.at(i).to = m_edges.at(m_edges.at(i).next).from;
        m_edges.at(i).pointingUp = m_parent->m_vertices.at(m_edges.at(i).to) < m_parent->m_vertices.at(m_edges.at(i).from);
        m_edges.at(i).helper = -1; // Not initialized here.
    }
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::removeZeroLengthEdges()
{
    for (int i = 0; i < m_edges.size(); ++i) {
        if (m_parent->m_vertices.at(m_edges.at(i).from) == m_parent->m_vertices.at(m_edges.at(i).to)) {
            m_edges.at(m_edges.at(i).previous).next = m_edges.at(i).next;
            m_edges.at(m_edges.at(i).next).previous = m_edges.at(i).previous;
            m_edges.at(m_edges.at(i).next).from = m_edges.at(i).from;
            m_edges.at(i).next = -1; // Mark as removed.
        }
    }

    QDataBuffer<int> newMapping(m_edges.size());
    newMapping.resize(m_edges.size());
    int count = 0;
    for (int i = 0; i < m_edges.size(); ++i) {
        if (m_edges.at(i).next != -1) {
            m_edges.at(count) = m_edges.at(i);
            newMapping.at(i) = count;
            ++count;
        }
    }
    m_edges.resize(count);
    for (int i = 0; i < m_edges.size(); ++i) {
        m_edges.at(i).next = newMapping.at(m_edges.at(i).next);
        m_edges.at(i).previous = newMapping.at(m_edges.at(i).previous);
    }
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::fillPriorityQueue()
{
    m_upperVertex.reset();
    m_upperVertex.reserve(m_edges.size());
    for (int i = 0; i < m_edges.size(); ++i)
        m_upperVertex.add(i);
    CompareVertices cmp(this);
    std::sort(m_upperVertex.data(), m_upperVertex.data() + m_upperVertex.size(), cmp);
    //for (int i = 1; i < m_upperVertex.size(); ++i) {
    //    Q_ASSERT(!cmp(m_upperVertex.at(i), m_upperVertex.at(i - 1)));
    //}
}

template <typename T>
bool QTriangulator<T>::SimpleToMonotone::edgeIsLeftOfEdge(int leftEdgeIndex, int rightEdgeIndex) const
{
    const Edge &leftEdge = m_edges.at(leftEdgeIndex);
    const Edge &rightEdge = m_edges.at(rightEdgeIndex);
    const QPodPoint &u = m_parent->m_vertices.at(rightEdge.upper());
    const QPodPoint &l = m_parent->m_vertices.at(rightEdge.lower());
    qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(m_parent->m_vertices.at(leftEdge.upper()), l, u);
    // d < 0: left, d > 0: right, d == 0: on top
    if (d == 0)
        d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(m_parent->m_vertices.at(leftEdge.lower()), l, u);
    return d < 0;
}

// Returns the rightmost edge not to the right of the given edge.
template <typename T>
QRBTree<int>::Node *QTriangulator<T>::SimpleToMonotone::searchEdgeLeftOfEdge(int edgeIndex) const
{
    QRBTree<int>::Node *current = m_edgeList.root;
    QRBTree<int>::Node *result = 0;
    while (current) {
        if (edgeIsLeftOfEdge(edgeIndex, current->data)) {
            current = current->left;
        } else {
            result = current;
            current = current->right;
        }
    }
    return result;
}

// Returns the rightmost edge left of the given point.
template <typename T>
QRBTree<int>::Node *QTriangulator<T>::SimpleToMonotone::searchEdgeLeftOfPoint(int pointIndex) const
{
    QRBTree<int>::Node *current = m_edgeList.root;
    QRBTree<int>::Node *result = 0;
    while (current) {
        const QPodPoint &p1 = m_parent->m_vertices.at(m_edges.at(current->data).lower());
        const QPodPoint &p2 = m_parent->m_vertices.at(m_edges.at(current->data).upper());
        qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(m_parent->m_vertices.at(pointIndex), p1, p2);
        if (d <= 0) {
            current = current->left;
        } else {
            result = current;
            current = current->right;
        }
    }
    return result;
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::classifyVertex(int i)
{
    Edge &e2 = m_edges.at(i);
    const Edge &e1 = m_edges.at(e2.previous);

    bool startOrSplit = (e1.pointingUp && !e2.pointingUp);
    bool endOrMerge = (!e1.pointingUp && e2.pointingUp);

    const QPodPoint &p1 = m_parent->m_vertices.at(e1.from);
    const QPodPoint &p2 = m_parent->m_vertices.at(e2.from);
    const QPodPoint &p3 = m_parent->m_vertices.at(e2.to);
    qint64 d = QT_PREPEND_NAMESPACE(qPointDistanceFromLine)(p1, p2, p3);
    Q_ASSERT(d != 0 || (!startOrSplit && !endOrMerge));

    e2.type = RegularVertex;

    if (m_clockwiseOrder) {
        if (startOrSplit)
            e2.type = (d < 0 ? SplitVertex : StartVertex);
        else if (endOrMerge)
            e2.type = (d < 0 ? MergeVertex : EndVertex);
    } else {
        if (startOrSplit)
            e2.type = (d > 0 ? SplitVertex : StartVertex);
        else if (endOrMerge)
            e2.type = (d > 0 ? MergeVertex : EndVertex);
    }
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::classifyVertices()
{
    for (int i = 0; i < m_edges.size(); ++i)
        classifyVertex(i);
}

template <typename T>
bool QTriangulator<T>::SimpleToMonotone::pointIsInSector(const QPodPoint &p, const QPodPoint &v1, const QPodPoint &v2, const QPodPoint &v3)
{
    bool leftOfPreviousEdge = !qPointIsLeftOfLine(p, v2, v1);
    bool leftOfNextEdge = !qPointIsLeftOfLine(p, v3, v2);

    if (qPointIsLeftOfLine(v1, v2, v3))
        return leftOfPreviousEdge && leftOfNextEdge;
    else
        return leftOfPreviousEdge || leftOfNextEdge;
}

template <typename T>
bool QTriangulator<T>::SimpleToMonotone::pointIsInSector(int vertex, int sector)
{
    const QPodPoint &center = m_parent->m_vertices.at(m_edges.at(sector).from);
    // Handle degenerate edges.
    while (m_parent->m_vertices.at(m_edges.at(vertex).from) == center)
        vertex = m_edges.at(vertex).next;
    int next = m_edges.at(sector).next;
    while (m_parent->m_vertices.at(m_edges.at(next).from) == center)
        next = m_edges.at(next).next;
    int previous = m_edges.at(sector).previous;
    while (m_parent->m_vertices.at(m_edges.at(previous).from) == center)
        previous = m_edges.at(previous).previous;

    const QPodPoint &p = m_parent->m_vertices.at(m_edges.at(vertex).from);
    const QPodPoint &v1 = m_parent->m_vertices.at(m_edges.at(previous).from);
    const QPodPoint &v3 = m_parent->m_vertices.at(m_edges.at(next).from);
    if (m_clockwiseOrder)
        return pointIsInSector(p, v3, center, v1);
    else
        return pointIsInSector(p, v1, center, v3);
}

template <typename T>
int QTriangulator<T>::SimpleToMonotone::findSector(int edge, int vertex)
{
    while (!pointIsInSector(vertex, edge)) {
        edge = m_edges.at(m_edges.at(edge).previous).twin;
        Q_ASSERT(edge != -1);
    }
    return edge;
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::createDiagonal(int lower, int upper)
{
    lower = findSector(lower, upper);
    upper = findSector(upper, lower);

    int prevLower = m_edges.at(lower).previous;
    int prevUpper = m_edges.at(upper).previous;

    Edge e;

    e.twin = m_edges.size() + 1;
    e.next = upper;
    e.previous = prevLower;
    e.from = m_edges.at(lower).from;
    e.to = m_edges.at(upper).from;
    m_edges.at(upper).previous = m_edges.at(prevLower).next = int(m_edges.size());
    m_edges.add(e);

    e.twin = m_edges.size() - 1;
    e.next = lower;
    e.previous = prevUpper;
    e.from = m_edges.at(upper).from;
    e.to = m_edges.at(lower).from;
    m_edges.at(lower).previous = m_edges.at(prevUpper).next = int(m_edges.size());
    m_edges.add(e);
}

template <typename T>
void QTriangulator<T>::SimpleToMonotone::monotoneDecomposition()
{
    if (m_edges.isEmpty())
        return;

    Q_ASSERT(!m_edgeList.root);
    QDataBuffer<QPair<int, int> > diagonals(m_upperVertex.size());

    int i = 0;
    for (int index = 1; index < m_edges.size(); ++index) {
        if (m_parent->m_vertices.at(m_edges.at(index).from) < m_parent->m_vertices.at(m_edges.at(i).from))
            i = index;
    }
    Q_ASSERT(i < m_edges.size());
    int j = m_edges.at(i).previous;
    Q_ASSERT(j < m_edges.size());
    m_clockwiseOrder = qPointIsLeftOfLine(m_parent->m_vertices.at((quint32)m_edges.at(i).from),
        m_parent->m_vertices.at((quint32)m_edges.at(j).from), m_parent->m_vertices.at((quint32)m_edges.at(i).to));

    classifyVertices();
    fillPriorityQueue();

    // debug: set helpers explicitly (shouldn't be necessary)
    //for (int i = 0; i < m_edges.size(); ++i)
    //    m_edges.at(i).helper = m_edges.at(i).upper();

    while (!m_upperVertex.isEmpty()) {
        i = m_upperVertex.last();
        Q_ASSERT(i < m_edges.size());
        m_upperVertex.pop_back();
        j = m_edges.at(i).previous;
        Q_ASSERT(j < m_edges.size());

        QRBTree<int>::Node *leftEdgeNode = 0;

        switch (m_edges.at(i).type) {
        case RegularVertex:
            // If polygon interior is to the right of the vertex...
            if (m_edges.at(i).pointingUp == m_clockwiseOrder) {
                if (m_edges.at(i).node) {
                    Q_ASSERT(!m_edges.at(j).node);
                    if (m_edges.at(m_edges.at(i).helper).type == MergeVertex)
                        diagonals.add(QPair<int, int>(i, m_edges.at(i).helper));
                    m_edges.at(j).node = m_edges.at(i).node;
                    m_edges.at(i).node = 0;
                    m_edges.at(j).node->data = j;
                    m_edges.at(j).helper = i;
                } else if (m_edges.at(j).node) {
                    Q_ASSERT(!m_edges.at(i).node);
                    if (m_edges.at(m_edges.at(j).helper).type == MergeVertex)
                        diagonals.add(QPair<int, int>(i, m_edges.at(j).helper));
                    m_edges.at(i).node = m_edges.at(j).node;
                    m_edges.at(j).node = 0;
                    m_edges.at(i).node->data = i;
                    m_edges.at(i).helper = i;
                } else {
                    qWarning("Inconsistent polygon. (#1)");
                }
            } else {
                leftEdgeNode = searchEdgeLeftOfPoint(m_edges.at(i).from);
                if (leftEdgeNode) {
                    if (m_edges.at(m_edges.at(leftEdgeNode->data).helper).type == MergeVertex)
                        diagonals.add(QPair<int, int>(i, m_edges.at(leftEdgeNode->data).helper));
                    m_edges.at(leftEdgeNode->data).helper = i;
                } else {
                    qWarning("Inconsistent polygon. (#2)");
                }
            }
            break;
        case SplitVertex:
            leftEdgeNode = searchEdgeLeftOfPoint(m_edges.at(i).from);
            if (leftEdgeNode) {
                diagonals.add(QPair<int, int>(i, m_edges.at(leftEdgeNode->data).helper));
                m_edges.at(leftEdgeNode->data).helper = i;
            } else {
                qWarning("Inconsistent polygon. (#3)");
            }
            // Fall through.
        case StartVertex:
            if (m_clockwiseOrder) {
                leftEdgeNode = searchEdgeLeftOfEdge(j);
                QRBTree<int>::Node *node = m_edgeList.newNode();
                node->data = j;
                m_edges.at(j).node = node;
                m_edges.at(j).helper = i;
                m_edgeList.attachAfter(leftEdgeNode, node);
                Q_ASSERT(m_edgeList.validate());
            } else  {
                leftEdgeNode = searchEdgeLeftOfEdge(i);
                QRBTree<int>::Node *node = m_edgeList.newNode();
                node->data = i;
                m_edges.at(i).node = node;
                m_edges.at(i).helper = i;
                m_edgeList.attachAfter(leftEdgeNode, node);
                Q_ASSERT(m_edgeList.validate());
            }
            break;
        case MergeVertex:
            leftEdgeNode = searchEdgeLeftOfPoint(m_edges.at(i).from);
            if (leftEdgeNode) {
                if (m_edges.at(m_edges.at(leftEdgeNode->data).helper).type == MergeVertex)
                    diagonals.add(QPair<int, int>(i, m_edges.at(leftEdgeNode->data).helper));
                m_edges.at(leftEdgeNode->data).helper = i;
            } else {
                qWarning("Inconsistent polygon. (#4)");
            }
            // Fall through.
        case EndVertex:
            if (m_clockwiseOrder) {
                if (m_edges.at(m_edges.at(i).helper).type == MergeVertex)
                    diagonals.add(QPair<int, int>(i, m_edges.at(i).helper));
                if (m_edges.at(i).node) {
                    m_edgeList.deleteNode(m_edges.at(i).node);
                    Q_ASSERT(m_edgeList.validate());
                } else {
                    qWarning("Inconsistent polygon. (#5)");
                }
            } else {
                if (m_edges.at(m_edges.at(j).helper).type == MergeVertex)
                    diagonals.add(QPair<int, int>(i, m_edges.at(j).helper));
                if (m_edges.at(j).node) {
                    m_edgeList.deleteNode(m_edges.at(j).node);
                    Q_ASSERT(m_edgeList.validate());
                } else {
                    qWarning("Inconsistent polygon. (#6)");
                }
            }
            break;
        }
    }

    for (int i = 0; i < diagonals.size(); ++i)
        createDiagonal(diagonals.at(i).first, diagonals.at(i).second);
}

template <typename T>
bool QTriangulator<T>::SimpleToMonotone::CompareVertices::operator () (int i, int j) const
{
    if (m_parent->m_edges.at(i).from == m_parent->m_edges.at(j).from)
        return m_parent->m_edges.at(i).type > m_parent->m_edges.at(j).type;
    return m_parent->m_parent->m_vertices.at(m_parent->m_edges.at(i).from) >
        m_parent->m_parent->m_vertices.at(m_parent->m_edges.at(j).from);
}

//============================================================================//
//                     QTriangulator::MonotoneToTriangles                     //
//============================================================================//
template <typename T>
void QTriangulator<T>::MonotoneToTriangles::decompose()
{
    QVector<T> result;
    QDataBuffer<int> stack(m_parent->m_indices.size());
    m_first = 0;
    // Require at least three more indices.
    while (m_first + 3 <= m_parent->m_indices.size()) {
        m_length = 0;
        while (m_parent->m_indices.at(m_first + m_length) != T(-1)) { // Q_TRIANGULATE_END_OF_POLYGON
            ++m_length;
            Q_ASSERT(m_first + m_length < m_parent->m_indices.size());
        }
        if (m_length < 3) {
            m_first += m_length + 1;
            continue;
        }

        int minimum = 0;
        while (less(next(minimum), minimum))
            minimum = next(minimum);
        while (less(previous(minimum), minimum))
            minimum = previous(minimum);

        stack.reset();
        stack.add(minimum);
        int left = previous(minimum);
        int right = next(minimum);
        bool stackIsOnLeftSide;
        bool clockwiseOrder = leftOfEdge(minimum, left, right);

        if (less(left, right)) {
            stack.add(left);
            left = previous(left);
            stackIsOnLeftSide = true;
        } else {
            stack.add(right);
            right = next(right);
            stackIsOnLeftSide = false;
        }

        for (int count = 0; count + 2 < m_length; ++count)
        {
            Q_ASSERT(stack.size() >= 2);
            if (less(left, right)) {
                if (stackIsOnLeftSide == false) {
                    for (int i = 0; i + 1 < stack.size(); ++i) {
                        result.push_back(indices(stack.at(i + 1)));
                        result.push_back(indices(left));
                        result.push_back(indices(stack.at(i)));
                    }
                    stack.first() = stack.last();
                    stack.resize(1);
                } else {
                    while (stack.size() >= 2 && (clockwiseOrder ^ !leftOfEdge(left, stack.at(stack.size() - 2), stack.last()))) {
                        result.push_back(indices(stack.at(stack.size() - 2)));
                        result.push_back(indices(left));
                        result.push_back(indices(stack.last()));
                        stack.pop_back();
                    }
                }
                stack.add(left);
                left = previous(left);
                stackIsOnLeftSide = true;
            } else {
                if (stackIsOnLeftSide == true) {
                    for (int i = 0; i + 1 < stack.size(); ++i) {
                        result.push_back(indices(stack.at(i)));
                        result.push_back(indices(right));
                        result.push_back(indices(stack.at(i + 1)));
                    }
                    stack.first() = stack.last();
                    stack.resize(1);
                } else {
                    while (stack.size() >= 2 && (clockwiseOrder ^ !leftOfEdge(right, stack.last(), stack.at(stack.size() - 2)))) {
                        result.push_back(indices(stack.last()));
                        result.push_back(indices(right));
                        result.push_back(indices(stack.at(stack.size() - 2)));
                        stack.pop_back();
                    }
                }
                stack.add(right);
                right = next(right);
                stackIsOnLeftSide = false;
            }
        }

        m_first += m_length + 1;
    }
    m_parent->m_indices = result;
}

//============================================================================//
//                                qTriangulate                                //
//============================================================================//

static bool hasElementIndexUint()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context)
        return false;
    return static_cast<QOpenGLExtensions *>(context->functions())->hasOpenGLExtension(QOpenGLExtensions::ElementIndexUint);
}

Q_GUI_EXPORT QTriangleSet qTriangulate(const qreal *polygon,
                          int count, uint hint, const QTransform &matrix)
{
    QTriangleSet triangleSet;
    if (hasElementIndexUint()) {
        QTriangulator<quint32> triangulator;
        triangulator.initialize(polygon, count, hint, matrix);
        QVertexSet<quint32> vertexSet = triangulator.triangulate();
        triangleSet.vertices = vertexSet.vertices;
        triangleSet.indices.setDataUint(vertexSet.indices);

    } else {
        QTriangulator<quint16> triangulator;
        triangulator.initialize(polygon, count, hint, matrix);
        QVertexSet<quint16> vertexSet = triangulator.triangulate();
        triangleSet.vertices = vertexSet.vertices;
        triangleSet.indices.setDataUshort(vertexSet.indices);
    }
    return triangleSet;
}

Q_GUI_EXPORT QTriangleSet qTriangulate(const QVectorPath &path,
                          const QTransform &matrix, qreal lod)
{
    QTriangleSet triangleSet;
    if (hasElementIndexUint()) {
        QTriangulator<quint32> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint32> vertexSet = triangulator.triangulate();
        triangleSet.vertices = vertexSet.vertices;
        triangleSet.indices.setDataUint(vertexSet.indices);
    } else {
        QTriangulator<quint16> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint16> vertexSet = triangulator.triangulate();
        triangleSet.vertices = vertexSet.vertices;
        triangleSet.indices.setDataUshort(vertexSet.indices);
    }
    return triangleSet;
}

QTriangleSet qTriangulate(const QPainterPath &path,
                          const QTransform &matrix, qreal lod)
{
    QTriangleSet triangleSet;
    if (hasElementIndexUint()) {
        QTriangulator<quint32> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint32> vertexSet = triangulator.triangulate();
        triangleSet.vertices = vertexSet.vertices;
        triangleSet.indices.setDataUint(vertexSet.indices);
    } else {
        QTriangulator<quint16> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint16> vertexSet = triangulator.triangulate();
        triangleSet.vertices = vertexSet.vertices;
        triangleSet.indices.setDataUshort(vertexSet.indices);
    }
    return triangleSet;
}

QPolylineSet qPolyline(const QVectorPath &path,
                       const QTransform &matrix, qreal lod)
{
    QPolylineSet polyLineSet;
    if (hasElementIndexUint()) {
        QTriangulator<quint32> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint32> vertexSet = triangulator.polyline();
        polyLineSet.vertices = vertexSet.vertices;
        polyLineSet.indices.setDataUint(vertexSet.indices);
    } else {
        QTriangulator<quint16> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint16> vertexSet = triangulator.polyline();
        polyLineSet.vertices = vertexSet.vertices;
        polyLineSet.indices.setDataUshort(vertexSet.indices);
    }
    return polyLineSet;
}

QPolylineSet qPolyline(const QPainterPath &path,
                       const QTransform &matrix, qreal lod)
{
    QPolylineSet polyLineSet;
    if (hasElementIndexUint()) {
        QTriangulator<quint32> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint32> vertexSet = triangulator.polyline();
        polyLineSet.vertices = vertexSet.vertices;
        polyLineSet.indices.setDataUint(vertexSet.indices);
    } else {
        QTriangulator<quint16> triangulator;
        triangulator.initialize(path, matrix, lod);
        QVertexSet<quint16> vertexSet = triangulator.polyline();
        polyLineSet.vertices = vertexSet.vertices;
        polyLineSet.indices.setDataUshort(vertexSet.indices);
    }
    return polyLineSet;
}

QT_END_NAMESPACE

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

#include "qtessellator_p.h"

#include <QRect>
#include <QList>
#include <QDebug>

#include <qmath.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

//#define DEBUG
#ifdef DEBUG
#define QDEBUG qDebug
#else
#define QDEBUG if (1){} else qDebug
#endif

static const bool emit_clever = true;
static const bool mark_clever = false;

enum VertexFlags {
    LineBeforeStarts = 0x1,
    LineBeforeEnds = 0x2,
    LineBeforeHorizontal = 0x4,
    LineAfterStarts = 0x8,
    LineAfterEnds = 0x10,
    LineAfterHorizontal = 0x20
};



class QTessellatorPrivate {
public:
    struct Vertices;

    QTessellatorPrivate() {}

    QRectF collectAndSortVertices(const QPointF *points, int *maxActiveEdges);
    void cancelCoincidingEdges();

    void emitEdges(QTessellator *tessellator);
    void processIntersections();
    void removeEdges();
    void addEdges();
    void addIntersections();

    struct Vertex : public QTessellator::Vertex
    {
        int flags;
    };

    struct Intersection
    {
        Q27Dot5 y;
        int edge;
        bool operator <(const Intersection &other) const {
            if (y != other.y)
                return y < other.y;
            return edge < other.edge;
        }
    };
    struct IntersectionLink
    {
        int next;
        int prev;
    };
    typedef QMap<Intersection, IntersectionLink> Intersections;

    struct Edge {
        Edge(const Vertices &v, int _edge);
        int edge;
        const Vertex *v0;
        const Vertex *v1;
        Q27Dot5 y_left;
        Q27Dot5 y_right;
        signed int winding : 8;
        bool mark;
        bool free;
        bool intersect_left;
        bool intersect_right;
        bool isLeftOf(const Edge &other, Q27Dot5 y) const;
        Q27Dot5 positionAt(Q27Dot5 y) const;
        bool intersect(const Edge &other, Q27Dot5 *y, bool *det_positive) const;

    };

    class EdgeSorter
    {
    public:
        EdgeSorter(int _y) : y(_y) {}
        bool operator() (const Edge *e1, const Edge *e2);
        int y;
    };

    class Scanline {
    public:
        Scanline();
        ~Scanline();

        void init(int maxActiveEdges);
        void done();

        int findEdgePosition(Q27Dot5 x, Q27Dot5 y) const;
        int findEdgePosition(const Edge &e) const;
        int findEdge(int edge) const;
        void clearMarks();

        void swap(int p1, int p2) {
            Edge *tmp = edges[p1];
            edges[p1] = edges[p2];
            edges[p2] = tmp;
        }
        void insert(int pos, const Edge &e);
        void removeAt(int pos);
        void markEdges(int pos1, int pos2);

        void prepareLine();
        void lineDone();

        Edge **old;
        int old_size;

        Edge **edges;
        int size;

    private:
        Edge *edge_table;
        int first_unused;
        int max_edges;
        enum { default_alloc = 32 };
    };

    struct Vertices {
        enum { default_alloc = 128 };
        Vertices();
        ~Vertices();
        void init(int maxVertices);
        void done();
        Vertex *storage;
        Vertex **sorted;

        Vertex *operator[] (int i) { return storage + i; }
        const Vertex *operator[] (int i) const { return storage + i; }
        int position(const Vertex *v) const {
            return v - storage;
        }
        Vertex *next(Vertex *v) {
            ++v;
            if (v == storage + nPoints)
                v = storage;
            return v;
        }
        const Vertex *next(const Vertex *v) const {
            ++v;
            if (v == storage + nPoints)
                v = storage;
            return v;
        }
        int nextPos(const Vertex *v) const {
            ++v;
            if (v == storage + nPoints)
                return 0;
            return v - storage;
        }
        Vertex *prev(Vertex *v) {
            if (v == storage)
                v = storage + nPoints;
            --v;
            return v;
        }
        const Vertex *prev(const Vertex *v) const {
            if (v == storage)
                v = storage + nPoints;
            --v;
            return v;
        }
        int prevPos(const Vertex *v) const {
            if (v == storage)
                v = storage + nPoints;
            --v;
            return v - storage;
        }
        int nPoints;
        int allocated;
    };
    Vertices vertices;
    Intersections intersections;
    Scanline scanline;
    bool winding;
    Q27Dot5 y;
    int currentVertex;

private:
    void addIntersection(const Edge *e1, const Edge *e2);
    bool edgeInChain(Intersection i, int edge);
};


QTessellatorPrivate::Edge::Edge(const QTessellatorPrivate::Vertices &vertices, int edge)
{
    this->edge = edge;
    intersect_left = intersect_right = true;
    mark = false;
    free = false;

    v0 = vertices[edge];
    v1 = vertices.next(v0);

    Q_ASSERT(v0->y != v1->y);

    if (v0->y > v1->y) {
        qSwap(v0, v1);
        winding = -1;
    } else {
        winding = 1;
    }
    y_left = y_right = v0->y;
}

// This is basically the algorithm from graphics gems. The algorithm
// is cubic in the coordinates at one place.  Since we use 64bit
// integers, this implies, that the allowed range for our coordinates
// is limited to 21 bits.  With 5 bits behind the decimal, this
// implies that differences in coordaintes can range from 2*SHORT_MIN
// to 2*SHORT_MAX, giving us efficiently a coordinate system from
// SHORT_MIN to SHORT_MAX.
//

// WARNING: It's absolutely critical that the intersect() and isLeftOf() methods use
// exactly the same algorithm to calulate yi. It's also important to be sure the algorithms
// are transitive (ie. the conditions below are true for all input data):
//
// a.intersect(b) == b.intersect(a)
// a.isLeftOf(b) != b.isLeftOf(a)
//
// This is tricky to get right, so be very careful when changing anything in here!

static inline bool sameSign(qint64 a, qint64 b) {
    return (((qint64) ((quint64) a ^ (quint64) b)) >= 0 );
}

bool QTessellatorPrivate::Edge::intersect(const Edge &other, Q27Dot5 *y, bool *det_positive) const
{
    qint64 a1 = v1->y - v0->y;
    qint64 b1 = v0->x - v1->x;

    qint64 a2 = other.v1->y - other.v0->y;
    qint64 b2 = other.v0->x - other.v1->x;

    qint64 det = a1 * b2 - a2 * b1;
    if (det == 0)
        return false;

    qint64 c1 = qint64(v1->x) * v0->y - qint64(v0->x) * v1->y;

    qint64 r3 = a1 * other.v0->x + b1 * other.v0->y + c1;
    qint64 r4 = a1 * other.v1->x + b1 * other.v1->y + c1;

    // Check signs of r3 and r4.  If both point 3 and point 4 lie on
    // same side of line 1, the line segments do not intersect.
    QDEBUG() << "        " << r3 << r4;
    if (r3 != 0 && r4 != 0 && sameSign( r3, r4 ))
        return false;

    qint64 c2 = qint64(other.v1->x) * other.v0->y - qint64(other.v0->x) * other.v1->y;

    qint64 r1 = a2 * v0->x + b2 * v0->y + c2;
    qint64 r2 = a2 * v1->x + b2 * v1->y + c2;

    // Check signs of r1 and r2.  If both point 1 and point 2 lie
    // on same side of second line segment, the line segments do not intersect.
    QDEBUG() << "        " << r1 << r2;
    if (r1 != 0 && r2 != 0 && sameSign( r1, r2 ))
        return false;

    // The det/2 is to get rounding instead of truncating.  It
    // is added or subtracted to the numerator, depending upon the
    // sign of the numerator.
    qint64 offset = det < 0 ? -det : det;
    offset >>= 1;

    qint64 num = a2 * c1 - a1 * c2;
    *y = ( num < 0 ? num - offset : num + offset ) / det;

    *det_positive = (det > 0);

    return true;
}

#undef SAME_SIGNS

bool QTessellatorPrivate::Edge::isLeftOf(const Edge &other, Q27Dot5 y) const
{
//     QDEBUG() << "isLeftOf" << edge << other.edge << y;
    qint64 a1 = v1->y - v0->y;
    qint64 b1 = v0->x - v1->x;
    qint64 a2 = other.v1->y - other.v0->y;
    qint64 b2 = other.v0->x - other.v1->x;

    qint64 c2 = qint64(other.v1->x) * other.v0->y - qint64(other.v0->x) * other.v1->y;

    qint64 det = a1 * b2 - a2 * b1;
    if (det == 0) {
        // lines are parallel. Only need to check side of one point
        // fixed ordering for coincident edges
        qint64 r1 = a2 * v0->x + b2 * v0->y + c2;
//         QDEBUG() << "det = 0" << r1;
        if (r1 == 0)
            return edge < other.edge;
        return (r1 < 0);
    }

    // not parallel, need to find the y coordinate of the intersection point
    qint64 c1 = qint64(v1->x) * v0->y - qint64(v0->x) * v1->y;

    qint64 offset = det < 0 ? -det : det;
    offset >>= 1;

    qint64 num = a2 * c1 - a1 * c2;
    qint64 yi = ( num < 0 ? num - offset : num + offset ) / det;
//     QDEBUG() << "    num=" << num << "offset=" << offset << "det=" << det;

    return ((yi > y) ^ (det < 0));
}

static inline bool compareVertex(const QTessellatorPrivate::Vertex *p1,
                                 const QTessellatorPrivate::Vertex *p2)
{
    if (p1->y == p2->y) {
        if (p1->x == p2->x)
            return p1 < p2;
        return p1->x < p2->x;
    }
    return p1->y < p2->y;
}

Q27Dot5 QTessellatorPrivate::Edge::positionAt(Q27Dot5 y) const
{
    if (y == v0->y)
        return v0->x;
    else if (y == v1->y)
        return v1->x;

    qint64 d = v1->x - v0->x;
    return (v0->x + d*(y - v0->y)/(v1->y-v0->y));
}

bool QTessellatorPrivate::EdgeSorter::operator() (const Edge *e1, const Edge *e2)
{
    return e1->isLeftOf(*e2, y);
}


QTessellatorPrivate::Scanline::Scanline()
{
    edges = 0;
    edge_table = 0;
    old = 0;
}

void QTessellatorPrivate::Scanline::init(int maxActiveEdges)
{
    maxActiveEdges *= 2;
    if (!edges || maxActiveEdges > default_alloc) {
        max_edges = maxActiveEdges;
        int s = qMax(maxActiveEdges + 1, default_alloc + 1);
        edges = q_check_ptr((Edge **)realloc(edges, s*sizeof(Edge *)));
        edge_table = q_check_ptr((Edge *)realloc(edge_table, s*sizeof(Edge)));
        old = q_check_ptr((Edge **)realloc(old, s*sizeof(Edge *)));
    }
    size = 0;
    old_size = 0;
    first_unused = 0;
    for (int i = 0; i < maxActiveEdges; ++i)
        edge_table[i].edge = i+1;
    edge_table[maxActiveEdges].edge = -1;
}

void QTessellatorPrivate::Scanline::done()
{
    if (max_edges > default_alloc) {
        free(edges);
        free(old);
        free(edge_table);
        edges = 0;
        old = 0;
        edge_table = 0;
    }
}

QTessellatorPrivate::Scanline::~Scanline()
{
    free(edges);
    free(old);
    free(edge_table);
}

int QTessellatorPrivate::Scanline::findEdgePosition(Q27Dot5 x, Q27Dot5 y) const
{
    int min = 0;
    int max = size - 1;
    while (min < max) {
        int pos = min + ((max - min + 1) >> 1);
        Q27Dot5 ax = edges[pos]->positionAt(y);
        if (ax > x) {
            max = pos - 1;
        } else {
            min = pos;
        }
    }
    return min;
}

int QTessellatorPrivate::Scanline::findEdgePosition(const Edge &e) const
{
//     qDebug() << ">>      findEdgePosition";
    int min = 0;
    int max = size;
    while (min < max) {
        int pos = min + ((max - min) >> 1);
//         qDebug() << "        " << min << max << pos << edges[pos]->isLeftOf(e, e.y0);
        if (edges[pos]->isLeftOf(e, e.v0->y)) {
            min = pos + 1;
        } else {
            max = pos;
        }
    }
//     qDebug() << "<<      findEdgePosition got" << min;
    return min;
}

int QTessellatorPrivate::Scanline::findEdge(int edge) const
{
    for (int i = 0; i < size; ++i) {
        int item_edge = edges[i]->edge;
        if (item_edge == edge)
            return i;
    }
    //Q_ASSERT(false);
    return -1;
}

void QTessellatorPrivate::Scanline::clearMarks()
{
    for (int i = 0; i < size; ++i) {
        edges[i]->mark = false;
        edges[i]->intersect_left = false;
        edges[i]->intersect_right = false;
    }
}

void QTessellatorPrivate::Scanline::prepareLine()
{
    Edge **end = edges + size;
    Edge **e = edges;
    Edge **o = old;
    while (e < end) {
        *o = *e;
        ++o;
        ++e;
    }
    old_size = size;
}

void QTessellatorPrivate::Scanline::lineDone()
{
    Edge **end = old + old_size;
    Edge **e = old;
    while (e < end) {
        if ((*e)->free) {
            (*e)->edge = first_unused;
            first_unused = (*e - edge_table);
        }
        ++e;
    }
}

void QTessellatorPrivate::Scanline::insert(int pos, const Edge &e)
{
    Edge *edge = edge_table + first_unused;
    first_unused = edge->edge;
    Q_ASSERT(first_unused != -1);
    *edge = e;
    memmove(edges + pos + 1, edges + pos, (size - pos)*sizeof(Edge *));
    edges[pos] = edge;
    ++size;
}

void QTessellatorPrivate::Scanline::removeAt(int pos)
{
    Edge *e = edges[pos];
    e->free = true;
    --size;
    memmove(edges + pos, edges + pos + 1, (size - pos)*sizeof(Edge *));
}

void QTessellatorPrivate::Scanline::markEdges(int pos1, int pos2)
{
    if (pos2 < pos1)
        return;

    for (int i = pos1; i <= pos2; ++i)
        edges[i]->mark = true;
}


QTessellatorPrivate::Vertices::Vertices()
{
    storage = 0;
    sorted = 0;
    allocated = 0;
    nPoints = 0;
}

QTessellatorPrivate::Vertices::~Vertices()
{
    if (storage) {
        free(storage);
        free(sorted);
    }
}

void QTessellatorPrivate::Vertices::init(int maxVertices)
{
    if (!storage || maxVertices > allocated) {
        int size = qMax((int)default_alloc, maxVertices);
        storage = q_check_ptr((Vertex *)realloc(storage, size*sizeof(Vertex)));
        sorted = q_check_ptr((Vertex **)realloc(sorted, size*sizeof(Vertex *)));
        allocated = maxVertices;
    }
}

void QTessellatorPrivate::Vertices::done()
{
    if (allocated > default_alloc) {
        free(storage);
        free(sorted);
        storage = 0;
        sorted = 0;
        allocated = 0;
    }
}



static inline void fillTrapezoid(Q27Dot5 y1, Q27Dot5 y2, int left, int right,
                                 const QTessellatorPrivate::Vertices &vertices,
                                 QTessellator::Trapezoid *trap)
{
    trap->top = y1;
    trap->bottom = y2;
    const QTessellatorPrivate::Vertex *v = vertices[left];
    trap->topLeft = v;
    trap->bottomLeft = vertices.next(v);
    if (trap->topLeft->y > trap->bottomLeft->y)
        qSwap(trap->topLeft,trap->bottomLeft);
    v = vertices[right];
    trap->topRight = v;
    trap->bottomRight = vertices.next(v);
    if (trap->topRight->y > trap->bottomRight->y)
        qSwap(trap->topRight, trap->bottomRight);
}

QRectF QTessellatorPrivate::collectAndSortVertices(const QPointF *points, int *maxActiveEdges)
{
    *maxActiveEdges = 0;
    Vertex *v = vertices.storage;
    Vertex **vv = vertices.sorted;

    qreal xmin(points[0].x());
    qreal xmax(points[0].x());
    qreal ymin(points[0].y());
    qreal ymax(points[0].y());

    // collect vertex data
    Q27Dot5 y_prev = FloatToQ27Dot5(points[vertices.nPoints-1].y());
    Q27Dot5 x_next = FloatToQ27Dot5(points[0].x());
    Q27Dot5 y_next = FloatToQ27Dot5(points[0].y());
    int j = 0;
    int i = 0;
    while (i < vertices.nPoints) {
        Q27Dot5 y_curr = y_next;

        *vv = v;

        v->x = x_next;
        v->y = y_next;
        v->flags = 0;

    next_point:

        xmin = qMin(xmin, points[i+1].x());
        xmax = qMax(xmax, points[i+1].x());
        ymin = qMin(ymin, points[i+1].y());
        ymax = qMax(ymax, points[i+1].y());

        y_next = FloatToQ27Dot5(points[i+1].y());
        x_next = FloatToQ27Dot5(points[i+1].x());

        // skip vertices on top of each other
        if (v->x == x_next && v->y == y_next) {
            ++i;
            if (i < vertices.nPoints)
                goto next_point;
            Vertex *v0 = vertices.storage;
            v0->flags &= ~(LineBeforeStarts|LineBeforeEnds|LineBeforeHorizontal);
            if (y_prev < y_curr)
                v0->flags |= LineBeforeEnds;
            else if (y_prev > y_curr)
                v0->flags |= LineBeforeStarts;
            else
                v0->flags |= LineBeforeHorizontal;
            if ((v0->flags & (LineBeforeStarts|LineAfterStarts))
                && !(v0->flags & (LineAfterEnds|LineBeforeEnds)))
                *maxActiveEdges += 2;
            break;
        }

        if (y_prev < y_curr)
            v->flags |= LineBeforeEnds;
        else if (y_prev > y_curr)
            v->flags |= LineBeforeStarts;
        else
            v->flags |= LineBeforeHorizontal;


        if (y_curr < y_next)
            v->flags |= LineAfterStarts;
        else if (y_curr > y_next)
            v->flags |= LineAfterEnds;
        else
            v->flags |= LineAfterHorizontal;
        // ### could probably get better limit by looping over sorted list and counting down on ending edges
        if ((v->flags & (LineBeforeStarts|LineAfterStarts))
            && !(v->flags & (LineAfterEnds|LineBeforeEnds)))
            *maxActiveEdges += 2;
        y_prev = y_curr;
        ++v;
        ++vv;
        ++j;
        ++i;
    }
    vertices.nPoints = j;

    QDEBUG() << "maxActiveEdges=" << *maxActiveEdges;
    vv = vertices.sorted;
    qSort(vv, vv + vertices.nPoints, compareVertex);

    return QRectF(xmin, ymin, xmax-xmin, ymax-ymin);
}

struct QCoincidingEdge {
    QTessellatorPrivate::Vertex *start;
    QTessellatorPrivate::Vertex *end;
    bool used;
    bool before;

    inline bool operator<(const QCoincidingEdge &e2) const
    {
        return end->y == e2.end->y ? end->x < e2.end->x : end->y < e2.end->y;
    }
};

static void cancelEdges(QCoincidingEdge &e1, QCoincidingEdge &e2)
{
    if (e1.before) {
        e1.start->flags &= ~(LineBeforeStarts|LineBeforeHorizontal);
        e1.end->flags &= ~(LineAfterEnds|LineAfterHorizontal);
    } else {
        e1.start->flags &= ~(LineAfterStarts|LineAfterHorizontal);
        e1.end->flags &= ~(LineBeforeEnds|LineBeforeHorizontal);
    }
    if (e2.before) {
        e2.start->flags &= ~(LineBeforeStarts|LineBeforeHorizontal);
        e2.end->flags &= ~(LineAfterEnds|LineAfterHorizontal);
    } else {
        e2.start->flags &= ~(LineAfterStarts|LineAfterHorizontal);
        e2.end->flags &= ~(LineBeforeEnds|LineBeforeHorizontal);
    }
    e1.used = e2.used = true;
}

void QTessellatorPrivate::cancelCoincidingEdges()
{
    Vertex **vv = vertices.sorted;

    QCoincidingEdge *tl = 0;
    int tlSize = 0;

    for (int i = 0; i < vertices.nPoints - 1; ++i) {
        Vertex *v = vv[i];
        int testListSize = 0;
        while (i < vertices.nPoints - 1) {
            Vertex *n = vv[i];
            if (v->x != n->x || v->y != n->y)
                break;

            if (testListSize > tlSize - 2) {
                tlSize = qMax(tlSize*2, 16);
                tl = q_check_ptr((QCoincidingEdge *)realloc(tl, tlSize*sizeof(QCoincidingEdge)));
            }
            if (n->flags & (LineBeforeStarts|LineBeforeHorizontal)) {
                tl[testListSize].start = n;
                tl[testListSize].end = vertices.prev(n);
                tl[testListSize].used = false;
                tl[testListSize].before = true;
                ++testListSize;
            }
            if (n->flags & (LineAfterStarts|LineAfterHorizontal)) {
                tl[testListSize].start = n;
                tl[testListSize].end = vertices.next(n);
                tl[testListSize].used = false;
                tl[testListSize].before = false;
                ++testListSize;
            }
            ++i;
        }
        if (!testListSize)
            continue;

        qSort(tl, tl + testListSize);

        for (int j = 0; j < testListSize; ++j) {
            if (tl[j].used)
                continue;

            for (int k = j + 1; k < testListSize; ++k) {
                if (tl[j].end->x != tl[k].end->x
                    || tl[j].end->y != tl[k].end->y
                    || tl[k].used)
                    break;

                if (!winding || tl[j].before != tl[k].before) {
                    cancelEdges(tl[j], tl[k]);
                    break;
                }
                ++k;
            }
            ++j;
        }
    }
    free(tl);
}


void QTessellatorPrivate::emitEdges(QTessellator *tessellator)
{
    //QDEBUG() << "TRAPS:";
    if (!scanline.old_size)
        return;

    // emit edges
    if (winding) {
        // winding fill rule
        int w = 0;

        scanline.old[0]->y_left = y;

        for (int i = 0; i < scanline.old_size - 1; ++i) {
            Edge *left = scanline.old[i];
            Edge *right = scanline.old[i+1];
            w += left->winding;
//             qDebug() << "i=" << i << "edge->winding=" << left->winding << "winding=" << winding;
            if (w == 0) {
                left->y_right = y;
                right->y_left = y;
            } else if (!emit_clever || left->mark || right->mark) {
                Q27Dot5 top = qMax(left->y_right, right->y_left);
                if (top != y) {
                    QTessellator::Trapezoid trap;
                    fillTrapezoid(top, y, left->edge, right->edge, vertices, &trap);
                    tessellator->addTrap(trap);
//                     QDEBUG() << "    top=" << Q27Dot5ToDouble(top) << "left=" << left->edge << "right=" << right->edge;
                }
                right->y_left = y;
                left->y_right = y;
            }
            left->mark = false;
        }
        if (scanline.old[scanline.old_size - 1]->mark) {
            scanline.old[scanline.old_size - 1]->y_right = y;
            scanline.old[scanline.old_size - 1]->mark = false;
        }
    } else {
        // odd-even fill rule
        for (int i = 0; i < scanline.old_size; i += 2) {
            Edge *left = scanline.old[i];
            Edge *right = scanline.old[i+1];
            if (!emit_clever || left->mark || right->mark) {
                Q27Dot5 top = qMax(left->y_right, right->y_left);
                if (top != y) {
                    QTessellator::Trapezoid trap;
                    fillTrapezoid(top, y, left->edge, right->edge, vertices, &trap);
                    tessellator->addTrap(trap);
                }
//                 QDEBUG() << "    top=" << Q27Dot5ToDouble(top) << "left=" << left->edge << "right=" << right->edge;
                left->y_left = y;
                left->y_right = y;
                right->y_left = y;
                right->y_right = y;
                left->mark = right->mark = false;
            }
        }
    }
}


void QTessellatorPrivate::processIntersections()
{
    QDEBUG() << "PROCESS INTERSECTIONS";
    // process intersections
    while (!intersections.isEmpty()) {
        Intersections::iterator it = intersections.begin();
        if (it.key().y != y)
            break;

        // swap edges
        QDEBUG() << "    swapping intersecting edges ";
        int min = scanline.size;
        int max = 0;
        Q27Dot5 xmin = INT_MAX;
        Q27Dot5 xmax = INT_MIN;
        int num = 0;
        while (1) {
            const Intersection &i = it.key();
            int next = it->next;

            int edgePos = scanline.findEdge(i.edge);
            if (edgePos >= 0) {
                ++num;
                min = qMin(edgePos, min);
                max = qMax(edgePos, max);
                Edge *edge = scanline.edges[edgePos];
                xmin = qMin(xmin, edge->positionAt(y));
                xmax = qMax(xmax, edge->positionAt(y));
            }
            Intersection key;
            key.y = y;
            key.edge = next;
            it = intersections.find(key);
            intersections.remove(i);
            if (it == intersections.end())
                break;
        }
        if (num < 2)
            continue;

        Q_ASSERT(min != max);
        QDEBUG() << "sorting between" << min << "and" << max << "xpos=" << xmin << xmax;
        while (min > 0 && scanline.edges[min - 1]->positionAt(y) >= xmin) {
            QDEBUG() << "    adding edge on left";
            --min;
        }
        while (max + 1 < scanline.size && scanline.edges[max + 1]->positionAt(y) <=  xmax) {
            QDEBUG() << "    adding edge on right";
            ++max;
        }

        qSort(scanline.edges + min, scanline.edges + max + 1, EdgeSorter(y));
#ifdef DEBUG
        for (int i = min; i <= max; ++i)
            QDEBUG() << "        " << scanline.edges[i]->edge << "at pos" << i;
#endif
        for (int i = min; i <= max; ++i) {
            Edge *edge = scanline.edges[i];
            edge->intersect_left = true;
            edge->intersect_right = true;
            edge->mark = true;
        }
    }
}

void QTessellatorPrivate::removeEdges()
{
    int cv = currentVertex;
    while (cv < vertices.nPoints) {
        const Vertex *v = vertices.sorted[cv];
        if (v->y > y)
            break;
        if (v->flags & LineBeforeEnds) {
            QDEBUG() << "    removing edge" << vertices.prevPos(v);
            int pos = scanline.findEdge(vertices.prevPos(v));
            if (pos == -1)
                continue;
            scanline.edges[pos]->mark = true;
            if (pos > 0)
                scanline.edges[pos - 1]->intersect_right = true;
            if (pos < scanline.size - 1)
                scanline.edges[pos + 1]->intersect_left = true;
            scanline.removeAt(pos);
        }
        if (v->flags & LineAfterEnds) {
            QDEBUG() << "    removing edge" << vertices.position(v);
            int pos = scanline.findEdge(vertices.position(v));
            if (pos == -1)
                continue;
            scanline.edges[pos]->mark = true;
            if (pos > 0)
                scanline.edges[pos - 1]->intersect_right = true;
            if (pos < scanline.size - 1)
                scanline.edges[pos + 1]->intersect_left = true;
            scanline.removeAt(pos);
        }
        ++cv;
    }
}

void QTessellatorPrivate::addEdges()
{
    while (currentVertex < vertices.nPoints) {
        const Vertex *v = vertices.sorted[currentVertex];
        if (v->y > y)
            break;
        if (v->flags & LineBeforeStarts) {
            // add new edge
            int start = vertices.prevPos(v);
            Edge e(vertices, start);
            int pos = scanline.findEdgePosition(e);
            QDEBUG() << "    adding edge" << start << "at position" << pos;
            scanline.insert(pos, e);
            if (!mark_clever || !(v->flags & LineAfterEnds)) {
                if (pos > 0)
                    scanline.edges[pos - 1]->mark = true;
                if (pos < scanline.size - 1)
                    scanline.edges[pos + 1]->mark = true;
            }
        }
        if (v->flags & LineAfterStarts) {
            Edge e(vertices, vertices.position(v));
            int pos = scanline.findEdgePosition(e);
            QDEBUG() << "    adding edge" << vertices.position(v) << "at position" << pos;
            scanline.insert(pos, e);
            if (!mark_clever || !(v->flags & LineBeforeEnds)) {
                if (pos > 0)
                    scanline.edges[pos - 1]->mark = true;
                if (pos < scanline.size - 1)
                    scanline.edges[pos + 1]->mark = true;
            }
        }
        if (v->flags & LineAfterHorizontal) {
            int pos1 = scanline.findEdgePosition(v->x, v->y);
            const Vertex *next = vertices.next(v);
            Q_ASSERT(v->y == next->y);
            int pos2 = scanline.findEdgePosition(next->x, next->y);
            if (pos2 < pos1)
                qSwap(pos1, pos2);
            if (pos1 > 0)
                --pos1;
            if (pos2 == scanline.size)
                --pos2;
            //QDEBUG() << "marking horizontal edge from " << pos1 << "to" << pos2;
            scanline.markEdges(pos1, pos2);
        }
        ++currentVertex;
    }
}

#ifdef DEBUG
static void checkLinkChain(const QTessellatorPrivate::Intersections &intersections,
                           QTessellatorPrivate::Intersection i)
{
//     qDebug() << "              Link chain: ";
    int end = i.edge;
    while (1) {
        QTessellatorPrivate::IntersectionLink l = intersections.value(i);
//         qDebug() << "                     " << i.edge << "next=" << l.next << "prev=" << l.prev;
        if (l.next == end)
            break;
        Q_ASSERT(l.next != -1);
        Q_ASSERT(l.prev != -1);

        QTessellatorPrivate::Intersection i2 = i;
        i2.edge = l.next;
        QTessellatorPrivate::IntersectionLink l2 = intersections.value(i2);

        Q_ASSERT(l2.next != -1);
        Q_ASSERT(l2.prev != -1);
        Q_ASSERT(l.next == i2.edge);
        Q_ASSERT(l2.prev == i.edge);
        i = i2;
    }
}
#endif

bool QTessellatorPrivate::edgeInChain(Intersection i, int edge)
{
    int end = i.edge;
    while (1) {
        if (i.edge == edge)
            return true;
        IntersectionLink l = intersections.value(i);
        if (l.next == end)
            break;
        Q_ASSERT(l.next != -1);
        Q_ASSERT(l.prev != -1);

        Intersection i2 = i;
        i2.edge = l.next;

#ifndef QT_NO_DEBUG
        IntersectionLink l2 = intersections.value(i2);
        Q_ASSERT(l2.next != -1);
        Q_ASSERT(l2.prev != -1);
        Q_ASSERT(l.next == i2.edge);
        Q_ASSERT(l2.prev == i.edge);
#endif
        i = i2;
    }
    return false;
}


void QTessellatorPrivate::addIntersection(const Edge *e1, const Edge *e2)
{
    const IntersectionLink emptyLink = {-1, -1};

    int next = vertices.nextPos(vertices[e1->edge]);
    if (e2->edge == next)
        return;
    int prev = vertices.prevPos(vertices[e1->edge]);
    if (e2->edge == prev)
        return;

    Q27Dot5 yi;
    bool det_positive;
    bool isect = e1->intersect(*e2, &yi, &det_positive);
    QDEBUG("checking edges %d and %d", e1->edge, e2->edge);
    if (!isect) {
        QDEBUG() << "    no intersection";
        return;
    }

    // don't emit an intersection if it's at the start of a line segment or above us
    if (yi <= y) {
        if (!det_positive)
            return;
        QDEBUG() << "        ----->>>>>> WRONG ORDER!";
        yi = y;
    }
    QDEBUG() << "   between edges " << e1->edge << "and" << e2->edge << "at point ("
             << Q27Dot5ToDouble(yi) << ')';

    Intersection i1;
    i1.y = yi;
    i1.edge = e1->edge;
    IntersectionLink link1 = intersections.value(i1, emptyLink);
    Intersection i2;
    i2.y = yi;
    i2.edge = e2->edge;
    IntersectionLink link2 = intersections.value(i2, emptyLink);

    // new pair of edges
    if (link1.next == -1 && link2.next == -1) {
        link1.next = link1.prev = i2.edge;
        link2.next = link2.prev = i1.edge;
    } else if (link1.next == i2.edge || link1.prev == i2.edge
               || link2.next == i1.edge || link2.prev == i1.edge) {
#ifdef DEBUG
        checkLinkChain(intersections, i1);
        checkLinkChain(intersections, i2);
        Q_ASSERT(edgeInChain(i1, i2.edge));
#endif
        return;
    } else if (link1.next == -1 || link2.next == -1) {
        if (link2.next == -1) {
            qSwap(i1, i2);
            qSwap(link1, link2);
        }
        Q_ASSERT(link1.next == -1);
#ifdef DEBUG
        checkLinkChain(intersections, i2);
#endif
        // only i2 in list
        link1.next = i2.edge;
        link1.prev = link2.prev;
        link2.prev = i1.edge;
        Intersection other;
        other.y = yi;
        other.edge = link1.prev;
        IntersectionLink link = intersections.value(other, emptyLink);
        Q_ASSERT(link.next == i2.edge);
        Q_ASSERT(link.prev != -1);
        link.next = i1.edge;
        intersections.insert(other, link);
    } else {
        bool connected = edgeInChain(i1, i2.edge);
        if (connected)
            return;
#ifdef DEBUG
        checkLinkChain(intersections, i1);
        checkLinkChain(intersections, i2);
#endif
        // both already in some list. Have to make sure they are connected
        // this can be done by cutting open the ring(s) after the two eges and
        // connecting them again
        Intersection other1;
        other1.y = yi;
        other1.edge = link1.next;
        IntersectionLink linko1 = intersections.value(other1, emptyLink);
        Intersection other2;
        other2.y = yi;
        other2.edge = link2.next;
        IntersectionLink linko2 = intersections.value(other2, emptyLink);

        linko1.prev = i2.edge;
        link2.next = other1.edge;

        linko2.prev = i1.edge;
        link1.next = other2.edge;
        intersections.insert(other1, linko1);
        intersections.insert(other2, linko2);
    }
    intersections.insert(i1, link1);
    intersections.insert(i2, link2);
#ifdef DEBUG
    checkLinkChain(intersections, i1);
    checkLinkChain(intersections, i2);
    Q_ASSERT(edgeInChain(i1, i2.edge));
#endif
    return;

}


void QTessellatorPrivate::addIntersections()
{
    if (scanline.size) {
        QDEBUG() << "INTERSECTIONS";
        // check marked edges for intersections
#ifdef DEBUG
        for (int i = 0; i < scanline.size; ++i) {
            Edge *e = scanline.edges[i];
            QDEBUG() << "    " << i << e->edge << "isect=(" << e->intersect_left << e->intersect_right
                     << ')';
        }
#endif

        for (int i = 0; i < scanline.size - 1; ++i) {
            Edge *e1 = scanline.edges[i];
            Edge *e2 = scanline.edges[i + 1];
            // check for intersection
            if (e1->intersect_right || e2->intersect_left)
                addIntersection(e1, e2);
        }
    }
#if 0
    if (intersections.constBegin().key().y == y) {
        QDEBUG() << "----------------> intersection on same line";
        scanline.clearMarks();
        scanline.processIntersections(y, &intersections);
        goto redo;
    }
#endif
}


QTessellator::QTessellator()
{
    d = new QTessellatorPrivate;
}

QTessellator::~QTessellator()
{
    delete d;
}

void QTessellator::setWinding(bool w)
{
    d->winding = w;
}


QRectF QTessellator::tessellate(const QPointF *points, int nPoints)
{
    Q_ASSERT(points[0] == points[nPoints-1]);
    --nPoints;

#ifdef DEBUG
    QDEBUG()<< "POINTS:";
    for (int i = 0; i < nPoints; ++i) {
        QDEBUG() << points[i];
    }
#endif

    // collect edges and calculate bounds
    d->vertices.nPoints = nPoints;
    d->vertices.init(nPoints);

    int maxActiveEdges = 0;
    QRectF br = d->collectAndSortVertices(points, &maxActiveEdges);
    d->cancelCoincidingEdges();

#ifdef DEBUG
    QDEBUG() << "nPoints = " << nPoints << "using " << d->vertices.nPoints;
    QDEBUG()<< "VERTICES:";
    for (int i = 0; i < d->vertices.nPoints; ++i) {
        QDEBUG() << "    " << i << ": "
                 << "point=" << d->vertices.position(d->vertices.sorted[i])
                 << "flags=" << d->vertices.sorted[i]->flags
                 << "pos=(" << Q27Dot5ToDouble(d->vertices.sorted[i]->x) << '/'
                 << Q27Dot5ToDouble(d->vertices.sorted[i]->y) << ')';
    }
#endif

    d->scanline.init(maxActiveEdges);
    d->y = INT_MIN/256;
    d->currentVertex = 0;

    while (d->currentVertex < d->vertices.nPoints) {
        d->scanline.clearMarks();

        d->y = d->vertices.sorted[d->currentVertex]->y;
        if (!d->intersections.isEmpty())
            d->y = qMin(d->y, d->intersections.constBegin().key().y);

        QDEBUG()<< "===== SCANLINE: y =" << Q27Dot5ToDouble(d->y) << " =====";

        d->scanline.prepareLine();
        d->processIntersections();
        d->removeEdges();
        d->addEdges();
        d->addIntersections();
        d->emitEdges(this);
        d->scanline.lineDone();

#ifdef DEBUG
        QDEBUG()<< "===== edges:";
        for (int i = 0; i < d->scanline.size; ++i) {
            QDEBUG() << "   " << d->scanline.edges[i]->edge
                     << "p0= (" << Q27Dot5ToDouble(d->scanline.edges[i]->v0->x)
                     << '/' << Q27Dot5ToDouble(d->scanline.edges[i]->v0->y)
                     << ") p1= (" << Q27Dot5ToDouble(d->scanline.edges[i]->v1->x)
                     << '/' << Q27Dot5ToDouble(d->scanline.edges[i]->v1->y) << ')'
                     << "x=" << Q27Dot5ToDouble(d->scanline.edges[i]->positionAt(d->y))
                     << "isLeftOfNext="
                     << ((i < d->scanline.size - 1)
                         ? d->scanline.edges[i]->isLeftOf(*d->scanline.edges[i+1], d->y)
                         : true);
        }
#endif
}

    d->scanline.done();
    d->intersections.clear();
    return br;
}

// tessellates the given convex polygon
void QTessellator::tessellateConvex(const QPointF *points, int nPoints)
{
    Q_ASSERT(points[0] == points[nPoints-1]);
    --nPoints;

    d->vertices.nPoints = nPoints;
    d->vertices.init(nPoints);

    for (int i = 0; i < nPoints; ++i) {
        d->vertices[i]->x = FloatToQ27Dot5(points[i].x());
        d->vertices[i]->y = FloatToQ27Dot5(points[i].y());
    }

    int left = 0, right = 0;

    int top = 0;
    for (int i = 1; i < nPoints; ++i) {
        if (d->vertices[i]->y < d->vertices[top]->y)
            top = i;
    }

    left = (top + nPoints - 1) % nPoints;
    right = (top + 1) % nPoints;

    while (d->vertices[left]->x == d->vertices[top]->x && d->vertices[left]->y == d->vertices[top]->y && left != right)
        left = (left + nPoints - 1) % nPoints;

    while (d->vertices[right]->x == d->vertices[top]->x && d->vertices[right]->y == d->vertices[top]->y && left != right)
        right = (right + 1) % nPoints;

    if (left == right)
        return;

    int dir = 1;

    Vertex dLeft = { d->vertices[top]->x - d->vertices[left]->x,
                     d->vertices[top]->y - d->vertices[left]->y };

    Vertex dRight = { d->vertices[right]->x - d->vertices[top]->x,
                      d->vertices[right]->y - d->vertices[top]->y };

    Q27Dot5 cross = dLeft.x * dRight.y - dLeft.y * dRight.x;

    // flip direction if polygon is clockwise
    if (cross < 0 || (cross == 0 && dLeft.x > 0)) {
        qSwap(left, right);
        dir = -1;
    }

    Vertex *lastLeft = d->vertices[top];
    Vertex *lastRight = d->vertices[top];

    QTessellator::Trapezoid trap;

    while (lastLeft->y == d->vertices[left]->y && left != right) {
        lastLeft = d->vertices[left];
        left = (left + nPoints - dir) % nPoints;
    }

    while (lastRight->y == d->vertices[right]->y && left != right) {
        lastRight = d->vertices[right];
        right = (right + nPoints + dir) % nPoints;
    }

    while (true) {
        trap.top = qMax(lastRight->y, lastLeft->y);
        trap.bottom = qMin(d->vertices[left]->y, d->vertices[right]->y);
        trap.topLeft = lastLeft;
        trap.topRight = lastRight;
        trap.bottomLeft = d->vertices[left];
        trap.bottomRight = d->vertices[right];

        if (trap.bottom > trap.top)
            addTrap(trap);

        if (left == right)
            break;

        if (d->vertices[right]->y < d->vertices[left]->y) {
            do {
                lastRight = d->vertices[right];
                right = (right + nPoints + dir) % nPoints;
            }
            while (lastRight->y == d->vertices[right]->y && left != right);
        } else {
            do {
                lastLeft = d->vertices[left];
                left = (left + nPoints - dir) % nPoints;
            }
            while (lastLeft->y == d->vertices[left]->y && left != right);
        }
    }
}

// tessellates the stroke of the line from a_ to b_ with the given width and a flat cap
void QTessellator::tessellateRect(const QPointF &a_, const QPointF &b_, qreal width)
{
    Vertex a = { FloatToQ27Dot5(a_.x()), FloatToQ27Dot5(a_.y()) };
    Vertex b = { FloatToQ27Dot5(b_.x()), FloatToQ27Dot5(b_.y()) };

    QPointF pa = a_, pb = b_;

    if (a.y > b.y) {
        qSwap(a, b);
        qSwap(pa, pb);
    }

    Vertex delta = { b.x - a.x, b.y - a.y };

    if (delta.x == 0 && delta.y == 0)
        return;

    qreal hw = qreal(0.5) * width;

    if (delta.x == 0) {
        Q27Dot5 halfWidth = FloatToQ27Dot5(hw);

        if (halfWidth == 0)
            return;

        Vertex topLeft = { a.x - halfWidth, a.y };
        Vertex topRight = { a.x + halfWidth, a.y };
        Vertex bottomLeft = { a.x - halfWidth, b.y };
        Vertex bottomRight = { a.x + halfWidth, b.y };

        QTessellator::Trapezoid trap = { topLeft.y, bottomLeft.y, &topLeft, &bottomLeft, &topRight, &bottomRight };
        addTrap(trap);
    } else if (delta.y == 0) {
        Q27Dot5 halfWidth = FloatToQ27Dot5(hw);

        if (halfWidth == 0)
            return;

        if (a.x > b.x)
            qSwap(a.x, b.x);

        Vertex topLeft = { a.x, a.y - halfWidth };
        Vertex topRight = { b.x, a.y - halfWidth };
        Vertex bottomLeft = { a.x, a.y + halfWidth };
        Vertex bottomRight = { b.x, a.y + halfWidth };

        QTessellator::Trapezoid trap = { topLeft.y, bottomLeft.y, &topLeft, &bottomLeft, &topRight, &bottomRight };
        addTrap(trap);
    } else {
        QPointF perp(pb.y() - pa.y(), pa.x() - pb.x());
        qreal length = qSqrt(perp.x() * perp.x() + perp.y() * perp.y());

        if (qFuzzyIsNull(length))
            return;

        // need the half of the width
        perp *= hw / length;

        QPointF pta = pa + perp;
        QPointF ptb = pa - perp;
        QPointF ptc = pb - perp;
        QPointF ptd = pb + perp;

        Vertex ta = { FloatToQ27Dot5(pta.x()), FloatToQ27Dot5(pta.y()) };
        Vertex tb = { FloatToQ27Dot5(ptb.x()), FloatToQ27Dot5(ptb.y()) };
        Vertex tc = { FloatToQ27Dot5(ptc.x()), FloatToQ27Dot5(ptc.y()) };
        Vertex td = { FloatToQ27Dot5(ptd.x()), FloatToQ27Dot5(ptd.y()) };

        if (ta.y < tb.y) {
            if (tb.y < td.y) {
                QTessellator::Trapezoid top = { ta.y, tb.y, &ta, &tb, &ta, &td };
                QTessellator::Trapezoid bottom = { td.y, tc.y, &tb, &tc, &td, &tc };
                addTrap(top);
                addTrap(bottom);

                QTessellator::Trapezoid middle = { tb.y, td.y, &tb, &tc, &ta, &td };
                addTrap(middle);
            } else {
                QTessellator::Trapezoid top = { ta.y, td.y, &ta, &tb, &ta, &td };
                QTessellator::Trapezoid bottom = { tb.y, tc.y, &tb, &tc, &td, &tc };
                addTrap(top);
                addTrap(bottom);

                if (tb.y != td.y) {
                    QTessellator::Trapezoid middle = { td.y, tb.y, &ta, &tb, &td, &tc };
                    addTrap(middle);
                }
            }
        } else {
            if (ta.y < tc.y) {
                QTessellator::Trapezoid top = { tb.y, ta.y, &tb, &tc, &tb, &ta };
                QTessellator::Trapezoid bottom = { tc.y, td.y, &tc, &td, &ta, &td };
                addTrap(top);
                addTrap(bottom);

                QTessellator::Trapezoid middle = { ta.y, tc.y, &tb, &tc, &ta, &td };
                addTrap(middle);
            } else {
                QTessellator::Trapezoid top = { tb.y, tc.y, &tb, &tc, &tb, &ta };
                QTessellator::Trapezoid bottom = { ta.y, td.y, &tc, &td, &ta, &td };
                addTrap(top);
                addTrap(bottom);

                if (ta.y != tc.y) {
                    QTessellator::Trapezoid middle = { tc.y, ta.y, &tc, &td, &tb, &ta };
                    addTrap(middle);
                }
            }
        }
    }
}

QT_END_NAMESPACE

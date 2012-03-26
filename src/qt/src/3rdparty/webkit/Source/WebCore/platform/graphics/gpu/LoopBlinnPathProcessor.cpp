/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"

#include "LoopBlinnPathProcessor.h"

#include "FloatPoint.h"
#include "FloatRect.h"
#include "LoopBlinnClassifier.h"
#include "LoopBlinnConstants.h"
#include "LoopBlinnLocalTriangulator.h"
#include "LoopBlinnMathUtils.h"
#include "LoopBlinnPathCache.h"
#include "LoopBlinnTextureCoords.h"
#include "PODArena.h"
#include "PODIntervalTree.h"
#include "Path.h"
#include "internal_glu.h"
#include <algorithm>
#include <wtf/Assertions.h>
#include <wtf/FastMalloc.h>
#include <wtf/UnusedParam.h>


#if USE(SKIA)
#include "SkGeometry.h"
#include "SkPath.h"
#include "SkScalar.h"
#else
// Must port to your platform.
#endif

namespace WebCore {

using LoopBlinnMathUtils::XRay;
using LoopBlinnMathUtils::chopCubicAt;
using LoopBlinnMathUtils::numXRayCrossingsForCubic;
using LoopBlinnMathUtils::trianglesOverlap;
using LoopBlinnMathUtils::xRayCrossesLine;
using LoopBlinnPathProcessorImplementation::Contour;
using LoopBlinnPathProcessorImplementation::Segment;

namespace {

#ifndef NDEBUG
String valueToString(const FloatRect& arg)
{
    StringBuilder builder;
    builder.append("[FloatRect x=");
    builder.append(String::number(arg.x()));
    builder.append(" y=");
    builder.append(String::number(arg.y()));
    builder.append(" maxX=");
    builder.append(String::number(arg.maxX()));
    builder.append(" maxY=");
    builder.append(String::number(arg.maxY()));
    builder.append("]");
    return builder.toString();
}
#endif

struct SweepData;

} // anonymous namespace

namespace LoopBlinnPathProcessorImplementation {
class Segment;
}

#ifndef NDEBUG
// Routines needed to print the types of IntervalNodes we instantiate
// in this file.
template <>
struct ValueToString<float> {
    static String string(const float& value)
    {
        return String::number(value);
    }
};

template <>
struct ValueToString<SweepData*> {
    static String string(SweepData* const& value)
    {
        return String::format("0x%p", value);
    }
};

template <>
struct ValueToString<LoopBlinnPathProcessorImplementation::Segment*> {
    static String string(LoopBlinnPathProcessorImplementation::Segment* const& value)
    {
        return String::format("0x%p", value);
    }
};
#endif

namespace LoopBlinnPathProcessorImplementation {

//----------------------------------------------------------------------
// Segment
//

// Describes a segment of the path: either a cubic or a line segment.
// These are stored in a doubly linked list to speed up curve
// subdivision, which occurs due to either rendering artifacts in the
// loop case or due to overlapping triangles.
class Segment {
    WTF_MAKE_NONCOPYABLE(Segment);
public:
    enum Kind {
        Cubic,
        Line
    };

    // No-argument constructor allows construction by the PODArena class.
    Segment()
         : m_arena(0)
         , m_kind(Cubic)
         , m_prev(0)
         , m_next(0)
         , m_contour(0)
         , m_triangulator(0)
         , m_markedForSubdivision(false)
    {
    }

    // Initializer for cubic curve segments.
    void setup(PODArena* arena,
               Contour* contour,
               FloatPoint cp0,
               FloatPoint cp1,
               FloatPoint cp2,
               FloatPoint cp3)
    {
        m_arena = arena;
        m_contour = contour;
        m_kind = Cubic;
        m_points[0] = cp0;
        m_points[1] = cp1;
        m_points[2] = cp2;
        m_points[3] = cp3;
        computeBoundingBox();
    }

    // Initializer for line segments.
    void setup(PODArena* arena,
               Contour* contour,
               FloatPoint p0,
               FloatPoint p1)
    {
        m_arena = arena;
        m_contour = contour;
        m_kind = Line;
        m_points[0] = p0;
        m_points[1] = p1;
        computeBoundingBox();
    }

    Kind kind() const { return m_kind; }

    // Returns the i'th control point, 0 <= i < 4.
    const FloatPoint& getPoint(int i)
    {
        ASSERT(i >= 0 && i < 4);
        return m_points[i];
    }

    Segment* next() const { return m_next; }
    Segment* prev() const { return m_prev; }

    void setNext(Segment* next) { m_next = next; }
    void setPrev(Segment* prev) { m_prev = prev; }

    // The contour this segment belongs to.
    Contour* contour() const { return m_contour; }

    // Subdivides the current segment at the given parameter value (0 <=
    // t <= 1) and replaces it with the two newly created Segments in
    // the linked list, if possible. Returns a pointer to the leftmost
    // Segment.
    Segment* subdivide(float param)
    {
        FloatPoint dst[7];
        chopCubicAt(m_points, dst, param);
        Segment* left = m_arena->allocateObject<Segment>();
        Segment* right = m_arena->allocateObject<Segment>();
        left->setup(m_arena, m_contour, dst[0], dst[1], dst[2], dst[3]);
        right->setup(m_arena, m_contour, dst[3], dst[4], dst[5], dst[6]);
        left->setNext(right);
        right->setPrev(left);
        // Try to set up a link between "this->prev()" and "left".
        if (prev()) {
            left->setPrev(prev());
            prev()->setNext(left);
        }
        // Try to set up a link between "this->next()" and "right".
        Segment* n = next();
        if (n) {
            right->setNext(n);
            n->setPrev(right);
        }
        // Set up a link between "this" and "left"; this is only to
        // provide a certain amount of continuity during forward iteration.
        setNext(left);
        return left;
    }

    // Subdivides the current segment at the halfway point and replaces
    // it with the two newly created Segments in the linked list, if
    // possible. Returns a pointer to the leftmost Segment.
    Segment* subdivide() { return subdivide(0.5f); }

    const FloatRect& boundingBox() const { return m_boundingBox; }

    // Computes the number of times a query line starting at the given
    // point and extending to x=+infinity crosses this segment. Outgoing
    // "ambiguous" argument indicates whether the query intersected an
    // endpoint or tangent point of the segment, indicating that another
    // query point is preferred.
    int numCrossingsForXRay(const XRay& xRay, bool& ambiguous) const
    {
        if (m_kind == Cubic)
            // Should consider caching the monotonic cubics.
            return numXRayCrossingsForCubic(xRay, m_points, ambiguous);

        return xRayCrossesLine(xRay, m_points, ambiguous) ? 1 : 0;
    }

    // Performs a local triangulation of the control points in this
    // segment. This operation only makes sense for cubic type segments.
    // texCoords may be null when the klm coordinates have not been
    // computed yet.
    void triangulate(LoopBlinnLocalTriangulator::InsideEdgeComputation computeInsideEdges,
                     const LoopBlinnTextureCoords::Result* texCoords);

    // Returns the number of control point triangles associated with
    // this segment.
    int numberOfTriangles() const
    {
        if (!m_triangulator)
            return 0;
        return m_triangulator->numberOfTriangles();
    }

    // Fetches the given control point triangle for this segment.
    LoopBlinnLocalTriangulator::Triangle* getTriangle(int index)
    {
        ASSERT(m_triangulator);
        return m_triangulator->getTriangle(index);
    }

    // Number of vertices along the inside edge of this segment. This
    // can be called either for line or cubic type segments.
    int numberOfInteriorVertices() const
    {
        if (m_kind == Cubic) {
            if (m_triangulator)
                return m_triangulator->numberOfInteriorVertices();

            return 0;
        }

        return 2;
    }

    // Returns the given interior vertex, 0 <= index < numberOfInteriorVertices().
    FloatPoint getInteriorVertex(int index) const
    {
        ASSERT(index >= 0 && index < numberOfInteriorVertices());
        if (m_kind == Cubic) {
            FloatPoint res;
            if (m_triangulator) {
                LoopBlinnLocalTriangulator::Vertex* vertex = m_triangulator->getInteriorVertex(index);
                if (vertex)
                    res.set(vertex->xyCoordinates().x(), vertex->xyCoordinates().y());
            }
            return res;
        }

        return m_points[index];
    }

    // State to assist with curve subdivision.
    bool markedForSubdivision() const { return m_markedForSubdivision; }
    void setMarkedForSubdivision(bool markedForSubdivision) { m_markedForSubdivision = markedForSubdivision; }

#ifndef NDEBUG
    // Suppport for printing Segments.
    String toString() const
    {
        StringBuilder builder;
        builder.append("[Segment kind=");
        builder.append(kind() == Line ? "line" : "cubic");
        builder.append(" boundingBox=");
        builder.append(valueToString(boundingBox()));
        builder.append(" contour=0x");
        builder.append(String::format("%p", contour()));
        builder.append(" markedForSubdivision=");
        builder.append(markedForSubdivision() ? "true" : "false");
        builder.append("]");
        return builder.toString();
    }
#endif

 private:
    // Computes the bounding box of this Segment.
    void computeBoundingBox()
    {
        switch (m_kind) {
        case Cubic:
            m_boundingBox.fitToPoints(m_points[0], m_points[1], m_points[2], m_points[3]);
            break;

        case Line:
            m_boundingBox.fitToPoints(m_points[0], m_points[1]);
            break;
        }
    }

    PODArena* m_arena;
    Kind m_kind;
    FloatPoint m_points[4];
    Segment* m_prev;
    Segment* m_next;
    Contour* m_contour;
    FloatRect m_boundingBox;
    LoopBlinnLocalTriangulator* m_triangulator;
    bool m_markedForSubdivision;
};

//----------------------------------------------------------------------
// Contour
//

// Describes a closed contour of the path.
class Contour {
    WTF_MAKE_NONCOPYABLE(Contour);
public:
    Contour()
    {
        m_first = &m_sentinel;
        m_first->setNext(m_first);
        m_first->setPrev(m_first);
        m_isOrientedCounterClockwise = true;
        m_boundingBoxDirty = false;
        m_fillSide = LoopBlinnConstants::RightSide;
    }

    void add(Segment* segment)
    {
        if (m_first == &m_sentinel) {
            // First element is the sentinel. Replace it with the incoming
            // segment.
            segment->setNext(m_first);
            segment->setPrev(m_first);
            m_first->setNext(segment);
            m_first->setPrev(segment);
            m_first = segment;
        } else {
            // m_first->prev() is the sentinel.
            ASSERT(m_first->prev() == &m_sentinel);
            Segment* last = m_sentinel.prev();
            last->setNext(segment);
            segment->setPrev(last);
            segment->setNext(&m_sentinel);
            m_sentinel.setPrev(segment);
        }
        m_boundingBoxDirty = true;
    }

    // Subdivides the given segment at the given parametric value.
    // Returns a pointer to the first of the two portions of the
    // subdivided segment.
    Segment* subdivide(Segment* segment, float param)
    {
        Segment* left = segment->subdivide(param);
        if (m_first == segment)
            m_first = left;
        return left;
    }

    // Subdivides the given segment at the halfway point. Returns a
    // pointer to the first of the two portions of the subdivided
    // segment.
    Segment* subdivide(Segment* segment)
    {
        Segment* left = segment->subdivide();
        if (m_first == segment)
            m_first = left;
        return left;
    }

    // Returns the first segment in the contour for iteration.
    Segment* begin() const { return m_first; }

    // Returns the last segment in the contour for iteration. Callers
    // should not iterate over this segment. In other words:
    //  for (Segment* cur = contour->begin();
    //       cur != contour->end();
    //       cur = cur->next()) {
    //    // .. process cur ...
    //  }
    Segment* end()
    {
        ASSERT(m_first->prev() == &m_sentinel);
        return &m_sentinel;
    }

    bool isOrientedCounterClockwise() const { return m_isOrientedCounterClockwise; }
    void setIsOrientedCounterClockwise(bool isOrientedCounterClockwise) { m_isOrientedCounterClockwise = isOrientedCounterClockwise; }

    const FloatRect& boundingBox()
    {
        if (m_boundingBoxDirty) {
            bool first = true;
            for (Segment* cur = begin(); cur != end(); cur = cur->next()) {
                if (first)
                    m_boundingBox = cur->boundingBox();
                else
                    m_boundingBox.unite(cur->boundingBox());
                first = false;
            }

            m_boundingBoxDirty = false;
        }
        return m_boundingBox;
    }

    // Returns which side of this contour is filled.
    LoopBlinnConstants::FillSide fillSide() const
    {
        return m_fillSide;
    }

    void setFillSide(LoopBlinnConstants::FillSide fillSide)
    {
        m_fillSide = fillSide;
    }

private:
    // The start of the segment chain. The segments are kept in a
    // circular doubly linked list for rapid access to the beginning and
    // end.
    Segment* m_first;

    // The sentinel element at the end of the chain, needed for
    // reasonable iteration semantics.
    Segment m_sentinel;

    bool m_isOrientedCounterClockwise;

    FloatRect m_boundingBox;
    bool m_boundingBoxDirty;

    // Which side of this contour should be filled.
    LoopBlinnConstants::FillSide m_fillSide;
};

//----------------------------------------------------------------------
// Segment
//

// Definition of Segment::triangulate(), which must come after
// declaration of Contour.
void Segment::triangulate(LoopBlinnLocalTriangulator::InsideEdgeComputation computeInsideEdges,
                          const LoopBlinnTextureCoords::Result* texCoords)
{
    ASSERT(m_kind == Cubic);
    if (!m_triangulator)
        m_triangulator = m_arena->allocateObject<LoopBlinnLocalTriangulator>();
    m_triangulator->reset();
    for (int i = 0; i < 4; i++) {
        LoopBlinnLocalTriangulator::Vertex* vertex = m_triangulator->getVertex(i);
        if (texCoords) {
            vertex->set(getPoint(i).x(),
                        getPoint(i).y(),
                        texCoords->klmCoordinates[i].x(),
                        texCoords->klmCoordinates[i].y(),
                        texCoords->klmCoordinates[i].z());
        } else {
            vertex->set(getPoint(i).x(),
                        getPoint(i).y(),
                        // No texture coordinates yet
                        0, 0, 0);
        }
    }
    m_triangulator->triangulate(computeInsideEdges, contour()->fillSide());
}

} // namespace LoopBlinnPathProcessorImplementation

//----------------------------------------------------------------------
// LoopBlinnPathProcessor
//

LoopBlinnPathProcessor::LoopBlinnPathProcessor()
    : m_arena(PODArena::create())
#ifndef NDEBUG
    , m_verboseLogging(false)
#endif
{
}

LoopBlinnPathProcessor::LoopBlinnPathProcessor(PassRefPtr<PODArena> arena)
    : m_arena(arena)
#ifndef NDEBUG
    , m_verboseLogging(false)
#endif
{
}

LoopBlinnPathProcessor::~LoopBlinnPathProcessor()
{
}

void LoopBlinnPathProcessor::process(const Path& path, LoopBlinnPathCache& cache)
{
    buildContours(path);

    // Run plane-sweep algorithm to determine overlaps of control point
    // curves and subdivide curves appropriately.
    subdivideCurves();

    // Determine orientations of countours. Based on orientation and the
    // number of curve crossings at a random point on the contour,
    // determine whether to fill the left or right side of the contour.
    determineSidesToFill();

    // Classify curves, compute texture coordinates and subdivide as
    // necessary to eliminate rendering artifacts. Do the final
    // triangulation of the curve segments, determining the path along
    // the interior of the shape.
    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;
        for (Segment* seg = cur->begin(); seg != cur->end(); seg = seg->next()) {
            if (seg->kind() == Segment::Cubic) {
                LoopBlinnClassifier::Result classification = LoopBlinnClassifier::classify(seg->getPoint(0),
                                                                                           seg->getPoint(1),
                                                                                           seg->getPoint(2),
                                                                                           seg->getPoint(3));
#ifndef NDEBUG
                if (m_verboseLogging)
                    LOG_ERROR("Classification: %d", (int) classification.curveType);
#endif
                LoopBlinnTextureCoords::Result texCoords =
                    LoopBlinnTextureCoords::compute(classification, cur->fillSide());
                if (texCoords.hasRenderingArtifact) {
                    // FIXME: there is a problem where the algorithm
                    // sometimes fails to converge when splitting at the
                    // subdivision parameter value. For the time being,
                    // split halfway.
                    cur->subdivide(seg);
                    // Next iteration will handle the newly subdivided curves
                } else {
                    if (!texCoords.isLineOrPoint) {
                        seg->triangulate(LoopBlinnLocalTriangulator::ComputeInsideEdges, &texCoords);
                        for (int i = 0; i < seg->numberOfTriangles(); i++) {
                            LoopBlinnLocalTriangulator::Triangle* triangle = seg->getTriangle(i);
                            for (int j = 0; j < 3; j++) {
                                LoopBlinnLocalTriangulator::Vertex* vert = triangle->getVertex(j);
                                cache.addVertex(vert->xyCoordinates().x(),
                                                vert->xyCoordinates().y(),
                                                vert->klmCoordinates().x(),
                                                vert->klmCoordinates().y(),
                                                vert->klmCoordinates().z());
                            }
                        }
#ifdef LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES
                        // Show the end user the interior edges as well
                        for (int i = 1; i < seg->numberOfInteriorVertices(); i++) {
                            FloatPoint vert = seg->getInteriorVertex(i);
                            // Duplicate previous vertex to be able to draw GL_LINES
                            FloatPoint prev = seg->getInteriorVertex(i - 1);
                            cache.addInteriorEdgeVertex(prev.x(), prev.y());
                            cache.addInteriorEdgeVertex(vert.x(), vert.y());
                        }
#endif // LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES
                    }
                }
            }
        }
    }

    // Run the interior paths through a tessellation algorithm
    // supporting multiple contours.
    tessellateInterior(cache);
}

void LoopBlinnPathProcessor::buildContours(const Path& path)
{
    // Clear out the contours
    m_contours.clear();
#if USE(SKIA)
    SkPath::Iter iter(*path.platformPath(), false);
    SkPoint points[4];
    SkPath::Verb verb;
    Contour* contour = 0;
    SkPoint curPoint = { 0 };
    SkPoint moveToPoint = { 0 };
    do {
        verb = iter.next(points);
        if (verb != SkPath::kMove_Verb) {
            if (!contour) {
                contour = m_arena->allocateObject<Contour>();
                m_contours.append(contour);
            }
        }
        switch (verb) {
        case SkPath::kMove_Verb: {
            contour = m_arena->allocateObject<Contour>();
            m_contours.append(contour);
            curPoint = points[0];
            moveToPoint = points[0];
#ifndef NDEBUG
            if (m_verboseLogging)
                LOG_ERROR("MoveTo (%f, %f)", points[0].fX, points[0].fY);
#endif
            break;
        }
        case SkPath::kLine_Verb: {
            Segment* segment = m_arena->allocateObject<Segment>();
            if (iter.isCloseLine()) {
                segment->setup(m_arena.get(), contour, curPoint, points[1]);
#ifndef NDEBUG
                if (m_verboseLogging)
                    LOG_ERROR("CloseLineTo (%f, %f), (%f, %f)", curPoint.fX, curPoint.fY, points[1].fX, points[1].fY);
#endif
                contour->add(segment);
                contour = 0;
            } else {
                segment->setup(m_arena.get(), contour, points[0], points[1]);
#ifndef NDEBUG
                if (m_verboseLogging)
                    LOG_ERROR("LineTo (%f, %f), (%f, %f)", points[0].fX, points[0].fY, points[1].fX, points[1].fY);
#endif
                contour->add(segment);
                curPoint = points[1];
            }
            break;
        }
        case SkPath::kQuad_Verb: {
            // Need to degree elevate the quadratic into a cubic
            SkPoint cubic[4];
            SkConvertQuadToCubic(points, cubic);
            Segment* segment = m_arena->allocateObject<Segment>();
            segment->setup(m_arena.get(), contour,
                           cubic[0], cubic[1], cubic[2], cubic[3]);
#ifndef NDEBUG
            if (m_verboseLogging)
                LOG_ERROR("Quad->CubicTo (%f, %f), (%f, %f), (%f, %f), (%f, %f)", cubic[0].fX, cubic[0].fY, cubic[1].fX, cubic[1].fY, cubic[2].fX, cubic[2].fY, cubic[3].fX, cubic[3].fY);
#endif
            contour->add(segment);
            curPoint = cubic[3];
            break;
        }
        case SkPath::kCubic_Verb: {
            Segment* segment = m_arena->allocateObject<Segment>();
            segment->setup(m_arena.get(), contour, points[0], points[1], points[2], points[3]);
#ifndef NDEBUG
            if (m_verboseLogging)
                LOG_ERROR("CubicTo (%f, %f), (%f, %f), (%f, %f), (%f, %f)", points[0].fX, points[0].fY, points[1].fX, points[1].fY, points[2].fX, points[2].fY, points[3].fX, points[3].fY);
#endif
            contour->add(segment);
            curPoint = points[3];
            break;
        }
        case SkPath::kClose_Verb: {
            Segment* segment = m_arena->allocateObject<Segment>();
            segment->setup(m_arena.get(), contour, curPoint, moveToPoint);
#ifndef NDEBUG
            if (m_verboseLogging)
                LOG_ERROR("Close (%f, %f) -> (%f, %f)", curPoint.fX, curPoint.fY, moveToPoint.fX, moveToPoint.fY);
#endif
            contour->add(segment);
            contour = 0;
        }
        case SkPath::kDone_Verb:
            break;
        }
    } while (verb != SkPath::kDone_Verb);
#else // !USE(SKIA)
    UNUSED_PARAM(path);
    // Must port to your platform.
    ASSERT_NOT_REACHED();
#endif
}

#ifndef NDEBUG
Vector<Segment*> LoopBlinnPathProcessor::allSegmentsOverlappingY(Contour* queryContour, float x, float y)
{
    Vector<Segment*> res;
    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;
        for (Segment* seg = cur->begin(); seg != cur->end(); seg = seg->next()) {
            const FloatRect& boundingBox = seg->boundingBox();
            if (boundingBox.y() <= y && y <= boundingBox.maxY())
                res.append(seg);
        }
    }
    return res;
}
#endif

// Uncomment this to debug the orientation computation.
// #define GPU_PATH_PROCESSOR_DEBUG_ORIENTATION

void LoopBlinnPathProcessor::determineSidesToFill()
{
    // Loop and Blinn's algorithm can only easily emulate the even/odd
    // fill rule, and only for non-intersecting curves. We can determine
    // which side of each curve segment to fill based on its
    // clockwise/counterclockwise orientation and how many other
    // contours surround it.

    // To optimize the query of all curve segments intersecting a
    // horizontal line going to x=+infinity, we build up an interval
    // tree whose keys are the y extents of the segments.
    PODIntervalTree<float, Segment*> tree(m_arena);
    typedef PODIntervalTree<float, Segment*>::IntervalType IntervalType;

    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;
        determineOrientation(cur);
        for (Segment* seg = cur->begin(); seg != cur->end(); seg = seg->next()) {
            const FloatRect& boundingBox = seg->boundingBox();
            tree.add(tree.createInterval(boundingBox.y(), boundingBox.maxY(), seg));
        }
    }

    // Now iterate through the contours and pick a random segment (in
    // this case we use the first) and a random point on that segment.
    // Find all segments from other contours which intersect this one
    // and count the number of crossings a horizontal line to
    // x=+infinity makes with those contours. This combined with the
    // orientation of the curve tells us which side to fill -- again,
    // assuming an even/odd fill rule, which is all we can easily
    // handle.
    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;

        bool ambiguous = true;
        int numCrossings = 0;

        // For each contour, attempt to find a point on the contour which,
        // when we cast an XRay, does not intersect the other contours at
        // an ambiguous point (the junction between two curves or at a
        // tangent point). Ambiguous points make the determination of
        // whether this contour is contained within another fragile. Note
        // that this loop is only an approximation to the selection of a
        // good casting point. We could as well evaluate a segment to
        // determine a point upon it.
        for (Segment* seg = cur->begin();
             ambiguous && seg != cur->end();
             seg = seg->next()) {
            numCrossings = 0;
            // We use a zero-sized vertical interval for the query.
            Vector<IntervalType> overlaps = tree.allOverlaps(tree.createInterval(seg->getPoint(0).y(),
                                                                                 seg->getPoint(0).y(),
                                                                                 0));
#if defined(GPU_PATH_PROCESSOR_DEBUG_ORIENTATION) && !defined(NDEBUG)
            Vector<Segment*> slowOverlaps = allSegmentsOverlappingY(cur, seg->getPoint(0).x(), seg->getPoint(0).y());
            if (overlaps.size() != slowOverlaps.size()) {
                LOG_ERROR("For query point (%f, %f) on contour 0x%p:", seg->getPoint(0).x(), seg->getPoint(0).y(), cur);
                LOG_ERROR(" overlaps:");
                for (size_t i = 0; i < overlaps.size(); i++)
                    LOG_ERROR("  %d: %s", i+1, overlaps[i].data()->toString().ascii().data());
                LOG_ERROR(" slowOverlaps:");
                for (size_t i = 0; i < slowOverlaps.size(); i++)
                    LOG_ERROR("  %d: %s", (i+1) slowOverlaps[i]->toString());
                LOG_ERROR("Interval tree:");
                tree.dump();
            }
            ASSERT(overlaps.size() == slowOverlaps.size());
#endif // defined(GPU_PATH_PROCESSOR_DEBUG_ORIENTATION) && !defined(NDEBUG)
            for (Vector<IntervalType>::iterator iter = overlaps.begin(); iter != overlaps.end(); ++iter) {
                const IntervalType& interval = *iter;
                Segment* querySegment = interval.data();
                // Ignore segments coming from the same contour.
                if (querySegment->contour() != cur) {
                    // Only perform queries that can affect the computation.
                    const FloatRect& boundingBox = querySegment->contour()->boundingBox();
                    if (seg->getPoint(0).x() >= boundingBox.x()
                        && seg->getPoint(0).x() <= boundingBox.maxX()) {
                        numCrossings += querySegment->numCrossingsForXRay(seg->getPoint(0),
                                                                          ambiguous);
                        if (ambiguous) {
#ifndef NDEBUG
                            if (m_verboseLogging) {
                                LOG_ERROR("Ambiguous intersection query at point (%f, %f)", seg->getPoint(0).x(), seg->getPoint(0).y());
                                LOG_ERROR("Query segment: %s", querySegment->toString().ascii().data());
                            }
#endif
                            break; // Abort iteration over overlaps.
                        }
                    }
                }
            }
        } // for (Segment* seg = cur->begin(); ...

        cur->setFillSide((cur->isOrientedCounterClockwise() ^ (numCrossings & 1)) ? LoopBlinnConstants::LeftSide : LoopBlinnConstants::RightSide);
    }
}

void LoopBlinnPathProcessor::determineOrientation(Contour* contour)
{
    // Determine signed area of the polygon represented by the points
    // along the segments. Consider this an approximation to the true
    // orientation of the polygon; it probably won't handle
    // self-intersecting curves correctly.
    //
    // There is also a pretty basic assumption here that the contour is
    // closed.
    float signedArea = 0;
    for (Segment* seg = contour->begin();
         seg != contour->end();
         seg = seg->next()) {
        int limit = (seg->kind() == Segment::Cubic) ? 4 : 2;
        for (int i = 1; i < limit; i++) {
            const FloatPoint& prevPoint = seg->getPoint(i - 1);
            const FloatPoint& point = seg->getPoint(i);
            float curArea = prevPoint.x() * point.y() - prevPoint.y() * point.x();
#ifndef NDEBUG
            if (m_verboseLogging)
                LOG_ERROR("Adding to signed area (%f, %f) -> (%f, %f) = %f", prevPoint.x(), prevPoint.y(), point.x(), point.y(), curArea);
#endif
            signedArea += curArea;
        }
    }

    if (signedArea > 0)
        contour->setIsOrientedCounterClockwise(true);
    else
        contour->setIsOrientedCounterClockwise(false);
}

namespace {

//----------------------------------------------------------------------
// Classes and typedefs needed for curve subdivision. These can't be scoped
// within the subdivideCurves() method itself, because templates then fail
// to instantiate.

// The user data which is placed in the PODIntervalTree.
struct SweepData {
    SweepData()
        : triangle(0)
        , segment(0)
    {
    }

    // The triangle this interval is associated with
    LoopBlinnLocalTriangulator::Triangle* triangle;
    // The segment the triangle is associated with
    Segment* segment;
};

typedef PODIntervalTree<float, SweepData*> SweepTree;
typedef SweepTree::IntervalType SweepInterval;

// The entry / exit events which occur at the minimum and maximum x
// coordinates of the control point triangles' bounding boxes.
//
// Note that this class requires its copy constructor and assignment
// operator since it needs to be stored in a Vector.
class SweepEvent {
public:
    SweepEvent()
        : m_x(0)
        , m_entry(false)
        , m_interval(0, 0, 0)
    {
    }

    // Initializes the SweepEvent.
    void setup(float x, bool entry, SweepInterval interval)
    {
        m_x = x;
        m_entry = entry;
        m_interval = interval;
    }

    float x() const { return m_x; }
    bool entry() const { return m_entry; }
    const SweepInterval& interval() const { return m_interval; }

    bool operator<(const SweepEvent& other) const
    {
        return m_x < other.m_x;
    }

private:
    float m_x;
    bool m_entry;
    SweepInterval m_interval;
};

bool trianglesOverlap(LoopBlinnLocalTriangulator::Triangle* t0,
                      LoopBlinnLocalTriangulator::Triangle* t1)
{
    return trianglesOverlap(t0->getVertex(0)->xyCoordinates(),
                            t0->getVertex(1)->xyCoordinates(),
                            t0->getVertex(2)->xyCoordinates(),
                            t1->getVertex(0)->xyCoordinates(),
                            t1->getVertex(1)->xyCoordinates(),
                            t1->getVertex(2)->xyCoordinates());
}

} // anonymous namespace

void LoopBlinnPathProcessor::subdivideCurves()
{
    // We need to determine all overlaps of all control point triangles
    // (from different segments, not the same segment) and, if any
    // exist, subdivide the associated curves.
    //
    // The plane-sweep algorithm determines all overlaps of a set of
    // rectangles in the 2D plane. Our problem maps very well to this
    // algorithm and significantly reduces the complexity compared to a
    // naive implementation.
    //
    // Each bounding box of a control point triangle is converted into
    // an "entry" event at its smallest X coordinate and an "exit" event
    // at its largest X coordinate. Each event has an associated
    // one-dimensional interval representing the Y span of the bounding
    // box. We sort these events by increasing X coordinate. We then
    // iterate through them. For each entry event we add the interval to
    // a side interval tree, and query this tree for overlapping
    // intervals. Any overlapping interval corresponds to an overlapping
    // bounding box. For each exit event we remove the associated
    // interval from the interval tree.

    Vector<Segment*> curSegments;
    Vector<Segment*> nextSegments;

    // Start things off by considering all of the segments
    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;
        for (Segment* seg = cur->begin(); seg != cur->end(); seg = seg->next()) {
            if (seg->kind() == Segment::Cubic) {
                seg->triangulate(LoopBlinnLocalTriangulator::DontComputeInsideEdges, 0);
                curSegments.append(seg);
            }
        }
    }

    // Subdivide curves at most this many times
    const int MaxIterations = 5;
    Vector<SweepInterval> overlaps;

    for (int currentIteration = 0; currentIteration < MaxIterations; ++currentIteration) {
        if (!curSegments.size())
            // Done
            break;

        Vector<SweepEvent> events;
        SweepTree tree(m_arena);
        for (Vector<Segment*>::iterator iter = curSegments.begin(); iter != curSegments.end(); ++iter) {
            Segment* seg = *iter;
            ASSERT(seg->kind() == Segment::Cubic);
            for (int i = 0; i < seg->numberOfTriangles(); i++) {
                LoopBlinnLocalTriangulator::Triangle* triangle = seg->getTriangle(i);
                FloatRect boundingBox;
                boundingBox.fitToPoints(triangle->getVertex(0)->xyCoordinates(),
                                        triangle->getVertex(1)->xyCoordinates(),
                                        triangle->getVertex(2)->xyCoordinates());
                // Ignore zero-width triangles to avoid issues with
                // coincident entry and exit events for the same triangle
                if (boundingBox.maxX() > boundingBox.x()) {
                    SweepData* data = m_arena->allocateObject<SweepData>();
                    data->triangle = triangle;
                    data->segment = seg;
                    SweepInterval interval = tree.createInterval(boundingBox.y(), boundingBox.maxY(), data);
                    // Add entry and exit events
                    SweepEvent event;
                    event.setup(boundingBox.x(), true, interval);
                    events.append(event);
                    event.setup(boundingBox.maxX(), false, interval);
                    events.append(event);
                }
            }
        }

        // Sort events by increasing X coordinate
        std::sort(events.begin(), events.end());
#ifndef NDEBUG
        for (size_t ii = 1; ii < events.size(); ++ii)
            ASSERT(events[ii - 1].x() <= events[ii].x());
#endif

        // Now iterate through the events
        for (Vector<SweepEvent>::iterator iter = events.begin(); iter != events.end(); ++iter) {
            SweepEvent event = *iter;
            if (event.entry()) {
                // See whether the associated segment has been subdivided yet
                if (!event.interval().data()->segment->markedForSubdivision()) {
                    // Query the tree
                    overlaps.clear();
                    tree.allOverlaps(event.interval(), overlaps);
                    // Now see exactly which triangles overlap this one
                    for (Vector<SweepInterval>::iterator iter = overlaps.begin(); iter != overlaps.end(); ++iter) {
                        SweepInterval overlap = *iter;
                        // Only pay attention to overlaps from a different Segment
                        if (event.interval().data()->segment != overlap.data()->segment) {
                            // See whether the triangles actually overlap
                            if (trianglesOverlap(event.interval().data()->triangle,
                                                 overlap.data()->triangle)) {
                                // Actually subdivide the segments.
                                // Each one might already have been subdivided.
                                Segment* seg = event.interval().data()->segment;
                                conditionallySubdivide(seg, nextSegments);
                                seg = overlap.data()->segment;
                                conditionallySubdivide(seg, nextSegments);
                            }
                        }
                    }
                }
                // Add this interval into the tree
                tree.add(event.interval());
            } else {
                // Remove this interval from the tree
                tree.remove(event.interval());
            }
        }

        curSegments.swap(nextSegments);
        nextSegments.clear();
    }
}

void LoopBlinnPathProcessor::conditionallySubdivide(Segment* seg, Vector<Segment*>& nextSegments)
{
    if (!seg->markedForSubdivision()) {
        seg->setMarkedForSubdivision(true);
        Segment* next = seg->contour()->subdivide(seg);
        // Triangulate the newly subdivided segments.
        next->triangulate(LoopBlinnLocalTriangulator::DontComputeInsideEdges, 0);
        next->next()->triangulate(LoopBlinnLocalTriangulator::DontComputeInsideEdges, 0);
        // Add them for the next iteration.
        nextSegments.append(next);
        nextSegments.append(next->next());
    }
}

#ifndef NDEBUG
void LoopBlinnPathProcessor::subdivideCurvesSlow()
{
    // Alternate, significantly slower algorithm for curve subdivision
    // for use in debugging.
    Vector<Segment*> curSegments;
    Vector<Segment*> nextSegments;

    // Start things off by considering all of the segments
    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;
        for (Segment* seg = cur->begin(); seg != cur->end(); seg = seg->next()) {
            if (seg->kind() == Segment::Cubic) {
                seg->triangulate(LoopBlinnLocalTriangulator::DontComputeInsideEdges, 0);
                curSegments.append(seg);
            }
        }
    }

    // Subdivide curves at most this many times
    const int MaxIterations = 5;

    for (int currentIteration = 0; currentIteration < MaxIterations; ++currentIteration) {
        if (!curSegments.size())
            // Done
            break;

        for (Vector<Segment*>::iterator iter = curSegments.begin(); iter != curSegments.end(); ++iter) {
            Segment* seg = *iter;
            ASSERT(seg->kind() == Segment::Cubic);
            for (Vector<Segment*>::iterator iter2 = curSegments.begin();
                 iter2 != curSegments.end();
                 iter2++) {
                Segment* seg2 = *iter2;
                ASSERT(seg2->kind() == Segment::Cubic);
                if (seg != seg2) {
                    for (int i = 0; i < seg->numberOfTriangles(); i++) {
                        LoopBlinnLocalTriangulator::Triangle* triangle = seg->getTriangle(i);
                        for (int j = 0; j < seg2->numberOfTriangles(); j++) {
                            LoopBlinnLocalTriangulator::Triangle* triangle2 = seg2->getTriangle(j);
                            if (trianglesOverlap(triangle, triangle2)) {
                                conditionallySubdivide(seg, nextSegments);
                                conditionallySubdivide(seg2, nextSegments);
                            }
                        }
                    }
                }
            }
        }

        curSegments.swap(nextSegments);
        nextSegments.clear();
    }
}
#endif

namespace {

//----------------------------------------------------------------------
// Structures and callbacks for tessellation of the interior region of
// the contours.

// The user data for the GLU tessellator.
struct TessellationState {
    TessellationState(LoopBlinnPathCache& inputCache)
        : cache(inputCache) { }

    LoopBlinnPathCache& cache;
    Vector<void*> allocatedPointers;
};

static void vertexCallback(void* vertexData, void* data)
{
    TessellationState* state = static_cast<TessellationState*>(data);
    GLdouble* location = static_cast<GLdouble*>(vertexData);
    state->cache.addInteriorVertex(static_cast<float>(location[0]),
                                   static_cast<float>(location[1]));
}

static void combineCallback(GLdouble coords[3], void* vertexData[4],
                            GLfloat weight[4], void** outData,
                            void* polygonData)
{
    UNUSED_PARAM(vertexData);
    UNUSED_PARAM(weight);
    TessellationState* state = static_cast<TessellationState*>(polygonData);
    GLdouble* outVertex = static_cast<GLdouble*>(fastMalloc(3 * sizeof(GLdouble)));
    state->allocatedPointers.append(outVertex);
    outVertex[0] = coords[0];
    outVertex[1] = coords[1];
    outVertex[2] = coords[2];
    *outData = outVertex;
}

static void edgeFlagCallback(GLboolean)
{
    // No-op just to prevent triangle strips and fans from being passed to us.
    // See the OpenGL Programming Guide, Chapter 11, "Tessellators and Quadrics".
}

} // anonymous namespace

void LoopBlinnPathProcessor::tessellateInterior(LoopBlinnPathCache& cache)
{
    // Because the GLU tessellator requires its input in
    // double-precision format, we need to make a separate copy of the
    // data.
    Vector<GLdouble> vertexData;
    Vector<size_t> contourEndings;
    // For avoiding adding coincident vertices.
    float curX = 0, curY = 0;
    for (Vector<Contour*>::iterator iter = m_contours.begin(); iter != m_contours.end(); ++iter) {
        Contour* cur = *iter;
        bool first = true;
        for (Segment* seg = cur->begin(); seg != cur->end(); seg = seg->next()) {
            int numberOfInteriorVertices = seg->numberOfInteriorVertices();
            for (int i = 0; i < numberOfInteriorVertices - 1; i++) {
                FloatPoint point = seg->getInteriorVertex(i);
                if (first) {
                    first = false;
                    vertexData.append(point.x());
                    vertexData.append(point.y());
                    vertexData.append(0);
                    curX = point.x();
                    curY = point.y();
                } else if (point.x() != curX || point.y() != curY)  {
                    vertexData.append(point.x());
                    vertexData.append(point.y());
                    vertexData.append(0);
                    curX = point.x();
                    curY = point.y();
                }
            }
        }
        contourEndings.append(vertexData.size());
    }
    // Now that we have all of the vertex data in a stable location in
    // memory, call the tessellator.
    GLUtesselator* tess = internal_gluNewTess();
    TessellationState state(cache);
    internal_gluTessCallback(tess, GLU_TESS_VERTEX_DATA,
                             reinterpret_cast<GLvoid (*)()>(vertexCallback));
    internal_gluTessCallback(tess, GLU_TESS_COMBINE_DATA,
                             reinterpret_cast<GLvoid (*)()>(combineCallback));
    internal_gluTessCallback(tess, GLU_TESS_EDGE_FLAG,
                             reinterpret_cast<GLvoid (*)()>(edgeFlagCallback));
    internal_gluTessBeginPolygon(tess, &state);
    internal_gluTessBeginContour(tess);
    GLdouble* base = vertexData.data();
    int contourIndex = 0;
    for (size_t i = 0; i < vertexData.size(); i += 3) {
        if (i == contourEndings[contourIndex]) {
            internal_gluTessEndContour(tess);
            internal_gluTessBeginContour(tess);
            ++contourIndex;
        }
        internal_gluTessVertex(tess, &base[i], &base[i]);
    }
    internal_gluTessEndContour(tess);
    internal_gluTessEndPolygon(tess);
    for (size_t i = 0; i < state.allocatedPointers.size(); i++)
        fastFree(state.allocatedPointers[i]);
    internal_gluDeleteTess(tess);
}

} // namespace WebCore

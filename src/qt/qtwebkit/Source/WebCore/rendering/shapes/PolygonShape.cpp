/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PolygonShape.h"

#include <wtf/MathExtras.h>

namespace WebCore {

enum EdgeIntersectionType {
    Normal,
    VertexMinY,
    VertexMaxY,
    VertexYBoth
};

struct EdgeIntersection {
    const FloatPolygonEdge* edge;
    FloatPoint point;
    EdgeIntersectionType type;
};

static inline float leftSide(const FloatPoint& vertex1, const FloatPoint& vertex2, const FloatPoint& point)
{
    return ((point.x() - vertex1.x()) * (vertex2.y() - vertex1.y())) - ((vertex2.x() - vertex1.x()) * (point.y() - vertex1.y()));
}

static inline bool isReflexVertex(const FloatPoint& prevVertex, const FloatPoint& vertex, const FloatPoint& nextVertex)
{
    return leftSide(prevVertex, nextVertex, vertex) < 0;
}

static bool computeXIntersection(const FloatPolygonEdge* edgePointer, float y, EdgeIntersection& result)
{
    const FloatPolygonEdge& edge = *edgePointer;

    if (edge.minY() > y || edge.maxY() < y)
        return false;

    const FloatPoint& vertex1 = edge.vertex1();
    const FloatPoint& vertex2 = edge.vertex2();
    float dy = vertex2.y() - vertex1.y();

    float intersectionX;
    EdgeIntersectionType intersectionType;

    if (!dy) {
        intersectionType = VertexYBoth;
        intersectionX = edge.minX();
    } else if (y == edge.minY()) {
        intersectionType = VertexMinY;
        intersectionX = (vertex1.y() < vertex2.y()) ? vertex1.x() : vertex2.x();
    } else if (y == edge.maxY()) {
        intersectionType = VertexMaxY;
        intersectionX = (vertex1.y() > vertex2.y()) ? vertex1.x() : vertex2.x();
    } else {
        intersectionType = Normal;
        intersectionX = ((y - vertex1.y()) * (vertex2.x() - vertex1.x()) / dy) + vertex1.x();
    }

    result.edge = edgePointer;
    result.type = intersectionType;
    result.point.set(intersectionX, y);

    return true;
}

static inline FloatSize inwardEdgeNormal(const FloatPolygonEdge& edge)
{
    FloatSize edgeDelta = edge.vertex2() - edge.vertex1();
    if (!edgeDelta.width())
        return FloatSize((edgeDelta.height() > 0 ? -1 : 1), 0);
    if (!edgeDelta.height())
        return FloatSize(0, (edgeDelta.width() > 0 ? 1 : -1));
    float edgeLength = edgeDelta.diagonalLength();
    return FloatSize(-edgeDelta.height() / edgeLength, edgeDelta.width() / edgeLength);
}

static inline FloatSize outwardEdgeNormal(const FloatPolygonEdge& edge)
{
    return -inwardEdgeNormal(edge);
}

static inline void appendArc(Vector<FloatPoint>& vertices, const FloatPoint& arcCenter, float arcRadius, const FloatPoint& startArcVertex, const FloatPoint& endArcVertex, bool padding)
{
    float startAngle = atan2(startArcVertex.y() - arcCenter.y(), startArcVertex.x() - arcCenter.x());
    float endAngle = atan2(endArcVertex.y() - arcCenter.y(), endArcVertex.x() - arcCenter.x());
    const float twoPI = piFloat * 2;
    if (startAngle < 0)
        startAngle += twoPI;
    if (endAngle < 0)
        endAngle += twoPI;
    float angle = (startAngle > endAngle) ? (startAngle - endAngle) : (startAngle + twoPI - endAngle);
    const float arcSegmentCount = 6; // An even number so that one arc vertex will be eactly arcRadius from arcCenter.
    float arcSegmentAngle =  ((padding) ? -angle : twoPI - angle) / arcSegmentCount;

    vertices.append(startArcVertex);
    for (unsigned i = 1; i < arcSegmentCount; ++i) {
        float angle = startAngle + arcSegmentAngle * i;
        vertices.append(arcCenter + FloatPoint(cos(angle) * arcRadius, sin(angle) * arcRadius));
    }
    vertices.append(endArcVertex);
}

static inline void snapVerticesToLayoutUnitGrid(Vector<FloatPoint>& vertices)
{
    for (unsigned i = 0; i < vertices.size(); ++i)
        vertices[i].set(LayoutUnit(vertices[i].x()).toFloat(), LayoutUnit(vertices[i].y()).toFloat());
}

static inline PassOwnPtr<FloatPolygon> computeShapePaddingBounds(const FloatPolygon& polygon, float padding, WindRule fillRule)
{
    OwnPtr<Vector<FloatPoint> > paddedVertices = adoptPtr(new Vector<FloatPoint>());
    FloatPoint intersection;

    for (unsigned i = 0; i < polygon.numberOfEdges(); ++i) {
        const FloatPolygonEdge& thisEdge = polygon.edgeAt(i);
        const FloatPolygonEdge& prevEdge = thisEdge.previousEdge();
        OffsetPolygonEdge thisOffsetEdge(thisEdge, inwardEdgeNormal(thisEdge) * padding);
        OffsetPolygonEdge prevOffsetEdge(prevEdge, inwardEdgeNormal(prevEdge) * padding);

        if (prevOffsetEdge.intersection(thisOffsetEdge, intersection))
            paddedVertices->append(intersection);
        else if (isReflexVertex(prevEdge.vertex1(), thisEdge.vertex1(), thisEdge.vertex2()))
            appendArc(*paddedVertices, thisEdge.vertex1(), padding, prevOffsetEdge.vertex2(), thisOffsetEdge.vertex1(), true);
    }

    snapVerticesToLayoutUnitGrid(*paddedVertices);
    return adoptPtr(new FloatPolygon(paddedVertices.release(), fillRule));
}

static inline PassOwnPtr<FloatPolygon> computeShapeMarginBounds(const FloatPolygon& polygon, float margin, WindRule fillRule)
{
    OwnPtr<Vector<FloatPoint> > marginVertices = adoptPtr(new Vector<FloatPoint>());
    FloatPoint intersection;

    for (unsigned i = 0; i < polygon.numberOfEdges(); ++i) {
        const FloatPolygonEdge& thisEdge = polygon.edgeAt(i);
        const FloatPolygonEdge& prevEdge = thisEdge.previousEdge();
        OffsetPolygonEdge thisOffsetEdge(thisEdge, outwardEdgeNormal(thisEdge) * margin);
        OffsetPolygonEdge prevOffsetEdge(prevEdge, outwardEdgeNormal(prevEdge) * margin);

        if (prevOffsetEdge.intersection(thisOffsetEdge, intersection))
            marginVertices->append(intersection);
        else
            appendArc(*marginVertices, thisEdge.vertex1(), margin, prevOffsetEdge.vertex2(), thisOffsetEdge.vertex1(), false);
    }

    snapVerticesToLayoutUnitGrid(*marginVertices);
    return adoptPtr(new FloatPolygon(marginVertices.release(), fillRule));
}

const FloatPolygon& PolygonShape::shapePaddingBounds() const
{
    ASSERT(shapePadding() >= 0);
    if (!shapePadding())
        return m_polygon;

    if (!m_paddingBounds)
        m_paddingBounds = computeShapePaddingBounds(m_polygon, shapePadding(), m_polygon.fillRule());

    return *m_paddingBounds;
}

const FloatPolygon& PolygonShape::shapeMarginBounds() const
{
    ASSERT(shapeMargin() >= 0);
    if (!shapeMargin())
        return m_polygon;

    if (!m_marginBounds)
        m_marginBounds = computeShapeMarginBounds(m_polygon, shapeMargin(), m_polygon.fillRule());

    return *m_marginBounds;
}

static inline bool getVertexIntersectionVertices(const EdgeIntersection& intersection, FloatPoint& prevVertex, FloatPoint& thisVertex, FloatPoint& nextVertex)
{
    if (intersection.type != VertexMinY && intersection.type != VertexMaxY)
        return false;

    ASSERT(intersection.edge && intersection.edge->polygon());
    const FloatPolygon& polygon = *(intersection.edge->polygon());
    const FloatPolygonEdge& thisEdge = *(intersection.edge);

    if ((intersection.type == VertexMinY && (thisEdge.vertex1().y() < thisEdge.vertex2().y()))
        || (intersection.type == VertexMaxY && (thisEdge.vertex1().y() > thisEdge.vertex2().y()))) {
        prevVertex = polygon.vertexAt(thisEdge.previousEdge().vertexIndex1());
        thisVertex = polygon.vertexAt(thisEdge.vertexIndex1());
        nextVertex = polygon.vertexAt(thisEdge.vertexIndex2());
    } else {
        prevVertex = polygon.vertexAt(thisEdge.vertexIndex1());
        thisVertex = polygon.vertexAt(thisEdge.vertexIndex2());
        nextVertex = polygon.vertexAt(thisEdge.nextEdge().vertexIndex2());
    }

    return true;
}

static inline bool appendIntervalX(float x, bool inside, Vector<ShapeInterval>& result)
{
    if (!inside)
        result.append(ShapeInterval(x));
    else
        result[result.size() - 1].x2 = x;

    return !inside;
}

static bool compareEdgeIntersectionX(const EdgeIntersection& intersection1, const EdgeIntersection& intersection2)
{
    float x1 = intersection1.point.x();
    float x2 = intersection2.point.x();
    return (x1 == x2) ? intersection1.type < intersection2.type : x1 < x2;
}

static void computeXIntersections(const FloatPolygon& polygon, float y, bool isMinY, Vector<ShapeInterval>& result)
{
    Vector<const FloatPolygonEdge*> edges;
    if (!polygon.overlappingEdges(y, y, edges))
        return;

    Vector<EdgeIntersection> intersections;
    EdgeIntersection intersection;
    for (unsigned i = 0; i < edges.size(); ++i) {
        if (computeXIntersection(edges[i], y, intersection) && intersection.type != VertexYBoth)
            intersections.append(intersection);
    }

    if (intersections.size() < 2)
        return;

    std::sort(intersections.begin(), intersections.end(), WebCore::compareEdgeIntersectionX);

    unsigned index = 0;
    int windCount = 0;
    bool inside = false;

    while (index < intersections.size()) {
        const EdgeIntersection& thisIntersection = intersections[index];
        if (index + 1 < intersections.size()) {
            const EdgeIntersection& nextIntersection = intersections[index + 1];
            if ((thisIntersection.point.x() == nextIntersection.point.x()) && (thisIntersection.type == VertexMinY || thisIntersection.type == VertexMaxY)) {
                if (thisIntersection.type == nextIntersection.type) {
                    // Skip pairs of intersections whose types are VertexMaxY,VertexMaxY and VertexMinY,VertexMinY.
                    index += 2;
                } else {
                    // Replace pairs of intersections whose types are VertexMinY,VertexMaxY or VertexMaxY,VertexMinY with one intersection.
                    ++index;
                }
                continue;
            }
        }

        const FloatPolygonEdge& thisEdge = *thisIntersection.edge;
        bool evenOddCrossing = !windCount;

        if (polygon.fillRule() == RULE_EVENODD) {
            windCount += (thisEdge.vertex2().y() > thisEdge.vertex1().y()) ? 1 : -1;
            evenOddCrossing = evenOddCrossing || !windCount;
        }

        if (evenOddCrossing) {
            bool edgeCrossing = thisIntersection.type == Normal;
            if (!edgeCrossing) {
                FloatPoint prevVertex;
                FloatPoint thisVertex;
                FloatPoint nextVertex;

                if (getVertexIntersectionVertices(thisIntersection, prevVertex, thisVertex, nextVertex)) {
                    if (nextVertex.y() == y)
                        edgeCrossing = (isMinY) ? prevVertex.y() > y : prevVertex.y() < y;
                    else if (prevVertex.y() == y)
                        edgeCrossing = (isMinY) ? nextVertex.y() > y : nextVertex.y() < y;
                    else
                        edgeCrossing = true;
                }
            }
            if (edgeCrossing)
                inside = appendIntervalX(thisIntersection.point.x(), inside, result);
        }

        ++index;
    }
}

static void computeOverlappingEdgeXProjections(const FloatPolygon& polygon, float y1, float y2, Vector<ShapeInterval>& result)
{
    Vector<const FloatPolygonEdge*> edges;
    if (!polygon.overlappingEdges(y1, y2, edges))
        return;

    EdgeIntersection intersection;
    for (unsigned i = 0; i < edges.size(); ++i) {
        const FloatPolygonEdge *edge = edges[i];
        float x1;
        float x2;

        if (edge->minY() < y1) {
            computeXIntersection(edge, y1, intersection);
            x1 = intersection.point.x();
        } else
            x1 = (edge->vertex1().y() < edge->vertex2().y()) ? edge->vertex1().x() : edge->vertex2().x();

        if (edge->maxY() > y2) {
            computeXIntersection(edge, y2, intersection);
            x2 = intersection.point.x();
        } else
            x2 = (edge->vertex1().y() > edge->vertex2().y()) ? edge->vertex1().x() : edge->vertex2().x();

        if (x1 > x2)
            std::swap(x1, x2);

        if (x2 > x1)
            result.append(ShapeInterval(x1, x2));
    }

    sortShapeIntervals(result);
}

void PolygonShape::getExcludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList& result) const
{
    const FloatPolygon& polygon = shapeMarginBounds();
    if (polygon.isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = logicalTop + logicalHeight;

    Vector<ShapeInterval> y1XIntervals, y2XIntervals;
    computeXIntersections(polygon, y1, true, y1XIntervals);
    computeXIntersections(polygon, y2, false, y2XIntervals);

    Vector<ShapeInterval> mergedIntervals;
    mergeShapeIntervals(y1XIntervals, y2XIntervals, mergedIntervals);

    Vector<ShapeInterval> edgeIntervals;
    computeOverlappingEdgeXProjections(polygon, y1, y2, edgeIntervals);

    Vector<ShapeInterval> excludedIntervals;
    mergeShapeIntervals(mergedIntervals, edgeIntervals, excludedIntervals);

    for (unsigned i = 0; i < excludedIntervals.size(); ++i) {
        ShapeInterval interval = excludedIntervals[i];
        result.append(LineSegment(interval.x1, interval.x2));
    }
}

void PolygonShape::getIncludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList& result) const
{
    const FloatPolygon& polygon = shapePaddingBounds();
    if (polygon.isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = logicalTop + logicalHeight;

    Vector<ShapeInterval> y1XIntervals, y2XIntervals;
    computeXIntersections(polygon, y1, true, y1XIntervals);
    computeXIntersections(polygon, y2, false, y2XIntervals);

    Vector<ShapeInterval> commonIntervals;
    intersectShapeIntervals(y1XIntervals, y2XIntervals, commonIntervals);

    Vector<ShapeInterval> edgeIntervals;
    computeOverlappingEdgeXProjections(polygon, y1, y2, edgeIntervals);

    Vector<ShapeInterval> includedIntervals;
    subtractShapeIntervals(commonIntervals, edgeIntervals, includedIntervals);

    for (unsigned i = 0; i < includedIntervals.size(); ++i) {
        ShapeInterval interval = includedIntervals[i];
        result.append(LineSegment(interval.x1, interval.x2));
    }
}

static inline bool firstFitRectInPolygon(const FloatPolygon& polygon, const FloatRect& rect, unsigned offsetEdgeIndex1, unsigned offsetEdgeIndex2)
{
    Vector<const FloatPolygonEdge*> edges;
    if (!polygon.overlappingEdges(rect.y(), rect.maxY(), edges))
        return true;

    for (unsigned i = 0; i < edges.size(); ++i) {
        const FloatPolygonEdge* edge = edges[i];
        if (edge->edgeIndex() != offsetEdgeIndex1 && edge->edgeIndex() != offsetEdgeIndex2 && edge->overlapsRect(rect))
            return false;
    }

    return true;
}

static inline bool aboveOrToTheLeft(const FloatRect& r1, const FloatRect& r2)
{
    if (r1.y() < r2.y())
        return true;
    if (r1.y() == r2.y())
        return r1.x() < r2.x();
    return false;
}

bool PolygonShape::firstIncludedIntervalLogicalTop(LayoutUnit minLogicalIntervalTop, const LayoutSize& minLogicalIntervalSize, LayoutUnit& result) const
{
    float minIntervalTop = minLogicalIntervalTop;
    float minIntervalHeight = minLogicalIntervalSize.height();
    float minIntervalWidth = minLogicalIntervalSize.width();

    const FloatPolygon& polygon = shapePaddingBounds();
    const FloatRect boundingBox = polygon.boundingBox();
    if (minIntervalWidth > boundingBox.width())
        return false;

    float minY = std::max(boundingBox.y(), minIntervalTop);
    float maxY = minY + minIntervalHeight;

    if (maxY > boundingBox.maxY())
        return false;

    Vector<const FloatPolygonEdge*> edges;
    polygon.overlappingEdges(minIntervalTop, boundingBox.maxY(), edges);

    float dx = minIntervalWidth / 2;
    float dy = minIntervalHeight / 2;
    Vector<OffsetPolygonEdge> offsetEdges;

    for (unsigned i = 0; i < edges.size(); ++i) {
        const FloatPolygonEdge& edge = *(edges[i]);
        const FloatPoint& vertex0 = edge.previousEdge().vertex1();
        const FloatPoint& vertex1 = edge.vertex1();
        const FloatPoint& vertex2 = edge.vertex2();
        Vector<OffsetPolygonEdge> offsetEdgeBuffer;

        if (vertex2.y() > vertex1.y() ? vertex2.x() >= vertex1.x() : vertex1.x() >= vertex2.x()) {
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(dx, -dy)));
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(-dx, dy)));
        } else {
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(dx, dy)));
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(-dx, -dy)));
        }

        if (isReflexVertex(vertex0, vertex1, vertex2)) {
            if (vertex2.x() <= vertex1.x() && vertex0.x() <= vertex1.x())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(dx, -dy), FloatSize(dx, dy)));
            else if (vertex2.x() >= vertex1.x() && vertex0.x() >= vertex1.x())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(-dx, -dy), FloatSize(-dx, dy)));
            if (vertex2.y() <= vertex1.y() && vertex0.y() <= vertex1.y())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(-dx, dy), FloatSize(dx, dy)));
            else if (vertex2.y() >= vertex1.y() && vertex0.y() >= vertex1.y())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(-dx, -dy), FloatSize(dx, -dy)));
        }

        for (unsigned j = 0; j < offsetEdgeBuffer.size(); ++j)
            if (offsetEdgeBuffer[j].maxY() >= minY)
                offsetEdges.append(offsetEdgeBuffer[j]);
    }

    offsetEdges.append(OffsetPolygonEdge(polygon, minIntervalTop, FloatSize(0, dy)));

    FloatPoint offsetEdgesIntersection;
    FloatRect firstFitRect;
    bool firstFitFound = false;

    for (unsigned i = 0; i < offsetEdges.size() - 1; ++i) {
        for (unsigned j = i + 1; j < offsetEdges.size(); ++j) {
            if (offsetEdges[i].intersection(offsetEdges[j], offsetEdgesIntersection)) {
                FloatPoint potentialFirstFitLocation(offsetEdgesIntersection.x() - dx, offsetEdgesIntersection.y() - dy);
                FloatRect potentialFirstFitRect(potentialFirstFitLocation, minLogicalIntervalSize);
                if ((offsetEdges[i].basis() == OffsetPolygonEdge::LineTop
                    || offsetEdges[j].basis() == OffsetPolygonEdge::LineTop
                    || potentialFirstFitLocation.y() >= minIntervalTop)
                    && (!firstFitFound || aboveOrToTheLeft(potentialFirstFitRect, firstFitRect))
                    && polygon.contains(offsetEdgesIntersection)
                    && firstFitRectInPolygon(polygon, potentialFirstFitRect, offsetEdges[i].edgeIndex(), offsetEdges[j].edgeIndex())) {
                    firstFitFound = true;
                    firstFitRect = potentialFirstFitRect;
                }
            }
        }
    }

    if (firstFitFound)
        result = ceiledLayoutUnit(firstFitRect.y());
    return firstFitFound;
}

} // namespace WebCore

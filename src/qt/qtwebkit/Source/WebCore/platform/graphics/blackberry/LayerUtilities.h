/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef LayerUtilities_h
#define LayerUtilities_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "FloatQuad.h"
#include "FloatSize.h"
#include "LayerCompositingThread.h"
#include "TransformationMatrix.h"

#include <algorithm>
#include <wtf/Vector.h>

namespace WebCore {

// determinant of column vectors
inline float determinant(const FloatSize& a, const FloatSize& b)
{
    return a.width() * b.height() - a.height() * b.width();
}

// dot product
inline float dot(const FloatSize& a, const FloatSize& b)
{
    return a.width() * b.width() + a.height() * b.height();
}

// Represents a line, not a finite line segment
class LayerClipEdge {
public:
    LayerClipEdge(const FloatPoint& first, const FloatPoint& second)
        : m_first(first)
        , m_second(second)
    {
    }

    inline bool isPointInside(const FloatPoint& p) const
    {
        // For numeric robustness, we prefer to consider a point to be inside rather than
        // clip it again.
        const float epsilon = 1e-6;
        return determinant(m_second - m_first, p - m_first) > -epsilon;
    }

    inline FloatPoint computeIntersection(const FloatPoint& p1, const FloatPoint& p2) const
    {
        const FloatPoint& p3 = m_first;
        const FloatPoint& p4 = m_second;
        float denominator = determinant(p1 - p2, p3 - p4);
        FloatSize determinants(determinant(toFloatSize(p1), toFloatSize(p2)), determinant(toFloatSize(p3), toFloatSize(p4)));
        FloatPoint result(
            determinant(determinants, FloatSize(p1.x() - p2.x(), p3.x() - p4.x())) / denominator,
            determinant(determinants, FloatSize(p1.y() - p2.y(), p3.y() - p4.y())) / denominator);
        return result;
    }

private:
    FloatPoint m_first;
    FloatPoint m_second;
};

// Specifies a clip plane with normal n and containing point p_0
// as p * n + d = 0, d = -p_0 * n. The asterisk is dot product.
class LayerClipPlane {
public:
    LayerClipPlane(FloatPoint3D n, float d)
        : m_n(n)
        , m_d(d)
    {
    }

    inline bool isPointInside(const FloatPoint3D& p) const
    {
        return p * m_n + m_d > 0;
    }

    inline FloatPoint3D computeIntersection(const FloatPoint3D& p1, const FloatPoint3D& p2) const
    {
        float u = (-m_d - p1 * m_n) / ((p2 - p1) * m_n);
        return p1 + u * (p2 - p1);
    }

protected:
    FloatPoint3D m_n;
    float m_d;
};

// Sutherland - Hodgman, inner loop
template<typename Point, size_t inlineCapacity, typename ClipPrimitive>
inline Vector<Point, inlineCapacity> intersect(const Vector<Point, inlineCapacity>& inputList, const ClipPrimitive& clipPrimitive)
{
    Vector<Point, inlineCapacity> outputList;
    Point s;
    if (!inputList.isEmpty())
        s = inputList.last();
    for (typename Vector<Point, inlineCapacity>::const_iterator eIterator = inputList.begin(); eIterator != inputList.end(); ++eIterator) {
        const Point& e = *eIterator;
        if (clipPrimitive.isPointInside(e)) {
            if (!clipPrimitive.isPointInside(s))
                outputList.append(clipPrimitive.computeIntersection(s, e));
            outputList.append(e);
        } else if (clipPrimitive.isPointInside(s))
            outputList.append(clipPrimitive.computeIntersection(s, e));
        s = e;
    }
    return outputList;
}

// Sutherland - Hodgman, main driver
template<size_t inlineCapacity>
inline Vector<FloatPoint, inlineCapacity> intersectPolygonWithRect(const Vector<FloatPoint, inlineCapacity>& subjectPolygon, const FloatRect& clipRect)
{
    FloatQuad clipQuad(clipRect);
    Vector<LayerClipEdge> edges;
    edges.append(LayerClipEdge(clipQuad.p1(), clipQuad.p2()));
    edges.append(LayerClipEdge(clipQuad.p2(), clipQuad.p3()));
    edges.append(LayerClipEdge(clipQuad.p3(), clipQuad.p4()));
    edges.append(LayerClipEdge(clipQuad.p4(), clipQuad.p1()));

    Vector<FloatPoint> outputList = subjectPolygon;
    for (Vector<LayerClipEdge>::const_iterator clipEdgeIterator = edges.begin(); clipEdgeIterator != edges.end(); ++clipEdgeIterator) {
        const LayerClipEdge& clipEdge = *clipEdgeIterator;
        Vector<FloatPoint> inputList = outputList;
        outputList = intersect(inputList, clipEdge);
    }
    return outputList;
}

template<size_t inlineCapacity>
inline FloatRect boundingBox(const Vector<FloatPoint, inlineCapacity>& points)
{
    if (points.isEmpty())
        return FloatRect();
    float xmin, xmax, ymin, ymax;
    xmin = ymin = std::numeric_limits<float>::infinity();
    xmax = ymax = -std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < points.size(); ++i) {
        const FloatPoint& p = points[i];
        if (p.x() < xmin)
            xmin = p.x();
        if (p.x() > xmax)
            xmax = p.x();
        if (p.y() < ymin)
            ymin = p.y();
        if (p.y() > ymax)
            ymax = p.y();
    }
    return FloatRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

inline FloatPoint3D computeBarycentricCoordinates(const FloatPoint& p, const FloatPoint& t1, const FloatPoint& t2, const FloatPoint& t3, bool& ok)
{
    // Compute vectors
    FloatSize v0 = t2 - t1;
    FloatSize v1 = t3 - t1;
    FloatSize v2 = p - t1;

    // Compute dot products
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    float denominator = (dot00 * dot11 - dot01 * dot01);
    ok = (denominator != 0.0);
    if (!ok)
        return FloatPoint3D();

    float v = (dot11 * dot02 - dot01 * dot12) / denominator;
    float w = (dot00 * dot12 - dot01 * dot02) / denominator;

    return FloatPoint3D(1.0f - v - w, v, w);
}

inline float manhattanDistanceToViewport(const FloatPoint& p)
{
    float d = 0;
    if (fabsf(p.x()) > 1)
        d += fabsf(p.x()) - 1;
    if (fabsf(p.y()) > 1)
        d += fabsf(p.y()) - 1;
    return d;
}

struct UnprojectionVertex {
    FloatPoint xy;
    float w;
    FloatSize uv;
};

inline bool compareManhattanDistanceToViewport(const UnprojectionVertex& a, const UnprojectionVertex& b)
{
    return manhattanDistanceToViewport(a.xy) < manhattanDistanceToViewport(b.xy);
}

template<size_t inlineCapacity>
inline Vector<FloatPoint, inlineCapacity> unproject(LayerCompositingThread* layer, const Vector<FloatPoint, inlineCapacity>& points)
{
    // Use perspective correct texturing logic to find the locations of these points in normalized layer coordinates
    Vector<FloatPoint, 4> bounds = layer->transformedBounds();
    Vector<float, 4> ws = layer->ws();
    if (ws.isEmpty())
        ws.fill(0.0f, bounds.size());
    const Vector<FloatPoint>& texCoords = layer->textureCoordinates();
    if (bounds.size() < 3)
        return Vector<FloatPoint, inlineCapacity>();

    Vector<UnprojectionVertex, 4> vertices(bounds.size());
    for (size_t i = 0; i < bounds.size(); ++i) {
        vertices[i].xy = bounds[i];
        vertices[i].w = ws[i];
        vertices[i].uv = toFloatSize(texCoords[i]);
    }
    // Each point needs to be qualified as lying in one of the triangles formed by the "bounds" triangle strip
    // Try to use only points that are onscreen, for numerical stability
    std::sort(vertices.begin(), vertices.end(), compareManhattanDistanceToViewport);

    Vector<FloatPoint, inlineCapacity> result;

    size_t i0 = 0;

    const FloatPoint& p0 = vertices[i0].xy;
    float w0 = vertices[i0].w;
    FloatSize uv0 = vertices[i0].uv;

    for (size_t j = 0; j < points.size(); ++j) {
        const FloatPoint& p = points[j];
        FloatPoint texCoord;
        for (size_t di = 1; di + 1 < vertices.size(); ++di) {
            size_t i = (i0 + di) % vertices.size();
            const FloatPoint& p1 = vertices[i].xy;
            const FloatPoint& p2 = vertices[i + 1].xy;
            float w1 = vertices[i].w;
            float w2 = vertices[i + 1].w;
            FloatSize uv1 = vertices[i].uv;
            FloatSize uv2 = vertices[i + 1].uv;

            bool ok;
            FloatPoint3D b = computeBarycentricCoordinates(p, p0, p1, p2, ok);
            if (!ok)
                continue;

            if (w0 && w1 && w2) {
                // Perspective correct interpolation
                FloatPoint3D bw(b.x() / w0, b.y() / w1, b.z() / w2);
                float denominator = bw.x() + bw.y() + bw.z();
                texCoord = FloatPoint::zero() + (bw.x() * uv0 + bw.y() * uv1 + bw.z() * uv2);
                texCoord.setX(texCoord.x() / denominator);
                texCoord.setY(texCoord.y() / denominator);
            } else {
                // Linear interpolation
                texCoord = FloatPoint::zero() + (b.x() * uv0 + b.y() * uv1 + b.z() * uv2);
            }
            break;
        }
        result.append(texCoord);
    }
    return result;
}

inline FloatPoint3D multVecMatrix(const TransformationMatrix& matrix, const FloatPoint3D& p, float& w)
{
    FloatPoint3D result(
        matrix.m41() + p.x() * matrix.m11() + p.y() * matrix.m21() + p.z() * matrix.m31(),
        matrix.m42() + p.x() * matrix.m12() + p.y() * matrix.m22() + p.z() * matrix.m32(),
        matrix.m43() + p.x() * matrix.m13() + p.y() * matrix.m23() + p.z() * matrix.m33());
    w = matrix.m44() + p.x() * matrix.m14() + p.y() * matrix.m24() + p.z() * matrix.m34();
    return result;
}

template<typename Point, size_t inlineCapacity>
inline Vector<Point, inlineCapacity> toVector(const FloatQuad& quad)
{
    Vector<Point, inlineCapacity> result;
    result.append(quad.p1());
    result.append(quad.p2());
    result.append(quad.p3());
    result.append(quad.p4());
    return result;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerUtilities_h

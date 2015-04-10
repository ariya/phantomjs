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

#include "config.h"

#if USE(ACCELERATED_COMPOSITING) || ENABLE(ACCELERATED_2D_CANVAS)

#include "LoopBlinnMathUtils.h"

#include "FloatPoint.h"
#include <algorithm>
#include <wtf/MathExtras.h>

namespace WebCore {
namespace LoopBlinnMathUtils {

namespace {

// Utility functions local to this file.
int orientation(const FloatPoint& p1,
                const FloatPoint& p2,
                const FloatPoint& p3)
{
    float crossProduct = (p2.y() - p1.y()) * (p3.x() - p2.x()) - (p3.y() - p2.y()) * (p2.x() - p1.x());
    return (crossProduct < 0.0f) ? -1 : ((crossProduct > 0.0f) ? 1 : 0);
}

bool edgeEdgeTest(const FloatSize& v0Delta,
                  const FloatPoint& v0,
                  const FloatPoint& u0,
                  const FloatPoint& u1)
{
    // This edge to edge test is based on Franlin Antonio's gem: "Faster
    // Line Segment Intersection", in Graphics Gems III, pp. 199-202.
    float ax = v0Delta.width();
    float ay = v0Delta.height();
    float bx = u0.x() - u1.x();
    float by = u0.y() - u1.y();
    float cx = v0.x() - u0.x();
    float cy = v0.y() - u0.y();
    float f = ay * bx - ax * by;
    float d = by * cx - bx * cy;
    if ((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f)) {
        float e = ax * cy - ay * cx;

        // This additional test avoids reporting coincident edges, which
        // is the behavior we want.
        if (approxEqual(e, 0) || approxEqual(f, 0) || approxEqual(e, f))
            return false;

        if (f > 0)
            return e >= 0 && e <= f;

        return e <= 0 && e >= f;
    }
    return false;
}

bool edgeAgainstTriangleEdges(const FloatPoint& v0,
                              const FloatPoint& v1,
                              const FloatPoint& u0,
                              const FloatPoint& u1,
                              const FloatPoint& u2)
{
    FloatSize delta = v1 - v0;
    // Test edge u0, u1 against v0, v1.
    if (edgeEdgeTest(delta, v0, u0, u1))
        return true;
    // Test edge u1, u2 against v0, v1.
    if (edgeEdgeTest(delta, v0, u1, u2))
        return true;
    // Test edge u2, u1 against v0, v1.
    if (edgeEdgeTest(delta, v0, u2, u0))
        return true;
    return false;
}

// A roundoff factor in the cubic classification and texture coordinate
// generation algorithms. It primarily determines the handling of corner
// cases during the classification process. Be careful when adjusting it;
// it has been determined empirically to work well. When changing it, you
// should look in particular at shapes that contain quadratic curves and
// ensure they still look smooth. Once pixel tests are running against this
// algorithm, they should provide sufficient coverage to ensure that
// adjusting the constant won't break anything.
const float Epsilon = 5.0e-4f;

} // anonymous namespace

// Exported routines

float roundToZero(float val)
{
    if (val < Epsilon && val > -Epsilon)
        return 0;
    return val;
}

bool approxEqual(const FloatPoint& v0, const FloatPoint& v1)
{
    return (v0 - v1).diagonalLengthSquared() < Epsilon * Epsilon;
}

bool approxEqual(const FloatPoint3D& v0, const FloatPoint3D& v1)
{
    return (v0 - v1).lengthSquared() < Epsilon * Epsilon;
}

bool approxEqual(float f0, float f1)
{
    return fabsf(f0 - f1) < Epsilon;
}

bool linesIntersect(const FloatPoint& p1,
                    const FloatPoint& q1,
                    const FloatPoint& p2,
                    const FloatPoint& q2)
{
    return (orientation(p1, q1, p2) != orientation(p1, q1, q2)
            && orientation(p2, q2, p1) != orientation(p2, q2, q1));
}

bool pointInTriangle(const FloatPoint& point,
                     const FloatPoint& a,
                     const FloatPoint& b,
                     const FloatPoint& c)
{
    // Algorithm from http://www.blackpawn.com/texts/pointinpoly/default.html
    float x0 = c.x() - a.x();
    float y0 = c.y() - a.y();
    float x1 = b.x() - a.x();
    float y1 = b.y() - a.y();
    float x2 = point.x() - a.x();
    float y2 = point.y() - a.y();

    float dot00 = x0 * x0 + y0 * y0;
    float dot01 = x0 * x1 + y0 * y1;
    float dot02 = x0 * x2 + y0 * y2;
    float dot11 = x1 * x1 + y1 * y1;
    float dot12 = x1 * x2 + y1 * y2;
    float denominator = dot00 * dot11 - dot01 * dot01;
    if (!denominator)
        // Triangle is zero-area. Treat query point as not being inside.
        return false;
    // Compute
    float inverseDenominator = 1.0f / denominator;
    float u = (dot11 * dot02 - dot01 * dot12) * inverseDenominator;
    float v = (dot00 * dot12 - dot01 * dot02) * inverseDenominator;

    return (u > 0.0f) && (v > 0.0f) && (u + v < 1.0f);
}

bool trianglesOverlap(const FloatPoint& a1,
                      const FloatPoint& b1,
                      const FloatPoint& c1,
                      const FloatPoint& a2,
                      const FloatPoint& b2,
                      const FloatPoint& c2)
{
    // Derived from coplanar_tri_tri() at
    // http://jgt.akpeters.com/papers/ShenHengTang03/tri_tri.html ,
    // simplified for the 2D case and modified so that overlapping edges
    // do not report overlapping triangles.

    // Test all edges of triangle 1 against the edges of triangle 2.
    if (edgeAgainstTriangleEdges(a1, b1, a2, b2, c2)
        || edgeAgainstTriangleEdges(b1, c1, a2, b2, c2)
        || edgeAgainstTriangleEdges(c1, a1, a2, b2, c2))
        return true;
    // Finally, test if tri1 is totally contained in tri2 or vice versa.
    // The paper above only performs the first two point-in-triangle tests.
    // Because we define that triangles sharing a vertex or edge don't
    // overlap, we must perform additional tests to see whether one
    // triangle is contained in the other.
    if (pointInTriangle(a1, a2, b2, c2)
        || pointInTriangle(a2, a1, b1, c1)
        || pointInTriangle(b1, a2, b2, c2)
        || pointInTriangle(b2, a1, b1, c1)
        || pointInTriangle(c1, a2, b2, c2)
        || pointInTriangle(c2, a1, b1, c1))
        return true;
    return false;
}

namespace {

// Helper routines for public XRay queries below. All of this code
// originated in Skia; see include/core/ and src/core/, SkScalar.h and
// SkGeometry.{cpp,h}.

const float NearlyZeroConstant = (1.0f / (1 << 12));

bool nearlyZero(float x, float tolerance = NearlyZeroConstant)
{
    ASSERT(tolerance > 0.0f);
    return ::fabsf(x) < tolerance;
}

// Linearly interpolate between a and b, based on t.
// If t is 0, return a; if t is 1, return b; else interpolate.
// t must be [0..1].
float interpolate(float a, float b, float t)
{
    ASSERT(t >= 0 && t <= 1);
    return a + (b - a) * t;
}

float evaluateCubic(float controlPoint0, float controlPoint1, float controlPoint2, float controlPoint3, float t)
{
    ASSERT(t >= 0 && t <= 1);

    if (!t)
        return controlPoint0;

    float ab = interpolate(controlPoint0, controlPoint1, t);
    float bc = interpolate(controlPoint1, controlPoint2, t);
    float cd = interpolate(controlPoint2, controlPoint3, t);
    float abc = interpolate(ab, bc, t);
    float bcd = interpolate(bc, cd, t);
    return interpolate(abc, bcd, t);
}

// Evaluates the point on the source cubic specified by t, 0 <= t <= 1.0.
FloatPoint evaluateCubicAt(const FloatPoint cubic[4], float t)
{
    return FloatPoint(evaluateCubic(cubic[0].x(), cubic[1].x(), cubic[2].x(), cubic[3].x(), t),
                      evaluateCubic(cubic[0].y(), cubic[1].y(), cubic[2].y(), cubic[3].y(), t));
}

bool xRayCrossesMonotonicCubic(const XRay& xRay, const FloatPoint cubic[4], bool& ambiguous)
{
    ambiguous = false;

    // Find the minimum and maximum y of the extrema, which are the
    // first and last points since this cubic is monotonic
    float minY = std::min(cubic[0].y(), cubic[3].y());
    float maxY = std::max(cubic[0].y(), cubic[3].y());

    if (xRay.y() == cubic[0].y()
        || xRay.y() < minY
        || xRay.y() > maxY) {
        // The query line definitely does not cross the curve
        ambiguous = (xRay.y() == cubic[0].y());
        return false;
    }

    const bool pointAtExtremum = (xRay.y() == cubic[3].y());

    float minX = std::min(std::min(std::min(cubic[0].x(), cubic[1].x()),
                                   cubic[2].x()),
                          cubic[3].x());
    if (xRay.x() < minX) {
        // The query line definitely crosses the curve
        ambiguous = pointAtExtremum;
        return true;
    }

    float maxX = std::max(std::max(std::max(cubic[0].x(), cubic[1].x()),
                                   cubic[2].x()),
                          cubic[3].x());
    if (xRay.x() > maxX)
        // The query line definitely does not cross the curve
        return false;

    // Do a binary search to find the parameter value which makes y as
    // close as possible to the query point. See whether the query
    // line's origin is to the left of the associated x coordinate.

    // MaxIterations is chosen as the number of mantissa bits for a float,
    // since there's no way we are going to get more precision by
    // iterating more times than that.
    const int MaxIterations = 23;
    FloatPoint evaluatedPoint;
    int iter = 0;
    float upperT;
    float lowerT;
    // Need to invert direction of t parameter if cubic goes up
    // instead of down
    if (cubic[3].y() > cubic[0].y()) {
        upperT = 1;
        lowerT = 0;
    } else {
        upperT = 0;
        lowerT = 1;
    }
    do {
        float t = 0.5f * (upperT + lowerT);
        evaluatedPoint = evaluateCubicAt(cubic, t);
        if (xRay.y() > evaluatedPoint.y())
            lowerT = t;
        else
            upperT = t;
    } while (++iter < MaxIterations && !nearlyZero(evaluatedPoint.y() - xRay.y()));

    // FIXME: once we have more regression tests for this code,
    // determine whether this should be using a fuzzy test.
    if (xRay.x() <= evaluatedPoint.x()) {
        ambiguous = pointAtExtremum;
        return true;
    }
    return false;
}

// Divides the numerator by the denominator safely for the case where
// the result must lie in the range (0..1). Result indicates whether
// the result is valid.
bool safeUnitDivide(float numerator, float denominator, float& ratio)
{
    if (numerator < 0) {
        // Make the "numerator >= denominator" check below work.
        numerator = -numerator;
        denominator = -denominator;
    }
    if (!numerator || !denominator || numerator >= denominator)
        return false;
    float r = numerator / denominator;
    if (std::isnan(r))
        return false;
    ASSERT(r >= 0 && r < 1);
    if (!r) // catch underflow if numerator <<<< denominator
        return false;
    ratio = r;
    return true;
}

// From Numerical Recipes in C.
//
//   q = -1/2 (b + sign(b) sqrt[b*b - 4*a*c])
//   x1 = q / a
//   x2 = c / q
//
// Returns the number of real roots of the equation [0..2]. Roots are
// returned in sorted order, smaller root first.
int findUnitQuadRoots(float a, float b, float c, float roots[2])
{
    if (!a)
        return safeUnitDivide(-c, b, roots[0]) ? 1 : 0;

    float discriminant = b*b - 4*a*c;
    if (discriminant < 0 || std::isnan(discriminant)) // complex roots
        return 0;
    discriminant = sqrtf(discriminant);

    float q = (b < 0) ? -(b - discriminant) / 2 : -(b + discriminant) / 2;
    int numberOfRoots = 0;
    if (safeUnitDivide(q, a, roots[numberOfRoots]))
        ++numberOfRoots;
    if (safeUnitDivide(c, q, roots[numberOfRoots]))
        ++numberOfRoots;
    if (numberOfRoots == 2) {
        // Seemingly have two roots. Check for equality and sort.
        if (roots[0] == roots[1])
            return 1;
        if (roots[0] > roots[1])
            std::swap(roots[0], roots[1]);
    }
    return numberOfRoots;
}

// Cubic'(t) = pt^2 + qt + r, where
//   p = 3(-a + 3(b - c) + d)
//   q = 6(a - 2b + c)
//   r = 3(b - a)
// Solve for t, keeping only those that fit between 0 < t < 1.
int findCubicExtrema(float a, float b, float c, float d, float tValues[2])
{
    // Divide p, q, and r by 3 to simplify the equations.
    float p = d - a + 3*(b - c);
    float q = 2*(a - b - b + c);
    float r = b - a;

    return findUnitQuadRoots(p, q, r, tValues);
}

void interpolateCubicCoords(float controlPoint0, float controlPoint1, float controlPoint2, float controlPoint3, float* dst, float t)
{
    float ab = interpolate(controlPoint0, controlPoint1, t);
    float bc = interpolate(controlPoint1, controlPoint2, t);
    float cd = interpolate(controlPoint2, controlPoint3, t);
    float abc = interpolate(ab, bc, t);
    float bcd = interpolate(bc, cd, t);
    float abcd = interpolate(abc, bcd, t);

    dst[0] = controlPoint0;
    dst[2] = ab;
    dst[4] = abc;
    dst[6] = abcd;
    dst[8] = bcd;
    dst[10] = cd;
    dst[12] = controlPoint3;
}

#ifndef NDEBUG
bool isUnitInterval(float x)
{
    return x > 0 && x < 1;
}
#endif

void chopCubicAtTValues(const FloatPoint src[4], FloatPoint dst[], const float tValues[], int roots)
{
#ifndef NDEBUG
    for (int i = 0; i < roots - 1; ++i) {
        ASSERT(isUnitInterval(tValues[i]));
        ASSERT(isUnitInterval(tValues[i+1]));
        ASSERT(tValues[i] < tValues[i+1]);
    }
#endif

    if (!roots) {
        // nothing to chop
        for (int j = 0; j < 4; ++j)
            dst[j] = src[j];
        return;
    }

    float t = tValues[0];
    FloatPoint tmp[4];
    for (int j = 0; j < 4; ++j)
        tmp[j] = src[j];

    for (int i = 0; i < roots; ++i) {
        chopCubicAt(tmp, dst, t);
        if (i == roots - 1)
            break;

        dst += 3;
        // Make tmp contain the remaining cubic (after the first chop).
        for (int j = 0; j < 4; ++j)
            tmp[j] = dst[j];

        // Watch out for the case that the renormalized t isn't in range.
        if (!safeUnitDivide(tValues[i+1] - tValues[i], 1.0f - tValues[i], t)) {
            // If it isn't, just create a degenerate cubic.
            dst[4] = dst[5] = dst[6] = tmp[3];
            break;
        }
    }
}

void flattenDoubleCubicYExtrema(FloatPoint coords[7])
{
    coords[2].setY(coords[3].y());
    coords[4].setY(coords[3].y());
}

int chopCubicAtYExtrema(const FloatPoint src[4], FloatPoint dst[10])
{
    float tValues[2];
    int roots = findCubicExtrema(src[0].y(), src[1].y(), src[2].y(), src[3].y(), tValues);

    chopCubicAtTValues(src, dst, tValues, roots);
    if (roots) {
        // we do some cleanup to ensure our Y extrema are flat
        flattenDoubleCubicYExtrema(&dst[0]);
        if (roots == 2)
            flattenDoubleCubicYExtrema(&dst[3]);
    }
    return roots;
}

} // anonymous namespace

// Public cubic operations.

void chopCubicAt(const FloatPoint src[4], FloatPoint dst[7], float t)
{
    ASSERT(t >= 0 && t <= 1);

    float output[14];
    interpolateCubicCoords(src[0].x(), src[1].x(), src[2].x(), src[3].x(), &output[0], t);
    interpolateCubicCoords(src[0].y(), src[1].y(), src[2].y(), src[3].y(), &output[1], t);
    for (int i = 0; i < 7; i++)
        dst[i].set(output[2 * i], output[2 * i + 1]);
}

// Public XRay queries.

bool xRayCrossesLine(const XRay& xRay, const FloatPoint pts[2], bool& ambiguous)
{
    ambiguous = false;

    // Determine quick discards.
    // Consider query line going exactly through point 0 to not
    // intersect, for symmetry with xRayCrossesMonotonicCubic.
    if (xRay.y() == pts[0].y()) {
        ambiguous = true;
        return false;
    }
    if (xRay.y() < pts[0].y() && xRay.y() < pts[1].y())
        return false;
    if (xRay.y() > pts[0].y() && xRay.y() > pts[1].y())
        return false;
    if (xRay.x() > pts[0].x() && xRay.x() > pts[1].x())
        return false;
    // Determine degenerate cases
    if (nearlyZero(pts[0].y() - pts[1].y()))
        return false;
    if (nearlyZero(pts[0].x() - pts[1].x())) {
        // We've already determined the query point lies within the
        // vertical range of the line segment.
        if (xRay.x() <= pts[0].x()) {
            ambiguous = (xRay.y() == pts[1].y());
            return true;
        }
        return false;
    }
    // Ambiguity check
    if (xRay.y() == pts[1].y()) {
        if (xRay.x() <= pts[1].x()) {
            ambiguous = true;
            return true;
        }
        return false;
    }
    // Full line segment evaluation
    float deltaY = pts[1].y() - pts[0].y();
    float deltaX = pts[1].x() - pts[0].x();
    float slope = deltaY / deltaX;
    float b = pts[0].y() - slope * pts[0].x();
    // Solve for x coordinate at y = xRay.y()
    float x = (xRay.y() - b) / slope;
    return xRay.x() <= x;
}

int numXRayCrossingsForCubic(const XRay& xRay, const FloatPoint cubic[4], bool& ambiguous)
{
    int numCrossings = 0;
    FloatPoint monotonicCubics[10];
    int numMonotonicCubics = 1 + chopCubicAtYExtrema(cubic, monotonicCubics);
    ambiguous = false;
    FloatPoint* monotonicCubicsPointer = &monotonicCubics[0];
    for (int i = 0; i < numMonotonicCubics; ++i) {
        if (xRayCrossesMonotonicCubic(xRay, monotonicCubicsPointer, ambiguous))
            ++numCrossings;
        if (ambiguous)
            return 0;
        monotonicCubicsPointer += 3;
    }
    return numCrossings;
}

/*
 * Based on C code from the article
 * "Testing the Convexity of a Polygon"
 * by Peter Schorn and Frederick Fisher,
 * (schorn@inf.ethz.ch, fred@kpc.com)
 * in "Graphics Gems IV", Academic Press, 1994
 */

static inline int convexCompare(const FloatSize& delta)
{
    return (delta.width() > 0) ? -1 : /* x coord diff, second pt > first pt */
           (delta.width() < 0) ?  1 : /* x coord diff, second pt < first pt */
           (delta.height() > 0) ? -1 : /* x coord same, second pt > first pt */
           (delta.height() < 0) ?  1 : /* x coord same, second pt > first pt */
           0; /* second pt equals first point */
}

static inline float convexCross(const FloatSize& p, const FloatSize& q)
{
    return p.width() * q.height() - p.height() * q.width();
}

static inline bool convexCheckTriple(const FloatSize& dcur, const FloatSize& dprev, int* curDir, int* dirChanges, int* angleSign)
{
    int thisDir = convexCompare(dcur);
    if (thisDir == -*curDir)
        ++*dirChanges;
    *curDir = thisDir;
    float cross = convexCross(dprev, dcur);
    if (cross > 0) {
        if (*angleSign == -1)
            return false;
        *angleSign = 1;
    } else if (cross < 0) {
        if (*angleSign == 1)
            return false;
        *angleSign = -1;
    }
    return true;
}

bool isConvex(const FloatPoint* vertices, int nVertices)
{
    int dirChanges = 0, angleSign = 0;
    FloatPoint second, third;
    FloatSize dprev, dcur;

    /* Get different point, return if less than 3 diff points. */
    if (nVertices < 3)
        return false;
    int i = 1;
    while (true) {
        second = vertices[i++];
        dprev = second - vertices[0];
        if (dprev.width() || dprev.height())
            break;
        /* Check if out of points. Check here to avoid slowing down cases
         * without repeated points.
         */
        if (i >= nVertices)
            return false;
    }
    FloatPoint saveSecond = second;
    int curDir = convexCompare(dprev);        /* Find initial direction */
    while (i < nVertices) {
        /* Get different point, break if no more points */
        third = vertices[i++];
        dcur = third - second;
        if (!dcur.width() && !dcur.height())
            continue;

        /* Check current three points */
        if (!convexCheckTriple(dcur, dprev, &curDir, &dirChanges, &angleSign)) 
            return false;
        second = third;     /* Remember ptr to current point. */
        dprev = dcur;       /* Remember current delta. */
    }

    /* Must check for direction changes from last vertex back to first */
    third = vertices[0];                  /* Prepare for 'ConvexCheckTriple' */
    dcur = third - second;
    if (convexCompare(dcur)) {
        if (!convexCheckTriple(dcur, dprev, &curDir, &dirChanges, &angleSign)) 
            return false;
        second = third;     /* Remember ptr to current point. */
        dprev = dcur;       /* Remember current delta. */
    }

    /* and check for direction changes back to second vertex */
    dcur = saveSecond - second;
    if (!convexCheckTriple(dcur, dprev, &curDir, &dirChanges, &angleSign)) 
        return false;

    /* Decide on polygon type given accumulated status */
    if (dirChanges > 2)
        return false;

    if (angleSign > 0 || angleSign < 0)
        return true;
    return false;
}

} // namespace LoopBlinnMathUtils
} // namespace WebCore

#endif

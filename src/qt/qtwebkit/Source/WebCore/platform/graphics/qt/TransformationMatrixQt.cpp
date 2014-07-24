/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AffineTransform.h"
#include "TransformationMatrix.h"

#include "FloatRect.h"
#include "IntRect.h"

namespace WebCore {

TransformationMatrix::operator QTransform() const
{
    return QTransform(m11(), m12(), m14(), m21(), m22(), m24(), m41(), m42(), m44());
}

AffineTransform::operator QTransform() const
{
    return QTransform(a(), b(), c(), d(), e(), f());
}

TransformationMatrix::TransformationMatrix(const QTransform& transform)
{
    setMatrix(transform.m11(), transform.m12(), 0, transform.m13(),
              transform.m21(), transform.m22(), 0, transform.m23(),
              0, 0, 1, 0,
              transform.m31(), transform.m32(), 0, transform.m33());
}

TransformationMatrix::operator QMatrix4x4() const
{
    return QMatrix4x4(m11(), m12(), m13(), m14(),
                      m21(), m22(), m23(), m24(),
                      m31(), m32(), m33(), m34(),
                      m41(), m42(), m43(), m44());
}

TransformationMatrix::TransformationMatrix(const QMatrix4x4& matrix)
{
    setMatrix(matrix(0, 0), matrix(1, 0), matrix(2, 0), matrix(3, 0),
              matrix(0, 1), matrix(1, 1), matrix(2, 1), matrix(3, 1),
              matrix(0, 2), matrix(1, 2), matrix(2, 2), matrix(3, 2),
              matrix(0, 3), matrix(1, 3), matrix(2, 3), matrix(3, 3));
}

}

// vim: ts=4 sw=4 et

/*
 * Copyright (C) 2012 University of Szeged
 * Copyright (C) 2012 Tamas Czene <tczene@inf.u-szeged.hu>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(FILTERS) && ENABLE(OPENCL)
#include "SourceGraphic.h"

#include "Filter.h"
#include "FilterContextOpenCL.h"
#include "ImageBuffer.h"

namespace WebCore {

bool SourceGraphic::platformApplyOpenCL()
{
    FilterContextOpenCL* context = FilterContextOpenCL::context();

    if (!context)
        return false;

    platformApplySoftware();
    ImageBuffer* sourceImage = asImageBuffer();
    if (!sourceImage)
        return false;

    RefPtr<Uint8ClampedArray> sourceImageData = sourceImage->getUnmultipliedImageData(IntRect(IntPoint(), sourceImage->internalSize()));
    createOpenCLImageResult(sourceImageData->data());
    return true;
}

} // namespace WebCore

#endif // ENABLE(FILTERS) && ENABLE(OPENCL)

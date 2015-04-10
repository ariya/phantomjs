/*
 * Copyright (C) 2009, 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

// This all-in-one cpp file cuts down on template bloat to allow us to build our Windows release build.

#include "RenderSVGBlock.cpp"
#include "RenderSVGContainer.cpp"
#include "RenderSVGEllipse.cpp"
#include "RenderSVGForeignObject.cpp"
#include "RenderSVGGradientStop.cpp"
#include "RenderSVGHiddenContainer.cpp"
#include "RenderSVGImage.cpp"
#include "RenderSVGInline.cpp"
#include "RenderSVGInlineText.cpp"
#include "RenderSVGModelObject.cpp"
#include "RenderSVGPath.cpp"
#include "RenderSVGRect.cpp"
#include "RenderSVGResource.cpp"
#include "RenderSVGResourceClipper.cpp"
#include "RenderSVGResourceContainer.cpp"
#include "RenderSVGResourceFilter.cpp"
#include "RenderSVGResourceFilterPrimitive.cpp"
#include "RenderSVGResourceGradient.cpp"
#include "RenderSVGResourceLinearGradient.cpp"
#include "RenderSVGResourceMarker.cpp"
#include "RenderSVGResourceMasker.cpp"
#include "RenderSVGResourcePattern.cpp"
#include "RenderSVGResourceRadialGradient.cpp"
#include "RenderSVGResourceSolidColor.cpp"
#include "RenderSVGRoot.cpp"
#include "RenderSVGShape.cpp"
#include "RenderSVGTSpan.cpp"
#include "RenderSVGText.cpp"
#include "RenderSVGTextPath.cpp"
#include "RenderSVGTransformableContainer.cpp"
#include "RenderSVGViewportContainer.cpp"
#include "SVGInlineFlowBox.cpp"
#include "SVGInlineTextBox.cpp"
#include "SVGPathData.cpp"
#include "SVGRenderSupport.cpp"
#include "SVGRenderTreeAsText.cpp"
#include "SVGRenderingContext.cpp"
#include "SVGResources.cpp"
#include "SVGResourcesCache.cpp"
#include "SVGResourcesCycleSolver.cpp"
#include "SVGRootInlineBox.cpp"
#include "SVGTextChunk.cpp"
#include "SVGTextChunkBuilder.cpp"
#include "SVGTextLayoutAttributes.cpp"
#include "SVGTextLayoutAttributesBuilder.cpp"
#include "SVGTextLayoutEngine.cpp"
#include "SVGTextLayoutEngineBaseline.cpp"
#include "SVGTextLayoutEngineSpacing.cpp"
#include "SVGTextRunRenderingContext.cpp"
#include "SVGTextMetrics.cpp"
#include "SVGTextMetricsBuilder.cpp"
#include "SVGTextQuery.cpp"


/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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

#include "ContentData.cpp"
#include "CounterDirectives.cpp"
#include "FillLayer.cpp"
#include "KeyframeList.cpp"
#include "NinePieceImage.cpp"
#include "QuotesData.cpp"
#include "RenderStyle.cpp"
#include "SVGRenderStyle.cpp"
#include "SVGRenderStyleDefs.cpp"
#include "ShadowData.cpp"
#include "StyleBackgroundData.cpp"
#include "StyleBoxData.cpp"
#include "StyleCachedImage.cpp"
#include "StyleDeprecatedFlexibleBoxData.cpp"
#include "StyleFilterData.cpp"
#include "StyleFlexibleBoxData.cpp"
#include "StyleGeneratedImage.cpp"
#include "StyleGridData.cpp"
#include "StyleGridItemData.cpp"
#include "StyleInheritedData.cpp"
#include "StyleMarqueeData.cpp"
#include "StyleMultiColData.cpp"
#include "StyleRareInheritedData.cpp"
#include "StyleRareNonInheritedData.cpp"
#include "StyleSurroundData.cpp"
#include "StyleTransformData.cpp"
#include "StyleVisualData.cpp"

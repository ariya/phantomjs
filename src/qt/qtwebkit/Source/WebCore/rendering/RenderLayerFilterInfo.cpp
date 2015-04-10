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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_FILTERS)
#include "RenderLayerFilterInfo.h"

#include "FilterEffectRenderer.h"
#include "RenderLayer.h"

#if ENABLE(SVG)
#include "CachedSVGDocument.h"
#include "CachedSVGDocumentReference.h"
#include "SVGElement.h"
#include "SVGFilter.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#endif

#if ENABLE(CSS_SHADERS)
#include "CustomFilterOperation.h"
#include "CustomFilterProgram.h"
#endif

namespace WebCore {

RenderLayerFilterInfoMap* RenderLayerFilterInfo::s_filterMap = 0;

RenderLayerFilterInfo* RenderLayerFilterInfo::filterInfoForRenderLayer(const RenderLayer* layer)
{
    if (!s_filterMap)
        return 0;
    RenderLayerFilterInfoMap::iterator iter = s_filterMap->find(layer);
    return (iter != s_filterMap->end()) ? iter->value : 0;
}

RenderLayerFilterInfo* RenderLayerFilterInfo::createFilterInfoForRenderLayerIfNeeded(RenderLayer* layer)
{
    if (!s_filterMap)
        s_filterMap = new RenderLayerFilterInfoMap();
    
    RenderLayerFilterInfoMap::iterator iter = s_filterMap->find(layer);
    if (iter != s_filterMap->end()) {
        ASSERT(layer->hasFilterInfo());
        return iter->value;
    }
    
    RenderLayerFilterInfo* filter = new RenderLayerFilterInfo(layer);
    s_filterMap->set(layer, filter);
    layer->setHasFilterInfo(true);
    return filter;
}

void RenderLayerFilterInfo::removeFilterInfoForRenderLayer(RenderLayer* layer)
{
    if (!s_filterMap)
        return;
    RenderLayerFilterInfo* filter = s_filterMap->take(layer);
    if (s_filterMap->isEmpty()) {
        delete s_filterMap;
        s_filterMap = 0;
    }
    if (!filter) {
        ASSERT(!layer->hasFilterInfo());
        return;
    }
    layer->setHasFilterInfo(false);
    delete filter;
}

RenderLayerFilterInfo::RenderLayerFilterInfo(RenderLayer* layer)
    : m_layer(layer)
{
}

RenderLayerFilterInfo::~RenderLayerFilterInfo()
{
#if ENABLE(CSS_SHADERS)
    removeCustomFilterClients();
#endif
#if ENABLE(SVG)
    removeReferenceFilterClients();
#endif
}

void RenderLayerFilterInfo::setRenderer(PassRefPtr<FilterEffectRenderer> renderer)
{ 
    m_renderer = renderer; 
}

#if ENABLE(SVG)
void RenderLayerFilterInfo::notifyFinished(CachedResource*)
{
    RenderObject* renderer = m_layer->renderer();
    renderer->node()->setNeedsStyleRecalc(SyntheticStyleChange);
    renderer->repaint();
}

void RenderLayerFilterInfo::updateReferenceFilterClients(const FilterOperations& operations)
{
    removeReferenceFilterClients();
    for (size_t i = 0; i < operations.size(); ++i) {
        RefPtr<FilterOperation> filterOperation = operations.operations().at(i);
        if (filterOperation->getOperationType() != FilterOperation::REFERENCE)
            continue;
        ReferenceFilterOperation* referenceFilterOperation = static_cast<ReferenceFilterOperation*>(filterOperation.get());
        CachedSVGDocumentReference* documentReference = referenceFilterOperation->cachedSVGDocumentReference();
        CachedSVGDocument* cachedSVGDocument = documentReference ? documentReference->document() : 0;

        if (cachedSVGDocument) {
            // Reference is external; wait for notifyFinished().
            cachedSVGDocument->addClient(this);
            m_externalSVGReferences.append(cachedSVGDocument);
        } else {
            // Reference is internal; add layer as a client so we can trigger
            // filter repaint on SVG attribute change.
            Element* filter = m_layer->renderer()->node()->document()->getElementById(referenceFilterOperation->fragment());
            if (!filter || !filter->renderer() || !filter->renderer()->isSVGResourceFilter())
                continue;
            filter->renderer()->toRenderSVGResourceContainer()->addClientRenderLayer(m_layer);
            m_internalSVGReferences.append(filter);
        }
    }
}

void RenderLayerFilterInfo::removeReferenceFilterClients()
{
    for (size_t i = 0; i < m_externalSVGReferences.size(); ++i)
        m_externalSVGReferences.at(i)->removeClient(this);
    m_externalSVGReferences.clear();
    for (size_t i = 0; i < m_internalSVGReferences.size(); ++i) {
        Element* filter = m_internalSVGReferences.at(i).get();
        if (!filter->renderer())
            continue;
        filter->renderer()->toRenderSVGResourceContainer()->removeClientRenderLayer(m_layer);
    }
    m_internalSVGReferences.clear();
}
#endif

#if ENABLE(CSS_SHADERS)
void RenderLayerFilterInfo::notifyCustomFilterProgramLoaded(CustomFilterProgram*)
{
    RenderObject* renderer = m_layer->renderer();
    renderer->node()->setNeedsStyleRecalc(SyntheticStyleChange);
    renderer->repaint();
}

void RenderLayerFilterInfo::updateCustomFilterClients(const FilterOperations& operations)
{
    if (!operations.size()) {
        removeCustomFilterClients();
        return;
    }
    CustomFilterProgramList cachedCustomFilterPrograms;
    for (size_t i = 0; i < operations.size(); ++i) {
        const FilterOperation* filterOperation = operations.at(i);
        if (filterOperation->getOperationType() != FilterOperation::CUSTOM)
            continue;
        const CustomFilterOperation* customFilterOperation = static_cast<const CustomFilterOperation*>(filterOperation);
        RefPtr<CustomFilterProgram> program = customFilterOperation->program();
        cachedCustomFilterPrograms.append(program);
        program->addClient(this);
    }
    // Remove the old clients here, after we've added the new ones, so that we don't flicker if some shaders are unchanged.
    removeCustomFilterClients();
    m_cachedCustomFilterPrograms.swap(cachedCustomFilterPrograms);
}

void RenderLayerFilterInfo::removeCustomFilterClients()
{
    for (size_t i = 0; i < m_cachedCustomFilterPrograms.size(); ++i)
        m_cachedCustomFilterPrograms.at(i)->removeClient(this);
    m_cachedCustomFilterPrograms.clear();
}
#endif

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)

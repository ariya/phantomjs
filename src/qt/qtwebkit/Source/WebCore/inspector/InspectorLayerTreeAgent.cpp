/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#if ENABLE(INSPECTOR)

#include "InspectorLayerTreeAgent.h"

#include "IdentifiersFactory.h"
#include "InspectorDOMAgent.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "IntRect.h"
#include "PseudoElement.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "RenderLayerCompositor.h"
#include "RenderView.h"

namespace WebCore {

namespace LayerTreeAgentState {
static const char layerTreeAgentEnabled[] = "layerTreeAgentEnabled";
};

InspectorLayerTreeAgent::InspectorLayerTreeAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state)
    : InspectorBaseAgent<InspectorLayerTreeAgent>("LayerTree", instrumentingAgents, state)
    , m_frontend(0)
{
}

InspectorLayerTreeAgent::~InspectorLayerTreeAgent()
{
    reset();
}

void InspectorLayerTreeAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->layertree();
}

void InspectorLayerTreeAgent::clearFrontend()
{
    m_frontend = 0;
    disable(0);
}

void InspectorLayerTreeAgent::restore()
{
    if (m_state->getBoolean(LayerTreeAgentState::layerTreeAgentEnabled))
        enable(0);
}

void InspectorLayerTreeAgent::reset()
{
    m_documentLayerToIdMap.clear();
    m_idToLayer.clear();
    m_pseudoElementToIdMap.clear();
    m_idToPseudoElement.clear();
}

void InspectorLayerTreeAgent::enable(ErrorString*)
{
    m_state->setBoolean(LayerTreeAgentState::layerTreeAgentEnabled, true);
    m_instrumentingAgents->setInspectorLayerTreeAgent(this);
}

void InspectorLayerTreeAgent::disable(ErrorString*)
{
    if (!m_state->getBoolean(LayerTreeAgentState::layerTreeAgentEnabled))
        return;
    m_state->setBoolean(LayerTreeAgentState::layerTreeAgentEnabled, false);
    m_instrumentingAgents->setInspectorLayerTreeAgent(0);
}

void InspectorLayerTreeAgent::layerTreeDidChange()
{
    m_frontend->layerTreeDidChange();
}

void InspectorLayerTreeAgent::renderLayerDestroyed(const RenderLayer* renderLayer)
{
    unbind(renderLayer);
}

void InspectorLayerTreeAgent::pseudoElementDestroyed(PseudoElement* pseudoElement)
{
    unbindPseudoElement(pseudoElement);
}

void InspectorLayerTreeAgent::layersForNode(ErrorString* errorString, int nodeId, RefPtr<TypeBuilder::Array<TypeBuilder::LayerTree::Layer> >& layers)
{
    layers = TypeBuilder::Array<TypeBuilder::LayerTree::Layer>::create();

    Node* node = m_instrumentingAgents->inspectorDOMAgent()->nodeForId(nodeId);
    if (!node) {
        *errorString = "Provided node id doesn't match any known node";
        return;
    }

    RenderObject* renderer = node->renderer();
    if (!renderer) {
        *errorString = "Node for provided node id doesn't have a renderer";
        return;
    }

    gatherLayersUsingRenderObjectHierarchy(errorString, renderer, layers);
}

void InspectorLayerTreeAgent::gatherLayersUsingRenderObjectHierarchy(ErrorString* errorString, RenderObject* renderer, RefPtr<TypeBuilder::Array<TypeBuilder::LayerTree::Layer> >& layers)
{
    if (renderer->hasLayer()) {
        gatherLayersUsingRenderLayerHierarchy(errorString, toRenderLayerModelObject(renderer)->layer(), layers);
        return;
    }

    for (renderer = renderer->firstChild(); renderer; renderer = renderer->nextSibling())
        gatherLayersUsingRenderObjectHierarchy(errorString, renderer, layers);
}

void InspectorLayerTreeAgent::gatherLayersUsingRenderLayerHierarchy(ErrorString* errorString, RenderLayer* renderLayer, RefPtr<TypeBuilder::Array<TypeBuilder::LayerTree::Layer> >& layers)
{
    if (renderLayer->isComposited())
        layers->addItem(buildObjectForLayer(errorString, renderLayer));

    for (renderLayer = renderLayer->firstChild(); renderLayer; renderLayer = renderLayer->nextSibling())
        gatherLayersUsingRenderLayerHierarchy(errorString, renderLayer, layers);
}

PassRefPtr<TypeBuilder::LayerTree::Layer> InspectorLayerTreeAgent::buildObjectForLayer(ErrorString* errorString, RenderLayer* renderLayer)
{
    RenderObject* renderer = renderLayer->renderer();
    RenderLayerBacking* backing = renderLayer->backing();
    Node* node = renderer->node();

    bool isReflection = renderLayer->isReflection();
    bool isGenerated = (isReflection ? renderer->parent() : renderer)->isBeforeOrAfterContent();
    bool isAnonymous = renderer->isAnonymous();

    if (isReflection && isGenerated)
        node = renderer->parent()->generatingNode();
    else if (isGenerated)
        node = renderer->generatingNode();
    else if (isReflection || isAnonymous)
        node = renderer->parent()->node();

    // Basic set of properties.
    RefPtr<TypeBuilder::LayerTree::Layer> layerObject = TypeBuilder::LayerTree::Layer::create()
        .setLayerId(bind(renderLayer))
        .setNodeId(idForNode(errorString, node))
        .setBounds(buildObjectForIntRect(renderer->absoluteBoundingBoxRect()))
        .setMemory(backing->backingStoreMemoryEstimate())
        .setCompositedBounds(buildObjectForIntRect(enclosingIntRect(backing->compositedBounds())))
        .setPaintCount(backing->graphicsLayer()->repaintCount());

    if (node && node->shadowHost())
        layerObject->setIsInShadowTree(true);

    if (isReflection)
        layerObject->setIsReflection(true);

    if (isGenerated) {
        if (isReflection)
            renderer = renderer->parent();
        layerObject->setIsGeneratedContent(true);
        layerObject->setPseudoElementId(bindPseudoElement(static_cast<PseudoElement*>(renderer->node())));
        if (renderer->isBeforeContent())
            layerObject->setPseudoElement("before");
        else if (renderer->isAfterContent())
            layerObject->setPseudoElement("after");
    }

    if (isAnonymous) {
        layerObject->setIsAnonymous(true);
        if (RenderStyle* style = renderer->style()) {
            if (style->styleType() == FIRST_LETTER)
                layerObject->setPseudoElement("first-letter");
            else if (style->styleType() == FIRST_LINE)
                layerObject->setPseudoElement("first-line");
        }
    }

    return layerObject;
}

int InspectorLayerTreeAgent::idForNode(ErrorString* errorString, Node* node)
{
    if (!node)
        return 0;

    InspectorDOMAgent* domAgent = m_instrumentingAgents->inspectorDOMAgent();
    
    int nodeId = domAgent->boundNodeId(node);
    if (!nodeId)
        nodeId = domAgent->pushNodeToFrontend(errorString, domAgent->boundNodeId(node->document()), node);

    return nodeId;
}

PassRefPtr<TypeBuilder::LayerTree::IntRect> InspectorLayerTreeAgent::buildObjectForIntRect(const IntRect& rect)
{
    return TypeBuilder::LayerTree::IntRect::create()
        .setX(rect.x())
        .setY(rect.y())
        .setWidth(rect.width())
        .setHeight(rect.height()).release();
}

void InspectorLayerTreeAgent::reasonsForCompositingLayer(ErrorString* errorString, const String& layerId, RefPtr<TypeBuilder::LayerTree::CompositingReasons>& compositingReasons)
{
    const RenderLayer* renderLayer = m_idToLayer.get(layerId);

    if (!renderLayer) {
        *errorString = "Could not find a bound layer for the provided id";
        return;
    }

    CompositingReasons reasonsBitmask = renderLayer->compositor()->reasonsForCompositing(renderLayer);
    compositingReasons = TypeBuilder::LayerTree::CompositingReasons::create();

    if (reasonsBitmask & CompositingReason3DTransform)
        compositingReasons->setTransform3D(true);

    if (reasonsBitmask & CompositingReasonVideo)
        compositingReasons->setVideo(true);
    else if (reasonsBitmask & CompositingReasonCanvas)
        compositingReasons->setCanvas(true);
    else if (reasonsBitmask & CompositingReasonPlugin)
        compositingReasons->setPlugin(true);
    else if (reasonsBitmask & CompositingReasonIFrame)
        compositingReasons->setIFrame(true);
    
    if (reasonsBitmask & CompositingReasonBackfaceVisibilityHidden)
        compositingReasons->setBackfaceVisibilityHidden(true);

    if (reasonsBitmask & CompositingReasonClipsCompositingDescendants)
        compositingReasons->setClipsCompositingDescendants(true);

    if (reasonsBitmask & CompositingReasonAnimation)
        compositingReasons->setAnimation(true);

    if (reasonsBitmask & CompositingReasonFilters)
        compositingReasons->setFilters(true);

    if (reasonsBitmask & CompositingReasonPositionFixed)
        compositingReasons->setPositionFixed(true);

    if (reasonsBitmask & CompositingReasonPositionSticky)
        compositingReasons->setPositionSticky(true);

    if (reasonsBitmask & CompositingReasonOverflowScrollingTouch)
        compositingReasons->setOverflowScrollingTouch(true);

    if (reasonsBitmask & CompositingReasonStacking)
        compositingReasons->setStacking(true);

    if (reasonsBitmask & CompositingReasonOverlap)
        compositingReasons->setOverlap(true);

    if (reasonsBitmask & CompositingReasonNegativeZIndexChildren)
        compositingReasons->setNegativeZIndexChildren(true);

    if (reasonsBitmask & CompositingReasonTransformWithCompositedDescendants)
        compositingReasons->setTransformWithCompositedDescendants(true);

    if (reasonsBitmask & CompositingReasonOpacityWithCompositedDescendants)
        compositingReasons->setOpacityWithCompositedDescendants(true);

    if (reasonsBitmask & CompositingReasonMaskWithCompositedDescendants)
        compositingReasons->setMaskWithCompositedDescendants(true);

    if (reasonsBitmask & CompositingReasonReflectionWithCompositedDescendants)
        compositingReasons->setReflectionWithCompositedDescendants(true);

    if (reasonsBitmask & CompositingReasonFilterWithCompositedDescendants)
        compositingReasons->setFilterWithCompositedDescendants(true);
            
    if (reasonsBitmask & CompositingReasonBlendingWithCompositedDescendants)
        compositingReasons->setBlendingWithCompositedDescendants(true);

    if (reasonsBitmask & CompositingReasonPerspective)
        compositingReasons->setPerspective(true);

    if (reasonsBitmask & CompositingReasonPreserve3D)
        compositingReasons->setPreserve3D(true);

    if (reasonsBitmask & CompositingReasonRoot)
        compositingReasons->setRoot(true);
}

String InspectorLayerTreeAgent::bind(const RenderLayer* layer)
{
    if (!layer)
        return emptyString();
    String identifier = m_documentLayerToIdMap.get(layer);
    if (identifier.isNull()) {
        identifier = IdentifiersFactory::createIdentifier();
        m_documentLayerToIdMap.set(layer, identifier);
        m_idToLayer.set(identifier, layer);
    }
    return identifier;
}

void InspectorLayerTreeAgent::unbind(const RenderLayer* layer)
{
    HashMap<const RenderLayer*, String>::iterator iterator = m_documentLayerToIdMap.find(layer);
    if (iterator == m_documentLayerToIdMap.end())
        return;
    m_idToLayer.remove(iterator->value);
    m_documentLayerToIdMap.remove(iterator);
}

String InspectorLayerTreeAgent::bindPseudoElement(PseudoElement* pseudoElement)
{
    if (!pseudoElement)
        return emptyString();
    String identifier = m_pseudoElementToIdMap.get(pseudoElement);
    if (identifier.isNull()) {
        identifier = IdentifiersFactory::createIdentifier();
        m_pseudoElementToIdMap.set(pseudoElement, identifier);
        m_idToPseudoElement.set(identifier, pseudoElement);
    }
    return identifier;
}

void InspectorLayerTreeAgent::unbindPseudoElement(PseudoElement* pseudoElement)
{
    HashMap<PseudoElement*, String>::iterator iterator = m_pseudoElementToIdMap.find(pseudoElement);
    if (iterator == m_pseudoElementToIdMap.end())
        return;
    m_idToPseudoElement.remove(iterator->value);
    m_pseudoElementToIdMap.remove(iterator);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
#endif // USE(ACCELERATED_COMPOSITING)

/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RemoteLayerTreeTransaction.h"

#include "ArgumentCoders.h"
#include "MessageDecoder.h"
#include "MessageEncoder.h"
#include "RemoteGraphicsLayer.h"
#include "WebCoreArgumentCoders.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

RemoteLayerTreeTransaction::LayerProperties::LayerProperties()
    : changedProperties(NoChange)
{
}

void RemoteLayerTreeTransaction::LayerProperties::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << changedProperties;

    if (changedProperties & NameChanged)
        encoder << name;

    if (changedProperties & ChildrenChanged)
        encoder << children;

    if (changedProperties & PositionChanged)
        encoder << position;

    if (changedProperties & SizeChanged)
        encoder << size;
}

bool RemoteLayerTreeTransaction::LayerProperties::decode(CoreIPC::ArgumentDecoder& decoder, LayerProperties& result)
{
    if (!decoder.decode(result.changedProperties))
        return false;

    if (result.changedProperties & NameChanged) {
        if (!decoder.decode(result.name))
            return false;
    }

    if (result.changedProperties & ChildrenChanged) {
        if (!decoder.decode(result.children))
            return false;

        for (auto layerID: result.children) {
            if (!layerID)
                return false;
        }
    }

    if (result.changedProperties & PositionChanged) {
        if (!decoder.decode(result.position))
            return false;
    }

    if (result.changedProperties & SizeChanged) {
        if (!decoder.decode(result.size))
            return false;
    }
    return true;
}

RemoteLayerTreeTransaction::RemoteLayerTreeTransaction()
{
}

RemoteLayerTreeTransaction::~RemoteLayerTreeTransaction()
{
}

void RemoteLayerTreeTransaction::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_rootLayerID;
    encoder << m_changedLayerProperties;
    encoder << m_destroyedLayerIDs;
}

bool RemoteLayerTreeTransaction::decode(CoreIPC::ArgumentDecoder& decoder, RemoteLayerTreeTransaction& result)
{
    if (!decoder.decode(result.m_rootLayerID))
        return false;
    if (!result.m_rootLayerID)
        return false;

    if (!decoder.decode(result.m_changedLayerProperties))
        return false;

    if (!decoder.decode(result.m_destroyedLayerIDs))
        return false;
    for (uint64_t layerID: result.m_destroyedLayerIDs) {
        if (!layerID)
            return false;
    }

    return true;
}

void RemoteLayerTreeTransaction::setRootLayerID(uint64_t rootLayerID)
{
    ASSERT_ARG(rootLayerID, rootLayerID);

    m_rootLayerID = rootLayerID;
}

void RemoteLayerTreeTransaction::layerPropertiesChanged(const RemoteGraphicsLayer* graphicsLayer, unsigned changedProperties)
{
    LayerProperties& layerProperties = m_changedLayerProperties.add(graphicsLayer->layerID(), LayerProperties()).iterator->value;

    layerProperties.changedProperties |= changedProperties;

    if (changedProperties & NameChanged)
        layerProperties.name = graphicsLayer->name();

    if (changedProperties & ChildrenChanged) {
        const Vector<GraphicsLayer*>& children = graphicsLayer->children();

        ASSERT(layerProperties.children.isEmpty());
        layerProperties.children.reserveCapacity(children.size());
        for (size_t i = 0; i < children.size(); ++i) {
            RemoteGraphicsLayer* childLayer = static_cast<RemoteGraphicsLayer*>(children[i]);
            layerProperties.children.uncheckedAppend(childLayer->layerID());
        }
    }

    if (changedProperties & PositionChanged)
        layerProperties.position = graphicsLayer->position();

    if (changedProperties & SizeChanged)
        layerProperties.size = graphicsLayer->size();
}

void RemoteLayerTreeTransaction::setDestroyedLayerIDs(Vector<uint64_t> destroyedLayerIDs)
{
    m_destroyedLayerIDs = std::move(destroyedLayerIDs);
}

#ifndef NDEBUG

static void writeIndent(StringBuilder& builder, int indent)
{
    for (int i = 0; i < indent; ++i)
        builder.append(' ');
}

static void dumpChangedLayers(StringBuilder& builder, const HashMap<uint64_t, RemoteLayerTreeTransaction::LayerProperties>& changedLayerProperties)
{
    if (changedLayerProperties.isEmpty())
        return;

    writeIndent(builder, 1);
    builder.append("(changed-layers\n");

    // Dump the layer properties sorted by layer ID.
    Vector<uint64_t> layerIDs;
    copyKeysToVector(changedLayerProperties, layerIDs);
    std::sort(layerIDs.begin(), layerIDs.end());

    for (uint64_t layerID: layerIDs) {
        const RemoteLayerTreeTransaction::LayerProperties& layerProperties = changedLayerProperties.get(layerID);

        writeIndent(builder, 2);
        builder.append("(layer ");
        builder.appendNumber(layerID);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::NameChanged) {
            builder.append('\n');
            writeIndent(builder, 3);
            builder.append("(name \"");
            builder.append(layerProperties.name);
            builder.append("\")");
        }

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::ChildrenChanged) {
            builder.append('\n');
            writeIndent(builder, 3);
            builder.append("(children (");
            for (size_t i = 0; i < layerProperties.children.size(); ++i) {
                if (i)
                    builder.append(' ');
                builder.appendNumber(layerProperties.children[i]);
            }

            builder.append(")");
        }

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::PositionChanged) {
            builder.append('\n');
            writeIndent(builder, 3);
            builder.append("(position ");
            builder.append(String::number(layerProperties.position.x()));
            builder.append(" ");
            builder.append(String::number(layerProperties.position.y()));
            builder.append(')');
        }

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::SizeChanged) {
            builder.append('\n');
            writeIndent(builder, 3);
            builder.append("(size ");
            builder.append(String::number(layerProperties.size.width()));
            builder.append(" ");
            builder.append(String::number(layerProperties.size.height()));
            builder.append(')');
        }

        builder.append(")\n");
    }
}

void RemoteLayerTreeTransaction::dump() const
{
    StringBuilder builder;

    builder.append("(\n");

    writeIndent(builder, 1);
    builder.append("(root-layer ");
    builder.appendNumber(m_rootLayerID);
    builder.append(")\n");

    dumpChangedLayers(builder, m_changedLayerProperties);

    if (!m_destroyedLayerIDs.isEmpty()) {
        writeIndent(builder, 1);
        builder.append("(destroyed-layers ");
        for (size_t i = 0; i < m_destroyedLayerIDs.size(); ++i) {
            if (i)
                builder.append(' ');
            builder.appendNumber(m_destroyedLayerIDs[i]);
        }
        builder.append(")\n");
    }
    builder.append(")\n");

    fprintf(stderr, "%s", builder.toString().utf8().data());
}

#endif // NDEBUG

} // namespace WebKit

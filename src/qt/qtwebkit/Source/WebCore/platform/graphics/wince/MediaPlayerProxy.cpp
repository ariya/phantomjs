/*
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#if ENABLE(VIDEO)

#include "config.h"
#include "MediaPlayerProxy.h"

#include "BridgeJSC.h"
#include "DocumentLoader.h"
#include "HTMLPlugInElement.h"
#include "HTMLVideoElement.h"
#include "JSDOMBinding.h"
#include "JSPluginElementFunctions.h"
#include "MediaPlayer.h"
#include "Node.h"
#include "PluginView.h"
#include "RenderPartObject.h"
#include "RenderWidget.h"
#include "Widget.h"
#include "c_class.h"
#include "c_instance.h"
#include "c_runtime.h"
#include "npruntime_impl.h"
#include <runtime/Identifier.h>
#include <wtf/text/WTFString.h>

using namespace JSC;

namespace WebCore {

using namespace Bindings;
using namespace HTMLNames;

WebMediaPlayerProxy::WebMediaPlayerProxy(MediaPlayer* player)
    : m_mediaPlayer(player)
    , m_init(false)
    , m_hasSentResponseToPlugin(false)
{
    if (!m_init)
        initEngine();
}

WebMediaPlayerProxy::~WebMediaPlayerProxy()
{
    m_instance.release();
}

ScriptInstance WebMediaPlayerProxy::pluginInstance()
{
    if (!m_instance) {
        RenderObject* r = element()->renderer();
        if (!r || !r->isWidget())
            return 0;

        Frame* frame = element()->document()->frame();

        RenderWidget* renderWidget = static_cast<RenderWidget*>(element()->renderer());
        if (renderWidget && renderWidget->widget())
            m_instance = frame->script()->createScriptInstanceForWidget(renderWidget->widget());
    }

    return m_instance;
}

void WebMediaPlayerProxy::load(const String& url)
{
    if (!m_init)
        initEngine();
    if (m_init)
        invokeMethod("play");
}

void WebMediaPlayerProxy::initEngine()
{
    HTMLMediaElement* element = toHTMLMediaElement(m_mediaPlayer->mediaPlayerClient());
    String url = element->initialURL();

    if (url.isEmpty())
        return;

    Frame* frame = element->document()->frame();
    Vector<String> paramNames;
    Vector<String> paramValues;
    String serviceType;

    // add all attributes set on the embed object
    if (element->hasAttributes()) {
        for (unsigned i = 0; i < element->attributeCount(); ++i) {
            Attribute* it = element->attributeItem(i);
            paramNames.append(it->name().localName().string());
            paramValues.append(it->value().string());
        }
    }
    serviceType = "application/x-mplayer2";
    frame->loader()->subframeLoader()->requestObject(static_cast<RenderPartObject*>(element->renderer()), url, nullAtom, serviceType, paramNames, paramValues);
    m_init = true;

}

HTMLMediaElement* WebMediaPlayerProxy::element()
{
    return toHTMLMediaElement(m_mediaPlayer->mediaPlayerClient());

}

void WebMediaPlayerProxy::invokeMethod(const String& methodName)
{
    Frame* frame = element()->document()->frame();
    RootObject *root = frame->script()->bindingRootObject();
    if (!root)
        return;
    ExecState *exec = root->globalObject()->globalExec();
    Instance* instance = pluginInstance().get();
    if (!instance)
        return;

    instance->begin();
    Class *aClass = instance->getClass();
    Identifier iden(exec, methodName);
    MethodList methodList = aClass->methodsNamed(iden, instance);
    ArgList args;
    instance->invokeMethod(exec, methodList , args);
    instance->end();
}

}

#endif

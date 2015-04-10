/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#if ENABLE(WEBGL)

#include "JSWebGLRenderingContext.h"

#include "EXTDrawBuffers.h"
#include "EXTTextureFilterAnisotropic.h"
#include "ExceptionCode.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "JSEXTDrawBuffers.h"
#include "JSEXTTextureFilterAnisotropic.h"
#include "JSFloat32Array.h"
#include "JSHTMLCanvasElement.h"
#include "JSHTMLImageElement.h"
#include "JSImageData.h"
#include "JSInt32Array.h"
#include "JSOESElementIndexUint.h"
#include "JSOESStandardDerivatives.h"
#include "JSOESTextureFloat.h"
#include "JSOESTextureHalfFloat.h"
#include "JSOESVertexArrayObject.h"
#include "JSUint32Array.h"
#include "JSUint8Array.h"
#include "JSWebGLBuffer.h"
#include "JSWebGLCompressedTextureATC.h"
#include "JSWebGLCompressedTexturePVRTC.h"
#include "JSWebGLCompressedTextureS3TC.h"
#include "JSWebGLDebugRendererInfo.h"
#include "JSWebGLDebugShaders.h"
#include "JSWebGLDepthTexture.h"
#include "JSWebGLFramebuffer.h"
#include "JSWebGLLoseContext.h"
#include "JSWebGLProgram.h"
#include "JSWebGLRenderbuffer.h"
#include "JSWebGLShader.h"
#include "JSWebGLTexture.h"
#include "JSWebGLUniformLocation.h"
#include "JSWebGLVertexArrayObjectOES.h"
#include "JSWebKitCSSMatrix.h"
#include "NotImplemented.h"
#include "OESElementIndexUint.h"
#include "OESStandardDerivatives.h"
#include "OESTextureFloat.h"
#include "OESTextureHalfFloat.h"
#include "OESVertexArrayObject.h"
#include "WebGLBuffer.h"
#include "WebGLCompressedTextureATC.h"
#include "WebGLCompressedTexturePVRTC.h"
#include "WebGLCompressedTextureS3TC.h"
#include "WebGLDebugRendererInfo.h"
#include "WebGLDebugShaders.h"
#include "WebGLDepthTexture.h"
#include "WebGLExtension.h"
#include "WebGLFramebuffer.h"
#include "WebGLGetInfo.h"
#include "WebGLLoseContext.h"
#include "WebGLProgram.h"
#include "WebGLRenderingContext.h"
#include "WebGLVertexArrayObjectOES.h"
#include <runtime/Error.h>
#include <runtime/JSArray.h>
#include <wtf/FastMalloc.h>
#include <wtf/Float32Array.h>
#include <wtf/Int32Array.h>
#include <wtf/Uint32Array.h>

#if ENABLE(VIDEO)
#include "HTMLVideoElement.h"
#include "JSHTMLVideoElement.h"
#endif

using namespace JSC;

namespace WebCore {

static JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, const WebGLGetInfo& info)
{
    switch (info.getType()) {
    case WebGLGetInfo::kTypeBool:
        return jsBoolean(info.getBool());
    case WebGLGetInfo::kTypeBoolArray: {
        MarkedArgumentBuffer list;
        const Vector<bool>& value = info.getBoolArray();
        for (size_t ii = 0; ii < value.size(); ++ii)
            list.append(jsBoolean(value[ii]));
        return constructArray(exec, 0, globalObject, list);
    }
    case WebGLGetInfo::kTypeFloat:
        return jsNumber(info.getFloat());
    case WebGLGetInfo::kTypeInt:
        return jsNumber(info.getInt());
    case WebGLGetInfo::kTypeNull:
        return jsNull();
    case WebGLGetInfo::kTypeString:
        return jsStringWithCache(exec, info.getString());
    case WebGLGetInfo::kTypeUnsignedInt:
        return jsNumber(info.getUnsignedInt());
    case WebGLGetInfo::kTypeWebGLBuffer:
        return toJS(exec, globalObject, info.getWebGLBuffer());
    case WebGLGetInfo::kTypeWebGLFloatArray:
        return toJS(exec, globalObject, info.getWebGLFloatArray());
    case WebGLGetInfo::kTypeWebGLFramebuffer:
        return toJS(exec, globalObject, info.getWebGLFramebuffer());
    case WebGLGetInfo::kTypeWebGLIntArray:
        return toJS(exec, globalObject, info.getWebGLIntArray());
    // FIXME: implement WebGLObjectArray
    // case WebGLGetInfo::kTypeWebGLObjectArray:
    case WebGLGetInfo::kTypeWebGLProgram:
        return toJS(exec, globalObject, info.getWebGLProgram());
    case WebGLGetInfo::kTypeWebGLRenderbuffer:
        return toJS(exec, globalObject, info.getWebGLRenderbuffer());
    case WebGLGetInfo::kTypeWebGLTexture:
        return toJS(exec, globalObject, info.getWebGLTexture());
    case WebGLGetInfo::kTypeWebGLUnsignedByteArray:
        return toJS(exec, globalObject, info.getWebGLUnsignedByteArray());
    case WebGLGetInfo::kTypeWebGLUnsignedIntArray:
        return toJS(exec, globalObject, info.getWebGLUnsignedIntArray());
    case WebGLGetInfo::kTypeWebGLVertexArrayObjectOES:
        return toJS(exec, globalObject, info.getWebGLVertexArrayObjectOES());
    default:
        notImplemented();
        return jsUndefined();
    }
}

enum ObjectType {
    kBuffer, kRenderbuffer, kTexture, kVertexAttrib
};

static JSValue getObjectParameter(JSWebGLRenderingContext* obj, ExecState* exec, ObjectType objectType)
{
    if (exec->argumentCount() != 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(obj->impl());
    unsigned target = exec->argument(0).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    unsigned pname = exec->argument(1).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    WebGLGetInfo info;
    switch (objectType) {
    case kBuffer:
        info = context->getBufferParameter(target, pname, ec);
        break;
    case kRenderbuffer:
        info = context->getRenderbufferParameter(target, pname, ec);
        break;
    case kTexture:
        info = context->getTexParameter(target, pname, ec);
        break;
    case kVertexAttrib:
        // target => index
        info = context->getVertexAttrib(target, pname, ec);
        break;
    default:
        notImplemented();
        break;
    }
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return toJS(exec, obj->globalObject(), info);
}

enum WhichProgramCall {
    kProgramParameter, kUniform
};

static JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, WebGLExtension* extension)
{
    if (!extension)
        return jsNull();
    switch (extension->getName()) {
    case WebGLExtension::WebGLLoseContextName:
        return toJS(exec, globalObject, static_cast<WebGLLoseContext*>(extension));
    case WebGLExtension::EXTDrawBuffersName:
        return toJS(exec, globalObject, static_cast<EXTDrawBuffers*>(extension));
    case WebGLExtension::EXTTextureFilterAnisotropicName:
        return toJS(exec, globalObject, static_cast<EXTTextureFilterAnisotropic*>(extension));
    case WebGLExtension::OESStandardDerivativesName:
        return toJS(exec, globalObject, static_cast<OESStandardDerivatives*>(extension));
    case WebGLExtension::OESTextureFloatName:
        return toJS(exec, globalObject, static_cast<OESTextureFloat*>(extension));
    case WebGLExtension::OESTextureHalfFloatName:
        return toJS(exec, globalObject, static_cast<OESTextureHalfFloat*>(extension));
    case WebGLExtension::OESVertexArrayObjectName:
        return toJS(exec, globalObject, static_cast<OESVertexArrayObject*>(extension));
    case WebGLExtension::OESElementIndexUintName:
        return toJS(exec, globalObject, static_cast<OESElementIndexUint*>(extension));
    case WebGLExtension::WebGLDebugRendererInfoName:
        return toJS(exec, globalObject, static_cast<WebGLDebugRendererInfo*>(extension));
    case WebGLExtension::WebGLDebugShadersName:
        return toJS(exec, globalObject, static_cast<WebGLDebugShaders*>(extension));
    case WebGLExtension::WebGLCompressedTextureATCName:
        return toJS(exec, globalObject, static_cast<WebGLCompressedTextureATC*>(extension));
    case WebGLExtension::WebGLCompressedTexturePVRTCName:
        return toJS(exec, globalObject, static_cast<WebGLCompressedTexturePVRTC*>(extension));
    case WebGLExtension::WebGLCompressedTextureS3TCName:
        return toJS(exec, globalObject, static_cast<WebGLCompressedTextureS3TC*>(extension));
    case WebGLExtension::WebGLDepthTextureName:
        return toJS(exec, globalObject, static_cast<WebGLDepthTexture*>(extension));
    }
    ASSERT_NOT_REACHED();
    return jsNull();
}

void JSWebGLRenderingContext::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSWebGLRenderingContext* thisObject = jsCast<JSWebGLRenderingContext*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);
    visitor.addOpaqueRoot(thisObject->impl());
}

JSValue JSWebGLRenderingContext::getAttachedShaders(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return throwError(exec, createNotEnoughArgumentsError(exec));
    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLProgram::s_info))
        return throwTypeError(exec);
    WebGLProgram* program = toWebGLProgram(exec->argument(0));
    if (exec->hadException())
        return jsNull();
    Vector<RefPtr<WebGLShader> > shaders;
    bool succeed = context->getAttachedShaders(program, shaders, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsNull();
    }
    if (!succeed)
        return jsNull();
    MarkedArgumentBuffer list;
    for (size_t ii = 0; ii < shaders.size(); ++ii)
        list.append(toJS(exec, globalObject(), shaders[ii].get()));
    return constructArray(exec, 0, globalObject(), list);
}

JSValue JSWebGLRenderingContext::getExtension(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    const String name = exec->argument(0).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();
    WebGLExtension* extension = context->getExtension(name);
    return toJS(exec, globalObject(), extension);
}

JSValue JSWebGLRenderingContext::getBufferParameter(ExecState* exec)
{
    return getObjectParameter(this, exec, kBuffer);
}

JSValue JSWebGLRenderingContext::getFramebufferAttachmentParameter(ExecState* exec)
{
    if (exec->argumentCount() != 3)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    unsigned target = exec->argument(0).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    unsigned attachment = exec->argument(1).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    unsigned pname = exec->argument(2).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    WebGLGetInfo info = context->getFramebufferAttachmentParameter(target, attachment, pname, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return toJS(exec, globalObject(), info);
}

JSValue JSWebGLRenderingContext::getParameter(ExecState* exec)
{
    if (exec->argumentCount() != 1)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    unsigned pname = exec->argument(0).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    WebGLGetInfo info = context->getParameter(pname, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return toJS(exec, globalObject(), info);
}

JSValue JSWebGLRenderingContext::getProgramParameter(ExecState* exec)
{
    if (exec->argumentCount() != 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLProgram::s_info))
        return throwTypeError(exec);
    WebGLProgram* program = toWebGLProgram(exec->argument(0));
    unsigned pname = exec->argument(1).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    WebGLGetInfo info = context->getProgramParameter(program, pname, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return toJS(exec, globalObject(), info);
}

JSValue JSWebGLRenderingContext::getRenderbufferParameter(ExecState* exec)
{
    return getObjectParameter(this, exec, kRenderbuffer);
}

JSValue JSWebGLRenderingContext::getShaderParameter(ExecState* exec)
{
    if (exec->argumentCount() != 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLShader::s_info))
        return throwTypeError(exec);
    WebGLShader* shader = toWebGLShader(exec->argument(0));
    unsigned pname = exec->argument(1).toInt32(exec);
    if (exec->hadException())
        return jsUndefined();
    WebGLGetInfo info = context->getShaderParameter(shader, pname, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return toJS(exec, globalObject(), info);
}

JSValue JSWebGLRenderingContext::getSupportedExtensions(ExecState* exec)
{
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    if (context->isContextLost())
        return jsNull();
    Vector<String> value = context->getSupportedExtensions();
    MarkedArgumentBuffer list;
    for (size_t ii = 0; ii < value.size(); ++ii)
        list.append(jsStringWithCache(exec, value[ii]));
    return constructArray(exec, 0, globalObject(), list);
}

JSValue JSWebGLRenderingContext::getTexParameter(ExecState* exec)
{
    return getObjectParameter(this, exec, kTexture);
}

JSValue JSWebGLRenderingContext::getUniform(ExecState* exec)
{
    if (exec->argumentCount() != 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ExceptionCode ec = 0;
    WebGLRenderingContext* context = static_cast<WebGLRenderingContext*>(impl());
    if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLProgram::s_info))
        return throwTypeError(exec);
    WebGLProgram* program = toWebGLProgram(exec->argument(0));
    if (exec->argumentCount() > 1 && !exec->argument(1).isUndefinedOrNull() && !exec->argument(1).inherits(&JSWebGLUniformLocation::s_info))
        return throwTypeError(exec);
    WebGLUniformLocation* loc = toWebGLUniformLocation(exec->argument(1));
    if (exec->hadException())
        return jsUndefined();
    WebGLGetInfo info = context->getUniform(program, loc, ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return toJS(exec, globalObject(), info);
}

JSValue JSWebGLRenderingContext::getVertexAttrib(ExecState* exec)
{
    return getObjectParameter(this, exec, kVertexAttrib);
}

template<typename T, size_t inlineCapacity>
bool toVector(JSC::ExecState* exec, JSC::JSValue value, Vector<T, inlineCapacity>& vector)
{
    if (!value.isObject())
        return false;

    JSC::JSObject* object = asObject(value);
    int32_t length = object->get(exec, exec->vm().propertyNames->length).toInt32(exec);

    if (!vector.tryReserveCapacity(length))
        return false;
    vector.resize(length);

    for (int32_t i = 0; i < length; ++i) {
        JSC::JSValue v = object->get(exec, i);
        if (exec->hadException())
            return false;
        vector[i] = static_cast<T>(v.toNumber(exec));
    }

    return true;
}

enum DataFunctionToCall {
    f_uniform1v, f_uniform2v, f_uniform3v, f_uniform4v,
    f_vertexAttrib1v, f_vertexAttrib2v, f_vertexAttrib3v, f_vertexAttrib4v
};

enum DataFunctionMatrixToCall {
    f_uniformMatrix2fv, f_uniformMatrix3fv, f_uniformMatrix4fv
};

static bool functionForUniform(DataFunctionToCall f)
{
    switch (f) {
    case f_uniform1v:
    case f_uniform2v:
    case f_uniform3v:
    case f_uniform4v:
        return true;
        break;
    default: break;
    }
    return false;
}

static JSC::JSValue dataFunctionf(DataFunctionToCall f, JSC::ExecState* exec, WebGLRenderingContext* context)
{
    if (exec->argumentCount() != 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));
    
    WebGLUniformLocation* location = 0;
    long index = -1;
    
    if (functionForUniform(f)) {
        if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLUniformLocation::s_info))
            return throwTypeError(exec);
        location = toWebGLUniformLocation(exec->argument(0));
    } else
        index = exec->argument(0).toInt32(exec);

    if (exec->hadException())
        return jsUndefined();
        
    RefPtr<Float32Array> webGLArray = toFloat32Array(exec->argument(1));
    if (exec->hadException())    
        return jsUndefined();
        
    ExceptionCode ec = 0;
    if (webGLArray) {
        switch (f) {
        case f_uniform1v:
            context->uniform1fv(location, webGLArray.get(), ec);
            break;
        case f_uniform2v:
            context->uniform2fv(location, webGLArray.get(), ec);
            break;
        case f_uniform3v:
            context->uniform3fv(location, webGLArray.get(), ec);
            break;
        case f_uniform4v:
            context->uniform4fv(location, webGLArray.get(), ec);
            break;
        case f_vertexAttrib1v:
            context->vertexAttrib1fv(index, webGLArray.get());
            break;
        case f_vertexAttrib2v:
            context->vertexAttrib2fv(index, webGLArray.get());
            break;
        case f_vertexAttrib3v:
            context->vertexAttrib3fv(index, webGLArray.get());
            break;
        case f_vertexAttrib4v:
            context->vertexAttrib4fv(index, webGLArray.get());
            break;
        }
        
        setDOMException(exec, ec);
        return jsUndefined();
    }

    Vector<float, 64> array;
    if (!toVector(exec, exec->argument(1), array))
        return throwTypeError(exec);

    switch (f) {
    case f_uniform1v:
        context->uniform1fv(location, array.data(), array.size(), ec);
        break;
    case f_uniform2v:
        context->uniform2fv(location, array.data(), array.size(), ec);
        break;
    case f_uniform3v:
        context->uniform3fv(location, array.data(), array.size(), ec);
        break;
    case f_uniform4v:
        context->uniform4fv(location, array.data(), array.size(), ec);
        break;
    case f_vertexAttrib1v:
        context->vertexAttrib1fv(index, array.data(), array.size());
        break;
    case f_vertexAttrib2v:
        context->vertexAttrib2fv(index, array.data(), array.size());
        break;
    case f_vertexAttrib3v:
        context->vertexAttrib3fv(index, array.data(), array.size());
        break;
    case f_vertexAttrib4v:
        context->vertexAttrib4fv(index, array.data(), array.size());
        break;
    }
    
    setDOMException(exec, ec);
    return jsUndefined();
}

static JSC::JSValue dataFunctioni(DataFunctionToCall f, JSC::ExecState* exec, WebGLRenderingContext* context)
{
    if (exec->argumentCount() != 2)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLUniformLocation::s_info))
        return throwTypeError(exec);
    WebGLUniformLocation* location = toWebGLUniformLocation(exec->argument(0));
  
    if (exec->hadException())
        return jsUndefined();
        
    RefPtr<Int32Array> webGLArray = toInt32Array(exec->argument(1));
    if (exec->hadException())    
        return jsUndefined();
        
    ExceptionCode ec = 0;
    if (webGLArray) {
        switch (f) {
        case f_uniform1v:
            context->uniform1iv(location, webGLArray.get(), ec);
            break;
        case f_uniform2v:
            context->uniform2iv(location, webGLArray.get(), ec);
            break;
        case f_uniform3v:
            context->uniform3iv(location, webGLArray.get(), ec);
            break;
        case f_uniform4v:
            context->uniform4iv(location, webGLArray.get(), ec);
            break;
        default:
            break;
        }
        
        setDOMException(exec, ec);
        return jsUndefined();
    }


    Vector<int, 64> array;
    if (!toVector(exec, exec->argument(1), array))
        return throwTypeError(exec);

    switch (f) {
    case f_uniform1v:
        context->uniform1iv(location, array.data(), array.size(), ec);
        break;
    case f_uniform2v:
        context->uniform2iv(location, array.data(), array.size(), ec);
        break;
    case f_uniform3v:
        context->uniform3iv(location, array.data(), array.size(), ec);
        break;
    case f_uniform4v:
        context->uniform4iv(location, array.data(), array.size(), ec);
        break;
    default:
        break;
    }
    
    setDOMException(exec, ec);
    return jsUndefined();
}

static JSC::JSValue dataFunctionMatrix(DataFunctionMatrixToCall f, JSC::ExecState* exec, WebGLRenderingContext* context)
{
    if (exec->argumentCount() != 3)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    if (exec->argumentCount() > 0 && !exec->argument(0).isUndefinedOrNull() && !exec->argument(0).inherits(&JSWebGLUniformLocation::s_info))
        return throwTypeError(exec);
    WebGLUniformLocation* location = toWebGLUniformLocation(exec->argument(0));

    if (exec->hadException())    
        return jsUndefined();
        
    bool transpose = exec->argument(1).toBoolean(exec);
    if (exec->hadException())    
        return jsUndefined();
        
    RefPtr<Float32Array> webGLArray = toFloat32Array(exec->argument(2));
    if (exec->hadException())    
        return jsUndefined();
        
    ExceptionCode ec = 0;
    if (webGLArray) {
        switch (f) {
        case f_uniformMatrix2fv:
            context->uniformMatrix2fv(location, transpose, webGLArray.get(), ec);
            break;
        case f_uniformMatrix3fv:
            context->uniformMatrix3fv(location, transpose, webGLArray.get(), ec);
            break;
        case f_uniformMatrix4fv:
            context->uniformMatrix4fv(location, transpose, webGLArray.get(), ec);
            break;
        }
        
        setDOMException(exec, ec);
        return jsUndefined();
    }

    Vector<float, 64> array;
    if (!toVector(exec, exec->argument(2), array))
        return throwTypeError(exec);

    switch (f) {
    case f_uniformMatrix2fv:
        context->uniformMatrix2fv(location, transpose, array.data(), array.size(), ec);
        break;
    case f_uniformMatrix3fv:
        context->uniformMatrix3fv(location, transpose, array.data(), array.size(), ec);
        break;
    case f_uniformMatrix4fv:
        context->uniformMatrix4fv(location, transpose, array.data(), array.size(), ec);
        break;
    }

    setDOMException(exec, ec);
    return jsUndefined();
}

JSC::JSValue JSWebGLRenderingContext::uniform1fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_uniform1v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform1iv(JSC::ExecState* exec)
{
    return dataFunctioni(f_uniform1v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform2fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_uniform2v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform2iv(JSC::ExecState* exec)
{
    return dataFunctioni(f_uniform2v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform3fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_uniform3v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform3iv(JSC::ExecState* exec)
{
    return dataFunctioni(f_uniform3v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform4fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_uniform4v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniform4iv(JSC::ExecState* exec)
{
    return dataFunctioni(f_uniform4v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniformMatrix2fv(JSC::ExecState* exec)
{
    return dataFunctionMatrix(f_uniformMatrix2fv, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniformMatrix3fv(JSC::ExecState* exec)
{
    return dataFunctionMatrix(f_uniformMatrix3fv, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::uniformMatrix4fv(JSC::ExecState* exec)
{
    return dataFunctionMatrix(f_uniformMatrix4fv, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::vertexAttrib1fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_vertexAttrib1v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::vertexAttrib2fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_vertexAttrib2v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::vertexAttrib3fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_vertexAttrib3v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

JSC::JSValue JSWebGLRenderingContext::vertexAttrib4fv(JSC::ExecState* exec)
{
    return dataFunctionf(f_vertexAttrib4v, exec, static_cast<WebGLRenderingContext*>(impl()));
}

} // namespace WebCore

#endif // ENABLE(WEBGL)

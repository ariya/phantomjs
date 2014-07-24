/*
 * Copyright (C) 2009, 2013 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "SerializedScriptValue.h"

#include "Blob.h"
#include "ExceptionCode.h"
#include "File.h"
#include "FileList.h"
#include "ImageData.h"
#include "JSArrayBuffer.h"
#include "JSArrayBufferView.h"
#include "JSBlob.h"
#include "JSDataView.h"
#include "JSDOMGlobalObject.h"
#include "JSFile.h"
#include "JSFileList.h"
#include "JSFloat32Array.h"
#include "JSFloat64Array.h"
#include "JSImageData.h"
#include "JSInt16Array.h"
#include "JSInt32Array.h"
#include "JSInt8Array.h"
#include "JSMessagePort.h"
#include "JSNavigator.h"
#include "JSUint16Array.h"
#include "JSUint32Array.h"
#include "JSUint8Array.h"
#include "JSUint8ClampedArray.h"
#include "NotImplemented.h"
#include "ScriptValue.h"
#include "SharedBuffer.h"
#include "WebCoreJSClientData.h"
#include <limits>
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/APIShims.h>
#include <runtime/BooleanObject.h>
#include <runtime/DateInstance.h>
#include <runtime/Error.h>
#include <runtime/ExceptionHelpers.h>
#include <runtime/ObjectConstructor.h>
#include <runtime/Operations.h>
#include <runtime/PropertyNameArray.h>
#include <runtime/RegExp.h>
#include <runtime/RegExpObject.h>
#include <wtf/ArrayBuffer.h>
#include <wtf/HashTraits.h>
#include <wtf/Uint8ClampedArray.h>
#include <wtf/Vector.h>

using namespace JSC;
using namespace std;

#if CPU(BIG_ENDIAN) || CPU(MIDDLE_ENDIAN) || CPU(NEEDS_ALIGNED_ACCESS)
#define ASSUME_LITTLE_ENDIAN 0
#else
#define ASSUME_LITTLE_ENDIAN 1
#endif

namespace WebCore {

static const unsigned maximumFilterRecursion = 40000;

enum WalkerState { StateUnknown, ArrayStartState, ArrayStartVisitMember, ArrayEndVisitMember,
    ObjectStartState, ObjectStartVisitMember, ObjectEndVisitMember };

// These can't be reordered, and any new types must be added to the end of the list
enum SerializationTag {
    ArrayTag = 1,
    ObjectTag = 2,
    UndefinedTag = 3,
    NullTag = 4,
    IntTag = 5,
    ZeroTag = 6,
    OneTag = 7,
    FalseTag = 8,
    TrueTag = 9,
    DoubleTag = 10,
    DateTag = 11,
    FileTag = 12,
    FileListTag = 13,
    ImageDataTag = 14,
    BlobTag = 15,
    StringTag = 16,
    EmptyStringTag = 17,
    RegExpTag = 18,
    ObjectReferenceTag = 19,
    MessagePortReferenceTag = 20,
    ArrayBufferTag = 21,
    ArrayBufferViewTag = 22,
    ArrayBufferTransferTag = 23,
    TrueObjectTag = 24,
    FalseObjectTag = 25,
    StringObjectTag = 26,
    EmptyStringObjectTag = 27,
    NumberObjectTag = 28,
    ErrorTag = 255
};

enum ArrayBufferViewSubtag {
    DataViewTag = 0,
    Int8ArrayTag = 1,
    Uint8ArrayTag = 2,
    Uint8ClampedArrayTag = 3,
    Int16ArrayTag = 4,
    Uint16ArrayTag = 5,
    Int32ArrayTag = 6,
    Uint32ArrayTag = 7,
    Float32ArrayTag = 8,
    Float64ArrayTag = 9
};

static unsigned typedArrayElementSize(ArrayBufferViewSubtag tag)
{
    switch (tag) {
    case DataViewTag:
    case Int8ArrayTag:
    case Uint8ArrayTag:
    case Uint8ClampedArrayTag:
        return 1;
    case Int16ArrayTag:
    case Uint16ArrayTag:
        return 2;
    case Int32ArrayTag:
    case Uint32ArrayTag:
    case Float32ArrayTag:
        return 4;
    case Float64ArrayTag:
        return 8;
    default:
        return 0;
    }

}

/* CurrentVersion tracks the serialization version so that persistant stores
 * are able to correctly bail out in the case of encountering newer formats.
 *
 * Initial version was 1.
 * Version 2. added the ObjectReferenceTag and support for serialization of cyclic graphs.
 * Version 3. added the FalseObjectTag, TrueObjectTag, NumberObjectTag, StringObjectTag
 * and EmptyStringObjectTag for serialization of Boolean, Number and String objects.
 * Version 4. added support for serializing non-index properties of arrays.
 */
static const unsigned CurrentVersion = 4;
static const unsigned TerminatorTag = 0xFFFFFFFF;
static const unsigned StringPoolTag = 0xFFFFFFFE;
static const unsigned NonIndexPropertiesTag = 0xFFFFFFFD;

/*
 * Object serialization is performed according to the following grammar, all tags
 * are recorded as a single uint8_t.
 *
 * IndexType (used for the object pool and StringData's constant pool) is the
 * minimum sized unsigned integer type required to represent the maximum index
 * in the constant pool.
 *
 * SerializedValue :- <CurrentVersion:uint32_t> Value
 * Value :- Array | Object | Terminal
 *
 * Array :-
 *     ArrayTag <length:uint32_t>(<index:uint32_t><value:Value>)* TerminatorTag
 *
 * Object :-
 *     ObjectTag (<name:StringData><value:Value>)* TerminatorTag
 *
 * Terminal :-
 *      UndefinedTag
 *    | NullTag
 *    | IntTag <value:int32_t>
 *    | ZeroTag
 *    | OneTag
 *    | FalseTag
 *    | TrueTag
 *    | FalseObjectTag
 *    | TrueObjectTag
 *    | DoubleTag <value:double>
 *    | NumberObjectTag <value:double>
 *    | DateTag <value:double>
 *    | String
 *    | EmptyStringTag
 *    | EmptyStringObjectTag
 *    | File
 *    | FileList
 *    | ImageData
 *    | Blob
 *    | ObjectReference
 *    | MessagePortReferenceTag <value:uint32_t>
 *    | ArrayBuffer
 *    | ArrayBufferViewTag ArrayBufferViewSubtag <byteOffset:uint32_t> <byteLenght:uint32_t> (ArrayBuffer | ObjectReference)
 *    | ArrayBufferTransferTag <value:uint32_t>
 *
 * String :-
 *      EmptyStringTag
 *      StringTag StringData
 *
 * StringObject:
 *      EmptyStringObjectTag
 *      StringObjectTag StringData
 *
 * StringData :-
 *      StringPoolTag <cpIndex:IndexType>
 *      (not (TerminatorTag | StringPoolTag))<length:uint32_t><characters:UChar{length}> // Added to constant pool when seen, string length 0xFFFFFFFF is disallowed
 *
 * File :-
 *    FileTag FileData
 *
 * FileData :-
 *    <path:StringData> <url:StringData> <type:StringData>
 *
 * FileList :-
 *    FileListTag <length:uint32_t>(<file:FileData>){length}
 *
 * ImageData :-
 *    ImageDataTag <width:int32_t><height:int32_t><length:uint32_t><data:uint8_t{length}>
 *
 * Blob :-
 *    BlobTag <url:StringData><type:StringData><size:long long>
 *
 * RegExp :-
 *    RegExpTag <pattern:StringData><flags:StringData>
 *
 * ObjectReference :-
 *    ObjectReferenceTag <opIndex:IndexType>
 *
 * ArrayBuffer :-
 *    ArrayBufferTag <length:uint32_t> <contents:byte{length}>
 */

typedef pair<JSC::JSValue, SerializationReturnCode> DeserializationResult;

class CloneBase {
protected:
    CloneBase(ExecState* exec)
        : m_exec(exec)
        , m_failed(false)
    {
    }

    bool shouldTerminate()
    {
        return m_exec->hadException();
    }

    void throwStackOverflow()
    {
        throwError(m_exec, createStackOverflowError(m_exec));
    }

    NO_RETURN_DUE_TO_ASSERT
    void fail()
    {
        ASSERT_NOT_REACHED();
        m_failed = true;
    }

    ExecState* m_exec;
    bool m_failed;
    MarkedArgumentBuffer m_gcBuffer;
};

#if ASSUME_LITTLE_ENDIAN
template <typename T> static void writeLittleEndian(Vector<uint8_t>& buffer, T value)
{
    buffer.append(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}
#else
template <typename T> static void writeLittleEndian(Vector<uint8_t>& buffer, T value)
{
    for (unsigned i = 0; i < sizeof(T); i++) {
        buffer.append(value & 0xFF);
        value >>= 8;
    }
}
#endif

template <> void writeLittleEndian<uint8_t>(Vector<uint8_t>& buffer, uint8_t value)
{
    buffer.append(value);
}

template <typename T> static bool writeLittleEndian(Vector<uint8_t>& buffer, const T* values, uint32_t length)
{
    if (length > numeric_limits<uint32_t>::max() / sizeof(T))
        return false;

#if ASSUME_LITTLE_ENDIAN
    buffer.append(reinterpret_cast<const uint8_t*>(values), length * sizeof(T));
#else
    for (unsigned i = 0; i < length; i++) {
        T value = values[i];
        for (unsigned j = 0; j < sizeof(T); j++) {
            buffer.append(static_cast<uint8_t>(value & 0xFF));
            value >>= 8;
        }
    }
#endif
    return true;
}

class CloneSerializer : CloneBase {
public:
    static SerializationReturnCode serialize(ExecState* exec, JSValue value,
                                             MessagePortArray* messagePorts, ArrayBufferArray* arrayBuffers,
                                             Vector<String>& blobURLs, Vector<uint8_t>& out)
    {
        CloneSerializer serializer(exec, messagePorts, arrayBuffers, blobURLs, out);
        return serializer.serialize(value);
    }

    static bool serialize(const String& s, Vector<uint8_t>& out)
    {
        writeLittleEndian(out, CurrentVersion);
        if (s.isEmpty()) {
            writeLittleEndian<uint8_t>(out, EmptyStringTag);
            return true;
        }
        writeLittleEndian<uint8_t>(out, StringTag);
        writeLittleEndian(out, s.length());
        return writeLittleEndian(out, s.impl()->characters(), s.length());
    }

    static void serializeUndefined(Vector<uint8_t>& out)
    {
        writeLittleEndian(out, CurrentVersion);
        writeLittleEndian<uint8_t>(out, UndefinedTag);
    }

    static void serializeBoolean(bool value, Vector<uint8_t>& out)
    {
        writeLittleEndian(out, CurrentVersion);
        writeLittleEndian<uint8_t>(out, value ? TrueTag : FalseTag);
    }

    static void serializeNumber(double value, Vector<uint8_t>& out)
    {
        writeLittleEndian(out, CurrentVersion);
        writeLittleEndian<uint8_t>(out, DoubleTag);
        union {
            double d;
            int64_t i;
        } u;
        u.d = value;
        writeLittleEndian(out, u.i);
    }

private:
    typedef HashMap<JSObject*, uint32_t> ObjectPool;

    CloneSerializer(ExecState* exec, MessagePortArray* messagePorts, ArrayBufferArray* arrayBuffers, Vector<String>& blobURLs, Vector<uint8_t>& out)
        : CloneBase(exec)
        , m_buffer(out)
        , m_blobURLs(blobURLs)
        , m_emptyIdentifier(exec, emptyString())
    {
        write(CurrentVersion);
        fillTransferMap(messagePorts, m_transferredMessagePorts);
        fillTransferMap(arrayBuffers, m_transferredArrayBuffers);
    }

    template <class T>
    void fillTransferMap(Vector<RefPtr<T>, 1>* input, ObjectPool& result)
    {
        if (!input)
            return;
        JSDOMGlobalObject* globalObject = jsCast<JSDOMGlobalObject*>(m_exec->lexicalGlobalObject());
        for (size_t i = 0; i < input->size(); i++) {
            JSC::JSValue value = toJS(m_exec, globalObject, input->at(i).get());
            JSC::JSObject* obj = value.getObject();
            if (obj && !result.contains(obj))
                result.add(obj, i);
        }
    }

    SerializationReturnCode serialize(JSValue in);

    bool isArray(JSValue value)
    {
        if (!value.isObject())
            return false;
        JSObject* object = asObject(value);
        return isJSArray(object) || object->inherits(&JSArray::s_info);
    }

    bool checkForDuplicate(JSObject* object)
    {
        // Record object for graph reconstruction
        ObjectPool::const_iterator found = m_objectPool.find(object);

        // Handle duplicate references
        if (found != m_objectPool.end()) {
            write(ObjectReferenceTag);
            ASSERT(static_cast<int32_t>(found->value) < m_objectPool.size());
            writeObjectIndex(found->value);
            return true;
        }

        return false;
    }

    void recordObject(JSObject* object)
    {
        m_objectPool.add(object, m_objectPool.size());
        m_gcBuffer.append(object);
    }

    bool startObjectInternal(JSObject* object)
    {
        if (checkForDuplicate(object))
            return false;
        recordObject(object);
        return true;
    }

    bool startObject(JSObject* object)
    {
        if (!startObjectInternal(object))
            return false;
        write(ObjectTag);
        return true;
    }

    bool startArray(JSArray* array)
    {
        if (!startObjectInternal(array))
            return false;

        unsigned length = array->length();
        write(ArrayTag);
        write(length);
        return true;
    }

    void endObject()
    {
        write(TerminatorTag);
    }

    JSValue getProperty(JSObject* object, const Identifier& propertyName)
    {
        PropertySlot slot(object);
        if (object->methodTable()->getOwnPropertySlot(object, m_exec, propertyName, slot))
            return slot.getValue(m_exec, propertyName);
        return JSValue();
    }

    void dumpImmediate(JSValue value)
    {
        if (value.isNull())
            write(NullTag);
        else if (value.isUndefined())
            write(UndefinedTag);
        else if (value.isNumber()) {
            if (value.isInt32()) {
                if (!value.asInt32())
                    write(ZeroTag);
                else if (value.asInt32() == 1)
                    write(OneTag);
                else {
                    write(IntTag);
                    write(static_cast<uint32_t>(value.asInt32()));
                }
            } else {
                write(DoubleTag);
                write(value.asDouble());
            }
        } else if (value.isBoolean()) {
            if (value.isTrue())
                write(TrueTag);
            else
                write(FalseTag);
        }
    }

    void dumpString(String str)
    {
        if (str.isEmpty())
            write(EmptyStringTag);
        else {
            write(StringTag);
            write(str);
        }
    }

    void dumpStringObject(String str)
    {
        if (str.isEmpty())
            write(EmptyStringObjectTag);
        else {
            write(StringObjectTag);
            write(str);
        }
    }

    bool dumpArrayBufferView(JSObject* obj, SerializationReturnCode& code)
    {
        write(ArrayBufferViewTag);
        if (obj->inherits(&JSDataView::s_info))
            write(DataViewTag);
        else if (obj->inherits(&JSUint8ClampedArray::s_info))
            write(Uint8ClampedArrayTag);
        else if (obj->inherits(&JSInt8Array::s_info))
            write(Int8ArrayTag);
        else if (obj->inherits(&JSUint8Array::s_info))
            write(Uint8ArrayTag);
        else if (obj->inherits(&JSInt16Array::s_info))
            write(Int16ArrayTag);
        else if (obj->inherits(&JSUint16Array::s_info))
            write(Uint16ArrayTag);
        else if (obj->inherits(&JSInt32Array::s_info))
            write(Int32ArrayTag);
        else if (obj->inherits(&JSUint32Array::s_info))
            write(Uint32ArrayTag);
        else if (obj->inherits(&JSFloat32Array::s_info))
            write(Float32ArrayTag);
        else if (obj->inherits(&JSFloat64Array::s_info))
            write(Float64ArrayTag);
        else
            return false;

        RefPtr<ArrayBufferView> arrayBufferView = toArrayBufferView(obj);
        write(static_cast<uint32_t>(arrayBufferView->byteOffset()));
        write(static_cast<uint32_t>(arrayBufferView->byteLength()));
        RefPtr<ArrayBuffer> arrayBuffer = arrayBufferView->buffer();
        if (!arrayBuffer) {
            code = ValidationError;
            return true;
        }
        JSValue bufferObj = toJS(m_exec, jsCast<JSDOMGlobalObject*>(m_exec->lexicalGlobalObject()), arrayBuffer.get());
        return dumpIfTerminal(bufferObj, code);
    }

    bool dumpIfTerminal(JSValue value, SerializationReturnCode& code)
    {
        if (!value.isCell()) {
            dumpImmediate(value);
            return true;
        }

        if (value.isString()) {
            String str = asString(value)->value(m_exec);
            dumpString(str);
            return true;
        }

        if (value.isNumber()) {
            write(DoubleTag);
            write(value.asNumber());
            return true;
        }

        if (value.isObject() && asObject(value)->inherits(&DateInstance::s_info)) {
            write(DateTag);
            write(asDateInstance(value)->internalNumber());
            return true;
        }

        if (isArray(value))
            return false;

        if (value.isObject()) {
            JSObject* obj = asObject(value);
            if (obj->inherits(&BooleanObject::s_info)) {
                if (!startObjectInternal(obj)) // handle duplicates
                    return true;
                write(asBooleanObject(value)->internalValue().toBoolean(m_exec) ? TrueObjectTag : FalseObjectTag);
                return true;
            }
            if (obj->inherits(&StringObject::s_info)) {
                if (!startObjectInternal(obj)) // handle duplicates
                    return true;
                String str = asString(asStringObject(value)->internalValue())->value(m_exec);
                dumpStringObject(str);
                return true;
            }
            if (obj->inherits(&NumberObject::s_info)) {
                if (!startObjectInternal(obj)) // handle duplicates
                    return true;
                write(NumberObjectTag);
                NumberObject* obj = static_cast<NumberObject*>(asObject(value));
                write(obj->internalValue().asNumber());
                return true;
            }
            if (obj->inherits(&JSFile::s_info)) {
                write(FileTag);
                write(toFile(obj));
                return true;
            }
            if (obj->inherits(&JSFileList::s_info)) {
                FileList* list = toFileList(obj);
                write(FileListTag);
                unsigned length = list->length();
                write(length);
                for (unsigned i = 0; i < length; i++)
                    write(list->item(i));
                return true;
            }
            if (obj->inherits(&JSBlob::s_info)) {
                write(BlobTag);
                Blob* blob = toBlob(obj);
                m_blobURLs.append(blob->url());
                write(blob->url());
                write(blob->type());
                write(blob->size());
                return true;
            }
            if (obj->inherits(&JSImageData::s_info)) {
                ImageData* data = toImageData(obj);
                write(ImageDataTag);
                write(data->width());
                write(data->height());
                write(data->data()->length());
                write(data->data()->data(), data->data()->length());
                return true;
            }
            if (obj->inherits(&RegExpObject::s_info)) {
                RegExpObject* regExp = asRegExpObject(obj);
                char flags[3];
                int flagCount = 0;
                if (regExp->regExp()->global())
                    flags[flagCount++] = 'g';
                if (regExp->regExp()->ignoreCase())
                    flags[flagCount++] = 'i';
                if (regExp->regExp()->multiline())
                    flags[flagCount++] = 'm';
                write(RegExpTag);
                write(regExp->regExp()->pattern());
                write(String(flags, flagCount));
                return true;
            }
            if (obj->inherits(&JSMessagePort::s_info)) {
                ObjectPool::iterator index = m_transferredMessagePorts.find(obj);
                if (index != m_transferredMessagePorts.end()) {
                    write(MessagePortReferenceTag);
                    write(index->value);
                    return true;
                }
                // MessagePort object could not be found in transferred message ports
                code = ValidationError;
                return true;
            }
            if (obj->inherits(&JSArrayBuffer::s_info)) {
                RefPtr<ArrayBuffer> arrayBuffer = toArrayBuffer(obj);
                if (arrayBuffer->isNeutered()) {
                    code = ValidationError;
                    return true;
                }
                ObjectPool::iterator index = m_transferredArrayBuffers.find(obj);
                if (index != m_transferredArrayBuffers.end()) {
                    write(ArrayBufferTransferTag);
                    write(index->value);
                    return true;
                }
                if (!startObjectInternal(obj)) // handle duplicates
                    return true;
                write(ArrayBufferTag);
                write(arrayBuffer->byteLength());
                write(static_cast<const uint8_t *>(arrayBuffer->data()), arrayBuffer->byteLength());
                return true;
            }
            if (obj->inherits(&JSArrayBufferView::s_info)) {
                if (checkForDuplicate(obj))
                    return true;
                bool success = dumpArrayBufferView(obj, code);
                recordObject(obj);
                return success;
            }

            return false;
        }
        // Any other types are expected to serialize as null.
        write(NullTag);
        return true;
    }

    void write(SerializationTag tag)
    {
        writeLittleEndian<uint8_t>(m_buffer, static_cast<uint8_t>(tag));
    }

    void write(ArrayBufferViewSubtag tag)
    {
        writeLittleEndian<uint8_t>(m_buffer, static_cast<uint8_t>(tag));
    }

    void write(uint8_t c)
    {
        writeLittleEndian(m_buffer, c);
    }

    void write(uint32_t i)
    {
        writeLittleEndian(m_buffer, i);
    }

    void write(double d)
    {
        union {
            double d;
            int64_t i;
        } u;
        u.d = d;
        writeLittleEndian(m_buffer, u.i);
    }

    void write(int32_t i)
    {
        writeLittleEndian(m_buffer, i);
    }

    void write(unsigned long long i)
    {
        writeLittleEndian(m_buffer, i);
    }
    
    void write(uint16_t ch)
    {
        writeLittleEndian(m_buffer, ch);
    }

    void writeStringIndex(unsigned i)
    {
        writeConstantPoolIndex(m_constantPool, i);
    }
    
    void writeObjectIndex(unsigned i)
    {
        writeConstantPoolIndex(m_objectPool, i);
    }

    template <class T> void writeConstantPoolIndex(const T& constantPool, unsigned i)
    {
        ASSERT(static_cast<int32_t>(i) < constantPool.size());
        if (constantPool.size() <= 0xFF)
            write(static_cast<uint8_t>(i));
        else if (constantPool.size() <= 0xFFFF)
            write(static_cast<uint16_t>(i));
        else
            write(static_cast<uint32_t>(i));
    }

    void write(const Identifier& ident)
    {
        const String& str = ident.string();
        StringConstantPool::AddResult addResult = m_constantPool.add(str.impl(), m_constantPool.size());
        if (!addResult.isNewEntry) {
            write(StringPoolTag);
            writeStringIndex(addResult.iterator->value);
            return;
        }

        // This condition is unlikely to happen as they would imply an ~8gb
        // string but we should guard against it anyway
        if (str.length() >= StringPoolTag) {
            fail();
            return;
        }

        // Guard against overflow
        if (str.length() > (numeric_limits<uint32_t>::max() - sizeof(uint32_t)) / sizeof(UChar)) {
            fail();
            return;
        }

        writeLittleEndian<uint32_t>(m_buffer, str.length());
        if (!writeLittleEndian<uint16_t>(m_buffer, reinterpret_cast<const uint16_t*>(str.characters()), str.length()))
            fail();
    }

    void write(const String& str)
    {
        if (str.isNull())
            write(m_emptyIdentifier);
        else
            write(Identifier(m_exec, str));
    }

    void write(const File* file)
    {
        m_blobURLs.append(file->url());
        write(file->path());
        write(file->url());
        write(file->type());
    }

    void write(const uint8_t* data, unsigned length)
    {
        m_buffer.append(data, length);
    }

    Vector<uint8_t>& m_buffer;
    Vector<String>& m_blobURLs;
    ObjectPool m_objectPool;
    ObjectPool m_transferredMessagePorts;
    ObjectPool m_transferredArrayBuffers;
    typedef HashMap<RefPtr<StringImpl>, uint32_t, IdentifierRepHash> StringConstantPool;
    StringConstantPool m_constantPool;
    Identifier m_emptyIdentifier;
};

SerializationReturnCode CloneSerializer::serialize(JSValue in)
{
    Vector<uint32_t, 16> indexStack;
    Vector<uint32_t, 16> lengthStack;
    Vector<PropertyNameArray, 16> propertyStack;
    Vector<JSObject*, 32> inputObjectStack;
    Vector<WalkerState, 16> stateStack;
    WalkerState state = StateUnknown;
    JSValue inValue = in;
    while (1) {
        switch (state) {
            arrayStartState:
            case ArrayStartState: {
                ASSERT(isArray(inValue));
                if (inputObjectStack.size() > maximumFilterRecursion)
                    return StackOverflowError;

                JSArray* inArray = asArray(inValue);
                unsigned length = inArray->length();
                if (!startArray(inArray))
                    break;
                inputObjectStack.append(inArray);
                indexStack.append(0);
                lengthStack.append(length);
                // fallthrough
            }
            arrayStartVisitMember:
            case ArrayStartVisitMember: {
                JSObject* array = inputObjectStack.last();
                uint32_t index = indexStack.last();
                if (index == lengthStack.last()) {
                    indexStack.removeLast();
                    lengthStack.removeLast();

                    propertyStack.append(PropertyNameArray(m_exec));
                    array->methodTable()->getOwnNonIndexPropertyNames(array, m_exec, propertyStack.last(), ExcludeDontEnumProperties);
                    if (propertyStack.last().size()) {
                        write(NonIndexPropertiesTag);
                        indexStack.append(0);
                        goto objectStartVisitMember;
                    }
                    propertyStack.removeLast();

                    endObject();
                    inputObjectStack.removeLast();
                    break;
                }
                inValue = array->getDirectIndex(m_exec, index);
                if (!inValue) {
                    indexStack.last()++;
                    goto arrayStartVisitMember;
                }

                write(index);
                SerializationReturnCode terminalCode = SuccessfullyCompleted;
                if (dumpIfTerminal(inValue, terminalCode)) {
                    if (terminalCode != SuccessfullyCompleted)
                        return terminalCode;
                    indexStack.last()++;
                    goto arrayStartVisitMember;
                }
                stateStack.append(ArrayEndVisitMember);
                goto stateUnknown;
            }
            case ArrayEndVisitMember: {
                indexStack.last()++;
                goto arrayStartVisitMember;
            }
            objectStartState:
            case ObjectStartState: {
                ASSERT(inValue.isObject());
                if (inputObjectStack.size() > maximumFilterRecursion)
                    return StackOverflowError;
                JSObject* inObject = asObject(inValue);
                if (!startObject(inObject))
                    break;
                // At this point, all supported objects other than Object
                // objects have been handled. If we reach this point and
                // the input is not an Object object then we should throw
                // a DataCloneError.
                if (inObject->classInfo() != &JSFinalObject::s_info)
                    return DataCloneError;
                inputObjectStack.append(inObject);
                indexStack.append(0);
                propertyStack.append(PropertyNameArray(m_exec));
                inObject->methodTable()->getOwnPropertyNames(inObject, m_exec, propertyStack.last(), ExcludeDontEnumProperties);
                // fallthrough
            }
            objectStartVisitMember:
            case ObjectStartVisitMember: {
                JSObject* object = inputObjectStack.last();
                uint32_t index = indexStack.last();
                PropertyNameArray& properties = propertyStack.last();
                if (index == properties.size()) {
                    endObject();
                    inputObjectStack.removeLast();
                    indexStack.removeLast();
                    propertyStack.removeLast();
                    break;
                }
                inValue = getProperty(object, properties[index]);
                if (shouldTerminate())
                    return ExistingExceptionError;

                if (!inValue) {
                    // Property was removed during serialisation
                    indexStack.last()++;
                    goto objectStartVisitMember;
                }
                write(properties[index]);

                if (shouldTerminate())
                    return ExistingExceptionError;

                SerializationReturnCode terminalCode = SuccessfullyCompleted;
                if (!dumpIfTerminal(inValue, terminalCode)) {
                    stateStack.append(ObjectEndVisitMember);
                    goto stateUnknown;
                }
                if (terminalCode != SuccessfullyCompleted)
                    return terminalCode;
                // fallthrough
            }
            case ObjectEndVisitMember: {
                if (shouldTerminate())
                    return ExistingExceptionError;

                indexStack.last()++;
                goto objectStartVisitMember;
            }
            stateUnknown:
            case StateUnknown: {
                SerializationReturnCode terminalCode = SuccessfullyCompleted;
                if (dumpIfTerminal(inValue, terminalCode)) {
                    if (terminalCode != SuccessfullyCompleted)
                        return terminalCode;
                    break;
                }

                if (isArray(inValue))
                    goto arrayStartState;
                goto objectStartState;
            }
        }
        if (stateStack.isEmpty())
            break;

        state = stateStack.last();
        stateStack.removeLast();
    }
    if (m_failed)
        return UnspecifiedError;

    return SuccessfullyCompleted;
}

typedef Vector<WTF::ArrayBufferContents> ArrayBufferContentsArray;

class CloneDeserializer : CloneBase {
public:
    static String deserializeString(const Vector<uint8_t>& buffer)
    {
        const uint8_t* ptr = buffer.begin();
        const uint8_t* end = buffer.end();
        uint32_t version;
        if (!readLittleEndian(ptr, end, version) || version > CurrentVersion)
            return String();
        uint8_t tag;
        if (!readLittleEndian(ptr, end, tag) || tag != StringTag)
            return String();
        uint32_t length;
        if (!readLittleEndian(ptr, end, length) || length >= StringPoolTag)
            return String();
        String str;
        if (!readString(ptr, end, str, length))
            return String();
        return String(str.impl());
    }

    static DeserializationResult deserialize(ExecState* exec, JSGlobalObject* globalObject,
                                             MessagePortArray* messagePorts, ArrayBufferContentsArray* arrayBufferContentsArray,
                                             const Vector<uint8_t>& buffer)
    {
        if (!buffer.size())
            return make_pair(jsNull(), UnspecifiedError);
        CloneDeserializer deserializer(exec, globalObject, messagePorts, arrayBufferContentsArray, buffer);
        if (!deserializer.isValid())
            return make_pair(JSValue(), ValidationError);
        return deserializer.deserialize();
    }

private:
    struct CachedString {
        CachedString(const String& string)
            : m_string(string)
        {
        }

        JSValue jsString(ExecState* exec)
        {
            if (!m_jsString)
                m_jsString = JSC::jsString(exec, m_string);
            return m_jsString;
        }
        const String& string() { return m_string; }

    private:
        String m_string;
        JSValue m_jsString;
    };

    struct CachedStringRef {
        CachedStringRef()
            : m_base(0)
            , m_index(0)
        {
        }
        CachedStringRef(Vector<CachedString>* base, size_t index)
            : m_base(base)
            , m_index(index)
        {
        }
        
        CachedString* operator->() { ASSERT(m_base); return &m_base->at(m_index); }
        
    private:
        Vector<CachedString>* m_base;
        size_t m_index;
    };

    CloneDeserializer(ExecState* exec, JSGlobalObject* globalObject, 
                      MessagePortArray* messagePorts, ArrayBufferContentsArray* arrayBufferContents,
                      const Vector<uint8_t>& buffer)
        : CloneBase(exec)
        , m_globalObject(globalObject)
        , m_isDOMGlobalObject(globalObject->inherits(&JSDOMGlobalObject::s_info))
        , m_ptr(buffer.data())
        , m_end(buffer.data() + buffer.size())
        , m_version(0xFFFFFFFF)
        , m_messagePorts(messagePorts)
        , m_arrayBufferContents(arrayBufferContents)
        , m_arrayBuffers(arrayBufferContents ? arrayBufferContents->size() : 0)
    {
        if (!read(m_version))
            m_version = 0xFFFFFFFF;
    }

    DeserializationResult deserialize();

    void throwValidationError()
    {
        throwError(m_exec, createTypeError(m_exec, "Unable to deserialize data."));
    }

    bool isValid() const { return m_version <= CurrentVersion; }

    template <typename T> bool readLittleEndian(T& value)
    {
        if (m_failed || !readLittleEndian(m_ptr, m_end, value)) {
            fail();
            return false;
        }
        return true;
    }
#if ASSUME_LITTLE_ENDIAN
    template <typename T> static bool readLittleEndian(const uint8_t*& ptr, const uint8_t* end, T& value)
    {
        if (ptr > end - sizeof(value))
            return false;

        if (sizeof(T) == 1)
            value = *ptr++;
        else {
            value = *reinterpret_cast<const T*>(ptr);
            ptr += sizeof(T);
        }
        return true;
    }
#else
    template <typename T> static bool readLittleEndian(const uint8_t*& ptr, const uint8_t* end, T& value)
    {
        if (ptr > end - sizeof(value))
            return false;

        if (sizeof(T) == 1)
            value = *ptr++;
        else {
            value = 0;
            for (unsigned i = 0; i < sizeof(T); i++)
                value += ((T)*ptr++) << (i * 8);
        }
        return true;
    }
#endif

    bool read(uint32_t& i)
    {
        return readLittleEndian(i);
    }

    bool read(int32_t& i)
    {
        return readLittleEndian(*reinterpret_cast<uint32_t*>(&i));
    }

    bool read(uint16_t& i)
    {
        return readLittleEndian(i);
    }

    bool read(uint8_t& i)
    {
        return readLittleEndian(i);
    }

    bool read(double& d)
    {
        union {
            double d;
            uint64_t i64;
        } u;
        if (!readLittleEndian(u.i64))
            return false;
        d = u.d;
        return true;
    }

    bool read(unsigned long long& i)
    {
        return readLittleEndian(i);
    }

    bool readStringIndex(uint32_t& i)
    {
        return readConstantPoolIndex(m_constantPool, i);
    }

    template <class T> bool readConstantPoolIndex(const T& constantPool, uint32_t& i)
    {
        if (constantPool.size() <= 0xFF) {
            uint8_t i8;
            if (!read(i8))
                return false;
            i = i8;
            return true;
        }
        if (constantPool.size() <= 0xFFFF) {
            uint16_t i16;
            if (!read(i16))
                return false;
            i = i16;
            return true;
        }
        return read(i);
    }

    static bool readString(const uint8_t*& ptr, const uint8_t* end, String& str, unsigned length)
    {
        if (length >= numeric_limits<int32_t>::max() / sizeof(UChar))
            return false;

        unsigned size = length * sizeof(UChar);
        if ((end - ptr) < static_cast<int>(size))
            return false;

#if ASSUME_LITTLE_ENDIAN
        str = String(reinterpret_cast<const UChar*>(ptr), length);
        ptr += length * sizeof(UChar);
#else
        Vector<UChar> buffer;
        buffer.reserveCapacity(length);
        for (unsigned i = 0; i < length; i++) {
            uint16_t ch;
            readLittleEndian(ptr, end, ch);
            buffer.append(ch);
        }
        str = String::adopt(buffer);
#endif
        return true;
    }

    bool readStringData(CachedStringRef& cachedString)
    {
        bool scratch;
        return readStringData(cachedString, scratch);
    }

    bool readStringData(CachedStringRef& cachedString, bool& wasTerminator)
    {
        if (m_failed)
            return false;
        uint32_t length = 0;
        if (!read(length))
            return false;
        if (length == TerminatorTag) {
            wasTerminator = true;
            return false;
        }
        if (length == StringPoolTag) {
            unsigned index = 0;
            if (!readStringIndex(index)) {
                fail();
                return false;
            }
            if (index >= m_constantPool.size()) {
                fail();
                return false;
            }
            cachedString = CachedStringRef(&m_constantPool, index);
            return true;
        }
        String str;
        if (!readString(m_ptr, m_end, str, length)) {
            fail();
            return false;
        }
        m_constantPool.append(str);
        cachedString = CachedStringRef(&m_constantPool, m_constantPool.size() - 1);
        return true;
    }

    SerializationTag readTag()
    {
        if (m_ptr >= m_end)
            return ErrorTag;
        return static_cast<SerializationTag>(*m_ptr++);
    }

    bool readArrayBufferViewSubtag(ArrayBufferViewSubtag& tag)
    {
        if (m_ptr >= m_end)
            return false;
        tag = static_cast<ArrayBufferViewSubtag>(*m_ptr++);
        return true;
    }

    void putProperty(JSObject* object, unsigned index, JSValue value)
    {
        object->putDirectIndex(m_exec, index, value);
    }

    void putProperty(JSObject* object, const Identifier& property, JSValue value)
    {
        object->putDirectMayBeIndex(m_exec, property, value);
    }

    bool readFile(RefPtr<File>& file)
    {
        CachedStringRef path;
        if (!readStringData(path))
            return 0;
        CachedStringRef url;
        if (!readStringData(url))
            return 0;
        CachedStringRef type;
        if (!readStringData(type))
            return 0;
        if (m_isDOMGlobalObject)
            file = File::create(path->string(), KURL(KURL(), url->string()), type->string());
        return true;
    }

    bool readArrayBuffer(RefPtr<ArrayBuffer>& arrayBuffer)
    {
        uint32_t length;
        if (!read(length))
            return false;
        if (m_ptr + length > m_end)
            return false;
        arrayBuffer = ArrayBuffer::create(m_ptr, length);
        m_ptr += length;
        return true;
    }

    bool readArrayBufferView(JSValue& arrayBufferView)
    {
        ArrayBufferViewSubtag arrayBufferViewSubtag;
        if (!readArrayBufferViewSubtag(arrayBufferViewSubtag))
            return false;
        uint32_t byteOffset;
        if (!read(byteOffset))
            return false;
        uint32_t byteLength;
        if (!read(byteLength))
            return false;
        JSObject* arrayBufferObj = asObject(readTerminal());
        if (!arrayBufferObj || !arrayBufferObj->inherits(&JSArrayBuffer::s_info))
            return false;

        unsigned elementSize = typedArrayElementSize(arrayBufferViewSubtag);
        if (!elementSize)
            return false;
        unsigned length = byteLength / elementSize;
        if (length * elementSize != byteLength)
            return false;

        RefPtr<ArrayBuffer> arrayBuffer = toArrayBuffer(arrayBufferObj);
        switch (arrayBufferViewSubtag) {
        case DataViewTag:
            arrayBufferView = getJSValue(DataView::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Int8ArrayTag:
            arrayBufferView = getJSValue(Int8Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Uint8ArrayTag:
            arrayBufferView = getJSValue(Uint8Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Uint8ClampedArrayTag:
            arrayBufferView = getJSValue(Uint8ClampedArray::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Int16ArrayTag:
            arrayBufferView = getJSValue(Int16Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Uint16ArrayTag:
            arrayBufferView = getJSValue(Uint16Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Int32ArrayTag:
            arrayBufferView = getJSValue(Int32Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Uint32ArrayTag:
            arrayBufferView = getJSValue(Uint32Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Float32ArrayTag:
            arrayBufferView = getJSValue(Float32Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        case Float64ArrayTag:
            arrayBufferView = getJSValue(Float64Array::create(arrayBuffer, byteOffset, length).get());
            return true;
        default:
            return false;
        }
    }

    template<class T>
    JSValue getJSValue(T* nativeObj)
    {
        return toJS(m_exec, jsCast<JSDOMGlobalObject*>(m_globalObject), nativeObj);
    }

    JSValue readTerminal()
    {
        SerializationTag tag = readTag();
        switch (tag) {
        case UndefinedTag:
            return jsUndefined();
        case NullTag:
            return jsNull();
        case IntTag: {
            int32_t i;
            if (!read(i))
                return JSValue();
            return jsNumber(i);
        }
        case ZeroTag:
            return jsNumber(0);
        case OneTag:
            return jsNumber(1);
        case FalseTag:
            return jsBoolean(false);
        case TrueTag:
            return jsBoolean(true);
        case FalseObjectTag: {
            BooleanObject* obj = BooleanObject::create(m_exec->vm(), m_globalObject->booleanObjectStructure());
            obj->setInternalValue(m_exec->vm(), jsBoolean(false));
            m_gcBuffer.append(obj);
            return obj;
        }
        case TrueObjectTag: {
            BooleanObject* obj = BooleanObject::create(m_exec->vm(), m_globalObject->booleanObjectStructure());
            obj->setInternalValue(m_exec->vm(), jsBoolean(true));
             m_gcBuffer.append(obj);
            return obj;
        }
        case DoubleTag: {
            double d;
            if (!read(d))
                return JSValue();
            return jsNumber(d);
        }
        case NumberObjectTag: {
            double d;
            if (!read(d))
                return JSValue();
            NumberObject* obj = constructNumber(m_exec, m_globalObject, jsNumber(d));
            m_gcBuffer.append(obj);
            return obj;
        }
        case DateTag: {
            double d;
            if (!read(d))
                return JSValue();
            return DateInstance::create(m_exec, m_globalObject->dateStructure(), d);
        }
        case FileTag: {
            RefPtr<File> file;
            if (!readFile(file))
                return JSValue();
            if (!m_isDOMGlobalObject)
                return jsNull();
            return toJS(m_exec, jsCast<JSDOMGlobalObject*>(m_globalObject), file.get());
        }
        case FileListTag: {
            unsigned length = 0;
            if (!read(length))
                return JSValue();
            RefPtr<FileList> result = FileList::create();
            for (unsigned i = 0; i < length; i++) {
                RefPtr<File> file;
                if (!readFile(file))
                    return JSValue();
                if (m_isDOMGlobalObject)
                    result->append(file.get());
            }
            if (!m_isDOMGlobalObject)
                return jsNull();
            return getJSValue(result.get());
        }
        case ImageDataTag: {
            int32_t width;
            if (!read(width))
                return JSValue();
            int32_t height;
            if (!read(height))
                return JSValue();
            uint32_t length;
            if (!read(length))
                return JSValue();
            if (m_end < ((uint8_t*)0) + length || m_ptr > m_end - length) {
                fail();
                return JSValue();
            }
            if (!m_isDOMGlobalObject) {
                m_ptr += length;
                return jsNull();
            }
            RefPtr<ImageData> result = ImageData::create(IntSize(width, height));
            memcpy(result->data()->data(), m_ptr, length);
            m_ptr += length;
            return getJSValue(result.get());
        }
        case BlobTag: {
            CachedStringRef url;
            if (!readStringData(url))
                return JSValue();
            CachedStringRef type;
            if (!readStringData(type))
                return JSValue();
            unsigned long long size = 0;
            if (!read(size))
                return JSValue();
            if (!m_isDOMGlobalObject)
                return jsNull();
            return getJSValue(Blob::create(KURL(KURL(), url->string()), type->string(), size).get());
        }
        case StringTag: {
            CachedStringRef cachedString;
            if (!readStringData(cachedString))
                return JSValue();
            return cachedString->jsString(m_exec);
        }
        case EmptyStringTag:
            return jsEmptyString(&m_exec->vm());
        case StringObjectTag: {
            CachedStringRef cachedString;
            if (!readStringData(cachedString))
                return JSValue();
            StringObject* obj = constructString(m_exec, m_globalObject, cachedString->jsString(m_exec));
            m_gcBuffer.append(obj);
            return obj;
        }
        case EmptyStringObjectTag: {
            StringObject* obj = constructString(m_exec, m_globalObject, jsEmptyString(&m_exec->vm()));
            m_gcBuffer.append(obj);
            return obj;
        }
        case RegExpTag: {
            CachedStringRef pattern;
            if (!readStringData(pattern))
                return JSValue();
            CachedStringRef flags;
            if (!readStringData(flags))
                return JSValue();
            RegExpFlags reFlags = regExpFlags(flags->string());
            ASSERT(reFlags != InvalidFlags);
            RegExp* regExp = RegExp::create(m_exec->vm(), pattern->string(), reFlags);
            return RegExpObject::create(m_exec, m_exec->lexicalGlobalObject(), m_globalObject->regExpStructure(), regExp); 
        }
        case ObjectReferenceTag: {
            unsigned index = 0;
            if (!readConstantPoolIndex(m_gcBuffer, index)) {
                fail();
                return JSValue();
            }
            return m_gcBuffer.at(index);
        }
        case MessagePortReferenceTag: {
            uint32_t index;
            bool indexSuccessfullyRead = read(index);
            if (!indexSuccessfullyRead || !m_messagePorts || index >= m_messagePorts->size()) {
                fail();
                return JSValue();
            }
            return getJSValue(m_messagePorts->at(index).get());
        }
        case ArrayBufferTag: {
            RefPtr<ArrayBuffer> arrayBuffer;
            if (!readArrayBuffer(arrayBuffer)) {
                fail();
                return JSValue();
            }
            JSValue result = getJSValue(arrayBuffer.get());
            m_gcBuffer.append(result);
            return result;
        }
        case ArrayBufferTransferTag: {
            uint32_t index;
            bool indexSuccessfullyRead = read(index);
            if (!indexSuccessfullyRead || index >= m_arrayBuffers.size()) {
                fail();
                return JSValue();
            }

            if (!m_arrayBuffers[index])
                m_arrayBuffers[index] = ArrayBuffer::create(m_arrayBufferContents->at(index));

            return getJSValue(m_arrayBuffers[index].get());
        }
        case ArrayBufferViewTag: {
            JSValue arrayBufferView;
            if (!readArrayBufferView(arrayBufferView)) {
                fail();
                return JSValue();
            }
            m_gcBuffer.append(arrayBufferView);
            return arrayBufferView;
        }
        default:
            m_ptr--; // Push the tag back
            return JSValue();
        }
    }

    JSGlobalObject* m_globalObject;
    bool m_isDOMGlobalObject;
    const uint8_t* m_ptr;
    const uint8_t* m_end;
    unsigned m_version;
    Vector<CachedString> m_constantPool;
    MessagePortArray* m_messagePorts;
    ArrayBufferContentsArray* m_arrayBufferContents;
    ArrayBufferArray m_arrayBuffers;
};

DeserializationResult CloneDeserializer::deserialize()
{
    Vector<uint32_t, 16> indexStack;
    Vector<Identifier, 16> propertyNameStack;
    Vector<JSObject*, 32> outputObjectStack;
    Vector<WalkerState, 16> stateStack;
    WalkerState state = StateUnknown;
    JSValue outValue;

    while (1) {
        switch (state) {
        arrayStartState:
        case ArrayStartState: {
            uint32_t length;
            if (!read(length)) {
                fail();
                goto error;
            }
            JSArray* outArray = constructEmptyArray(m_exec, 0, m_globalObject, length);
            m_gcBuffer.append(outArray);
            outputObjectStack.append(outArray);
            // fallthrough
        }
        arrayStartVisitMember:
        case ArrayStartVisitMember: {
            uint32_t index;
            if (!read(index)) {
                fail();
                goto error;
            }
            if (index == TerminatorTag) {
                JSObject* outArray = outputObjectStack.last();
                outValue = outArray;
                outputObjectStack.removeLast();
                break;
            } else if (index == NonIndexPropertiesTag) {
                goto objectStartVisitMember;
            }

            if (JSValue terminal = readTerminal()) {
                putProperty(outputObjectStack.last(), index, terminal);
                goto arrayStartVisitMember;
            }
            if (m_failed)
                goto error;
            indexStack.append(index);
            stateStack.append(ArrayEndVisitMember);
            goto stateUnknown;
        }
        case ArrayEndVisitMember: {
            JSObject* outArray = outputObjectStack.last();
            putProperty(outArray, indexStack.last(), outValue);
            indexStack.removeLast();
            goto arrayStartVisitMember;
        }
        objectStartState:
        case ObjectStartState: {
            if (outputObjectStack.size() > maximumFilterRecursion)
                return make_pair(JSValue(), StackOverflowError);
            JSObject* outObject = constructEmptyObject(m_exec, m_globalObject->objectPrototype());
            m_gcBuffer.append(outObject);
            outputObjectStack.append(outObject);
            // fallthrough
        }
        objectStartVisitMember:
        case ObjectStartVisitMember: {
            CachedStringRef cachedString;
            bool wasTerminator = false;
            if (!readStringData(cachedString, wasTerminator)) {
                if (!wasTerminator)
                    goto error;

                JSObject* outObject = outputObjectStack.last();
                outValue = outObject;
                outputObjectStack.removeLast();
                break;
            }

            if (JSValue terminal = readTerminal()) {
                putProperty(outputObjectStack.last(), Identifier(m_exec, cachedString->string()), terminal);
                goto objectStartVisitMember;
            }
            stateStack.append(ObjectEndVisitMember);
            propertyNameStack.append(Identifier(m_exec, cachedString->string()));
            goto stateUnknown;
        }
        case ObjectEndVisitMember: {
            putProperty(outputObjectStack.last(), propertyNameStack.last(), outValue);
            propertyNameStack.removeLast();
            goto objectStartVisitMember;
        }
        stateUnknown:
        case StateUnknown:
            if (JSValue terminal = readTerminal()) {
                outValue = terminal;
                break;
            }
            SerializationTag tag = readTag();
            if (tag == ArrayTag)
                goto arrayStartState;
            if (tag == ObjectTag)
                goto objectStartState;
            goto error;
        }
        if (stateStack.isEmpty())
            break;

        state = stateStack.last();
        stateStack.removeLast();
    }
    ASSERT(outValue);
    ASSERT(!m_failed);
    return make_pair(outValue, SuccessfullyCompleted);
error:
    fail();
    return make_pair(JSValue(), ValidationError);
}



SerializedScriptValue::~SerializedScriptValue()
{
}

SerializedScriptValue::SerializedScriptValue(const Vector<uint8_t>& buffer)
    : m_data(buffer)
{
}

SerializedScriptValue::SerializedScriptValue(Vector<uint8_t>& buffer)
{
    m_data.swap(buffer);
}

SerializedScriptValue::SerializedScriptValue(Vector<uint8_t>& buffer, Vector<String>& blobURLs)
{
    m_data.swap(buffer);
    m_blobURLs.swap(blobURLs);
}

SerializedScriptValue::SerializedScriptValue(Vector<uint8_t>& buffer, Vector<String>& blobURLs, PassOwnPtr<ArrayBufferContentsArray> arrayBufferContentsArray)
    : m_arrayBufferContentsArray(arrayBufferContentsArray)
{
    m_data.swap(buffer);
    m_blobURLs.swap(blobURLs);
}

static void neuterView(JSCell* jsView)
{
    if (!jsView)
        return;
    
    switch (jsView->classInfo()->typedArrayStorageType) {
    case TypedArrayNone:
        // This could be a DataView, for example. Assume that there are views that the
        // DFG doesn't care about.
        return;
    case TypedArrayInt8:
        jsCast<JSInt8Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayInt16:
        jsCast<JSInt16Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayInt32:
        jsCast<JSInt32Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayUint8:
        jsCast<JSUint8Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayUint8Clamped:
        jsCast<JSUint8ClampedArray*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayUint16:
        jsCast<JSUint16Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayUint32:
        jsCast<JSUint32Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayFloat32:
        jsCast<JSFloat32Array*>(jsView)->m_storageLength = 0;
        return;
    case TypedArrayFloat64:
        jsCast<JSFloat64Array*>(jsView)->m_storageLength = 0;
        return;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

PassOwnPtr<SerializedScriptValue::ArrayBufferContentsArray> SerializedScriptValue::transferArrayBuffers(
    ExecState* exec, ArrayBufferArray& arrayBuffers, SerializationReturnCode& code)
{
    for (size_t i = 0; i < arrayBuffers.size(); i++) {
        if (arrayBuffers[i]->isNeutered()) {
            code = ValidationError;
            return nullptr;
        }
    }

    OwnPtr<ArrayBufferContentsArray> contents = adoptPtr(new ArrayBufferContentsArray(arrayBuffers.size()));
    Vector<RefPtr<DOMWrapperWorld> > worlds;
    static_cast<WebCoreJSClientData*>(exec->vm().clientData)->getAllWorlds(worlds);

    HashSet<WTF::ArrayBuffer*> visited;
    for (size_t arrayBufferIndex = 0; arrayBufferIndex < arrayBuffers.size(); arrayBufferIndex++) {
        Vector<RefPtr<ArrayBufferView> > neuteredViews;

        if (visited.contains(arrayBuffers[arrayBufferIndex].get()))
            continue;
        visited.add(arrayBuffers[arrayBufferIndex].get());

        bool result = arrayBuffers[arrayBufferIndex]->transfer(contents->at(arrayBufferIndex), neuteredViews);
        if (!result) {
            code = ValidationError;
            return nullptr;
        }
        
        // The views may have been neutered, but their wrappers also need to be neutered, too.
        for (size_t viewIndex = neuteredViews.size(); viewIndex--;) {
            ArrayBufferView* view = neuteredViews[viewIndex].get();
            for (size_t worldIndex = worlds.size(); worldIndex--;) {
                DOMWrapperWorld* world = worlds[worldIndex].get();
                neuterView(getCachedWrapper(world, view));
            }
        }
    }
    return contents.release();
}


PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(ExecState* exec, JSValue value,
                                                                MessagePortArray* messagePorts, ArrayBufferArray* arrayBuffers,
                                                                SerializationErrorMode throwExceptions)
{
    Vector<uint8_t> buffer;
    Vector<String> blobURLs;
    SerializationReturnCode code = CloneSerializer::serialize(exec, value, messagePorts, arrayBuffers, blobURLs, buffer);

    OwnPtr<ArrayBufferContentsArray> arrayBufferContentsArray;

    if (arrayBuffers && serializationDidCompleteSuccessfully(code))
        arrayBufferContentsArray = transferArrayBuffers(exec, *arrayBuffers, code);

    if (throwExceptions == Throwing)
        maybeThrowExceptionIfSerializationFailed(exec, code);

    if (!serializationDidCompleteSuccessfully(code))
        return 0;

    return adoptRef(new SerializedScriptValue(buffer, blobURLs, arrayBufferContentsArray.release()));
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create()
{
    Vector<uint8_t> buffer;
    return adoptRef(new SerializedScriptValue(buffer));
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(const String& string)
{
    Vector<uint8_t> buffer;
    if (!CloneSerializer::serialize(string, buffer))
        return 0;
    return adoptRef(new SerializedScriptValue(buffer));
}

#if ENABLE(INDEXED_DATABASE)
PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(JSC::ExecState* exec, JSC::JSValue value)
{
    return SerializedScriptValue::create(exec, value, 0, 0);
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::numberValue(double value)
{
    Vector<uint8_t> buffer;
    CloneSerializer::serializeNumber(value, buffer);
    return adoptRef(new SerializedScriptValue(buffer));
}

JSValue SerializedScriptValue::deserialize(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
    return deserialize(exec, globalObject, 0);
}
#endif

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(JSContextRef originContext, JSValueRef apiValue, 
                                                                MessagePortArray* messagePorts, ArrayBufferArray* arrayBuffers,
                                                                JSValueRef* exception)
{
    ExecState* exec = toJS(originContext);
    APIEntryShim entryShim(exec);
    JSValue value = toJS(exec, apiValue);
    RefPtr<SerializedScriptValue> serializedValue = SerializedScriptValue::create(exec, value, messagePorts, arrayBuffers);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    ASSERT(serializedValue);
    return serializedValue.release();
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(JSContextRef originContext, JSValueRef apiValue,
                                                                JSValueRef* exception)
{
    return create(originContext, apiValue, 0, 0, exception);
}

String SerializedScriptValue::toString()
{
    return CloneDeserializer::deserializeString(m_data);
}

JSValue SerializedScriptValue::deserialize(ExecState* exec, JSGlobalObject* globalObject,
                                           MessagePortArray* messagePorts, SerializationErrorMode throwExceptions)
{
    DeserializationResult result = CloneDeserializer::deserialize(exec, globalObject, messagePorts,
                                                                  m_arrayBufferContentsArray.get(), m_data);
    if (throwExceptions == Throwing)
        maybeThrowExceptionIfSerializationFailed(exec, result.second);
    return result.first;
}

#if ENABLE(INSPECTOR)
ScriptValue SerializedScriptValue::deserializeForInspector(ScriptState* scriptState)
{
    JSValue value = deserialize(scriptState, scriptState->lexicalGlobalObject(), 0);
    return ScriptValue(scriptState->vm(), value);
}
#endif

JSValueRef SerializedScriptValue::deserialize(JSContextRef destinationContext, JSValueRef* exception, MessagePortArray* messagePorts)
{
    ExecState* exec = toJS(destinationContext);
    APIEntryShim entryShim(exec);
    JSValue value = deserialize(exec, exec->lexicalGlobalObject(), messagePorts);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    ASSERT(value);
    return toRef(exec, value);
}


JSValueRef SerializedScriptValue::deserialize(JSContextRef destinationContext, JSValueRef* exception)
{
    return deserialize(destinationContext, exception, 0);
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::nullValue()
{
    return SerializedScriptValue::create();
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::undefinedValue()
{
    Vector<uint8_t> buffer;
    CloneSerializer::serializeUndefined(buffer);
    return adoptRef(new SerializedScriptValue(buffer));
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::booleanValue(bool value)
{
    Vector<uint8_t> buffer;
    CloneSerializer::serializeBoolean(value, buffer);
    return adoptRef(new SerializedScriptValue(buffer));
}

void SerializedScriptValue::maybeThrowExceptionIfSerializationFailed(ExecState* exec, SerializationReturnCode code)
{
    if (code == SuccessfullyCompleted)
        return;
    
    switch (code) {
    case StackOverflowError:
        throwError(exec, createStackOverflowError(exec));
        break;
    case ValidationError:
        throwError(exec, createTypeError(exec, "Unable to deserialize data."));
        break;
    case DataCloneError:
        setDOMException(exec, DATA_CLONE_ERR);
        break;
    case ExistingExceptionError:
        break;
    case UnspecifiedError:
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

bool SerializedScriptValue::serializationDidCompleteSuccessfully(SerializationReturnCode code)
{
    return (code == SuccessfullyCompleted);
}

uint32_t SerializedScriptValue::wireFormatVersion()
{
    return CurrentVersion;
}

}

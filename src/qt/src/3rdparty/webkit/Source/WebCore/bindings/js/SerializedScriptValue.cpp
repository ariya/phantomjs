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
 *
 */

#include "config.h"
#include "SerializedScriptValue.h"

#include "Blob.h"
#include "File.h"
#include "FileList.h"
#include "ImageData.h"
#include "JSBlob.h"
#include "JSDOMGlobalObject.h"
#include "JSFile.h"
#include "JSFileList.h"
#include "JSImageData.h"
#include "JSNavigator.h"
#include "SharedBuffer.h"
#include <limits>
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/APIShims.h>
#include <runtime/DateInstance.h>
#include <runtime/Error.h>
#include <runtime/ExceptionHelpers.h>
#include <runtime/PropertyNameArray.h>
#include <runtime/RegExp.h>
#include <runtime/RegExpObject.h>
#include <wtf/ByteArray.h>
#include <wtf/HashTraits.h>
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
    ErrorTag = 255
};

/* CurrentVersion tracks the serialization version so that persistant stores
 * are able to correctly bail out in the case of encountering newer formats.
 *
 * Initial version was 1.
 * Version 2. added the ObjectReferenceTag and support for serialization of cyclic graphs.
 */
static const unsigned int CurrentVersion = 2;
static const unsigned int TerminatorTag = 0xFFFFFFFF;
static const unsigned int StringPoolTag = 0xFFFFFFFE;

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
 *    | DoubleTag <value:double>
 *    | DateTag <value:double>
 *    | String
 *    | EmptyStringTag
 *    | File
 *    | FileList
 *    | ImageData
 *    | Blob
 *    | ObjectReferenceTag <opIndex:IndexType>
 *
 * String :-
 *      EmptyStringTag
 *      StringTag StringData
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
 */

typedef pair<JSC::JSValue, SerializationReturnCode> DeserializationResult;

class CloneBase {
protected:
    CloneBase(ExecState* exec)
        : m_exec(exec)
        , m_failed(false)
        , m_timeoutChecker(exec->globalData().timeoutChecker)
    {
    }

    bool shouldTerminate()
    {
        return m_exec->hadException();
    }

    unsigned ticksUntilNextCheck()
    {
        return m_timeoutChecker.ticksUntilNextCheck();
    }

    bool didTimeOut()
    {
        return m_timeoutChecker.didTimeOut(m_exec);
    }

    void throwStackOverflow()
    {
        throwError(m_exec, createStackOverflowError(m_exec));
    }

    void throwInterruptedException()
    {
        throwError(m_exec, createInterruptedExecutionException(&m_exec->globalData()));
    }

    void fail()
    {
        ASSERT_NOT_REACHED();
        m_failed = true;
    }

    ExecState* m_exec;
    bool m_failed;
    TimeoutChecker m_timeoutChecker;
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
    static SerializationReturnCode serialize(ExecState* exec, JSValue value, Vector<uint8_t>& out)
    {
        CloneSerializer serializer(exec, out);
        return serializer.serialize(value);
    }

    static bool serialize(String s, Vector<uint8_t>& out)
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

private:
    CloneSerializer(ExecState* exec, Vector<uint8_t>& out)
        : CloneBase(exec)
        , m_buffer(out)
        , m_emptyIdentifier(exec, UString("", 0))
    {
        write(CurrentVersion);
    }

    SerializationReturnCode serialize(JSValue in);

    bool isArray(JSValue value)
    {
        if (!value.isObject())
            return false;
        JSObject* object = asObject(value);
        return isJSArray(&m_exec->globalData(), object) || object->inherits(&JSArray::s_info);
    }

    bool startObjectInternal(JSObject* object)
    {
        // Record object for graph reconstruction
        pair<ObjectPool::iterator, bool> iter = m_objectPool.add(object, m_objectPool.size());
        
        // Handle duplicate references
        if (!iter.second) {
            write(ObjectReferenceTag);
            ASSERT(static_cast<int32_t>(iter.first->second) < m_objectPool.size());
            writeObjectIndex(iter.first->second);
            return false;
        }
        
        m_gcBuffer.append(object);
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

    JSValue getSparseIndex(JSArray* array, unsigned propertyName, bool& hasIndex)
    {
        PropertySlot slot(array);
        if (isJSArray(&m_exec->globalData(), array)) {
            if (array->JSArray::getOwnPropertySlot(m_exec, propertyName, slot)) {
                hasIndex = true;
                return slot.getValue(m_exec, propertyName);
            }
        } else if (array->getOwnPropertySlot(m_exec, propertyName, slot)) {
            hasIndex = true;
            return slot.getValue(m_exec, propertyName);
        }
        hasIndex = false;
        return jsNull();
    }

    JSValue getProperty(JSObject* object, const Identifier& propertyName)
    {
        PropertySlot slot(object);
        if (object->getOwnPropertySlot(m_exec, propertyName, slot))
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

    void dumpString(UString str)
    {
        if (str.isEmpty())
            write(EmptyStringTag);
        else {
            write(StringTag);
            write(str);
        }
    }

    bool dumpIfTerminal(JSValue value)
    {
        if (!value.isCell()) {
            dumpImmediate(value);
            return true;
        }

        if (value.isString()) {
            UString str = asString(value)->value(m_exec);
            dumpString(str);
            return true;
        }

        if (value.isNumber()) {
            write(DoubleTag);
            write(value.uncheckedGetNumber());
            return true;
        }

        if (value.isObject() && asObject(value)->inherits(&DateInstance::s_info)) {
            write(DateTag);
            write(asDateInstance(value)->internalNumber());
            return true;
        }

        if (isArray(value))
            return false;
           
        // Object cannot be serialized because the act of walking the object creates new objects
        if (value.isObject() && asObject(value)->inherits(&JSNavigator::s_info)) {
            fail();
            write(NullTag);
            return true; 
        }

        if (value.isObject()) {
            JSObject* obj = asObject(value);
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
                write(data->data()->data()->data(), data->data()->length());
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
                write(UString(flags, flagCount));
                return true;
            }

            CallData unusedData;
            if (getCallData(value, unusedData) == CallTypeNone)
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
        UString str = ident.ustring();
        pair<StringConstantPool::iterator, bool> iter = m_constantPool.add(str.impl(), m_constantPool.size());
        if (!iter.second) {
            write(StringPoolTag);
            writeStringIndex(iter.first->second);
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

    void write(const UString& str)
    {
        if (str.isNull())
            write(m_emptyIdentifier);
        else
            write(Identifier(m_exec, str));
    }

    void write(const String& str)
    {
        if (str.isEmpty())
            write(m_emptyIdentifier);
        else
            write(Identifier(m_exec, str.impl()));
    }

    void write(const File* file)
    {
        write(file->path());
        write(file->url());
        write(file->type());
    }

    void write(const uint8_t* data, unsigned length)
    {
        m_buffer.append(data, length);
    }

    Vector<uint8_t>& m_buffer;
    typedef HashMap<JSObject*, uint32_t> ObjectPool;
    ObjectPool m_objectPool;
    typedef HashMap<RefPtr<StringImpl>, uint32_t, IdentifierRepHash> StringConstantPool;
    StringConstantPool m_constantPool;
    Identifier m_emptyIdentifier;
};

SerializationReturnCode CloneSerializer::serialize(JSValue in)
{
    Vector<uint32_t, 16> indexStack;
    Vector<uint32_t, 16> lengthStack;
    Vector<PropertyNameArray, 16> propertyStack;
    Vector<JSObject*, 16> inputObjectStack;
    Vector<JSArray*, 16> inputArrayStack;
    Vector<WalkerState, 16> stateStack;
    WalkerState state = StateUnknown;
    JSValue inValue = in;
    unsigned tickCount = ticksUntilNextCheck();
    while (1) {
        switch (state) {
            arrayStartState:
            case ArrayStartState: {
                ASSERT(isArray(inValue));
                if (inputObjectStack.size() + inputArrayStack.size() > maximumFilterRecursion)
                    return StackOverflowError;

                JSArray* inArray = asArray(inValue);
                unsigned length = inArray->length();
                if (!startArray(inArray))
                    break;
                inputArrayStack.append(inArray);
                indexStack.append(0);
                lengthStack.append(length);
                // fallthrough
            }
            arrayStartVisitMember:
            case ArrayStartVisitMember: {
                if (!--tickCount) {
                    if (didTimeOut())
                        return InterruptedExecutionError;
                    tickCount = ticksUntilNextCheck();
                }

                JSArray* array = inputArrayStack.last();
                uint32_t index = indexStack.last();
                if (index == lengthStack.last()) {
                    endObject();
                    inputArrayStack.removeLast();
                    indexStack.removeLast();
                    lengthStack.removeLast();
                    break;
                }
                if (array->canGetIndex(index))
                    inValue = array->getIndex(index);
                else {
                    bool hasIndex = false;
                    inValue = getSparseIndex(array, index, hasIndex);
                    if (!hasIndex) {
                        indexStack.last()++;
                        goto arrayStartVisitMember;
                    }
                }

                write(index);
                if (dumpIfTerminal(inValue)) {
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
                if (inputObjectStack.size() + inputArrayStack.size() > maximumFilterRecursion)
                    return StackOverflowError;
                JSObject* inObject = asObject(inValue);
                if (!startObject(inObject))
                    break;
                inputObjectStack.append(inObject);
                indexStack.append(0);
                propertyStack.append(PropertyNameArray(m_exec));
                inObject->getOwnPropertyNames(m_exec, propertyStack.last());
                // fallthrough
            }
            objectStartVisitMember:
            case ObjectStartVisitMember: {
                if (!--tickCount) {
                    if (didTimeOut())
                        return InterruptedExecutionError;
                    tickCount = ticksUntilNextCheck();
                }

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

                if (!dumpIfTerminal(inValue)) {
                    stateStack.append(ObjectEndVisitMember);
                    goto stateUnknown;
                }
                // fallthrough
            }
            case ObjectEndVisitMember: {
                if (shouldTerminate())
                    return ExistingExceptionError;

                indexStack.last()++;
                goto objectStartVisitMember;
            }
            stateUnknown:
            case StateUnknown:
                if (dumpIfTerminal(inValue))
                    break;

                if (isArray(inValue))
                    goto arrayStartState;
                goto objectStartState;
        }
        if (stateStack.isEmpty())
            break;

        state = stateStack.last();
        stateStack.removeLast();

        if (!--tickCount) {
            if (didTimeOut())
                return InterruptedExecutionError;
            tickCount = ticksUntilNextCheck();
        }
    }
    if (m_failed)
        return UnspecifiedError;

    return SuccessfullyCompleted;
}

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
        UString str;
        if (!readString(ptr, end, str, length))
            return String();
        return String(str.impl());
    }

    static DeserializationResult deserialize(ExecState* exec, JSGlobalObject* globalObject, const Vector<uint8_t>& buffer)
    {
        if (!buffer.size())
            return make_pair(jsNull(), UnspecifiedError);
        CloneDeserializer deserializer(exec, globalObject, buffer);
        if (!deserializer.isValid())
            return make_pair(JSValue(), ValidationError);
        return deserializer.deserialize();
    }

private:
    struct CachedString {
        CachedString(const UString& string)
            : m_string(string)
        {
        }

        JSValue jsString(ExecState* exec)
        {
            if (!m_jsString)
                m_jsString = JSC::jsString(exec, m_string);
            return m_jsString;
        }
        const UString& ustring() { return m_string; }

    private:
        UString m_string;
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

    CloneDeserializer(ExecState* exec, JSGlobalObject* globalObject, const Vector<uint8_t>& buffer)
        : CloneBase(exec)
        , m_globalObject(globalObject)
        , m_isDOMGlobalObject(globalObject->inherits(&JSDOMGlobalObject::s_info))
        , m_ptr(buffer.data())
        , m_end(buffer.data() + buffer.size())
        , m_version(0xFFFFFFFF)
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

    static bool readString(const uint8_t*& ptr, const uint8_t* end, UString& str, unsigned length)
    {
        if (length >= numeric_limits<int32_t>::max() / sizeof(UChar))
            return false;

        unsigned size = length * sizeof(UChar);
        if ((end - ptr) < static_cast<int>(size))
            return false;

#if ASSUME_LITTLE_ENDIAN
        str = UString(reinterpret_cast<const UChar*>(ptr), length);
        ptr += length * sizeof(UChar);
#else
        Vector<UChar> buffer;
        buffer.reserveCapacity(length);
        for (unsigned i = 0; i < length; i++) {
            uint16_t ch;
            readLittleEndian(ptr, end, ch);
            buffer.append(ch);
        }
        str = UString::adopt(buffer);
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
        UString str;
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

    void putProperty(JSArray* array, unsigned index, JSValue value)
    {
        if (array->canSetIndex(index))
            array->setIndex(m_exec->globalData(), index, value);
        else
            array->put(m_exec, index, value);
    }

    void putProperty(JSObject* object, const Identifier& property, JSValue value)
    {
        object->putDirect(m_exec->globalData(), property, value);
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
            file = File::create(String(path->ustring().impl()), KURL(KURL(), String(url->ustring().impl())), String(type->ustring().impl()));
        return true;
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
        case DoubleTag: {
            double d;
            if (!read(d))
                return JSValue();
            return jsNumber(d);
        }
        case DateTag: {
            double d;
            if (!read(d))
                return JSValue();
            return new (m_exec) DateInstance(m_exec, m_globalObject->dateStructure(), d);
        }
        case FileTag: {
            RefPtr<File> file;
            if (!readFile(file))
                return JSValue();
            if (!m_isDOMGlobalObject)
                return jsNull();
            return toJS(m_exec, static_cast<JSDOMGlobalObject*>(m_globalObject), file.get());
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
            return toJS(m_exec, static_cast<JSDOMGlobalObject*>(m_globalObject), result.get());
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
            memcpy(result->data()->data()->data(), m_ptr, length);
            m_ptr += length;
            return toJS(m_exec, static_cast<JSDOMGlobalObject*>(m_globalObject), result.get());
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
            return toJS(m_exec, static_cast<JSDOMGlobalObject*>(m_globalObject), Blob::create(KURL(KURL(), url->ustring().impl()), String(type->ustring().impl()), size));
        }
        case StringTag: {
            CachedStringRef cachedString;
            if (!readStringData(cachedString))
                return JSValue();
            return cachedString->jsString(m_exec);
        }
        case EmptyStringTag:
            return jsEmptyString(&m_exec->globalData());
        case RegExpTag: {
            CachedStringRef pattern;
            if (!readStringData(pattern))
                return JSValue();
            CachedStringRef flags;
            if (!readStringData(flags))
                return JSValue();
            RegExpFlags reFlags = regExpFlags(flags->ustring());
            ASSERT(reFlags != InvalidFlags);
            RefPtr<RegExp> regExp = RegExp::create(&m_exec->globalData(), pattern->ustring(), reFlags);
            return new (m_exec) RegExpObject(m_exec->lexicalGlobalObject(), m_globalObject->regExpStructure(), regExp); 
        }
        case ObjectReferenceTag: {
            unsigned index = 0;
            if (!readConstantPoolIndex(m_gcBuffer, index)) {
                fail();
                return JSValue();
            }
            return m_gcBuffer.at(index);
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
};

DeserializationResult CloneDeserializer::deserialize()
{
    Vector<uint32_t, 16> indexStack;
    Vector<Identifier, 16> propertyNameStack;
    Vector<JSObject*, 16> outputObjectStack;
    Vector<JSArray*, 16> outputArrayStack;
    Vector<WalkerState, 16> stateStack;
    WalkerState state = StateUnknown;
    JSValue outValue;

    unsigned tickCount = ticksUntilNextCheck();
    while (1) {
        switch (state) {
        arrayStartState:
        case ArrayStartState: {
            uint32_t length;
            if (!read(length)) {
                fail();
                goto error;
            }
            JSArray* outArray = constructEmptyArray(m_exec, m_globalObject);
            outArray->setLength(length);
            m_gcBuffer.append(outArray);
            outputArrayStack.append(outArray);
            // fallthrough
        }
        arrayStartVisitMember:
        case ArrayStartVisitMember: {
            if (!--tickCount) {
                if (didTimeOut())
                    return make_pair(JSValue(), InterruptedExecutionError);
                tickCount = ticksUntilNextCheck();
            }

            uint32_t index;
            if (!read(index)) {
                fail();
                goto error;
            }
            if (index == TerminatorTag) {
                JSArray* outArray = outputArrayStack.last();
                outValue = outArray;
                outputArrayStack.removeLast();
                break;
            }

            if (JSValue terminal = readTerminal()) {
                putProperty(outputArrayStack.last(), index, terminal);
                goto arrayStartVisitMember;
            }
            if (m_failed)
                goto error;
            indexStack.append(index);
            stateStack.append(ArrayEndVisitMember);
            goto stateUnknown;
        }
        case ArrayEndVisitMember: {
            JSArray* outArray = outputArrayStack.last();
            putProperty(outArray, indexStack.last(), outValue);
            indexStack.removeLast();
            goto arrayStartVisitMember;
        }
        objectStartState:
        case ObjectStartState: {
            if (outputObjectStack.size() + outputArrayStack.size() > maximumFilterRecursion)
                return make_pair(JSValue(), StackOverflowError);
            JSObject* outObject = constructEmptyObject(m_exec, m_globalObject);
            m_gcBuffer.append(outObject);
            outputObjectStack.append(outObject);
            // fallthrough
        }
        objectStartVisitMember:
        case ObjectStartVisitMember: {
            if (!--tickCount) {
                if (didTimeOut())
                    return make_pair(JSValue(), InterruptedExecutionError);
                tickCount = ticksUntilNextCheck();
            }

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
                putProperty(outputObjectStack.last(), Identifier(m_exec, cachedString->ustring()), terminal);
                goto objectStartVisitMember;
            }
            stateStack.append(ObjectEndVisitMember);
            propertyNameStack.append(Identifier(m_exec, cachedString->ustring()));
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

        if (!--tickCount) {
            if (didTimeOut())
                return make_pair(JSValue(), InterruptedExecutionError);
            tickCount = ticksUntilNextCheck();
        }
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

SerializedScriptValue::SerializedScriptValue(Vector<uint8_t>& buffer)
{
    m_data.swap(buffer);
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(ExecState* exec, JSValue value, SerializationErrorMode throwExceptions)
{
    Vector<uint8_t> buffer;
    SerializationReturnCode code = CloneSerializer::serialize(exec, value, buffer);
    if (throwExceptions == Throwing)
        maybeThrowExceptionIfSerializationFailed(exec, code);

    if (!serializationDidCompleteSuccessfully(code))
        return 0;
        
    return adoptRef(new SerializedScriptValue(buffer));
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create()
{
    Vector<uint8_t> buffer;
    return adoptRef(new SerializedScriptValue(buffer));
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(String string)
{
    Vector<uint8_t> buffer;
    if (!CloneSerializer::serialize(string, buffer))
        return 0;
    return adoptRef(new SerializedScriptValue(buffer));
}

PassRefPtr<SerializedScriptValue> SerializedScriptValue::create(JSContextRef originContext, JSValueRef apiValue, JSValueRef* exception)
{
    ExecState* exec = toJS(originContext);
    APIEntryShim entryShim(exec);
    JSValue value = toJS(exec, apiValue);
    PassRefPtr<SerializedScriptValue> serializedValue = SerializedScriptValue::create(exec, value);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    ASSERT(serializedValue);
    return serializedValue;
}

String SerializedScriptValue::toString()
{
    return CloneDeserializer::deserializeString(m_data);
}

JSValue SerializedScriptValue::deserialize(ExecState* exec, JSGlobalObject* globalObject, SerializationErrorMode throwExceptions)
{
    DeserializationResult result = CloneDeserializer::deserialize(exec, globalObject, m_data);
    if (throwExceptions == Throwing)
        maybeThrowExceptionIfSerializationFailed(exec, result.second);
    return result.first;
}

JSValueRef SerializedScriptValue::deserialize(JSContextRef destinationContext, JSValueRef* exception)
{
    ExecState* exec = toJS(destinationContext);
    APIEntryShim entryShim(exec);
    JSValue value = deserialize(exec, exec->lexicalGlobalObject());
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    ASSERT(value);
    return toRef(exec, value);
}

SerializedScriptValue* SerializedScriptValue::nullValue()
{
    DEFINE_STATIC_LOCAL(RefPtr<SerializedScriptValue>, emptyValue, (SerializedScriptValue::create()));
    return emptyValue.get();
}

void SerializedScriptValue::maybeThrowExceptionIfSerializationFailed(ExecState* exec, SerializationReturnCode code)
{
    if (code == SuccessfullyCompleted)
        return;
    
    switch (code) {
    case StackOverflowError:
        throwError(exec, createStackOverflowError(exec));
        break;
    case InterruptedExecutionError:
        throwError(exec, createInterruptedExecutionException(&exec->globalData()));
        break;
    case ValidationError:
        throwError(exec, createTypeError(exec, "Unable to deserialize data."));
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

}

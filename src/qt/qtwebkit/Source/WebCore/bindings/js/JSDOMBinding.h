/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2013 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *  Copyright (C) 2012 Ericsson AB. All rights reserved.
 *  Copyright (C) 2013 Michael Pruett <michael@68k.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef JSDOMBinding_h
#define JSDOMBinding_h

#include "BindingState.h"
#include "JSDOMGlobalObject.h"
#include "JSDOMWrapper.h"
#include "DOMWrapperWorld.h"
#include "Document.h"
#include "ScriptWrappable.h"
#include "ScriptWrappableInlines.h"
#include <heap/SlotVisitor.h>
#include <heap/Weak.h>
#include <heap/WeakInlines.h>
#include <runtime/Error.h>
#include <runtime/FunctionPrototype.h>
#include <runtime/JSArray.h>
#include <runtime/Lookup.h>
#include <runtime/ObjectPrototype.h>
#include <runtime/Operations.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/NullPtr.h>
#include <wtf/Vector.h>

namespace JSC {
class HashEntry;
}

namespace WebCore {

class DOMStringList;

    class CachedScript;
    class Frame;
    class KURL;

    typedef int ExceptionCode;

    // Base class for all constructor objects in the JSC bindings.
    class DOMConstructorObject : public JSDOMWrapper {
        typedef JSDOMWrapper Base;
    public:
        static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
        {
            return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), &s_info);
        }

    protected:
        static const unsigned StructureFlags = JSC::ImplementsHasInstance | JSC::OverridesVisitChildren | JSDOMWrapper::StructureFlags;
        DOMConstructorObject(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
            : JSDOMWrapper(structure, globalObject)
        {
        }
    };

    // Constructors using this base class depend on being in a Document and
    // can never be used from a WorkerGlobalScope.
    class DOMConstructorWithDocument : public DOMConstructorObject {
        typedef DOMConstructorObject Base;
    public:
        Document* document() const
        {
            return toDocument(scriptExecutionContext());
        }

    protected:
        DOMConstructorWithDocument(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
            : DOMConstructorObject(structure, globalObject)
        {
        }

        void finishCreation(JSDOMGlobalObject* globalObject)
        {
            Base::finishCreation(globalObject->vm());
            ASSERT(globalObject->scriptExecutionContext()->isDocument());
        }
    };
    
    JSC::Structure* getCachedDOMStructure(JSDOMGlobalObject*, const JSC::ClassInfo*);
    JSC::Structure* cacheDOMStructure(JSDOMGlobalObject*, JSC::Structure*, const JSC::ClassInfo*);

    inline JSDOMGlobalObject* deprecatedGlobalObjectForPrototype(JSC::ExecState* exec)
    {
        // FIXME: Callers to this function should be using the global object
        // from which the object is being created, instead of assuming the lexical one.
        // e.g. subframe.document.body should use the subframe's global object, not the lexical one.
        return JSC::jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
    }

    template<class WrapperClass> inline JSC::Structure* getDOMStructure(JSC::ExecState* exec, JSDOMGlobalObject* globalObject)
    {
        if (JSC::Structure* structure = getCachedDOMStructure(globalObject, &WrapperClass::s_info))
            return structure;
        return cacheDOMStructure(globalObject, WrapperClass::createStructure(exec->vm(), globalObject, WrapperClass::createPrototype(exec, globalObject)), &WrapperClass::s_info);
    }

    template<class WrapperClass> inline JSC::Structure* deprecatedGetDOMStructure(JSC::ExecState* exec)
    {
        // FIXME: This function is wrong.  It uses the wrong global object for creating the prototype structure.
        return getDOMStructure<WrapperClass>(exec, deprecatedGlobalObjectForPrototype(exec));
    }

    template<class WrapperClass> inline JSC::JSObject* getDOMPrototype(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
    {
        return JSC::jsCast<JSC::JSObject*>(asObject(getDOMStructure<WrapperClass>(exec, JSC::jsCast<JSDOMGlobalObject*>(globalObject))->storedPrototype()));
    }

    inline JSDOMWrapper* getInlineCachedWrapper(DOMWrapperWorld*, void*) { return 0; }
    inline bool setInlineCachedWrapper(DOMWrapperWorld*, void*, JSDOMWrapper*, JSC::WeakHandleOwner*, void*) { return false; }
    inline bool clearInlineCachedWrapper(DOMWrapperWorld*, void*, JSDOMWrapper*) { return false; }

    inline JSDOMWrapper* getInlineCachedWrapper(DOMWrapperWorld* world, ScriptWrappable* domObject)
    {
        if (!world->isNormal())
            return 0;
        return domObject->wrapper();
    }

    inline bool setInlineCachedWrapper(DOMWrapperWorld* world, ScriptWrappable* domObject, JSDOMWrapper* wrapper, JSC::WeakHandleOwner* wrapperOwner, void* context)
    {
        if (!world->isNormal())
            return false;
        domObject->setWrapper(*world->vm(), wrapper, wrapperOwner, context);
        return true;
    }

    inline bool clearInlineCachedWrapper(DOMWrapperWorld* world, ScriptWrappable* domObject, JSDOMWrapper* wrapper)
    {
        if (!world->isNormal())
            return false;
        domObject->clearWrapper(wrapper);
        return true;
    }

    template <typename DOMClass> inline JSDOMWrapper* getCachedWrapper(DOMWrapperWorld* world, DOMClass* domObject)
    {
        if (JSDOMWrapper* wrapper = getInlineCachedWrapper(world, domObject))
            return wrapper;
        return world->m_wrappers.get(domObject);
    }

    template <typename DOMClass> inline void cacheWrapper(DOMWrapperWorld* world, DOMClass* domObject, JSDOMWrapper* wrapper)
    {
        JSC::WeakHandleOwner* owner = wrapperOwner(world, domObject);
        void* context = wrapperContext(world, domObject);
        if (setInlineCachedWrapper(world, domObject, wrapper, owner, context))
            return;
        JSC::PassWeak<JSDOMWrapper> passWeak(wrapper, owner, context);
        weakAdd(world->m_wrappers, (void*)domObject, passWeak);
    }

    template <typename DOMClass> inline void uncacheWrapper(DOMWrapperWorld* world, DOMClass* domObject, JSDOMWrapper* wrapper)
    {
        if (clearInlineCachedWrapper(world, domObject, wrapper))
            return;
        weakRemove(world->m_wrappers, (void*)domObject, wrapper);
    }
    
    #define CREATE_DOM_WRAPPER(exec, globalObject, className, object) createWrapper<JS##className>(exec, globalObject, static_cast<className*>(object))
    template<class WrapperClass, class DOMClass> inline JSDOMWrapper* createWrapper(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* node)
    {
        ASSERT(node);
        ASSERT(!getCachedWrapper(currentWorld(exec), node));
        WrapperClass* wrapper = WrapperClass::create(getDOMStructure<WrapperClass>(exec, globalObject), globalObject, node);
        // FIXME: The entire function can be removed, once we fix caching.
        // This function is a one-off hack to make Nodes cache in the right global object.
        cacheWrapper(currentWorld(exec), node, wrapper);
        return wrapper;
    }

    template<class WrapperClass, class DOMClass> inline JSC::JSValue wrap(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* domObject)
    {
        if (!domObject)
            return JSC::jsNull();
        if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), domObject))
            return wrapper;
        return createWrapper<WrapperClass>(exec, globalObject, domObject);
    }

    template<class WrapperClass, class DOMClass> inline JSC::JSValue getExistingWrapper(JSC::ExecState* exec, DOMClass* domObject)
    {
        ASSERT(domObject);
        return getCachedWrapper(currentWorld(exec), domObject);
    }

    template<class WrapperClass, class DOMClass> inline JSC::JSValue createNewWrapper(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* domObject)
    {
        ASSERT(domObject);
        ASSERT(!getCachedWrapper(currentWorld(exec), domObject));
        return createWrapper<WrapperClass>(exec, globalObject, domObject);
    }

    inline JSC::JSValue argumentOrNull(JSC::ExecState* exec, unsigned index)
    {
        return index >= exec->argumentCount() ? JSC::JSValue() : exec->argument(index);
    }

    const JSC::HashTable* getHashTableForGlobalData(JSC::VM&, const JSC::HashTable* staticTable);

    void reportException(JSC::ExecState*, JSC::JSValue exception, CachedScript* = 0);
    void reportCurrentException(JSC::ExecState*);

    // Convert a DOM implementation exception code into a JavaScript exception in the execution state.
    void setDOMException(JSC::ExecState*, ExceptionCode);

    JSC::JSValue jsStringWithCache(JSC::ExecState*, const String&);
    JSC::JSValue jsString(JSC::ExecState*, const KURL&); // empty if the URL is null
    inline JSC::JSValue jsStringWithCache(JSC::ExecState* exec, const AtomicString& s)
    { 
        return jsStringWithCache(exec, s.string());
    }
        
    JSC::JSValue jsStringOrNull(JSC::ExecState*, const String&); // null if the string is null
    JSC::JSValue jsStringOrNull(JSC::ExecState*, const KURL&); // null if the URL is null

    JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const String&); // undefined if the string is null
    JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const KURL&); // undefined if the URL is null

    // See JavaScriptCore for explanation: Should be used for any string that is already owned by another
    // object, to let the engine know that collecting the JSString wrapper is unlikely to save memory.
    JSC::JSValue jsOwnedStringOrNull(JSC::ExecState*, const String&); 

    String propertyNameToString(JSC::PropertyName);

    AtomicString propertyNameToAtomicString(JSC::PropertyName);
    AtomicStringImpl* findAtomicString(JSC::PropertyName);

    String valueToStringWithNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null
    String valueToStringWithUndefinedOrNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null or undefined

    inline int32_t finiteInt32Value(JSC::JSValue value, JSC::ExecState* exec, bool& okay)
    {
        double number = value.toNumber(exec);
        okay = std::isfinite(number);
        return JSC::toInt32(number);
    }

    enum IntegerConversionConfiguration {
        NormalConversion,
        EnforceRange,
        // FIXME: Implement Clamp
    };

    int32_t toInt32EnforceRange(JSC::ExecState*, JSC::JSValue);
    uint32_t toUInt32EnforceRange(JSC::ExecState*, JSC::JSValue);

    int8_t toInt8(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);
    uint8_t toUInt8(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);

    /*
        Convert a value to an integer as per <http://www.w3.org/TR/WebIDL/>.
        The conversion fails if the value cannot be converted to a number or,
        if EnforceRange is specified, the value is outside the range of the
        destination integer type.
    */
    inline int32_t toInt32(JSC::ExecState* exec, JSC::JSValue value, IntegerConversionConfiguration configuration)
    {
        if (configuration == EnforceRange)
            return toInt32EnforceRange(exec, value);
        return value.toInt32(exec);
    }

    inline uint32_t toUInt32(JSC::ExecState* exec, JSC::JSValue value, IntegerConversionConfiguration configuration)
    {
        if (configuration == EnforceRange)
            return toUInt32EnforceRange(exec, value);
        return value.toUInt32(exec);
    }

    int64_t toInt64(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);
    uint64_t toUInt64(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);

    // Returns a Date instance for the specified value, or null if the value is NaN or infinity.
    JSC::JSValue jsDateOrNull(JSC::ExecState*, double);
    // NaN if the value can't be converted to a date.
    double valueToDate(JSC::ExecState*, JSC::JSValue);

    // Validates that the passed object is a sequence type per section 4.1.13 of the WebIDL spec.
    inline JSC::JSObject* toJSSequence(JSC::ExecState* exec, JSC::JSValue value, unsigned& length)
    {
        JSC::JSObject* object = value.getObject();
        if (!object) {
            throwTypeError(exec);
            return 0;
        }

        JSC::JSValue lengthValue = object->get(exec, exec->propertyNames().length);
        if (exec->hadException())
            return 0;

        if (lengthValue.isUndefinedOrNull()) {
            throwTypeError(exec);
            return 0;
        }

        length = lengthValue.toUInt32(exec);
        if (exec->hadException())
            return 0;

        return object;
    }

    template <typename T>
    inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<T> ptr)
    {
        return toJS(exec, globalObject, ptr.get());
    }

    template <class T>
    struct JSValueTraits {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const T& value)
        {
            return toJS(exec, globalObject, WTF::getPtr(value));
        }
    };

    template<>
    struct JSValueTraits<String> {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState* exec, JSDOMGlobalObject*, const String& value)
        {
            return jsStringWithCache(exec, value);
        }
    };

    template<>
    struct JSValueTraits<float> {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const float& value)
        {
            return JSC::jsNumber(value);
        }
    };

    template<>
    struct JSValueTraits<unsigned long> {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const unsigned long& value)
        {
            return JSC::jsNumber(value);
        }
    };

    template <typename T, size_t inlineCapacity>
    JSC::JSValue jsArray(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const Vector<T, inlineCapacity>& iterator)
    {
        JSC::MarkedArgumentBuffer list;
        typename Vector<T, inlineCapacity>::const_iterator end = iterator.end();        
        typedef JSValueTraits<T> TraitsType;

        for (typename Vector<T, inlineCapacity>::const_iterator iter = iterator.begin(); iter != end; ++iter)
            list.append(TraitsType::arrayJSValue(exec, globalObject, *iter));

        return JSC::constructArray(exec, 0, globalObject, list);
    }

    JSC::JSValue jsArray(JSC::ExecState*, JSDOMGlobalObject*, PassRefPtr<DOMStringList>);

    template<class T> struct NativeValueTraits;

    template<>
    struct NativeValueTraits<String> {
        static inline bool nativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, String& indexedValue)
        {
            indexedValue = jsValue.toString(exec)->value(exec);
            return true;
        }
    };

    template<>
    struct NativeValueTraits<unsigned> {
        static inline bool nativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, unsigned& indexedValue)
        {
            if (!jsValue.isNumber())
                return false;

            indexedValue = jsValue.toUInt32(exec);
            if (exec->hadException())
                return false;

            return true;
        }
    };

    template<>
    struct NativeValueTraits<float> {
        static inline bool nativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, float& indexedValue)
        {
            indexedValue = jsValue.toFloat(exec);
            return !exec->hadException();
        }
    };

    template <class T, class JST>
    Vector<RefPtr<T> > toRefPtrNativeArray(JSC::ExecState* exec, JSC::JSValue value, T* (*toT)(JSC::JSValue value))
    {
        if (!isJSArray(value))
            return Vector<RefPtr<T> >();

        Vector<RefPtr<T> > result;
        JSC::JSArray* array = asArray(value);
        for (size_t i = 0; i < array->length(); ++i) {
            JSC::JSValue element = array->getIndex(exec, i);
            if (element.inherits(&JST::s_info))
                result.append((*toT)(element));
            else {
                throwVMError(exec, createTypeError(exec, "Invalid Array element type"));
                return Vector<RefPtr<T> >();
            }
        }
        return result;
    }

    template <class T>
    Vector<T> toNativeArray(JSC::ExecState* exec, JSC::JSValue value)
    {
        unsigned length = 0;
        if (isJSArray(value)) {
            JSC::JSArray* array = asArray(value);
            length = array->length();
        } else
            toJSSequence(exec, value, length);

        JSC::JSObject* object = value.getObject();
        Vector<T> result;
        typedef NativeValueTraits<T> TraitsType;

        for (unsigned i = 0; i < length; ++i) {
            T indexValue;
            if (!TraitsType::nativeValue(exec, object->get(exec, i), indexValue))
                return Vector<T>();
            result.append(indexValue);
        }
        return result;
    }

    template <class T>
    Vector<T> toNativeArguments(JSC::ExecState* exec, size_t startIndex = 0)
    {
        size_t length = exec->argumentCount();
        ASSERT(startIndex <= length);

        Vector<T> result;
        typedef NativeValueTraits<T> TraitsType;

        for (size_t i = startIndex; i < length; ++i) {
            T indexValue;
            if (!TraitsType::nativeValue(exec, exec->argument(i), indexValue))
                return Vector<T>();
            result.append(indexValue);
        }
        return result;
    }

    bool shouldAllowAccessToNode(JSC::ExecState*, Node*);
    bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*);
    bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*, String& message);
    bool shouldAllowAccessToDOMWindow(BindingState*, DOMWindow*, String& message);

    void printErrorMessageForFrame(Frame*, const String& message);
    JSC::JSValue objectToStringFunctionGetter(JSC::ExecState*, JSC::JSValue, JSC::PropertyName);

    inline JSC::JSValue jsStringWithCache(JSC::ExecState* exec, const String& s)
    {
        StringImpl* stringImpl = s.impl();
        if (!stringImpl || !stringImpl->length())
            return jsEmptyString(exec);

        if (stringImpl->length() == 1) {
            UChar singleCharacter = (*stringImpl)[0u];
            if (singleCharacter <= JSC::maxSingleCharacterString) {
                JSC::VM* vm = &exec->vm();
                return vm->smallStrings.singleCharacterString(vm, static_cast<unsigned char>(singleCharacter));
            }
        }

        JSStringCache& stringCache = currentWorld(exec)->m_stringCache;
        JSStringCache::AddResult addResult = stringCache.add(stringImpl, nullptr);
        if (addResult.isNewEntry)
            addResult.iterator->value = JSC::jsString(exec, String(stringImpl));
        return JSC::JSValue(addResult.iterator->value.get());
    }

    inline String propertyNameToString(JSC::PropertyName propertyName)
    {
        return propertyName.publicName();
    }

    inline AtomicString propertyNameToAtomicString(JSC::PropertyName propertyName)
    {
        return AtomicString(propertyName.publicName());
    }

    template <class ThisImp>
    inline const JSC::HashEntry* getStaticValueSlotEntryWithoutCaching(JSC::ExecState* exec, JSC::PropertyName propertyName)
    {
        const JSC::HashEntry* entry = ThisImp::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (!entry) // not found, forward to parent
            return getStaticValueSlotEntryWithoutCaching<typename ThisImp::Base>(exec, propertyName);
        return entry;
    }

    template <>
    inline const JSC::HashEntry* getStaticValueSlotEntryWithoutCaching<JSDOMWrapper>(JSC::ExecState*, JSC::PropertyName)
    {
        return 0;
    }

    template<typename T>
    class HasMemoryCostMemberFunction {
        typedef char YesType;
        struct NoType {
            char padding[8];
        };

        struct BaseMixin {
            size_t memoryCost();
        };

        struct Base : public T, public BaseMixin { };

        template<typename U, U> struct
        TypeChecker { };

        template<typename U>
        static NoType dummy(U*, TypeChecker<size_t (BaseMixin::*)(), &U::memoryCost>* = 0);
        static YesType dummy(...);

    public:
        static const bool value = sizeof(dummy(static_cast<Base*>(0))) == sizeof(YesType);
    };
    template <typename T, bool hasReportCostFunction = HasMemoryCostMemberFunction<T>::value > struct ReportMemoryCost;
    template <typename T> struct ReportMemoryCost<T, true> {
        static void reportMemoryCost(JSC::ExecState* exec, T* impl)
        {
            exec->heap()->reportExtraMemoryCost(impl->memoryCost());
        }
    };
    template <typename T> struct ReportMemoryCost<T, false> {
        static void reportMemoryCost(JSC::ExecState*, T*)
        {
        }
    };

} // namespace WebCore

#endif // JSDOMBinding_h

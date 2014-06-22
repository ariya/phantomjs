/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SoftLinking_h
#define SoftLinking_h

#include <windows.h>
#include <wtf/Assertions.h>

#define SOFT_LINK_LIBRARY_HELPER(lib, suffix) \
    static HMODULE lib##Library() \
    { \
        static HMODULE library = LoadLibraryW(L###lib suffix); \
        return library; \
    }

#if OS(WINCE)
#define SOFT_LINK_GETPROCADDRESS GetProcAddressA
#else
#define SOFT_LINK_GETPROCADDRESS GetProcAddress
#endif

#define SOFT_LINK_LIBRARY(lib) SOFT_LINK_LIBRARY_HELPER(lib, L".dll")
#define SOFT_LINK_DEBUG_LIBRARY(lib) SOFT_LINK_LIBRARY_HELPER(lib, L"_debug.dll")

#define SOFT_LINK(library, functionName, resultType, callingConvention, parameterDeclarations, parameterNames) \
    static resultType callingConvention init##functionName parameterDeclarations; \
    static resultType (callingConvention*softLink##functionName) parameterDeclarations = init##functionName; \
    \
    static resultType callingConvention init##functionName parameterDeclarations \
    { \
        softLink##functionName = reinterpret_cast<resultType (callingConvention*) parameterDeclarations>(SOFT_LINK_GETPROCADDRESS(library##Library(), #functionName)); \
        ASSERT(softLink##functionName); \
        return softLink##functionName parameterNames; \
    }\
    \
    inline resultType functionName parameterDeclarations \
    {\
        return softLink##functionName parameterNames; \
    }

#define SOFT_LINK_OPTIONAL(library, functionName, resultType, callingConvention, parameterDeclarations) \
    typedef resultType (callingConvention *functionName##PtrType) parameterDeclarations; \
    static functionName##PtrType functionName##Ptr() \
    { \
        static functionName##PtrType ptr; \
        static bool initialized; \
        \
        if (initialized) \
            return ptr; \
        initialized = true; \
        \
        ptr = reinterpret_cast<functionName##PtrType>(SOFT_LINK_GETPROCADDRESS(library##Library(), #functionName)); \
        return ptr; \
    }\

#define SOFT_LINK_LOADED_LIBRARY(library, functionName, resultType, callingConvention, parameterDeclarations) \
    typedef resultType (callingConvention *functionName##PtrType) parameterDeclarations; \
    static functionName##PtrType functionName##Ptr() \
    { \
        static functionName##PtrType ptr; \
        static bool initialized; \
        \
        if (initialized) \
            return ptr; \
        initialized = true; \
        \
        static HINSTANCE libraryInstance = ::GetModuleHandle(L#library); \
        \
        ptr = reinterpret_cast<functionName##PtrType>(SOFT_LINK_GETPROCADDRESS(libraryInstance, #functionName)); \
        return ptr; \
    }\

/*
    In order to soft link against functions decorated with __declspec(dllimport), we prepend "softLink_" to the function names.
    If you use SOFT_LINK_DLL_IMPORT(), you will also need to #define the function name to account for this, e.g.:
    
    SOFT_LINK_DLL_IMPORT(myLibrary, myFunction, ...)
    #define myFunction softLink_myFunction
*/ 
#define SOFT_LINK_DLL_IMPORT(library, functionName, resultType, callingConvention, parameterDeclarations, parameterNames) \
    static resultType callingConvention init##functionName parameterDeclarations; \
    static resultType(callingConvention*softLink##functionName) parameterDeclarations = init##functionName; \
    \
    static resultType callingConvention init##functionName parameterDeclarations \
    { \
        softLink##functionName = reinterpret_cast<resultType (callingConvention*)parameterDeclarations>(SOFT_LINK_GETPROCADDRESS(library##Library(), #functionName)); \
        ASSERT(softLink##functionName); \
        return softLink##functionName parameterNames; \
    }\
    \
    inline resultType softLink_##functionName parameterDeclarations \
    {\
        return softLink##functionName parameterNames; \
    }

#define SOFT_LINK_DLL_IMPORT_OPTIONAL(library, functionName, resultType, callingConvention, parameterDeclarations) \
    typedef resultType (callingConvention *functionName##PtrType) parameterDeclarations; \
    static functionName##PtrType functionName##Ptr() \
    { \
        static functionName##PtrType ptr; \
        static bool initialized; \
        \
        if (initialized) \
            return ptr; \
        initialized = true; \
        \
        ptr = reinterpret_cast<resultType(callingConvention*)parameterDeclarations>(SOFT_LINK_GETPROCADDRESS(library##Library(), #functionName)); \
        return ptr; \
    }\

#define SOFT_LINK_DLL_IMPORT_OPTIONAL(library, functionName, resultType, callingConvention, parameterDeclarations) \
    typedef resultType (callingConvention *functionName##PtrType) parameterDeclarations; \
    static functionName##PtrType functionName##Ptr() \
    { \
        static functionName##PtrType ptr; \
        static bool initialized; \
        \
        if (initialized) \
            return ptr; \
        initialized = true; \
        \
        ptr = reinterpret_cast<resultType(callingConvention*)parameterDeclarations>(SOFT_LINK_GETPROCADDRESS(library##Library(), #functionName)); \
        return ptr; \
    }\

/*
    Variables exported by a DLL need to be accessed through a function.
    If you use SOFT_LINK_VARIABLE_DLL_IMPORT(), you will also need to #define the variable name to account for this, e.g.:
    
    SOFT_LINK_VARIABLE_DLL_IMPORT(myLibrary, myVar, int)
    #define myVar get_myVar()
*/ 
#define SOFT_LINK_VARIABLE_DLL_IMPORT(library, variableName, variableType) \
    static variableType get##variableName() \
    { \
        static variableType* ptr = reinterpret_cast<variableType*>(SOFT_LINK_GETPROCADDRESS(library##Library(), #variableName)); \
        ASSERT(ptr); \
        return *ptr; \
    }\

/*
    Note that this will only work for variable types for which a return value of 0 can signal an error.
 */
#define SOFT_LINK_VARIABLE_DLL_IMPORT_OPTIONAL(library, variableName, variableType) \
    static variableType get##variableName() \
    { \
        static variableType* ptr = reinterpret_cast<variableType*>(SOFT_LINK_GETPROCADDRESS(library##Library(), #variableName)); \
        if (!ptr) \
            return 0; \
        return *ptr; \
    }\

#endif // SoftLinking_h

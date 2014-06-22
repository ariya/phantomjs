//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _BASICTYPES_INCLUDED_
#define _BASICTYPES_INCLUDED_

//
// Precision qualifiers
//
enum TPrecision
{
    // These need to be kept sorted
    EbpUndefined,
    EbpLow,
    EbpMedium,
    EbpHigh
};

inline const char* getPrecisionString(TPrecision p)
{
    switch(p)
    {
    case EbpHigh:		return "highp";		break;
    case EbpMedium:		return "mediump";	break;
    case EbpLow:		return "lowp";		break;
    default:			return "mediump";   break;   // Safest fallback
    }
}

//
// Basic type.  Arrays, vectors, etc., are orthogonal to this.
//
enum TBasicType
{
    EbtVoid,
    EbtFloat,
    EbtInt,
    EbtBool,
    EbtGuardSamplerBegin,  // non type:  see implementation of IsSampler()
    EbtSampler2D,
    EbtSamplerCube,
    EbtSamplerExternalOES,  // Only valid if OES_EGL_image_external exists.
    EbtSampler2DRect,       // Only valid if GL_ARB_texture_rectangle exists.
    EbtGuardSamplerEnd,    // non type:  see implementation of IsSampler()
    EbtStruct,
    EbtAddress,            // should be deprecated??
    EbtInvariant          // used as a type when qualifying a previously declared variable as being invariant
};

inline const char* getBasicString(TBasicType t)
{
    switch (t)
    {
    case EbtVoid:              return "void";              break;
    case EbtFloat:             return "float";             break;
    case EbtInt:               return "int";               break;
    case EbtBool:              return "bool";              break;
    case EbtSampler2D:         return "sampler2D";         break;
    case EbtSamplerCube:       return "samplerCube";       break;
    case EbtSamplerExternalOES: return "samplerExternalOES"; break;
    case EbtSampler2DRect:     return "sampler2DRect";     break;
    case EbtStruct:            return "structure";         break;
    default:                   return "unknown type";
    }
}

inline bool IsSampler(TBasicType type)
{
    return type > EbtGuardSamplerBegin && type < EbtGuardSamplerEnd;
}

//
// Qualifiers and built-ins.  These are mainly used to see what can be read
// or written, and by the machine dependent translator to know which registers
// to allocate variables in.  Since built-ins tend to go to different registers
// than varying or uniform, it makes sense they are peers, not sub-classes.
//
enum TQualifier
{
    EvqTemporary,     // For temporaries (within a function), read/write
    EvqGlobal,        // For globals read/write
    EvqConst,         // User defined constants and non-output parameters in functions
    EvqAttribute,     // Readonly
    EvqVaryingIn,     // readonly, fragment shaders only
    EvqVaryingOut,    // vertex shaders only  read/write
    EvqInvariantVaryingIn,     // readonly, fragment shaders only
    EvqInvariantVaryingOut,    // vertex shaders only  read/write
    EvqUniform,       // Readonly, vertex and fragment

    // parameters
    EvqIn,
    EvqOut,
    EvqInOut,
    EvqConstReadOnly,

    // built-ins written by vertex shader
    EvqPosition,
    EvqPointSize,

    // built-ins read by fragment shader
    EvqFragCoord,
    EvqFrontFacing,
    EvqPointCoord,

    // built-ins written by fragment shader
    EvqFragColor,
    EvqFragData,
    EvqFragDepth,

    // end of list
    EvqLast
};

//
// This is just for debug print out, carried along with the definitions above.
//
inline const char* getQualifierString(TQualifier q)
{
    switch(q)
    {
    case EvqTemporary:      return "Temporary";      break;
    case EvqGlobal:         return "Global";         break;
    case EvqConst:          return "const";          break;
    case EvqConstReadOnly:  return "const";          break;
    case EvqAttribute:      return "attribute";      break;
    case EvqVaryingIn:      return "varying";        break;
    case EvqVaryingOut:     return "varying";        break;
    case EvqInvariantVaryingIn: return "invariant varying";	break;
    case EvqInvariantVaryingOut:return "invariant varying";	break;
    case EvqUniform:        return "uniform";        break;
    case EvqIn:             return "in";             break;
    case EvqOut:            return "out";            break;
    case EvqInOut:          return "inout";          break;
    case EvqPosition:       return "Position";       break;
    case EvqPointSize:      return "PointSize";      break;
    case EvqFragCoord:      return "FragCoord";      break;
    case EvqFrontFacing:    return "FrontFacing";    break;
    case EvqFragColor:      return "FragColor";      break;
    case EvqFragData:       return "FragData";      break;
    case EvqFragDepth:      return "FragDepth";     break;
    default:                return "unknown qualifier";
    }
}

#endif // _BASICTYPES_INCLUDED_

//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _BASICTYPES_INCLUDED_
#define _BASICTYPES_INCLUDED_

#include <assert.h>

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
    EbtUInt,
    EbtBool,
    EbtGVec4,              // non type: represents vec4, ivec4 and uvec4
    EbtGuardSamplerBegin,  // non type: see implementation of IsSampler()
    EbtSampler2D,
    EbtSampler3D,
    EbtSamplerCube,
    EbtSampler2DArray,
    EbtSamplerExternalOES,  // Only valid if OES_EGL_image_external exists.
    EbtSampler2DRect,       // Only valid if GL_ARB_texture_rectangle exists.
    EbtISampler2D,
    EbtISampler3D,
    EbtISamplerCube,
    EbtISampler2DArray,
    EbtUSampler2D,
    EbtUSampler3D,
    EbtUSamplerCube,
    EbtUSampler2DArray,
    EbtSampler2DShadow,
    EbtSamplerCubeShadow,
    EbtSampler2DArrayShadow,
    EbtGuardSamplerEnd,    // non type: see implementation of IsSampler()
    EbtGSampler2D,         // non type: represents sampler2D, isampler2D and usampler2D
    EbtGSampler3D,         // non type: represents sampler3D, isampler3D and usampler3D
    EbtGSamplerCube,       // non type: represents samplerCube, isamplerCube and usamplerCube
    EbtGSampler2DArray,    // non type: represents sampler2DArray, isampler2DArray and usampler2DArray
    EbtStruct,
    EbtInterfaceBlock,
    EbtAddress,            // should be deprecated??
};

const char* getBasicString(TBasicType t);

inline bool IsSampler(TBasicType type)
{
    return type > EbtGuardSamplerBegin && type < EbtGuardSamplerEnd;
}

inline bool IsIntegerSampler(TBasicType type)
{
    switch (type)
    {
      case EbtISampler2D:
      case EbtISampler3D:
      case EbtISamplerCube:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSampler3D:
      case EbtUSamplerCube:
      case EbtUSampler2DArray:
        return true;
      case EbtSampler2D:
      case EbtSampler3D:
      case EbtSamplerCube:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
      case EbtSampler2DArrayShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSampler2D(TBasicType type)
{
    switch (type)
    {
      case EbtSampler2D:
      case EbtISampler2D:
      case EbtUSampler2D:
      case EbtSampler2DArray:
      case EbtISampler2DArray:
      case EbtUSampler2DArray:
      case EbtSampler2DRect:
      case EbtSamplerExternalOES:
      case EbtSampler2DShadow:
      case EbtSampler2DArrayShadow:
        return true;
      case EbtSampler3D:
      case EbtISampler3D:
      case EbtUSampler3D:
      case EbtISamplerCube:
      case EbtUSamplerCube:
      case EbtSamplerCube:
      case EbtSamplerCubeShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSamplerCube(TBasicType type)
{
    switch (type)
    {
      case EbtSamplerCube:
      case EbtISamplerCube:
      case EbtUSamplerCube:
      case EbtSamplerCubeShadow:
        return true;
      case EbtSampler2D:
      case EbtSampler3D:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
      case EbtISampler2D:
      case EbtISampler3D:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSampler3D:
      case EbtUSampler2DArray:
      case EbtSampler2DShadow:
      case EbtSampler2DArrayShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSampler3D(TBasicType type)
{
    switch (type)
    {
      case EbtSampler3D:
      case EbtISampler3D:
      case EbtUSampler3D:
        return true;
      case EbtSampler2D:
      case EbtSamplerCube:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
      case EbtISampler2D:
      case EbtISamplerCube:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSamplerCube:
      case EbtUSampler2DArray:
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
      case EbtSampler2DArrayShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSamplerArray(TBasicType type)
{
    switch (type)
    {
      case EbtSampler2DArray:
      case EbtISampler2DArray:
      case EbtUSampler2DArray:
      case EbtSampler2DArrayShadow:
        return true;
      case EbtSampler2D:
      case EbtISampler2D:
      case EbtUSampler2D:
      case EbtSampler2DRect:
      case EbtSamplerExternalOES:
      case EbtSampler3D:
      case EbtISampler3D:
      case EbtUSampler3D:
      case EbtISamplerCube:
      case EbtUSamplerCube:
      case EbtSamplerCube:
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsShadowSampler(TBasicType type)
{
    switch (type)
    {
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
      case EbtSampler2DArrayShadow:
        return true;
      case EbtISampler2D:
      case EbtISampler3D:
      case EbtISamplerCube:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSampler3D:
      case EbtUSamplerCube:
      case EbtUSampler2DArray:
      case EbtSampler2D:
      case EbtSampler3D:
      case EbtSamplerCube:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool SupportsPrecision(TBasicType type)
{
    return type == EbtFloat || type == EbtInt || type == EbtUInt || IsSampler(type);
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
    EvqInternal,      // For internal use, not visible to the user
    EvqConst,         // User defined constants and non-output parameters in functions
    EvqAttribute,     // Readonly
    EvqVaryingIn,     // readonly, fragment shaders only
    EvqVaryingOut,    // vertex shaders only  read/write
    EvqInvariantVaryingIn,     // readonly, fragment shaders only
    EvqInvariantVaryingOut,    // vertex shaders only  read/write
    EvqUniform,       // Readonly, vertex and fragment

    EvqVertexIn,      // Vertex shader input
    EvqFragmentOut,   // Fragment shader output
    EvqVertexOut,     // Vertex shader output
    EvqFragmentIn,    // Fragment shader input

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

    // GLSL ES 3.0 vertex output and fragment input
    EvqSmooth,        // Incomplete qualifier, smooth is the default
    EvqFlat,          // Incomplete qualifier
    EvqSmoothOut = EvqSmooth,
    EvqFlatOut = EvqFlat,
    EvqCentroidOut,   // Implies smooth
    EvqSmoothIn,
    EvqFlatIn,
    EvqCentroidIn,    // Implies smooth

    // end of list
    EvqLast
};

enum TLayoutMatrixPacking
{
    EmpUnspecified,
    EmpRowMajor,
    EmpColumnMajor
};

enum TLayoutBlockStorage
{
    EbsUnspecified,
    EbsShared,
    EbsPacked,
    EbsStd140
};

struct TLayoutQualifier
{
    int location;
    TLayoutMatrixPacking matrixPacking;
    TLayoutBlockStorage blockStorage;

    static TLayoutQualifier create()
    {
        TLayoutQualifier layoutQualifier;

        layoutQualifier.location = -1;
        layoutQualifier.matrixPacking = EmpUnspecified;
        layoutQualifier.blockStorage = EbsUnspecified;

        return layoutQualifier;
    }

    bool isEmpty() const
    {
        return location == -1 && matrixPacking == EmpUnspecified && blockStorage == EbsUnspecified;
    }
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
    case EvqVertexIn:       return "in";             break;
    case EvqFragmentOut:    return "out";            break;
    case EvqVertexOut:      return "out";            break;
    case EvqFragmentIn:     return "in";             break;
    case EvqIn:             return "in";             break;
    case EvqOut:            return "out";            break;
    case EvqInOut:          return "inout";          break;
    case EvqPosition:       return "Position";       break;
    case EvqPointSize:      return "PointSize";      break;
    case EvqFragCoord:      return "FragCoord";      break;
    case EvqFrontFacing:    return "FrontFacing";    break;
    case EvqFragColor:      return "FragColor";      break;
    case EvqFragData:       return "FragData";       break;
    case EvqFragDepth:      return "FragDepth";      break;
    case EvqSmoothOut:      return "smooth out";     break;
    case EvqCentroidOut:    return "centroid out";   break;
    case EvqFlatOut:        return "flat out";       break;
    case EvqSmoothIn:       return "smooth in";      break;
    case EvqCentroidIn:     return "centroid in";    break;
    case EvqFlatIn:         return "flat in";        break;
    default:                return "unknown qualifier";
    }
}

inline const char* getMatrixPackingString(TLayoutMatrixPacking mpq)
{
    switch (mpq)
    {
    case EmpUnspecified:    return "mp_unspecified";
    case EmpRowMajor:       return "row_major";
    case EmpColumnMajor:    return "column_major";
    default:                return "unknown matrix packing";
    }
}

inline const char* getBlockStorageString(TLayoutBlockStorage bsq)
{
    switch (bsq)
    {
    case EbsUnspecified:    return "bs_unspecified";
    case EbsShared:         return "shared";
    case EbsPacked:         return "packed";
    case EbsStd140:         return "std140";
    default:                return "unknown block storage";
    }
}

inline const char* getInterpolationString(TQualifier q)
{
    switch(q)
    {
    case EvqSmoothOut:      return "smooth";   break;
    case EvqCentroidOut:    return "centroid"; break;
    case EvqFlatOut:        return "flat";     break;
    case EvqSmoothIn:       return "smooth";   break;
    case EvqCentroidIn:     return "centroid"; break;
    case EvqFlatIn:         return "flat";     break;
    default:                return "unknown interpolation";
    }
}

#endif // _BASICTYPES_INCLUDED_

//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UtilsHLSL.h:
//   Utility methods for GLSL to HLSL translation.
//

#ifndef TRANSLATOR_UTILSHLSL_H_
#define TRANSLATOR_UTILSHLSL_H_

#include <vector>
#include "compiler/translator/Types.h"

#include "angle_gl.h"

namespace sh
{

TString TextureString(const TType &type);
TString SamplerString(const TType &type);
// Prepends an underscore to avoid naming clashes
TString Decorate(const TString &string);
TString DecorateUniform(const TString &string, const TType &type);
TString DecorateField(const TString &string, const TStructure &structure);
TString DecoratePrivate(const TString &privateText);
TString TypeString(const TType &type);
TString StructNameString(const TStructure &structure);
TString QualifiedStructNameString(const TStructure &structure, bool useHLSLRowMajorPacking,
                                  bool useStd140Packing);
TString InterpolationString(TQualifier qualifier);
TString QualifierString(TQualifier qualifier);

}

#endif // TRANSLATOR_UTILSHLSL_H_

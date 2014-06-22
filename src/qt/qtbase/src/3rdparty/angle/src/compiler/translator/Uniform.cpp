//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/Uniform.h"

namespace sh
{

Uniform::Uniform(GLenum type, GLenum precision, const char *name, int arraySize, int registerIndex)
{
    this->type = type;
    this->precision = precision;
    this->name = name;
    this->arraySize = arraySize;
    this->registerIndex = registerIndex;
}

}

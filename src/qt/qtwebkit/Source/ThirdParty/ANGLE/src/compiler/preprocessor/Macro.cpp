//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Macro.h"

#include "Token.h"

namespace pp
{

bool Macro::equals(const Macro& other) const
{
    return (type == other.type) &&
           (name == other.name) &&
           (parameters == other.parameters) &&
           (replacements == other.replacements);
}

}  // namespace pp


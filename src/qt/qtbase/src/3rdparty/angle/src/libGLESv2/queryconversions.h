//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// queryconversions.h: Declaration of state query cast conversions

namespace gl
{

// The GL state query API types are: bool, int, uint, float, int64
template <typename QueryT>
void CastStateValues(Context *context, GLenum nativeType, GLenum pname,
                     unsigned int numParams, QueryT *outParams);

}

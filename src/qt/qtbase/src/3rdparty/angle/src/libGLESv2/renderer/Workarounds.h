//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angletypes.h: Workarounds for driver bugs and other issues.

#ifndef LIBGLESV2_RENDERER_WORKAROUNDS_H_
#define LIBGLESV2_RENDERER_WORKAROUNDS_H_

// TODO(jmadill,zmo,geofflang): make a workarounds library that can operate
// independent of ANGLE's renderer. Workarounds should also be accessible
// outside of the Renderer.

namespace rx
{

enum D3DWorkaroundType
{
    ANGLE_D3D_WORKAROUND_NONE,
    ANGLE_D3D_WORKAROUND_SKIP_OPTIMIZATION,
    ANGLE_D3D_WORKAROUND_MAX_OPTIMIZATION
};

struct Workarounds
{
    Workarounds()
        : mrtPerfWorkaround(false),
          setDataFasterThanImageUpload(false)
    {}

    bool mrtPerfWorkaround;
    bool setDataFasterThanImageUpload;
};

}

#endif // LIBGLESV2_RENDERER_WORKAROUNDS_H_

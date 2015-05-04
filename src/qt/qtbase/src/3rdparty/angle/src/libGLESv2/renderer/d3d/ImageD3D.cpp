//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.h: Implements the rx::Image class, an abstract base class for the
// renderer-specific classes which will define the interface to the underlying
// surfaces or resources.

#include "libGLESv2/renderer/d3d/ImageD3D.h"

namespace rx
{

ImageD3D::ImageD3D()
{
}

ImageD3D *ImageD3D::makeImageD3D(Image *img)
{
    ASSERT(HAS_DYNAMIC_TYPE(ImageD3D*, img));
    return static_cast<ImageD3D*>(img);
}

}

//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Error.h: Defines the egl::Error class which encapsulates an EGL error
// and optional error message.

#ifndef LIBEGL_ERROR_H_
#define LIBEGL_ERROR_H_

#include <EGL/egl.h>

#include <string>

namespace egl
{

class Error
{
  public:
    explicit Error(EGLint errorCode);
    Error(EGLint errorCode, const char *msg, ...);
    Error(const Error &other);
    Error &operator=(const Error &other);

    EGLint getCode() const { return mCode; }
    bool isError() const { return (mCode != EGL_SUCCESS); }

    const std::string &getMessage() const { return mMessage; }

  private:
    EGLint mCode;
    std::string mMessage;
};

}

#endif // LIBEGL_ERROR_H_

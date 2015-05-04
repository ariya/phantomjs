//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// pp_utils.h: Common preprocessor utilities

#ifndef COMPILER_PREPROCESSOR_PPUTILS_H_
#define COMPILER_PREPROCESSOR_PPUTILS_H_

// A macro to disallow the copy constructor and operator= functions
// This must be used in the private: declarations for a class.
#define PP_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &);               \
  void operator=(const TypeName &)

#endif // COMPILER_PREPROCESSOR_PPUTILS_H_

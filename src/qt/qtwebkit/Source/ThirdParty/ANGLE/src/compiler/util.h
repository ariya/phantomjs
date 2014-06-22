//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_UTIL_H
#define COMPILER_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// atof_dot is like atof but forcing C locale, i.e. forcing '.' as decimal point.
double atof_dot(const char *str);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // COMPILER_UTIL_H

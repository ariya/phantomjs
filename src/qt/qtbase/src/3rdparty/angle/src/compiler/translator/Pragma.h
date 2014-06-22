//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PRAGMA_H_
#define COMPILER_PRAGMA_H_

struct TPragma {
    // By default optimization is turned on and debug is turned off.
    TPragma() : optimize(true), debug(false) { }
    TPragma(bool o, bool d) : optimize(o), debug(d) { }

    bool optimize;
    bool debug;
};

#endif // COMPILER_PRAGMA_H_

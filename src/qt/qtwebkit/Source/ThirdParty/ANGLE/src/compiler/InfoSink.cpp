//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/InfoSink.h"

void TInfoSinkBase::prefix(TPrefixType p) {
    switch(p) {
        case EPrefixNone:
            break;
        case EPrefixWarning:
            sink.append("WARNING: ");
            break;
        case EPrefixError:
            sink.append("ERROR: ");
            break;
        case EPrefixInternalError:
            sink.append("INTERNAL ERROR: ");
            break;
        case EPrefixUnimplemented:
            sink.append("UNIMPLEMENTED: ");
            break;
        case EPrefixNote:
            sink.append("NOTE: ");
            break;
        default:
            sink.append("UNKOWN ERROR: ");
            break;
    }
}

void TInfoSinkBase::location(int file, int line) {
    TPersistStringStream stream;
    if (line)
        stream << file << ":" << line;
    else
        stream << file << ":? ";
    stream << ": ";

    sink.append(stream.str());
}

void TInfoSinkBase::location(const TSourceLoc& loc) {
    location(loc.first_file, loc.first_line);
}

void TInfoSinkBase::message(TPrefixType p, const TSourceLoc& loc, const char* m) {
    prefix(p);
    location(loc);
    sink.append(m);
    sink.append("\n");
}

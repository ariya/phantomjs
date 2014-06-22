//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _INFOSINK_INCLUDED_
#define _INFOSINK_INCLUDED_

#include <math.h>
#include "compiler/Common.h"

// Returns the fractional part of the given floating-point number.
inline float fractionalPart(float f) {
  float intPart = 0.0f;
  return modff(f, &intPart);
}

//
// TPrefixType is used to centralize how info log messages start.
// See below.
//
enum TPrefixType {
    EPrefixNone,
    EPrefixWarning,
    EPrefixError,
    EPrefixInternalError,
    EPrefixUnimplemented,
    EPrefixNote
};

//
// Encapsulate info logs for all objects that have them.
//
// The methods are a general set of tools for getting a variety of
// messages and types inserted into the log.
//
class TInfoSinkBase {
public:
    TInfoSinkBase() {}

    template <typename T>
    TInfoSinkBase& operator<<(const T& t) {
        TPersistStringStream stream;
        stream << t;
        sink.append(stream.str());
        return *this;
    }
    // Override << operator for specific types. It is faster to append strings
    // and characters directly to the sink.
    TInfoSinkBase& operator<<(char c) {
        sink.append(1, c);
        return *this;
    }
    TInfoSinkBase& operator<<(const char* str) {
        sink.append(str);
        return *this;
    }
    TInfoSinkBase& operator<<(const TPersistString& str) {
        sink.append(str);
        return *this;
    }
    TInfoSinkBase& operator<<(const TString& str) {
        sink.append(str.c_str());
        return *this;
    }
    // Make sure floats are written with correct precision.
    TInfoSinkBase& operator<<(float f) {
        // Make sure that at least one decimal point is written. If a number
        // does not have a fractional part, the default precision format does
        // not write the decimal portion which gets interpreted as integer by
        // the compiler.
        TPersistStringStream stream;
        if (fractionalPart(f) == 0.0f) {
            stream.precision(1);
            stream << std::showpoint << std::fixed << f;
        } else {
            stream.unsetf(std::ios::fixed);
            stream.unsetf(std::ios::scientific);
            stream.precision(8);
            stream << f;
        }
        sink.append(stream.str());
        return *this;
    }
    // Write boolean values as their names instead of integral value.
    TInfoSinkBase& operator<<(bool b) {
        const char* str = b ? "true" : "false";
        sink.append(str);
        return *this;
    }

    void erase() { sink.clear(); }
    int size() { return static_cast<int>(sink.size()); }

    const TPersistString& str() const { return sink; }
    const char* c_str() const { return sink.c_str(); }

    void prefix(TPrefixType p);
    void location(int file, int line);
    void location(const TSourceLoc& loc);
    void message(TPrefixType p, const TSourceLoc& loc, const char* m);

private:
    TPersistString sink;
};

class TInfoSink {
public:
    TInfoSinkBase info;
    TInfoSinkBase debug;
    TInfoSinkBase obj;
};

#endif // _INFOSINK_INCLUDED_

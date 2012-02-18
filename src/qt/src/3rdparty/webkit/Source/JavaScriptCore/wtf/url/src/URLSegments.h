// Copyright 2007, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef URLSegments_h
#define URLSegments_h

#include "URLComponent.h"

namespace WTF {

// A structure that holds the identified parts of an input URL. This structure
// does NOT store the URL itself. The caller will have to store the URL text
// and its corresponding Parsed structure separately.
class URLSegments {
public:
    // Identifies different components.
    enum ComponentType {
        Scheme,
        Username,
        Password,
        Host,
        Port,
        Path,
        Query,
        Fragment,
    };

    URLSegments() { }

    // Returns the length of the URL (the end of the last component).
    //
    // Note that for some invalid, non-canonical URLs, this may not be the length
    // of the string. For example "http://": the parsed structure will only
    // contain an entry for the four-character scheme, and it doesn't know about
    // the "://". For all other last-components, it will return the real length.
    int length() const;

    // Returns the number of characters before the given component if it exists,
    // or where the component would be if it did exist. This will return the
    // string length if the component would be appended to the end.
    //
    // Note that this can get a little funny for the port, query, and fragment
    // components which have a delimiter that is not counted as part of the
    // component. The |includeDelimiter| flag controls if you want this counted
    // as part of the component or not when the component exists.
    //
    // This example shows the difference between the two flags for two of these
    // delimited components that is present (the port and query) and one that
    // isn't (the reference). The components that this flag affects are marked
    // with a *.
    //                 0         1         2
    //                 012345678901234567890
    // Example input:  http://foo:80/?query
    //              include_delim=true,  ...=false  ("<-" indicates different)
    //      Scheme: 0                    0
    //    Username: 5                    5
    //    Password: 5                    5
    //        Host: 7                    7
    //       *Port: 10                   11 <-
    //        Path: 13                   13
    //      *Query: 14                   15 <-
    //        *Fragment: 20                   20
    //
    int charactersBefore(ComponentType, bool includeDelimiter) const;

    // Each component excludes the related delimiters and has a length of -1
    // if that component is absent but 0 if the component exists but is empty.
    URLComponent scheme;
    URLComponent username;
    URLComponent password;
    URLComponent host;
    URLComponent port;
    URLComponent path;
    URLComponent query;
    URLComponent fragment;
};

} // namespace WTF

#endif // URLSegments_h

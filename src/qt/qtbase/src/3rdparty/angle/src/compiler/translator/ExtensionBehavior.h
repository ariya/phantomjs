//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _EXTENSION_BEHAVIOR_INCLUDED_
#define _EXTENSION_BEHAVIOR_INCLUDED_

#include <map>
#include <string>

typedef enum
{
    EBhRequire,
    EBhEnable,
    EBhWarn,
    EBhDisable,
    EBhUndefined
} TBehavior;

inline const char* getBehaviorString(TBehavior b)
{
    switch(b)
    {
      case EBhRequire: return "require";
      case EBhEnable: return "enable";
      case EBhWarn: return "warn";
      case EBhDisable: return "disable";
      default: return NULL;
    }
}

// Mapping between extension name and behavior.
typedef std::map<std::string, TBehavior> TExtensionBehavior;

#endif // _EXTENSION_TABLE_INCLUDED_

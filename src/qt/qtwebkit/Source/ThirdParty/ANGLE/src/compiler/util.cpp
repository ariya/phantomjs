//
// Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <math.h>
#include <stdlib.h>

#include "util.h"

#ifdef _MSC_VER
    #include <locale.h>
#else
    #include <sstream>
#endif

double atof_dot(const char *str)
{
#ifdef _MSC_VER
    _locale_t l = _create_locale(LC_NUMERIC, "C");
    double result = _atof_l(str, l);
    _free_locale(l);
    return result;
#else
    double result;
    std::istringstream s(str);
    std::locale l("C");
    s.imbue(l);
    s >> result;
    return result;
#endif
}

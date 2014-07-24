/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

void func1()
{
}

void func2()
{
    return 123;
}

void func3() { return 123; }

void func4();

void func5()
{
    /* comment */
}

/*
void funcInsideComment()
{
}
*/

void func6()
{
}

#define MACRO 123 \
    456 \
    789

void func7()
{
}

#if 1 || 1 || \
    1 || 1 || 1
void func8()
{
}
#else
void func9()
{
}
#endif

void func10()
{
}

std::string str = "abcde"
"void funcInsideDoubleQuotedString()"
"{"
"}";

void func11()
{
}

std::string str = 'abcde'
'void funcInsideSingleQuotedString()'
'{'
'}';

void func12(int a)
{
}

void func13(int a, int b, int c)
{
}

void func14(int a, int b,
            int c, int d
            , int e, int f)
{
}

void func15
    (int a, int b)
{
}

void funcOverloaded()
{
}

void funcOverloaded(int a)
{
}

void funcOverloaded(float a)
{
}

void Class::func16()
{
}

void Class1::Class2::func17()
{
}

static void Class2::func18()
{
}

inline void Class2::func19()
{
}

const void Class2::func20()
{
}

Class1::Type Class2::func21()
{
}

inline static const Class1::Type Class2::func22()
{
}

template<class T> void func23(T t)
{
}

template<class T>
void func24(T t)
{
}

inline static Class1::Type Class2::func25()
{
}

class Class1 {
public:
    void func26();
};

void Class1::func26()
{
}

class Class2 {
    void func27()
    {
    }
};

class Class3 : public Class4, Class5, Class6 {
    void func28()
    {
    }
};

class Class7 {
    int operator+()
    {
        return 123;
    }
};

Class100::Class100()
{
}

Class101::~Class101()
{
}

Class102::Class102() :
    member(1), member(2)
{
}

Class103::Class103()
    : member(1), member(2)
{
}

struct Struct1 {
public:
    void func29();
};

void Struct1::func29()
{
}

struct Struct2 {
    void func30()
    {
    }
};

namespace NameSpace1 {

void func30()
{
}

}

namespace NameSpace1 {
namespace NameSpace2 {

void func31()
{
}

}
}

class Class104 {
    int a;
    int b;
    int c;
    int d;
};

class Class105 {
public:
    int a;
    int b;
private:
    int c;
    int d;
};

class Class106 {
    int a;
    int b;
    void func32()
    {
        int c;
        int d;
    }
    int e;
    int f;
    void func33()
    {
        int g;
        int h;
    }
    int i;
    int j;
};

namespace NameSpace3 {
int a;
int b;
namespace NameSpace4 {
int c;
int d;
};
int e;
int f;
};

namespace NameSpace5 {
int a;
int b;
namespace NameSpace6 {
int c;
int d;
class Class107 {
    int e;
    int f;
    void func34()
    {
        int g;
        int h;
    }
    int i;
    int j;
};
int k;
int ll;
};
int m;
int n;
};

class Class108 {
    int a;
    void func35()
    {
        int b;
        if (1) {
            int c;
            for (;;) {
                int d;
                int e;
            }
            int f;
        }
        int g;
    }
    int h;
};

int a[] = { };
int a[] = {
};
int a[] = { 1, 2, 3 };
int a[] = {
    1,
    2,
    3
};
int a[3] = { 1, 2, 3 };
int a[][3] = { {1, 2, 3}, {4, 5, 6} };
int a[2][3] = { {1, 2, 3}, {4, 5, 6} };
extern int a[];
char a[4] = "test";

namespace NameSpace7 {
int a[] = { };
int a[] = {
};
int a[] = { 1, 2, 3 };
int a[] = {
    1,
    2,
    3
};
int a[3] = { 1, 2, 3 };
int a[][3] = { {1, 2, 3}, {4, 5, 6} };
int a[2][3] = { {1, 2, 3}, {4, 5, 6} };
extern int a[];
char a[4] = "test";

namespace NameSpace8 {
int a[] = { };
int a[] = {
};
int a[] = { 1, 2, 3 };
int a[] = {
    1,
    2,
    3
};
int a[3] = { 1, 2, 3 };
int a[][3] = { {1, 2, 3}, {4, 5, 6} };
int a[2][3] = { {1, 2, 3}, {4, 5, 6} };
extern int a[];
char a[4] = "test";
};

class Class109 {
    int a[] = { };
    int a[] = {
    };
    int a[] = { 1, 2, 3 };
    int a[] = {
        1,
        2,
        3
    };
    int a[3] = { 1, 2, 3 };
    int a[][3] = { {1, 2, 3}, {4, 5, 6} };
    int a[2][3] = { {1, 2, 3}, {4, 5, 6} };
    extern int a[];
    char a[4] = "test";
};

};

//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// utilities.cpp: Conversion functions and other utility routines.

#include "common/utilities.h"
#include "common/mathutil.h"
#include "common/platform.h"

#include <set>

#if defined(ANGLE_ENABLE_WINDOWS_STORE)
#  include <wrl.h>
#  include <wrl/wrappers/corewrappers.h>
#  include <windows.applicationmodel.core.h>
#  include <windows.graphics.display.h>
#endif

namespace gl
{

int VariableComponentCount(GLenum type)
{
    return VariableRowCount(type) * VariableColumnCount(type);
}

GLenum VariableComponentType(GLenum type)
{
    switch(type)
    {
      case GL_BOOL:
      case GL_BOOL_VEC2:
      case GL_BOOL_VEC3:
      case GL_BOOL_VEC4:
        return GL_BOOL;
      case GL_FLOAT:
      case GL_FLOAT_VEC2:
      case GL_FLOAT_VEC3:
      case GL_FLOAT_VEC4:
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT4x2:
      case GL_FLOAT_MAT3x4:
      case GL_FLOAT_MAT4x3:
        return GL_FLOAT;
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D:
      case GL_INT_SAMPLER_3D:
      case GL_INT_SAMPLER_CUBE:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_CUBE_SHADOW:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
      case GL_INT_VEC2:
      case GL_INT_VEC3:
      case GL_INT_VEC4:
        return GL_INT;
      case GL_UNSIGNED_INT:
      case GL_UNSIGNED_INT_VEC2:
      case GL_UNSIGNED_INT_VEC3:
      case GL_UNSIGNED_INT_VEC4:
        return GL_UNSIGNED_INT;
      default:
        UNREACHABLE();
    }

    return GL_NONE;
}

size_t VariableComponentSize(GLenum type)
{
    switch(type)
    {
      case GL_BOOL:         return sizeof(GLint);
      case GL_FLOAT:        return sizeof(GLfloat);
      case GL_INT:          return sizeof(GLint);
      case GL_UNSIGNED_INT: return sizeof(GLuint);
      default:       UNREACHABLE();
    }

    return 0;
}

size_t VariableInternalSize(GLenum type)
{
    // Expanded to 4-element vectors
    return VariableComponentSize(VariableComponentType(type)) * VariableRowCount(type) * 4;
}

size_t VariableExternalSize(GLenum type)
{
    return VariableComponentSize(VariableComponentType(type)) * VariableComponentCount(type);
}

GLenum VariableBoolVectorType(GLenum type)
{
    switch (type)
    {
      case GL_FLOAT:
      case GL_INT:
      case GL_UNSIGNED_INT:
        return GL_BOOL;
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_UNSIGNED_INT_VEC2:
        return GL_BOOL_VEC2;
      case GL_FLOAT_VEC3:
      case GL_INT_VEC3:
      case GL_UNSIGNED_INT_VEC3:
        return GL_BOOL_VEC3;
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_UNSIGNED_INT_VEC4:
        return GL_BOOL_VEC4;

      default:
        UNREACHABLE();
        return GL_NONE;
    }
}

int VariableRowCount(GLenum type)
{
    switch (type)
    {
      case GL_NONE:
      case GL_STRUCT_ANGLEX:
        return 0;
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_UNSIGNED_INT_VEC2:
      case GL_BOOL_VEC3:
      case GL_FLOAT_VEC3:
      case GL_INT_VEC3:
      case GL_UNSIGNED_INT_VEC3:
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_UNSIGNED_INT_VEC4:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_EXTERNAL_OES:
      case GL_SAMPLER_2D_RECT_ARB:
      case GL_INT_SAMPLER_2D:
      case GL_INT_SAMPLER_3D:
      case GL_INT_SAMPLER_CUBE:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_CUBE_SHADOW:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return 1;
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_MAT4x2:
        return 2;
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT4x3:
        return 3;
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT3x4:
        return 4;
      default:
        UNREACHABLE();
    }

    return 0;
}

int VariableColumnCount(GLenum type)
{
    switch (type)
    {
      case GL_NONE:
      case GL_STRUCT_ANGLEX:
        return 0;
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D:
      case GL_INT_SAMPLER_3D:
      case GL_INT_SAMPLER_CUBE:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_EXTERNAL_OES:
      case GL_SAMPLER_2D_RECT_ARB:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_CUBE_SHADOW:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return 1;
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_UNSIGNED_INT_VEC2:
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT2x4:
        return 2;
      case GL_BOOL_VEC3:
      case GL_FLOAT_VEC3:
      case GL_INT_VEC3:
      case GL_UNSIGNED_INT_VEC3:
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_MAT3x4:
        return 3;
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_UNSIGNED_INT_VEC4:
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT4x2:
      case GL_FLOAT_MAT4x3:
        return 4;
      default:
        UNREACHABLE();
    }

    return 0;
}

bool IsSampler(GLenum type)
{
    switch (type)
    {
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D:
      case GL_INT_SAMPLER_3D:
      case GL_INT_SAMPLER_CUBE:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_CUBE_SHADOW:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return true;
    }

    return false;
}

bool IsMatrixType(GLenum type)
{
    return VariableRowCount(type) > 1;
}

GLenum TransposeMatrixType(GLenum type)
{
    if (!IsMatrixType(type))
    {
        return type;
    }

    switch (type)
    {
      case GL_FLOAT_MAT2:   return GL_FLOAT_MAT2;
      case GL_FLOAT_MAT3:   return GL_FLOAT_MAT3;
      case GL_FLOAT_MAT4:   return GL_FLOAT_MAT4;
      case GL_FLOAT_MAT2x3: return GL_FLOAT_MAT3x2;
      case GL_FLOAT_MAT3x2: return GL_FLOAT_MAT2x3;
      case GL_FLOAT_MAT2x4: return GL_FLOAT_MAT4x2;
      case GL_FLOAT_MAT4x2: return GL_FLOAT_MAT2x4;
      case GL_FLOAT_MAT3x4: return GL_FLOAT_MAT4x3;
      case GL_FLOAT_MAT4x3: return GL_FLOAT_MAT3x4;
      default: UNREACHABLE(); return GL_NONE;
    }
}

int MatrixRegisterCount(GLenum type, bool isRowMajorMatrix)
{
    ASSERT(IsMatrixType(type));
    return isRowMajorMatrix ? VariableRowCount(type) : VariableColumnCount(type);
}

int MatrixComponentCount(GLenum type, bool isRowMajorMatrix)
{
    ASSERT(IsMatrixType(type));
    return isRowMajorMatrix ? VariableColumnCount(type) : VariableRowCount(type);
}

int VariableRegisterCount(GLenum type)
{
    return IsMatrixType(type) ? VariableColumnCount(type) : 1;
}

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize)
{
    ASSERT(allocationSize <= bitsSize);

    unsigned int mask = std::numeric_limits<unsigned int>::max() >> (std::numeric_limits<unsigned int>::digits - allocationSize);

    for (unsigned int i = 0; i < bitsSize - allocationSize + 1; i++)
    {
        if ((*bits & mask) == 0)
        {
            *bits |= mask;
            return i;
        }

        mask <<= 1;
    }

    return -1;
}

bool IsCubemapTextureTarget(GLenum target)
{
    return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
}

bool IsTriangleMode(GLenum drawMode)
{
    switch (drawMode)
    {
      case GL_TRIANGLES:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLE_STRIP:
        return true;
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
        return false;
      default: UNREACHABLE();
    }

    return false;
}

// [OpenGL ES SL 3.00.4] Section 11 p. 120
// Vertex Outs/Fragment Ins packing priorities
int VariableSortOrder(GLenum type)
{
    switch (type)
    {
      // 1. Arrays of mat4 and mat4
      // Non-square matrices of type matCxR consume the same space as a square
      // matrix of type matN where N is the greater of C and R
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT3x4:
      case GL_FLOAT_MAT4x2:
      case GL_FLOAT_MAT4x3:
        return 0;

      // 2. Arrays of mat2 and mat2 (since they occupy full rows)
      case GL_FLOAT_MAT2:
        return 1;

      // 3. Arrays of vec4 and vec4
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_BOOL_VEC4:
      case GL_UNSIGNED_INT_VEC4:
        return 2;

      // 4. Arrays of mat3 and mat3
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT3x2:
        return 3;

      // 5. Arrays of vec3 and vec3
      case GL_FLOAT_VEC3:
      case GL_INT_VEC3:
      case GL_BOOL_VEC3:
      case GL_UNSIGNED_INT_VEC3:
        return 4;

      // 6. Arrays of vec2 and vec2
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_BOOL_VEC2:
      case GL_UNSIGNED_INT_VEC2:
        return 5;

      // 7. Single component types
      case GL_FLOAT:
      case GL_INT:
      case GL_BOOL:
      case GL_UNSIGNED_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_EXTERNAL_OES:
      case GL_SAMPLER_2D_RECT_ARB:
      case GL_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_3D:
      case GL_INT_SAMPLER_2D:
      case GL_INT_SAMPLER_3D:
      case GL_INT_SAMPLER_CUBE:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
      case GL_SAMPLER_CUBE_SHADOW:
        return 6;

      default:
        UNREACHABLE();
        return 0;
    }
}

}

#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
std::string getTempPath()
{
#ifdef ANGLE_PLATFORM_WINDOWS
    char path[MAX_PATH];
    DWORD pathLen = GetTempPathA(sizeof(path) / sizeof(path[0]), path);
    if (pathLen == 0)
    {
        UNREACHABLE();
        return std::string();
    }

    UINT unique = GetTempFileNameA(path, "sh", 0, path);
    if (unique == 0)
    {
        UNREACHABLE();
        return std::string();
    }

    return path;
#else
    UNIMPLEMENTED();
    return "";
#endif
}

void writeFile(const char* path, const void* content, size_t size)
{
    FILE* file = fopen(path, "w");
    if (!file)
    {
        UNREACHABLE();
        return;
    }

    fwrite(content, sizeof(char), size, file);
    fclose(file);
}
#endif // !ANGLE_ENABLE_WINDOWS_STORE

#if defined(ANGLE_ENABLE_WINDOWS_STORE) && _MSC_FULL_VER < 180031101

void Sleep(unsigned long dwMilliseconds)
{
    static HANDLE singletonEvent = nullptr;
    HANDLE sleepEvent = singletonEvent;
    if (!sleepEvent)
    {
        sleepEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

        if (!sleepEvent)
            return;

        HANDLE previousEvent = InterlockedCompareExchangePointerRelease(&singletonEvent, sleepEvent, nullptr);

        if (previousEvent)
        {
            // Back out if multiple threads try to demand create at the same time.
            CloseHandle(sleepEvent);
            sleepEvent = previousEvent;
        }
    }

    // Emulate sleep by waiting with timeout on an event that is never signalled.
    WaitForSingleObjectEx(sleepEvent, dwMilliseconds, false);
}

#endif // ANGLE_ENABLE_WINDOWS_STORE

//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.h: Defines the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#ifndef LIBGLESV2_PROGRAM_BINARY_H_
#define LIBGLESV2_PROGRAM_BINARY_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <vector>

#include "common/RefCountObject.h"
#include "angletypes.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/Uniform.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Constants.h"

namespace rx
{
class ShaderExecutable;
class Renderer;
struct TranslatedAttribute;
}

namespace gl
{
class FragmentShader;
class VertexShader;
class InfoLog;
class AttributeBindings;
struct Varying;

// Struct used for correlating uniforms/elements of uniform arrays to handles
struct UniformLocation
{
    UniformLocation()
    {
    }

    UniformLocation(const std::string &name, unsigned int element, unsigned int index);

    std::string name;
    unsigned int element;
    unsigned int index;
};

// This is the result of linking a program. It is the state that would be passed to ProgramBinary.
class ProgramBinary : public RefCountObject
{
  public:
    explicit ProgramBinary(rx::Renderer *renderer);
    ~ProgramBinary();

    rx::ShaderExecutable *getPixelExecutable();
    rx::ShaderExecutable *getVertexExecutable();
    rx::ShaderExecutable *getGeometryExecutable();

    GLuint getAttributeLocation(const char *name);
    int getSemanticIndex(int attributeIndex);

    GLint getSamplerMapping(SamplerType type, unsigned int samplerIndex);
    TextureType getSamplerTextureType(SamplerType type, unsigned int samplerIndex);
    GLint getUsedSamplerRange(SamplerType type);
    bool usesPointSize() const;
    bool usesPointSpriteEmulation() const;
    bool usesGeometryShader() const;

    GLint getUniformLocation(std::string name);
    bool setUniform1fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform2fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform3fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform4fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value);
    bool setUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value);
    bool setUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value);
    bool setUniform1iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform2iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform3iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform4iv(GLint location, GLsizei count, const GLint *v);

    bool getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params);
    bool getUniformiv(GLint location, GLsizei *bufSize, GLint *params);

    void dirtyAllUniforms();
    void applyUniforms();

    bool load(InfoLog &infoLog, const void *binary, GLsizei length);
    bool save(void* binary, GLsizei bufSize, GLsizei *length);
    GLint getLength();

    bool link(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader);
    void getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders);

    void getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
    GLint getActiveAttributeCount() const;
    GLint getActiveAttributeMaxLength() const;

    void getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
    GLint getActiveUniformCount() const;
    GLint getActiveUniformMaxLength() const;

    void validate(InfoLog &infoLog);
    bool validateSamplers(InfoLog *infoLog);
    bool isValidated() const;

    unsigned int getSerial() const;

    void sortAttributesByLayout(rx::TranslatedAttribute attributes[gl::MAX_VERTEX_ATTRIBS], int sortedSemanticIndices[MAX_VERTEX_ATTRIBS]) const;

    static std::string decorateAttribute(const std::string &name);    // Prepend an underscore

  private:
    DISALLOW_COPY_AND_ASSIGN(ProgramBinary);

    int packVaryings(InfoLog &infoLog, const Varying *packing[][4], FragmentShader *fragmentShader);
    bool linkVaryings(InfoLog &infoLog, int registers, const Varying *packing[][4],
                      std::string& pixelHLSL, std::string& vertexHLSL,
                      FragmentShader *fragmentShader, VertexShader *vertexShader);

    bool linkAttributes(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader);

    bool linkUniforms(InfoLog &infoLog, const sh::ActiveUniforms &vertexUniforms, const sh::ActiveUniforms &fragmentUniforms);
    bool defineUniform(GLenum shader, const sh::Uniform &constant, InfoLog &infoLog);
    
    std::string generateGeometryShaderHLSL(int registers, const Varying *packing[][4], FragmentShader *fragmentShader, VertexShader *vertexShader) const;
    std::string generatePointSpriteHLSL(int registers, const Varying *packing[][4], FragmentShader *fragmentShader, VertexShader *vertexShader) const;

    rx::Renderer *const mRenderer;

    rx::ShaderExecutable *mPixelExecutable;
    rx::ShaderExecutable *mVertexExecutable;
    rx::ShaderExecutable *mGeometryExecutable;

    Attribute mLinkedAttribute[MAX_VERTEX_ATTRIBS];
    int mSemanticIndex[MAX_VERTEX_ATTRIBS];

    struct Sampler
    {
        Sampler();

        bool active;
        GLint logicalTextureUnit;
        TextureType textureType;
    };

    Sampler mSamplersPS[MAX_TEXTURE_IMAGE_UNITS];
    Sampler mSamplersVS[IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];
    GLuint mUsedVertexSamplerRange;
    GLuint mUsedPixelSamplerRange;
    bool mUsesPointSize;

    UniformArray mUniforms;
    typedef std::vector<UniformLocation> UniformIndex;
    UniformIndex mUniformIndex;

    bool mValidated;

    const unsigned int mSerial;

    static unsigned int issueSerial();
    static unsigned int mCurrentSerial;
};
}

#endif   // LIBGLESV2_PROGRAM_BINARY_H_

//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramImpl.h: Defines the abstract rx::ProgramImpl class.

#ifndef LIBGLESV2_RENDERER_PROGRAMIMPL_H_
#define LIBGLESV2_RENDERER_PROGRAMIMPL_H_

#include "common/angleutils.h"
#include "libGLESv2/BinaryStream.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/renderer/Renderer.h"

#include <map>

namespace rx
{

class ProgramImpl
{
  public:
    ProgramImpl() { }
    virtual ~ProgramImpl();

    const std::vector<gl::LinkedUniform*> &getUniforms() const { return mUniforms; }
    const std::vector<gl::VariableLocation> &getUniformIndices() const { return mUniformIndex; }
    const std::vector<gl::UniformBlock*> &getUniformBlocks() const { return mUniformBlocks; }
    const std::vector<gl::LinkedVarying> &getTransformFeedbackLinkedVaryings() const { return mTransformFeedbackLinkedVaryings; }
    const sh::Attribute *getShaderAttributes() const { return mShaderAttributes; }

    std::vector<gl::LinkedUniform*> &getUniforms() { return mUniforms; }
    std::vector<gl::VariableLocation> &getUniformIndices() { return mUniformIndex; }
    std::vector<gl::UniformBlock*> &getUniformBlocks() { return mUniformBlocks; }
    std::vector<gl::LinkedVarying> &getTransformFeedbackLinkedVaryings() { return mTransformFeedbackLinkedVaryings; }
    sh::Attribute *getShaderAttributes() { return mShaderAttributes; }

    gl::LinkedUniform *getUniformByLocation(GLint location) const;
    gl::LinkedUniform *getUniformByName(const std::string &name) const;
    gl::UniformBlock *getUniformBlockByIndex(GLuint blockIndex) const;

    GLint getUniformLocation(std::string name);
    GLuint getUniformIndex(std::string name);
    GLuint getUniformBlockIndex(std::string name) const;

    virtual bool usesPointSize() const = 0;
    virtual int getShaderVersion() const = 0;
    virtual GLenum getTransformFeedbackBufferMode() const = 0;

    virtual GLenum getBinaryFormat() = 0;
    virtual gl::LinkResult load(gl::InfoLog &infoLog, gl::BinaryInputStream *stream) = 0;
    virtual gl::Error save(gl::BinaryOutputStream *stream) = 0;

    virtual gl::LinkResult link(const gl::Data &data, gl::InfoLog &infoLog,
                                gl::Shader *fragmentShader, gl::Shader *vertexShader,
                                const std::vector<std::string> &transformFeedbackVaryings,
                                GLenum transformFeedbackBufferMode,
                                int *registers, std::vector<gl::LinkedVarying> *linkedVaryings,
                                std::map<int, gl::VariableLocation> *outputVariables) = 0;

    virtual void setUniform1fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform2fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform3fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform4fv(GLint location, GLsizei count, const GLfloat *v) = 0;
    virtual void setUniform1iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void setUniform2iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void setUniform3iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void setUniform4iv(GLint location, GLsizei count, const GLint *v) = 0;
    virtual void setUniform1uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniform2uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniform3uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniform4uiv(GLint location, GLsizei count, const GLuint *v) = 0;
    virtual void setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;
    virtual void setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = 0;

    virtual void getUniformfv(GLint location, GLfloat *params) = 0;
    virtual void getUniformiv(GLint location, GLint *params) = 0;
    virtual void getUniformuiv(GLint location, GLuint *params) = 0;

    virtual void reset();

    // TODO: The following functions are possibly only applicable to D3D backends. The should be carefully evaluated to
    // determine if they can be removed from this interface.
    virtual GLint getSamplerMapping(gl::SamplerType type, unsigned int samplerIndex, const gl::Caps &caps) const = 0;
    virtual GLenum getSamplerTextureType(gl::SamplerType type, unsigned int samplerIndex) const = 0;
    virtual GLint getUsedSamplerRange(gl::SamplerType type) const = 0;
    virtual void updateSamplerMapping() = 0;
    virtual bool validateSamplers(gl::InfoLog *infoLog, const gl::Caps &caps) = 0;

    virtual gl::LinkResult compileProgramExecutables(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
                                                     int registers) = 0;

    virtual bool linkUniforms(gl::InfoLog &infoLog, const gl::Shader &vertexShader, const gl::Shader &fragmentShader,
                              const gl::Caps &caps) = 0;
    virtual bool defineUniformBlock(gl::InfoLog &infoLog, const gl::Shader &shader, const sh::InterfaceBlock &interfaceBlock,
                                    const gl::Caps &caps) = 0;

    virtual gl::Error applyUniforms() = 0;
    virtual gl::Error applyUniformBuffers(const std::vector<gl::Buffer*> boundBuffers, const gl::Caps &caps) = 0;
    virtual bool assignUniformBlockRegister(gl::InfoLog &infoLog, gl::UniformBlock *uniformBlock, GLenum shader,
                                            unsigned int registerIndex, const gl::Caps &caps) = 0;

  protected:
    DISALLOW_COPY_AND_ASSIGN(ProgramImpl);

    std::vector<gl::LinkedUniform*> mUniforms;
    std::vector<gl::VariableLocation> mUniformIndex;
    std::vector<gl::UniformBlock*> mUniformBlocks;
    std::vector<gl::LinkedVarying> mTransformFeedbackLinkedVaryings;

    sh::Attribute mShaderAttributes[gl::MAX_VERTEX_ATTRIBS];
};

}

#endif // LIBGLESV2_RENDERER_PROGRAMIMPL_H_

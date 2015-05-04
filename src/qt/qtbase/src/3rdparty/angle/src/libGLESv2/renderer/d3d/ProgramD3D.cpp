//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.cpp: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#include "libGLESv2/renderer/d3d/ProgramD3D.h"

#include "common/features.h"
#include "common/utilities.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/ShaderExecutable.h"
#include "libGLESv2/renderer/d3d/DynamicHLSL.h"
#include "libGLESv2/renderer/d3d/RendererD3D.h"
#include "libGLESv2/renderer/d3d/ShaderD3D.h"

namespace rx
{

namespace
{

GLenum GetTextureType(GLenum samplerType)
{
    switch (samplerType)
    {
      case GL_SAMPLER_2D:
      case GL_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_SAMPLER_2D_SHADOW:
        return GL_TEXTURE_2D;
      case GL_SAMPLER_3D:
      case GL_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
        return GL_TEXTURE_3D;
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_CUBE_SHADOW:
        return GL_TEXTURE_CUBE_MAP;
      case GL_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return GL_TEXTURE_CUBE_MAP;
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return GL_TEXTURE_2D_ARRAY;
      default: UNREACHABLE();
    }

    return GL_TEXTURE_2D;
}

void GetDefaultInputLayoutFromShader(const std::vector<sh::Attribute> &shaderAttributes, gl::VertexFormat inputLayout[gl::MAX_VERTEX_ATTRIBS])
{
    size_t layoutIndex = 0;
    for (size_t attributeIndex = 0; attributeIndex < shaderAttributes.size(); attributeIndex++)
    {
        ASSERT(layoutIndex < gl::MAX_VERTEX_ATTRIBS);

        const sh::Attribute &shaderAttr = shaderAttributes[attributeIndex];

        if (shaderAttr.type != GL_NONE)
        {
            GLenum transposedType = gl::TransposeMatrixType(shaderAttr.type);

            for (size_t rowIndex = 0; static_cast<int>(rowIndex) < gl::VariableRowCount(transposedType); rowIndex++, layoutIndex++)
            {
                gl::VertexFormat *defaultFormat = &inputLayout[layoutIndex];

                defaultFormat->mType = gl::VariableComponentType(transposedType);
                defaultFormat->mNormalized = false;
                defaultFormat->mPureInteger = (defaultFormat->mType != GL_FLOAT); // note: inputs can not be bool
                defaultFormat->mComponents = gl::VariableColumnCount(transposedType);
            }
        }
    }
}

std::vector<GLenum> GetDefaultOutputLayoutFromShader(const std::vector<PixelShaderOutputVariable> &shaderOutputVars)
{
    std::vector<GLenum> defaultPixelOutput(1);

    ASSERT(!shaderOutputVars.empty());
    defaultPixelOutput[0] = GL_COLOR_ATTACHMENT0 + shaderOutputVars[0].outputIndex;

    return defaultPixelOutput;
}

bool IsRowMajorLayout(const sh::InterfaceBlockField &var)
{
    return var.isRowMajorLayout;
}

bool IsRowMajorLayout(const sh::ShaderVariable &var)
{
    return false;
}

}

ProgramD3D::VertexExecutable::VertexExecutable(const gl::VertexFormat inputLayout[],
                                               const GLenum signature[],
                                               ShaderExecutable *shaderExecutable)
    : mShaderExecutable(shaderExecutable)
{
    for (size_t attributeIndex = 0; attributeIndex < gl::MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        mInputs[attributeIndex] = inputLayout[attributeIndex];
        mSignature[attributeIndex] = signature[attributeIndex];
    }
}

ProgramD3D::VertexExecutable::~VertexExecutable()
{
    SafeDelete(mShaderExecutable);
}

bool ProgramD3D::VertexExecutable::matchesSignature(const GLenum signature[]) const
{
    for (size_t attributeIndex = 0; attributeIndex < gl::MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (mSignature[attributeIndex] != signature[attributeIndex])
        {
            return false;
        }
    }

    return true;
}

ProgramD3D::PixelExecutable::PixelExecutable(const std::vector<GLenum> &outputSignature, ShaderExecutable *shaderExecutable)
    : mOutputSignature(outputSignature),
      mShaderExecutable(shaderExecutable)
{
}

ProgramD3D::PixelExecutable::~PixelExecutable()
{
    SafeDelete(mShaderExecutable);
}

ProgramD3D::Sampler::Sampler() : active(false), logicalTextureUnit(0), textureType(GL_TEXTURE_2D)
{
}

ProgramD3D::ProgramD3D(RendererD3D *renderer)
    : ProgramImpl(),
      mRenderer(renderer),
      mDynamicHLSL(NULL),
      mGeometryExecutable(NULL),
      mVertexWorkarounds(ANGLE_D3D_WORKAROUND_NONE),
      mPixelWorkarounds(ANGLE_D3D_WORKAROUND_NONE),
      mUsesPointSize(false),
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL),
      mUsedVertexSamplerRange(0),
      mUsedPixelSamplerRange(0),
      mDirtySamplerMapping(true),
      mShaderVersion(100)
{
    mDynamicHLSL = new DynamicHLSL(renderer);
}

ProgramD3D::~ProgramD3D()
{
    reset();
    SafeDelete(mDynamicHLSL);
}

ProgramD3D *ProgramD3D::makeProgramD3D(ProgramImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(ProgramD3D*, impl));
    return static_cast<ProgramD3D*>(impl);
}

const ProgramD3D *ProgramD3D::makeProgramD3D(const ProgramImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const ProgramD3D*, impl));
    return static_cast<const ProgramD3D*>(impl);
}

bool ProgramD3D::usesPointSpriteEmulation() const
{
    return mUsesPointSize && mRenderer->getMajorShaderModel() >= 4;
}

bool ProgramD3D::usesGeometryShader() const
{
    return usesPointSpriteEmulation();
}

GLint ProgramD3D::getSamplerMapping(gl::SamplerType type, unsigned int samplerIndex, const gl::Caps &caps) const
{
    GLint logicalTextureUnit = -1;

    switch (type)
    {
      case gl::SAMPLER_PIXEL:
        ASSERT(samplerIndex < caps.maxTextureImageUnits);
        if (samplerIndex < mSamplersPS.size() && mSamplersPS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersPS[samplerIndex].logicalTextureUnit;
        }
        break;
      case gl::SAMPLER_VERTEX:
        ASSERT(samplerIndex < caps.maxVertexTextureImageUnits);
        if (samplerIndex < mSamplersVS.size() && mSamplersVS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersVS[samplerIndex].logicalTextureUnit;
        }
        break;
      default: UNREACHABLE();
    }

    if (logicalTextureUnit >= 0 && logicalTextureUnit < static_cast<GLint>(caps.maxCombinedTextureImageUnits))
    {
        return logicalTextureUnit;
    }

    return -1;
}

// Returns the texture type for a given Direct3D 9 sampler type and
// index (0-15 for the pixel shader and 0-3 for the vertex shader).
GLenum ProgramD3D::getSamplerTextureType(gl::SamplerType type, unsigned int samplerIndex) const
{
    switch (type)
    {
      case gl::SAMPLER_PIXEL:
        ASSERT(samplerIndex < mSamplersPS.size());
        ASSERT(mSamplersPS[samplerIndex].active);
        return mSamplersPS[samplerIndex].textureType;
      case gl::SAMPLER_VERTEX:
        ASSERT(samplerIndex < mSamplersVS.size());
        ASSERT(mSamplersVS[samplerIndex].active);
        return mSamplersVS[samplerIndex].textureType;
      default: UNREACHABLE();
    }

    return GL_TEXTURE_2D;
}

GLint ProgramD3D::getUsedSamplerRange(gl::SamplerType type) const
{
    switch (type)
    {
      case gl::SAMPLER_PIXEL:
        return mUsedPixelSamplerRange;
      case gl::SAMPLER_VERTEX:
        return mUsedVertexSamplerRange;
      default:
        UNREACHABLE();
        return 0;
    }
}

void ProgramD3D::updateSamplerMapping()
{
    if (!mDirtySamplerMapping)
    {
        return;
    }

    mDirtySamplerMapping = false;

    // Retrieve sampler uniform values
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        gl::LinkedUniform *targetUniform = mUniforms[uniformIndex];

        if (targetUniform->dirty)
        {
            if (gl::IsSampler(targetUniform->type))
            {
                int count = targetUniform->elementCount();
                GLint (*v)[4] = reinterpret_cast<GLint(*)[4]>(targetUniform->data);

                if (targetUniform->isReferencedByFragmentShader())
                {
                    unsigned int firstIndex = targetUniform->psRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < mSamplersPS.size())
                        {
                            ASSERT(mSamplersPS[samplerIndex].active);
                            mSamplersPS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }

                if (targetUniform->isReferencedByVertexShader())
                {
                    unsigned int firstIndex = targetUniform->vsRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < mSamplersVS.size())
                        {
                            ASSERT(mSamplersVS[samplerIndex].active);
                            mSamplersVS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }
            }
        }
    }
}

bool ProgramD3D::validateSamplers(gl::InfoLog *infoLog, const gl::Caps &caps)
{
    // if any two active samplers in a program are of different types, but refer to the same
    // texture image unit, and this is the current program, then ValidateProgram will fail, and
    // DrawArrays and DrawElements will issue the INVALID_OPERATION error.
    updateSamplerMapping();

    std::vector<GLenum> textureUnitTypes(caps.maxCombinedTextureImageUnits, GL_NONE);

    for (unsigned int i = 0; i < mUsedPixelSamplerRange; ++i)
    {
        if (mSamplersPS[i].active)
        {
            unsigned int unit = mSamplersPS[i].logicalTextureUnit;

            if (unit >= textureUnitTypes.size())
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, textureUnitTypes.size());
                }

                return false;
            }

            if (textureUnitTypes[unit] != GL_NONE)
            {
                if (mSamplersPS[i].textureType != textureUnitTypes[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitTypes[unit] = mSamplersPS[i].textureType;
            }
        }
    }

    for (unsigned int i = 0; i < mUsedVertexSamplerRange; ++i)
    {
        if (mSamplersVS[i].active)
        {
            unsigned int unit = mSamplersVS[i].logicalTextureUnit;

            if (unit >= textureUnitTypes.size())
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, textureUnitTypes.size());
                }

                return false;
            }

            if (textureUnitTypes[unit] != GL_NONE)
            {
                if (mSamplersVS[i].textureType != textureUnitTypes[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitTypes[unit] = mSamplersVS[i].textureType;
            }
        }
    }

    return true;
}

gl::LinkResult ProgramD3D::load(gl::InfoLog &infoLog, gl::BinaryInputStream *stream)
{
    stream->readInt(&mShaderVersion);

    const unsigned int psSamplerCount = stream->readInt<unsigned int>();
    for (unsigned int i = 0; i < psSamplerCount; ++i)
    {
        Sampler sampler;
        stream->readBool(&sampler.active);
        stream->readInt(&sampler.logicalTextureUnit);
        stream->readInt(&sampler.textureType);
        mSamplersPS.push_back(sampler);
    }
    const unsigned int vsSamplerCount = stream->readInt<unsigned int>();
    for (unsigned int i = 0; i < vsSamplerCount; ++i)
    {
        Sampler sampler;
        stream->readBool(&sampler.active);
        stream->readInt(&sampler.logicalTextureUnit);
        stream->readInt(&sampler.textureType);
        mSamplersVS.push_back(sampler);
    }

    stream->readInt(&mUsedVertexSamplerRange);
    stream->readInt(&mUsedPixelSamplerRange);

    const unsigned int uniformCount = stream->readInt<unsigned int>();
    if (stream->error())
    {
        infoLog.append("Invalid program binary.");
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUniforms.resize(uniformCount);
    for (unsigned int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++)
    {
        GLenum type = stream->readInt<GLenum>();
        GLenum precision = stream->readInt<GLenum>();
        std::string name = stream->readString();
        unsigned int arraySize = stream->readInt<unsigned int>();
        int blockIndex = stream->readInt<int>();

        int offset = stream->readInt<int>();
        int arrayStride = stream->readInt<int>();
        int matrixStride = stream->readInt<int>();
        bool isRowMajorMatrix = stream->readBool();

        const sh::BlockMemberInfo blockInfo(offset, arrayStride, matrixStride, isRowMajorMatrix);

        gl::LinkedUniform *uniform = new gl::LinkedUniform(type, precision, name, arraySize, blockIndex, blockInfo);

        stream->readInt(&uniform->psRegisterIndex);
        stream->readInt(&uniform->vsRegisterIndex);
        stream->readInt(&uniform->registerCount);
        stream->readInt(&uniform->registerElement);

        mUniforms[uniformIndex] = uniform;
    }

    const unsigned int uniformIndexCount = stream->readInt<unsigned int>();
    if (stream->error())
    {
        infoLog.append("Invalid program binary.");
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUniformIndex.resize(uniformIndexCount);
    for (unsigned int uniformIndexIndex = 0; uniformIndexIndex < uniformIndexCount; uniformIndexIndex++)
    {
        stream->readString(&mUniformIndex[uniformIndexIndex].name);
        stream->readInt(&mUniformIndex[uniformIndexIndex].element);
        stream->readInt(&mUniformIndex[uniformIndexIndex].index);
    }

    unsigned int uniformBlockCount = stream->readInt<unsigned int>();
    if (stream->error())
    {
        infoLog.append("Invalid program binary.");
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUniformBlocks.resize(uniformBlockCount);
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < uniformBlockCount; ++uniformBlockIndex)
    {
        std::string name = stream->readString();
        unsigned int elementIndex = stream->readInt<unsigned int>();
        unsigned int dataSize = stream->readInt<unsigned int>();

        gl::UniformBlock *uniformBlock = new gl::UniformBlock(name, elementIndex, dataSize);

        stream->readInt(&uniformBlock->psRegisterIndex);
        stream->readInt(&uniformBlock->vsRegisterIndex);

        unsigned int numMembers = stream->readInt<unsigned int>();
        uniformBlock->memberUniformIndexes.resize(numMembers);
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < numMembers; blockMemberIndex++)
        {
            stream->readInt(&uniformBlock->memberUniformIndexes[blockMemberIndex]);
        }

        mUniformBlocks[uniformBlockIndex] = uniformBlock;
    }

    stream->readInt(&mTransformFeedbackBufferMode);
    const unsigned int transformFeedbackVaryingCount = stream->readInt<unsigned int>();
    mTransformFeedbackLinkedVaryings.resize(transformFeedbackVaryingCount);
    for (unsigned int varyingIndex = 0; varyingIndex < transformFeedbackVaryingCount; varyingIndex++)
    {
        gl::LinkedVarying &varying = mTransformFeedbackLinkedVaryings[varyingIndex];

        stream->readString(&varying.name);
        stream->readInt(&varying.type);
        stream->readInt(&varying.size);
        stream->readString(&varying.semanticName);
        stream->readInt(&varying.semanticIndex);
        stream->readInt(&varying.semanticIndexCount);
    }

    stream->readString(&mVertexHLSL);
    stream->readInt(&mVertexWorkarounds);
    stream->readString(&mPixelHLSL);
    stream->readInt(&mPixelWorkarounds);
    stream->readBool(&mUsesFragDepth);
    stream->readBool(&mUsesPointSize);

    const size_t pixelShaderKeySize = stream->readInt<unsigned int>();
    mPixelShaderKey.resize(pixelShaderKeySize);
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKeySize; pixelShaderKeyIndex++)
    {
        stream->readInt(&mPixelShaderKey[pixelShaderKeyIndex].type);
        stream->readString(&mPixelShaderKey[pixelShaderKeyIndex].name);
        stream->readString(&mPixelShaderKey[pixelShaderKeyIndex].source);
        stream->readInt(&mPixelShaderKey[pixelShaderKeyIndex].outputIndex);
    }

    const unsigned char* binary = reinterpret_cast<const unsigned char*>(stream->data());

    const unsigned int vertexShaderCount = stream->readInt<unsigned int>();
    for (unsigned int vertexShaderIndex = 0; vertexShaderIndex < vertexShaderCount; vertexShaderIndex++)
    {
        gl::VertexFormat inputLayout[gl::MAX_VERTEX_ATTRIBS];

        for (size_t inputIndex = 0; inputIndex < gl::MAX_VERTEX_ATTRIBS; inputIndex++)
        {
            gl::VertexFormat *vertexInput = &inputLayout[inputIndex];
            stream->readInt(&vertexInput->mType);
            stream->readInt(&vertexInput->mNormalized);
            stream->readInt(&vertexInput->mComponents);
            stream->readBool(&vertexInput->mPureInteger);
        }

        unsigned int vertexShaderSize = stream->readInt<unsigned int>();
        const unsigned char *vertexShaderFunction = binary + stream->offset();

        ShaderExecutable *shaderExecutable = NULL;
        gl::Error error = mRenderer->loadExecutable(vertexShaderFunction, vertexShaderSize,
                                                    SHADER_VERTEX,
                                                    mTransformFeedbackLinkedVaryings,
                                                    (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                    &shaderExecutable);
        if (error.isError())
        {
            return gl::LinkResult(false, error);
        }

        if (!shaderExecutable)
        {
            infoLog.append("Could not create vertex shader.");
            return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
        }

        // generated converted input layout
        GLenum signature[gl::MAX_VERTEX_ATTRIBS];
        getInputLayoutSignature(inputLayout, signature);

        // add new binary
        mVertexExecutables.push_back(new VertexExecutable(inputLayout, signature, shaderExecutable));

        stream->skip(vertexShaderSize);
    }

    const size_t pixelShaderCount = stream->readInt<unsigned int>();
    for (size_t pixelShaderIndex = 0; pixelShaderIndex < pixelShaderCount; pixelShaderIndex++)
    {
        const size_t outputCount = stream->readInt<unsigned int>();
        std::vector<GLenum> outputs(outputCount);
        for (size_t outputIndex = 0; outputIndex < outputCount; outputIndex++)
        {
            stream->readInt(&outputs[outputIndex]);
        }

        const size_t pixelShaderSize = stream->readInt<unsigned int>();
        const unsigned char *pixelShaderFunction = binary + stream->offset();
        ShaderExecutable *shaderExecutable = NULL;
        gl::Error error = mRenderer->loadExecutable(pixelShaderFunction, pixelShaderSize, SHADER_PIXEL,
                                                    mTransformFeedbackLinkedVaryings,
                                                    (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                    &shaderExecutable);
        if (error.isError())
        {
            return gl::LinkResult(false, error);
        }

        if (!shaderExecutable)
        {
            infoLog.append("Could not create pixel shader.");
            return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
        }

        // add new binary
        mPixelExecutables.push_back(new PixelExecutable(outputs, shaderExecutable));

        stream->skip(pixelShaderSize);
    }

    unsigned int geometryShaderSize = stream->readInt<unsigned int>();

    if (geometryShaderSize > 0)
    {
        const unsigned char *geometryShaderFunction = binary + stream->offset();
        gl::Error error = mRenderer->loadExecutable(geometryShaderFunction, geometryShaderSize, SHADER_GEOMETRY,
                                                    mTransformFeedbackLinkedVaryings,
                                                    (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                    &mGeometryExecutable);
        if (error.isError())
        {
            return gl::LinkResult(false, error);
        }

        if (!mGeometryExecutable)
        {
            infoLog.append("Could not create geometry shader.");
            return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
        }
        stream->skip(geometryShaderSize);
    }

    GUID binaryIdentifier = {0};
    stream->readBytes(reinterpret_cast<unsigned char*>(&binaryIdentifier), sizeof(GUID));

    GUID identifier = mRenderer->getAdapterIdentifier();
    if (memcmp(&identifier, &binaryIdentifier, sizeof(GUID)) != 0)
    {
        infoLog.append("Invalid program binary.");
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    initializeUniformStorage();

    return gl::LinkResult(true, gl::Error(GL_NO_ERROR));
}

gl::Error ProgramD3D::save(gl::BinaryOutputStream *stream)
{
    stream->writeInt(mShaderVersion);

    stream->writeInt(mSamplersPS.size());
    for (unsigned int i = 0; i < mSamplersPS.size(); ++i)
    {
        stream->writeInt(mSamplersPS[i].active);
        stream->writeInt(mSamplersPS[i].logicalTextureUnit);
        stream->writeInt(mSamplersPS[i].textureType);
    }

    stream->writeInt(mSamplersVS.size());
    for (unsigned int i = 0; i < mSamplersVS.size(); ++i)
    {
        stream->writeInt(mSamplersVS[i].active);
        stream->writeInt(mSamplersVS[i].logicalTextureUnit);
        stream->writeInt(mSamplersVS[i].textureType);
    }

    stream->writeInt(mUsedVertexSamplerRange);
    stream->writeInt(mUsedPixelSamplerRange);

    stream->writeInt(mUniforms.size());
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); ++uniformIndex)
    {
        const gl::LinkedUniform &uniform = *mUniforms[uniformIndex];

        stream->writeInt(uniform.type);
        stream->writeInt(uniform.precision);
        stream->writeString(uniform.name);
        stream->writeInt(uniform.arraySize);
        stream->writeInt(uniform.blockIndex);

        stream->writeInt(uniform.blockInfo.offset);
        stream->writeInt(uniform.blockInfo.arrayStride);
        stream->writeInt(uniform.blockInfo.matrixStride);
        stream->writeInt(uniform.blockInfo.isRowMajorMatrix);

        stream->writeInt(uniform.psRegisterIndex);
        stream->writeInt(uniform.vsRegisterIndex);
        stream->writeInt(uniform.registerCount);
        stream->writeInt(uniform.registerElement);
    }

    stream->writeInt(mUniformIndex.size());
    for (size_t i = 0; i < mUniformIndex.size(); ++i)
    {
        stream->writeString(mUniformIndex[i].name);
        stream->writeInt(mUniformIndex[i].element);
        stream->writeInt(mUniformIndex[i].index);
    }

    stream->writeInt(mUniformBlocks.size());
    for (size_t uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); ++uniformBlockIndex)
    {
        const gl::UniformBlock& uniformBlock = *mUniformBlocks[uniformBlockIndex];

        stream->writeString(uniformBlock.name);
        stream->writeInt(uniformBlock.elementIndex);
        stream->writeInt(uniformBlock.dataSize);

        stream->writeInt(uniformBlock.memberUniformIndexes.size());
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
        {
            stream->writeInt(uniformBlock.memberUniformIndexes[blockMemberIndex]);
        }

        stream->writeInt(uniformBlock.psRegisterIndex);
        stream->writeInt(uniformBlock.vsRegisterIndex);
    }

    stream->writeInt(mTransformFeedbackBufferMode);
    stream->writeInt(mTransformFeedbackLinkedVaryings.size());
    for (size_t i = 0; i < mTransformFeedbackLinkedVaryings.size(); i++)
    {
        const gl::LinkedVarying &varying = mTransformFeedbackLinkedVaryings[i];

        stream->writeString(varying.name);
        stream->writeInt(varying.type);
        stream->writeInt(varying.size);
        stream->writeString(varying.semanticName);
        stream->writeInt(varying.semanticIndex);
        stream->writeInt(varying.semanticIndexCount);
    }

    stream->writeString(mVertexHLSL);
    stream->writeInt(mVertexWorkarounds);
    stream->writeString(mPixelHLSL);
    stream->writeInt(mPixelWorkarounds);
    stream->writeInt(mUsesFragDepth);
    stream->writeInt(mUsesPointSize);

    const std::vector<PixelShaderOutputVariable> &pixelShaderKey = mPixelShaderKey;
    stream->writeInt(pixelShaderKey.size());
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKey.size(); pixelShaderKeyIndex++)
    {
        const PixelShaderOutputVariable &variable = pixelShaderKey[pixelShaderKeyIndex];
        stream->writeInt(variable.type);
        stream->writeString(variable.name);
        stream->writeString(variable.source);
        stream->writeInt(variable.outputIndex);
    }

    stream->writeInt(mVertexExecutables.size());
    for (size_t vertexExecutableIndex = 0; vertexExecutableIndex < mVertexExecutables.size(); vertexExecutableIndex++)
    {
        VertexExecutable *vertexExecutable = mVertexExecutables[vertexExecutableIndex];

        for (size_t inputIndex = 0; inputIndex < gl::MAX_VERTEX_ATTRIBS; inputIndex++)
        {
            const gl::VertexFormat &vertexInput = vertexExecutable->inputs()[inputIndex];
            stream->writeInt(vertexInput.mType);
            stream->writeInt(vertexInput.mNormalized);
            stream->writeInt(vertexInput.mComponents);
            stream->writeInt(vertexInput.mPureInteger);
        }

        size_t vertexShaderSize = vertexExecutable->shaderExecutable()->getLength();
        stream->writeInt(vertexShaderSize);

        const uint8_t *vertexBlob = vertexExecutable->shaderExecutable()->getFunction();
        stream->writeBytes(vertexBlob, vertexShaderSize);
    }

    stream->writeInt(mPixelExecutables.size());
    for (size_t pixelExecutableIndex = 0; pixelExecutableIndex < mPixelExecutables.size(); pixelExecutableIndex++)
    {
        PixelExecutable *pixelExecutable = mPixelExecutables[pixelExecutableIndex];

        const std::vector<GLenum> outputs = pixelExecutable->outputSignature();
        stream->writeInt(outputs.size());
        for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
        {
            stream->writeInt(outputs[outputIndex]);
        }

        size_t pixelShaderSize = pixelExecutable->shaderExecutable()->getLength();
        stream->writeInt(pixelShaderSize);

        const uint8_t *pixelBlob = pixelExecutable->shaderExecutable()->getFunction();
        stream->writeBytes(pixelBlob, pixelShaderSize);
    }

    size_t geometryShaderSize = (mGeometryExecutable != NULL) ? mGeometryExecutable->getLength() : 0;
    stream->writeInt(geometryShaderSize);

    if (mGeometryExecutable != NULL && geometryShaderSize > 0)
    {
        const uint8_t *geometryBlob = mGeometryExecutable->getFunction();
        stream->writeBytes(geometryBlob, geometryShaderSize);
    }

    GUID binaryIdentifier = mRenderer->getAdapterIdentifier();
    stream->writeBytes(reinterpret_cast<unsigned char*>(&binaryIdentifier), sizeof(GUID));

    return gl::Error(GL_NO_ERROR);
}

gl::Error ProgramD3D::getPixelExecutableForFramebuffer(const gl::Framebuffer *fbo, ShaderExecutable **outExecutable)
{
    std::vector<GLenum> outputs;

    const gl::ColorbufferInfo &colorbuffers = fbo->getColorbuffersForRender(mRenderer->getWorkarounds());

    for (size_t colorAttachment = 0; colorAttachment < colorbuffers.size(); ++colorAttachment)
    {
        const gl::FramebufferAttachment *colorbuffer = colorbuffers[colorAttachment];

        if (colorbuffer)
        {
            outputs.push_back(colorbuffer->getBinding() == GL_BACK ? GL_COLOR_ATTACHMENT0 : colorbuffer->getBinding());
        }
        else
        {
            outputs.push_back(GL_NONE);
        }
    }

    return getPixelExecutableForOutputLayout(outputs, outExecutable);
}

gl::Error ProgramD3D::getPixelExecutableForOutputLayout(const std::vector<GLenum> &outputSignature, ShaderExecutable **outExectuable)
{
    for (size_t executableIndex = 0; executableIndex < mPixelExecutables.size(); executableIndex++)
    {
        if (mPixelExecutables[executableIndex]->matchesSignature(outputSignature))
        {
            *outExectuable = mPixelExecutables[executableIndex]->shaderExecutable();
            return gl::Error(GL_NO_ERROR);
        }
    }

    std::string finalPixelHLSL = mDynamicHLSL->generatePixelShaderForOutputSignature(mPixelHLSL, mPixelShaderKey, mUsesFragDepth,
                                                                                     outputSignature);

    // Generate new pixel executable
    gl::InfoLog tempInfoLog;
    ShaderExecutable *pixelExecutable = NULL;
    gl::Error error = mRenderer->compileToExecutable(tempInfoLog, finalPixelHLSL, SHADER_PIXEL,
                                                     mTransformFeedbackLinkedVaryings,
                                                     (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                     mPixelWorkarounds, &pixelExecutable);
    if (error.isError())
    {
        return error;
    }

    if (!pixelExecutable)
    {
        std::vector<char> tempCharBuffer(tempInfoLog.getLength() + 3);
        tempInfoLog.getLog(tempInfoLog.getLength(), NULL, &tempCharBuffer[0]);
        ERR("Error compiling dynamic pixel executable:\n%s\n", &tempCharBuffer[0]);
    }
    else
    {
        mPixelExecutables.push_back(new PixelExecutable(outputSignature, pixelExecutable));
    }

    *outExectuable = pixelExecutable;
    return gl::Error(GL_NO_ERROR);
}

gl::Error ProgramD3D::getVertexExecutableForInputLayout(const gl::VertexFormat inputLayout[gl::MAX_VERTEX_ATTRIBS], ShaderExecutable **outExectuable)
{
    GLenum signature[gl::MAX_VERTEX_ATTRIBS];
    getInputLayoutSignature(inputLayout, signature);

    for (size_t executableIndex = 0; executableIndex < mVertexExecutables.size(); executableIndex++)
    {
        if (mVertexExecutables[executableIndex]->matchesSignature(signature))
        {
            *outExectuable = mVertexExecutables[executableIndex]->shaderExecutable();
            return gl::Error(GL_NO_ERROR);
        }
    }

    // Generate new dynamic layout with attribute conversions
    std::string finalVertexHLSL = mDynamicHLSL->generateVertexShaderForInputLayout(mVertexHLSL, inputLayout, mShaderAttributes);

    // Generate new vertex executable
    gl::InfoLog tempInfoLog;
    ShaderExecutable *vertexExecutable = NULL;
    gl::Error error = mRenderer->compileToExecutable(tempInfoLog, finalVertexHLSL, SHADER_VERTEX,
                                                     mTransformFeedbackLinkedVaryings,
                                                     (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                                     mVertexWorkarounds, &vertexExecutable);
    if (error.isError())
    {
        return error;
    }

    if (!vertexExecutable)
    {
        std::vector<char> tempCharBuffer(tempInfoLog.getLength()+3);
        tempInfoLog.getLog(tempInfoLog.getLength(), NULL, &tempCharBuffer[0]);
        ERR("Error compiling dynamic vertex executable:\n%s\n", &tempCharBuffer[0]);
    }
    else
    {
        mVertexExecutables.push_back(new VertexExecutable(inputLayout, signature, vertexExecutable));
    }

    *outExectuable = vertexExecutable;
    return gl::Error(GL_NO_ERROR);
}

gl::LinkResult ProgramD3D::compileProgramExecutables(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
                                                     int registers)
{
    ShaderD3D *vertexShaderD3D = ShaderD3D::makeShaderD3D(vertexShader->getImplementation());
    ShaderD3D *fragmentShaderD3D = ShaderD3D::makeShaderD3D(fragmentShader->getImplementation());

    gl::VertexFormat defaultInputLayout[gl::MAX_VERTEX_ATTRIBS];
    GetDefaultInputLayoutFromShader(vertexShader->getActiveAttributes(), defaultInputLayout);
    ShaderExecutable *defaultVertexExecutable = NULL;
    gl::Error error = getVertexExecutableForInputLayout(defaultInputLayout, &defaultVertexExecutable);
    if (error.isError())
    {
        return gl::LinkResult(false, error);
    }

    std::vector<GLenum> defaultPixelOutput = GetDefaultOutputLayoutFromShader(getPixelShaderKey());
    ShaderExecutable *defaultPixelExecutable = NULL;
    error = getPixelExecutableForOutputLayout(defaultPixelOutput, &defaultPixelExecutable);
    if (error.isError())
    {
        return gl::LinkResult(false, error);
    }

    if (usesGeometryShader())
    {
        std::string geometryHLSL = mDynamicHLSL->generateGeometryShaderHLSL(registers, fragmentShaderD3D, vertexShaderD3D);


        error = mRenderer->compileToExecutable(infoLog, geometryHLSL, SHADER_GEOMETRY, mTransformFeedbackLinkedVaryings,
                                               (mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS),
                                               ANGLE_D3D_WORKAROUND_NONE, &mGeometryExecutable);
        if (error.isError())
        {
            return gl::LinkResult(false, error);
        }
    }

#if ANGLE_SHADER_DEBUG_INFO == ANGLE_ENABLED
    if (usesGeometryShader() && mGeometryExecutable)
    {
        // Geometry shaders are currently only used internally, so there is no corresponding shader object at the interface level
        // For now the geometry shader debug info is pre-pended to the vertex shader, this is a bit of a clutch
        vertexShaderD3D->appendDebugInfo("// GEOMETRY SHADER BEGIN\n\n");
        vertexShaderD3D->appendDebugInfo(mGeometryExecutable->getDebugInfo());
        vertexShaderD3D->appendDebugInfo("\nGEOMETRY SHADER END\n\n\n");
    }

    if (defaultVertexExecutable)
    {
        vertexShaderD3D->appendDebugInfo(defaultVertexExecutable->getDebugInfo());
    }

    if (defaultPixelExecutable)
    {
        fragmentShaderD3D->appendDebugInfo(defaultPixelExecutable->getDebugInfo());
    }
#endif

    bool linkSuccess = (defaultVertexExecutable && defaultPixelExecutable && (!usesGeometryShader() || mGeometryExecutable));
    return gl::LinkResult(linkSuccess, gl::Error(GL_NO_ERROR));
}

gl::LinkResult ProgramD3D::link(const gl::Data &data, gl::InfoLog &infoLog,
                                gl::Shader *fragmentShader, gl::Shader *vertexShader,
                                const std::vector<std::string> &transformFeedbackVaryings,
                                GLenum transformFeedbackBufferMode,
                                int *registers, std::vector<gl::LinkedVarying> *linkedVaryings,
                                std::map<int, gl::VariableLocation> *outputVariables)
{
    ShaderD3D *vertexShaderD3D = ShaderD3D::makeShaderD3D(vertexShader->getImplementation());
    ShaderD3D *fragmentShaderD3D = ShaderD3D::makeShaderD3D(fragmentShader->getImplementation());

    mSamplersPS.resize(data.caps->maxTextureImageUnits);
    mSamplersVS.resize(data.caps->maxVertexTextureImageUnits);

    mTransformFeedbackBufferMode = transformFeedbackBufferMode;

    mPixelHLSL = fragmentShaderD3D->getTranslatedSource();
    mPixelWorkarounds = fragmentShaderD3D->getD3DWorkarounds();

    mVertexHLSL = vertexShaderD3D->getTranslatedSource();
    mVertexWorkarounds = vertexShaderD3D->getD3DWorkarounds();
    mShaderVersion = vertexShaderD3D->getShaderVersion();

    // Map the varyings to the register file
    VaryingPacking packing = { NULL };
    *registers = mDynamicHLSL->packVaryings(infoLog, packing, fragmentShaderD3D, vertexShaderD3D, transformFeedbackVaryings);

    if (*registers < 0)
    {
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    if (!gl::ProgramBinary::linkVaryings(infoLog, fragmentShader, vertexShader))
    {
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    if (!mDynamicHLSL->generateShaderLinkHLSL(data, infoLog, *registers, packing, mPixelHLSL, mVertexHLSL,
                                              fragmentShaderD3D, vertexShaderD3D, transformFeedbackVaryings,
                                              linkedVaryings, outputVariables, &mPixelShaderKey, &mUsesFragDepth))
    {
        return gl::LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUsesPointSize = vertexShaderD3D->usesPointSize();

    return gl::LinkResult(true, gl::Error(GL_NO_ERROR));
}

void ProgramD3D::getInputLayoutSignature(const gl::VertexFormat inputLayout[], GLenum signature[]) const
{
    mDynamicHLSL->getInputLayoutSignature(inputLayout, signature);
}

void ProgramD3D::initializeUniformStorage()
{
    // Compute total default block size
    unsigned int vertexRegisters = 0;
    unsigned int fragmentRegisters = 0;
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const gl::LinkedUniform &uniform = *mUniforms[uniformIndex];

        if (!gl::IsSampler(uniform.type))
        {
            if (uniform.isReferencedByVertexShader())
            {
                vertexRegisters = std::max(vertexRegisters, uniform.vsRegisterIndex + uniform.registerCount);
            }
            if (uniform.isReferencedByFragmentShader())
            {
                fragmentRegisters = std::max(fragmentRegisters, uniform.psRegisterIndex + uniform.registerCount);
            }
        }
    }

    mVertexUniformStorage = mRenderer->createUniformStorage(vertexRegisters * 16u);
    mFragmentUniformStorage = mRenderer->createUniformStorage(fragmentRegisters * 16u);
}

gl::Error ProgramD3D::applyUniforms()
{
    updateSamplerMapping();

    gl::Error error = mRenderer->applyUniforms(*this, mUniforms);
    if (error.isError())
    {
        return error;
    }

    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        mUniforms[uniformIndex]->dirty = false;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error ProgramD3D::applyUniformBuffers(const std::vector<gl::Buffer*> boundBuffers, const gl::Caps &caps)
{
    ASSERT(boundBuffers.size() == mUniformBlocks.size());

    const gl::Buffer *vertexUniformBuffers[gl::IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS] = {NULL};
    const gl::Buffer *fragmentUniformBuffers[gl::IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS] = {NULL};

    const unsigned int reservedBuffersInVS = mRenderer->getReservedVertexUniformBuffers();
    const unsigned int reservedBuffersInFS = mRenderer->getReservedFragmentUniformBuffers();

    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); uniformBlockIndex++)
    {
        gl::UniformBlock *uniformBlock = mUniformBlocks[uniformBlockIndex];
        gl::Buffer *uniformBuffer = boundBuffers[uniformBlockIndex];

        ASSERT(uniformBlock && uniformBuffer);

        if (uniformBuffer->getSize() < uniformBlock->dataSize)
        {
            // undefined behaviour
            return gl::Error(GL_INVALID_OPERATION, "It is undefined behaviour to use a uniform buffer that is too small.");
        }

        // Unnecessary to apply an unreferenced standard or shared UBO
        if (!uniformBlock->isReferencedByVertexShader() && !uniformBlock->isReferencedByFragmentShader())
        {
            continue;
        }

        if (uniformBlock->isReferencedByVertexShader())
        {
            unsigned int registerIndex = uniformBlock->vsRegisterIndex - reservedBuffersInVS;
            ASSERT(vertexUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < caps.maxVertexUniformBlocks);
            vertexUniformBuffers[registerIndex] = uniformBuffer;
        }

        if (uniformBlock->isReferencedByFragmentShader())
        {
            unsigned int registerIndex = uniformBlock->psRegisterIndex - reservedBuffersInFS;
            ASSERT(fragmentUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < caps.maxFragmentUniformBlocks);
            fragmentUniformBuffers[registerIndex] = uniformBuffer;
        }
    }

    return mRenderer->setUniformBuffers(vertexUniformBuffers, fragmentUniformBuffers);
}

bool ProgramD3D::assignUniformBlockRegister(gl::InfoLog &infoLog, gl::UniformBlock *uniformBlock, GLenum shader,
                                            unsigned int registerIndex, const gl::Caps &caps)
{
    if (shader == GL_VERTEX_SHADER)
    {
        uniformBlock->vsRegisterIndex = registerIndex;
        if (registerIndex - mRenderer->getReservedVertexUniformBuffers() >= caps.maxVertexUniformBlocks)
        {
            infoLog.append("Vertex shader uniform block count exceed GL_MAX_VERTEX_UNIFORM_BLOCKS (%u)", caps.maxVertexUniformBlocks);
            return false;
        }
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        uniformBlock->psRegisterIndex = registerIndex;
        if (registerIndex - mRenderer->getReservedFragmentUniformBuffers() >= caps.maxFragmentUniformBlocks)
        {
            infoLog.append("Fragment shader uniform block count exceed GL_MAX_FRAGMENT_UNIFORM_BLOCKS (%u)", caps.maxFragmentUniformBlocks);
            return false;
        }
    }
    else UNREACHABLE();

    return true;
}

void ProgramD3D::dirtyAllUniforms()
{
    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        mUniforms[index]->dirty = true;
    }
}

void ProgramD3D::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    setUniform(location, count, v, GL_FLOAT);
}

void ProgramD3D::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC2);
}

void ProgramD3D::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC3);
}

void ProgramD3D::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC4);
}

void ProgramD3D::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 2>(location, count, transpose, value, GL_FLOAT_MAT2);
}

void ProgramD3D::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 3>(location, count, transpose, value, GL_FLOAT_MAT3);
}

void ProgramD3D::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 4>(location, count, transpose, value, GL_FLOAT_MAT4);
}

void ProgramD3D::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 3>(location, count, transpose, value, GL_FLOAT_MAT2x3);
}

void ProgramD3D::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 2>(location, count, transpose, value, GL_FLOAT_MAT3x2);
}

void ProgramD3D::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 4>(location, count, transpose, value, GL_FLOAT_MAT2x4);
}

void ProgramD3D::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 2>(location, count, transpose, value, GL_FLOAT_MAT4x2);
}

void ProgramD3D::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 4>(location, count, transpose, value, GL_FLOAT_MAT3x4);
}

void ProgramD3D::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 3>(location, count, transpose, value, GL_FLOAT_MAT4x3);
}

void ProgramD3D::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT);
}

void ProgramD3D::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC2);
}

void ProgramD3D::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC3);
}

void ProgramD3D::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC4);
}

void ProgramD3D::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT);
}

void ProgramD3D::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC2);
}

void ProgramD3D::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC3);
}

void ProgramD3D::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC4);
}

void ProgramD3D::getUniformfv(GLint location, GLfloat *params)
{
    getUniformv(location, params, GL_FLOAT);
}

void ProgramD3D::getUniformiv(GLint location, GLint *params)
{
    getUniformv(location, params, GL_INT);
}

void ProgramD3D::getUniformuiv(GLint location, GLuint *params)
{
    getUniformv(location, params, GL_UNSIGNED_INT);
}

bool ProgramD3D::linkUniforms(gl::InfoLog &infoLog, const gl::Shader &vertexShader, const gl::Shader &fragmentShader,
                              const gl::Caps &caps)
{
    const ShaderD3D *vertexShaderD3D = ShaderD3D::makeShaderD3D(vertexShader.getImplementation());
    const ShaderD3D *fragmentShaderD3D = ShaderD3D::makeShaderD3D(fragmentShader.getImplementation());

    const std::vector<sh::Uniform> &vertexUniforms = vertexShader.getUniforms();
    const std::vector<sh::Uniform> &fragmentUniforms = fragmentShader.getUniforms();

    // Check that uniforms defined in the vertex and fragment shaders are identical
    typedef std::map<std::string, const sh::Uniform*> UniformMap;
    UniformMap linkedUniforms;

    for (unsigned int vertexUniformIndex = 0; vertexUniformIndex < vertexUniforms.size(); vertexUniformIndex++)
    {
        const sh::Uniform &vertexUniform = vertexUniforms[vertexUniformIndex];
        linkedUniforms[vertexUniform.name] = &vertexUniform;
    }

    for (unsigned int fragmentUniformIndex = 0; fragmentUniformIndex < fragmentUniforms.size(); fragmentUniformIndex++)
    {
        const sh::Uniform &fragmentUniform = fragmentUniforms[fragmentUniformIndex];
        UniformMap::const_iterator entry = linkedUniforms.find(fragmentUniform.name);
        if (entry != linkedUniforms.end())
        {
            const sh::Uniform &vertexUniform = *entry->second;
            const std::string &uniformName = "uniform '" + vertexUniform.name + "'";
            if (!gl::ProgramBinary::linkValidateUniforms(infoLog, uniformName, vertexUniform, fragmentUniform))
            {
                return false;
            }
        }
    }

    for (unsigned int uniformIndex = 0; uniformIndex < vertexUniforms.size(); uniformIndex++)
    {
        const sh::Uniform &uniform = vertexUniforms[uniformIndex];

        if (uniform.staticUse)
        {
            defineUniformBase(GL_VERTEX_SHADER, uniform, vertexShaderD3D->getUniformRegister(uniform.name));
        }
    }

    for (unsigned int uniformIndex = 0; uniformIndex < fragmentUniforms.size(); uniformIndex++)
    {
        const sh::Uniform &uniform = fragmentUniforms[uniformIndex];

        if (uniform.staticUse)
        {
            defineUniformBase(GL_FRAGMENT_SHADER, uniform, fragmentShaderD3D->getUniformRegister(uniform.name));
        }
    }

    if (!indexUniforms(infoLog, caps))
    {
        return false;
    }

    initializeUniformStorage();

    // special case for gl_DepthRange, the only built-in uniform (also a struct)
    if (vertexShaderD3D->usesDepthRange() || fragmentShaderD3D->usesDepthRange())
    {
        const sh::BlockMemberInfo &defaultInfo = sh::BlockMemberInfo::getDefaultBlockInfo();

        mUniforms.push_back(new gl::LinkedUniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.near", 0, -1, defaultInfo));
        mUniforms.push_back(new gl::LinkedUniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.far", 0, -1, defaultInfo));
        mUniforms.push_back(new gl::LinkedUniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.diff", 0, -1, defaultInfo));
    }

    return true;
}

void ProgramD3D::defineUniformBase(GLenum shader, const sh::Uniform &uniform, unsigned int uniformRegister)
{
    ShShaderOutput outputType = ShaderD3D::getCompilerOutputType(shader);
    sh::HLSLBlockEncoder encoder(sh::HLSLBlockEncoder::GetStrategyFor(outputType));
    encoder.skipRegisters(uniformRegister);

    defineUniform(shader, uniform, uniform.name, &encoder);
}

void ProgramD3D::defineUniform(GLenum shader, const sh::ShaderVariable &uniform,
                                  const std::string &fullName, sh::HLSLBlockEncoder *encoder)
{
    if (uniform.isStruct())
    {
        for (unsigned int elementIndex = 0; elementIndex < uniform.elementCount(); elementIndex++)
        {
            const std::string &elementString = (uniform.isArray() ? ArrayString(elementIndex) : "");

            encoder->enterAggregateType();

            for (size_t fieldIndex = 0; fieldIndex < uniform.fields.size(); fieldIndex++)
            {
                const sh::ShaderVariable &field = uniform.fields[fieldIndex];
                const std::string &fieldFullName = (fullName + elementString + "." + field.name);

                defineUniform(shader, field, fieldFullName, encoder);
            }

            encoder->exitAggregateType();
        }
    }
    else // Not a struct
    {
        // Arrays are treated as aggregate types
        if (uniform.isArray())
        {
            encoder->enterAggregateType();
        }

        gl::LinkedUniform *linkedUniform = getUniformByName(fullName);

        if (!linkedUniform)
        {
            linkedUniform = new gl::LinkedUniform(uniform.type, uniform.precision, fullName, uniform.arraySize,
                                              -1, sh::BlockMemberInfo::getDefaultBlockInfo());
            ASSERT(linkedUniform);
            linkedUniform->registerElement = encoder->getCurrentElement();
            mUniforms.push_back(linkedUniform);
        }

        ASSERT(linkedUniform->registerElement == encoder->getCurrentElement());

        if (shader == GL_FRAGMENT_SHADER)
        {
            linkedUniform->psRegisterIndex = encoder->getCurrentRegister();
        }
        else if (shader == GL_VERTEX_SHADER)
        {
            linkedUniform->vsRegisterIndex = encoder->getCurrentRegister();
        }
        else UNREACHABLE();

        // Advance the uniform offset, to track registers allocation for structs
        encoder->encodeType(uniform.type, uniform.arraySize, false);

        // Arrays are treated as aggregate types
        if (uniform.isArray())
        {
            encoder->exitAggregateType();
        }
    }
}

template <typename T>
static inline void SetIfDirty(T *dest, const T& source, bool *dirtyFlag)
{
    ASSERT(dest != NULL);
    ASSERT(dirtyFlag != NULL);

    *dirtyFlag = *dirtyFlag || (memcmp(dest, &source, sizeof(T)) != 0);
    *dest = source;
}

template <typename T>
void ProgramD3D::setUniform(GLint location, GLsizei count, const T* v, GLenum targetUniformType)
{
    const int components = gl::VariableComponentCount(targetUniformType);
    const GLenum targetBoolType = gl::VariableBoolVectorType(targetUniformType);

    gl::LinkedUniform *targetUniform = getUniformByLocation(location);

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == targetUniformType)
    {
        T *target = reinterpret_cast<T*>(targetUniform->data) + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            T *dest = target + (i * 4);
            const T *source = v + (i * components);

            for (int c = 0; c < components; c++)
            {
                SetIfDirty(dest + c, source[c], &targetUniform->dirty);
            }
            for (int c = components; c < 4; c++)
            {
                SetIfDirty(dest + c, T(0), &targetUniform->dirty);
            }
        }
    }
    else if (targetUniform->type == targetBoolType)
    {
        GLint *boolParams = reinterpret_cast<GLint*>(targetUniform->data) + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            GLint *dest = boolParams + (i * 4);
            const T *source = v + (i * components);

            for (int c = 0; c < components; c++)
            {
                SetIfDirty(dest + c, (source[c] == static_cast<T>(0)) ? GL_FALSE : GL_TRUE, &targetUniform->dirty);
            }
            for (int c = components; c < 4; c++)
            {
                SetIfDirty(dest + c, GL_FALSE, &targetUniform->dirty);
            }
        }
    }
    else if (gl::IsSampler(targetUniform->type))
    {
        ASSERT(targetUniformType == GL_INT);

        GLint *target = reinterpret_cast<GLint*>(targetUniform->data) + mUniformIndex[location].element * 4;

        bool wasDirty = targetUniform->dirty;

        for (int i = 0; i < count; i++)
        {
            GLint *dest = target + (i * 4);
            const GLint *source = reinterpret_cast<const GLint*>(v) + (i * components);

            SetIfDirty(dest + 0, source[0], &targetUniform->dirty);
            SetIfDirty(dest + 1, 0, &targetUniform->dirty);
            SetIfDirty(dest + 2, 0, &targetUniform->dirty);
            SetIfDirty(dest + 3, 0, &targetUniform->dirty);
        }

        if (!wasDirty && targetUniform->dirty)
        {
            mDirtySamplerMapping = true;
        }
    }
    else UNREACHABLE();
}

template<typename T>
bool transposeMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    bool dirty = false;
    int copyWidth = std::min(targetHeight, srcWidth);
    int copyHeight = std::min(targetWidth, srcHeight);

    for (int x = 0; x < copyWidth; x++)
    {
        for (int y = 0; y < copyHeight; y++)
        {
            SetIfDirty(target + (x * targetWidth + y), static_cast<T>(value[y * srcWidth + x]), &dirty);
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyWidth; y++)
    {
        for (int x = copyHeight; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }
    // clear unfilled bottom.
    for (int y = copyWidth; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }

    return dirty;
}

template<typename T>
bool expandMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    bool dirty = false;
    int copyWidth = std::min(targetWidth, srcWidth);
    int copyHeight = std::min(targetHeight, srcHeight);

    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = 0; x < copyWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(value[y * srcWidth + x]), &dirty);
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = copyWidth; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }
    // clear unfilled bottom.
    for (int y = copyHeight; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }

    return dirty;
}

template <int cols, int rows>
void ProgramD3D::setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum targetUniformType)
{
    gl::LinkedUniform *targetUniform = getUniformByLocation(location);

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    const unsigned int targetMatrixStride = (4 * rows);
    GLfloat *target = (GLfloat*)(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * targetMatrixStride);

    for (int i = 0; i < count; i++)
    {
        // Internally store matrices as transposed versions to accomodate HLSL matrix indexing
        if (transpose == GL_FALSE)
        {
            targetUniform->dirty = transposeMatrix<GLfloat>(target, value, 4, rows, rows, cols) || targetUniform->dirty;
        }
        else
        {
            targetUniform->dirty = expandMatrix<GLfloat>(target, value, 4, rows, cols, rows) || targetUniform->dirty;
        }
        target += targetMatrixStride;
        value += cols * rows;
    }
}

template <typename T>
void ProgramD3D::getUniformv(GLint location, T *params, GLenum uniformType)
{
    gl::LinkedUniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (gl::IsMatrixType(targetUniform->type))
    {
        const int rows = gl::VariableRowCount(targetUniform->type);
        const int cols = gl::VariableColumnCount(targetUniform->type);
        transposeMatrix(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4 * rows, rows, cols, 4, rows);
    }
    else if (uniformType == gl::VariableComponentType(targetUniform->type))
    {
        unsigned int size = gl::VariableComponentCount(targetUniform->type);
        memcpy(params, targetUniform->data + mUniformIndex[location].element * 4 * sizeof(T),
                size * sizeof(T));
    }
    else
    {
        unsigned int size = gl::VariableComponentCount(targetUniform->type);
        switch (gl::VariableComponentType(targetUniform->type))
        {
          case GL_BOOL:
            {
                GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = (boolParams[i] == GL_FALSE) ? static_cast<T>(0) : static_cast<T>(1);
                }
            }
            break;

          case GL_FLOAT:
            {
                GLfloat *floatParams = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(floatParams[i]);
                }
            }
            break;

          case GL_INT:
            {
                GLint *intParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(intParams[i]);
                }
            }
            break;

          case GL_UNSIGNED_INT:
            {
                GLuint *uintParams = (GLuint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(uintParams[i]);
                }
            }
            break;

          default: UNREACHABLE();
        }
    }
}

template <typename VarT>
void ProgramD3D::defineUniformBlockMembers(const std::vector<VarT> &fields, const std::string &prefix, int blockIndex,
                                           sh::BlockLayoutEncoder *encoder, std::vector<unsigned int> *blockUniformIndexes,
                                           bool inRowMajorLayout)
{
    for (unsigned int uniformIndex = 0; uniformIndex < fields.size(); uniformIndex++)
    {
        const VarT &field = fields[uniformIndex];
        const std::string &fieldName = (prefix.empty() ? field.name : prefix + "." + field.name);

        if (field.isStruct())
        {
            bool rowMajorLayout = (inRowMajorLayout || IsRowMajorLayout(field));

            for (unsigned int arrayElement = 0; arrayElement < field.elementCount(); arrayElement++)
            {
                encoder->enterAggregateType();

                const std::string uniformElementName = fieldName + (field.isArray() ? ArrayString(arrayElement) : "");
                defineUniformBlockMembers(field.fields, uniformElementName, blockIndex, encoder, blockUniformIndexes, rowMajorLayout);

                encoder->exitAggregateType();
            }
        }
        else
        {
            bool isRowMajorMatrix = (gl::IsMatrixType(field.type) && inRowMajorLayout);

            sh::BlockMemberInfo memberInfo = encoder->encodeType(field.type, field.arraySize, isRowMajorMatrix);

            gl::LinkedUniform *newUniform = new gl::LinkedUniform(field.type, field.precision, fieldName, field.arraySize,
                                                          blockIndex, memberInfo);

            // add to uniform list, but not index, since uniform block uniforms have no location
            blockUniformIndexes->push_back(mUniforms.size());
            mUniforms.push_back(newUniform);
        }
    }
}

bool ProgramD3D::defineUniformBlock(gl::InfoLog &infoLog, const gl::Shader &shader, const sh::InterfaceBlock &interfaceBlock,
                                    const gl::Caps &caps)
{
    const ShaderD3D* shaderD3D = ShaderD3D::makeShaderD3D(shader.getImplementation());

    // create uniform block entries if they do not exist
    if (getUniformBlockIndex(interfaceBlock.name) == GL_INVALID_INDEX)
    {
        std::vector<unsigned int> blockUniformIndexes;
        const unsigned int blockIndex = mUniformBlocks.size();

        // define member uniforms
        sh::BlockLayoutEncoder *encoder = NULL;

        if (interfaceBlock.layout == sh::BLOCKLAYOUT_STANDARD)
        {
            encoder = new sh::Std140BlockEncoder;
        }
        else
        {
            encoder = new sh::HLSLBlockEncoder(sh::HLSLBlockEncoder::ENCODE_PACKED);
        }
        ASSERT(encoder);

        defineUniformBlockMembers(interfaceBlock.fields, "", blockIndex, encoder, &blockUniformIndexes, interfaceBlock.isRowMajorLayout);

        size_t dataSize = encoder->getBlockSize();

        // create all the uniform blocks
        if (interfaceBlock.arraySize > 0)
        {
            for (unsigned int uniformBlockElement = 0; uniformBlockElement < interfaceBlock.arraySize; uniformBlockElement++)
            {
                gl::UniformBlock *newUniformBlock = new gl::UniformBlock(interfaceBlock.name, uniformBlockElement, dataSize);
                newUniformBlock->memberUniformIndexes = blockUniformIndexes;
                mUniformBlocks.push_back(newUniformBlock);
            }
        }
        else
        {
            gl::UniformBlock *newUniformBlock = new gl::UniformBlock(interfaceBlock.name, GL_INVALID_INDEX, dataSize);
            newUniformBlock->memberUniformIndexes = blockUniformIndexes;
            mUniformBlocks.push_back(newUniformBlock);
        }
    }

    if (interfaceBlock.staticUse)
    {
        // Assign registers to the uniform blocks
        const GLuint blockIndex = getUniformBlockIndex(interfaceBlock.name);
        const unsigned int elementCount = std::max(1u, interfaceBlock.arraySize);
        ASSERT(blockIndex != GL_INVALID_INDEX);
        ASSERT(blockIndex + elementCount <= mUniformBlocks.size());

        unsigned int interfaceBlockRegister = shaderD3D->getInterfaceBlockRegister(interfaceBlock.name);

        for (unsigned int uniformBlockElement = 0; uniformBlockElement < elementCount; uniformBlockElement++)
        {
            gl::UniformBlock *uniformBlock = mUniformBlocks[blockIndex + uniformBlockElement];
            ASSERT(uniformBlock->name == interfaceBlock.name);

            if (!assignUniformBlockRegister(infoLog, uniformBlock, shader.getType(),
                                            interfaceBlockRegister + uniformBlockElement, caps))
            {
                return false;
            }
        }
    }

    return true;
}

bool ProgramD3D::assignSamplers(unsigned int startSamplerIndex,
                                   GLenum samplerType,
                                   unsigned int samplerCount,
                                   std::vector<Sampler> &outSamplers,
                                   GLuint *outUsedRange)
{
    unsigned int samplerIndex = startSamplerIndex;

    do
    {
        if (samplerIndex < outSamplers.size())
        {
            Sampler& sampler = outSamplers[samplerIndex];
            sampler.active = true;
            sampler.textureType = GetTextureType(samplerType);
            sampler.logicalTextureUnit = 0;
            *outUsedRange = std::max(samplerIndex + 1, *outUsedRange);
        }
        else
        {
            return false;
        }

        samplerIndex++;
    } while (samplerIndex < startSamplerIndex + samplerCount);

    return true;
}

bool ProgramD3D::indexSamplerUniform(const gl::LinkedUniform &uniform, gl::InfoLog &infoLog, const gl::Caps &caps)
{
    ASSERT(gl::IsSampler(uniform.type));
    ASSERT(uniform.vsRegisterIndex != GL_INVALID_INDEX || uniform.psRegisterIndex != GL_INVALID_INDEX);

    if (uniform.vsRegisterIndex != GL_INVALID_INDEX)
    {
        if (!assignSamplers(uniform.vsRegisterIndex, uniform.type, uniform.arraySize, mSamplersVS,
                            &mUsedVertexSamplerRange))
        {
            infoLog.append("Vertex shader sampler count exceeds the maximum vertex texture units (%d).",
                           mSamplersVS.size());
            return false;
        }

        unsigned int maxVertexVectors = mRenderer->getReservedVertexUniformVectors() + caps.maxVertexUniformVectors;
        if (uniform.vsRegisterIndex + uniform.registerCount > maxVertexVectors)
        {
            infoLog.append("Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS (%u)",
                           caps.maxVertexUniformVectors);
            return false;
        }
    }

    if (uniform.psRegisterIndex != GL_INVALID_INDEX)
    {
        if (!assignSamplers(uniform.psRegisterIndex, uniform.type, uniform.arraySize, mSamplersPS,
                            &mUsedPixelSamplerRange))
        {
            infoLog.append("Pixel shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS (%d).",
                           mSamplersPS.size());
            return false;
        }

        unsigned int maxFragmentVectors = mRenderer->getReservedFragmentUniformVectors() + caps.maxFragmentUniformVectors;
        if (uniform.psRegisterIndex + uniform.registerCount > maxFragmentVectors)
        {
            infoLog.append("Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS (%u)",
                           caps.maxFragmentUniformVectors);
            return false;
        }
    }

    return true;
}

bool ProgramD3D::indexUniforms(gl::InfoLog &infoLog, const gl::Caps &caps)
{
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const gl::LinkedUniform &uniform = *mUniforms[uniformIndex];

        if (gl::IsSampler(uniform.type))
        {
            if (!indexSamplerUniform(uniform, infoLog, caps))
            {
                return false;
            }
        }

        for (unsigned int arrayElementIndex = 0; arrayElementIndex < uniform.elementCount(); arrayElementIndex++)
        {
            mUniformIndex.push_back(gl::VariableLocation(uniform.name, arrayElementIndex, uniformIndex));
        }
    }

    return true;
}

void ProgramD3D::reset()
{
    ProgramImpl::reset();

    SafeDeleteContainer(mVertexExecutables);
    SafeDeleteContainer(mPixelExecutables);
    SafeDelete(mGeometryExecutable);

    mTransformFeedbackBufferMode = GL_NONE;

    mVertexHLSL.clear();
    mVertexWorkarounds = ANGLE_D3D_WORKAROUND_NONE;
    mShaderVersion = 100;

    mPixelHLSL.clear();
    mPixelWorkarounds = ANGLE_D3D_WORKAROUND_NONE;
    mUsesFragDepth = false;
    mPixelShaderKey.clear();
    mUsesPointSize = false;

    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);

    mSamplersPS.clear();
    mSamplersVS.clear();

    mUsedVertexSamplerRange = 0;
    mUsedPixelSamplerRange = 0;
    mDirtySamplerMapping = true;
}

}

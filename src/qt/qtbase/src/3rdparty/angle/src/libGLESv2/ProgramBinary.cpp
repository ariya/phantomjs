//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libGLESv2/BinaryStream.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/ShaderExecutable.h"

#include "common/debug.h"
#include "common/version.h"
#include "common/utilities.h"
#include "common/platform.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/ProgramImpl.h"
#include "libGLESv2/renderer/d3d/ShaderD3D.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Buffer.h"
#include "common/blocklayout.h"
#include "common/features.h"

namespace gl
{

namespace
{

unsigned int ParseAndStripArrayIndex(std::string* name)
{
    unsigned int subscript = GL_INVALID_INDEX;

    // Strip any trailing array operator and retrieve the subscript
    size_t open = name->find_last_of('[');
    size_t close = name->find_last_of(']');
    if (open != std::string::npos && close == name->length() - 1)
    {
        subscript = atoi(name->substr(open + 1).c_str());
        name->erase(open);
    }

    return subscript;
}

}

VariableLocation::VariableLocation(const std::string &name, unsigned int element, unsigned int index)
    : name(name), element(element), index(index)
{
}

LinkedVarying::LinkedVarying()
{
}

LinkedVarying::LinkedVarying(const std::string &name, GLenum type, GLsizei size, const std::string &semanticName,
                             unsigned int semanticIndex, unsigned int semanticIndexCount)
    : name(name), type(type), size(size), semanticName(semanticName), semanticIndex(semanticIndex), semanticIndexCount(semanticIndexCount)
{
}

LinkResult::LinkResult(bool linkSuccess, const Error &error)
    : linkSuccess(linkSuccess),
      error(error)
{
}

unsigned int ProgramBinary::mCurrentSerial = 1;

ProgramBinary::ProgramBinary(rx::ProgramImpl *impl)
    : RefCountObject(0),
      mProgram(impl),
      mValidated(false),
      mSerial(issueSerial())
{
    ASSERT(impl);

    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        mSemanticIndex[index] = -1;
    }
}

ProgramBinary::~ProgramBinary()
{
    reset();
    SafeDelete(mProgram);
}

unsigned int ProgramBinary::getSerial() const
{
    return mSerial;
}

unsigned int ProgramBinary::issueSerial()
{
    return mCurrentSerial++;
}

GLuint ProgramBinary::getAttributeLocation(const char *name)
{
    if (name)
    {
        for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
        {
            if (mLinkedAttribute[index].name == std::string(name))
            {
                return index;
            }
        }
    }

    return -1;
}

int ProgramBinary::getSemanticIndex(int attributeIndex)
{
    ASSERT(attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS);

    return mSemanticIndex[attributeIndex];
}

// Returns one more than the highest sampler index used.
GLint ProgramBinary::getUsedSamplerRange(SamplerType type)
{
    return mProgram->getUsedSamplerRange(type);
}

bool ProgramBinary::usesPointSize() const
{
    return mProgram->usesPointSize();
}

GLint ProgramBinary::getSamplerMapping(SamplerType type, unsigned int samplerIndex, const Caps &caps)
{
    return mProgram->getSamplerMapping(type, samplerIndex, caps);
}

GLenum ProgramBinary::getSamplerTextureType(SamplerType type, unsigned int samplerIndex)
{
    return mProgram->getSamplerTextureType(type, samplerIndex);
}

GLint ProgramBinary::getUniformLocation(std::string name)
{
    return mProgram->getUniformLocation(name);
}

GLuint ProgramBinary::getUniformIndex(std::string name)
{
    return mProgram->getUniformIndex(name);
}

GLuint ProgramBinary::getUniformBlockIndex(std::string name)
{
    return mProgram->getUniformBlockIndex(name);
}

UniformBlock *ProgramBinary::getUniformBlockByIndex(GLuint blockIndex)
{
    return mProgram->getUniformBlockByIndex(blockIndex);
}

GLint ProgramBinary::getFragDataLocation(const char *name) const
{
    std::string baseName(name);
    unsigned int arrayIndex;
    arrayIndex = ParseAndStripArrayIndex(&baseName);

    for (auto locationIt = mOutputVariables.begin(); locationIt != mOutputVariables.end(); locationIt++)
    {
        const VariableLocation &outputVariable = locationIt->second;

        if (outputVariable.name == baseName && (arrayIndex == GL_INVALID_INDEX || arrayIndex == outputVariable.element))
        {
            return static_cast<GLint>(locationIt->first);
        }
    }

    return -1;
}

size_t ProgramBinary::getTransformFeedbackVaryingCount() const
{
    return mProgram->getTransformFeedbackLinkedVaryings().size();
}

const LinkedVarying &ProgramBinary::getTransformFeedbackVarying(size_t idx) const
{
    return mProgram->getTransformFeedbackLinkedVaryings()[idx];
}

GLenum ProgramBinary::getTransformFeedbackBufferMode() const
{
    return mProgram->getTransformFeedbackBufferMode();
}

void ProgramBinary::setUniform1fv(GLint location, GLsizei count, const GLfloat *v) {
    mProgram->setUniform1fv(location, count, v);
}

void ProgramBinary::setUniform2fv(GLint location, GLsizei count, const GLfloat *v) {
    mProgram->setUniform2fv(location, count, v);
}

void ProgramBinary::setUniform3fv(GLint location, GLsizei count, const GLfloat *v) {
    mProgram->setUniform3fv(location, count, v);
}

void ProgramBinary::setUniform4fv(GLint location, GLsizei count, const GLfloat *v) {
    mProgram->setUniform4fv(location, count, v);
}

void ProgramBinary::setUniform1iv(GLint location, GLsizei count, const GLint *v) {
    mProgram->setUniform1iv(location, count, v);
}

void ProgramBinary::setUniform2iv(GLint location, GLsizei count, const GLint *v) {
    mProgram->setUniform2iv(location, count, v);
}

void ProgramBinary::setUniform3iv(GLint location, GLsizei count, const GLint *v) {
    mProgram->setUniform3iv(location, count, v);
}

void ProgramBinary::setUniform4iv(GLint location, GLsizei count, const GLint *v) {
    mProgram->setUniform4iv(location, count, v);
}

void ProgramBinary::setUniform1uiv(GLint location, GLsizei count, const GLuint *v) {
    mProgram->setUniform1uiv(location, count, v);
}

void ProgramBinary::setUniform2uiv(GLint location, GLsizei count, const GLuint *v) {
    mProgram->setUniform2uiv(location, count, v);
}

void ProgramBinary::setUniform3uiv(GLint location, GLsizei count, const GLuint *v) {
    mProgram->setUniform3uiv(location, count, v);
}

void ProgramBinary::setUniform4uiv(GLint location, GLsizei count, const GLuint *v) {
    mProgram->setUniform4uiv(location, count, v);
}

void ProgramBinary::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix2fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix3fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix4fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix2x3fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix2x4fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix3x2fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix3x4fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix4x2fv(location, count, transpose, v);
}

void ProgramBinary::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v) {
    mProgram->setUniformMatrix4x3fv(location, count, transpose, v);
}

void ProgramBinary::getUniformfv(GLint location, GLfloat *v) {
    mProgram->getUniformfv(location, v);
}

void ProgramBinary::getUniformiv(GLint location, GLint *v) {
    mProgram->getUniformiv(location, v);
}

void ProgramBinary::getUniformuiv(GLint location, GLuint *v) {
    mProgram->getUniformuiv(location, v);
}

void ProgramBinary::updateSamplerMapping()
{
    return mProgram->updateSamplerMapping();
}

// Applies all the uniforms set for this program object to the renderer
Error ProgramBinary::applyUniforms()
{
    return mProgram->applyUniforms();
}

Error ProgramBinary::applyUniformBuffers(const std::vector<gl::Buffer*> boundBuffers, const Caps &caps)
{
    return mProgram->applyUniformBuffers(boundBuffers, caps);
}

bool ProgramBinary::linkVaryings(InfoLog &infoLog, Shader *fragmentShader, Shader *vertexShader)
{
    std::vector<PackedVarying> &fragmentVaryings = fragmentShader->getVaryings();
    std::vector<PackedVarying> &vertexVaryings = vertexShader->getVaryings();

    for (size_t fragVaryingIndex = 0; fragVaryingIndex < fragmentVaryings.size(); fragVaryingIndex++)
    {
        PackedVarying *input = &fragmentVaryings[fragVaryingIndex];
        bool matched = false;

        // Built-in varyings obey special rules
        if (input->isBuiltIn())
        {
            continue;
        }

        for (size_t vertVaryingIndex = 0; vertVaryingIndex < vertexVaryings.size(); vertVaryingIndex++)
        {
            PackedVarying *output = &vertexVaryings[vertVaryingIndex];
            if (output->name == input->name)
            {
                if (!linkValidateVaryings(infoLog, output->name, *input, *output))
                {
                    return false;
                }

                output->registerIndex = input->registerIndex;
                output->columnIndex = input->columnIndex;

                matched = true;
                break;
            }
        }

        // We permit unmatched, unreferenced varyings
        if (!matched && input->staticUse)
        {
            infoLog.append("Fragment varying %s does not match any vertex varying", input->name.c_str());
            return false;
        }
    }

    return true;
}

LinkResult ProgramBinary::load(InfoLog &infoLog, GLenum binaryFormat, const void *binary, GLsizei length)
{
#if ANGLE_PROGRAM_BINARY_LOAD == ANGLE_DISABLED
    return LinkResult(false, Error(GL_NO_ERROR));
#else
    ASSERT(binaryFormat == mProgram->getBinaryFormat());

    reset();

    BinaryInputStream stream(binary, length);

    GLenum format = stream.readInt<GLenum>();
    if (format != mProgram->getBinaryFormat())
    {
        infoLog.append("Invalid program binary format.");
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    int majorVersion = stream.readInt<int>();
    int minorVersion = stream.readInt<int>();
    if (majorVersion != ANGLE_MAJOR_VERSION || minorVersion != ANGLE_MINOR_VERSION)
    {
        infoLog.append("Invalid program binary version.");
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    unsigned char commitString[ANGLE_COMMIT_HASH_SIZE];
    stream.readBytes(commitString, ANGLE_COMMIT_HASH_SIZE);
    if (memcmp(commitString, ANGLE_COMMIT_HASH, sizeof(unsigned char) * ANGLE_COMMIT_HASH_SIZE) != 0)
    {
        infoLog.append("Invalid program binary version.");
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    int compileFlags = stream.readInt<int>();
    if (compileFlags != ANGLE_COMPILE_OPTIMIZATION_LEVEL)
    {
        infoLog.append("Mismatched compilation flags.");
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.readInt(&mLinkedAttribute[i].type);
        stream.readString(&mLinkedAttribute[i].name);
        stream.readInt(&mProgram->getShaderAttributes()[i].type);
        stream.readString(&mProgram->getShaderAttributes()[i].name);
        stream.readInt(&mSemanticIndex[i]);
    }

    initAttributesByLayout();

    LinkResult result = mProgram->load(infoLog, &stream);
    if (result.error.isError() || !result.linkSuccess)
    {
        return result;
    }

    return LinkResult(true, Error(GL_NO_ERROR));
#endif // #if ANGLE_PROGRAM_BINARY_LOAD == ANGLE_ENABLED
}

Error ProgramBinary::save(GLenum *binaryFormat, void *binary, GLsizei bufSize, GLsizei *length)
{
    if (binaryFormat)
    {
        *binaryFormat = mProgram->getBinaryFormat();
    }

    BinaryOutputStream stream;

    stream.writeInt(mProgram->getBinaryFormat());
    stream.writeInt(ANGLE_MAJOR_VERSION);
    stream.writeInt(ANGLE_MINOR_VERSION);
    stream.writeBytes(reinterpret_cast<const unsigned char*>(ANGLE_COMMIT_HASH), ANGLE_COMMIT_HASH_SIZE);
    stream.writeInt(ANGLE_COMPILE_OPTIMIZATION_LEVEL);

    for (unsigned int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.writeInt(mLinkedAttribute[i].type);
        stream.writeString(mLinkedAttribute[i].name);
        stream.writeInt(mProgram->getShaderAttributes()[i].type);
        stream.writeString(mProgram->getShaderAttributes()[i].name);
        stream.writeInt(mSemanticIndex[i]);
    }

    mProgram->save(&stream);

    GLsizei streamLength = stream.length();
    const void *streamData = stream.data();

    if (streamLength > bufSize)
    {
        if (length)
        {
            *length = 0;
        }

        // TODO: This should be moved to the validation layer but computing the size of the binary before saving
        // it causes the save to happen twice.  It may be possible to write the binary to a separate buffer, validate
        // sizes and then copy it.
        return Error(GL_INVALID_OPERATION);
    }

    if (binary)
    {
        char *ptr = (char*) binary;

        memcpy(ptr, streamData, streamLength);
        ptr += streamLength;

        ASSERT(ptr - streamLength == binary);
    }

    if (length)
    {
        *length = streamLength;
    }

    return Error(GL_NO_ERROR);
}

GLint ProgramBinary::getLength()
{
    GLint length;
    Error error = save(NULL, NULL, INT_MAX, &length);
    if (error.isError())
    {
        return 0;
    }

    return length;
}

LinkResult ProgramBinary::link(const Data &data, InfoLog &infoLog, const AttributeBindings &attributeBindings,
                               Shader *fragmentShader, Shader *vertexShader,
                               const std::vector<std::string> &transformFeedbackVaryings,
                               GLenum transformFeedbackBufferMode)
{
    if (!fragmentShader || !fragmentShader->isCompiled())
    {
        return LinkResult(false, Error(GL_NO_ERROR));
    }
    ASSERT(fragmentShader->getType() == GL_FRAGMENT_SHADER);

    if (!vertexShader || !vertexShader->isCompiled())
    {
        return LinkResult(false, Error(GL_NO_ERROR));
    }
    ASSERT(vertexShader->getType() == GL_VERTEX_SHADER);

    reset();

    int registers;
    std::vector<LinkedVarying> linkedVaryings;
    LinkResult result = mProgram->link(data, infoLog, fragmentShader, vertexShader, transformFeedbackVaryings, transformFeedbackBufferMode,
                                       &registers, &linkedVaryings, &mOutputVariables);
    if (result.error.isError() || !result.linkSuccess)
    {
        return result;
    }

    if (!linkAttributes(infoLog, attributeBindings, vertexShader))
    {
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    if (!mProgram->linkUniforms(infoLog, *vertexShader, *fragmentShader, *data.caps))
    {
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    if (!linkUniformBlocks(infoLog, *vertexShader, *fragmentShader, *data.caps))
    {
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    if (!gatherTransformFeedbackLinkedVaryings(infoLog, linkedVaryings, transformFeedbackVaryings,
                                               transformFeedbackBufferMode, &mProgram->getTransformFeedbackLinkedVaryings(), *data.caps))
    {
        return LinkResult(false, Error(GL_NO_ERROR));
    }

    // TODO: The concept of "executables" is D3D only, and as such this belongs in ProgramD3D. It must be called,
    // however, last in this function, so it can't simply be moved to ProgramD3D::link without further shuffling.
    result = mProgram->compileProgramExecutables(infoLog, fragmentShader, vertexShader, registers);
    if (result.error.isError() || !result.linkSuccess)
    {
        infoLog.append("Failed to create D3D shaders.");
        reset();
        return result;
    }

    return LinkResult(true, Error(GL_NO_ERROR));
}

bool ProgramBinary::linkUniformBlocks(gl::InfoLog &infoLog, const gl::Shader &vertexShader, const gl::Shader &fragmentShader,
                                   const gl::Caps &caps)
{
    const std::vector<sh::InterfaceBlock> &vertexInterfaceBlocks = vertexShader.getInterfaceBlocks();
    const std::vector<sh::InterfaceBlock> &fragmentInterfaceBlocks = fragmentShader.getInterfaceBlocks();

    // Check that interface blocks defined in the vertex and fragment shaders are identical
    typedef std::map<std::string, const sh::InterfaceBlock*> UniformBlockMap;
    UniformBlockMap linkedUniformBlocks;

    for (unsigned int blockIndex = 0; blockIndex < vertexInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &vertexInterfaceBlock = vertexInterfaceBlocks[blockIndex];
        linkedUniformBlocks[vertexInterfaceBlock.name] = &vertexInterfaceBlock;
    }

    for (unsigned int blockIndex = 0; blockIndex < fragmentInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &fragmentInterfaceBlock = fragmentInterfaceBlocks[blockIndex];
        UniformBlockMap::const_iterator entry = linkedUniformBlocks.find(fragmentInterfaceBlock.name);
        if (entry != linkedUniformBlocks.end())
        {
            const sh::InterfaceBlock &vertexInterfaceBlock = *entry->second;
            if (!areMatchingInterfaceBlocks(infoLog, vertexInterfaceBlock, fragmentInterfaceBlock))
            {
                return false;
            }
        }
    }

    for (unsigned int blockIndex = 0; blockIndex < vertexInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &interfaceBlock = vertexInterfaceBlocks[blockIndex];

        // Note: shared and std140 layouts are always considered active
        if (interfaceBlock.staticUse || interfaceBlock.layout != sh::BLOCKLAYOUT_PACKED)
        {
            if (!mProgram->defineUniformBlock(infoLog, vertexShader, interfaceBlock, caps))
            {
                return false;
            }
        }
    }

    for (unsigned int blockIndex = 0; blockIndex < fragmentInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &interfaceBlock = fragmentInterfaceBlocks[blockIndex];

        // Note: shared and std140 layouts are always considered active
        if (interfaceBlock.staticUse || interfaceBlock.layout != sh::BLOCKLAYOUT_PACKED)
        {
            if (!mProgram->defineUniformBlock(infoLog, fragmentShader, interfaceBlock, caps))
            {
                return false;
            }
        }
    }

    return true;
}

bool ProgramBinary::areMatchingInterfaceBlocks(gl::InfoLog &infoLog, const sh::InterfaceBlock &vertexInterfaceBlock,
                                            const sh::InterfaceBlock &fragmentInterfaceBlock)
{
    const char* blockName = vertexInterfaceBlock.name.c_str();

    // validate blocks for the same member types
    if (vertexInterfaceBlock.fields.size() != fragmentInterfaceBlock.fields.size())
    {
        infoLog.append("Types for interface block '%s' differ between vertex and fragment shaders", blockName);
        return false;
    }

    if (vertexInterfaceBlock.arraySize != fragmentInterfaceBlock.arraySize)
    {
        infoLog.append("Array sizes differ for interface block '%s' between vertex and fragment shaders", blockName);
        return false;
    }

    if (vertexInterfaceBlock.layout != fragmentInterfaceBlock.layout || vertexInterfaceBlock.isRowMajorLayout != fragmentInterfaceBlock.isRowMajorLayout)
    {
        infoLog.append("Layout qualifiers differ for interface block '%s' between vertex and fragment shaders", blockName);
        return false;
    }

    const unsigned int numBlockMembers = vertexInterfaceBlock.fields.size();
    for (unsigned int blockMemberIndex = 0; blockMemberIndex < numBlockMembers; blockMemberIndex++)
    {
        const sh::InterfaceBlockField &vertexMember = vertexInterfaceBlock.fields[blockMemberIndex];
        const sh::InterfaceBlockField &fragmentMember = fragmentInterfaceBlock.fields[blockMemberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog.append("Name mismatch for field %d of interface block '%s': (in vertex: '%s', in fragment: '%s')",
                           blockMemberIndex, blockName, vertexMember.name.c_str(), fragmentMember.name.c_str());
            return false;
        }

        std::string memberName = "interface block '" + vertexInterfaceBlock.name + "' member '" + vertexMember.name + "'";
        if (!gl::ProgramBinary::linkValidateInterfaceBlockFields(infoLog, memberName, vertexMember, fragmentMember))
        {
            return false;
        }
    }

    return true;
}

// Determines the mapping between GL attributes and Direct3D 9 vertex stream usage indices
bool ProgramBinary::linkAttributes(InfoLog &infoLog, const AttributeBindings &attributeBindings, const Shader *vertexShader)
{
    const rx::ShaderD3D *vertexShaderD3D = rx::ShaderD3D::makeShaderD3D(vertexShader->getImplementation());

    unsigned int usedLocations = 0;
    const std::vector<sh::Attribute> &shaderAttributes = vertexShader->getActiveAttributes();

    // Link attributes that have a binding location
    for (unsigned int attributeIndex = 0; attributeIndex < shaderAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = shaderAttributes[attributeIndex];

        ASSERT(attribute.staticUse);

        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        mProgram->getShaderAttributes()[attributeIndex] = attribute;

        if (location != -1)   // Set by glBindAttribLocation or by location layout qualifier
        {
            const int rows = VariableRegisterCount(attribute.type);

            if (rows + location > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Active attribute (%s) at location %d is too big to fit", attribute.name.c_str(), location);

                return false;
            }

            for (int row = 0; row < rows; row++)
            {
                const int rowLocation = location + row;
                sh::ShaderVariable &linkedAttribute = mLinkedAttribute[rowLocation];

                // In GLSL 3.00, attribute aliasing produces a link error
                // In GLSL 1.00, attribute aliasing is allowed
                if (mProgram->getShaderVersion() >= 300)
                {
                    if (!linkedAttribute.name.empty())
                    {
                        infoLog.append("Attribute '%s' aliases attribute '%s' at location %d", attribute.name.c_str(), linkedAttribute.name.c_str(), rowLocation);
                        return false;
                    }
                }

                linkedAttribute = attribute;
                usedLocations |= 1 << rowLocation;
            }
        }
    }

    // Link attributes that don't have a binding location
    for (unsigned int attributeIndex = 0; attributeIndex < shaderAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = shaderAttributes[attributeIndex];

        ASSERT(attribute.staticUse);

        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        if (location == -1)   // Not set by glBindAttribLocation or by location layout qualifier
        {
            int rows = VariableRegisterCount(attribute.type);
            int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, MAX_VERTEX_ATTRIBS);

            if (availableIndex == -1 || availableIndex + rows > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Too many active attributes (%s)", attribute.name.c_str());

                return false;   // Fail to link
            }

            mLinkedAttribute[availableIndex] = attribute;
        }
    }

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; )
    {
        int index = vertexShaderD3D->getSemanticIndex(mLinkedAttribute[attributeIndex].name);
        int rows = VariableRegisterCount(mLinkedAttribute[attributeIndex].type);

        for (int r = 0; r < rows; r++)
        {
            mSemanticIndex[attributeIndex++] = index++;
        }
    }

    initAttributesByLayout();

    return true;
}

bool ProgramBinary::linkValidateVariablesBase(InfoLog &infoLog, const std::string &variableName, const sh::ShaderVariable &vertexVariable,
                                              const sh::ShaderVariable &fragmentVariable, bool validatePrecision)
{
    if (vertexVariable.type != fragmentVariable.type)
    {
        infoLog.append("Types for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    if (vertexVariable.arraySize != fragmentVariable.arraySize)
    {
        infoLog.append("Array sizes for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    if (validatePrecision && vertexVariable.precision != fragmentVariable.precision)
    {
        infoLog.append("Precisions for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }

    if (vertexVariable.fields.size() != fragmentVariable.fields.size())
    {
        infoLog.append("Structure lengths for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    const unsigned int numMembers = vertexVariable.fields.size();
    for (unsigned int memberIndex = 0; memberIndex < numMembers; memberIndex++)
    {
        const sh::ShaderVariable &vertexMember = vertexVariable.fields[memberIndex];
        const sh::ShaderVariable &fragmentMember = fragmentVariable.fields[memberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog.append("Name mismatch for field '%d' of %s: (in vertex: '%s', in fragment: '%s')",
                           memberIndex, variableName.c_str(),
                           vertexMember.name.c_str(), fragmentMember.name.c_str());
            return false;
        }

        const std::string memberName = variableName.substr(0, variableName.length() - 1) + "." +
                                       vertexMember.name + "'";

        if (!linkValidateVariablesBase(infoLog, vertexMember.name, vertexMember, fragmentMember, validatePrecision))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::linkValidateUniforms(InfoLog &infoLog, const std::string &uniformName, const sh::Uniform &vertexUniform, const sh::Uniform &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    return true;
}

bool ProgramBinary::linkValidateVaryings(InfoLog &infoLog, const std::string &varyingName, const sh::Varying &vertexVarying, const sh::Varying &fragmentVarying)
{
    if (!linkValidateVariablesBase(infoLog, varyingName, vertexVarying, fragmentVarying, false))
    {
        return false;
    }

    if (vertexVarying.interpolation != fragmentVarying.interpolation)
    {
        infoLog.append("Interpolation types for %s differ between vertex and fragment shaders", varyingName.c_str());
        return false;
    }

    return true;
}

bool ProgramBinary::linkValidateInterfaceBlockFields(InfoLog &infoLog, const std::string &uniformName, const sh::InterfaceBlockField &vertexUniform, const sh::InterfaceBlockField &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    if (vertexUniform.isRowMajorLayout != fragmentUniform.isRowMajorLayout)
    {
        infoLog.append("Matrix packings for %s differ between vertex and fragment shaders", uniformName.c_str());
        return false;
    }

    return true;
}

bool ProgramBinary::gatherTransformFeedbackLinkedVaryings(InfoLog &infoLog, const std::vector<LinkedVarying> &linkedVaryings,
                                                          const std::vector<std::string> &transformFeedbackVaryingNames,
                                                          GLenum transformFeedbackBufferMode,
                                                          std::vector<LinkedVarying> *outTransformFeedbackLinkedVaryings,
                                                          const Caps &caps) const
{
    size_t totalComponents = 0;

    // Gather the linked varyings that are used for transform feedback, they should all exist.
    outTransformFeedbackLinkedVaryings->clear();
    for (size_t i = 0; i < transformFeedbackVaryingNames.size(); i++)
    {
        bool found = false;
        for (size_t j = 0; j < linkedVaryings.size(); j++)
        {
            if (transformFeedbackVaryingNames[i] == linkedVaryings[j].name)
            {
                for (size_t k = 0; k < outTransformFeedbackLinkedVaryings->size(); k++)
                {
                    if (outTransformFeedbackLinkedVaryings->at(k).name == linkedVaryings[j].name)
                    {
                        infoLog.append("Two transform feedback varyings specify the same output variable (%s).", linkedVaryings[j].name.c_str());
                        return false;
                    }
                }

                size_t componentCount = linkedVaryings[j].semanticIndexCount * 4;
                if (transformFeedbackBufferMode == GL_SEPARATE_ATTRIBS &&
                    componentCount > caps.maxTransformFeedbackSeparateComponents)
                {
                    infoLog.append("Transform feedback varying's %s components (%u) exceed the maximum separate components (%u).",
                                   linkedVaryings[j].name.c_str(), componentCount, caps.maxTransformFeedbackSeparateComponents);
                    return false;
                }

                totalComponents += componentCount;

                outTransformFeedbackLinkedVaryings->push_back(linkedVaryings[j]);
                found = true;
                break;
            }
        }

        // All transform feedback varyings are expected to exist since packVaryings checks for them.
        ASSERT(found);
    }

    if (transformFeedbackBufferMode == GL_INTERLEAVED_ATTRIBS && totalComponents > caps.maxTransformFeedbackInterleavedComponents)
    {
        infoLog.append("Transform feedback varying total components (%u) exceed the maximum interleaved components (%u).",
                       totalComponents, caps.maxTransformFeedbackInterleavedComponents);
        return false;
    }

    return true;
}

bool ProgramBinary::isValidated() const
{
    return mValidated;
}

void ProgramBinary::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    // Skip over inactive attributes
    unsigned int activeAttribute = 0;
    unsigned int attribute;
    for (attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if (mLinkedAttribute[attribute].name.empty())
        {
            continue;
        }

        if (activeAttribute == index)
        {
            break;
        }

        activeAttribute++;
    }

    if (bufsize > 0)
    {
        const char *string = mLinkedAttribute[attribute].name.c_str();

        strncpy(name, string, bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = 1;   // Always a single 'type' instance

    *type = mLinkedAttribute[attribute].type;
}

GLint ProgramBinary::getActiveAttributeCount() const
{
    int count = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            count++;
        }
    }

    return count;
}

GLint ProgramBinary::getActiveAttributeMaxLength() const
{
    int maxLength = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            maxLength = std::max((int)(mLinkedAttribute[attributeIndex].name.length() + 1), maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    ASSERT(index < mProgram->getUniforms().size());   // index must be smaller than getActiveUniformCount()

    if (bufsize > 0)
    {
        std::string string = mProgram->getUniforms()[index]->name;

        if (mProgram->getUniforms()[index]->isArray())
        {
            string += "[0]";
        }

        strncpy(name, string.c_str(), bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = mProgram->getUniforms()[index]->elementCount();

    *type = mProgram->getUniforms()[index]->type;
}

GLint ProgramBinary::getActiveUniformCount() const
{
    return mProgram->getUniforms().size();
}

GLint ProgramBinary::getActiveUniformMaxLength() const
{
    int maxLength = 0;

    unsigned int numUniforms = mProgram->getUniforms().size();
    for (unsigned int uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
    {
        if (!mProgram->getUniforms()[uniformIndex]->name.empty())
        {
            int length = (int)(mProgram->getUniforms()[uniformIndex]->name.length() + 1);
            if (mProgram->getUniforms()[uniformIndex]->isArray())
            {
                length += 3;  // Counting in "[0]".
            }
            maxLength = std::max(length, maxLength);
        }
    }

    return maxLength;
}

GLint ProgramBinary::getActiveUniformi(GLuint index, GLenum pname) const
{
    const gl::LinkedUniform& uniform = *mProgram->getUniforms()[index];

    switch (pname)
    {
      case GL_UNIFORM_TYPE:         return static_cast<GLint>(uniform.type);
      case GL_UNIFORM_SIZE:         return static_cast<GLint>(uniform.elementCount());
      case GL_UNIFORM_NAME_LENGTH:  return static_cast<GLint>(uniform.name.size() + 1 + (uniform.isArray() ? 3 : 0));
      case GL_UNIFORM_BLOCK_INDEX:  return uniform.blockIndex;

      case GL_UNIFORM_OFFSET:       return uniform.blockInfo.offset;
      case GL_UNIFORM_ARRAY_STRIDE: return uniform.blockInfo.arrayStride;
      case GL_UNIFORM_MATRIX_STRIDE: return uniform.blockInfo.matrixStride;
      case GL_UNIFORM_IS_ROW_MAJOR: return static_cast<GLint>(uniform.blockInfo.isRowMajorMatrix);

      default:
        UNREACHABLE();
        break;
    }
    return 0;
}

bool ProgramBinary::isValidUniformLocation(GLint location) const
{
    ASSERT(rx::IsIntegerCastSafe<GLint>(mProgram->getUniformIndices().size()));
    return (location >= 0 && location < static_cast<GLint>(mProgram->getUniformIndices().size()));
}

LinkedUniform *ProgramBinary::getUniformByLocation(GLint location) const
{
    return mProgram->getUniformByLocation(location);
}

LinkedUniform *ProgramBinary::getUniformByName(const std::string &name) const
{
    return mProgram->getUniformByName(name);
}

void ProgramBinary::getActiveUniformBlockName(GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) const
{
    ASSERT(uniformBlockIndex < mProgram->getUniformBlocks().size());   // index must be smaller than getActiveUniformBlockCount()

    const UniformBlock &uniformBlock = *mProgram->getUniformBlocks()[uniformBlockIndex];

    if (bufSize > 0)
    {
        std::string string = uniformBlock.name;

        if (uniformBlock.isArrayElement())
        {
            string += ArrayString(uniformBlock.elementIndex);
        }

        strncpy(uniformBlockName, string.c_str(), bufSize);
        uniformBlockName[bufSize - 1] = '\0';

        if (length)
        {
            *length = strlen(uniformBlockName);
        }
    }
}

void ProgramBinary::getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const
{
    ASSERT(uniformBlockIndex < mProgram->getUniformBlocks().size());   // index must be smaller than getActiveUniformBlockCount()

    const UniformBlock &uniformBlock = *mProgram->getUniformBlocks()[uniformBlockIndex];

    switch (pname)
    {
      case GL_UNIFORM_BLOCK_DATA_SIZE:
        *params = static_cast<GLint>(uniformBlock.dataSize);
        break;
      case GL_UNIFORM_BLOCK_NAME_LENGTH:
        *params = static_cast<GLint>(uniformBlock.name.size() + 1 + (uniformBlock.isArrayElement() ? 3 : 0));
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
        *params = static_cast<GLint>(uniformBlock.memberUniformIndexes.size());
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
        {
            for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
            {
                params[blockMemberIndex] = static_cast<GLint>(uniformBlock.memberUniformIndexes[blockMemberIndex]);
            }
        }
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByVertexShader());
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByFragmentShader());
        break;
      default: UNREACHABLE();
    }
}

GLuint ProgramBinary::getActiveUniformBlockCount() const
{
    return mProgram->getUniformBlocks().size();
}

GLuint ProgramBinary::getActiveUniformBlockMaxLength() const
{
    unsigned int maxLength = 0;

    unsigned int numUniformBlocks = mProgram->getUniformBlocks().size();
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < numUniformBlocks; uniformBlockIndex++)
    {
        const UniformBlock &uniformBlock = *mProgram->getUniformBlocks()[uniformBlockIndex];
        if (!uniformBlock.name.empty())
        {
            const unsigned int length = uniformBlock.name.length() + 1;

            // Counting in "[0]".
            const unsigned int arrayLength = (uniformBlock.isArrayElement() ? 3 : 0);

            maxLength = std::max(length + arrayLength, maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::validate(InfoLog &infoLog, const Caps &caps)
{
    applyUniforms();
    if (!validateSamplers(&infoLog, caps))
    {
        mValidated = false;
    }
    else
    {
        mValidated = true;
    }
}

bool ProgramBinary::validateSamplers(InfoLog *infoLog, const Caps &caps)
{
    return mProgram->validateSamplers(infoLog, caps);
}

struct AttributeSorter
{
    AttributeSorter(const int (&semanticIndices)[MAX_VERTEX_ATTRIBS])
        : originalIndices(semanticIndices)
    {
    }

    bool operator()(int a, int b)
    {
        if (originalIndices[a] == -1) return false;
        if (originalIndices[b] == -1) return true;
        return (originalIndices[a] < originalIndices[b]);
    }

    const int (&originalIndices)[MAX_VERTEX_ATTRIBS];
};

void ProgramBinary::initAttributesByLayout()
{
    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        mAttributesByLayout[i] = i;
    }

    std::sort(&mAttributesByLayout[0], &mAttributesByLayout[MAX_VERTEX_ATTRIBS], AttributeSorter(mSemanticIndex));
}

void ProgramBinary::sortAttributesByLayout(rx::TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS], int sortedSemanticIndices[MAX_VERTEX_ATTRIBS]) const
{
    rx::TranslatedAttribute oldTranslatedAttributes[MAX_VERTEX_ATTRIBS];

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        oldTranslatedAttributes[i] = attributes[i];
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        int oldIndex = mAttributesByLayout[i];
        sortedSemanticIndices[i] = oldIndex;
        attributes[i] = oldTranslatedAttributes[oldIndex];
    }
}

void ProgramBinary::reset()
{
    mOutputVariables.clear();

    mProgram->reset();

    mValidated = false;
}

}

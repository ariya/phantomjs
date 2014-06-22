/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef LayerFilterRenderer_h
#define LayerFilterRenderer_h

#if USE(ACCELERATED_COMPOSITING) && ENABLE(CSS_FILTERS)

#include "IntRect.h"
#include "LayerData.h"
#include "LayerTexture.h"
#include "TransformationMatrix.h"

#include <BlackBerryPlatformGLES2Context.h>
#include <BlackBerryPlatformIntRectRegion.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class LayerCompositingThread;
class LayerFilterRenderer;
class LayerFilterRendererAction;
class LayerRendererSurface;

class Parameter : public RefCounted<Parameter> {
    WTF_MAKE_NONCOPYABLE(Parameter);
public:
    virtual void apply() = 0;
    virtual void restoreState() { }
    virtual ~Parameter() { }
protected:
    explicit Parameter() { }
};

class Uniform : public Parameter {
    WTF_MAKE_NONCOPYABLE(Uniform);
public:
    virtual void apply() = 0;
    virtual ~Uniform() { }

protected:
    Uniform(int location);
    const int& location() const { return m_location; }

private:
    int m_location;
};

class Uniform1f : public Uniform {
public:
    static PassRefPtr<Uniform> create(int location, float val);

    template<typename Functor>
    static PassRefPtr<Uniform> createWithFunctor(int location, Functor);

protected:
    Uniform1f(int location, float val);

    virtual void apply();

    float m_val;
};

template<typename Functor>
class Uniform1fWithFunctor : public Uniform1f {
public:
    static PassRefPtr<Uniform> create(int location, Functor functor)
    {
        return adoptRef(new Uniform1fWithFunctor(location, functor));
    }

protected:
    Uniform1fWithFunctor(int location, Functor functor)
        : Uniform1f(location, 0)
        , m_functor(functor)
    {
    }

    virtual void apply()
    {
        m_val = m_functor();
        Uniform1f::apply();
    }

    Functor m_functor;
};

template<typename Functor>
inline PassRefPtr<Uniform> Uniform1f::createWithFunctor(int location, Functor functor)
{
    return Uniform1fWithFunctor<Functor>::create(location, functor);
}

class Uniform1i : public Uniform {
public:
    static PassRefPtr<Uniform> create(int location, int val);

protected:
    Uniform1i(int location, int val);

private:
    virtual void apply();
    int m_val;
};

class Uniform2f : public Uniform {
public:
    static PassRefPtr<Uniform> create(int location, float val0, float val1);

protected:
    Uniform2f(int location, float val0, float val1);

private:
    virtual void apply();
    float m_val[2];
};

class Uniform3f : public Uniform {
public:
    static PassRefPtr<Uniform> create(int location, float val0, float val1, float val2);

protected:
    Uniform3f(int location, float val0, float val1, float val2);

private:
    virtual void apply();
    float m_val[3];
};

class Uniform4f : public Uniform {
public:
    static PassRefPtr<Uniform> create(int location, float val0, float val1, float val2, float val3);

protected:
    Uniform4f(int location, float val0, float val1, float val2, float val3);

private:
    virtual void apply();
    float m_val[4];
};

class Matrix4fv : public Uniform {
public:
    static PassRefPtr<Parameter> create(GLint location, GLsizei, GLboolean transpose, GLfloat* array);

protected:
    Matrix4fv(GLint location, GLsizei length, GLboolean transpose, GLfloat* array);
    ~Matrix4fv();

private:
    virtual void apply();
    GLint m_location;
    GLsizei m_size;
    GLboolean m_transpose;
    GLfloat* m_array;
};

class Buffer : public Parameter {
public:
    static PassRefPtr<Parameter> create(GLenum buffer, GLuint index);

protected:
    Buffer(GLenum buffer, GLuint index);

private:
    virtual void apply();
    virtual void restoreState();
    GLenum m_buffer;
    GLuint m_object;
};

class VertexAttribf : public Parameter {
public:
    static PassRefPtr<Parameter> create(int location, int size, int bytesPerVertex, int offset);

protected:
    VertexAttribf(int location, int size, int bytesPerVertex, int offset);

private:
    virtual void apply();
    virtual void restoreState();
    int m_location;
    int m_size;
    int m_bytesPerVertex;
    int m_offset;
};

class LayerFilterRendererAction : public RefCounted<LayerFilterRendererAction> {
public:
    static PassRefPtr<LayerFilterRendererAction> create(int programId);
        // A vector of actions must have an even count, so if you have an odd count, add a passthrough event at the end.
        // See the ping-pong note in LayerFilterRenderer::applyActions.

    bool shouldPushSnapshot() const { return m_pushSnapshot; }
    void setPushSnapshot() { m_pushSnapshot = true; }

    bool shouldPopSnapshot() const { return m_popSnapshot; }
    void setPopSnapshot() { m_popSnapshot = true; }

    void appendParameter(const RefPtr<Parameter>& uniform) { m_parameters.append(uniform); }
    void useActionOn(LayerFilterRenderer*);
    void restoreState();

    enum DrawingMode {
        DrawTriangleFanArrays = 0,
        DrawTriangleElementsUShort0,
        NumberOfDrawingModes
    };

    DrawingMode drawingMode() const { return m_drawingMode; }
    void setDrawingMode(const DrawingMode& mode) { m_drawingMode = mode; }

    int drawingModeParameter() const { return m_drawingModeParameter; }
    void setDrawingModeParameter(int p) { m_drawingModeParameter = p; }


protected:
    int m_programId;
    bool m_pushSnapshot;
    bool m_popSnapshot;

    Vector<RefPtr<Parameter> > m_parameters;

    DrawingMode m_drawingMode;
    int m_drawingModeParameter;

private:
    LayerFilterRendererAction(int programId);
};

class LayerFilterRenderer {
    WTF_MAKE_NONCOPYABLE(LayerFilterRenderer);

public:
    static PassOwnPtr<LayerFilterRenderer> create(const int& positionLocation, const int& texCoordLocation);
    void applyActions(unsigned& fbo, LayerCompositingThread*, Vector<RefPtr<LayerFilterRendererAction> >);
    Vector<RefPtr<LayerFilterRendererAction> > actionsForOperations(LayerRendererSurface*, const Vector<RefPtr<FilterOperation> >&);

    // If initialization fails, or disable() is called, this is false.
    bool isEnabled() const { return m_enabled; }
    void disable() { m_enabled = false; }

private:
    LayerFilterRenderer(const int& positionLocation, const int& texCoordLocation);
    void bindCommonAttribLocation(int location, const char* attribName);
    bool initializeSharedGLObjects();

    // See note about ping-ponging in applyActions()
    void ping(LayerRendererSurface*);
    void pong(LayerRendererSurface*);

    // This is for shadows, where we need to create a shadow, and then repaint the original image
    // on top of the shadow.
    void pushSnapshot(LayerRendererSurface*, int sourceId);
    void popSnapshot();

    bool m_enabled;

    // ESSL attributes shared with LayerRenderer - see constructor:
    const int m_positionLocation;
    const int m_texCoordLocation;

    // ESSL program object IDs:
    unsigned m_cssFilterProgramObject[LayerData::NumberOfCSSFilterShaders];

    // ESSL uniform locations:
    int m_amountLocation[LayerData::NumberOfCSSFilterShaders];
    int m_blurAmountLocation[2]; // 0 = Y, 1 = X
    int m_shadowColorLocation;
    int m_offsetLocation;

    // Textures for playing ping-pong - see note in applyActions()
    RefPtr<LayerTexture> m_texture;
    RefPtr<LayerTexture> m_snapshotTexture;

    friend class LayerFilterRendererAction;
};

}

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(CSS_FILTERS)

#endif // LayerFilterRenderer_h

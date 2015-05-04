CONFIG += simd installed
include(../common/common.pri)

INCLUDEPATH += $$OUT_PWD/.. $$ANGLE_DIR/src/libGLESv2

# Remember to adapt tools/configure/configureapp.cpp if the Direct X version changes.
!winrt: \
    LIBS_PRIVATE += -ld3d9
winrt: \
    LIBS_PRIVATE += -ld3dcompiler -ldxgi -ld3d11

LIBS_PRIVATE += -ldxguid

STATICLIBS = translator preprocessor
for(libname, STATICLIBS) {
    # Appends 'd' to the library for debug builds and builds up the fully
    # qualified path to pass to the linker.
    staticlib = $$QT_BUILD_TREE/lib/$${QMAKE_PREFIX_STATICLIB}$$qtLibraryTarget($$libname).$${QMAKE_EXTENSION_STATICLIB}
    LIBS_PRIVATE += $$staticlib
    PRE_TARGETDEPS += $$staticlib
}

HEADERS += \
    $$ANGLE_DIR/src/common/blocklayout.h \
    $$ANGLE_DIR/src/common/shadervars.h \
    $$ANGLE_DIR/src/common/utilities.h \
    $$ANGLE_DIR/src/common/NativeWindow.h \
    $$ANGLE_DIR/src/libGLESv2/angletypes.h \
    $$ANGLE_DIR/src/libGLESv2/BinaryStream.h \
    $$ANGLE_DIR/src/libGLESv2/Buffer.h \
    $$ANGLE_DIR/src/libGLESv2/Caps.h \
    $$ANGLE_DIR/src/libGLESv2/Context.h \
    $$ANGLE_DIR/src/libGLESv2/Data.h \
    $$ANGLE_DIR/src/libGLESv2/Error.h \
    $$ANGLE_DIR/src/libGLESv2/Fence.h \
    $$ANGLE_DIR/src/libGLESv2/formatutils.h \
    $$ANGLE_DIR/src/libGLESv2/Framebuffer.h \
    $$ANGLE_DIR/src/libGLESv2/FramebufferAttachment.h \
    $$ANGLE_DIR/src/libGLESv2/HandleAllocator.h \
    $$ANGLE_DIR/src/libGLESv2/ImageIndex.h \
    $$ANGLE_DIR/src/libGLESv2/main.h \
    $$ANGLE_DIR/src/libGLESv2/Program.h \
    $$ANGLE_DIR/src/libGLESv2/ProgramBinary.h \
    $$ANGLE_DIR/src/libGLESv2/Query.h \
    $$ANGLE_DIR/src/libGLESv2/queryconversions.h \
    $$ANGLE_DIR/src/libGLESv2/Renderbuffer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/BufferImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/copyimage.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/copyvertex.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/BufferD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/DynamicHLSL.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/HLSLCompiler.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/ImageD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/IndexBuffer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/IndexDataManager.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/MemoryBuffer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/ProgramD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/RenderbufferD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/RendererD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/ShaderD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/TextureD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/TextureStorage.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/TransformFeedbackD3D.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/VertexArrayImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/VertexBuffer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/vertexconversion.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/VertexDataManager.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/FenceImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/generatemip.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/Image.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/imageformats.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexCacheRange.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/loadimage.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/ProgramImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/QueryImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/RenderbufferImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/Renderer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/ShaderExecutable.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/ShaderImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/SwapChain.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/TextureImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/TextureFeedbackImpl.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexDeclarationCache.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/Workarounds.h \
    $$ANGLE_DIR/src/libGLESv2/resource.h \
    $$ANGLE_DIR/src/libGLESv2/ResourceManager.h \
    $$ANGLE_DIR/src/libGLESv2/Sampler.h \
    $$ANGLE_DIR/src/libGLESv2/Shader.h \
    $$ANGLE_DIR/src/libGLESv2/State.h \
    $$ANGLE_DIR/src/libGLESv2/Texture.h \
    $$ANGLE_DIR/src/libGLESv2/TransformFeedback.h \
    $$ANGLE_DIR/src/libGLESv2/Uniform.h \
    $$ANGLE_DIR/src/libGLESv2/validationES.h \
    $$ANGLE_DIR/src/libGLESv2/validationES2.h \
    $$ANGLE_DIR/src/libGLESv2/validationES3.h \
    $$ANGLE_DIR/src/libGLESv2/VertexArray.h \
    $$ANGLE_DIR/src/libGLESv2/VertexAttribute.h \
    $$ANGLE_DIR/src/libGLESv2/vertexconversion.h \
    $$ANGLE_DIR/src/third_party/murmurhash/MurmurHash3.h \

SOURCES += \
    $$ANGLE_DIR/src/common/blocklayout.cpp \
    $$ANGLE_DIR/src/common/mathutil.cpp \
    $$ANGLE_DIR/src/common/utilities.cpp \
    $$ANGLE_DIR/src/third_party/murmurhash/MurmurHash3.cpp \
    $$ANGLE_DIR/src/libGLESv2/angletypes.cpp \
    $$ANGLE_DIR/src/libGLESv2/Buffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/Caps.cpp \
    $$ANGLE_DIR/src/libGLESv2/Context.cpp \
    $$ANGLE_DIR/src/libGLESv2/Data.cpp \
    $$ANGLE_DIR/src/libGLESv2/Error.cpp \
    $$ANGLE_DIR/src/libGLESv2/Fence.cpp \
    $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp \
    $$ANGLE_DIR/src/libGLESv2/Framebuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/FramebufferAttachment.cpp \
    $$ANGLE_DIR/src/libGLESv2/formatutils.cpp \
    $$ANGLE_DIR/src/libGLESv2/HandleAllocator.cpp \
    $$ANGLE_DIR/src/libGLESv2/ImageIndex.cpp \
    $$ANGLE_DIR/src/libGLESv2/libGLESv2.cpp \
    $$ANGLE_DIR/src/libGLESv2/main.cpp \
    $$ANGLE_DIR/src/libGLESv2/Program.cpp \
    $$ANGLE_DIR/src/libGLESv2/ProgramBinary.cpp \
    $$ANGLE_DIR/src/libGLESv2/Query.cpp \
    $$ANGLE_DIR/src/libGLESv2/queryconversions.cpp \
    $$ANGLE_DIR/src/libGLESv2/Renderbuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/ResourceManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/Sampler.cpp \
    $$ANGLE_DIR/src/libGLESv2/Shader.cpp \
    $$ANGLE_DIR/src/libGLESv2/State.cpp \
    $$ANGLE_DIR/src/libGLESv2/Texture.cpp \
    $$ANGLE_DIR/src/libGLESv2/TransformFeedback.cpp \
    $$ANGLE_DIR/src/libGLESv2/Uniform.cpp \
    $$ANGLE_DIR/src/libGLESv2/validationES.cpp \
    $$ANGLE_DIR/src/libGLESv2/validationES2.cpp \
    $$ANGLE_DIR/src/libGLESv2/validationES3.cpp \
    $$ANGLE_DIR/src/libGLESv2/VertexArray.cpp \
    $$ANGLE_DIR/src/libGLESv2/VertexAttribute.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/copyimage.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/loadimage.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/Image.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexRangeCache.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/ProgramImpl.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/RenderbufferImpl.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/Renderer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/BufferD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/DynamicHLSL.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/HLSLCompiler.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/ImageD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/IndexBuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/IndexDataManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/MemoryBuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/ProgramD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/RenderbufferD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/RendererD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/ShaderD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/TextureD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/TextureStorage.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/TransformFeedbackD3D.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/VertexBuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/d3d/VertexDataManager.cpp

angle_d3d11 {
    HEADERS += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Blit11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Buffer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Clear11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Fence11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/formatutils11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Image11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/IndexBuffer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/InputLayoutCache.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/PixelTransfer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Query11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Renderer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/renderer11_utils.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/RenderTarget11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/RenderStateCache.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/ShaderExecutable11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/SwapChain11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/TextureStorage11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/VertexBuffer11.h

    SOURCES += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Blit11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Buffer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Clear11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Fence11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/formatutils11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Image11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/IndexBuffer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/InputLayoutCache.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/PixelTransfer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Query11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/Renderer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/renderer11_utils.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/RenderTarget11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/RenderStateCache.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/ShaderExecutable11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/SwapChain11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/TextureStorage11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/VertexBuffer11.cpp
}

SSE2_SOURCES += $$ANGLE_DIR/src/libGLESv2/renderer/loadimageSSE2.cpp

!winrt {
    HEADERS += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Blit9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Buffer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Fence9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/formatutils9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Image9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/IndexBuffer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Query9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Renderer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/renderer9_utils.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/RenderTarget9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/ShaderExecutable9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/SwapChain9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/TextureStorage9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/VertexBuffer9.h \
        $$ANGLE_DIR/src/third_party/systeminfo/SystemInfo.h

    SOURCES += \
        $$ANGLE_DIR/src/common/win32/NativeWindow.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Blit9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Buffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Fence9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/formatutils9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Image9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/IndexBuffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Query9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/Renderer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/renderer9_utils.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/RenderTarget9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/ShaderExecutable9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/SwapChain9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/TextureStorage9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/VertexBuffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/VertexDeclarationCache.cpp \
        $$ANGLE_DIR/src/third_party/systeminfo/SystemInfo.cpp
} else {
    HEADERS += \
        $$ANGLE_DIR/src/common/winrt/CoreWindowNativeWindow.h \
        $$ANGLE_DIR/src/common/winrt/InspectableNativeWindow.h \
        $$ANGLE_DIR/src/common/winrt/SwapChainPanelNativeWindow.h

    SOURCES += \
        $$ANGLE_DIR/src/common/winrt/CoreWindowNativeWindow.cpp \
        $$ANGLE_DIR/src/common/winrt/InspectableNativeWindow.cpp \
        $$ANGLE_DIR/src/common/winrt/SwapChainPanelNativeWindow.cpp
}

!static {
    DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}.def
    mingw:equals(QT_ARCH, i386): DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}_mingw32.def
}

float_converter.target = float_converter
float_converter.commands = python $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.py \
                > $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp
QMAKE_EXTRA_TARGETS += float_converter

# Generate the shader header files.
SHADER9_INPUT_DIR = $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d9/shaders
SHADER11_INPUT_DIR = $$ANGLE_DIR/src/libGLESv2/renderer/d3d/d3d11/shaders

BLITPS_INPUT = $$SHADER9_INPUT_DIR/Blit.ps
BLITPS.shaders = PS_passthrough PS_luminance PS_componentmask
BLITPS.profile = 2_0

BLITVS_INPUT = $$SHADER9_INPUT_DIR/Blit.vs
BLITVS.shaders = VS_standard VS_flipy
BLITVS.profile = 2_0

BUFFERTOTEXTURE_INPUT = $$SHADER11_INPUT_DIR/BufferToTexture11.hlsl
BUFFERTOTEXTURE.shaders = \
    PS_BufferToTexture_4F PS_BufferToTexture_4I PS_BufferToTexture_4UI \
    VS_BufferToTexture GS_BufferToTexture
BUFFERTOTEXTURE.profile = 4_0

CLEAR_INPUT = $$SHADER11_INPUT_DIR/Clear11.hlsl
CLEAR.shaders = \
    PS_ClearUint PS_ClearSint \
    VS_ClearUint VS_ClearSint
CLEAR.shaders_compat = PS_ClearFloat VS_ClearFloat
CLEAR.profile = 4_0

PASSTHROUGH2D_INPUT = $$SHADER11_INPUT_DIR/Passthrough2D11.hlsl
PASSTHROUGH2D.shaders = \
    PS_PassthroughRGBA2DUI PS_PassthroughRGBA2DI \
    PS_PassthroughRGB2DUI PS_PassthroughRGB2DI \
    PS_PassthroughRG2DUI PS_PassthroughRG2DI \
    PS_PassthroughR2DUI PS_PassthroughR2DI \
    PS_PassthroughDepth2D
PASSTHROUGH2D.shaders_compat = \
    PS_PassthroughRGBA2D PS_PassthroughRGB2D \
    PS_PassthroughRG2D PS_PassthroughR2D \
    PS_PassthroughLum2D PS_PassthroughLumAlpha2D \
    VS_Passthrough2D
PASSTHROUGH2D.profile = 4_0

PASSTHROUGH3D_INPUT = $$SHADER11_INPUT_DIR/Passthrough3D11.hlsl
PASSTHROUGH3D.shaders = \
    PS_PassthroughRGBA3D PS_PassthroughRGBA3DUI PS_PassthroughRGBA3DI \
    PS_PassthroughRGB3D PS_PassthroughRGB3DUI PS_PassthroughRGB3DI \
    PS_PassthroughRG3D PS_PassthroughRG3DUI PS_PassthroughRG3DI \
    PS_PassthroughR3D PS_PassthroughR3DUI PS_PassthroughR3DI \
    PS_PassthroughLum3D PS_PassthroughLumAlpha3D \
    VS_Passthrough3D GS_Passthrough3D
PASSTHROUGH3D.profile = 4_0

SWIZZLE_INPUT = $$SHADER11_INPUT_DIR/Swizzle11.hlsl
SWIZZLE.shaders = \
    PS_SwizzleI2D PS_SwizzleUI2D \
    PS_SwizzleF3D PS_SwizzleI3D PS_SwizzleUI3D \
    PS_SwizzleF2DArray PS_SwizzleI2DArray PS_SwizzleUI2DArray
SWIZZLE.shaders_compat = PS_SwizzleF2D
SWIZZLE.profile = 4_0

angle_d3d11: FXC_JOBS = BUFFERTOTEXTURE CLEAR PASSTHROUGH2D PASSTHROUGH3D SWIZZLE
!winrt: FXC_JOBS += BLITPS BLITVS

for (JOB, FXC_JOBS) {
    INPUT = $${JOB}_INPUT
    OUT_DIR = $$OUT_PWD/$$relative_path($$dirname($$INPUT), $$ANGLE_DIR/src/libGLESv2)/compiled
    SHADERS_COMPAT = $$eval($${JOB}.shaders_compat)
    SHADERS = $$eval($${JOB}.shaders) $$SHADERS_COMPAT
    for(SHADER, SHADERS) {
        TYPE = $$lower($$section(SHADER, _, 0, 0))
        PROFILE = $${TYPE}_$$eval($${JOB}.profile)
        contains(SHADERS_COMPAT, $$SHADER): PROFILE = $${PROFILE}_level_9_1
        fxc_$${SHADER}_$${PROFILE}.commands = $$FXC /nologo /E $${SHADER} /T $${PROFILE} /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
        fxc_$${SHADER}_$${PROFILE}.output = $$OUT_DIR/$$section(SHADER, _, 1)$${TYPE}.h
        fxc_$${SHADER}_$${PROFILE}.input = $$INPUT
        fxc_$${SHADER}_$${PROFILE}.dependency_type = TYPE_C
        fxc_$${SHADER}_$${PROFILE}.variable_out = HEADERS
        fxc_$${SHADER}_$${PROFILE}.CONFIG += target_predeps
        QMAKE_EXTRA_COMPILERS += fxc_$${SHADER}_$${PROFILE}
    }
}

khr_headers.files = $$ANGLE_DIR/include/KHR/khrplatform.h
khr_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/KHR
gles2_headers.files = \
    $$ANGLE_DIR/include/GLES2/gl2.h \
    $$ANGLE_DIR/include/GLES2/gl2ext.h \
    $$ANGLE_DIR/include/GLES2/gl2platform.h
gles2_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES2
gles3_headers.files = \
    $$ANGLE_DIR/include/GLES3/gl3.h \
    $$ANGLE_DIR/include/GLES3/gl3ext.h \
    $$ANGLE_DIR/include/GLES3/gl3platform.h
gles3_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES3
INSTALLS += khr_headers gles2_headers
angle_d3d11: INSTALLS += gles3_headers

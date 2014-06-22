/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_SHADERS)

#include "CustomFilterValidatedProgram.h"

#include "ANGLEWebKitBridge.h"
#include "CustomFilterCompiledProgram.h"
#include "CustomFilterConstants.h"
#include "CustomFilterGlobalContext.h"
#include "CustomFilterProgramInfo.h"
#include "NotImplemented.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

#define SHADER(Src) (#Src) 

typedef HashMap<String, ShDataType> SymbolNameToTypeMap;

static SymbolNameToTypeMap* builtInAttributeNameToTypeMap()
{
    static SymbolNameToTypeMap* nameToTypeMap = 0;
    if (!nameToTypeMap) {
        nameToTypeMap = new SymbolNameToTypeMap;        
        nameToTypeMap->set("a_meshCoord", SH_FLOAT_VEC2);
        nameToTypeMap->set("a_position", SH_FLOAT_VEC4);
        nameToTypeMap->set("a_texCoord", SH_FLOAT_VEC2);
        nameToTypeMap->set("a_triangleCoord", SH_FLOAT_VEC3);
    }
    return nameToTypeMap;
}

static SymbolNameToTypeMap* builtInUniformNameToTypeMap()
{
    static SymbolNameToTypeMap* nameToTypeMap = 0;
    if (!nameToTypeMap) {
        nameToTypeMap = new SymbolNameToTypeMap;
        nameToTypeMap->set("u_meshBox", SH_FLOAT_VEC4);
        nameToTypeMap->set("u_meshSize", SH_FLOAT_VEC2);
        nameToTypeMap->set("u_projectionMatrix", SH_FLOAT_MAT4);
        nameToTypeMap->set("u_textureSize", SH_FLOAT_VEC2);
        nameToTypeMap->set("u_tileSize", SH_FLOAT_VEC2);
    }
    return nameToTypeMap;
}

static bool validateSymbols(const Vector<ANGLEShaderSymbol>& symbols, CustomFilterMeshType meshType)
{
    for (size_t i = 0; i < symbols.size(); ++i) {
        const ANGLEShaderSymbol& symbol = symbols[i];
        switch (symbol.symbolType) {
        case SHADER_SYMBOL_TYPE_ATTRIBUTE: {
            SymbolNameToTypeMap* attributeNameToTypeMap = builtInAttributeNameToTypeMap();
            SymbolNameToTypeMap::iterator builtInAttribute = attributeNameToTypeMap->find(symbol.name);
            if (builtInAttribute == attributeNameToTypeMap->end()) {
                // The author defined a custom attribute.
                // FIXME: Report the validation error.
                // https://bugs.webkit.org/show_bug.cgi?id=74416
                return false;
            }
            if (meshType == MeshTypeAttached && symbol.name == "a_triangleCoord") {
                // a_triangleCoord is only available for detached meshes.
                // FIXME: Report the validation error.
                // https://bugs.webkit.org/show_bug.cgi?id=74416
                return false;
            }
            if (symbol.dataType != builtInAttribute->value) {
                // The author defined one of the built-in attributes with the wrong type.
                // FIXME: Report the validation error.
                // https://bugs.webkit.org/show_bug.cgi?id=74416
                return false;
            }
            break;
        }
        case SHADER_SYMBOL_TYPE_UNIFORM: {
            if (symbol.isSampler()) {
                // FIXME: For now, we restrict shaders with any sampler defined.
                // When we implement texture parameters, we will allow shaders whose samplers are bound to valid textures.
                // We must not allow OpenGL to give unbound samplers a default value of 0 because that references the element texture,
                // which should be inaccessible to the author's shader code.
                // https://bugs.webkit.org/show_bug.cgi?id=96230
                return false;
            }

            SymbolNameToTypeMap* uniformNameToTypeMap = builtInUniformNameToTypeMap();
            SymbolNameToTypeMap::iterator builtInUniform = uniformNameToTypeMap->find(symbol.name);
            if (builtInUniform != uniformNameToTypeMap->end() && (symbol.isArray || symbol.dataType != builtInUniform->value)) {
                // The author defined one of the built-in uniforms with the wrong type.
                // FIXME: Report the validation error.
                // https://bugs.webkit.org/show_bug.cgi?id=74416
                return false;
            }
            break;
        }
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    return true;
}

String CustomFilterValidatedProgram::defaultVertexShaderString()
{
    DEFINE_STATIC_LOCAL(String, vertexShaderString, (ASCIILiteral(SHADER(
        attribute mediump vec4 a_position;
        uniform mediump mat4 u_projectionMatrix;

        void main()
        {
            gl_Position = u_projectionMatrix * a_position;
        }
    ))));
    return vertexShaderString;
}

String CustomFilterValidatedProgram::defaultFragmentShaderString()
{
    DEFINE_STATIC_LOCAL(String, fragmentShaderString, (ASCIILiteral(SHADER(
        void main()
        {
        }
    ))));
    return fragmentShaderString;
}

CustomFilterValidatedProgram::CustomFilterValidatedProgram(CustomFilterGlobalContext* globalContext, const CustomFilterProgramInfo& programInfo)
    : m_programInfo(programInfo)
    , m_isInitialized(false)
{
    platformInit();

    String originalVertexShader = programInfo.vertexShaderString();
    if (originalVertexShader.isNull())
        originalVertexShader = defaultVertexShaderString();

    String originalFragmentShader = programInfo.fragmentShaderString();
    if (originalFragmentShader.isNull())
        originalFragmentShader = defaultFragmentShaderString();

    // Shaders referenced from the CSS mix function use a different validator than regular WebGL shaders. See CustomFilterGlobalContext.h for more details.
    bool blendsElementTexture = (programInfo.programType() == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE);
    ANGLEWebKitBridge* validator = blendsElementTexture ? globalContext->mixShaderValidator() : globalContext->webglShaderValidator();
    String vertexShaderLog, fragmentShaderLog;
    Vector<ANGLEShaderSymbol> symbols;
    bool vertexShaderValid = validator->compileShaderSource(originalVertexShader.utf8().data(), SHADER_TYPE_VERTEX, m_validatedVertexShader, vertexShaderLog, symbols);
    bool fragmentShaderValid = validator->compileShaderSource(originalFragmentShader.utf8().data(), SHADER_TYPE_FRAGMENT, m_validatedFragmentShader, fragmentShaderLog, symbols);
    if (!vertexShaderValid || !fragmentShaderValid) {
        // FIXME: Report the validation errors.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        return;
    }

    if (!validateSymbols(symbols, m_programInfo.meshType())) {
        // FIXME: Report validation errors.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        return;
    }

    // We need to add texture access, blending, and compositing code to shaders that are referenced from the CSS mix function.
    if (blendsElementTexture) {
        rewriteMixVertexShader(symbols);
        rewriteMixFragmentShader();
    }

    m_isInitialized = true;
}

PassRefPtr<CustomFilterCompiledProgram> CustomFilterValidatedProgram::compiledProgram()
{
    return m_compiledProgram;
}

void CustomFilterValidatedProgram::setCompiledProgram(PassRefPtr<CustomFilterCompiledProgram> compiledProgram)
{
    m_compiledProgram = compiledProgram;
}

bool CustomFilterValidatedProgram::needsInputTexture() const
{
    return m_programInfo.programType() == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE
        && m_programInfo.mixSettings().compositeOperator != CompositeClear
        && m_programInfo.mixSettings().compositeOperator != CompositeCopy;
}

void CustomFilterValidatedProgram::rewriteMixVertexShader(const Vector<ANGLEShaderSymbol>& symbols)
{
    ASSERT(m_programInfo.programType() == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE);

    // If the author defined a_texCoord, we can use it to shuttle the texture coordinate to the fragment shader.
    // Note that vertex attributes are read-only in GLSL, so the author could not have changed a_texCoord's value.
    // Also, note that we would have already rejected the shader if the author defined a_texCoord with the wrong type.
    bool texCoordAttributeDefined = false;
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (symbols[i].name == "a_texCoord")
            texCoordAttributeDefined = true;
    }

    if (!texCoordAttributeDefined)
        m_validatedVertexShader.append("attribute mediump vec2 a_texCoord;");

    // During validation, ANGLE renamed the author's "main" function to "css_main".
    // We write our own "main" function and call "css_main" from it.
    // This makes rewriting easy and ensures that our code runs after all author code.
    m_validatedVertexShader.append(SHADER(
        varying mediump vec2 css_v_texCoord;

        void main()
        {
            css_main();
            css_v_texCoord = a_texCoord;
        }
    ));
}

void CustomFilterValidatedProgram::rewriteMixFragmentShader()
{
    ASSERT(m_programInfo.programType() == PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE);

    StringBuilder builder;
    // ANGLE considered these symbols as built-ins during validation under the SH_CSS_SHADERS_SPEC flag.
    // Now, we have to define these symbols in order to make this shader valid GLSL.
    // We define these symbols before the author's shader code, which makes them accessible to author code.
    builder.append(SHADER(
        mediump vec4 css_MixColor = vec4(0.0);
        mediump mat4 css_ColorMatrix = mat4(1.0);
    ));
    builder.append(m_validatedFragmentShader);
    builder.append(blendFunctionString(m_programInfo.mixSettings().blendMode));
    builder.append(compositeFunctionString(m_programInfo.mixSettings().compositeOperator));
    // We define symbols like "css_u_texture" after the author's shader code, which makes them inaccessible to author code.
    // In particular, "css_u_texture" represents the DOM element texture, so it's important to keep it inaccessible to
    // author code for security reasons.
    builder.append(SHADER(
        uniform sampler2D css_u_texture;
        varying mediump vec2 css_v_texCoord;

        void main()
        {
            css_main();
            mediump vec4 originalColor = texture2D(css_u_texture, css_v_texCoord);
            mediump vec4 multipliedColor = clamp(css_ColorMatrix * originalColor, 0.0, 1.0);
            mediump vec4 clampedMixColor = clamp(css_MixColor, 0.0, 1.0);
            mediump vec3 blendedColor = css_BlendColor(multipliedColor.rgb, clampedMixColor.rgb);
            mediump vec3 weightedColor = (1.0 - multipliedColor.a) * clampedMixColor.rgb + multipliedColor.a * blendedColor;
            gl_FragColor = css_Composite(multipliedColor.rgb, multipliedColor.a, weightedColor.rgb, clampedMixColor.a);
        }
    ));
    m_validatedFragmentShader = builder.toString();
}

String CustomFilterValidatedProgram::blendFunctionString(BlendMode blendMode)
{
    // Implemented using the same symbol names as the Compositing and Blending spec:
    // https://dvcs.w3.org/hg/FXTF/rawfile/tip/compositing/index.html#blendingnormal
    // Cs: is the source color in css_BlendColor() and the source color component in css_BlendComponent()
    // Cb: is the backdrop color in css_BlendColor() and the backdrop color component in css_BlendComponent()
    const char* blendColorExpression = "vec3(css_BlendComponent(Cb.r, Cs.r), css_BlendComponent(Cb.g, Cs.g), css_BlendComponent(Cb.b, Cs.b))";
    const char* blendComponentExpression = "Co = 0.0;";
    bool needsLuminosityHelperFunctions = false;
    bool needsSaturationHelperFunctions = false;
    String blendFunctionString;
    switch (blendMode) {
    case BlendModeNormal:
        blendColorExpression = "Cs";
        break;
    case BlendModeMultiply:
        blendColorExpression = "Cs * Cb";
        break;
    case BlendModeScreen:
        blendColorExpression = "Cb + Cs - (Cb * Cs)";
        break;
    case BlendModeDarken:
        blendColorExpression = "min(Cb, Cs)";
        break;
    case BlendModeLighten:
        blendColorExpression = "max(Cb, Cs)";
        break;
    case BlendModeDifference:
        blendColorExpression = "abs(Cb - Cs)";
        break;
    case BlendModeExclusion:
        blendColorExpression = "Cb + Cs - 2.0 * Cb * Cs";
        break;
    case BlendModeOverlay:
        /*
            Co = HardLight(Cs, Cb)
               = if(Cb <= 0.5)
                     Multiply(Cs, 2 x Cb)
                 else
                     Screen(Cs, 2 x Cb - 1)
               = if(Cb <= 0.5)
                     Cs x (2 x Cb)
                 else
                     Cs + (2 x Cb - 1) - (Cs x (2 x Cb - 1))
        */
        blendComponentExpression = SHADER(
            if (Cb <= 0.5)
                Co = Cs * (2.0 * Cb);
            else
                Co = Cs + (2.0 * Cb - 1.0) - (Cs * (2.0 * Cb - 1.0));
        );
        break;
    case BlendModeColorDodge:
        /*
            Co = if(Cs < 1)
                     min(1, Cb / (1 - Cs))
                 else
                     1
        */
        blendComponentExpression = SHADER(
            if (Cs < 1.0)
                Co = min(1.0, Cb / (1.0 - Cs));
            else
                Co = 1.0;
        );
        break;
    case BlendModeColorBurn:
        /*
            Co = if(Cs > 0)
                     1 - min(1, (1 - Cb) / Cs)
                 else
                     0
        */
        blendComponentExpression = SHADER(
            if (Cs > 0.0)
                Co = 1.0 - min(1.0, (1.0 - Cb) / Cs);
            else
                Co = 0.0;
        );
        break;
    case BlendModeHardLight:
        /*
            Co = if(Cs <= 0.5)
                     Multiply(Cb, 2 x Cs)
                 else
                     Screen(Cb, 2 x Cs -1)
               = if(Cs <= 0.5)
                     Cb x (2 x Cs)
                 else
                     Cb + (2 x Cs - 1) - (Cb x (2 x Cs - 1))
        */
        blendComponentExpression = SHADER(
            if (Cs <= 0.5)
                Co = Cb * (2.0 * Cs);
            else
                Co = Cb + (2.0 * Cs - 1.0) - (Cb * (2.0 * Cs - 1.0));
        );
        break;
    case BlendModeSoftLight:
        /*
            Co = if(Cs <= 0.5)
                     Cb - (1 - 2 x Cs) x Cb x (1 - Cb)
                 else
                     Cb + (2 x Cs - 1) x (D(Cb) - Cb)

            with

            D(Cb) = if(Cb <= 0.25)
                        (16 * Cb - 12) x Cb + 4) x Cb
                    else
                        sqrt(Cb)
        */
        blendComponentExpression = SHADER(
            mediump float D;
            if (Cb <= 0.25)
                D = ((16.0 * Cb - 12.0) * Cb + 4.0) * Cb;
            else
                D = sqrt(Cb);

            if (Cs <= 0.5)
                Co = Cb - (1.0 - 2.0 * Cs) * Cb * (1.0 - Cb);
            else
                Co = Cb + (2.0 * Cs - 1.0) * (D - Cb);
        );
        break;
    case BlendModeColor:
        needsLuminosityHelperFunctions = true;
        blendColorExpression = "css_SetLum(Cs, css_Lum(Cb))";
        break;
    case BlendModeLuminosity:
        needsLuminosityHelperFunctions = true;
        blendColorExpression = "css_SetLum(Cb, css_Lum(Cs))";
        break;
    case BlendModeHue:
        needsLuminosityHelperFunctions = true;
        needsSaturationHelperFunctions = true;
        blendColorExpression = "css_SetLum(css_SetSat(Cs, css_Sat(Cb)), css_Lum(Cb))";
        break;
    case BlendModeSaturation:
        needsLuminosityHelperFunctions = true;
        needsSaturationHelperFunctions = true;
        blendColorExpression = "css_SetLum(css_SetSat(Cb, css_Sat(Cs)), css_Lum(Cb))";
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (needsLuminosityHelperFunctions) {
        blendFunctionString.append(SHADER(
            mediump float css_Lum(mediump vec3 C)
            {
                return 0.3 * C.r + 0.59 * C.g + 0.11 * C.b;
            }
            mediump vec3 css_ClipColor(mediump vec3 C)
            {
                mediump float L = css_Lum(C);
                mediump float n = min(min(C.r, C.g), C.b);
                mediump float x = max(max(C.r, C.g), C.b);
                if (n < 0.0)
                    C = L + (((C - L) * L) / (L - n));
                if (x > 1.0)
                    C = L + (((C - L) * (1.0 - L) / (x - L)));
                return C;
            }
            mediump vec3 css_SetLum(mediump vec3 C, mediump float l)
            {
                C += l - css_Lum(C);
                return css_ClipColor(C);
            }
        ));
    }

    if (needsSaturationHelperFunctions) {
        blendFunctionString.append(SHADER(
            mediump float css_Sat(mediump vec3 C)
            {
                mediump float cMin = min(min(C.r, C.g), C.b);
                mediump float cMax = max(max(C.r, C.g), C.b);
                return cMax - cMin;
            }
            void css_SetSatHelper(inout mediump float cMin, inout mediump float cMid, inout mediump float cMax, mediump float s)
            {
                if (cMax > cMin) {
                    cMid = (((cMid - cMin) * s) / (cMax - cMin));
                    cMax = s;
                } else
                    cMid = cMax = 0.0;
                cMin = 0.0;
            }
            mediump vec3 css_SetSat(mediump vec3 C, mediump float s)
            {
                if (C.r <= C.g) {
                    if (C.g <= C.b)
                        css_SetSatHelper(C.r, C.g, C.b, s);
                    else {
                        if (C.r <= C.b)
                            css_SetSatHelper(C.r, C.b, C.g, s);
                        else
                            css_SetSatHelper(C.b, C.r, C.g, s);
                    }
                } else {
                    if (C.r <= C.b)
                        css_SetSatHelper(C.g, C.r, C.b, s);
                    else {
                        if (C.g <= C.b)
                            css_SetSatHelper(C.g, C.b, C.r, s);
                        else
                            css_SetSatHelper(C.b, C.g, C.r, s);
                    }
                }
                return C;
            }
        ));
    }

    blendFunctionString.append(String::format(SHADER(
        mediump float css_BlendComponent(mediump float Cb, mediump float Cs)
        {
            mediump float Co;
            %s
            return Co;
        }
        mediump vec3 css_BlendColor(mediump vec3 Cb, mediump vec3 Cs)
        {
            return %s;
        }
    ), blendComponentExpression, blendColorExpression));

    return blendFunctionString;
}

String CustomFilterValidatedProgram::compositeFunctionString(CompositeOperator compositeOperator)
{
    // Use the same symbol names as the Compositing and Blending spec:
    // https://dvcs.w3.org/hg/FXTF/rawfile/tip/compositing/index.html#blendingnormal
    // Cs: is the source color
    // Cb: is the backdrop color
    // as: is the source alpha
    // ab: is the backdrop alpha
    // Fa: is defined by the Porter Duff operator in use
    // Fb: is defined by the Porter Duff operator in use
    const char* Fa = 0;
    const char* Fb = 0;
    switch (compositeOperator) {
    case CompositeSourceAtop:
        Fa = "ab";
        Fb = "1.0 - as";
        break;
    case CompositeClear:
        Fa = "0.0";
        Fb = "0.0";
        break;
    case CompositeCopy:
        Fa = "1.0";
        Fb = "0.0";
        break;
    case CompositeSourceOver:
        Fa = "1.0";
        Fb = "1.0 - as";
        break;
    case CompositeSourceIn:
        Fa = "ab";
        Fb = "0.0";
        break;
    case CompositeSourceOut:
        Fa = "1.0 - ab";
        Fb = "0.0";
        break;
    case CompositeDestinationOver:
        Fa = "1.0 - ab";
        Fb = "1.0";
        break;
    case CompositeDestinationIn:
        Fa = "0.0";
        Fb = "as";
        break;
    case CompositeDestinationOut:
        Fa = "0.0";
        Fb = "1.0 - as";
        break;
    case CompositeDestinationAtop:
        Fa = "1.0 - ab";
        Fb = "as";
        break;
    case CompositeXOR:
        Fa = "1.0 - ab";
        Fb = "1.0 - as";
        break;
    case CompositePlusLighter:
        notImplemented();
        return String();
    default:
        // The CSS parser should not have accepted any other composite operators.
        ASSERT_NOT_REACHED();
        return String();
    }

    ASSERT(Fa && Fb);
    // Use the general formula for compositing, lifted from the spec:
    // https://dvcs.w3.org/hg/FXTF/rawfile/tip/compositing/index.html#generalformula
    return String::format(SHADER(
        mediump vec4 css_Composite(mediump vec3 Cb, mediump float ab, mediump vec3 Cs, mediump float as)
        {
            mediump float Fa = %s;
            mediump float Fb = %s;
            return vec4(as * Fa * Cs + ab * Fb * Cb, as * Fa + ab * Fb); 
        }
    ), Fa, Fb);
}
    
CustomFilterValidatedProgram::~CustomFilterValidatedProgram()
{
    platformDestroy();
}

CustomFilterProgramInfo CustomFilterValidatedProgram::validatedProgramInfo() const
{
    ASSERT(m_isInitialized);
    return CustomFilterProgramInfo(m_validatedVertexShader, m_validatedFragmentShader, m_programInfo.programType(), m_programInfo.mixSettings(), m_programInfo.meshType());
}

#if !PLATFORM(BLACKBERRY) && !USE(TEXTURE_MAPPER)
void CustomFilterValidatedProgram::platformInit()
{
}

void CustomFilterValidatedProgram::platformDestroy()
{
}
#endif

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

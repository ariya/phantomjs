#include "precompiled.h"
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image11.h: Implements the rx::Image11 class, which acts as the interface to
// the actual underlying resources of a Texture

#include "libGLESv2/renderer/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d11/Image11.h"
#include "libGLESv2/renderer/d3d11/TextureStorage11.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"

#include "libGLESv2/main.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/renderer/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/generatemip.h"

namespace rx
{

Image11::Image11()
{
    mStagingTexture = NULL;
    mRenderer = NULL;
    mDXGIFormat = DXGI_FORMAT_UNKNOWN;
}

Image11::~Image11()
{
    if (mStagingTexture)
    {
        mStagingTexture->Release();
    }
}

Image11 *Image11::makeImage11(Image *img)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::Image11*, img));
    return static_cast<rx::Image11*>(img);
}

void Image11::generateMipmap(Image11 *dest, Image11 *src)
{
    ASSERT(src->getDXGIFormat() == dest->getDXGIFormat());
    ASSERT(src->getWidth() == 1 || src->getWidth() / 2 == dest->getWidth());
    ASSERT(src->getHeight() == 1 || src->getHeight() / 2 == dest->getHeight());

    D3D11_MAPPED_SUBRESOURCE destMapped, srcMapped;
    dest->map(D3D11_MAP_WRITE, &destMapped);
    src->map(D3D11_MAP_READ, &srcMapped);

    const unsigned char *sourceData = reinterpret_cast<const unsigned char*>(srcMapped.pData);
    unsigned char *destData = reinterpret_cast<unsigned char*>(destMapped.pData);

    if (sourceData && destData)
    {
        switch (src->getDXGIFormat())
        {
          case DXGI_FORMAT_R8G8B8A8_UNORM:
          case DXGI_FORMAT_B8G8R8A8_UNORM:
            GenerateMip<R8G8B8A8>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_A8_UNORM:
            GenerateMip<A8>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R8_UNORM:
            GenerateMip<R8>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R32G32B32A32_FLOAT:
            GenerateMip<A32B32G32R32F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R32G32B32_FLOAT:
            GenerateMip<R32G32B32F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R16G16B16A16_FLOAT:
            GenerateMip<A16B16G16R16F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R8G8_UNORM:
            GenerateMip<R8G8>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R16_FLOAT:
            GenerateMip<R16F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R16G16_FLOAT:
            GenerateMip<R16G16F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R32_FLOAT:
            GenerateMip<R32F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          case DXGI_FORMAT_R32G32_FLOAT:
            GenerateMip<R32G32F>(src->getWidth(), src->getHeight(), sourceData, srcMapped.RowPitch, destData, destMapped.RowPitch);
            break;
          default:
            UNREACHABLE();
            break;
        }

        dest->unmap();
        src->unmap();
    }

    dest->markDirty();
}

static bool FormatRequiresInitialization(DXGI_FORMAT dxgiFormat, GLenum internalFormat)
{
    return (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM && gl::GetAlphaSize(internalFormat) == 0) ||
           (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT && gl::GetAlphaSize(internalFormat) == 0);
}

bool Image11::isDirty() const
{
    return ((mStagingTexture || FormatRequiresInitialization(mDXGIFormat, mInternalFormat)) && mDirty);
}

bool Image11::updateSurface(TextureStorageInterface2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    TextureStorage11_2D *storage11 = TextureStorage11_2D::makeTextureStorage11_2D(storage->getStorageInstance());
    return storage11->updateSubresourceLevel(getStagingTexture(), getStagingSubresource(), level, 0, xoffset, yoffset, width, height);
}

bool Image11::updateSurface(TextureStorageInterfaceCube *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    TextureStorage11_Cube *storage11 = TextureStorage11_Cube::makeTextureStorage11_Cube(storage->getStorageInstance());
    return storage11->updateSubresourceLevel(getStagingTexture(), getStagingSubresource(), level, face, xoffset, yoffset, width, height);
}

bool Image11::redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, bool forceRelease)
{
    if (mWidth != width ||
        mHeight != height ||
        mInternalFormat != internalformat ||
        forceRelease)
    {
        mRenderer = Renderer11::makeRenderer11(renderer);

        mWidth = width;
        mHeight = height;
        mInternalFormat = internalformat;
        // compute the d3d format that will be used
        mDXGIFormat = gl_d3d11::ConvertTextureFormat(internalformat, mRenderer->getFeatureLevel());
        mActualFormat = d3d11_gl::ConvertTextureInternalFormat(mDXGIFormat);

        if (mStagingTexture)
        {
            mStagingTexture->Release();
            mStagingTexture = NULL;
        }
        
        return true;
    }

    return false;
}

bool Image11::isRenderableFormat() const
{
    return TextureStorage11::IsTextureFormatRenderable(mDXGIFormat);
}

DXGI_FORMAT Image11::getDXGIFormat() const
{
    // this should only happen if the image hasn't been redefined first
    // which would be a bug by the caller
    ASSERT(mDXGIFormat != DXGI_FORMAT_UNKNOWN);

    return mDXGIFormat;
}

// Store the pixel rectangle designated by xoffset,yoffset,width,height with pixels stored as format/type at input
// into the target pixel rectangle.
void Image11::loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                       GLint unpackAlignment, const void *input)
{
    D3D11_MAPPED_SUBRESOURCE mappedImage;
    HRESULT result = map(D3D11_MAP_WRITE, &mappedImage);
    if (FAILED(result))
    {
        ERR("Could not map image for loading.");
        return;
    }
    
    GLsizei inputPitch = gl::ComputePitch(width, mInternalFormat, unpackAlignment);
    size_t pixelSize = d3d11::ComputePixelSizeBits(mDXGIFormat) / 8;
    void* offsetMappedData = (void*)((BYTE *)mappedImage.pData + (yoffset * mappedImage.RowPitch + xoffset * pixelSize));

    switch (mInternalFormat)
    {
      case GL_ALPHA8_EXT:
        if (mRenderer->getFeatureLevel() >= D3D_FEATURE_LEVEL_10_0)
            loadAlphaDataToNative(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        else
            loadAlphaDataToBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE8_EXT:
        loadLuminanceDataToNativeOrBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData, false);
        break;
      case GL_ALPHA32F_EXT:
        loadAlphaFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE32F_EXT:
        loadLuminanceFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_ALPHA16F_EXT:
        loadAlphaHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE16F_EXT:
        loadLuminanceHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE8_ALPHA8_EXT:
        loadLuminanceAlphaDataToNativeOrBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData, false);
        break;
      case GL_LUMINANCE_ALPHA32F_EXT:
        loadLuminanceAlphaFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE_ALPHA16F_EXT:
        loadLuminanceAlphaHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB8_OES:
        loadRGBUByteDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB565:
        loadRGB565DataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGBA8_OES:
        loadRGBAUByteDataToNative(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGBA4:
        loadRGBA4444DataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB5_A1:
        loadRGBA5551DataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_BGRA8_EXT:
        loadBGRADataToBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB32F_EXT:
        loadRGBFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB16F_EXT:
        loadRGBHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGBA32F_EXT:
        loadRGBAFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGBA16F_EXT:
        loadRGBAHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      default: UNREACHABLE(); 
    }

    unmap();
}

void Image11::loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                const void *input)
{
    ASSERT(xoffset % 4 == 0);
    ASSERT(yoffset % 4 == 0);

    D3D11_MAPPED_SUBRESOURCE mappedImage;
    HRESULT result = map(D3D11_MAP_WRITE, &mappedImage);
    if (FAILED(result))
    {
        ERR("Could not map image for loading.");
        return;
    }

    // Size computation assumes a 4x4 block compressed texture format
    size_t blockSize = d3d11::ComputeBlockSizeBits(mDXGIFormat) / 8;
    void* offsetMappedData = (void*)((BYTE *)mappedImage.pData + ((yoffset / 4) * mappedImage.RowPitch + (xoffset / 4) * blockSize));

    GLsizei inputSize = gl::ComputeCompressedSize(width, height, mInternalFormat);
    GLsizei inputPitch = gl::ComputeCompressedPitch(width, mInternalFormat);
    int rows = inputSize / inputPitch;
    for (int i = 0; i < rows; ++i)
    {
        memcpy((void*)((BYTE*)offsetMappedData + i * mappedImage.RowPitch), (void*)((BYTE*)input + i * inputPitch), inputPitch);
    }

    unmap();
}

void Image11::copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source)
{
    gl::Renderbuffer *colorbuffer = source->getReadColorbuffer();

    if (colorbuffer && colorbuffer->getActualFormat() == (GLuint)mActualFormat)
    {
        // No conversion needed-- use copyback fastpath
        ID3D11Texture2D *colorBufferTexture = NULL;
        unsigned int subresourceIndex = 0;

        if (mRenderer->getRenderTargetResource(colorbuffer, &subresourceIndex, &colorBufferTexture))
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            colorBufferTexture->GetDesc(&textureDesc);

            ID3D11Device *device = mRenderer->getDevice();
            ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

            ID3D11Texture2D* srcTex = NULL;
            if (textureDesc.SampleDesc.Count > 1)
            {
                D3D11_TEXTURE2D_DESC resolveDesc;
                resolveDesc.Width = textureDesc.Width;
                resolveDesc.Height = textureDesc.Height;
                resolveDesc.MipLevels = 1;
                resolveDesc.ArraySize = 1;
                resolveDesc.Format = textureDesc.Format;
                resolveDesc.SampleDesc.Count = 1;
                resolveDesc.SampleDesc.Quality = 0;
                resolveDesc.Usage = D3D11_USAGE_DEFAULT;
                resolveDesc.BindFlags = 0;
                resolveDesc.CPUAccessFlags = 0;
                resolveDesc.MiscFlags = 0;

                HRESULT result = device->CreateTexture2D(&resolveDesc, NULL, &srcTex);
                if (FAILED(result))
                {
                    ERR("Failed to create resolve texture for Image11::copy, HRESULT: 0x%X.", result);
                    return;
                }

                deviceContext->ResolveSubresource(srcTex, 0, colorBufferTexture, subresourceIndex, textureDesc.Format);
                subresourceIndex = 0;
            }
            else
            {
                srcTex = colorBufferTexture;
                srcTex->AddRef();
            }

            D3D11_BOX srcBox;
            srcBox.left = x;
            srcBox.right = x + width;
            srcBox.top = y;
            srcBox.bottom = y + height;
            srcBox.front = 0;
            srcBox.back = 1;

            deviceContext->CopySubresourceRegion(mStagingTexture, 0, xoffset, yoffset, 0, srcTex, subresourceIndex, &srcBox);

            srcTex->Release();
            colorBufferTexture->Release();
        }
    }
    else
    {
        // This format requires conversion, so we must copy the texture to staging and manually convert via readPixels
        D3D11_MAPPED_SUBRESOURCE mappedImage;
        HRESULT result = map(D3D11_MAP_WRITE, &mappedImage);

        // determine the offset coordinate into the destination buffer
        GLsizei rowOffset = gl::ComputePixelSize(mActualFormat) * xoffset;
        void *dataOffset = static_cast<unsigned char*>(mappedImage.pData) + mappedImage.RowPitch * yoffset + rowOffset;

        mRenderer->readPixels(source, x, y, width, height, gl::ExtractFormat(mInternalFormat), 
                              gl::ExtractType(mInternalFormat), mappedImage.RowPitch, false, 4, dataOffset);

        unmap();
    }
}

ID3D11Texture2D *Image11::getStagingTexture()
{
    createStagingTexture();

    return mStagingTexture;
}

unsigned int Image11::getStagingSubresource()
{
    createStagingTexture();

    return mStagingSubresource;
}

template <typename T, size_t N>
static void setDefaultData(ID3D11DeviceContext *deviceContext, ID3D11Texture2D *texture, UINT subresource,
                           GLsizei width, GLsizei height, const T (&defaultData)[N])
{
    D3D11_MAPPED_SUBRESOURCE map;
    deviceContext->Map(texture, subresource, D3D11_MAP_WRITE, 0, &map);

    unsigned char* ptr = reinterpret_cast<unsigned char*>(map.pData);
    size_t pixelSize = sizeof(T) * N;

    for (GLsizei y = 0; y < height; y++)
    {
        for (GLsizei x = 0; x < width; x++)
        {
            memcpy(ptr + (y * map.RowPitch) + (x * pixelSize), defaultData, pixelSize);
        }
    }

    deviceContext->Unmap(texture, subresource);
}

void Image11::createStagingTexture()
{
    if (mStagingTexture)
    {
        return;
    }

    ID3D11Texture2D *newTexture = NULL;
    int lodOffset = 1;
    const DXGI_FORMAT dxgiFormat = getDXGIFormat();
    ASSERT(!d3d11::IsDepthStencilFormat(dxgiFormat)); // We should never get here for depth textures

    if (mWidth != 0 && mHeight != 0)
    {
        GLsizei width = mWidth;
        GLsizei height = mHeight;

        // adjust size if needed for compressed textures
        gl::MakeValidSize(false, d3d11::IsCompressed(dxgiFormat), &width, &height, &lodOffset);
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = lodOffset + 1;
        desc.ArraySize = 1;
        desc.Format = dxgiFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &newTexture);

        if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            return gl::error(GL_OUT_OF_MEMORY);
        }
    }

    mStagingTexture = newTexture;
    mStagingSubresource = D3D11CalcSubresource(lodOffset, 0, lodOffset + 1);
    mDirty = false;

    if (mDXGIFormat == DXGI_FORMAT_R8G8B8A8_UNORM && gl::GetAlphaSize(mInternalFormat) == 0)
    {
        unsigned char defaultPixel[4] = { 0, 0, 0, 255 };
        setDefaultData(mRenderer->getDeviceContext(), mStagingTexture, mStagingSubresource, mWidth, mHeight, defaultPixel);
    }
    else if (mDXGIFormat == DXGI_FORMAT_R32G32B32A32_FLOAT && gl::GetAlphaSize(mInternalFormat) == 0)
    {
        float defaultPixel[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        setDefaultData(mRenderer->getDeviceContext(), mStagingTexture, mStagingSubresource, mWidth, mHeight, defaultPixel);
    }
}

HRESULT Image11::map(D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE *map)
{
    createStagingTexture();

    HRESULT result = E_FAIL;

    if (mStagingTexture)
    {
        ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();
        result = deviceContext->Map(mStagingTexture, mStagingSubresource, mapType, 0, map);

        // this can fail if the device is removed (from TDR)
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
        }
        else if (SUCCEEDED(result))
        {
            mDirty = true;
        }
    }

    return result;
}

void Image11::unmap()
{
    if (mStagingTexture)
    {
        ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();
        deviceContext->Unmap(mStagingTexture, mStagingSubresource);
    }
}

}

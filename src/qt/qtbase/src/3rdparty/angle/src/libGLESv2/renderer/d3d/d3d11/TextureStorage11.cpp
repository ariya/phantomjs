//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage11.cpp: Implements the abstract rx::TextureStorage11 class and its concrete derived
// classes TextureStorage11_2D and TextureStorage11_Cube, which act as the interface to the D3D11 texture.

#include "libGLESv2/renderer/d3d/d3d11/TextureStorage11.h"

#include <tuple>

#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/RenderTarget11.h"
#include "libGLESv2/renderer/d3d/d3d11/SwapChain11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/Blit11.h"
#include "libGLESv2/renderer/d3d/d3d11/formatutils11.h"
#include "libGLESv2/renderer/d3d/d3d11/Image11.h"
#include "libGLESv2/renderer/d3d/MemoryBuffer.h"
#include "libGLESv2/renderer/d3d/TextureD3D.h"
#include "libGLESv2/main.h"
#include "libGLESv2/ImageIndex.h"

#include "common/utilities.h"

namespace rx
{

TextureStorage11::SwizzleCacheValue::SwizzleCacheValue()
    : swizzleRed(GL_NONE), swizzleGreen(GL_NONE), swizzleBlue(GL_NONE), swizzleAlpha(GL_NONE)
{
}

TextureStorage11::SwizzleCacheValue::SwizzleCacheValue(GLenum red, GLenum green, GLenum blue, GLenum alpha)
    : swizzleRed(red), swizzleGreen(green), swizzleBlue(blue), swizzleAlpha(alpha)
{
}

bool TextureStorage11::SwizzleCacheValue::operator==(const SwizzleCacheValue &other) const
{
    return swizzleRed == other.swizzleRed &&
           swizzleGreen == other.swizzleGreen &&
           swizzleBlue == other.swizzleBlue &&
           swizzleAlpha == other.swizzleAlpha;
}

bool TextureStorage11::SwizzleCacheValue::operator!=(const SwizzleCacheValue &other) const
{
    return !(*this == other);
}

TextureStorage11::SRVKey::SRVKey(int baseLevel, int mipLevels, bool swizzle)
    : baseLevel(baseLevel), mipLevels(mipLevels), swizzle(swizzle)
{
}

bool TextureStorage11::SRVKey::operator<(const SRVKey &rhs) const
{
    return std::tie(baseLevel, mipLevels, swizzle) < std::tie(rhs.baseLevel, rhs.mipLevels, rhs.swizzle);
}

TextureStorage11::TextureStorage11(Renderer11 *renderer, UINT bindFlags)
    : mBindFlags(bindFlags),
      mTopLevel(0),
      mMipLevels(0),
      mInternalFormat(GL_NONE),
      mTextureFormat(DXGI_FORMAT_UNKNOWN),
      mShaderResourceFormat(DXGI_FORMAT_UNKNOWN),
      mRenderTargetFormat(DXGI_FORMAT_UNKNOWN),
      mDepthStencilFormat(DXGI_FORMAT_UNKNOWN),
      mTextureWidth(0),
      mTextureHeight(0),
      mTextureDepth(0)
{
    mRenderer = Renderer11::makeRenderer11(renderer);

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mLevelSRVs[i] = NULL;
    }
}

TextureStorage11::~TextureStorage11()
{
    for (unsigned int level = 0; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        SafeRelease(mLevelSRVs[level]);
    }

    for (SRVCache::iterator i = mSrvCache.begin(); i != mSrvCache.end(); i++)
    {
        SafeRelease(i->second);
    }
    mSrvCache.clear();
}

TextureStorage11 *TextureStorage11::makeTextureStorage11(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11*, storage));
    return static_cast<TextureStorage11*>(storage);
}

DWORD TextureStorage11::GetTextureBindFlags(GLenum internalFormat, bool renderTarget)
{
    UINT bindFlags = 0;

    const d3d11::TextureFormat &formatInfo = d3d11::GetTextureFormatInfo(internalFormat);
    if (formatInfo.srvFormat != DXGI_FORMAT_UNKNOWN)
    {
        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (formatInfo.dsvFormat != DXGI_FORMAT_UNKNOWN)
    {
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    if (formatInfo.rtvFormat != DXGI_FORMAT_UNKNOWN && renderTarget)
    {
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    }

    return bindFlags;
}

UINT TextureStorage11::getBindFlags() const
{
    return mBindFlags;
}

int TextureStorage11::getTopLevel() const
{
    return mTopLevel;
}

bool TextureStorage11::isRenderTarget() const
{
    return (mBindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_DEPTH_STENCIL)) != 0;
}

bool TextureStorage11::isManaged() const
{
    return false;
}

int TextureStorage11::getLevelCount() const
{
    return mMipLevels - mTopLevel;
}

int TextureStorage11::getLevelWidth(int mipLevel) const
{
    return std::max(static_cast<int>(mTextureWidth) >> mipLevel, 1);
}

int TextureStorage11::getLevelHeight(int mipLevel) const
{
    return std::max(static_cast<int>(mTextureHeight) >> mipLevel, 1);
}

int TextureStorage11::getLevelDepth(int mipLevel) const
{
    return std::max(static_cast<int>(mTextureDepth) >> mipLevel, 1);
}

UINT TextureStorage11::getSubresourceIndex(const gl::ImageIndex &index) const
{
    UINT mipSlice = static_cast<UINT>(index.mipIndex + mTopLevel);
    UINT arraySlice = static_cast<UINT>(index.hasLayer() ? index.layerIndex : 0);
    UINT subresource = D3D11CalcSubresource(mipSlice, arraySlice, mMipLevels);
    ASSERT(subresource != std::numeric_limits<UINT>::max());
    return subresource;
}

gl::Error TextureStorage11::getSRV(const gl::SamplerState &samplerState, ID3D11ShaderResourceView **outSRV)
{
    bool swizzleRequired = samplerState.swizzleRequired();
    bool mipmapping = gl::IsMipmapFiltered(samplerState);
    unsigned int mipLevels = mipmapping ? (samplerState.maxLevel - samplerState.baseLevel) : 1;

    // Make sure there's 'mipLevels' mipmap levels below the base level (offset by the top level,  which corresponds to GL level 0)
    mipLevels = std::min(mipLevels, mMipLevels - mTopLevel - samplerState.baseLevel);

    if (swizzleRequired)
    {
        verifySwizzleExists(samplerState.swizzleRed, samplerState.swizzleGreen, samplerState.swizzleBlue, samplerState.swizzleAlpha);
    }

    SRVKey key(samplerState.baseLevel, mipLevels, swizzleRequired);
    SRVCache::const_iterator iter = mSrvCache.find(key);
    if (iter != mSrvCache.end())
    {
        *outSRV = iter->second;
    }
    else
    {
        ID3D11Resource *texture = NULL;
        if (swizzleRequired)
        {
            gl::Error error = getSwizzleTexture(&texture);
            if (error.isError())
            {
                return error;
            }
        }
        else
        {
            gl::Error error = getResource(&texture);
            if (error.isError())
            {
                return error;
            }
        }

        ID3D11ShaderResourceView *srv = NULL;
        DXGI_FORMAT format = (swizzleRequired ? mSwizzleShaderResourceFormat : mShaderResourceFormat);
        gl::Error error = createSRV(samplerState.baseLevel, mipLevels, format, texture, &srv);
        if (error.isError())
        {
            return error;
        }

        mSrvCache.insert(std::make_pair(key, srv));
        *outSRV = srv;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11::getSRVLevel(int mipLevel, ID3D11ShaderResourceView **outSRV)
{
    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());

    if (!mLevelSRVs[mipLevel])
    {
        ID3D11Resource *resource = NULL;
        gl::Error error = getResource(&resource);
        if (error.isError())
        {
            return error;
        }

        error = createSRV(mipLevel, 1, mShaderResourceFormat, resource, &mLevelSRVs[mipLevel]);
        if (error.isError())
        {
            return error;
        }
    }

    *outSRV = mLevelSRVs[mipLevel];

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11::generateSwizzles(GLenum swizzleRed, GLenum swizzleGreen, GLenum swizzleBlue, GLenum swizzleAlpha)
{
    SwizzleCacheValue swizzleTarget(swizzleRed, swizzleGreen, swizzleBlue, swizzleAlpha);
    for (int level = 0; level < getLevelCount(); level++)
    {
        // Check if the swizzle for this level is out of date
        if (mSwizzleCache[level] != swizzleTarget)
        {
            // Need to re-render the swizzle for this level
            ID3D11ShaderResourceView *sourceSRV = NULL;
            gl::Error error = getSRVLevel(level, &sourceSRV);
            if (error.isError())
            {
                return error;
            }

            ID3D11RenderTargetView *destRTV = NULL;
            error = getSwizzleRenderTarget(level, &destRTV);
            if (error.isError())
            {
                return error;
            }

            gl::Extents size(getLevelWidth(level), getLevelHeight(level), getLevelDepth(level));

            Blit11 *blitter = mRenderer->getBlitter();

            error = blitter->swizzleTexture(sourceSRV, destRTV, size, swizzleRed, swizzleGreen, swizzleBlue, swizzleAlpha);
            if (error.isError())
            {
                return error;
            }

            mSwizzleCache[level] = swizzleTarget;
        }
    }

    return gl::Error(GL_NO_ERROR);
}

void TextureStorage11::invalidateSwizzleCacheLevel(int mipLevel)
{
    if (mipLevel >= 0 && static_cast<unsigned int>(mipLevel) < ArraySize(mSwizzleCache))
    {
        // The default constructor of SwizzleCacheValue has GL_NONE for all channels which is not a
        // valid swizzle combination
        mSwizzleCache[mipLevel] = SwizzleCacheValue();
    }
}

void TextureStorage11::invalidateSwizzleCache()
{
    for (unsigned int mipLevel = 0; mipLevel < ArraySize(mSwizzleCache); mipLevel++)
    {
        invalidateSwizzleCacheLevel(mipLevel);
    }
}

gl::Error TextureStorage11::updateSubresourceLevel(ID3D11Resource *srcTexture, unsigned int sourceSubresource,
                                                   const gl::ImageIndex &index, const gl::Box &copyArea)
{
    ASSERT(srcTexture);

    GLint level = index.mipIndex;

    invalidateSwizzleCacheLevel(level);

    gl::Extents texSize(getLevelWidth(level), getLevelHeight(level), getLevelDepth(level));

    bool fullCopy = copyArea.x == 0 &&
                    copyArea.y == 0 &&
                    copyArea.z == 0 &&
                    copyArea.width  == texSize.width &&
                    copyArea.height == texSize.height &&
                    copyArea.depth  == texSize.depth;

    ID3D11Resource *dstTexture = NULL;
    gl::Error error = getResource(&dstTexture);
    if (error.isError())
    {
        return error;
    }

    unsigned int dstSubresource = getSubresourceIndex(index);

    ASSERT(dstTexture);

    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(mTextureFormat);
    if (!fullCopy && (dxgiFormatInfo.depthBits > 0 || dxgiFormatInfo.stencilBits > 0))
    {
        // CopySubresourceRegion cannot copy partial depth stencils, use the blitter instead
        Blit11 *blitter = mRenderer->getBlitter();

        return blitter->copyDepthStencil(srcTexture, sourceSubresource, copyArea, texSize,
                                         dstTexture, dstSubresource, copyArea, texSize,
                                         NULL);
    }
    else
    {
        const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(mTextureFormat);

        D3D11_BOX srcBox;
        srcBox.left = copyArea.x;
        srcBox.top = copyArea.y;
        srcBox.right = copyArea.x + roundUp(static_cast<UINT>(copyArea.width), dxgiFormatInfo.blockWidth);
        srcBox.bottom = copyArea.y + roundUp(static_cast<UINT>(copyArea.height), dxgiFormatInfo.blockHeight);
        srcBox.front = copyArea.z;
        srcBox.back = copyArea.z + copyArea.depth;

        ID3D11DeviceContext *context = mRenderer->getDeviceContext();

        context->CopySubresourceRegion(dstTexture, dstSubresource, copyArea.x, copyArea.y, copyArea.z,
                                       srcTexture, sourceSubresource, fullCopy ? NULL : &srcBox);
        return gl::Error(GL_NO_ERROR);
    }
}

gl::Error TextureStorage11::copySubresourceLevel(ID3D11Resource* dstTexture, unsigned int dstSubresource,
                                                 const gl::ImageIndex &index, const gl::Box &region)
{
    ASSERT(dstTexture);

    ID3D11Resource *srcTexture = NULL;
    gl::Error error = getResource(&srcTexture);
    if (error.isError())
    {
        return error;
    }

    ASSERT(srcTexture);

    unsigned int srcSubresource = getSubresourceIndex(index);

    ID3D11DeviceContext *context = mRenderer->getDeviceContext();
    context->CopySubresourceRegion(dstTexture, dstSubresource, region.x, region.y, region.z,
                                   srcTexture, srcSubresource, NULL);

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11::generateMipmap(const gl::ImageIndex &sourceIndex, const gl::ImageIndex &destIndex)
{
    ASSERT(sourceIndex.layerIndex == destIndex.layerIndex);

    invalidateSwizzleCacheLevel(destIndex.mipIndex);

    RenderTarget *source = NULL;
    gl::Error error = getRenderTarget(sourceIndex, &source);
    if (error.isError())
    {
        return error;
    }

    RenderTarget *dest = NULL;
    error = getRenderTarget(destIndex, &dest);
    if (error.isError())
    {
        return error;
    }

    ID3D11ShaderResourceView *sourceSRV = RenderTarget11::makeRenderTarget11(source)->getShaderResourceView();
    ID3D11RenderTargetView *destRTV = RenderTarget11::makeRenderTarget11(dest)->getRenderTargetView();

    gl::Box sourceArea(0, 0, 0, source->getWidth(), source->getHeight(), source->getDepth());
    gl::Extents sourceSize(source->getWidth(), source->getHeight(), source->getDepth());

    gl::Box destArea(0, 0, 0, dest->getWidth(), dest->getHeight(), dest->getDepth());
    gl::Extents destSize(dest->getWidth(), dest->getHeight(), dest->getDepth());

    Blit11 *blitter = mRenderer->getBlitter();
    return blitter->copyTexture(sourceSRV, sourceArea, sourceSize, destRTV, destArea, destSize, NULL,
                                gl::GetInternalFormatInfo(source->getInternalFormat()).format, GL_LINEAR);
}

void TextureStorage11::verifySwizzleExists(GLenum swizzleRed, GLenum swizzleGreen, GLenum swizzleBlue, GLenum swizzleAlpha)
{
    SwizzleCacheValue swizzleTarget(swizzleRed, swizzleGreen, swizzleBlue, swizzleAlpha);
    for (unsigned int level = 0; level < mMipLevels; level++)
    {
        ASSERT(mSwizzleCache[level] == swizzleTarget);
    }
}

gl::Error TextureStorage11::copyToStorage(TextureStorage *destStorage)
{
    ASSERT(destStorage);

    ID3D11Resource *sourceResouce = NULL;
    gl::Error error = getResource(&sourceResouce);
    if (error.isError())
    {
        return error;
    }

    TextureStorage11 *dest11 = TextureStorage11::makeTextureStorage11(destStorage);
    ID3D11Resource *destResource = NULL;
    error = dest11->getResource(&destResource);
    if (error.isError())
    {
        return error;
    }

    ID3D11DeviceContext *immediateContext = mRenderer->getDeviceContext();
    immediateContext->CopyResource(destResource, sourceResouce);

    dest11->invalidateSwizzleCache();

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11::setData(const gl::ImageIndex &index, Image *image, const gl::Box *destBox, GLenum type,
                                    const gl::PixelUnpackState &unpack, const uint8_t *pixelData)
{
    ID3D11Resource *resource = NULL;
    gl::Error error = getResource(&resource);
    if (error.isError())
    {
        return error;
    }
    ASSERT(resource);

    UINT destSubresource = getSubresourceIndex(index);

    const gl::InternalFormat &internalFormatInfo = gl::GetInternalFormatInfo(image->getInternalFormat());

    bool fullUpdate = (destBox == NULL || *destBox == gl::Box(0, 0, 0, mTextureWidth, mTextureHeight, mTextureDepth));
    ASSERT(internalFormatInfo.depthBits == 0 || fullUpdate);

    // TODO(jmadill): Handle compressed formats
    // Compressed formats have different load syntax, so we'll have to handle them with slightly
    // different logic. Will implemnent this in a follow-up patch, and ensure we do not use SetData
    // with compressed formats in the calling logic.
    ASSERT(!internalFormatInfo.compressed);

    int width = destBox ? destBox->width : static_cast<int>(image->getWidth());
    int height = destBox ? destBox->height : static_cast<int>(image->getHeight());
    int depth = destBox ? destBox->depth : static_cast<int>(image->getDepth());
    UINT srcRowPitch = internalFormatInfo.computeRowPitch(type, width, unpack.alignment);
    UINT srcDepthPitch = internalFormatInfo.computeDepthPitch(type, width, height, unpack.alignment);

    const d3d11::TextureFormat &d3d11Format = d3d11::GetTextureFormatInfo(image->getInternalFormat());
    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(d3d11Format.texFormat);

    size_t outputPixelSize = dxgiFormatInfo.pixelBytes;

    UINT bufferRowPitch = outputPixelSize * width;
    UINT bufferDepthPitch = bufferRowPitch * height;

    MemoryBuffer conversionBuffer;
    if (!conversionBuffer.resize(bufferDepthPitch * depth))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to allocate internal buffer.");
    }

    // TODO: fast path
    LoadImageFunction loadFunction = d3d11Format.loadFunctions.at(type);
    loadFunction(width, height, depth,
                 pixelData, srcRowPitch, srcDepthPitch,
                 conversionBuffer.data(), bufferRowPitch, bufferDepthPitch);

    ID3D11DeviceContext *immediateContext = mRenderer->getDeviceContext();

    if (!fullUpdate)
    {
        ASSERT(destBox);

        D3D11_BOX destD3DBox;
        destD3DBox.left = destBox->x;
        destD3DBox.right = destBox->x + destBox->width;
        destD3DBox.top = destBox->y;
        destD3DBox.bottom = destBox->y + destBox->height;
        destD3DBox.front = 0;
        destD3DBox.back = 1;

        immediateContext->UpdateSubresource(resource, destSubresource,
                                            &destD3DBox, conversionBuffer.data(),
                                            bufferRowPitch, bufferDepthPitch);
    }
    else
    {
        immediateContext->UpdateSubresource(resource, destSubresource,
                                            NULL, conversionBuffer.data(),
                                            bufferRowPitch, bufferDepthPitch);
    }

    return gl::Error(GL_NO_ERROR);
}

TextureStorage11_2D::TextureStorage11_2D(Renderer11 *renderer, SwapChain11 *swapchain)
    : TextureStorage11(renderer, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE),
      mTexture(swapchain->getOffscreenTexture()),
      mSwizzleTexture(NULL)
{
    mTexture->AddRef();

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mAssociatedImages[i] = NULL;
        mRenderTarget[i] = NULL;
        mSwizzleRenderTargets[i] = NULL;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    mTexture->GetDesc(&texDesc);
    mMipLevels = texDesc.MipLevels;
    mTextureFormat = texDesc.Format;
    mTextureWidth = texDesc.Width;
    mTextureHeight = texDesc.Height;
    mTextureDepth = 1;

    mInternalFormat = swapchain->GetBackBufferInternalFormat();

    ID3D11ShaderResourceView *srv = swapchain->getRenderTargetShaderResource();
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srv->GetDesc(&srvDesc);
    mShaderResourceFormat = srvDesc.Format;

    ID3D11RenderTargetView* offscreenRTV = swapchain->getRenderTarget();
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    offscreenRTV->GetDesc(&rtvDesc);
    mRenderTargetFormat = rtvDesc.Format;

    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(mTextureFormat);
    const d3d11::TextureFormat &formatInfo = d3d11::GetTextureFormatInfo(dxgiFormatInfo.internalFormat);
    mSwizzleTextureFormat = formatInfo.swizzleTexFormat;
    mSwizzleShaderResourceFormat = formatInfo.swizzleSRVFormat;
    mSwizzleRenderTargetFormat = formatInfo.swizzleRTVFormat;

    mDepthStencilFormat = DXGI_FORMAT_UNKNOWN;

    initializeSerials(1, 1);
}

TextureStorage11_2D::TextureStorage11_2D(Renderer11 *renderer, GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderTarget)),
      mTexture(NULL),
      mSwizzleTexture(NULL)
{
    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mAssociatedImages[i] = NULL;
        mRenderTarget[i] = NULL;
        mSwizzleRenderTargets[i] = NULL;
    }

    mInternalFormat = internalformat;

    const d3d11::TextureFormat &formatInfo = d3d11::GetTextureFormatInfo(internalformat);
    mTextureFormat = formatInfo.texFormat;
    mShaderResourceFormat = formatInfo.srvFormat;
    mDepthStencilFormat = formatInfo.dsvFormat;
    mRenderTargetFormat = formatInfo.rtvFormat;
    mSwizzleTextureFormat = formatInfo.swizzleTexFormat;
    mSwizzleShaderResourceFormat = formatInfo.swizzleSRVFormat;
    mSwizzleRenderTargetFormat = formatInfo.swizzleRTVFormat;

    d3d11::MakeValidSize(false, mTextureFormat, &width, &height, &mTopLevel);
    mMipLevels = mTopLevel + levels;
    mTextureWidth = width;
    mTextureHeight = height;
    mTextureDepth = 1;

    initializeSerials(getLevelCount(), 1);
}

TextureStorage11_2D::~TextureStorage11_2D()
{
    for (unsigned i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        if (mAssociatedImages[i] != NULL)
        {
            bool imageAssociationCorrect = mAssociatedImages[i]->isAssociatedStorageValid(this);
            ASSERT(imageAssociationCorrect);

            if (imageAssociationCorrect)
            {
                // We must let the Images recover their data before we delete it from the TextureStorage.
                gl::Error error = mAssociatedImages[i]->recoverFromAssociatedStorage();
                if (error.isError())
                {
                    // TODO: Find a way to report this back to the context
                }
            }
        }
    }

    SafeRelease(mTexture);
    SafeRelease(mSwizzleTexture);

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        SafeDelete(mRenderTarget[i]);
        SafeRelease(mSwizzleRenderTargets[i]);
    }
}

TextureStorage11_2D *TextureStorage11_2D::makeTextureStorage11_2D(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_2D*, storage));
    return static_cast<TextureStorage11_2D*>(storage);
}

void TextureStorage11_2D::associateImage(Image11* image, const gl::ImageIndex &index)
{
    GLint level = index.mipIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        mAssociatedImages[level] = image;
    }
}

bool TextureStorage11_2D::isAssociatedImageValid(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        // This validation check should never return false. It means the Image/TextureStorage association is broken.
        bool retValue = (mAssociatedImages[level] == expectedImage);
        ASSERT(retValue);
        return retValue;
    }

    return false;
}

// disassociateImage allows an Image to end its association with a Storage.
void TextureStorage11_2D::disassociateImage(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        ASSERT(mAssociatedImages[level] == expectedImage);

        if (mAssociatedImages[level] == expectedImage)
        {
            mAssociatedImages[level] = NULL;
        }
    }
}

// releaseAssociatedImage prepares the Storage for a new Image association. It lets the old Image recover its data before ending the association.
gl::Error TextureStorage11_2D::releaseAssociatedImage(const gl::ImageIndex &index, Image11* incomingImage)
{
    GLint level = index.mipIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        // No need to let the old Image recover its data, if it is also the incoming Image.
        if (mAssociatedImages[level] != NULL && mAssociatedImages[level] != incomingImage)
        {
            // Ensure that the Image is still associated with this TextureStorage. This should be true.
            bool imageAssociationCorrect = mAssociatedImages[level]->isAssociatedStorageValid(this);
            ASSERT(imageAssociationCorrect);

            if (imageAssociationCorrect)
            {
                // Force the image to recover from storage before its data is overwritten.
                // This will reset mAssociatedImages[level] to NULL too.
                gl::Error error = mAssociatedImages[level]->recoverFromAssociatedStorage();
                if (error.isError())
                {
                    return error;
                }
            }
        }
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2D::getResource(ID3D11Resource **outResource)
{
    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (mTexture == NULL && mTextureWidth > 0 && mTextureHeight > 0)
    {
        ASSERT(mMipLevels > 0);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mTextureWidth;      // Compressed texture size constraints?
        desc.Height = mTextureHeight;
        desc.MipLevels = mRenderer->isLevel9() ? 1 : mMipLevels;
        desc.ArraySize = 1;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create 2D texture storage, result: 0x%X.", result);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create 2D texture storage, result: 0x%X.", result);
        }
    }

    *outResource = mTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2D::getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT)
{
    ASSERT(!index.hasLayer());

    int level = index.mipIndex;
    ASSERT(level >= 0 && level < getLevelCount());

    if (!mRenderTarget[level])
    {
        ID3D11Resource *texture = NULL;
        gl::Error error = getResource(&texture);
        if (error.isError())
        {
            return error;
        }

        ID3D11ShaderResourceView *srv = NULL;
        error = getSRVLevel(level, &srv);
        if (error.isError())
        {
            return error;
        }

        if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
        {
            ID3D11Device *device = mRenderer->getDevice();

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = mRenderTargetFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = mTopLevel + level;

            ID3D11RenderTargetView *rtv;
            HRESULT result = device->CreateRenderTargetView(texture, &rtvDesc, &rtv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal render target view for texture storage, result: 0x%X.", result);
            }

            mRenderTarget[level] = new TextureRenderTarget11(rtv, texture, srv, mInternalFormat, getLevelWidth(level), getLevelHeight(level), 1, 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(rtv);
        }
        else if (mDepthStencilFormat != DXGI_FORMAT_UNKNOWN)
        {
            ID3D11Device *device = mRenderer->getDevice();

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = mDepthStencilFormat;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = mTopLevel + level;
            dsvDesc.Flags = 0;

            ID3D11DepthStencilView *dsv;
            HRESULT result = device->CreateDepthStencilView(texture, &dsvDesc, &dsv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                return gl::Error(GL_OUT_OF_MEMORY,"Failed to create internal depth stencil view for texture storage, result: 0x%X.", result);
            }

            mRenderTarget[level] = new TextureRenderTarget11(dsv, texture, srv, mInternalFormat, getLevelWidth(level), getLevelHeight(level), 1, 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(dsv);
        }
        else
        {
            UNREACHABLE();
        }
    }

    ASSERT(outRT);
    *outRT = mRenderTarget[level];
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2D::createSRV(int baseLevel, int mipLevels, DXGI_FORMAT format, ID3D11Resource *texture,
                                         ID3D11ShaderResourceView **outSRV) const
{
    ASSERT(outSRV);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = mTopLevel + baseLevel;
    srvDesc.Texture2D.MipLevels = mRenderer->isLevel9() ? -1 : mipLevels;

    ID3D11Device *device = mRenderer->getDevice();
    HRESULT result = device->CreateShaderResourceView(texture, &srvDesc, outSRV);

    ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal texture storage SRV, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2D::getSwizzleTexture(ID3D11Resource **outTexture)
{
    ASSERT(outTexture);

    if (!mSwizzleTexture)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.MipLevels = mMipLevels;
        desc.ArraySize = 1;
        desc.Format = mSwizzleTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mSwizzleTexture);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle texture, result: 0x%X.", result);
        }
    }

    *outTexture = mSwizzleTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2D::getSwizzleRenderTarget(int mipLevel, ID3D11RenderTargetView **outRTV)
{
    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());
    ASSERT(outRTV);

    if (!mSwizzleRenderTargets[mipLevel])
    {
        ID3D11Resource *swizzleTexture = NULL;
        gl::Error error = getSwizzleTexture(&swizzleTexture);
        if (error.isError())
        {
            return error;
        }

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = mSwizzleRenderTargetFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = mTopLevel + mipLevel;

        HRESULT result = device->CreateRenderTargetView(mSwizzleTexture, &rtvDesc, &mSwizzleRenderTargets[mipLevel]);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle render target view, result: 0x%X.", result);
        }
    }

    *outRTV = mSwizzleRenderTargets[mipLevel];
    return gl::Error(GL_NO_ERROR);
}

TextureStorage11_Cube::TextureStorage11_Cube(Renderer11 *renderer, GLenum internalformat, bool renderTarget, int size, int levels)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderTarget))
{
    mTexture = NULL;
    mSwizzleTexture = NULL;

    for (unsigned int level = 0; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mSwizzleRenderTargets[level] = NULL;
        for (unsigned int face = 0; face < CUBE_FACE_COUNT; face++)
        {
            mAssociatedImages[face][level] = NULL;
            mRenderTarget[face][level] = NULL;
        }
    }

    mInternalFormat = internalformat;

    const d3d11::TextureFormat &formatInfo = d3d11::GetTextureFormatInfo(internalformat);
    mTextureFormat = formatInfo.texFormat;
    mShaderResourceFormat = formatInfo.srvFormat;
    mDepthStencilFormat = formatInfo.dsvFormat;
    mRenderTargetFormat = formatInfo.rtvFormat;
    mSwizzleTextureFormat = formatInfo.swizzleTexFormat;
    mSwizzleShaderResourceFormat = formatInfo.swizzleSRVFormat;
    mSwizzleRenderTargetFormat = formatInfo.swizzleRTVFormat;

    // adjust size if needed for compressed textures
    int height = size;
    d3d11::MakeValidSize(false, mTextureFormat, &size, &height, &mTopLevel);

    mMipLevels = mTopLevel + levels;
    mTextureWidth = size;
    mTextureHeight = size;
    mTextureDepth = 1;

    initializeSerials(getLevelCount() * CUBE_FACE_COUNT, CUBE_FACE_COUNT);
}

TextureStorage11_Cube::~TextureStorage11_Cube()
{
    for (unsigned int level = 0; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        for (unsigned int face = 0; face < CUBE_FACE_COUNT; face++)
        {
            if (mAssociatedImages[face][level] != NULL)
            {
                bool imageAssociationCorrect = mAssociatedImages[face][level]->isAssociatedStorageValid(this);
                ASSERT(imageAssociationCorrect);

                if (imageAssociationCorrect)
                {
                    // We must let the Images recover their data before we delete it from the TextureStorage.
                    mAssociatedImages[face][level]->recoverFromAssociatedStorage();
                }
            }
        }
    }

    SafeRelease(mTexture);
    SafeRelease(mSwizzleTexture);

    for (unsigned int level = 0; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        SafeRelease(mSwizzleRenderTargets[level]);
        for (unsigned int face = 0; face < CUBE_FACE_COUNT; face++)
        {
            SafeDelete(mRenderTarget[face][level]);
        }
    }
}

TextureStorage11_Cube *TextureStorage11_Cube::makeTextureStorage11_Cube(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_Cube*, storage));
    return static_cast<TextureStorage11_Cube*>(storage);
}

void TextureStorage11_Cube::associateImage(Image11* image, const gl::ImageIndex &index)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);
    ASSERT(0 <= layerTarget && layerTarget < CUBE_FACE_COUNT);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        if (0 <= layerTarget && layerTarget < CUBE_FACE_COUNT)
        {
            mAssociatedImages[layerTarget][level] = image;
        }
    }
}

bool TextureStorage11_Cube::isAssociatedImageValid(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        if (0 <= layerTarget && layerTarget < CUBE_FACE_COUNT)
        {
            // This validation check should never return false. It means the Image/TextureStorage association is broken.
            bool retValue = (mAssociatedImages[layerTarget][level] == expectedImage);
            ASSERT(retValue);
            return retValue;
        }
    }

    return false;
}

// disassociateImage allows an Image to end its association with a Storage.
void TextureStorage11_Cube::disassociateImage(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);
    ASSERT(0 <= layerTarget && layerTarget < CUBE_FACE_COUNT);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        if (0 <= layerTarget && layerTarget < CUBE_FACE_COUNT)
        {
            ASSERT(mAssociatedImages[layerTarget][level] == expectedImage);

            if (mAssociatedImages[layerTarget][level] == expectedImage)
            {
                mAssociatedImages[layerTarget][level] = NULL;
            }
        }
    }
}

// releaseAssociatedImage prepares the Storage for a new Image association. It lets the old Image recover its data before ending the association.
gl::Error TextureStorage11_Cube::releaseAssociatedImage(const gl::ImageIndex &index, Image11* incomingImage)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);
    ASSERT(0 <= layerTarget && layerTarget < CUBE_FACE_COUNT);

    if ((0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS))
    {
        if (0 <= layerTarget && layerTarget < CUBE_FACE_COUNT)
        {
            // No need to let the old Image recover its data, if it is also the incoming Image.
            if (mAssociatedImages[layerTarget][level] != NULL && mAssociatedImages[layerTarget][level] != incomingImage)
            {
                // Ensure that the Image is still associated with this TextureStorage. This should be true.
                bool imageAssociationCorrect = mAssociatedImages[layerTarget][level]->isAssociatedStorageValid(this);
                ASSERT(imageAssociationCorrect);

                if (imageAssociationCorrect)
                {
                    // Force the image to recover from storage before its data is overwritten.
                    // This will reset mAssociatedImages[level] to NULL too.
                    gl::Error error = mAssociatedImages[layerTarget][level]->recoverFromAssociatedStorage();
                    if (error.isError())
                    {
                        return error;
                    }
                }
            }
        }
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_Cube::getResource(ID3D11Resource **outResource)
{
    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (mTexture == NULL && mTextureWidth > 0 && mTextureHeight > 0)
    {
        ASSERT(mMipLevels > 0);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.MipLevels = mMipLevels;
        desc.ArraySize = CUBE_FACE_COUNT;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create cube texture storage, result: 0x%X.", result);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create cube texture storage, result: 0x%X.", result);
        }
    }

    *outResource = mTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_Cube::getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT)
{
    int faceIndex = index.layerIndex;
    int level = index.mipIndex;

    ASSERT(level >= 0 && level < getLevelCount());
    ASSERT(faceIndex >= 0 && faceIndex < CUBE_FACE_COUNT);

    if (!mRenderTarget[faceIndex][level])
    {
        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result;

        ID3D11Resource *texture = NULL;
        gl::Error error = getResource(&texture);
        if (error.isError())
        {
            return error;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY; // Will be used with Texture2D sampler, not TextureCube
        srvDesc.Texture2DArray.MostDetailedMip = mTopLevel + level;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = faceIndex;
        srvDesc.Texture2DArray.ArraySize = 1;

        ID3D11ShaderResourceView *srv;
        result = device->CreateShaderResourceView(texture, &srvDesc, &srv);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal shader resource view for texture storage, result: 0x%X.", result);
        }

        if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = mRenderTargetFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = mTopLevel + level;
            rtvDesc.Texture2DArray.FirstArraySlice = faceIndex;
            rtvDesc.Texture2DArray.ArraySize = 1;

            ID3D11RenderTargetView *rtv;
            result = device->CreateRenderTargetView(texture, &rtvDesc, &rtv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                SafeRelease(srv);
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal render target view for texture storage, result: 0x%X.", result);
            }

            mRenderTarget[faceIndex][level] = new TextureRenderTarget11(rtv, texture, srv, mInternalFormat, getLevelWidth(level), getLevelHeight(level), 1, 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(rtv);
            SafeRelease(srv);
        }
        else if (mDepthStencilFormat != DXGI_FORMAT_UNKNOWN)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = mDepthStencilFormat;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Flags = 0;
            dsvDesc.Texture2DArray.MipSlice = mTopLevel + level;
            dsvDesc.Texture2DArray.FirstArraySlice = faceIndex;
            dsvDesc.Texture2DArray.ArraySize = 1;

            ID3D11DepthStencilView *dsv;
            result = device->CreateDepthStencilView(texture, &dsvDesc, &dsv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                SafeRelease(srv);
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal depth stencil view for texture storage, result: 0x%X.", result);
            }

            mRenderTarget[faceIndex][level] = new TextureRenderTarget11(dsv, texture, srv, mInternalFormat, getLevelWidth(level), getLevelHeight(level), 1, 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(dsv);
            SafeRelease(srv);
        }
        else
        {
            UNREACHABLE();
        }
    }

    ASSERT(outRT);
    *outRT = mRenderTarget[faceIndex][level];
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_Cube::createSRV(int baseLevel, int mipLevels, DXGI_FORMAT format, ID3D11Resource *texture,
                                           ID3D11ShaderResourceView **outSRV) const
{
    ASSERT(outSRV);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = format;

    // Unnormalized integer cube maps are not supported by DX11; we emulate them as an array of six 2D textures
    const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(format);
    if (dxgiFormatInfo.componentType == GL_INT || dxgiFormatInfo.componentType == GL_UNSIGNED_INT)
    {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = mTopLevel + baseLevel;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = CUBE_FACE_COUNT;
    }
    else
    {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = mipLevels;
        srvDesc.TextureCube.MostDetailedMip = mTopLevel + baseLevel;
    }

    ID3D11Device *device = mRenderer->getDevice();
    HRESULT result = device->CreateShaderResourceView(texture, &srvDesc, outSRV);

    ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal texture storage SRV, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_Cube::getSwizzleTexture(ID3D11Resource **outTexture)
{
    ASSERT(outTexture);

    if (!mSwizzleTexture)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.MipLevels = mMipLevels;
        desc.ArraySize = CUBE_FACE_COUNT;
        desc.Format = mSwizzleTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mSwizzleTexture);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle texture, result: 0x%X.", result);
        }
    }

    *outTexture = mSwizzleTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_Cube::getSwizzleRenderTarget(int mipLevel, ID3D11RenderTargetView **outRTV)
{
    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());
    ASSERT(outRTV);

    if (!mSwizzleRenderTargets[mipLevel])
    {
        ID3D11Resource *swizzleTexture = NULL;
        gl::Error error = getSwizzleTexture(&swizzleTexture);
        if (error.isError())
        {
            return error;
        }

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = mSwizzleRenderTargetFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = mTopLevel + mipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = 0;
        rtvDesc.Texture2DArray.ArraySize = CUBE_FACE_COUNT;

        HRESULT result = device->CreateRenderTargetView(mSwizzleTexture, &rtvDesc, &mSwizzleRenderTargets[mipLevel]);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle render target view, result: 0x%X.", result);
        }
    }

    *outRTV = mSwizzleRenderTargets[mipLevel];
    return gl::Error(GL_NO_ERROR);
}

TextureStorage11_3D::TextureStorage11_3D(Renderer11 *renderer, GLenum internalformat, bool renderTarget,
                                         GLsizei width, GLsizei height, GLsizei depth, int levels)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderTarget))
{
    mTexture = NULL;
    mSwizzleTexture = NULL;

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mAssociatedImages[i] = NULL;
        mLevelRenderTargets[i] = NULL;
        mSwizzleRenderTargets[i] = NULL;
    }

    mInternalFormat = internalformat;

    const d3d11::TextureFormat &formatInfo = d3d11::GetTextureFormatInfo(internalformat);
    mTextureFormat = formatInfo.texFormat;
    mShaderResourceFormat = formatInfo.srvFormat;
    mDepthStencilFormat = formatInfo.dsvFormat;
    mRenderTargetFormat = formatInfo.rtvFormat;
    mSwizzleTextureFormat = formatInfo.swizzleTexFormat;
    mSwizzleShaderResourceFormat = formatInfo.swizzleSRVFormat;
    mSwizzleRenderTargetFormat = formatInfo.swizzleRTVFormat;

    // adjust size if needed for compressed textures
    d3d11::MakeValidSize(false, mTextureFormat, &width, &height, &mTopLevel);

    mMipLevels = mTopLevel + levels;
    mTextureWidth = width;
    mTextureHeight = height;
    mTextureDepth = depth;

    initializeSerials(getLevelCount() * depth, depth);
}

TextureStorage11_3D::~TextureStorage11_3D()
{
    for (unsigned i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        if (mAssociatedImages[i] != NULL)
        {
            bool imageAssociationCorrect = mAssociatedImages[i]->isAssociatedStorageValid(this);
            ASSERT(imageAssociationCorrect);

            if (imageAssociationCorrect)
            {
                // We must let the Images recover their data before we delete it from the TextureStorage.
                mAssociatedImages[i]->recoverFromAssociatedStorage();
            }
        }
    }

    SafeRelease(mTexture);
    SafeRelease(mSwizzleTexture);

    for (RenderTargetMap::iterator i = mLevelLayerRenderTargets.begin(); i != mLevelLayerRenderTargets.end(); i++)
    {
        SafeDelete(i->second);
    }
    mLevelLayerRenderTargets.clear();

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        SafeDelete(mLevelRenderTargets[i]);
        SafeRelease(mSwizzleRenderTargets[i]);
    }
}

TextureStorage11_3D *TextureStorage11_3D::makeTextureStorage11_3D(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_3D*, storage));
    return static_cast<TextureStorage11_3D*>(storage);
}

void TextureStorage11_3D::associateImage(Image11* image, const gl::ImageIndex &index)
{
    GLint level = index.mipIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        mAssociatedImages[level] = image;
    }
}

bool TextureStorage11_3D::isAssociatedImageValid(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        // This validation check should never return false. It means the Image/TextureStorage association is broken.
        bool retValue = (mAssociatedImages[level] == expectedImage);
        ASSERT(retValue);
        return retValue;
    }

    return false;
}

// disassociateImage allows an Image to end its association with a Storage.
void TextureStorage11_3D::disassociateImage(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;

    ASSERT(0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        ASSERT(mAssociatedImages[level] == expectedImage);

        if (mAssociatedImages[level] == expectedImage)
        {
            mAssociatedImages[level] = NULL;
        }
    }
}

// releaseAssociatedImage prepares the Storage for a new Image association. It lets the old Image recover its data before ending the association.
gl::Error TextureStorage11_3D::releaseAssociatedImage(const gl::ImageIndex &index, Image11* incomingImage)
{
    GLint level = index.mipIndex;

    ASSERT((0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS));

    if (0 <= level && level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
    {
        // No need to let the old Image recover its data, if it is also the incoming Image.
        if (mAssociatedImages[level] != NULL && mAssociatedImages[level] != incomingImage)
        {
            // Ensure that the Image is still associated with this TextureStorage. This should be true.
            bool imageAssociationCorrect = mAssociatedImages[level]->isAssociatedStorageValid(this);
            ASSERT(imageAssociationCorrect);

            if (imageAssociationCorrect)
            {
                // Force the image to recover from storage before its data is overwritten.
                // This will reset mAssociatedImages[level] to NULL too.
                gl::Error error = mAssociatedImages[level]->recoverFromAssociatedStorage();
                if (error.isError())
                {
                    return error;
                }
            }
        }
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_3D::getResource(ID3D11Resource **outResource)
{
    // If the width, height or depth are not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (mTexture == NULL && mTextureWidth > 0 && mTextureHeight > 0 && mTextureDepth > 0)
    {
        ASSERT(mMipLevels > 0);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE3D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.Depth = mTextureDepth;
        desc.MipLevels = mMipLevels;
        desc.Format = mTextureFormat;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture3D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create 3D texture storage, result: 0x%X.", result);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create 3D texture storage, result: 0x%X.", result);
        }
    }

    *outResource = mTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_3D::createSRV(int baseLevel, int mipLevels, DXGI_FORMAT format, ID3D11Resource *texture,
                                         ID3D11ShaderResourceView **outSRV) const
{
    ASSERT(outSRV);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MostDetailedMip = baseLevel;
    srvDesc.Texture3D.MipLevels = mipLevels;

    ID3D11Device *device = mRenderer->getDevice();
    HRESULT result = device->CreateShaderResourceView(texture, &srvDesc, outSRV);

    ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal texture storage SRV, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_3D::getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT)
{
    int mipLevel = index.mipIndex;
    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());

    ASSERT(mRenderTargetFormat != DXGI_FORMAT_UNKNOWN);

    if (!index.hasLayer())
    {
        if (!mLevelRenderTargets[mipLevel])
        {
            ID3D11Resource *texture = NULL;
            gl::Error error = getResource(&texture);
            if (error.isError())
            {
                return error;
            }

            ID3D11ShaderResourceView *srv = NULL;
            error = getSRVLevel(mipLevel, &srv);
            if (error.isError())
            {
                return error;
            }

            ID3D11Device *device = mRenderer->getDevice();

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = mRenderTargetFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = mTopLevel + mipLevel;
            rtvDesc.Texture3D.FirstWSlice = 0;
            rtvDesc.Texture3D.WSize = -1;

            ID3D11RenderTargetView *rtv;
            HRESULT result = device->CreateRenderTargetView(texture, &rtvDesc, &rtv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                SafeRelease(srv);
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal render target view for texture storage, result: 0x%X.", result);
            }

            mLevelRenderTargets[mipLevel] = new TextureRenderTarget11(rtv, texture, srv, mInternalFormat, getLevelWidth(mipLevel), getLevelHeight(mipLevel), getLevelDepth(mipLevel), 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(rtv);
        }

        ASSERT(outRT);
        *outRT = mLevelRenderTargets[mipLevel];
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        int layer = index.layerIndex;

        LevelLayerKey key(mipLevel, layer);
        if (mLevelLayerRenderTargets.find(key) == mLevelLayerRenderTargets.end())
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            ID3D11Resource *texture = NULL;
            gl::Error error = getResource(&texture);
            if (error.isError())
            {
                return error;
            }

            // TODO, what kind of SRV is expected here?
            ID3D11ShaderResourceView *srv = NULL;

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = mRenderTargetFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = mTopLevel + mipLevel;
            rtvDesc.Texture3D.FirstWSlice = layer;
            rtvDesc.Texture3D.WSize = 1;

            ID3D11RenderTargetView *rtv;
            result = device->CreateRenderTargetView(texture, &rtvDesc, &rtv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                SafeRelease(srv); return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal render target view for texture storage, result: 0x%X.", result);
            }
            ASSERT(SUCCEEDED(result));

            mLevelLayerRenderTargets[key] = new TextureRenderTarget11(rtv, texture, srv, mInternalFormat, getLevelWidth(mipLevel), getLevelHeight(mipLevel), 1, 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(rtv);
        }

        ASSERT(outRT);
        *outRT = mLevelLayerRenderTargets[key];
        return gl::Error(GL_NO_ERROR);
    }
}

gl::Error TextureStorage11_3D::getSwizzleTexture(ID3D11Resource **outTexture)
{
    ASSERT(outTexture);

    if (!mSwizzleTexture)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE3D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.Depth = mTextureDepth;
        desc.MipLevels = mMipLevels;
        desc.Format = mSwizzleTextureFormat;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture3D(&desc, NULL, &mSwizzleTexture);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle texture, result: 0x%X.", result);
        }
    }

    *outTexture = mSwizzleTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_3D::getSwizzleRenderTarget(int mipLevel, ID3D11RenderTargetView **outRTV)
{
    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());
    ASSERT(outRTV);

    if (!mSwizzleRenderTargets[mipLevel])
    {
        ID3D11Resource *swizzleTexture = NULL;
        gl::Error error = getSwizzleTexture(&swizzleTexture);
        if (error.isError())
        {
            return error;
        }

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = mSwizzleRenderTargetFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.MipSlice = mTopLevel + mipLevel;
        rtvDesc.Texture3D.FirstWSlice = 0;
        rtvDesc.Texture3D.WSize = -1;

        HRESULT result = device->CreateRenderTargetView(mSwizzleTexture, &rtvDesc, &mSwizzleRenderTargets[mipLevel]);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle render target view, result: 0x%X.", result);
        }
    }

    *outRTV = mSwizzleRenderTargets[mipLevel];
    return gl::Error(GL_NO_ERROR);
}

TextureStorage11_2DArray::TextureStorage11_2DArray(Renderer11 *renderer, GLenum internalformat, bool renderTarget,
                                                   GLsizei width, GLsizei height, GLsizei depth, int levels)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderTarget))
{
    mTexture = NULL;
    mSwizzleTexture = NULL;

    for (unsigned int level = 0; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mSwizzleRenderTargets[level] = NULL;
    }

    mInternalFormat = internalformat;

    const d3d11::TextureFormat &formatInfo = d3d11::GetTextureFormatInfo(internalformat);
    mTextureFormat = formatInfo.texFormat;
    mShaderResourceFormat = formatInfo.srvFormat;
    mDepthStencilFormat = formatInfo.dsvFormat;
    mRenderTargetFormat = formatInfo.rtvFormat;
    mSwizzleTextureFormat = formatInfo.swizzleTexFormat;
    mSwizzleShaderResourceFormat = formatInfo.swizzleSRVFormat;
    mSwizzleRenderTargetFormat = formatInfo.swizzleRTVFormat;

    // adjust size if needed for compressed textures
    d3d11::MakeValidSize(false, mTextureFormat, &width, &height, &mTopLevel);

    mMipLevels = mTopLevel + levels;
    mTextureWidth = width;
    mTextureHeight = height;
    mTextureDepth = depth;

    initializeSerials(getLevelCount() * depth, depth);
}

TextureStorage11_2DArray::~TextureStorage11_2DArray()
{
    for (ImageMap::iterator i = mAssociatedImages.begin(); i != mAssociatedImages.end(); i++)
    {
        bool imageAssociationCorrect = i->second->isAssociatedStorageValid(this);
        ASSERT(imageAssociationCorrect);

        if (imageAssociationCorrect)
        {
            // We must let the Images recover their data before we delete it from the TextureStorage.
            i->second->recoverFromAssociatedStorage();
        }
    }
    mAssociatedImages.clear();

    SafeRelease(mTexture);
    SafeRelease(mSwizzleTexture);

    for (unsigned int level = 0; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        SafeRelease(mSwizzleRenderTargets[level]);
    }

    for (RenderTargetMap::iterator i = mRenderTargets.begin(); i != mRenderTargets.end(); i++)
    {
        SafeDelete(i->second);
    }
    mRenderTargets.clear();
}

TextureStorage11_2DArray *TextureStorage11_2DArray::makeTextureStorage11_2DArray(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_2DArray*, storage));
    return static_cast<TextureStorage11_2DArray*>(storage);
}

void TextureStorage11_2DArray::associateImage(Image11* image, const gl::ImageIndex &index)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    ASSERT(0 <= level && level < getLevelCount());

    if (0 <= level && level < getLevelCount())
    {
        LevelLayerKey key(level, layerTarget);
        mAssociatedImages[key] = image;
    }
}

bool TextureStorage11_2DArray::isAssociatedImageValid(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    LevelLayerKey key(level, layerTarget);

    // This validation check should never return false. It means the Image/TextureStorage association is broken.
    bool retValue = (mAssociatedImages.find(key) != mAssociatedImages.end() && (mAssociatedImages[key] == expectedImage));
    ASSERT(retValue);
    return retValue;
}

// disassociateImage allows an Image to end its association with a Storage.
void TextureStorage11_2DArray::disassociateImage(const gl::ImageIndex &index, Image11* expectedImage)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    LevelLayerKey key(level, layerTarget);

    bool imageAssociationCorrect = (mAssociatedImages.find(key) != mAssociatedImages.end() && (mAssociatedImages[key] == expectedImage));
    ASSERT(imageAssociationCorrect);

    if (imageAssociationCorrect)
    {
        mAssociatedImages[key] = NULL;
    }
}

// releaseAssociatedImage prepares the Storage for a new Image association. It lets the old Image recover its data before ending the association.
gl::Error TextureStorage11_2DArray::releaseAssociatedImage(const gl::ImageIndex &index, Image11* incomingImage)
{
    GLint level = index.mipIndex;
    GLint layerTarget = index.layerIndex;

    LevelLayerKey key(level, layerTarget);

    ASSERT(mAssociatedImages.find(key) != mAssociatedImages.end());

    if (mAssociatedImages.find(key) != mAssociatedImages.end())
    {
        if (mAssociatedImages[key] != NULL && mAssociatedImages[key] != incomingImage)
        {
            // Ensure that the Image is still associated with this TextureStorage. This should be true.
            bool imageAssociationCorrect = mAssociatedImages[key]->isAssociatedStorageValid(this);
            ASSERT(imageAssociationCorrect);

            if (imageAssociationCorrect)
            {
                // Force the image to recover from storage before its data is overwritten.
                // This will reset mAssociatedImages[level] to NULL too.
                gl::Error error = mAssociatedImages[key]->recoverFromAssociatedStorage();
                if (error.isError())
                {
                    return error;
                }
            }
        }
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2DArray::getResource(ID3D11Resource **outResource)
{
    // if the width, height or depth is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (mTexture == NULL && mTextureWidth > 0 && mTextureHeight > 0 && mTextureDepth > 0)
    {
        ASSERT(mMipLevels > 0);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.MipLevels = mMipLevels;
        desc.ArraySize = mTextureDepth;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create 2D array texture storage, result: 0x%X.", result);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create 2D array texture storage, result: 0x%X.", result);
        }
    }

    *outResource = mTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2DArray::createSRV(int baseLevel, int mipLevels, DXGI_FORMAT format, ID3D11Resource *texture,
                                              ID3D11ShaderResourceView **outSRV) const
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = mTopLevel + baseLevel;
    srvDesc.Texture2DArray.MipLevels = mipLevels;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = mTextureDepth;

    ID3D11Device *device = mRenderer->getDevice();
    HRESULT result = device->CreateShaderResourceView(texture, &srvDesc, outSRV);

    ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal texture storage SRV, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2DArray::getRenderTarget(const gl::ImageIndex &index, RenderTarget **outRT)
{
    ASSERT(index.hasLayer());

    int mipLevel = index.mipIndex;
    int layer = index.layerIndex;

    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());

    LevelLayerKey key(mipLevel, layer);
    if (mRenderTargets.find(key) == mRenderTargets.end())
    {
        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result;

        ID3D11Resource *texture = NULL;
        gl::Error error = getResource(&texture);
        if (error.isError())
        {
            return error;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = mTopLevel + mipLevel;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = layer;
        srvDesc.Texture2DArray.ArraySize = 1;

        ID3D11ShaderResourceView *srv;
        result = device->CreateShaderResourceView(texture, &srvDesc, &srv);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal shader resource view for texture storage, result: 0x%X.", result);
        }

        if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = mRenderTargetFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = mTopLevel + mipLevel;
            rtvDesc.Texture2DArray.FirstArraySlice = layer;
            rtvDesc.Texture2DArray.ArraySize = 1;

            ID3D11RenderTargetView *rtv;
            result = device->CreateRenderTargetView(texture, &rtvDesc, &rtv);

            ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
            if (FAILED(result))
            {
                SafeRelease(srv);
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal render target view for texture storage, result: 0x%X.", result);
            }

            mRenderTargets[key] = new TextureRenderTarget11(rtv, texture, srv, mInternalFormat, getLevelWidth(mipLevel), getLevelHeight(mipLevel), 1, 0);

            // RenderTarget will take ownership of these resources
            SafeRelease(rtv);
            SafeRelease(srv);
        }
        else
        {
            UNREACHABLE();
        }
    }

    ASSERT(outRT);
    *outRT = mRenderTargets[key];
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2DArray::getSwizzleTexture(ID3D11Resource **outTexture)
{
    if (!mSwizzleTexture)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mTextureWidth;
        desc.Height = mTextureHeight;
        desc.MipLevels = mMipLevels;
        desc.ArraySize = mTextureDepth;
        desc.Format = mSwizzleTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mSwizzleTexture);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle texture, result: 0x%X.", result);
        }
    }

    *outTexture = mSwizzleTexture;
    return gl::Error(GL_NO_ERROR);
}

gl::Error TextureStorage11_2DArray::getSwizzleRenderTarget(int mipLevel, ID3D11RenderTargetView **outRTV)
{
    ASSERT(mipLevel >= 0 && mipLevel < getLevelCount());
    ASSERT(outRTV);

    if (!mSwizzleRenderTargets[mipLevel])
    {
        ID3D11Resource *swizzleTexture = NULL;
        gl::Error error = getSwizzleTexture(&swizzleTexture);
        if (error.isError())
        {
            return error;
        }

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = mSwizzleRenderTargetFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = mTopLevel + mipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = 0;
        rtvDesc.Texture2DArray.ArraySize = mTextureDepth;

        HRESULT result = device->CreateRenderTargetView(mSwizzleTexture, &rtvDesc, &mSwizzleRenderTargets[mipLevel]);

        ASSERT(result == E_OUTOFMEMORY || SUCCEEDED(result));
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal swizzle render target view, result: 0x%X.", result);
        }
    }

    *outRTV = mSwizzleRenderTargets[mipLevel];
    return gl::Error(GL_NO_ERROR);
}

}

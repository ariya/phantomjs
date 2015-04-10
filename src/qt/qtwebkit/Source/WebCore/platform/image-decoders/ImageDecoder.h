/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008-2009 Torch Mobile, Inc.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ImageDecoder_h
#define ImageDecoder_h

#include "IntRect.h"
#include "ImageSource.h"
#include "PlatformScreen.h"
#include "SharedBuffer.h"
#include <wtf/Assertions.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if USE(QCMSLIB)
#include "qcms.h"
#if OS(DARWIN)
#include "GraphicsContextCG.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/RetainPtr.h>
#endif
#endif

namespace WebCore {

    // ImageFrame represents the decoded image data.  This buffer is what all
    // decoders write a single frame into.
    class ImageFrame {
    public:
        enum FrameStatus { FrameEmpty, FramePartial, FrameComplete };
        enum FrameDisposalMethod {
            // If you change the numeric values of these, make sure you audit
            // all users, as some users may cast raw values to/from these
            // constants.
            DisposeNotSpecified,      // Leave frame in framebuffer
            DisposeKeep,              // Leave frame in framebuffer
            DisposeOverwriteBgcolor,  // Clear frame to transparent
            DisposeOverwritePrevious  // Clear frame to previous framebuffer
                                      // contents
        };
        typedef unsigned PixelData;

        ImageFrame();

        ImageFrame(const ImageFrame& other) { operator=(other); }

        // For backends which refcount their data, this operator doesn't need to
        // create a new copy of the image data, only increase the ref count.
        ImageFrame& operator=(const ImageFrame& other);

        // These do not touch other metadata, only the raw pixel data.
        void clearPixelData();
        void zeroFillPixelData();
        void zeroFillFrameRect(const IntRect&);

        // Makes this frame have an independent copy of the provided image's
        // pixel data, so that modifications in one frame are not reflected in
        // the other.  Returns whether the copy succeeded.
        bool copyBitmapData(const ImageFrame&);

        // Copies the pixel data at [(startX, startY), (endX, startY)) to the
        // same X-coordinates on each subsequent row up to but not including
        // endY.
        void copyRowNTimes(int startX, int endX, int startY, int endY)
        {
            ASSERT(startX < width());
            ASSERT(endX <= width());
            ASSERT(startY < height());
            ASSERT(endY <= height());
            const int rowBytes = (endX - startX) * sizeof(PixelData);
            const PixelData* const startAddr = getAddr(startX, startY);
            for (int destY = startY + 1; destY < endY; ++destY)
                memcpy(getAddr(startX, destY), startAddr, rowBytes);
        }

        // Allocates space for the pixel data.  Must be called before any pixels
        // are written.  Must only be called once.  Returns whether allocation
        // succeeded.
        bool setSize(int newWidth, int newHeight);

        // Returns a caller-owned pointer to the underlying native image data.
        // (Actual use: This pointer will be owned by BitmapImage and freed in
        // FrameData::clear()).
        PassNativeImagePtr asNewNativeImage() const;

        bool hasAlpha() const;
        const IntRect& originalFrameRect() const { return m_originalFrameRect; }
        FrameStatus status() const { return m_status; }
        unsigned duration() const { return m_duration; }
        FrameDisposalMethod disposalMethod() const { return m_disposalMethod; }
        bool premultiplyAlpha() const { return m_premultiplyAlpha; }

        void setHasAlpha(bool alpha);
        void setColorProfile(const ColorProfile&);
        void setOriginalFrameRect(const IntRect& r) { m_originalFrameRect = r; }
        void setStatus(FrameStatus status);
        void setDuration(unsigned duration) { m_duration = duration; }
        void setDisposalMethod(FrameDisposalMethod method) { m_disposalMethod = method; }
        void setPremultiplyAlpha(bool premultiplyAlpha) { m_premultiplyAlpha = premultiplyAlpha; }

        inline void setRGBA(int x, int y, unsigned r, unsigned g, unsigned b, unsigned a)
        {
            setRGBA(getAddr(x, y), r, g, b, a);
        }

        inline PixelData* getAddr(int x, int y)
        {
            return m_bytes + (y * width()) + x;
        }

        inline bool hasPixelData() const
        {
            return m_bytes;
        }

        // Use fix point multiplier instead of integer division or floating point math.
        // This multipler produces exactly the same result for all values in range 0 - 255.
        static const unsigned fixPointShift = 24;
        static const unsigned fixPointMult = static_cast<unsigned>(1.0 / 255.0 * (1 << fixPointShift)) + 1;
        // Multiplies unsigned value by fixpoint value and converts back to unsigned.
        static unsigned fixPointUnsignedMultiply(unsigned fixed, unsigned v)
        {
            return  (fixed * v) >> fixPointShift;
        }

        inline void setRGBA(PixelData* dest, unsigned r, unsigned g, unsigned b, unsigned a)
        {
            if (m_premultiplyAlpha && a < 255) {
                if (!a) {
                    *dest = 0;
                    return;
                }

                unsigned alphaMult = a * fixPointMult;
                r = fixPointUnsignedMultiply(r, alphaMult);
                g = fixPointUnsignedMultiply(g, alphaMult);
                b = fixPointUnsignedMultiply(b, alphaMult);
            }
            *dest = (a << 24 | r << 16 | g << 8 | b);
        }

    private:
        int width() const
        {
            return m_size.width();
        }

        int height() const
        {
            return m_size.height();
        }

        Vector<PixelData> m_backingStore;
        PixelData* m_bytes; // The memory is backed by m_backingStore.
        IntSize m_size;
        // FIXME: Do we need m_colorProfile anymore?
        ColorProfile m_colorProfile;
        bool m_hasAlpha;
        IntRect m_originalFrameRect; // This will always just be the entire
                                     // buffer except for GIF frames whose
                                     // original rect was smaller than the
                                     // overall image size.
        FrameStatus m_status;
        unsigned m_duration;
        FrameDisposalMethod m_disposalMethod;
        bool m_premultiplyAlpha;
    };

    // ImageDecoder is a base for all format-specific decoders
    // (e.g. JPEGImageDecoder).  This base manages the ImageFrame cache.
    //
    // ENABLE(IMAGE_DECODER_DOWN_SAMPLING) allows image decoders to downsample
    // at decode time.  Image decoders will downsample any images larger than
    // |m_maxNumPixels|.  FIXME: Not yet supported by all decoders.
    class ImageDecoder {
        WTF_MAKE_NONCOPYABLE(ImageDecoder); WTF_MAKE_FAST_ALLOCATED;
    public:
        ImageDecoder(ImageSource::AlphaOption alphaOption, ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
            : m_scaled(false)
            , m_premultiplyAlpha(alphaOption == ImageSource::AlphaPremultiplied)
            , m_ignoreGammaAndColorProfile(gammaAndColorProfileOption == ImageSource::GammaAndColorProfileIgnored)
            , m_sizeAvailable(false)
            , m_maxNumPixels(-1)
            , m_isAllDataReceived(false)
            , m_failed(false) { }

        virtual ~ImageDecoder() { }

        // Returns a caller-owned decoder of the appropriate type.  Returns 0 if
        // we can't sniff a supported type from the provided data (possibly
        // because there isn't enough data yet).
        static ImageDecoder* create(const SharedBuffer& data, ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);

        virtual String filenameExtension() const = 0;

        bool isAllDataReceived() const { return m_isAllDataReceived; }

        virtual void setData(SharedBuffer* data, bool allDataReceived)
        {
            if (m_failed)
                return;
            m_data = data;
            m_isAllDataReceived = allDataReceived;
        }

        // Lazily-decodes enough of the image to get the size (if possible).
        // FIXME: Right now that has to be done by each subclass; factor the
        // decode call out and use it here.
        virtual bool isSizeAvailable()
        {
            return !m_failed && m_sizeAvailable;
        }

        virtual IntSize size() const { return m_size; }

        IntSize scaledSize() const
        {
            return m_scaled ? IntSize(m_scaledColumns.size(), m_scaledRows.size()) : size();
        }

        // This will only differ from size() for ICO (where each frame is a
        // different icon) or other formats where different frames are different
        // sizes.  This does NOT differ from size() for GIF, since decoding GIFs
        // composites any smaller frames against previous frames to create full-
        // size frames.
        virtual IntSize frameSizeAtIndex(size_t) const
        {
            return size();
        }

        // Returns whether the size is legal (i.e. not going to result in
        // overflow elsewhere).  If not, marks decoding as failed.
        virtual bool setSize(unsigned width, unsigned height)
        {
            if (isOverSize(width, height))
                return setFailed();
            m_size = IntSize(width, height);
            m_sizeAvailable = true;
            return true;
        }

        // Lazily-decodes enough of the image to get the frame count (if
        // possible), without decoding the individual frames.
        // FIXME: Right now that has to be done by each subclass; factor the
        // decode call out and use it here.
        virtual size_t frameCount() { return 1; }

        virtual int repetitionCount() const { return cAnimationNone; }

        // Decodes as much of the requested frame as possible, and returns an
        // ImageDecoder-owned pointer.
        virtual ImageFrame* frameBufferAtIndex(size_t) = 0;

        // Make the best effort guess to check if the requested frame has alpha channel.
        virtual bool frameHasAlphaAtIndex(size_t) const;

        // Whether or not the frame is fully received.
        virtual bool frameIsCompleteAtIndex(size_t) const;

        // Duration for displaying a frame in seconds. This method is used by animated images only.
        virtual float frameDurationAtIndex(size_t) const { return 0; }

        // Number of bytes in the decoded frame requested. Return 0 if not yet decoded.
        virtual unsigned frameBytesAtIndex(size_t) const;

        void setIgnoreGammaAndColorProfile(bool flag) { m_ignoreGammaAndColorProfile = flag; }
        bool ignoresGammaAndColorProfile() const { return m_ignoreGammaAndColorProfile; }

        ImageOrientation orientation() const { return m_orientation; }

        enum { iccColorProfileHeaderLength = 128 };

        static bool rgbColorProfile(const char* profileData, unsigned profileLength)
        {
            ASSERT_UNUSED(profileLength, profileLength >= iccColorProfileHeaderLength);

            return !memcmp(&profileData[16], "RGB ", 4);
        }

        static bool inputDeviceColorProfile(const char* profileData, unsigned profileLength)
        {
            ASSERT_UNUSED(profileLength, profileLength >= iccColorProfileHeaderLength);

            return !memcmp(&profileData[12], "mntr", 4) || !memcmp(&profileData[12], "scnr", 4);
        }

#if USE(QCMSLIB)
        static qcms_profile* qcmsOutputDeviceProfile()
        {
            static qcms_profile* outputDeviceProfile = 0;

            static bool qcmsInitialized = false;
            if (!qcmsInitialized) {
                qcmsInitialized = true;
                // FIXME: Add optional ICCv4 support.
#if OS(DARWIN)
                RetainPtr<CGColorSpaceRef> monitorColorSpace = adoptCF(CGDisplayCopyColorSpace(CGMainDisplayID()));
                CFDataRef iccProfile(CGColorSpaceCopyICCProfile(monitorColorSpace.get()));
                if (iccProfile) {
                    size_t length = CFDataGetLength(iccProfile);
                    const unsigned char* systemProfile = CFDataGetBytePtr(iccProfile);
                    outputDeviceProfile = qcms_profile_from_memory(systemProfile, length);
                }
#else
                // FIXME: add support for multiple monitors.
                ColorProfile profile;
                screenColorProfile(profile);
                if (!profile.isEmpty())
                    outputDeviceProfile = qcms_profile_from_memory(profile.data(), profile.size());
#endif
                if (outputDeviceProfile && qcms_profile_is_bogus(outputDeviceProfile)) {
                    qcms_profile_release(outputDeviceProfile);
                    outputDeviceProfile = 0;
                }
                if (!outputDeviceProfile)
                    outputDeviceProfile = qcms_profile_sRGB();
                if (outputDeviceProfile)
                    qcms_profile_precache_output_transform(outputDeviceProfile);
            }
            return outputDeviceProfile;
        }
#endif

        // Sets the "decode failure" flag.  For caller convenience (since so
        // many callers want to return false after calling this), returns false
        // to enable easy tailcalling.  Subclasses may override this to also
        // clean up any local data.
        virtual bool setFailed()
        {
            m_failed = true;
            return false;
        }

        bool failed() const { return m_failed; }

        // Clears decoded pixel data from before the provided frame unless that
        // data may be needed to decode future frames (e.g. due to GIF frame
        // compositing).
        virtual void clearFrameBufferCache(size_t) { }

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
        void setMaxNumPixels(int m) { m_maxNumPixels = m; }
#endif

        // If the image has a cursor hot-spot, stores it in the argument
        // and returns true. Otherwise returns false.
        virtual bool hotSpot(IntPoint&) const { return false; }

    protected:
        void prepareScaleDataIfNecessary();
        int upperBoundScaledX(int origX, int searchStart = 0);
        int lowerBoundScaledX(int origX, int searchStart = 0);
        int upperBoundScaledY(int origY, int searchStart = 0);
        int lowerBoundScaledY(int origY, int searchStart = 0);
        int scaledY(int origY, int searchStart = 0);

        RefPtr<SharedBuffer> m_data; // The encoded data.
        Vector<ImageFrame, 1> m_frameBufferCache;
        // FIXME: Do we need m_colorProfile any more, for any port?
        ColorProfile m_colorProfile;
        bool m_scaled;
        Vector<int> m_scaledColumns;
        Vector<int> m_scaledRows;
        bool m_premultiplyAlpha;
        bool m_ignoreGammaAndColorProfile;
        ImageOrientation m_orientation;

    private:
        // Some code paths compute the size of the image as "width * height * 4"
        // and return it as a (signed) int.  Avoid overflow.
        static bool isOverSize(unsigned width, unsigned height)
        {
            unsigned long long total_size = static_cast<unsigned long long>(width)
                                          * static_cast<unsigned long long>(height);
            return total_size > ((1 << 29) - 1);
        }

        IntSize m_size;
        bool m_sizeAvailable;
        int m_maxNumPixels;
        bool m_isAllDataReceived;
        bool m_failed;
    };

} // namespace WebCore

#endif

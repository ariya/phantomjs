/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef GIFImageReader_h
#define GIFImageReader_h

// Define ourselves as the clientPtr.  Mozilla just hacked their C++ callback class into this old C decoder,
// so we will too.
#include "GIFImageDecoder.h"
#include "SharedBuffer.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

#define MAX_LZW_BITS          12
#define MAX_BYTES           4097 /* 2^MAX_LZW_BITS+1 */
#define MAX_COLORS           256
#define GIF_COLORS             3

const int cLoopCountNotSeen = -2;

// List of possible parsing states.
enum GIFState {
    GIFType,
    GIFGlobalHeader,
    GIFGlobalColormap,
    GIFImageStart,            
    GIFImageHeader,
    GIFImageColormap,
    GIFImageBody,
    GIFLZWStart,
    GIFLZW,
    GIFSubBlock,
    GIFExtension,
    GIFControlExtension,
    GIFConsumeBlock,
    GIFSkipBlock,
    GIFDone,
    GIFCommentExtension,
    GIFApplicationExtension,
    GIFNetscapeExtensionBlock,
    GIFConsumeNetscapeExtension,
    GIFConsumeComment
};

struct GIFFrameContext;

// LZW decoder state machine.
class GIFLZWContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    GIFLZWContext(WebCore::GIFImageDecoder* client, const GIFFrameContext* frameContext)
        : stackp(0)
        , codesize(0)
        , codemask(0)
        , clearCode(0)
        , avail(0)
        , oldcode(0)
        , firstchar(0)
        , bits(0)
        , datum(0)
        , ipass(0)
        , irow(0)
        , rowPosition(0)
        , rowsRemaining(0)
        , m_client(client)
        , m_frameContext(frameContext)
    { }

    bool prepareToDecode();
    bool outputRow();
    bool doLZW(const unsigned char* block, size_t bytesInBlock);
    bool hasRemainingRows() { return rowsRemaining; }

private:
    // LZW decoding states and output states.
    size_t stackp; // Current stack pointer.
    int codesize;
    int codemask;
    int clearCode; // Codeword used to trigger dictionary reset.
    int avail; // Index of next available slot in dictionary.
    int oldcode;
    unsigned char firstchar;
    int bits; // Number of unread bits in "datum".
    int datum; // 32-bit input buffer.
    int ipass; // Interlace pass; Ranges 1-4 if interlaced.
    size_t irow; // Current output row, starting at zero.
    size_t rowPosition;
    size_t rowsRemaining; // Rows remaining to be output.

    Vector<unsigned short> prefix;
    Vector<unsigned char> suffix;
    Vector<unsigned char> stack;
    Vector<unsigned char> rowBuffer; // Single scanline temporary buffer.

    // Initialized during construction and read-only.
    WebCore::GIFImageDecoder* m_client;
    const GIFFrameContext* m_frameContext;
};

// Data structure for one LZW block.
struct GIFLZWBlock {
    WTF_MAKE_FAST_ALLOCATED;
public:
    GIFLZWBlock(size_t position, size_t size)
        : blockPosition(position)
        , blockSize(size)
    {
    }

    size_t blockPosition;
    size_t blockSize;
};

// Frame output state machine.
struct GIFFrameContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    // FIXME: Move these members to private section.
    int frameId;
    unsigned xOffset;
    unsigned yOffset; // With respect to "screen" origin.
    unsigned width;
    unsigned height;
    int tpixel; // Index of transparent pixel.
    WebCore::ImageFrame::FrameDisposalMethod disposalMethod; // Restore to background, leave in place, etc.
    size_t localColormapPosition; // Per-image colormap.
    int localColormapSize; // Size of local colormap array.
    int datasize;
    
    bool isLocalColormapDefined : 1;
    bool progressiveDisplay : 1; // If true, do Haeberli interlace hack.
    bool interlaced : 1; // True, if scanlines arrive interlaced order.
    bool isTransparent : 1; // TRUE, if tpixel is valid.

    unsigned delayTime; // Display time, in milliseconds, for this image in a multi-image GIF.

    GIFFrameContext(int id)
        : frameId(id)
        , xOffset(0)
        , yOffset(0)
        , width(0)
        , height(0)
        , tpixel(0)
        , disposalMethod(WebCore::ImageFrame::DisposeNotSpecified)
        , localColormapPosition(0)
        , localColormapSize(0)
        , datasize(0)
        , isLocalColormapDefined(false)
        , progressiveDisplay(false)
        , interlaced(false)
        , isTransparent(false)
        , delayTime(0)
        , m_currentLzwBlock(0)
        , m_isComplete(false)
        , m_isHeaderDefined(false)
        , m_isDataSizeDefined(false)
    {
    }
    
    ~GIFFrameContext()
    {
    }

    void addLzwBlock(size_t position, size_t size)
    {
        m_lzwBlocks.append(GIFLZWBlock(position, size));
    }

    bool decode(const unsigned char* data, size_t length, WebCore::GIFImageDecoder* client, bool* frameDecoded);

    bool isComplete() const { return m_isComplete; }
    void setComplete() { m_isComplete = true; }
    bool isHeaderDefined() const { return m_isHeaderDefined; }
    void setHeaderDefined() { m_isHeaderDefined = true; }
    bool isDataSizeDefined() const { return m_isDataSizeDefined; }
    void setDataSize(int size)
    {
        datasize = size;
        m_isDataSizeDefined = true;
    }

private:
    OwnPtr<GIFLZWContext> m_lzwContext;
    Vector<GIFLZWBlock> m_lzwBlocks; // LZW blocks for this frame.
    size_t m_currentLzwBlock;
    bool m_isComplete;
    bool m_isHeaderDefined;
    bool m_isDataSizeDefined;
};

class GIFImageReader {
    WTF_MAKE_FAST_ALLOCATED;
public:
    GIFImageReader(WebCore::GIFImageDecoder* client = 0)
        : m_client(client)
        , m_state(GIFType)
        , m_bytesToConsume(6) // Number of bytes for GIF type, either "GIF87a" or "GIF89a".
        , m_bytesRead(0)
        , m_screenBgcolor(0)
        , m_version(0)
        , m_screenWidth(0)
        , m_screenHeight(0)
        , m_isGlobalColormapDefined(false)
        , m_globalColormapPosition(0)
        , m_globalColormapSize(0)
        , m_loopCount(cLoopCountNotSeen)
        , m_currentDecodingFrame(0)
        , m_parseCompleted(false)
    {
    }

    ~GIFImageReader()
    {
    }

    void setData(PassRefPtr<WebCore::SharedBuffer> data) { m_data = data; }
    // FIXME: haltAtFrame should be size_t.
    bool decode(WebCore::GIFImageDecoder::GIFQuery, unsigned haltAtFrame);

    size_t imagesCount() const
    {
        if (m_frames.isEmpty())
            return 0;

        // This avoids counting an empty frame when the file is truncated right after
        // GIFControlExtension but before GIFImageHeader.
        // FIXME: This extra complexity is not necessary and we should just report m_frames.size().
        return m_frames.last()->isHeaderDefined() ? m_frames.size() : m_frames.size() - 1;
    }
    int loopCount() const { return m_loopCount; }

    const unsigned char* globalColormap() const
    {
        return m_isGlobalColormapDefined ? data(m_globalColormapPosition) : 0;
    }
    int globalColormapSize() const
    {
        return m_isGlobalColormapDefined ? m_globalColormapSize : 0;
    }

    const unsigned char* localColormap(const GIFFrameContext* frame) const
    {
        return frame->isLocalColormapDefined ? data(frame->localColormapPosition) : 0;
    }
    int localColormapSize(const GIFFrameContext* frame) const
    {
        return frame->isLocalColormapDefined ? frame->localColormapSize : 0;
    }

    const GIFFrameContext* frameContext(size_t index) const
    {
        return index < m_frames.size() ? m_frames[index].get() : 0;
    }

    bool parseCompleted() const { return m_parseCompleted; }

private:
    bool parse(size_t dataPosition, size_t len, bool parseSizeOnly);
    void setRemainingBytes(size_t);

    const unsigned char* data(size_t dataPosition) const
    {
        return reinterpret_cast<const unsigned char*>(m_data->data()) + dataPosition;
    }

    void addFrameIfNecessary();
    bool currentFrameIsFirstFrame() const
    {
        return m_frames.isEmpty() || (m_frames.size() == 1u && !m_frames[0]->isComplete());
    }

    WebCore::GIFImageDecoder* m_client;

    // Parsing state machine.
    GIFState m_state; // Current decoder master state.
    size_t m_bytesToConsume; // Number of bytes to consume for next stage of parsing.
    size_t m_bytesRead; // Number of bytes processed.
    
    // Global (multi-image) state.
    int m_screenBgcolor; // Logical screen background color.
    int m_version; // Either 89 for GIF89 or 87 for GIF87.
    unsigned m_screenWidth; // Logical screen width & height.
    unsigned m_screenHeight;
    bool m_isGlobalColormapDefined;
    size_t m_globalColormapPosition; // (3* MAX_COLORS in size) Default colormap if local not supplied, 3 bytes for each color.
    int m_globalColormapSize; // Size of global colormap array.
    int m_loopCount; // Netscape specific extension block to control the number of animation loops a GIF renders.
    
    Vector<OwnPtr<GIFFrameContext> > m_frames;
    size_t m_currentDecodingFrame;

    RefPtr<WebCore::SharedBuffer> m_data;
    bool m_parseCompleted;
};

#endif

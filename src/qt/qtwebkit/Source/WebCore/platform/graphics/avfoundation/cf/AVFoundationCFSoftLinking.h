/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#include "SoftLinking.h"

// Soft-link against AVFoundationCF functions and variables required by MediaPlayerPrivateAVFoundationCF.cpp.

#ifdef DEBUG_ALL
// FIXME: <rdar://problem/9898937> AVFoundationCF doesn't currently deliver a debug library.
SOFT_LINK_LIBRARY(AVFoundationCF)
#else
SOFT_LINK_LIBRARY(AVFoundationCF)
#endif

// Functions

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetCancelLoading, void, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetCancelLoading softLink_AVCFAssetCancelLoading

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetCopyAssetTracks, CFArrayRef, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetCopyAssetTracks softLink_AVCFAssetCopyAssetTracks

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetCopyAvailableMediaCharacteristicsWithMediaSelectionOptions, CFArrayRef, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetCopyAvailableMediaCharacteristicsWithMediaSelectionOptions softLink_AVCFAssetCopyAvailableMediaCharacteristicsWithMediaSelectionOptions

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetCopyTracksWithMediaCharacteristic, CFArrayRef, __cdecl, (AVCFAssetRef asset, CFStringRef mediaCharacteristic), (asset, mediaCharacteristic))
#define AVCFAssetCopyTracksWithMediaCharacteristic softLink_AVCFAssetCopyTracksWithMediaCharacteristic

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetCopyTracksWithMediaType, CFArrayRef, __cdecl, (AVCFAssetRef asset, CFStringRef mediaType), (asset, mediaType))
#define AVCFAssetCopyTracksWithMediaType softLink_AVCFAssetCopyTracksWithMediaType

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetGetDuration, CMTime, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetGetDuration softLink_AVCFAssetGetDuration

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetGetNaturalSize, CGSize, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetGetNaturalSize softLink_AVCFAssetGetNaturalSize

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetGetPreferredTransform, CGAffineTransform, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetGetPreferredTransform softLink_AVCFAssetGetPreferredTransform

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetGetStatusOfValueForProperty, AVCFPropertyValueStatus, __cdecl, (AVCFAssetRef asset, CFStringRef property, CFErrorRef *errorOut), (asset, property, errorOut))
#define AVCFAssetGetStatusOfValueForProperty softLink_AVCFAssetGetStatusOfValueForProperty

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorCopyCGImageAtTime, CGImageRef, __cdecl, (AVCFAssetImageGeneratorRef generator, CMTime requestedTime, CMTime *actualTimeOut, CFErrorRef *errorOut), (generator, requestedTime, actualTimeOut, errorOut))
#define AVCFAssetImageGeneratorCopyCGImageAtTime softLink_AVCFAssetImageGeneratorCopyCGImageAtTime

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorCreateWithAsset, AVCFAssetImageGeneratorRef, __cdecl, (CFAllocatorRef allocator, AVCFAssetRef asset), (allocator, asset))
#define AVCFAssetImageGeneratorCreateWithAsset softLink_AVCFAssetImageGeneratorCreateWithAsset

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorSetApertureMode, void, __cdecl, (AVCFAssetImageGeneratorRef generator, CFStringRef mode), (generator, mode))
#define AVCFAssetImageGeneratorSetApertureMode softLink_AVCFAssetImageGeneratorSetApertureMode

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorSetAppliesPreferredTrackTransform, void, __cdecl, (AVCFAssetImageGeneratorRef generator, Boolean appliesTransfrom), (generator, appliesTransfrom))
#define AVCFAssetImageGeneratorSetAppliesPreferredTrackTransform softLink_AVCFAssetImageGeneratorSetAppliesPreferredTrackTransform

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorSetMaximumSize, void, __cdecl, (AVCFAssetImageGeneratorRef generator, CGSize maxSize), (generator, maxSize))
#define AVCFAssetImageGeneratorSetMaximumSize softLink_AVCFAssetImageGeneratorSetMaximumSize

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorSetRequestedTimeToleranceAfter, void, __cdecl, (AVCFAssetImageGeneratorRef generator, CMTime toleranceAfter), (generator, toleranceAfter))
#define AVCFAssetImageGeneratorSetRequestedTimeToleranceAfter softLink_AVCFAssetImageGeneratorSetRequestedTimeToleranceAfter

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorSetRequestedTimeToleranceBefore, void, __cdecl, (AVCFAssetImageGeneratorRef generator, CMTime toleranceBefore), (generator, toleranceBefore))
#define AVCFAssetImageGeneratorSetRequestedTimeToleranceBefore softLink_AVCFAssetImageGeneratorSetRequestedTimeToleranceBefore

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetIsPlayable, Boolean, __cdecl, (AVCFAssetRef asset), (asset))
#define AVCFAssetIsPlayable softLink_AVCFAssetIsPlayable

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetLoadValuesAsynchronouslyForProperties, void, __cdecl, (AVCFAssetRef asset, CFArrayRef properties, AVCFAssetLoadValuesCompletionCallback callback, void *clientContext), (asset, properties, callback, clientContext))
#define AVCFAssetLoadValuesAsynchronouslyForProperties softLink_AVCFAssetLoadValuesAsynchronouslyForProperties

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackCopyCommonMetadata, CFArrayRef, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackCopyCommonMetadata softLink_AVCFAssetTrackCopyCommonMetadata

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackCopyExtendedLanguageTag, CFStringRef, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackCopyExtendedLanguageTag softLink_AVCFAssetTrackCopyExtendedLanguageTag

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackCopyLanguageCode, CFStringRef, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackCopyLanguageCode softLink_AVCFAssetTrackCopyLanguageCode

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackGetMediaType, CFStringRef, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackGetMediaType softLink_AVCFAssetTrackGetMediaType

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackGetNaturalSize, CGSize, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackGetNaturalSize softLink_AVCFAssetTrackGetNaturalSize

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackGetPreferredTransform, CGAffineTransform, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackGetPreferredTransform softLink_AVCFAssetTrackGetPreferredTransform

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetTrackGetTotalSampleDataLength, int64_t, __cdecl, (AVCFAssetTrackRef assetTrack), (assetTrack))
#define AVCFAssetTrackGetTotalSampleDataLength softLink_AVCFAssetTrackGetTotalSampleDataLength

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionCopyOptionsFromArrayFilteredAndSortedAccordingToPreferredLanguages, CFArrayRef, __cdecl, (CFArrayRef selectionOptions, CFArrayRef preferredLanguages), (selectionOptions, preferredLanguages))
#define AVCFMediaSelectionCopyOptionsFromArrayFilteredAndSortedAccordingToPreferredLanguages softLink_AVCFMediaSelectionCopyOptionsFromArrayFilteredAndSortedAccordingToPreferredLanguages

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMetadataItemCopyStringValue, CFStringRef, __cdecl, (AVCFMetadataItemRef metadataItem), (metadataItem))
#define AVCFMetadataItemCopyStringValue softLink_AVCFMetadataItemCopyStringValue

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMetadataItemCopyItemsWithKeyAndKeySpace, CFArrayRef, __cdecl, (CFArrayRef array, CFTypeRef key, CFStringRef keySpace), (array, key, keySpace))
#define AVCFMetadataItemCopyItemsWithKeyAndKeySpace softLink_AVCFMetadataItemCopyItemsWithKeyAndKeySpace

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMetadataItemCopyItemsWithLocale, CFArrayRef, __cdecl, (CFArrayRef items, CFLocaleRef locale), (items, locale))
#define AVCFMetadataItemCopyItemsWithLocale softLink_AVCFMetadataItemCopyItemsWithLocale

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerCreatePeriodicTimeObserverForInterval, AVCFPlayerObserverRef, __cdecl, (AVCFPlayerRef player, CMTime interval, dispatch_queue_t queue, AVCFPlayerPeriodicTimeObserverCallback callback, void *clientContext), (player, interval, queue, callback, clientContext))
#define AVCFPlayerCreatePeriodicTimeObserverForInterval softLink_AVCFPlayerCreatePeriodicTimeObserverForInterval

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemCreateWithAsset, AVCFPlayerItemRef, __cdecl, (CFAllocatorRef allocator, AVCFAssetRef asset, dispatch_queue_t notificationQueue), (allocator, asset, notificationQueue))
#define AVCFPlayerItemCreateWithAsset softLink_AVCFPlayerItemCreateWithAsset

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerCreateWithPlayerItemAndOptions, AVCFPlayerRef, __cdecl, (CFAllocatorRef allocator, AVCFPlayerItemRef playerItem, CFDictionaryRef options, dispatch_queue_t notificationQueue), (allocator, playerItem, options, notificationQueue))
#define AVCFPlayerCreateWithPlayerItemAndOptions softLink_AVCFPlayerCreateWithPlayerItemAndOptions

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemCopyLoadedTimeRanges, CFArrayRef, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemCopyLoadedTimeRanges softLink_AVCFPlayerItemCopyLoadedTimeRanges

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemCopySeekableTimeRanges, CFArrayRef, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemCopySeekableTimeRanges softLink_AVCFPlayerItemCopySeekableTimeRanges

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemCopyTracks, CFArrayRef, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemCopyTracks softLink_AVCFPlayerItemCopyTracks

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemGetAsset, AVCFAssetRef, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemGetAsset softLink_AVCFPlayerItemGetAsset

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemGetCurrentTime, CMTime, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemGetCurrentTime softLink_AVCFPlayerItemGetCurrentTime

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemGetDuration, CMTime, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemGetDuration softLink_AVCFPlayerItemGetDuration

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemGetStatus, AVCFPlayerItemStatus, __cdecl, (AVCFPlayerItemRef playerItem, CFErrorRef *errorOut), (playerItem, errorOut))
#define AVCFPlayerItemGetStatus softLink_AVCFPlayerItemGetStatus

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemIsPlaybackBufferEmpty, Boolean, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemIsPlaybackBufferEmpty softLink_AVCFPlayerItemIsPlaybackBufferEmpty

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemIsPlaybackBufferFull, Boolean, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemIsPlaybackBufferFull softLink_AVCFPlayerItemIsPlaybackBufferFull

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemIsPlaybackLikelyToKeepUp, Boolean, __cdecl, (AVCFPlayerItemRef playerItem), (playerItem))
#define AVCFPlayerItemIsPlaybackLikelyToKeepUp softLink_AVCFPlayerItemIsPlaybackLikelyToKeepUp

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLegibleOutputGetTypeID, CFTypeID, __cdecl, (), ())
#define AVCFPlayerItemLegibleOutputGetTypeID softLink_AVCFPlayerItemLegibleOutputGetTypeID

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemRemoveOutput, void, __cdecl, (AVCFPlayerItemRef playerItem, AVCFPlayerItemOutputRef output), (playerItem, output))
#define AVCFPlayerItemRemoveOutput softLink_AVCFPlayerItemRemoveOutput

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemSeekToTimeWithToleranceAndCompletionCallback, AVCFAssetRef, __cdecl, (AVCFPlayerItemRef playerItem, CMTime time, CMTime toleranceBefore, CMTime toleranceAfter, AVCFPlayerItemSeekCompletionCallback completionCallback, void *context), (playerItem, time, toleranceBefore, toleranceAfter, completionCallback, context))
#define AVCFPlayerItemSeekToTimeWithToleranceAndCompletionCallback softLink_AVCFPlayerItemSeekToTimeWithToleranceAndCompletionCallback

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemTrackCopyAssetTrack, AVCFAssetTrackRef, __cdecl, (AVCFPlayerItemTrackRef track), (track))
#define AVCFPlayerItemTrackCopyAssetTrack softLink_AVCFPlayerItemTrackCopyAssetTrack

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemTrackIsEnabled, Boolean, __cdecl, (AVCFPlayerItemTrackRef track), (track))
#define AVCFPlayerItemTrackIsEnabled softLink_AVCFPlayerItemTrackIsEnabled

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerGetRate, Float32, __cdecl, (AVCFPlayerRef player), (player))
#define AVCFPlayerGetRate softLink_AVCFPlayerGetRate

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerLayerCopyCACFLayer, CACFLayerRef, __cdecl, (AVCFPlayerLayerRef playerLayer), (playerLayer))
#define AVCFPlayerLayerCopyCACFLayer softLink_AVCFPlayerLayerCopyCACFLayer

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerLayerCreateWithAVCFPlayer, AVCFPlayerLayerRef, __cdecl, (CFAllocatorRef allocator, AVCFPlayerRef player, dispatch_queue_t notificationQueue), (allocator, player, notificationQueue))
#define AVCFPlayerLayerCreateWithAVCFPlayer softLink_AVCFPlayerLayerCreateWithAVCFPlayer

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerLayerIsReadyForDisplay, Boolean, __cdecl, (AVCFPlayerLayerRef playerLayer), (playerLayer))
#define AVCFPlayerLayerIsReadyForDisplay softLink_AVCFPlayerLayerIsReadyForDisplay

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerLayerSetPlayer, void, __cdecl, (AVCFPlayerLayerRef playerLayer, AVCFPlayerRef player), (playerLayer, player))
#define AVCFPlayerLayerSetPlayer softLink_AVCFPlayerLayerSetPlayer

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerPlay, void, __cdecl, (AVCFPlayerRef player), (player))
#define AVCFPlayerPlay softLink_AVCFPlayerPlay

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerRemoveObserver, OSStatus, __cdecl, (AVCFPlayerRef player, AVCFPlayerObserverRef observer), (player, observer))
#define AVCFPlayerRemoveObserver softLink_AVCFPlayerRemoveObserver

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerSetClosedCaptionDisplayEnabled, void, __cdecl, (AVCFPlayerRef player, Boolean enabled), (player, enabled))
#define AVCFPlayerSetClosedCaptionDisplayEnabled softLink_AVCFPlayerSetClosedCaptionDisplayEnabled

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerSetRate, void, __cdecl, (AVCFPlayerRef player, Float32 rate), (player, rate))
#define AVCFPlayerSetRate softLink_AVCFPlayerSetRate

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerSetVolume, void, __cdecl, (AVCFPlayerRef player, Float32 volume), (player, volume))
#define AVCFPlayerSetVolume softLink_AVCFPlayerSetVolume

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFURLAssetCopyAudiovisualMIMETypes, CFArrayRef, __cdecl, (), ())
#define AVCFURLAssetCopyAudiovisualMIMETypes softLink_AVCFURLAssetCopyAudiovisualMIMETypes

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFURLAssetCreateWithURLAndOptions, AVCFURLAssetRef, __cdecl, (CFAllocatorRef allocator, CFURLRef URL, CFDictionaryRef options, dispatch_queue_t notificationQueue), (allocator, URL, options, notificationQueue))
#define AVCFURLAssetCreateWithURLAndOptions softLink_AVCFURLAssetCreateWithURLAndOptions

SOFT_LINK_DLL_IMPORT_OPTIONAL(AVFoundationCF, AVCFPlayerSetDirect3DDevice, void, __cdecl, (AVCFPlayerRef player, IDirect3DDevice9* d3dDevice))

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFAssetGetSelectionGroupForMediaCharacteristic , AVCFMediaSelectionGroupRef, __cdecl, (AVCFAssetRef asset, CFStringRef mediaCharacteristic), (asset, mediaCharacteristic))
#define AVCFAssetGetSelectionGroupForMediaCharacteristic  softLink_AVCFAssetGetSelectionGroupForMediaCharacteristic 

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionCopyPlayableOptionsFromArray, CFArrayRef, __cdecl, (CFArrayRef selectionOptions), (selectionOptions))
#define AVCFMediaSelectionCopyPlayableOptionsFromArray softLink_AVCFMediaSelectionCopyPlayableOptionsFromArray

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionOptionCopyCommonMetadata, CFArrayRef, __cdecl, (AVCFMediaSelectionOptionRef selectionOption), (selectionOption))
#define AVCFMediaSelectionOptionCopyCommonMetadata softLink_AVCFMediaSelectionOptionCopyCommonMetadata

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionOptionCopyLocale, CFLocaleRef, __cdecl, (AVCFMediaSelectionOptionRef selectionOption), (selectionOption))
#define AVCFMediaSelectionOptionCopyLocale softLink_AVCFMediaSelectionOptionCopyLocale

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionOptionGetMediaType, CFStringRef, __cdecl, (AVCFMediaSelectionOptionRef selectionOption), (selectionOption))
#define AVCFMediaSelectionOptionGetMediaType softLink_AVCFMediaSelectionOptionGetMediaType

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionOptionHasMediaCharacteristic, Boolean, __cdecl, (AVCFMediaSelectionOptionRef selectionOption, CFStringRef mediaCharacteristic), (selectionOption, mediaCharacteristic))
#define AVCFMediaSelectionOptionHasMediaCharacteristic softLink_AVCFMediaSelectionOptionHasMediaCharacteristic

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionGroupAllowsEmptySelection, Boolean, __cdecl, (AVCFMediaSelectionGroupRef selectionGroup), (selectionGroup))
#define AVCFMediaSelectionGroupAllowsEmptySelection softLink_AVCFMediaSelectionGroupAllowsEmptySelection

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFMediaSelectionGroupGetOptions, CFArrayRef, __cdecl, (AVCFMediaSelectionGroupRef selectionGroup), (selectionGroup))
#define AVCFMediaSelectionGroupGetOptions softLink_AVCFMediaSelectionGroupGetOptions

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemAddOutput, void, __cdecl, (AVCFPlayerItemRef playerItem, AVCFPlayerItemOutputRef output), (playerItem, output))
#define AVCFPlayerItemAddOutput softLink_AVCFPlayerItemAddOutput

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLegibleOutputCreateWithMediaSubtypesForNativeRepresentation, AVCFPlayerItemLegibleOutputRef, __cdecl, (CFAllocatorRef allocator, CFArrayRef subtypes), (allocator, subtypes))
#define AVCFPlayerItemLegibleOutputCreateWithMediaSubtypesForNativeRepresentation softLink_AVCFPlayerItemLegibleOutputCreateWithMediaSubtypesForNativeRepresentation

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemGetSelectedMediaOptionInMediaSelectionGroup, AVCFMediaSelectionOptionRef, __cdecl, (AVCFPlayerItemRef playerItem, AVCFMediaSelectionGroupRef selectionGroup), (playerItem, selectionGroup))
#define AVCFPlayerItemGetSelectedMediaOptionInMediaSelectionGroup softLink_AVCFPlayerItemGetSelectedMediaOptionInMediaSelectionGroup

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLegibleOutputSetAdvanceIntervalForCallbackInvocation, void, __cdecl, (AVCFPlayerItemLegibleOutputRef legibleOutput, CFTimeInterval timeInterval), (legibleOutput, timeInterval))
#define AVCFPlayerItemLegibleOutputSetAdvanceIntervalForCallbackInvocation softLink_AVCFPlayerItemLegibleOutputSetAdvanceIntervalForCallbackInvocation

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLegibleOutputSetCallbacks, void, __cdecl, (AVCFPlayerItemLegibleOutputRef output, const AVCFPlayerItemLegibleOutputCallbacks* callbacks, dispatch_queue_t callbackQueue), (output, callbacks, callbackQueue))
#define AVCFPlayerItemLegibleOutputSetCallbacks softLink_AVCFPlayerItemLegibleOutputSetCallbacks

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLegibleOutputSetTextStylingResolution, void, __cdecl, (AVCFPlayerItemLegibleOutputRef output, CFStringRef textStylingResolution), (output, textStylingResolution))
#define AVCFPlayerItemLegibleOutputSetTextStylingResolution softLink_AVCFPlayerItemLegibleOutputSetTextStylingResolution

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemOutputSetSuppressPlayerRendering, void, __cdecl, (AVCFPlayerItemOutputRef output, Boolean suppressed), (output, suppressed))
#define AVCFPlayerItemOutputSetSuppressPlayerRendering softLink_AVCFPlayerItemOutputSetSuppressPlayerRendering

SOFT_LINK_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemSelectMediaOptionInMediaSelectionGroup, void, __cdecl, (AVCFPlayerItemRef playerItem, AVCFMediaSelectionOptionRef selectionOption, AVCFMediaSelectionGroupRef selectionGroup), (playerItem, selectionOption, selectionGroup))
#define AVCFPlayerItemSelectMediaOptionInMediaSelectionGroup softLink_AVCFPlayerItemSelectMediaOptionInMediaSelectionGroup

#endif

// Variables

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetImageGeneratorApertureModeCleanAperture, const CFStringRef);
#define AVCFAssetImageGeneratorApertureModeCleanAperture getAVCFAssetImageGeneratorApertureModeCleanAperture()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyDuration, const CFStringRef);
#define AVCFAssetPropertyDuration getAVCFAssetPropertyDuration()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyNaturalSize, const CFStringRef);
#define AVCFAssetPropertyNaturalSize getAVCFAssetPropertyNaturalSize()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyPlayable, const CFStringRef);
#define AVCFAssetPropertyPlayable getAVCFAssetPropertyPlayable()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyPreferredRate, const CFStringRef);
#define AVCFAssetPropertyPreferredRate getAVCFAssetPropertyPreferredRate()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyPreferredTransform, const CFStringRef);
#define AVCFAssetPropertyPreferredTransform getAVCFAssetPropertyPreferredTransform()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyTracks, const CFStringRef);
#define AVCFAssetPropertyTracks getAVCFAssetPropertyTracks()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicAudible, const CFStringRef);
#define AVCFMediaCharacteristicAudible getAVCFMediaCharacteristicAudible()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicVisual, const CFStringRef);
#define AVCFMediaCharacteristicVisual getAVCFMediaCharacteristicVisual()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaTypeAudio, const CFStringRef);
#define AVCFMediaTypeAudio getAVCFMediaTypeAudio()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaTypeClosedCaption, const CFStringRef);
#define AVCFMediaTypeClosedCaption getAVCFMediaTypeClosedCaption()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaTypeVideo, const CFStringRef);
#define AVCFMediaTypeVideo getAVCFMediaTypeVideo()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMetadataCommonKeyTitle, const CFStringRef);
#define AVCFMetadataCommonKeyTitle getAVCFMetadataCommonKeyTitle()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMetadataKeySpaceCommon, const CFStringRef);
#define AVCFMetadataKeySpaceCommon getAVCFMetadataKeySpaceCommon()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemDidPlayToEndTimeNotification, const CFStringRef);
#define AVCFPlayerItemDidPlayToEndTimeNotification getAVCFPlayerItemDidPlayToEndTimeNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemDurationChangedNotification, const CFStringRef);
#define AVCFPlayerItemDurationChangedNotification getAVCFPlayerItemDurationChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemIsPlaybackBufferEmptyChangedNotification, const CFStringRef);
#define AVCFPlayerItemIsPlaybackBufferEmptyChangedNotification getAVCFPlayerItemIsPlaybackBufferEmptyChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemIsPlaybackBufferFullChangedNotification, const CFStringRef);
#define AVCFPlayerItemIsPlaybackBufferFullChangedNotification getAVCFPlayerItemIsPlaybackBufferFullChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemIsPlaybackLikelyToKeepUpChangedNotification, const CFStringRef);
#define AVCFPlayerItemIsPlaybackLikelyToKeepUpChangedNotification getAVCFPlayerItemIsPlaybackLikelyToKeepUpChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLoadedTimeRangesChangedNotification, const CFStringRef);
#define AVCFPlayerItemLoadedTimeRangesChangedNotification getAVCFPlayerItemLoadedTimeRangesChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemPresentationSizeChangedNotification, const CFStringRef);
#define AVCFPlayerItemPresentationSizeChangedNotification getAVCFPlayerItemPresentationSizeChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemSeekableTimeRangesChangedNotification, const CFStringRef);
#define AVCFPlayerItemSeekableTimeRangesChangedNotification getAVCFPlayerItemSeekableTimeRangesChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemStatusChangedNotification, const CFStringRef);
#define AVCFPlayerItemStatusChangedNotification getAVCFPlayerItemStatusChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemTracksChangedNotification, const CFStringRef);
#define AVCFPlayerItemTracksChangedNotification getAVCFPlayerItemTracksChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerRateChangedNotification, const CFStringRef);
#define AVCFPlayerRateChangedNotification getAVCFPlayerRateChangedNotification()

SOFT_LINK_VARIABLE_DLL_IMPORT_OPTIONAL(AVFoundationCF, AVCFPlayerEnableHardwareAcceleratedVideoDecoderKey, const CFStringRef);
#define AVCFPlayerEnableHardwareAcceleratedVideoDecoderKey getAVCFPlayerEnableHardwareAcceleratedVideoDecoderKey()

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFAssetPropertyAvailableMediaCharacteristicsWithMediaSelectionOptions, const CFStringRef);
#define AVCFAssetPropertyAvailableMediaCharacteristicsWithMediaSelectionOptions  getAVCFAssetPropertyAvailableMediaCharacteristicsWithMediaSelectionOptions()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicEasyToRead, const CFStringRef);
#define AVCFMediaCharacteristicEasyToRead getAVCFMediaCharacteristicEasyToRead()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicContainsOnlyForcedSubtitles, const CFStringRef);
#define AVCFMediaCharacteristicContainsOnlyForcedSubtitles getAVCFMediaCharacteristicContainsOnlyForcedSubtitles()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicDescribesMusicAndSoundForAccessibility, const CFStringRef);
#define AVCFMediaCharacteristicDescribesMusicAndSoundForAccessibility getAVCFMediaCharacteristicDescribesMusicAndSoundForAccessibility()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicIsMainProgramContent, const CFStringRef);
#define AVCFMediaCharacteristicIsMainProgramContent getAVCFMediaCharacteristicIsMainProgramContent()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicLegible, const CFStringRef);
#define AVCFMediaCharacteristicLegible getAVCFMediaCharacteristicLegible()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaCharacteristicTranscribesSpokenDialogForAccessibility, const CFStringRef);
#define AVCFMediaCharacteristicTranscribesSpokenDialogForAccessibility getAVCFMediaCharacteristicTranscribesSpokenDialogForAccessibility()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFMediaTypeSubtitle, const CFStringRef);
#define AVCFMediaTypeSubtitle getAVCFMediaTypeSubtitle()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerAppliesMediaSelectionCriteriaAutomaticallyKey, const CFStringRef);
#define AVCFPlayerAppliesMediaSelectionCriteriaAutomaticallyKey getAVCFPlayerAppliesMediaSelectionCriteriaAutomaticallyKey()

SOFT_LINK_VARIABLE_DLL_IMPORT(AVFoundationCF, AVCFPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly, const CFStringRef);
#define AVCFPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly getAVCFPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly()

#endif

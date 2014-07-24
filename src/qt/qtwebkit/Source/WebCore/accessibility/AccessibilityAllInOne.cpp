/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

// This all-in-one cpp file cuts down on template bloat to allow us to build our Windows release build.

#include "AXObjectCache.cpp"
#include "AccessibilityARIAGrid.cpp"
#include "AccessibilityARIAGridCell.cpp"
#include "AccessibilityARIAGridRow.cpp"
#include "AccessibilityImageMapLink.cpp"
#include "AccessibilityList.cpp"
#include "AccessibilityListBox.cpp"
#include "AccessibilityListBoxOption.cpp"
#include "AccessibilityMediaControls.cpp"
#include "AccessibilityNodeObject.cpp"
#include "AccessibilityObject.cpp"
#include "AccessibilityProgressIndicator.cpp"
#include "AccessibilityRenderObject.cpp"
#include "AccessibilitySVGRoot.cpp"
#include "AccessibilityScrollView.cpp"
#include "AccessibilityScrollbar.cpp"
#include "AccessibilitySlider.cpp"
#include "AccessibilityTable.cpp"
#include "AccessibilityTableCell.cpp"
#include "AccessibilityTableColumn.cpp"
#include "AccessibilityTableHeaderContainer.cpp"
#include "AccessibilityTableRow.cpp"


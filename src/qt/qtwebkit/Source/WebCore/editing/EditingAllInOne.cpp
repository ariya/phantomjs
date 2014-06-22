/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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

#include <AlternativeTextController.cpp>
#include <AppendNodeCommand.cpp>
#include <ApplyBlockElementCommand.cpp>
#include <ApplyStyleCommand.cpp>
#include <BreakBlockquoteCommand.cpp>
#include <CompositeEditCommand.cpp>
#include <CreateLinkCommand.cpp>
#include <DeleteButton.cpp>
#include <DeleteButtonController.cpp>
#include <DeleteFromTextNodeCommand.cpp>
#include <DeleteSelectionCommand.cpp>
#include <DictationAlternative.cpp>
#include <DictationCommand.cpp>
#include <EditCommand.cpp>
#include <EditingStyle.cpp>
#include <Editor.cpp>
#include <EditorCommand.cpp>
#include <FormatBlockCommand.cpp>
#include <FrameSelection.cpp>
#include <HTMLInterchange.cpp>
#include <IndentOutdentCommand.cpp>
#include <InsertIntoTextNodeCommand.cpp>
#include <InsertLineBreakCommand.cpp>
#include <InsertListCommand.cpp>
#include <InsertNodeBeforeCommand.cpp>
#include <InsertParagraphSeparatorCommand.cpp>
#include <InsertTextCommand.cpp>
#include <MarkupAccumulator.cpp>
#include <MergeIdenticalElementsCommand.cpp>
#include <ModifySelectionListLevel.cpp>
#include <MoveSelectionCommand.cpp>
#include <RemoveCSSPropertyCommand.cpp>
#include <RemoveFormatCommand.cpp>
#include <RemoveNodeCommand.cpp>
#include <RemoveNodePreservingChildrenCommand.cpp>
#include <RenderedPosition.cpp>
#include <ReplaceNodeWithSpanCommand.cpp>
#include <ReplaceSelectionCommand.cpp>
#include <SetNodeAttributeCommand.cpp>
#include <SetSelectionCommand.cpp>
#include <SimplifyMarkupCommand.cpp>
#include <SmartReplace.cpp>
#if USE(CF)
#include <SmartReplaceCF.cpp>
#endif
#include <SpellingCorrectionCommand.cpp>
#include <SpellChecker.cpp>
#include <SplitElementCommand.cpp>
#include <SplitTextNodeCommand.cpp>
#include <SplitTextNodeContainingElementCommand.cpp>
#include <TextCheckingHelper.cpp>
#include <TextInsertionBaseCommand.cpp>
#include <TextIterator.cpp>
#include <TypingCommand.cpp>
#include <UnlinkCommand.cpp>
#include <VisiblePosition.cpp>
#include <VisibleSelection.cpp>
#include <VisibleUnits.cpp>
#include <WrapContentsInDummySpanCommand.cpp>
#include <htmlediting.cpp>
#include <markup.cpp>

/*
 * Copyright (C) Research In Motion Limited 2013. All rights reserved.
 */

#ifndef SpellingHandler_h
#define SpellingHandler_h

#include "TextCheckerClient.h"
#include "Timer.h"
#include "VisiblePosition.h"
#include "VisibleSelection.h"

#include <wtf/RefPtr.h>

/**
 * SpellingHandler
 */

namespace WebCore {
class Range;
}

namespace BlackBerry {
namespace WebKit {

class InputHandler;

class SpellingHandler {
public:
    SpellingHandler(InputHandler*);
    ~SpellingHandler();

    void spellCheckTextBlock(const WebCore::Element*, const WebCore::TextCheckingProcessType);
    bool isSpellCheckActive() { return m_isSpellCheckActive; }
    void setSpellCheckActive(bool active) { m_isSpellCheckActive = active; }

private:
    void createSpellCheckRequest(PassRefPtr<WebCore::Range> rangeForSpellCheckingPtr);
    void parseBlockForSpellChecking(WebCore::Timer<SpellingHandler>*);
    void incrementSentinels(bool shouldIncrementStartPosition);
    PassRefPtr<WebCore::Range> handleOversizedRange();

    InputHandler* m_inputHandler;

    WebCore::Timer<SpellingHandler> m_iterationDelayTimer;
    WebCore::VisiblePosition m_startPosition;
    WebCore::VisiblePosition m_endPosition;
    WebCore::VisiblePosition m_cachedEndPosition;
    WebCore::VisiblePosition m_endOfRange;
    WebCore::TextCheckingProcessType m_textCheckingProcessType;
    bool m_isSpellCheckActive;
};

} // WebKit
} // BlackBerry

#endif // SpellingHandler_h

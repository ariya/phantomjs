/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief covers API from ewk_text_checker.h
 * @file test_ewk2_text_checker.cpp
 */

#include "config.h"

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static const uint64_t defaultDocumentTag = 123;
static const char expectedMisspelledWord[] = "aa";
static const Evas_Object* defaultView = 0;
static bool isSettingEnabled = false;
static Ecore_Timer* timeoutTimer = 0;
static double defaultTimeoutInSeconds = 0.5;

static bool wasContextMenuShown = false;
static const char noGuessesString[] = "No Guesses Found";
static const char ignoreSpellingString[] = "Ignore Spelling";
static const char learnSpellingString[] = "Learn Spelling";

static const char* clientSuggestionsForWord[] = { "clientSuggestion1", "clientSuggestion2", "clientSuggestion3" };
static unsigned contextMenuItemsNumber = 0;
static String knownWord;

/**
 * Structure keeps information which callbacks were called.
 * Its values are reset before each test.
 */
static struct {
    bool settingChange;
    bool spellDocumentTag;
    bool spellDocumentTagClose;
    bool spellingCheck;
    bool wordGuesses;
    bool wordLearn;
    bool wordIgnore;
} callbacksExecutionStats;

class EWK2TextCheckerTest : public EWK2UnitTestBase {
public:
    static void resetCallbacksExecutionStats()
    {
        callbacksExecutionStats.settingChange = false;
        callbacksExecutionStats.spellDocumentTag = false;
        callbacksExecutionStats.spellDocumentTagClose = false;
        callbacksExecutionStats.spellingCheck = false;
        callbacksExecutionStats.wordGuesses = false;
        callbacksExecutionStats.wordLearn = false;
        callbacksExecutionStats.wordIgnore = false;
    }

    /**
     * Handle the timeout, it may happen for the asynchronous tests.
     *
     * @internal
     *
     * @return the ECORE_CALLBACK_CANCEL flag to delete the timer automatically
     */
    static Eina_Bool onTimeout(void*)
    {
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
    }

    /**
     * This callback tests whether the client's callback is called when the spell checking setting was changed.
     *
     * @internal
     *
     * Verify the new setting value (passes in the @a flag parameter) if it equals to the previously set.
     *
     * @internal
     *
     * @param flag the new setting value
     */
    static void onSettingChange(Eina_Bool flag)
    {
        EXPECT_EQ(isSettingEnabled, flag);
        callbacksExecutionStats.settingChange = true;
    }

    /**
     * Returns unique tag (an identifier).
     *
     * @internal
     *
     * It will be used for onSpellingCheck, onWordGuesses etc. to notify
     * the client on which object (associated to the tag) the spelling is being invoked.
     *
     * @param ewkView the view object to get unique tag
     *
     * @return unique tag for the given @a ewkView object
     */
    static uint64_t onSpellDocumentTag(const Evas_Object* ewkView)
    {
        EXPECT_EQ(defaultView, ewkView);
        callbacksExecutionStats.spellDocumentTag = true;

        return defaultDocumentTag;
    }

    /**
     * The view which is associated to the @a tag has been destroyed.
     *
     * @internal
     *
     * @param tag the tag to be closed
     */
    static void onSpellDocumentTagClose(uint64_t tag)
    {
        ASSERT_EQ(defaultDocumentTag, tag);
        callbacksExecutionStats.spellDocumentTagClose = true;
    }

    /**
     * Checks spelling for the given @a text.
     *
     * @internal
     *
     * @param tag unique tag to notify the client on which object the spelling is being performed
     * @param text the text containing the words to spellcheck
     * @param misspelling_location a pointer to store the beginning of the misspelled @a text, @c -1 if the @a text is correct
     * @param misspelling_length a pointer to store the length of misspelled @a text, @c 0 if the @a text is correct
     */
    static void onSpellingCheck(uint64_t tag, const char* text, int32_t* misspellingLocation, int32_t* misspellingLength)
    {
        ASSERT_EQ(defaultDocumentTag, tag);
        ASSERT_STREQ(expectedMisspelledWord, text);

        ASSERT_TRUE(misspellingLocation);
        ASSERT_TRUE(misspellingLength);

        // The client is able to show the misselled text through its location (the beginning of misspelling)
        // and length (the end of misspelling).
        *misspellingLocation = 0;
        *misspellingLength = strlen(expectedMisspelledWord);

        callbacksExecutionStats.spellingCheck = true;
    }

    /**
     * Checks spelling for the given @a text and compares it with the knownWord.
     *
     * @internal
     *
     * @param text the text containing the words to spellcheck
     * @param misspelling_location a pointer to store the beginning of the misspelled @a text, @c -1 if the @a text is correct
     * @param misspelling_length a pointer to store the length of misspelled @a text, @c 0 if the @a text is correct
     */
    static void onSpellingForKnownWord(uint64_t, const char* text, int32_t* misspellingLocation, int32_t* misspellingLength)
    {
        ASSERT_STREQ(knownWord.utf8().data(), text);

        ASSERT_TRUE(misspellingLocation);
        ASSERT_TRUE(misspellingLength);

        *misspellingLocation = -1;
        *misspellingLength = 0;

        callbacksExecutionStats.spellingCheck = true;
    }

    /**
     * Gets a list of suggested spellings for a misspelled @a word.
     *
     * @internal
     *
     * @param tag unique tag to notify the client on which object the spelling is being performed
     * @param word the word to get guesses
     * @return a list of dynamically allocated strings (as char*) and
     *         caller is responsible for destroying them.
     */
    static Eina_List* onWordGuesses(uint64_t tag, const char* word)
    {
        EXPECT_EQ(defaultDocumentTag, tag);
        EXPECT_STREQ(expectedMisspelledWord, word);

        Eina_List* suggestionsForWord = 0;
        size_t numberOfSuggestions = WTF_ARRAY_LENGTH(clientSuggestionsForWord);
        for (size_t i = 0; i < numberOfSuggestions; ++i)
            suggestionsForWord = eina_list_append(suggestionsForWord, strdup(clientSuggestionsForWord[i]));

        callbacksExecutionStats.wordGuesses = true;
        return suggestionsForWord;
    }

    /**
     * Adds the @a word to the spell checker dictionary.
     *
     * @internal
     *
     * @param tag unique tag to notify the client on which object the spelling is being performed
     * @param word the word to add
     */
    static void onWordLearn(uint64_t tag, const char* word)
    {
        ASSERT_EQ(defaultDocumentTag, tag);
        ASSERT_STREQ(expectedMisspelledWord, word);
        knownWord = word;
        callbacksExecutionStats.wordLearn = true;
    }

    /**
     * Tells the spell checker to ignore a given @a word.
     *
     * @internal
     *
     * @param tag unique tag to notify the client on which object the spelling is being performed
     * @param word the word to ignore
     */
    static void onWordIgnore(uint64_t tag, const char* word)
    {
        ASSERT_EQ(defaultDocumentTag, tag);
        ASSERT_STREQ(expectedMisspelledWord, word);
        knownWord = word;
        callbacksExecutionStats.wordIgnore = true;
    }

    /**
     * Helper, get required item from context menu.
     *
     * @param contextMenu the context menu object
     * @param itemAction action of item to get
     * @param itemType type of item to get
     *
     * @return required item
     */
    static Ewk_Context_Menu_Item* findContextMenuItem(const Ewk_Context_Menu* contextMenu, Ewk_Context_Menu_Item_Action itemAction, Ewk_Context_Menu_Item_Type itemType)
    {
        const Eina_List* contextMenuItems = ewk_context_menu_items_get(contextMenu);

        void* itemData;
        const Eina_List* listIterator;
        EINA_LIST_FOREACH(contextMenuItems, listIterator, itemData) {
            Ewk_Context_Menu_Item* item = static_cast<Ewk_Context_Menu_Item*>(itemData);
            if (ewk_context_menu_item_action_get(item) == itemAction
                && ewk_context_menu_item_type_get(item) == itemType)
                return item;
        }

        ADD_FAILURE();
        return 0;
    }

    static Eina_Bool checkCorrectnessOfSpellingItems(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        const Eina_List* contextMenuItems = ewk_context_menu_items_get(contextMenu);

        bool noGuessesAvailable = false;
        bool isIgnoreSpellingAvailable = false;
        bool isLearnSpellingAvailable = false;

        const Eina_List* listIterator;
        void* itemData;
        EINA_LIST_FOREACH(contextMenuItems, listIterator, itemData) {
            Ewk_Context_Menu_Item* item = static_cast<Ewk_Context_Menu_Item*>(itemData);
            if (!strcmp(ewk_context_menu_item_title_get(item), noGuessesString))
                noGuessesAvailable = true;
            else if (!strcmp(ewk_context_menu_item_title_get(item), ignoreSpellingString))
                isIgnoreSpellingAvailable = true;
            else if (!strcmp(ewk_context_menu_item_title_get(item), learnSpellingString))
                isLearnSpellingAvailable = true;
        }

        EXPECT_FALSE(noGuessesAvailable);
        EXPECT_TRUE(isIgnoreSpellingAvailable);
        EXPECT_TRUE(isLearnSpellingAvailable);

        wasContextMenuShown = true;
        return true;
    }

    static Eina_Bool toogleCheckSpellingWhileTyping(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        Ewk_Context_Menu_Item* spellingAndGrammarItem = findContextMenuItem(contextMenu, EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_MENU, EWK_SUBMENU_TYPE);
        Ewk_Context_Menu* spellingAndGrammarSubmenu = ewk_context_menu_item_submenu_get(spellingAndGrammarItem);
        Ewk_Context_Menu_Item* checkSpellingWhileTypingItem = findContextMenuItem(spellingAndGrammarSubmenu, EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING_WHILE_TYPING, EWK_CHECKABLE_ACTION_TYPE);

        return ewk_context_menu_item_select(spellingAndGrammarSubmenu, checkSpellingWhileTypingItem);
    }

    static Eina_Bool checkClientSuggestionsForWord(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        const Eina_List* contextMenuItems = ewk_context_menu_items_get(contextMenu);

        size_t numberOfSuggestions = WTF_ARRAY_LENGTH(clientSuggestionsForWord);
        // contextMenuItems should contain suggestions and another options.
        if (numberOfSuggestions > eina_list_count(contextMenuItems)) {
            ADD_FAILURE();
            return true;
        }
        // Verify suggestions from the top of context menu list.
        for (size_t i = 0; i < numberOfSuggestions; ++i) {
            Ewk_Context_Menu_Item* item = static_cast<Ewk_Context_Menu_Item*>(eina_list_data_get(contextMenuItems));
            EXPECT_STREQ(clientSuggestionsForWord[i], ewk_context_menu_item_title_get(item));
            contextMenuItems = eina_list_next(contextMenuItems);
        }

        wasContextMenuShown = true;
        return true;
    }

    static Eina_Bool selectLearnSpelling(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        return ewk_context_menu_item_select(contextMenu, findContextMenuItem(contextMenu, EWK_CONTEXT_MENU_ITEM_TAG_LEARN_SPELLING, EWK_ACTION_TYPE));
    }

    static Eina_Bool selectIgnoreSpelling(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        return ewk_context_menu_item_select(contextMenu, findContextMenuItem(contextMenu, EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_SPELLING, EWK_ACTION_TYPE));
    }

    /**
     * Count number of elements in context menu.
     */
    static Eina_Bool countContextMenuItems(Ewk_View_Smart_Data*, Evas_Coord, Evas_Coord, Ewk_Context_Menu* contextMenu)
    {
        contextMenuItemsNumber = eina_list_count(ewk_context_menu_items_get(contextMenu));
        wasContextMenuShown = true;
        return true;
    }

protected:
    enum Line { FirstLine, SecondLine };
    enum Button { SelectAllWordsWithSpellcheckButton, SelectAllWordsWithoutSpellcheckButton, SelectSubWordWithSpellcheckButton };

    void clickButton(Button button)
    {
        switch (button) {
        case SelectAllWordsWithSpellcheckButton:
            mouseClick(60, 60);
            break;
        case SelectAllWordsWithoutSpellcheckButton:
            mouseClick(500, 60);
            break;
        case SelectSubWordWithSpellcheckButton :
            mouseClick(200, 60);
            break;
        }
    }

    void showContextMenu(Line line)
    {
        switch (line) {
        case FirstLine:
            mouseClick(10, 20, 3);
            break;
        case SecondLine:
            mouseClick(35, 35, 3);
            break;
        }
    }

    void selectFirstWord(Line line)
    {
        switch (line) {
        case FirstLine:
            mouseDoubleClick(10, 20);
            break;
        case SecondLine:
            mouseDoubleClick(35, 35);
            break;
        }
    }
};

/**
 * Test whether there are spelling suggestions when misspelled word is directly context clicked.
 */
TEST_F(EWK2TextCheckerTest, spelling_suggestion_for_context_click)
{
    wasContextMenuShown = false;

    // Checking number of context menu items when element has no spellcheck suggestions.
    ewkViewClass()->context_menu_show = countContextMenuItems;
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    showContextMenu(FirstLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));
    unsigned numberItemsWithoutSpellCheck = contextMenuItemsNumber;

    wasContextMenuShown = false;

    // Testing how many items are in context menu when spellcheck is enabled.
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    showContextMenu(SecondLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));

    EXPECT_LT(numberItemsWithoutSpellCheck, contextMenuItemsNumber);
}

/**
 * Test whether there are no spelling suggestions when multiple words are selected (that are not a single misspelling).
 */
TEST_F(EWK2TextCheckerTest, no_spelling_suggestion_for_multiword_selection)
{
    wasContextMenuShown = false;

    // Checking number of context menu items when element has no spellcheck suggestions.
    ewkViewClass()->context_menu_show = countContextMenuItems;
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    clickButton(SelectAllWordsWithoutSpellcheckButton);
    showContextMenu(FirstLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));
    unsigned numberItemsWithoutSpellCheck = contextMenuItemsNumber;

    wasContextMenuShown = false;

    // Testing how many items are in context menu when multiple words are selected.
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    clickButton(SelectAllWordsWithSpellcheckButton);
    showContextMenu(SecondLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));

    EXPECT_EQ(numberItemsWithoutSpellCheck, contextMenuItemsNumber);
}

/**
 * Test whether there are no spelling suggestions when part of misspelled word are selected.
 */
TEST_F(EWK2TextCheckerTest, no_spelling_suggestion_for_subword_selection)
{
    wasContextMenuShown = false;

    // Checking number of context menu items when element has no spellcheck suggestions.
    ewkViewClass()->context_menu_show = countContextMenuItems;
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    clickButton(SelectAllWordsWithoutSpellcheckButton);
    showContextMenu(FirstLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));
    unsigned numberItemsWithoutSpellCheck = contextMenuItemsNumber;

    wasContextMenuShown = false;

    // Testing how many items are in context menu when part of word is selected.
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    clickButton(SelectSubWordWithSpellcheckButton);
    showContextMenu(SecondLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));

    EXPECT_EQ(numberItemsWithoutSpellCheck, contextMenuItemsNumber);
}

/**
 * Test whether context menu spelling items are available when misspelled word has selection as the double click.
 */
TEST_F(EWK2TextCheckerTest, spelling_suggestion_for_double_clicked_word)
{
    wasContextMenuShown = false;

    // Checking number of context menu items when element has no spell check suggestions.
    ewkViewClass()->context_menu_show = countContextMenuItems;
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    clickButton(SelectAllWordsWithoutSpellcheckButton);
    showContextMenu(FirstLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));
    unsigned numberItemsWithoutSpellCheck = contextMenuItemsNumber;

    wasContextMenuShown = false;

    // Making double click on misspelled word to select it, and checking are there context menu spell check suggestions.
    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_selection_tests.html").data()));
    selectFirstWord(SecondLine);
    showContextMenu(SecondLine);

    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));

    EXPECT_LT(numberItemsWithoutSpellCheck, contextMenuItemsNumber);
}

/**
 * Test whether the default language is loaded independently of
 * continuous spell checking setting.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_spell_checking_languages_get)
{
    ewk_text_checker_continuous_spell_checking_enabled_set(false);
    // The language is being loaded on the idler, wait for it.
    timeoutTimer = ecore_timer_add(defaultTimeoutInSeconds, onTimeout, 0);
    ecore_main_loop_begin();

    Eina_List* loadedLanguages = ewk_text_checker_spell_checking_languages_get();
    // No dictionary is available/installed.
    if (!loadedLanguages)
        return;

    EXPECT_EQ(1, eina_list_count(loadedLanguages));

    void* data;
    EINA_LIST_FREE(loadedLanguages, data)
        eina_stringshare_del(static_cast<Eina_Stringshare*>(data));

    // Repeat the checking when continuous spell checking setting is on.
    ewk_text_checker_continuous_spell_checking_enabled_set(true);
    timeoutTimer = ecore_timer_add(defaultTimeoutInSeconds, onTimeout, 0);
    ecore_main_loop_begin();

    loadedLanguages = ewk_text_checker_spell_checking_languages_get();
    if (!loadedLanguages)
        return;

    EXPECT_EQ(1, eina_list_count(loadedLanguages));

    EINA_LIST_FREE(loadedLanguages, data)
        eina_stringshare_del(static_cast<Eina_Stringshare*>(data));
}

/**
 * Test whether the context menu spelling items (suggestions, learn and ignore spelling)
 * are available when continuous spell checking is off.
 */
TEST_F(EWK2TextCheckerTest, context_menu_spelling_items_availability)
{
    ewk_text_checker_continuous_spell_checking_enabled_set(false);
    ewkViewClass()->context_menu_show = checkCorrectnessOfSpellingItems;

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));
    showContextMenu(FirstLine);

    while (!wasContextMenuShown)
        ecore_main_loop_iterate();
}

/**
 * Test setter/getter for the continuous spell checking:
 *  - ewk_text_checker_continuous_spell_checking_enabled_get
 *  - ewk_text_checker_continuous_spell_checking_enabled_set
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_continuous_spell_checking_enabled)
{
    ewk_text_checker_continuous_spell_checking_enabled_set(true);
#if ENABLE(SPELLCHECK)
    EXPECT_TRUE(ewk_text_checker_continuous_spell_checking_enabled_get());
#else
    EXPECT_FALSE(ewk_text_checker_continuous_spell_checking_enabled_get());
#endif

    ewk_text_checker_continuous_spell_checking_enabled_set(false);
    EXPECT_FALSE(ewk_text_checker_continuous_spell_checking_enabled_get());
}

/**
 * Test whether the onSettingChange callback is called when "Check Spelling While Typing" setting was changed in context menu.
 * Two cases are tested:
 *  - "Check Spelling While Typing" is enabled,
 *  - "Check Spelling While Typing" is disabled.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_check_spelling_while_typing_toggle)
{
    resetCallbacksExecutionStats();
    ewkViewClass()->context_menu_show = toogleCheckSpellingWhileTyping;

    ewk_text_checker_continuous_spell_checking_change_cb_set(onSettingChange);
    isSettingEnabled = !ewk_text_checker_continuous_spell_checking_enabled_get();

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    showContextMenu(FirstLine);
    ASSERT_TRUE(waitUntilTrue(callbacksExecutionStats.settingChange));

    resetCallbacksExecutionStats();

    // Test case, when "Check Spelling While Typing" is in reverse to the previous one.
    isSettingEnabled = !isSettingEnabled;

    showContextMenu(FirstLine);
    ASSERT_TRUE(waitUntilTrue(callbacksExecutionStats.settingChange));

    ewk_text_checker_continuous_spell_checking_change_cb_set(0);
}

/**
 * Test whether the onSettingChange callback is not called when the spell checking setting was changed by client.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_continuous_spell_checking_change_cb_set)
{
    resetCallbacksExecutionStats();

    ewk_text_checker_continuous_spell_checking_change_cb_set(onSettingChange);

    isSettingEnabled = ewk_text_checker_continuous_spell_checking_enabled_get();
    isSettingEnabled = !isSettingEnabled;
    // The notifications about the setting change shouldn't be sent if the change was made
    // on the client's request (public API).
    ewk_text_checker_continuous_spell_checking_enabled_set(isSettingEnabled);

    // The notification about the change of the spell checking setting is called on idler.
    ASSERT_FALSE(waitUntilTrue(callbacksExecutionStats.settingChange, /*Timeout*/ 0));

    ewk_text_checker_continuous_spell_checking_change_cb_set(0);
}

/**
 * Test whether the onSettingChange callback is not called, if the client does not set it.
 * "Check Spelling While Typing" option is toggled in context menu.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_continuous_spell_checking_change_cb_unset)
{
    resetCallbacksExecutionStats();
    ewkViewClass()->context_menu_show = toogleCheckSpellingWhileTyping;

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    showContextMenu(FirstLine);
    ASSERT_FALSE(waitUntilTrue(callbacksExecutionStats.settingChange, /*Timeout*/ 0));
}

/**
 * This unit test sets all available/installed dictionaries and verifies them
 * if they are in use.
 * All the dictionaries from the list can be set to perform spellchecking.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_spell_checking_available_languages_get)
{
    Eina_List* availableLanguages = ewk_text_checker_spell_checking_available_languages_get();
    // No dictionary is available/installed or the SPELLCHECK macro is disabled.
    if (!availableLanguages)
        return;

    // Helper to create one string with comma separated languages.
    void* actual = 0;
    WTF::StringBuilder languages;
    Eina_List* listIterator = 0;
    unsigned lastIndex = eina_list_count(availableLanguages) - 1;
    unsigned i = 0;
    EINA_LIST_FOREACH(availableLanguages, listIterator, actual) {
        languages.append(static_cast<const char*>(actual));
        // Add the comma after all but the last language IDs.
        if (i++ != lastIndex)
            languages.append(',');
    }

    // Set all available languages.
    ewk_text_checker_spell_checking_languages_set(languages.toString().utf8().data());

    // Languages are being loaded on the idler, wait for them.
    timeoutTimer = ecore_timer_add(defaultTimeoutInSeconds, onTimeout, 0);
    ecore_main_loop_begin();

    // Get the languages in use.
    Eina_List* loadedLanguages = ewk_text_checker_spell_checking_languages_get();
    ASSERT_EQ(eina_list_count(loadedLanguages), eina_list_count(availableLanguages));

    i = 0;
    void* expected = 0;
    // Verify whether the loaded languages list is equal to the available languages list.
    EINA_LIST_FOREACH(loadedLanguages, listIterator, actual) {
        expected = eina_list_nth(availableLanguages, i++);
        EXPECT_STREQ(static_cast<const char*>(expected), static_cast<const char*>(actual));
    }

    // Delete the lists.
    EINA_LIST_FREE(availableLanguages, actual)
        eina_stringshare_del(static_cast<const char*>(actual));

    EINA_LIST_FREE(loadedLanguages, actual)
        eina_stringshare_del(static_cast<const char*>(actual));
}

/**
 * Here we test the following scenarios:
 *  - setting the default language,
 *  - if two arbitrarily selected dictionaries are set correctly.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_spell_checking_languages)
{
    // Set the default language.
    ewk_text_checker_spell_checking_languages_set(0);

    // Languages are being loaded on the idler, wait for them.
    timeoutTimer = ecore_timer_add(defaultTimeoutInSeconds, onTimeout, 0);
    ecore_main_loop_begin();

    Eina_List* loadedLanguages = ewk_text_checker_spell_checking_languages_get();
    // No dictionary is available/installed or the SPELLCHECK macro is disabled.
    if (!loadedLanguages)
        return;

    ASSERT_EQ(1, eina_list_count(loadedLanguages));

    // Delete the list.
    void* actual = 0;
    EINA_LIST_FREE(loadedLanguages, actual)
        eina_stringshare_del(static_cast<const char*>(actual));

    // Get the first and last language from installed dictionaries.
    Eina_List* availableLanguages = ewk_text_checker_spell_checking_available_languages_get();
    unsigned numberOfAvailableLanguages = eina_list_count(availableLanguages);
    // We assume that user has installed at lest two dictionaries.
    if (numberOfAvailableLanguages < 2)
        return;

    const char* firstExpected = static_cast<const char*>(eina_list_nth(availableLanguages, 0));
    const char* lastExpected = static_cast<const char*>(eina_list_data_get(eina_list_last(availableLanguages)));

    // Case sensitivity of dictionaries doesn't affect on loading the dictionaries,
    // the Enchant library will 'normalize' them.
    WTF::StringBuilder languages;
    languages.append(String(firstExpected).upper());
    languages.append(',');
    languages.append(String(lastExpected).lower());

    // Set both languages (the first and the last) from the list.
    ewk_text_checker_spell_checking_languages_set(languages.toString().utf8().data());

    timeoutTimer = ecore_timer_add(defaultTimeoutInSeconds, onTimeout, 0);
    ecore_main_loop_begin();

    loadedLanguages = ewk_text_checker_spell_checking_languages_get();
    ASSERT_EQ(2, eina_list_count(loadedLanguages));

    EXPECT_STREQ(firstExpected, static_cast<const char*>(eina_list_nth(loadedLanguages, 0)));
    EXPECT_STREQ(lastExpected, static_cast<const char*>(eina_list_nth(loadedLanguages, 1)));

    // Delete the lists.
    EINA_LIST_FREE(availableLanguages, actual)
        eina_stringshare_del(static_cast<const char*>(actual));

    EINA_LIST_FREE(loadedLanguages, actual)
        eina_stringshare_del(static_cast<const char*>(actual));
}

/**
 * Test whether the client's callbacks aren't called (if not specified).
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker)
{
    resetCallbacksExecutionStats();
    ewk_text_checker_continuous_spell_checking_enabled_set(true);

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    // If user doesn't specify callback functions responsible for spelling
    // the default WebKit implementation (based on the Enchant library) will be used.
    ASSERT_FALSE(callbacksExecutionStats.spellDocumentTag);
    ASSERT_FALSE(callbacksExecutionStats.spellDocumentTagClose);
    ASSERT_FALSE(callbacksExecutionStats.spellingCheck);

    // It doesn't make sense to verify others callbacks (onWordGuesses,
    // onWordLearn, onWordIgnore) as they need the context menu feature
    // which is not implemented for WK2-EFL.
}

/**
 * Test whether the client's callbacks (onSpellDocumentTag, onSpellDocumentTagClose) are called.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_unique_spell_document_tag)
{
    resetCallbacksExecutionStats();
    defaultView = webView();
    ewk_text_checker_continuous_spell_checking_enabled_set(true);

    ewk_text_checker_unique_spell_document_tag_get_cb_set(onSpellDocumentTag);
    ewk_text_checker_unique_spell_document_tag_close_cb_set(onSpellDocumentTagClose);

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    // Check whether the callback was called.
    ASSERT_TRUE(callbacksExecutionStats.spellDocumentTag);
    // It's not possible to check whether onSpellDocumentTagClose was called here, because
    // it's invoked when WebPage is being destroyed.
    // It should be verified for example when view is freed.
}

/**
 * Test whether the client's callback (onSpellingCheck) is called when
 * the word to input field was put.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_string_spelling_check_cb_set)
{
    resetCallbacksExecutionStats();
    defaultView = webView();
    ewk_text_checker_continuous_spell_checking_enabled_set(true);

    ewk_text_checker_string_spelling_check_cb_set(onSpellingCheck);

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    // Check whether the callback was called.
    ASSERT_TRUE(callbacksExecutionStats.spellingCheck);
}

/**
 * Test whether the client's callback (onWordGuesses) is called when
 * the context menu was shown on the misspelled word.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_word_guesses_get_cb_set)
{
    resetCallbacksExecutionStats();
    wasContextMenuShown = false;
    defaultView = webView();
    ewkViewClass()->context_menu_show = checkClientSuggestionsForWord;
    ewk_text_checker_word_guesses_get_cb_set(onWordGuesses);

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    showContextMenu(FirstLine);
    ASSERT_TRUE(waitUntilTrue(wasContextMenuShown));

    // Check whether the callback is called.
    ASSERT_TRUE(callbacksExecutionStats.wordGuesses);

    ewk_text_checker_word_guesses_get_cb_set(0);
}

/**
 * Test whether the client's callback (onWordLearn) is called when
 * the context menu option "Learn spelling" was chosen. In the next step,
 * check whether the learned word is treated as spelled correctly while spell checking.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_word_learn_cb_set)
{
    resetCallbacksExecutionStats();
    knownWord = emptyString();
    defaultView = webView();
    ewk_text_checker_word_learn_cb_set(onWordLearn);
    ewkViewClass()->context_menu_show = selectLearnSpelling;

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));
    selectFirstWord(FirstLine);
    showContextMenu(FirstLine);

    ASSERT_TRUE(waitUntilTrue(callbacksExecutionStats.wordLearn));

    // Open html again and check whether the learned word
    // is treated as spelled correctly while spell checking.
    resetCallbacksExecutionStats();
    ewk_text_checker_string_spelling_check_cb_set(onSpellingForKnownWord);

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    ASSERT_TRUE(callbacksExecutionStats.spellingCheck);

    ewk_text_checker_string_spelling_check_cb_set(0);
    ewk_text_checker_word_learn_cb_set(0);
}

/**
 * Test whether the client's callback (onWordIgnore) is called when
 * the context menu option "Ignore spelling" was chosen. In the next step,
 * check whether the ignored word is treated as spelled correctly while spell checking.
 */
TEST_F(EWK2TextCheckerTest, ewk_text_checker_word_ignore_cb_set)
{
    resetCallbacksExecutionStats();
    knownWord = emptyString();
    defaultView = webView();
    ewk_text_checker_word_ignore_cb_set(onWordIgnore);
    ewkViewClass()->context_menu_show = selectIgnoreSpelling;

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));
    selectFirstWord(FirstLine);
    showContextMenu(FirstLine);

    ASSERT_TRUE(waitUntilTrue(callbacksExecutionStats.wordIgnore));

    // Open html again and check whether the ignored word
    // is treated as spelled correctly while spell checking.
    resetCallbacksExecutionStats();
    ewk_text_checker_string_spelling_check_cb_set(onSpellingForKnownWord);

    ASSERT_TRUE(loadUrlSync(environment->urlForResource("spelling_test.html").data()));

    ASSERT_TRUE(callbacksExecutionStats.spellingCheck);

    ewk_text_checker_string_spelling_check_cb_set(0);
    ewk_text_checker_word_ignore_cb_set(0);
}

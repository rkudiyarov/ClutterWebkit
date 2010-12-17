/*
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2009 Diego Escalante Urrelo <diegoe@gnome.org>
 *  Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *  Copyright (C) 2009, 2010 Igalia S.L.
 *  Copyright (C) 2010, Martin Robinson <mrobinson@webkit.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "EditorClientClutter.h"

#include "EditCommand.h"
#include "Editor.h"
#include <enchant.h>
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include <glib.h>
#include "KeyboardEvent.h"
#include "markup.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PasteboardHelperClutter.h"
#include "PlatformKeyboardEvent.h"
#include "WindowsKeyboardCodes.h"
#include "webkitmarshal.h"
#include "webkitprivate.h"
#include <wtf/text/CString.h>

// Arbitrary depth limit for the undo stack, to keep it from using
// unbounded memory.  This is the maximum number of distinct undoable
// actions -- unbroken stretches of typed characters are coalesced
// into a single action.
#define maximumUndoStackDepth 1000

using namespace WebCore;

namespace WebKit {

void EditorClient::updatePendingComposition(const gchar* newComposition)
{
    // The IMContext may signal more than one completed composition in a row,
    // in which case we want to append them, rather than overwrite the old one.
    if (!m_pendingComposition)
        m_pendingComposition.set(g_strdup(newComposition));
    else
        m_pendingComposition.set(g_strconcat(m_pendingComposition.get(), newComposition, NULL));
}

void EditorClient::willSetInputMethodState()
{
}

void EditorClient::setInputMethodState(bool active)
{
    notImplemented();
}

bool EditorClient::shouldDeleteRange(Range*)
{
    notImplemented();
    return true;
}

bool EditorClient::shouldShowDeleteInterface(HTMLElement*)
{
    return false;
}

bool EditorClient::isContinuousSpellCheckingEnabled()
{
    WebKitWebSettings* settings = webkit_web_view_get_settings(m_webView);

    gboolean enabled;
    g_object_get(settings, "enable-spell-checking", &enabled, NULL);

    return enabled;
}

bool EditorClient::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

int EditorClient::spellCheckerDocumentTag()
{
    notImplemented();
    return 0;
}

bool EditorClient::shouldBeginEditing(WebCore::Range*)
{
    clearPendingComposition();

    notImplemented();
    return true;
}

bool EditorClient::shouldEndEditing(WebCore::Range*)
{
    clearPendingComposition();

    notImplemented();
    return true;
}

bool EditorClient::shouldInsertText(const String&, Range*, EditorInsertAction)
{
    notImplemented();
    return true;
}

bool EditorClient::shouldChangeSelectedRange(Range*, Range*, EAffinity, bool)
{
    notImplemented();
    return true;
}

bool EditorClient::shouldApplyStyle(WebCore::CSSStyleDeclaration*, WebCore::Range*)
{
    notImplemented();
    return true;
}

bool EditorClient::shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*)
{
    notImplemented();
    return true;
}

void EditorClient::didBeginEditing()
{
    notImplemented();
}

void EditorClient::respondToChangedContents()
{
    notImplemented();
}

void EditorClient::respondToChangedSelection()
{
    notImplemented();
}

void EditorClient::didEndEditing()
{
    notImplemented();
}

void EditorClient::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void EditorClient::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

bool EditorClient::isEditable()
{
    return webkit_web_view_get_editable(m_webView);
}

void EditorClient::registerCommandForUndo(WTF::PassRefPtr<WebCore::EditCommand> command)
{
    if (undoStack.size() == maximumUndoStackDepth)
        undoStack.removeFirst();
    if (!m_isInRedo)
        redoStack.clear();
    undoStack.append(command);
}

void EditorClient::registerCommandForRedo(WTF::PassRefPtr<WebCore::EditCommand> command)
{
    redoStack.append(command);
}

void EditorClient::clearUndoRedoOperations()
{
    undoStack.clear();
    redoStack.clear();
}

bool EditorClient::canUndo() const
{
    return !undoStack.isEmpty();
}

bool EditorClient::canRedo() const
{
    return !redoStack.isEmpty();
}

void EditorClient::undo()
{
    if (canUndo()) {
        RefPtr<WebCore::EditCommand> command(*(--undoStack.end()));
        undoStack.remove(--undoStack.end());
        // unapply will call us back to push this command onto the redo stack.
        command->unapply();
    }
}

void EditorClient::redo()
{
    if (canRedo()) {
        RefPtr<WebCore::EditCommand> command(*(--redoStack.end()));
        redoStack.remove(--redoStack.end());

        ASSERT(!m_isInRedo);
        m_isInRedo = true;
        // reapply will call us back to push this command onto the undo stack.
        command->reapply();
        m_isInRedo = false;
    }
}

bool EditorClient::shouldInsertNode(Node*, Range*, EditorInsertAction)
{
    notImplemented();
    return true;
}

void EditorClient::pageDestroyed()
{
    delete this;
}

bool EditorClient::smartInsertDeleteEnabled()
{
    notImplemented();
    return false;
}

bool EditorClient::isSelectTrailingWhitespaceEnabled()
{
    notImplemented();
    return false;
}

void EditorClient::toggleContinuousSpellChecking()
{
    WebKitWebSettings* settings = webkit_web_view_get_settings(m_webView);

    gboolean enabled;
    g_object_get(settings, "enable-spell-checking", &enabled, NULL);

    g_object_set(settings, "enable-spell-checking", !enabled, NULL);
}

void EditorClient::toggleGrammarChecking()
{
}

void EditorClient::generateEditorCommands(const KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    notImplemented();
}

bool EditorClient::executePendingEditorCommands(Frame* frame, bool allowTextInsertion)
{
    Vector<Editor::Command> commands;
    for (size_t i = 0; i < m_pendingEditorCommands.size(); i++) {
        Editor::Command command = frame->editor()->command(m_pendingEditorCommands.at(i));
        if (command.isTextInsertion() && !allowTextInsertion)
            return false;

        commands.append(command);
    }

    bool success = true;
    for (size_t i = 0; i < commands.size(); i++) {
        if (!commands.at(i).execute()) {
            success = false;
            break;
        }
    }

    m_pendingEditorCommands.clear();

    // If we successfully completed all editor commands, then
    // this signals a canceling of the composition.
    if (success)
        clearPendingComposition();

    return success;
}

void EditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return;

    generateEditorCommands(event);
    if (m_pendingEditorCommands.size() > 0) {

        // During RawKeyDown events if an editor command will insert text, defer
        // the insertion until the keypress event. We want keydown to bubble up
        // through the DOM first.
        if (platformEvent->type() == PlatformKeyboardEvent::RawKeyDown) {
            if (executePendingEditorCommands(frame, false))
                event->setDefaultHandled();

            return;
        }

        // Only allow text insertion commands if the current node is editable.
        if (executePendingEditorCommands(frame, frame->editor()->canEdit())) {
            event->setDefaultHandled();
            return;
        }
    }

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor()->canEdit())
        return;

    // This is just a normal text insertion, so wait to execute the insertion
    // until a keypress event happens. This will ensure that the insertion will not
    // be reflected in the contents of the field until the keyup DOM event.
    if (event->type() == eventNames().keypressEvent) {

        // If we have a pending composition at this point, it happened while
        // filtering a keypress, so we treat it as a normal text insertion.
        // This will also ensure that if the keypress event handler changed the
        // currently focused node, the text is still inserted into the original
        // node (insertText() has this logic, but confirmComposition() does not).
        if (m_pendingComposition) {
            frame->editor()->insertText(String::fromUTF8(m_pendingComposition.get()), event);
            clearPendingComposition();
            event->setDefaultHandled();

        } else {
            // Don't insert null or control characters as they can result in unexpected behaviour
            if (event->charCode() < ' ')
                return;

            // Don't insert anything if a modifier is pressed
            if (platformEvent->ctrlKey() || platformEvent->altKey())
                return;

            if (frame->editor()->insertText(platformEvent->text(), event))
                event->setDefaultHandled();
        }
    }
}

void EditorClient::handleInputMethodKeydown(KeyboardEvent* event)
{
    notImplemented();
}

void EditorClient::handleInputMethodMousePress()
{
    notImplemented();
}

EditorClient::EditorClient(WebKitWebView* webView)
    : m_isInRedo(false)
    , m_webView(webView)
    , m_preventNextCompositionCommit(false)
    , m_treatContextCommitAsKeyEvent(false)
//    , m_nativeWidget(gtk_text_view_new())
{
    notImplemented();
}

EditorClient::~EditorClient()
{
    notImplemented();
}

void EditorClient::textFieldDidBeginEditing(Element*)
{
}

void EditorClient::textFieldDidEndEditing(Element*)
{
}

void EditorClient::textDidChangeInTextField(Element*)
{
}

bool EditorClient::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    return false;
}

void EditorClient::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void EditorClient::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

void EditorClient::ignoreWordInSpellDocument(const String& text)
{
    GSList* dicts = webkit_web_settings_get_enchant_dicts(m_webView);

    for (; dicts; dicts = dicts->next) {
        EnchantDict* dict = static_cast<EnchantDict*>(dicts->data);

        enchant_dict_add_to_session(dict, text.utf8().data(), -1);
    }
}

void EditorClient::learnWord(const String& text)
{
    GSList* dicts = webkit_web_settings_get_enchant_dicts(m_webView);

    for (; dicts; dicts = dicts->next) {
        EnchantDict* dict = static_cast<EnchantDict*>(dicts->data);

        enchant_dict_add_to_personal(dict, text.utf8().data(), -1);
    }
}

void EditorClient::checkSpellingOfString(const UChar* text, int length, int* misspellingLocation, int* misspellingLength)
{
    GSList* dicts = webkit_web_settings_get_enchant_dicts(m_webView);
    if (!dicts)
        return;

    UChar *ttext = const_cast<UChar*>(text);
    gchar* ctext = g_utf16_to_utf8((gunichar2*)ttext, length, 0, 0, 0);
    int utflen = g_utf8_strlen(ctext, -1);

    PangoLanguage* language = pango_language_get_default();
    PangoLogAttr* attrs = g_new(PangoLogAttr, utflen+1);

    // pango_get_log_attrs uses an aditional position at the end of the text.
    pango_get_log_attrs(ctext, -1, -1, language, attrs, utflen+1);

    for (int i = 0; i < length+1; i++) {
        // We go through each character until we find an is_word_start,
        // then we get into an inner loop to find the is_word_end corresponding
        // to it.
        if (attrs[i].is_word_start) {
            int start = i;
            int end = i;
            int wordLength;

            while (attrs[end].is_word_end < 1)
                end++;

            wordLength = end - start;
            // Set the iterator to be at the current word end, so we don't
            // check characters twice.
            i = end;

            for (; dicts; dicts = dicts->next) {
                EnchantDict* dict = static_cast<EnchantDict*>(dicts->data);
                gchar* cstart = g_utf8_offset_to_pointer(ctext, start);
                gint bytes = static_cast<gint>(g_utf8_offset_to_pointer(ctext, end) - cstart);
                gchar* word = g_new0(gchar, bytes+1);
                int result;

                g_utf8_strncpy(word, cstart, end - start);

                result = enchant_dict_check(dict, word, -1);
                g_free(word);
                if (result) {
                    *misspellingLocation = start;
                    *misspellingLength = wordLength;
                } else {
                    // Stop checking, this word is ok in at least one dict.
                    *misspellingLocation = -1;
                    *misspellingLength = 0;
                    break;
                }
            }
        }
    }

    g_free(attrs);
    g_free(ctext);
}

String EditorClient::getAutoCorrectSuggestionForMisspelledWord(const String& inputWord)
{
    // This method can be implemented using customized algorithms for the particular browser.
    // Currently, it computes an empty string.
    return String();
}

void EditorClient::checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*)
{
    notImplemented();
}

void EditorClient::updateSpellingUIWithGrammarString(const String&, const GrammarDetail&)
{
    notImplemented();
}

void EditorClient::updateSpellingUIWithMisspelledWord(const String&)
{
    notImplemented();
}

void EditorClient::showSpellingUI(bool)
{
    notImplemented();
}

bool EditorClient::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void EditorClient::getGuessesForWord(const String& word, WTF::Vector<String>& guesses)
{
    GSList* dicts = webkit_web_settings_get_enchant_dicts(m_webView);
    guesses.clear();

    for (; dicts; dicts = dicts->next) {
        size_t numberOfSuggestions;
        size_t i;

        EnchantDict* dict = static_cast<EnchantDict*>(dicts->data);
        gchar** suggestions = enchant_dict_suggest(dict, word.utf8().data(), -1, &numberOfSuggestions);

        for (i = 0; i < numberOfSuggestions && i < 10; i++)
            guesses.append(String::fromUTF8(suggestions[i]));

        if (numberOfSuggestions > 0)
            enchant_dict_free_suggestions(dict, suggestions);
    }
}

}

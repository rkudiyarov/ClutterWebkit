/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2009, 2010 Kakai, Inc. <brian@kakai.com>
 * Copyright (C) 2010 Igalia S.L.
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

#include "config.h"
#include "PluginView.h"

#include "Bridge.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Element.h"
#include "FocusController.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "FrameTree.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HostWindow.h"
#include "Image.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PluginDebug.h"
#include "PluginMainThreadScheduler.h"
#include "PluginPackage.h"
#include "RenderLayer.h"
#include "Settings.h"
#include "JSDOMBinding.h"
#include "ScriptController.h"
#include "npruntime_impl.h"
#include "runtime_root.h"
#include <runtime/JSLock.h>
#include <runtime/JSValue.h>

using JSC::ExecState;
using JSC::Interpreter;
using JSC::JSLock;
using JSC::JSObject;
using JSC::UString;

using std::min;

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

bool PluginView::dispatchNPEvent(NPEvent& event)
{
    notImplemented();
    return false;
}

void PluginView::updatePluginWidget()
{
    notImplemented();
}

void PluginView::setFocus(bool focused)
{
    ASSERT(platformPluginWidget() == platformWidget());
    Widget::setFocus(focused);
}

void PluginView::show()
{
    ASSERT(platformPluginWidget() == platformWidget());
    Widget::show();
}

void PluginView::hide()
{
    ASSERT(platformPluginWidget() == platformWidget());
    Widget::hide();
}

void PluginView::paint(GraphicsContext* context, const IntRect& rect)
{
    notImplemented();
}

void PluginView::handleKeyboardEvent(KeyboardEvent* event)
{
    notImplemented();
}

void PluginView::handleMouseEvent(MouseEvent* event)
{
    notImplemented();
}

#if defined(XP_UNIX)
void PluginView::handleFocusInEvent()
{
    notImplemented();
}

void PluginView::handleFocusOutEvent()
{
    notImplemented();
}
#endif

void PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

    if (parent)
        init();
}

void PluginView::setNPWindowRect(const IntRect&)
{
    notImplemented();
}

void PluginView::setNPWindowIfNeeded()
{
    notImplemented();
}

void PluginView::setParentVisible(bool visible)
{
    if (isParentVisible() == visible)
        return;

    Widget::setParentVisible(visible);

    notImplemented();
}

NPError PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf)
{
    WTF::String filename(buf, len);

    if (filename.startsWith("file:///"))
        filename = filename.substring(8);

    // Get file info
    if (!g_file_test ((filename.utf8()).data(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)))
        return NPERR_FILE_NOT_FOUND;

    //FIXME - read the file data into buffer
    FILE* fileHandle = fopen((filename.utf8()).data(), "r");
    
    if (fileHandle == 0)
        return NPERR_FILE_NOT_FOUND;

    //buffer.resize();

    int bytesRead = fread(buffer.data(), 1, 0, fileHandle);

    fclose(fileHandle);

    if (bytesRead <= 0)
        return NPERR_FILE_NOT_FOUND;

    return NPERR_NO_ERROR;
}

bool PluginView::platformGetValueStatic(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVToolkit:
#if defined(XP_UNIX)
        *static_cast<uint32_t*>(value) = 2;
#else
        *static_cast<uint32_t*>(value) = 0;
#endif
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsXEmbedBool:
#if defined(XP_UNIX)
        *static_cast<NPBool*>(value) = true;
#else
        *static_cast<NPBool*>(value) = false;
#endif
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVjavascriptEnabledBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsWindowless:
#if defined(XP_UNIX)
        *static_cast<NPBool*>(value) = true;
#else
        *static_cast<NPBool*>(value) = false;
#endif
        *result = NPERR_NO_ERROR;
        return true;

    default:
        return false;
    }
}

bool PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVxDisplay:
        *result = NPERR_GENERIC_ERROR;
        return true;

#if defined(XP_UNIX)
    case NPNVxtAppContext:
            *result = NPERR_GENERIC_ERROR;
        return true;
#endif

        case NPNVnetscapeWindow: {
            *result = NPERR_GENERIC_ERROR;
            return true;
        }

    default:
        return false;
    }
}

void PluginView::invalidateRect(const IntRect& rect)
{
    notImplemented();
}

void PluginView::invalidateRect(NPRect* rect)
{
    notImplemented();
}

void PluginView::invalidateRegion(NPRegion)
{
    notImplemented();
}

void PluginView::forceRedraw()
{
    notImplemented();
}

bool PluginView::platformStart()
{
    notImplemented();
    m_status = PluginStatusCanNotLoadPlugin;
    return false;
}

void PluginView::platformDestroy()
{
    notImplemented();
}

void PluginView::halt()
{
}

void PluginView::restart()
{
}

} // namespace WebCore


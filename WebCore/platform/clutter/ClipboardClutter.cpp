/*
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
#include "ClipboardClutter.h"

#include "CachedImage.h"
#include "DragData.h"
#include "Editor.h"
#include "Element.h"
#include "FileList.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "Image.h"
#include "NotImplemented.h"
#include "Pasteboard.h"
#include "PasteboardHelperWebCoreClutter.h"
#include "RenderImage.h"
#include "ScriptExecutionContext.h"
#include "markup.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

PassRefPtr<Clipboard> Editor::newGeneralClipboard(ClipboardAccessPolicy policy, Frame* frame)
{
    return ClipboardClutter::create(policy, Clipboard::CopyAndPaste);
}

PassRefPtr<Clipboard> Clipboard::create(ClipboardAccessPolicy policy, DragData* dragData, Frame* frame)
{
    return ClipboardClutter::create(policy, DragAndDrop);
}

ClipboardClutter::ClipboardClutter(ClipboardAccessPolicy policy, ClipboardType clipboardType)
    : Clipboard(policy, clipboardType)
{
    notImplemented();
}

ClipboardClutter::~ClipboardClutter()
{
    notImplemented();
}

void ClipboardClutter::clearData(const String& typeString)
{
    notImplemented();
}


void ClipboardClutter::clearAllData()
{
    notImplemented();
}

String ClipboardClutter::getData(const String& typeString, bool& success) const
{
    notImplemented();
    success = false;

    return String();
}

bool ClipboardClutter::setData(const String& typeString, const String& data)
{
    notImplemented();

    return false;
}

HashSet<String> ClipboardClutter::types() const
{
    notImplemented();

    return HashSet<String>();
}

PassRefPtr<FileList> ClipboardClutter::files() const
{
    notImplemented();
    return FileList::create();
}

void ClipboardClutter::setDragImage(CachedImage* image, const IntPoint& location)
{
    notImplemented();
}

void ClipboardClutter::setDragImageElement(Node* element, const IntPoint& location)
{
    notImplemented();
}

void ClipboardClutter::setDragImage(CachedImage* image, Node* element, const IntPoint& location)
{
    notImplemented();
}

DragImageRef ClipboardClutter::createDragImage(IntPoint& location) const
{
    notImplemented();
    return 0;
}

void ClipboardClutter::declareAndWriteDragImage(Element* element, const KURL& url, const String& label, Frame* frame)
{
    notImplemented();
}

void ClipboardClutter::writeURL(const KURL& url, const String& label, Frame*)
{
    notImplemented();
}

void ClipboardClutter::writeRange(Range* range, Frame* frame)
{
    notImplemented();
}

void ClipboardClutter::writePlainText(const String& text)
{
    notImplemented();
}

bool ClipboardClutter::hasData()
{
    notImplemented();
    return false;
}

}

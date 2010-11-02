/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (C) 2010 Igalia S.L
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
#include "ContextMenu.h"
#include "ContextMenuItem.h"
#include "NotImplemented.h"
#include <wtf/text/CString.h>

#include <clutter/clutter.h>

#define WEBKIT_CONTEXT_MENU_ACTION "webkit-context-menu"
#define STOCK_ICON_NAME "FIXME"

namespace WebCore {

static const char* clutterStockIDFromContextMenuAction(const ContextMenuAction& action)
{
    switch (action) {
    case ContextMenuItemTagCopyLinkToClipboard:
    case ContextMenuItemTagCopyImageToClipboard:
    case ContextMenuItemTagCopyMediaLinkToClipboard:
    case ContextMenuItemTagCopy:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagOpenLinkInNewWindow:
    case ContextMenuItemTagOpenImageInNewWindow:
    case ContextMenuItemTagOpenFrameInNewWindow:
    case ContextMenuItemTagOpenMediaInNewWindow:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagDownloadLinkToDisk:
    case ContextMenuItemTagDownloadImageToDisk:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagGoBack:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagGoForward:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagStop:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagReload:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagCut:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagPaste:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagDelete:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagSelectAll:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagSpellingGuess:
        return 0;
    case ContextMenuItemTagIgnoreSpelling:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagLearnSpelling:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagOther:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagSearchInSpotlight:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagSearchWeb:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagOpenWithDefaultApplication:
        return STOCK_ICON_NAME;
    case ContextMenuItemPDFZoomIn:
        return STOCK_ICON_NAME;
    case ContextMenuItemPDFZoomOut:
        return STOCK_ICON_NAME;
    case ContextMenuItemPDFAutoSize:
        return STOCK_ICON_NAME;
    case ContextMenuItemPDFNextPage:
        return STOCK_ICON_NAME;
    case ContextMenuItemPDFPreviousPage:
        return STOCK_ICON_NAME;
    // New tags, not part of API
    case ContextMenuItemTagOpenLink:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagCheckSpelling:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagFontMenu:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagShowFonts:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagBold:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagItalic:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagUnderline:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagShowColors:
        return STOCK_ICON_NAME;
    case ContextMenuItemTagToggleMediaControls:
    case ContextMenuItemTagToggleMediaLoop:
        // No icon for this.
        return 0;
    case ContextMenuItemTagEnterVideoFullscreen:
        return STOCK_ICON_NAME;
    default:
        return 0;
    }
}

ContextMenuItem::ContextMenuItem(ContextMenu*)
{
    notImplemented();
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType type, ContextMenuAction action, const String& title, ContextMenu* subMenu)
{
    m_platformDescription.type = type;
    m_platformDescription.action = action;
    m_platformDescription.title = title;

    //setSubMenu(subMenu);
}

ContextMenuItem::~ContextMenuItem()
{
}

ClutterActor* ContextMenuItem::createNativeMenuItem(const PlatformMenuItemDescription& menu)
{
    ClutterActor* item = 0;
    notImplemented();
    
    return item;
}

PlatformMenuItemDescription ContextMenuItem::releasePlatformDescription()
{
    PlatformMenuItemDescription description = m_platformDescription;
    m_platformDescription = PlatformMenuItemDescription();
    return description;
}

ContextMenuItemType ContextMenuItem::type() const
{
    return m_platformDescription.type;
}

void ContextMenuItem::setType(ContextMenuItemType type)
{
    m_platformDescription.type = type;
}

ContextMenuAction ContextMenuItem::action() const
{
    return m_platformDescription.action;
}

void ContextMenuItem::setAction(ContextMenuAction action)
{
    m_platformDescription.action = action;
}

String ContextMenuItem::title() const
{
    notImplemented();
    return String();
}

void ContextMenuItem::setTitle(const String& title)
{
    notImplemented();
}

PlatformMenuDescription ContextMenuItem::platformSubMenu() const
{
    notImplemented();
    return 0;
}

void ContextMenuItem::setSubMenu(ContextMenu* menu)
{
    notImplemented();
}

void ContextMenuItem::setChecked(bool shouldCheck)
{
    m_platformDescription.checked = shouldCheck;
}

void ContextMenuItem::setEnabled(bool shouldEnable)
{
    m_platformDescription.enabled = shouldEnable;
}

}

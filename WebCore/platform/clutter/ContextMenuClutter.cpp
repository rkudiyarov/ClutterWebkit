/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
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

#include "ContextMenuController.h"
#include <clutter/clutter.h>

namespace WebCore {

// TODO: ref-counting correctness checking.
// See http://bugs.webkit.org/show_bug.cgi?id=16115

static void menuItemActivated(GtkMenuItem* item, ContextMenuController* controller)
{
    ContextMenuItem contextItem(item);
    controller->contextMenuItemSelected(&contextItem);
}

ContextMenu::ContextMenu(const HitTestResult& result)
    : m_hitTestResult(result)
{
    m_platformDescription = 0;
}

ContextMenu::~ContextMenu()
{
    if (m_platformDescription)
        g_list_free(m_platformDescription);
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
    //ASSERT(m_platformDescription);
    checkOrEnableIfNeeded(item);

    ContextMenuItemType type = item.type();
    ClutterActor* platformItem = ContextMenuItem::createNativeMenuItem(item.releasePlatformDescription());
    //ASSERT(platformItem);
    m_platformDescription = g_list_append(m_platformDescription, platformItem);
    clutter_actor_show(CLUTTER_ACTOR(platformItem));
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription menu)
{
    ASSERT(menu);
    if (m_platformDescription)
        g_list_free(m_platformDescription);

    m_platformDescription = menu;
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    return m_platformDescription;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    PlatformMenuDescription description = m_platformDescription;
    m_platformDescription = 0;

    return description;
}

}

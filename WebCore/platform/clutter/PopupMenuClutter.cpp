/*
 * This file is part of the popup menu implementation for <select> elements in WebCore.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "PopupMenuClutter.h"

#include "NotImplemented.h"
#include "FrameView.h"
#include "HostWindow.h"
#include "PopupFactoryClientClutter.h"
#include "PlatformString.h"
#include <wtf/text/CString.h>

namespace WebCore {

PopupMenuClutter::PopupMenuClutter(PopupMenuClient* client)
    : m_popupClient(client)
{
}

PopupMenuClutter::~PopupMenuClutter()
{
}

void PopupMenuClutter::show(const IntRect& rect, FrameView* view, int index)
{
    // Page* page = view->frame()->page();
    // PopupFactoryClient* factory = page->popupFactoryClient();
    // 
    // factory->createPopupMenu(client(), this, index);
    notImplemented();
}

void PopupMenuClutter::hide()
{
    //     Page* page = client()->clientDocument()->page();
    // 
    //     if (!page) {
    // return;
    //     }
    // 
    //     PopupFactoryClient* factory = page->popupFactoryClient();
    //     factory->hidePopupMenu();
    notImplemented();
}

void PopupMenuClutter::updateFromElement()
{
    client()->setTextFromItem(client()->selectedIndex());
    notImplemented();
}

void PopupMenuClutter::disconnectClient()
{
    m_popupClient = 0;
}

}


/*
 * Copyright (C) 2008 Openedhand Ltd
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

#ifndef PopupFactoryClient_h
#define PopupFactoryClient_h

#include "PopupMenu.h"
#include "PopupMenuClient.h"

namespace WebCore {

class PopupFactoryClient {
public:
    virtual ~PopupFactoryClient() {}

    virtual void createPopupMenu(PopupMenuClient*, PopupMenu*, int) = 0;
    virtual void hidePopupMenu() = 0;
};

}

#endif

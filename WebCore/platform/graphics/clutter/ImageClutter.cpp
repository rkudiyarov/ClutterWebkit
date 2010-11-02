/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#include "BitmapImage.h"
#include "SharedBuffer.h"
#include "GOwnPtr.h"
#include "NotImplemented.h"
#include <wtf/text/CString.h>
#include <cairo.h>

#if PLATFORM(WIN)
#include <mbstring.h>
#include <shlobj.h>

static HMODULE hmodule;

extern "C" {
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        hmodule = hinstDLL;
    return TRUE;
}
}

static const char* getWebKitDataDirectory()
{
    static char* dataDirectory = 0;
    if (dataDirectory)
        return dataDirectory;

    dataDirectory = new char[PATH_MAX];
    if (!GetModuleFileName(hmodule, static_cast<CHAR*>(dataDirectory), sizeof(dataDirectory) - 10))
        return DATA_DIR;

    // FIXME: This is pretty ugly. Ideally we should be using Windows API
    // functions or GLib methods to calculate paths.
    unsigned char *p;
    p = _mbsrchr(static_cast<const unsigned char *>(dataDirectory), '\\');
    *p = '\0';
    p = _mbsrchr(static_cast<const unsigned char *>(dataDirectory), '\\');
    if (p) {
        if (!stricmp((const char *) (p+1), "bin"))
            *p = '\0';
    }
    strcat(dataDirectory, "\\share");

    return dataDirectory;
}

#else

static const char* getWebKitDataDirectory()
{
    return DATA_DIR;
}

#endif

namespace WebCore {

static CString getThemeIconFileName(const char* name, int size)
{
    notImplemented();
    return CString();
}

static PassRefPtr<SharedBuffer> loadResourceSharedBuffer(CString name)
{
    GOwnPtr<gchar> content;
    gsize length;
    if (!g_file_get_contents(name.data(), &content.outPtr(), &length, 0))
        return SharedBuffer::create();

    return SharedBuffer::create(content.get(), length);
}

void BitmapImage::initPlatformData()
{
}

void BitmapImage::invalidatePlatformData()
{
}

PassRefPtr<Image> loadImageFromFile(CString fileName)
{
    RefPtr<BitmapImage> img = BitmapImage::create();
    if (!fileName.isNull()) {
        RefPtr<SharedBuffer> buffer = loadResourceSharedBuffer(fileName);
        img->setData(buffer.release(), true);
    }
    return img.release();
}

PassRefPtr<Image> Image::loadPlatformResource(const char* name)
{
    CString fileName;
    if (fileName.isNull() || !strcmp("missingImage", name)) {
        GOwnPtr<gchar> imageName(g_strdup_printf("%s.png", name));
        GOwnPtr<gchar> glibFileName(g_build_filename(getWebKitDataDirectory(), "webkit-clutter-"WEBKITCLUTTER_API_VERSION_STRING, "images", imageName.get(), NULL));
        fileName = glibFileName.get();
    }

    return loadImageFromFile(fileName);
}

}

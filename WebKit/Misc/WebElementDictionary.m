/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebElementDictionary.h"

#import "WebDOMOperations.h"
#import "WebFrame.h"
#import "WebFrameBridge.h"
#import "WebFrameInternal.h"
#import "WebView.h"
#import "WebViewPrivate.h"

#import <WebKit/DOMCore.h>
#import <WebKit/DOMExtensions.h>
#import <WebCore/FrameMac.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/Image.h>
#import <WebCore/KURL.h>

using namespace WebCore;

static CFMutableDictionaryRef lookupTable = NULL;

static void addLookupKey(NSString *key, SEL selector)
{
    CFDictionaryAddValue(lookupTable, key, selector);
}

static void cacheValueForKey(const void *key, const void *value, void *self)
{
    // calling objectForKey will cache the value in our _cache dictionary
    [(WebElementDictionary *)self objectForKey:(NSString *)key];
}

@implementation WebElementDictionary

+ (void)initializeLookupTable
{
    if (lookupTable)
        return;

    lookupTable = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFCopyStringDictionaryKeyCallBacks, NULL);

    addLookupKey(WebElementDOMNodeKey, @selector(_domNode));
    addLookupKey(WebElementFrameKey, @selector(_webFrame));
    addLookupKey(WebElementImageAltStringKey, @selector(_altDisplayString));
    addLookupKey(WebElementImageKey, @selector(_image));
    addLookupKey(WebElementImageRectKey, @selector(_imageRect));
    addLookupKey(WebElementImageURLKey, @selector(_absoluteImageURL));
    addLookupKey(WebElementIsSelectedKey, @selector(_isSelected));
    addLookupKey(WebElementSpellingToolTipKey, @selector(_spellingToolTip));
    addLookupKey(WebElementTitleKey, @selector(_title));
    addLookupKey(WebElementLinkURLKey, @selector(_absoluteLinkURL));
    addLookupKey(WebElementLinkTargetFrameKey, @selector(_targetWebFrame));
    addLookupKey(WebElementLinkTitleKey, @selector(_titleDisplayString));
    addLookupKey(WebElementLinkLabelKey, @selector(_textContent));
    addLookupKey(WebElementLinkIsLiveKey, @selector(_isLiveLink));
}

- (id)initWithHitTestResult:(const HitTestResult&)result
{
    [[self class] initializeLookupTable];
    [super init];
    _result = new HitTestResult(result);
    return self;
}

- (void)dealloc
{
    delete _result;
    [_cache release];
    [_nilValues release];
    [super dealloc];
}

- (void)finalize
{
    delete _result;
    [super finalize];
}

- (void)_fillCache
{
    CFDictionaryApplyFunction(lookupTable, cacheValueForKey, self);
    _cacheComplete = YES;
}

- (unsigned)count
{
    if (!_cacheComplete)
        [self _fillCache];
    return [_cache count];
}

- (NSEnumerator *)keyEnumerator
{
    if (!_cacheComplete)
        [self _fillCache];
    return [_cache keyEnumerator];
}

- (id)objectForKey:(id)key
{
    id value = [_cache objectForKey:key];
    if (value || _cacheComplete || [_nilValues containsObject:key])
        return value;

    SEL selector = (SEL)CFDictionaryGetValue(lookupTable, key);
    if (!selector)
        return nil;
    value = [self performSelector:selector];

    unsigned lookupTableCount = CFDictionaryGetCount(lookupTable);
    if (value) {
        if (!_cache)
            _cache = [[NSMutableDictionary alloc] initWithCapacity:lookupTableCount];
        [_cache setObject:value forKey:key];
    } else {
        if (!_nilValues)
            _nilValues = [[NSMutableSet alloc] initWithCapacity:lookupTableCount];
        [_nilValues addObject:key];
    }

    _cacheComplete = ([_cache count] + [_nilValues count]) == lookupTableCount;

    return value;
}

- (DOMNode *)_domNode
{
    return kit(_result->innerNonSharedNode());
}

- (WebFrame *)_webFrame
{
    return [[[self _domNode] ownerDocument] webFrame];
}

- (NSString *)_altDisplayString
{
    return _result->altDisplayString();
}

- (NSString *)_spellingToolTip
{
    return _result->spellingToolTip();
}

- (NSImage *)_image
{
    Image* image = _result->image();
    return image ? image->getNSImage() : nil;
}

- (NSValue *)_imageRect
{
    if ([self objectForKey:WebElementImageURLKey])
        return [NSValue valueWithRect:_result->boundingBox()];
    return nil;
}

- (NSURL *)_absoluteImageURL
{
    return _result->absoluteImageURL().getNSURL();
}

- (NSNumber *)_isSelected
{
    return [NSNumber numberWithBool:_result->isSelected()];
}

- (NSString *)_title
{
    return _result->title();
}

- (NSURL *)_absoluteLinkURL
{
    return _result->absoluteLinkURL().getNSURL();
}

- (WebFrame *)_targetWebFrame
{
    return kit(_result->targetFrame());
}

- (NSString *)_titleDisplayString
{
    return _result->titleDisplayString();
}

- (NSString *)_textContent
{
    return _result->textContent();
}

- (NSNumber *)_isLiveLink
{
    return [NSNumber numberWithBool:_result->isLiveLink()];
}

@end

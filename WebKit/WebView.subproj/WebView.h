/*	
        WebView.h
	Copyright 2001, 2002, Apple Computer, Inc.

        Public header file.
*/

#import <Cocoa/Cocoa.h>

@class WebBackForwardList;
@class WebHistoryItem;
@class WebViewPrivate;
@class WebDataSource;
@class WebFrame;
@class NSURLConnection;
@class NSURLDownload;
@class WebPreferences;
@class WebFrameView;


// These strings are keys into the element dictionary provided in
// the WebContextMenuDelegate's contextMenuItemsForElement and the WebwebViewPolicyDelegate's clickPolicyForElement.

extern NSString *WebElementFrameKey;		// WebFrame of the element
extern NSString *WebElementImageAltStringKey;	// NSString of the ALT attribute of the image element
extern NSString *WebElementImageKey;		// NSImage of the image element
extern NSString *WebElementImageRectKey;	// NSValue of an NSRect, the rect of the image element
extern NSString *WebElementImageURLKey;		// NSURL of the image element
extern NSString *WebElementIsSelectedTextKey; 	// NSNumber of BOOL indicating whether the element is selected text or not 
extern NSString *WebElementLinkURLKey;		// NSURL of the link if the element is within an anchor
extern NSString *WebElementLinkTargetFrameKey;	// NSString of the target of the anchor
extern NSString *WebElementLinkTitleKey;	// NSString of the title of the anchor
extern NSString *WebElementLinkLabelKey;	// NSString of the text within the anchor

/*!
    @class WebView
    WebView manages the interaction between WebFrameViews and WebDataSources.  Modification
    of the policies and behavior of the WebKit is largely managed by WebViews and their
    delegates.
    
    <p>
    Typical usage:
    </p>
    <pre>
    WebView *webView;
    WebFrame *mainFrame;
    
    webView  = [[WebView alloc] initWithFrame: NSMakeRect (0,0,640,480)];
    mainFrame = [webView mainFrame];
    [mainFrame loadRequest:request];
    </pre>
    
    WebViews have the following delegates:  WebUIDelegate, WebResourceLoadDelegate,
    WebFrameLoadDelegate, and WebPolicyDelegate.
    
    WebKit depends on the WebView's WebUIDelegate for all window
    related management, including opening new windows and controlling the user interface
    elements in those windows.
    
    WebResourceLoadDelegate is used to monitor the progress of resources as they are
    loaded.  This delegate may be used to present users with a progress monitor.
    
    The WebFrameLoadDelegate receives messages when the URL in a WebFrame is
    changed.
    
    WebView's WebPolicyDelegate can make determinations about how
    content should be handled, based on the resource's URL and MIME type.
*/
@interface WebView : NSView
{
@private
    WebViewPrivate *_private;
}

/*!
    @method canShowMIMEType:
    @abstract Checks if the WebKit can show content of a certain MIME type.
    @param MIMEType The MIME type to check.
    @result YES if the WebKit can show content with MIMEtype.
*/
+ (BOOL)canShowMIMEType:(NSString *)MIMEType;

/*!
    @method initWithFrame:frameName:groupName:
    @abstract The designated initializer for WebView.
    @discussion Initialize a WebView with the supplied parameters. This method will 
    create a main WebFrame with the view. Passing a top level frame name is useful if you
    handle a targetted frame navigation that would normally open a window in some other 
    way that still ends up creating a new WebView.
    @param frame The frame used to create the view.
    @param frameName The name to use for the top level frame. May be nil.
    @param groupName The name of the webView set to which this webView will be added.  May be nil.
    @result Returns an initialized WebView.
*/
- initWithFrame: (NSRect)frame frameName: (NSString *)frameName groupName: (NSString *)groupName;

/*!
    @method setUIDelegate:
    @abstract Set the WebView's WebUIDelegate.
    @param delegate The WebUIDelegate to set as the delegate.
*/    
- (void)setUIDelegate: (id)delegate;

/*!
    @method UIDelegate
    @abstract Return the WebView's WebUIDelegate.
    @result The WebView's WebUIDelegate.
*/
- (id)UIDelegate;

/*!
    @method setResourceLoadDelegate:
    @abstract Set the WebView's WebResourceLoadDelegate load delegate.
    @param delegate The WebResourceLoadDelegate to set as the load delegate.
*/
- (void)setResourceLoadDelegate: (id)delegate;

/*!
    @method resourceLoadDelegate
    @result Return the WebView's WebResourceLoadDelegate.
*/    
- (id)resourceLoadDelegate;

/*!
    @method setDownloadDelegate:
    @abstract Set the WebView's NSURLDownloadDelegate.
    @discussion The download delegate is retained by NSURLDownload when any downloads are in progress.
    @param delegate The NSURLDownloadDelegate to set as the download delegate.
*/    
- (void)setDownloadDelegate: (id)delegate;

/*!
    @method downloadDelegate
    @abstract Return the WebView's NSURLDownloadDelegate.
    @result The WebView's NSURLDownloadDelegate.
*/    
- (id)downloadDelegate;

/*!
    @method setFrameLoadDelegate:
    @abstract Set the WebView's WebFrameLoadDelegate delegate.
    @param delegate The WebFrameLoadDelegate to set as the delegate.
*/    
- (void)setFrameLoadDelegate: (id)delegate;

/*!
    @method frameLoadDelegate
    @abstract Return the WebView's WebFrameLoadDelegate delegate.
    @result The WebView's WebFrameLoadDelegate delegate.
*/    
- (id)frameLoadDelegate;

/*!
    @method setPolicyDelegate:
    @abstract Set the WebView's WebPolicyDelegate delegate.
    @param delegate The WebPolicyDelegate to set as the delegate.
*/    
- (void)setPolicyDelegate: (id)delegate;

/*!
    @method policyDelegate
    @abstract Return the WebView's WebPolicyDelegate.
    @result The WebView's WebPolicyDelegate.
*/    
- (id)policyDelegate;

/*!
    @method mainFrame
    @abstract Return the top level frame.  
    @discussion Note that even document that are not framesets will have a
    mainFrame.
    @result The main frame.
*/    
- (WebFrame *)mainFrame;

/*!
    @method backForwardList
    @result The backforward list for this webView.
*/    
- (WebBackForwardList *)backForwardList;

/*!
    @method setUseBackForwardList:
    @abstract Enable or disable the use of a backforward list for this webView.
    @param flag Turns use of the back forward list on or off
*/    
- (void)setMaintainsBackForwardList: (BOOL)flag;

/*!
    @method goBack
    @abstract Go back to the previous URL in the backforward list.
    @result YES if able to go back in the backforward list, NO otherwise.
*/    
- (BOOL)goBack;

/*!
    @method goForward
    @abstract Go forward to the next URL in the backforward list.
    @result YES if able to go forward in the backforward list, NO otherwise.
*/    
- (BOOL)goForward;

/*!
    @method goBackOrForwardToItem:
    @abstract Go back or forward to an item in the backforward list.
    @result YES if able to go to the item, NO otherwise.
*/    
- (BOOL)goBackOrForwardToItem:(WebHistoryItem *)item;

/*!
    @method setTextSizeMultiplier:
    @abstract Change the size of the text rendering in views managed by this webView.
    @param multiplier A fractional percentage value, 1.0 is 100%.
*/    
- (void)setTextSizeMultiplier:(float)multiplier;

/*!
    @method textSizeMultiplier
    @result The text size multipler.
*/    
- (float)textSizeMultiplier;

/*!
    @method setApplicationNameForUserAgent:
    @abstract Set the application name. 
    @discussion This name will be used in user-agent strings
    that are chosen for best results in rendering web pages.
    @param applicationName The application name
*/
- (void)setApplicationNameForUserAgent:(NSString *)applicationName;

/*!
    @method applicationNameForUserAgent
    @result The name of the application as used in the user-agent string.
*/
- (NSString *)applicationNameForUserAgent;

/*!
    @method setCustomUserAgent:
    @abstract Set the user agent. 
    @discussion Setting this means that the webView should use this user-agent string
    instead of constructing a user-agent string for each URL. Setting it to nil
    causes the webView to construct the user-agent string for each URL
    for best results rendering web pages.
    @param userAgentString The user agent description
*/
- (void)setCustomUserAgent:(NSString *)userAgentString;

/*!
    @method customUserAgent
    @result The custom user-agent string or nil if no custom user-agent string has been set.
*/
- (NSString *)customUserAgent;

/*!
    @method userAgentForURL:
    @abstract Get the appropriate user-agent string for a particular URL.
    @param URL The URL.
    @result The user-agent string for the supplied URL.
*/
- (NSString *)userAgentForURL:(NSURL *)URL;


/*!
    @method supportsTextEncoding
    @abstract Find out if the current web page supports text encodings.
    @result YES if the document view of the current web page can
    support different text encodings.
*/
- (BOOL)supportsTextEncoding;

/*!
    @method setCustomTextEncodingName:
    @discussion Make the page display with a different text encoding; stops any load in progress.
    The text encoding passed in overrides the normal text encoding smarts including
    what's specified in a web page's header or HTTP response.
    The text encoding automatically goes back to the default when the top level frame
    changes to a new location.
    Setting the text encoding name to nil makes the webView use default encoding rules.
    @param encoding The text encoding name to use to display a page or nil.
*/
- (void)setCustomTextEncodingName:(NSString *)encodingName;

/*!
    @method customTextEncodingName
    @result The custom text encoding name or nil if no custom text encoding name has been set.
*/
- (NSString *)customTextEncodingName;

/*!
    @method stringByEvaluatingJavaScriptFromString:
    @param script The text of the JavaScript.
    @result The result of the script, converted to a string, or nil for failure.
*/
- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)script;

/*!
    @method setPreferences:
    @param preferences The preferences to use for the webView.
    @abstract Override the standard setting for the webView. 
*/
- (void)setPreferences: (WebPreferences *)prefs;

/*!
    @method preferences
    @result Returns the preferences used by this webView.
    @discussion This method will return [WebPreferences standardPreferences] if no
    other instance of WebPreferences has been set.
*/
- (WebPreferences *)preferences;

/*!
    @method setHostWindow:
    @param hostWindow The host window for the web view.
    @discussion Parts of WebKit (such as plug-ins and JavaScript) depend on a window to function
    properly. Set a host window so these parts continue to function even when the web view is
    not in an actual window.
*/
- (void)setHostWindow:(NSWindow *)hostWindow;

/*!
    @method hostWindow
    @result The host window for the web view.
*/
- (NSWindow *)hostWindow;

@end


@interface WebView (WebIBActions)
- (IBAction)takeStringURLFrom:(id)sender;
- (IBAction)stopLoading:(id)sender;
- (IBAction)reload:(id)sender;
- (BOOL)canGoBack;
- (IBAction)goBack:(id)sender;
- (BOOL)canGoForward;
- (IBAction)goForward:(id)sender;
- (BOOL)canMakeTextLarger;
- (IBAction)makeTextLarger:(id)sender;
- (BOOL)canMakeTextSmaller;
- (IBAction)makeTextSmaller:(id)sender;
@end

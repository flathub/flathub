#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>
#import <dispatch/dispatch.h>
#import <objc/runtime.h>
#import <stdbool.h>

typedef struct {
    NSInteger macos_major_version;
    NSInteger tier_tag;
    NSInteger toolbar_height;
    NSInteger traffic_light_inset_leading;
} OKWindowShellProfile;

typedef NS_ENUM(NSInteger, OKWindowShellTier) {
    OKWindowShellTierDesktop = 0,
    OKWindowShellTierMacLegacy = 1,
    OKWindowShellTierMacNative = 2,
};

static const void *OKWindowShellSplitViewKey = &OKWindowShellSplitViewKey;
static const void *OKWindowShellContainerViewKey = &OKWindowShellContainerViewKey;
static const void *OKWindowShellSplitControllerKey = &OKWindowShellSplitControllerKey;
static const void *OKWindowShellSidebarItemKey = &OKWindowShellSidebarItemKey;
static const void *OKWindowShellSidebarControllerKey = &OKWindowShellSidebarControllerKey;
static const void *OKWindowShellMainControllerKey = &OKWindowShellMainControllerKey;

static NSSplitViewController *openkara_window_shell_split_controller(NSWindow *window);
static NSSplitViewItem *openkara_window_shell_sidebar_item(NSWindow *window);

static NSSplitViewController *openkara_window_shell_split_controller(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellSplitControllerKey);
}

static NSSplitViewItem *openkara_window_shell_sidebar_item(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellSidebarItemKey);
}

static NSViewController *openkara_window_shell_sidebar_controller(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellSidebarControllerKey);
}

static NSViewController *openkara_window_shell_main_controller(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellMainControllerKey);
}

static NSView *openkara_window_shell_container_view(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellContainerViewKey);
}

static void openkara_set_window_shell_split_view(NSWindow *window, NSSplitView *splitView) {
    objc_setAssociatedObject(
        window,
        OKWindowShellSplitViewKey,
        splitView,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_split_controller(
    NSWindow *window,
    NSSplitViewController *splitController
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellSplitControllerKey,
        splitController,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_sidebar_item(
    NSWindow *window,
    NSSplitViewItem *sidebarItem
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellSidebarItemKey,
        sidebarItem,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_sidebar_controller(
    NSWindow *window,
    NSViewController *controller
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellSidebarControllerKey,
        controller,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_main_controller(
    NSWindow *window,
    NSViewController *controller
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellMainControllerKey,
        controller,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_container_view(NSWindow *window, NSView *containerView) {
    objc_setAssociatedObject(
        window,
        OKWindowShellContainerViewKey,
        containerView,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static CGFloat openkara_clamp_sidebar_width(NSSplitView *splitView, CGFloat sidebarWidth) {
    CGFloat minMainPaneWidth = 420.0;
    CGFloat maxSidebarWidth = MAX(0.0, NSWidth(splitView.bounds) - minMainPaneWidth);
    return MIN(sidebarWidth, maxSidebarWidth);
}

static NSViewController *openkara_make_pane_controller(void) {
    NSViewController *controller = [[NSViewController alloc] init];
    NSView *view = [[NSView alloc] initWithFrame:NSZeroRect];
    view.autoresizesSubviews = YES;
    view.wantsLayer = YES;
    view.layer.masksToBounds = YES;
    controller.view = view;
    return controller;
}

static void openkara_attach_webview_to_pane(WKWebView *webview, NSView *pane) {
    if (webview == nil || pane == nil) {
        return;
    }

    if (webview.superview != pane) {
        [webview removeFromSuperview];
        webview.frame = pane.bounds;
        webview.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [pane addSubview:webview];
    } else {
        webview.frame = pane.bounds;
    }
}

static void openkara_sync_split_sidebar_visibility(
    NSWindow *window,
    CGFloat sidebarWidth,
    BOOL sidebarVisible
) {
    NSSplitViewController *splitController = openkara_window_shell_split_controller(window);
    NSSplitViewItem *sidebarItem = openkara_window_shell_sidebar_item(window);
    NSSplitView *splitView = splitController.splitView;
    if (splitView == nil || sidebarItem == nil) {
        return;
    }

    CGFloat clampedSidebarWidth = openkara_clamp_sidebar_width(splitView, sidebarWidth);
    sidebarItem.canCollapse = YES;
    sidebarItem.minimumThickness = 220.0;
    sidebarItem.maximumThickness = MAX(220.0, clampedSidebarWidth);
    sidebarItem.holdingPriority = NSLayoutPriorityDefaultHigh;

    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
        context.duration = 0.18;
        if (sidebarVisible) {
            sidebarItem.animator.collapsed = NO;
            [[splitView animator] setPosition:clampedSidebarWidth ofDividerAtIndex:0];
        } else {
            sidebarItem.animator.collapsed = YES;
        }
    } completionHandler:nil];
}

static void openkara_run_on_main_thread_sync(dispatch_block_t block) {
    if (block == nil) {
        return;
    }

    if ([NSThread isMainThread]) {
        block();
        return;
    }

    dispatch_sync(dispatch_get_main_queue(), block);
}

void ok_window_shell_detect_profile(OKWindowShellProfile *profile_out) {
    if (profile_out == NULL) {
        return;
    }

    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    profile_out->macos_major_version = version.majorVersion;
    profile_out->tier_tag = version.majorVersion >= 26 ? OKWindowShellTierMacNative
                                                       : OKWindowShellTierMacLegacy;
    profile_out->toolbar_height = version.majorVersion >= 26 ? 56 : 52;
    profile_out->traffic_light_inset_leading = version.majorVersion >= 26 ? 78 : 64;
}

bool ok_window_shell_configure_main_window(
    void *ns_view_ptr,
    NSInteger tier_tag,
    double toolbar_height,
    double traffic_light_inset_leading,
    OKWindowShellProfile *profile_out
) {
    if (ns_view_ptr == NULL) {
        return false;
    }

    __block BOOL configured = NO;
    openkara_run_on_main_thread_sync(^{
        NSView *view = (__bridge NSView *)ns_view_ptr;
        NSWindow *window = view.window;
        if (window == nil) {
            window = NSApp.keyWindow ?: NSApp.mainWindow ?: NSApp.orderedWindows.firstObject;
        }
        if (window == nil) {
            return;
        }

        window.titleVisibility = NSWindowTitleHidden;
        window.titlebarAppearsTransparent = YES;
        window.tabbingMode = NSWindowTabbingModeDisallowed;
        window.toolbarStyle = tier_tag == OKWindowShellTierMacNative
                                  ? NSWindowToolbarStyleUnified
                                  : NSWindowToolbarStyleUnifiedCompact;
        window.backgroundColor = [NSColor windowBackgroundColor];

        NSWindowStyleMask styleMask = [window styleMask];
        if ((styleMask & NSWindowStyleMaskFullSizeContentView) == 0) {
            [window setStyleMask:(styleMask | NSWindowStyleMaskFullSizeContentView)];
        }

        [window setToolbar:nil];

        NSButton *closeButton = [window standardWindowButton:NSWindowCloseButton];
        NSButton *minimizeButton = [window standardWindowButton:NSWindowMiniaturizeButton];
        NSButton *zoomButton = [window standardWindowButton:NSWindowZoomButton];
        CGFloat resolvedLeadingInset = traffic_light_inset_leading;
        CGFloat maxTrafficLightEdge = 0.0;
        if (closeButton != nil) {
            maxTrafficLightEdge = MAX(maxTrafficLightEdge, NSMaxX(closeButton.frame));
        }
        if (minimizeButton != nil) {
            maxTrafficLightEdge = MAX(maxTrafficLightEdge, NSMaxX(minimizeButton.frame));
        }
        if (zoomButton != nil) {
            maxTrafficLightEdge = MAX(maxTrafficLightEdge, NSMaxX(zoomButton.frame));
        }
        if (maxTrafficLightEdge > 0.0) {
            // RATIONALE: The sidebar toggle should sit after the standard window
            // controls with a stable click target, so collapsing the sidebar does
            // not move the user's cursor off the button.
            CGFloat trafficLightGap =
                tier_tag == OKWindowShellTierMacNative ? 14.0 : 12.0;
            resolvedLeadingInset = maxTrafficLightEdge + trafficLightGap;
        }

        CGFloat resolvedToolbarHeight = toolbar_height;

        if (profile_out != NULL) {
            NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
            profile_out->macos_major_version = version.majorVersion;
            profile_out->tier_tag = tier_tag;
            profile_out->toolbar_height = (NSInteger)lround(resolvedToolbarHeight);
            profile_out->traffic_light_inset_leading = (NSInteger)lround(resolvedLeadingInset);
        }

        configured = YES;
    });

    return configured;
}

bool ok_window_shell_mount_container_views(
    void *ns_window_ptr,
    void *sidebar_webview_ptr,
    void *main_content_webview_ptr,
    double sidebar_width,
    bool sidebar_visible
) {
    if (ns_window_ptr == NULL || sidebar_webview_ptr == NULL || main_content_webview_ptr == NULL) {
        return false;
    }

    __block BOOL mounted = NO;
    openkara_run_on_main_thread_sync(^{
        NSWindow *window = (__bridge NSWindow *)ns_window_ptr;
        WKWebView *sidebarWebview = (__bridge WKWebView *)sidebar_webview_ptr;
        WKWebView *mainContentWebview = (__bridge WKWebView *)main_content_webview_ptr;
        NSView *contentView = window.contentView;
        if (window == nil || contentView == nil || sidebarWebview == nil || mainContentWebview == nil) {
            return;
        }

        NSView *containerView = openkara_window_shell_container_view(window);
        if (containerView == nil) {
            containerView = [[NSView alloc] initWithFrame:NSZeroRect];
            containerView.translatesAutoresizingMaskIntoConstraints = NO;
            [contentView addSubview:containerView];
            [NSLayoutConstraint activateConstraints:@[
                [containerView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
                [containerView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor],
                [containerView.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor],
                // RATIONALE: The native sidebar should own the full left edge of
                // the window, like System Settings, instead of starting below a
                // separate titlebar band.
                [containerView.topAnchor constraintEqualToAnchor:contentView.topAnchor],
            ]];
            openkara_set_window_shell_container_view(window, containerView);
        }

        NSSplitViewController *splitController = openkara_window_shell_split_controller(window);
        NSViewController *sidebarController = openkara_window_shell_sidebar_controller(window);
        NSViewController *mainController = openkara_window_shell_main_controller(window);
        NSSplitViewItem *sidebarItem = openkara_window_shell_sidebar_item(window);
        if (splitController == nil) {
            splitController = [[NSSplitViewController alloc] init];
            splitController.view.translatesAutoresizingMaskIntoConstraints = NO;

            sidebarController = openkara_make_pane_controller();
            mainController = openkara_make_pane_controller();

            sidebarItem = [NSSplitViewItem sidebarWithViewController:sidebarController];
            sidebarItem.canCollapse = YES;
            sidebarItem.minimumThickness = 220.0;
            sidebarItem.maximumThickness = MAX(220.0, sidebar_width);
            sidebarItem.holdingPriority = NSLayoutPriorityDefaultHigh;

            NSSplitViewItem *mainItem = [NSSplitViewItem splitViewItemWithViewController:mainController];
            mainItem.minimumThickness = 420.0;

            [splitController addSplitViewItem:sidebarItem];
            [splitController addSplitViewItem:mainItem];

            splitController.splitView.dividerStyle = NSSplitViewDividerStyleThin;

            [containerView addSubview:splitController.view];
            [NSLayoutConstraint activateConstraints:@[
                [splitController.view.leadingAnchor constraintEqualToAnchor:containerView.leadingAnchor],
                [splitController.view.trailingAnchor constraintEqualToAnchor:containerView.trailingAnchor],
                [splitController.view.topAnchor constraintEqualToAnchor:containerView.topAnchor],
                [splitController.view.bottomAnchor constraintEqualToAnchor:containerView.bottomAnchor],
            ]];

            openkara_set_window_shell_split_controller(window, splitController);
            openkara_set_window_shell_split_view(window, splitController.splitView);
            openkara_set_window_shell_sidebar_item(window, sidebarItem);
            openkara_set_window_shell_sidebar_controller(window, sidebarController);
            openkara_set_window_shell_main_controller(window, mainController);
        }

        openkara_attach_webview_to_pane(sidebarWebview, sidebarController.view);
        openkara_attach_webview_to_pane(mainContentWebview, mainController.view);

        [containerView layoutSubtreeIfNeeded];
        [splitController.splitView adjustSubviews];
        openkara_sync_split_sidebar_visibility(window, sidebar_width, sidebar_visible);
        mounted = YES;
    });

    return mounted;
}

bool ok_window_shell_set_split_sidebar_visibility(
    void *ns_window_ptr,
    double sidebar_width,
    bool sidebar_visible
) {
    if (ns_window_ptr == NULL) {
        return false;
    }

    __block BOOL updated = NO;
    openkara_run_on_main_thread_sync(^{
        NSWindow *window = (__bridge NSWindow *)ns_window_ptr;
        if (window == nil) {
            return;
        }

        openkara_sync_split_sidebar_visibility(window, sidebar_width, sidebar_visible);
        updated = YES;
    });

    return updated;
}

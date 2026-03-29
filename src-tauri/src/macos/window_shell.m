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
    NSInteger sidebar_header_height;
} OKWindowShellProfile;

typedef NS_ENUM(NSInteger, OKWindowShellTier) {
    OKWindowShellTierDesktop = 0,
    OKWindowShellTierMacLegacy = 1,
    OKWindowShellTierMacNative = 2,
};

static const void *OKWindowShellContainerViewKey = &OKWindowShellContainerViewKey;
static const void *OKWindowShellSplitControllerKey = &OKWindowShellSplitControllerKey;
static const void *OKWindowShellSidebarItemKey = &OKWindowShellSidebarItemKey;
static const void *OKWindowShellSidebarControllerKey = &OKWindowShellSidebarControllerKey;
static const void *OKWindowShellMainControllerKey = &OKWindowShellMainControllerKey;
static const void *OKWindowShellSidebarBackdropKey = &OKWindowShellSidebarBackdropKey;
static const void *OKWindowShellSidebarBackdropWidthConstraintKey =
    &OKWindowShellSidebarBackdropWidthConstraintKey;
static const void *OKWindowShellTrafficLightsHostKey = &OKWindowShellTrafficLightsHostKey;
static const void *OKWindowShellResolvedLeadingInsetKey = &OKWindowShellResolvedLeadingInsetKey;
static const void *OKWindowShellResolvedSidebarHeaderHeightKey =
    &OKWindowShellResolvedSidebarHeaderHeightKey;

static const CGFloat OKWindowShellNativeTrafficLightTrailingGap = 14.0;
static const CGFloat OKWindowShellNativeSidebarHeaderHeight = 40.0;

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

static NSView *openkara_window_shell_chrome_host_view(NSWindow *window) {
    NSView *contentView = window.contentView;
    if (contentView == nil) {
        return nil;
    }

    // RATIONALE: Standard traffic lights live in the window chrome subtree,
    // not inside contentView.  To place a native sidebar material behind them,
    // the backdrop must be attached to the shared chrome host above contentView.
    return contentView.superview;
}

static NSVisualEffectView *openkara_window_shell_sidebar_backdrop(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellSidebarBackdropKey);
}

static NSLayoutConstraint *openkara_window_shell_sidebar_backdrop_width_constraint(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellSidebarBackdropWidthConstraintKey);
}

static NSView *openkara_window_shell_traffic_lights_host(NSWindow *window) {
    return objc_getAssociatedObject(window, OKWindowShellTrafficLightsHostKey);
}

static CGFloat openkara_window_shell_resolved_leading_inset(NSWindow *window) {
    NSNumber *value = objc_getAssociatedObject(window, OKWindowShellResolvedLeadingInsetKey);
    return value != nil ? value.doubleValue : 78.0;
}

static CGFloat openkara_window_shell_resolved_sidebar_header_height(NSWindow *window) {
    NSNumber *value = objc_getAssociatedObject(window, OKWindowShellResolvedSidebarHeaderHeightKey);
    return value != nil ? value.doubleValue : OKWindowShellNativeSidebarHeaderHeight;
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

static void openkara_set_window_shell_sidebar_backdrop(
    NSWindow *window,
    NSVisualEffectView *backdrop
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellSidebarBackdropKey,
        backdrop,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_sidebar_backdrop_width_constraint(
    NSWindow *window,
    NSLayoutConstraint *constraint
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellSidebarBackdropWidthConstraintKey,
        constraint,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_traffic_lights_host(NSWindow *window, NSView *host) {
    objc_setAssociatedObject(
        window,
        OKWindowShellTrafficLightsHostKey,
        host,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_resolved_leading_inset(NSWindow *window, CGFloat inset) {
    objc_setAssociatedObject(
        window,
        OKWindowShellResolvedLeadingInsetKey,
        @(inset),
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
}

static void openkara_set_window_shell_resolved_sidebar_header_height(
    NSWindow *window,
    CGFloat height
) {
    objc_setAssociatedObject(
        window,
        OKWindowShellResolvedSidebarHeaderHeightKey,
        @(height),
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

static void openkara_configure_sidebar_backdrop(NSVisualEffectView *backdrop) {
    if (backdrop == nil) {
        return;
    }

    backdrop.material = NSVisualEffectMaterialSidebar;
    backdrop.blendingMode = NSVisualEffectBlendingModeWithinWindow;
    backdrop.state = NSVisualEffectStateFollowsWindowActiveState;
    backdrop.wantsLayer = YES;
    // RATIONALE: Both the titlebar bridge and the sidebar body must share the
    // same native tone, otherwise the traffic-light strip reads as a separate band.
    backdrop.layer.backgroundColor =
        [NSColor colorWithSRGBRed:0.08 green:0.10 blue:0.14 alpha:0.12].CGColor;
}

static void openkara_configure_webview_background(WKWebView *webview, BOOL transparent) {
    if (webview == nil) {
        return;
    }

    [webview setValue:@(!transparent) forKey:@"drawsBackground"];
    webview.wantsLayer = YES;
    webview.layer.backgroundColor =
        transparent ? [NSColor clearColor].CGColor : [NSColor windowBackgroundColor].CGColor;
}

static void openkara_set_standard_window_buttons_hidden(NSWindow *window, BOOL hidden) {
    if (window == nil) {
        return;
    }

    NSButton *buttons[] = {
        [window standardWindowButton:NSWindowCloseButton],
        [window standardWindowButton:NSWindowMiniaturizeButton],
        [window standardWindowButton:NSWindowZoomButton],
    };

    for (NSUInteger index = 0; index < sizeof(buttons) / sizeof(buttons[0]); index += 1) {
        NSButton *button = buttons[index];
        if (button == nil) {
            continue;
        }

        button.hidden = hidden;
        button.enabled = !hidden;
    }
}

static NSButton *openkara_make_custom_traffic_light_button(
    NSWindow *window,
    NSColor *color,
    SEL action,
    NSString *accessibilityLabel,
    NSInteger tag
) {
    NSButton *button = [NSButton buttonWithTitle:@"" target:window action:action];
    button.tag = tag;
    button.translatesAutoresizingMaskIntoConstraints = NO;
    button.bordered = NO;
    button.bezelStyle = NSBezelStyleRegularSquare;
    button.buttonType = NSButtonTypeMomentaryChange;
    button.wantsLayer = YES;
    button.layer.cornerRadius = 6.0;
    button.layer.masksToBounds = YES;
    button.layer.backgroundColor = color.CGColor;
    button.accessibilityLabel = accessibilityLabel;
    return button;
}

static void openkara_attach_native_traffic_lights_to_host(
    NSWindow *window,
    NSView *host,
    CGFloat leadingInset,
    CGFloat headerHeight
) {
    if (window == nil || host == nil) {
        return;
    }

    NSButton *closeButton = [host viewWithTag:7101];
    NSButton *minimizeButton = [host viewWithTag:7102];
    NSButton *zoomButton = [host viewWithTag:7103];

    if (closeButton == nil || minimizeButton == nil || zoomButton == nil) {
        [host.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];

        closeButton = openkara_make_custom_traffic_light_button(
            window,
            [NSColor colorWithSRGBRed:1.0 green:0.372 blue:0.337 alpha:1.0],
            @selector(performClose:),
            @"Close Window",
            7101
        );
        minimizeButton = openkara_make_custom_traffic_light_button(
            window,
            [NSColor colorWithSRGBRed:1.0 green:0.741 blue:0.18 alpha:1.0],
            @selector(performMiniaturize:),
            @"Minimize Window",
            7102
        );
        zoomButton = openkara_make_custom_traffic_light_button(
            window,
            [NSColor colorWithSRGBRed:0.165 green:0.829 blue:0.349 alpha:1.0],
            @selector(performZoom:),
            @"Zoom Window",
            7103
        );

        [host addSubview:closeButton];
        [host addSubview:minimizeButton];
        [host addSubview:zoomButton];
    }

    CGFloat buttonSize = 12.0;
    CGFloat buttonGap = 8.0;
    CGFloat clusterWidth = buttonSize * 3.0 + buttonGap * 2.0;
    CGFloat targetMinX = MAX(12.0, leadingInset - clusterWidth - OKWindowShellNativeTrafficLightTrailingGap);
    CGFloat targetMinY = MAX(0.0, floor((headerHeight - buttonSize) * 0.5));

    closeButton.frame = NSMakeRect(targetMinX, targetMinY, buttonSize, buttonSize);
    minimizeButton.frame = NSMakeRect(targetMinX + buttonSize + buttonGap, targetMinY, buttonSize, buttonSize);
    zoomButton.frame = NSMakeRect(targetMinX + (buttonSize + buttonGap) * 2.0, targetMinY, buttonSize, buttonSize);
}

static BOOL openkara_apply_native_sidebar_state(
    NSWindow *window,
    CGFloat sidebarWidth,
    BOOL sidebarVisible,
    BOOL animated
) {
    NSSplitViewController *splitController = openkara_window_shell_split_controller(window);
    NSSplitViewItem *sidebarItem = openkara_window_shell_sidebar_item(window);
    NSLayoutConstraint *widthConstraint =
        openkara_window_shell_sidebar_backdrop_width_constraint(window);
    NSSplitView *splitView = splitController.splitView;
    if (splitView == nil || sidebarItem == nil || widthConstraint == nil) {
        return NO;
    }

    CGFloat clampedSidebarWidth = openkara_clamp_sidebar_width(splitView, sidebarWidth);
    sidebarItem.canCollapse = YES;
    sidebarItem.minimumThickness = 220.0;
    sidebarItem.maximumThickness = MAX(220.0, clampedSidebarWidth);
    sidebarItem.holdingPriority = NSLayoutPriorityDefaultHigh;

    if (!animated) {
        widthConstraint.constant = sidebarVisible ? clampedSidebarWidth : 0.0;
        if (sidebarVisible) {
            sidebarItem.collapsed = NO;
            [splitView setPosition:clampedSidebarWidth ofDividerAtIndex:0];
        } else {
            sidebarItem.collapsed = YES;
        }
        return YES;
    }

    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
        context.duration = 0.18;
        [[widthConstraint animator] setConstant:(sidebarVisible ? clampedSidebarWidth : 0.0)];
        if (sidebarVisible) {
            sidebarItem.animator.collapsed = NO;
            [[splitView animator] setPosition:clampedSidebarWidth ofDividerAtIndex:0];
        } else {
            sidebarItem.animator.collapsed = YES;
        }
    } completionHandler:nil];

    return YES;
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

static NSRect openkara_union_rect(NSRect current, NSRect next, BOOL *hasRect) {
    if (!*hasRect) {
        *hasRect = YES;
        return next;
    }

    return NSUnionRect(current, next);
}

static BOOL openkara_layout_native_traffic_lights(
    NSWindow *window,
    CGFloat sidebarHeaderHeight,
    CGFloat *resolvedLeadingInsetOut,
    CGFloat *resolvedSidebarHeaderHeightOut
) {
    NSButton *buttons[] = {
        [window standardWindowButton:NSWindowCloseButton],
        [window standardWindowButton:NSWindowMiniaturizeButton],
        [window standardWindowButton:NSWindowZoomButton],
    };
    NSView *buttonContainer = nil;
    NSRect clusterBounds = NSZeroRect;
    BOOL hasClusterBounds = NO;

    for (NSUInteger index = 0; index < sizeof(buttons) / sizeof(buttons[0]); index += 1) {
        NSButton *button = buttons[index];
        if (button == nil || button.superview == nil) {
            continue;
        }

        if (buttonContainer == nil) {
            buttonContainer = button.superview;
        }

        if (button.superview != buttonContainer) {
            return NO;
        }

        clusterBounds = openkara_union_rect(clusterBounds, button.frame, &hasClusterBounds);
    }

    if (buttonContainer == nil || !hasClusterBounds) {
        return NO;
    }

    CGFloat resolvedSidebarHeaderHeight = MAX(sidebarHeaderHeight, NSHeight(clusterBounds));

    if (resolvedLeadingInsetOut != NULL) {
        // RATIONALE: Native mode keeps the standard traffic lights in AppKit's
        // titlebar control subtree.  We measure their real edge and let the
        // sidebar's native backdrop extend underneath, instead of reparenting or
        // restyling the system buttons.
        *resolvedLeadingInsetOut = NSMaxX(clusterBounds) + OKWindowShellNativeTrafficLightTrailingGap;
    }
    if (resolvedSidebarHeaderHeightOut != NULL) {
        *resolvedSidebarHeaderHeightOut = resolvedSidebarHeaderHeight;
    }

    return YES;
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
    profile_out->sidebar_header_height = version.majorVersion >= 26 ? 40 : 0;
}

bool ok_window_shell_configure_main_window(
    void *ns_view_ptr,
    NSInteger tier_tag,
    double toolbar_height,
    double traffic_light_inset_leading,
    double sidebar_header_height,
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
            return;
        }

        window.titleVisibility = NSWindowTitleHidden;
        window.titlebarAppearsTransparent = YES;
        window.tabbingMode = NSWindowTabbingModeDisallowed;

        // RATIONALE: NSWindowToolbarStyleUnified inflates the titlebar height
        // even without a toolbar, creating the visual band that the native
        // shell design explicitly avoids.  Only the legacy tier needs a
        // toolbar style (UnifiedCompact) for its web-rendered toolbar.
        if (tier_tag != OKWindowShellTierMacNative) {
            window.toolbarStyle = NSWindowToolbarStyleUnifiedCompact;
        }

        if (tier_tag == OKWindowShellTierMacNative) {
            window.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
        }

        window.backgroundColor = [NSColor windowBackgroundColor];
        openkara_set_standard_window_buttons_hidden(window, tier_tag == OKWindowShellTierMacNative);

        NSWindowStyleMask styleMask = [window styleMask];
        if ((styleMask & NSWindowStyleMaskFullSizeContentView) == 0) {
            [window setStyleMask:(styleMask | NSWindowStyleMaskFullSizeContentView)];
        }

        [window setToolbar:nil];

        CGFloat resolvedLeadingInset = traffic_light_inset_leading;
        CGFloat resolvedSidebarHeaderHeight = sidebar_header_height;
        if (tier_tag == OKWindowShellTierMacNative) {
            if (!openkara_layout_native_traffic_lights(
                window,
                MAX(sidebar_header_height, OKWindowShellNativeSidebarHeaderHeight),
                &resolvedLeadingInset,
                &resolvedSidebarHeaderHeight
            )) {
                return;
            }

            openkara_set_window_shell_resolved_leading_inset(window, resolvedLeadingInset);
            openkara_set_window_shell_resolved_sidebar_header_height(
                window,
                resolvedSidebarHeaderHeight
            );
        } else {
            NSButton *closeButton = [window standardWindowButton:NSWindowCloseButton];
            NSButton *minimizeButton = [window standardWindowButton:NSWindowMiniaturizeButton];
            NSButton *zoomButton = [window standardWindowButton:NSWindowZoomButton];
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
                resolvedLeadingInset = maxTrafficLightEdge + 12.0;
            }
        }

        CGFloat resolvedToolbarHeight = toolbar_height;

        if (profile_out != NULL) {
            NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
            profile_out->macos_major_version = version.majorVersion;
            profile_out->tier_tag = tier_tag;
            profile_out->toolbar_height = (NSInteger)lround(resolvedToolbarHeight);
            profile_out->traffic_light_inset_leading = (NSInteger)lround(resolvedLeadingInset);
            profile_out->sidebar_header_height = (NSInteger)lround(resolvedSidebarHeaderHeight);
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
        NSView *chromeHostView = openkara_window_shell_chrome_host_view(window);
        if (window == nil || contentView == nil || chromeHostView == nil || sidebarWebview == nil || mainContentWebview == nil) {
            return;
        }

        openkara_configure_webview_background(sidebarWebview, YES);
        openkara_configure_webview_background(mainContentWebview, NO);

        NSView *containerView = openkara_window_shell_container_view(window);
        if (containerView == nil) {
            containerView = [[NSView alloc] initWithFrame:NSZeroRect];
            containerView.translatesAutoresizingMaskIntoConstraints = NO;
            [chromeHostView addSubview:containerView];
            [NSLayoutConstraint activateConstraints:@[
                [containerView.leadingAnchor constraintEqualToAnchor:chromeHostView.leadingAnchor],
                [containerView.trailingAnchor constraintEqualToAnchor:chromeHostView.trailingAnchor],
                [containerView.bottomAnchor constraintEqualToAnchor:chromeHostView.bottomAnchor],
                // RATIONALE: The native sidebar should own the full left edge of
                // the window, like System Settings, instead of starting below a
                // separate titlebar band.
                [containerView.topAnchor constraintEqualToAnchor:chromeHostView.topAnchor],
            ]];
            openkara_set_window_shell_container_view(window, containerView);
        }

        NSVisualEffectView *sidebarBackdrop = openkara_window_shell_sidebar_backdrop(window);
        if (sidebarBackdrop == nil) {
            sidebarBackdrop = [[NSVisualEffectView alloc] initWithFrame:NSZeroRect];
            sidebarBackdrop.translatesAutoresizingMaskIntoConstraints = NO;
            openkara_configure_sidebar_backdrop(sidebarBackdrop);

            [chromeHostView addSubview:sidebarBackdrop positioned:NSWindowBelow relativeTo:containerView];

            NSLayoutConstraint *widthConstraint =
                [sidebarBackdrop.widthAnchor constraintEqualToConstant:sidebar_visible ? sidebar_width : 0.0];

            [NSLayoutConstraint activateConstraints:@[
                [sidebarBackdrop.leadingAnchor constraintEqualToAnchor:chromeHostView.leadingAnchor],
                [sidebarBackdrop.topAnchor constraintEqualToAnchor:chromeHostView.topAnchor],
                [sidebarBackdrop.bottomAnchor constraintEqualToAnchor:chromeHostView.bottomAnchor],
                widthConstraint,
            ]];

            openkara_set_window_shell_sidebar_backdrop(window, sidebarBackdrop);
            openkara_set_window_shell_sidebar_backdrop_width_constraint(window, widthConstraint);
        } else if (sidebarBackdrop.superview == chromeHostView) {
            [chromeHostView addSubview:sidebarBackdrop positioned:NSWindowBelow relativeTo:containerView];
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
            openkara_set_window_shell_sidebar_item(window, sidebarItem);
            openkara_set_window_shell_sidebar_controller(window, sidebarController);
            openkara_set_window_shell_main_controller(window, mainController);
        }

        NSView *trafficLightsHost = openkara_window_shell_traffic_lights_host(window);
        if (trafficLightsHost == nil) {
            trafficLightsHost = [[NSView alloc] initWithFrame:NSZeroRect];
            trafficLightsHost.translatesAutoresizingMaskIntoConstraints = NO;
            [containerView addSubview:trafficLightsHost positioned:NSWindowAbove relativeTo:splitController.view];
            [NSLayoutConstraint activateConstraints:@[
                [trafficLightsHost.leadingAnchor constraintEqualToAnchor:containerView.leadingAnchor],
                [trafficLightsHost.topAnchor constraintEqualToAnchor:containerView.topAnchor],
                [trafficLightsHost.widthAnchor constraintEqualToConstant:openkara_window_shell_resolved_leading_inset(window)],
                [trafficLightsHost.heightAnchor constraintEqualToConstant:openkara_window_shell_resolved_sidebar_header_height(window)],
            ]];
            openkara_set_window_shell_traffic_lights_host(window, trafficLightsHost);
        }

        openkara_attach_native_traffic_lights_to_host(
            window,
            trafficLightsHost,
            openkara_window_shell_resolved_leading_inset(window),
            openkara_window_shell_resolved_sidebar_header_height(window)
        );

        sidebarController.view.layer.backgroundColor = [NSColor clearColor].CGColor;
        mainController.view.layer.backgroundColor = [NSColor windowBackgroundColor].CGColor;

        openkara_attach_webview_to_pane(sidebarWebview, sidebarController.view);
        openkara_attach_webview_to_pane(mainContentWebview, mainController.view);

        [containerView layoutSubtreeIfNeeded];
        [splitController.splitView adjustSubviews];

        mounted = openkara_apply_native_sidebar_state(window, sidebar_width, sidebar_visible, NO);
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

        if (openkara_window_shell_split_controller(window) == nil ||
            openkara_window_shell_sidebar_item(window) == nil ||
            openkara_window_shell_sidebar_backdrop_width_constraint(window) == nil) {
            return;
        }

        updated = openkara_apply_native_sidebar_state(window, sidebar_width, sidebar_visible, YES);
    });

    return updated;
}

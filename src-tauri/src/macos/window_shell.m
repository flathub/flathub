#import <AppKit/AppKit.h>
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
    OKWindowShellTierMac = 1,
};

static const CGFloat OKWindowShellTrafficLightTrailingGap = 14.0;
static const CGFloat OKWindowShellSidebarHeaderHeight = 28.0;
static const CGFloat OKWindowShellTrafficLightLeadingInset = 14.0;
static const CGFloat OKWindowShellTrafficLightTopInset = 14.0;
static const CGFloat OKWindowShellTrafficLightHorizontalGap = 6.0;

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
    }

    if (buttonContainer == nil) {
        return NO;
    }

    CGFloat nextLeadingX = OKWindowShellTrafficLightLeadingInset;
    for (NSUInteger index = 0; index < sizeof(buttons) / sizeof(buttons[0]); index += 1) {
        NSButton *button = buttons[index];
        if (button == nil || button.superview != buttonContainer) {
            continue;
        }

        NSRect frame = button.frame;
        frame.origin.x = nextLeadingX;
        frame.origin.y = NSHeight(buttonContainer.bounds) - OKWindowShellTrafficLightTopInset - NSHeight(frame);
        [button setFrameOrigin:frame.origin];
        nextLeadingX += NSWidth(frame) + OKWindowShellTrafficLightHorizontalGap;
    }

    NSRect clusterBounds = NSZeroRect;
    BOOL hasClusterBounds = NO;
    for (NSUInteger index = 0; index < sizeof(buttons) / sizeof(buttons[0]); index += 1) {
        NSButton *button = buttons[index];
        if (button == nil || button.superview != buttonContainer) {
            continue;
        }
        clusterBounds = openkara_union_rect(clusterBounds, button.frame, &hasClusterBounds);
    }
    if (!hasClusterBounds) {
        return NO;
    }

    CGFloat resolvedSidebarHeaderHeight =
        MAX(sidebarHeaderHeight, NSHeight(clusterBounds) + OKWindowShellTrafficLightTopInset);

    if (resolvedLeadingInsetOut != NULL) {
        // RATIONALE: Keep the standard traffic lights in AppKit's titlebar control
        // subtree; measure their edge instead of reparenting or restyling.
        *resolvedLeadingInsetOut = NSMaxX(clusterBounds) + OKWindowShellTrafficLightTrailingGap;
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
    profile_out->tier_tag = OKWindowShellTierMac;
    profile_out->toolbar_height = 48;
    profile_out->traffic_light_inset_leading = 78;
    profile_out->sidebar_header_height = 28;
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
        window.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
        window.movableByWindowBackground = YES;

        window.backgroundColor = [NSColor windowBackgroundColor];

        NSWindowStyleMask styleMask = [window styleMask];
        if ((styleMask & NSWindowStyleMaskFullSizeContentView) == 0) {
            [window setStyleMask:(styleMask | NSWindowStyleMaskFullSizeContentView)];
        }

        [window setToolbar:nil];

        CGFloat resolvedLeadingInset = traffic_light_inset_leading;
        CGFloat resolvedSidebarHeaderHeight = sidebar_header_height;
        if (tier_tag != OKWindowShellTierMac) {
            return;
        }
        if (!openkara_layout_native_traffic_lights(
            window,
            MAX(sidebar_header_height, OKWindowShellSidebarHeaderHeight),
            &resolvedLeadingInset,
            &resolvedSidebarHeaderHeight
        )) {
            return;
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

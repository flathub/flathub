#import <AppKit/AppKit.h>
#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>

typedef void (*OKAirPlayStateCallback)(bool active, const char *route_name, int mode_tag);

typedef NS_ENUM(NSInteger, OKAirPlayMode) {
    OKAirPlayModeIdle = 0,
    OKAirPlayModeLyrics = 1,
    OKAirPlayModeCdg = 2,
};

@interface OKAirPlayBridge : NSObject API_AVAILABLE(macos(10.15))

@property(nonatomic, strong) AVPlayer *player;
@property(nonatomic, strong) AVRoutePickerView *routePickerView;
@property(nonatomic, copy) NSString *silenceAssetPath;
@property(nonatomic, assign) OKAirPlayMode currentMode;
@property(nonatomic, assign) OKAirPlayStateCallback stateCallback;
@property(nonatomic, assign) BOOL observingPlayer;

+ (instancetype)sharedBridge;
- (void)syncRoutePickerForRootView:(NSView *)rootView
                              left:(CGFloat)left
                               top:(CGFloat)top
                             width:(CGFloat)width
                            height:(CGFloat)height
                           mounted:(BOOL)mounted
                  silenceAssetPath:(NSString *)silenceAssetPath;
- (void)updateMode:(OKAirPlayMode)mode
 placeholderEnabled:(BOOL)placeholderEnabled
  silenceAssetPath:(NSString *)silenceAssetPath;

@end

@implementation OKAirPlayBridge

+ (instancetype)sharedBridge {
    static OKAirPlayBridge *sharedBridge = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedBridge = [[OKAirPlayBridge alloc] init];
    });
    return sharedBridge;
}

- (void)dealloc {
    if (self.observingPlayer) {
        [self.player removeObserver:self forKeyPath:@"externalPlaybackActive"];
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)emitState {
    if (self.stateCallback == NULL) {
        return;
    }

    BOOL active = self.player != nil && self.player.externalPlaybackActive;
    self.stateCallback(active, NULL, (int)self.currentMode);
}

- (void)ensurePlayer {
    if (self.player != nil) {
        return;
    }

    self.player = [[AVPlayer alloc] init];
    self.player.muted = YES;
    self.player.allowsExternalPlayback = YES;
    self.player.actionAtItemEnd = AVPlayerActionAtItemEndNone;
    [self.player addObserver:self
                  forKeyPath:@"externalPlaybackActive"
                     options:NSKeyValueObservingOptionNew
                     context:NULL];
    self.observingPlayer = YES;

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePlayerItemDidEnd:)
                                                 name:AVPlayerItemDidPlayToEndTimeNotification
                                               object:nil];
}

- (AVPlayerItem *)buildPlaceholderItem {
    if (self.silenceAssetPath.length == 0) {
        return nil;
    }

    NSURL *url = [NSURL fileURLWithPath:self.silenceAssetPath isDirectory:NO];
    return [AVPlayerItem playerItemWithURL:url];
}

- (void)enablePlaceholderPlayback {
    [self ensurePlayer];

    if (self.player.currentItem == nil) {
        AVPlayerItem *item = [self buildPlaceholderItem];
        if (item != nil) {
            [self.player replaceCurrentItemWithPlayerItem:item];
        }
    }

    if (self.player.currentItem != nil) {
        [self.player seekToTime:kCMTimeZero];
        [self.player play];
    }
}

- (void)disablePlaceholderPlayback {
    if (self.player == nil) {
        [self emitState];
        return;
    }

    [self.player pause];
    [self.player replaceCurrentItemWithPlayerItem:nil];
    [self emitState];
}

- (void)handlePlayerItemDidEnd:(NSNotification *)notification {
    if (notification.object != self.player.currentItem) {
        return;
    }

    [self.player seekToTime:kCMTimeZero];
    [self.player play];
}

- (void)syncRoutePickerForRootView:(NSView *)rootView
                              left:(CGFloat)left
                               top:(CGFloat)top
                             width:(CGFloat)width
                            height:(CGFloat)height
                           mounted:(BOOL)mounted
                  silenceAssetPath:(NSString *)silenceAssetPath {
    self.silenceAssetPath = silenceAssetPath ?: self.silenceAssetPath;

    if (!mounted || rootView == nil) {
        [self.routePickerView removeFromSuperview];
        return;
    }

    [self ensurePlayer];

    CGFloat hostHeight = NSHeight(rootView.bounds);
    NSRect frame = NSMakeRect(left, hostHeight - top - height, width, height);

    if (self.routePickerView == nil) {
        self.routePickerView = [[AVRoutePickerView alloc] initWithFrame:frame];
        self.routePickerView.routePickerButtonBordered = NO;
        self.routePickerView.player = self.player;
    } else {
        self.routePickerView.player = self.player;
        self.routePickerView.frame = frame;
    }

    if (self.routePickerView.superview != rootView) {
        [self.routePickerView removeFromSuperview];
        [rootView addSubview:self.routePickerView];
    }
}

- (void)updateMode:(OKAirPlayMode)mode
 placeholderEnabled:(BOOL)placeholderEnabled
  silenceAssetPath:(NSString *)silenceAssetPath {
    self.currentMode = mode;
    self.silenceAssetPath = silenceAssetPath ?: self.silenceAssetPath;

    if (placeholderEnabled) {
        [self enablePlaceholderPlayback];
    } else {
        [self disablePlaceholderPlayback];
    }

    [self emitState];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
    if ([keyPath isEqualToString:@"externalPlaybackActive"] && object == self.player) {
        [self emitState];
        return;
    }

    [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

@end

static void ok_airplay_dispatch_main(void (^block)(void)) {
    if ([NSThread isMainThread]) {
        block();
        return;
    }

    dispatch_sync(dispatch_get_main_queue(), block);
}

void ok_airplay_set_state_callback(OKAirPlayStateCallback callback) {
    if (@available(macOS 10.15, *)) {
        ok_airplay_dispatch_main(^{
            [OKAirPlayBridge sharedBridge].stateCallback = callback;
            [[OKAirPlayBridge sharedBridge] emitState];
        });
        return;
    }

    if (callback != NULL) {
        callback(false, NULL, (int)OKAirPlayModeIdle);
    }
}

void ok_airplay_sync_route_picker(
    void *ns_view_ptr,
    double left,
    double top,
    double width,
    double height,
    bool mounted,
    const char *silence_asset_path
) {
    if (@available(macOS 10.15, *)) {
        NSString *path = silence_asset_path == NULL ? nil : [NSString stringWithUTF8String:silence_asset_path];

        ok_airplay_dispatch_main(^{
            NSView *rootView = (__bridge NSView *)ns_view_ptr;
            [[OKAirPlayBridge sharedBridge] syncRoutePickerForRootView:rootView
                                                                  left:(CGFloat)left
                                                                   top:(CGFloat)top
                                                                 width:(CGFloat)width
                                                                height:(CGFloat)height
                                                               mounted:mounted
                                                      silenceAssetPath:path];
        });
    }
}

void ok_airplay_update_mode(int mode, bool placeholder_enabled, const char *silence_asset_path) {
    if (@available(macOS 10.15, *)) {
        NSString *path = silence_asset_path == NULL ? nil : [NSString stringWithUTF8String:silence_asset_path];

        ok_airplay_dispatch_main(^{
            [[OKAirPlayBridge sharedBridge] updateMode:(OKAirPlayMode)mode
                                     placeholderEnabled:placeholder_enabled
                                       silenceAssetPath:path];
        });
        return;
    }
}

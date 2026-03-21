#import <AppKit/AppKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreText/CoreText.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

typedef void (*OKAirPlayStateCallback)(
    bool active,
    bool audio_active,
    const char *route_name,
    int mode_tag,
    int phase_tag,
    const char *detail,
    long long displayed_position_ms,
    unsigned long long stream_generation,
    long long latency_ms
);

typedef NS_ENUM(NSInteger, OKAirPlayMode) {
    OKAirPlayModeIdle = 0,
    OKAirPlayModeLyrics = 1,
    OKAirPlayModeCdg = 2,
};

typedef NS_ENUM(NSInteger, OKAirPlayPhase) {
    OKAirPlayPhaseIdle = 0,
    OKAirPlayPhaseRouteSelected = 1,
    OKAirPlayPhaseBuffering = 2,
    OKAirPlayPhasePlaying = 3,
    OKAirPlayPhaseFailed = 4,
};

static const NSInteger OKAirPlayVideoWidth = 1280;
static const NSInteger OKAirPlayVideoHeight = 720;
static const NSInteger OKAirPlayFramesPerSecond = 30;
static const NSInteger OKAirPlayAudioSampleRate = 44100;
static const NSInteger OKAirPlayAudioChannels = 2;
static const NSInteger OKAirPlayAudioFramesPerTick =
    OKAirPlayAudioSampleRate / OKAirPlayFramesPerSecond;
// RATIONALE: This bridge currently emits ordinary live HLS, not LL-HLS with
// partial segments and blocking playlist reload. Apple explicitly recommends
// whole-second segment intervals for HLS, and an ultra-short live window makes
// receivers fall off the live edge after only a few seconds.
static const NSInteger OKAirPlayPlaylistWindow = 8;
static const NSInteger OKAirPlayPreferredSegmentIntervalSeconds = 1;
static const NSTimeInterval OKAirPlayPreferredForwardBufferDuration = 2.0;
static const NSInteger OKAirPlayMaxSyncPoints = 720;
static const void *OKAirPlayStateQueueSpecificKey = &OKAirPlayStateQueueSpecificKey;

static NSString *OKStringValue(id value);
static NSDictionary *OKDictionaryValue(id value);
static NSArray *OKArrayValue(id value);
static long long OKLongLongValue(id value);
static NSInteger OKIntegerValue(id value);
static BOOL OKBoolValue(id value);

@interface OKAirPlaySegmentEntry : NSObject

@property(nonatomic, assign) NSInteger sequence;
@property(nonatomic, copy) NSString *filename;
@property(nonatomic, assign) NSTimeInterval duration;

@end

@implementation OKAirPlaySegmentEntry
@end

@interface OKAirPlayVariantStream : NSObject

@property(nonatomic, copy) NSString *playlistFilename;
@property(nonatomic, copy) NSString *filenamePrefix;
@property(nonatomic, assign) BOOL includesVideo;
@property(nonatomic, strong) AVAssetWriter *writer;
@property(nonatomic, strong) AVAssetWriterInput *videoInput;
@property(nonatomic, strong) AVAssetWriterInput *audioInput;
@property(nonatomic, strong) AVAssetWriterInputPixelBufferAdaptor *pixelBufferAdaptor;
@property(nonatomic, assign) BOOL writerStarted;
@property(nonatomic, assign) BOOL hasInitializationSegment;
@property(nonatomic, assign) BOOL hasWrittenVideo;
@property(nonatomic, assign) BOOL hasWrittenAudio;
@property(nonatomic, strong) NSMutableArray<OKAirPlaySegmentEntry *> *segments;
@property(nonatomic, assign) NSInteger nextSegmentSequence;
@property(nonatomic, assign) int64_t nextVideoFrameIndex;
@property(nonatomic, assign) int64_t nextAudioFrameIndex;

- (instancetype)initWithPlaylistFilename:(NSString *)playlistFilename
                          filenamePrefix:(NSString *)filenamePrefix
                           includesVideo:(BOOL)includesVideo;
- (void)reset;

@end

@implementation OKAirPlayVariantStream

- (instancetype)initWithPlaylistFilename:(NSString *)playlistFilename
                          filenamePrefix:(NSString *)filenamePrefix
                           includesVideo:(BOOL)includesVideo {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _playlistFilename = [playlistFilename copy];
    _filenamePrefix = [filenamePrefix copy];
    _includesVideo = includesVideo;
    _segments = [NSMutableArray array];
    return self;
}

- (void)reset {
    self.writer = nil;
    self.videoInput = nil;
    self.audioInput = nil;
    self.pixelBufferAdaptor = nil;
    self.writerStarted = NO;
    self.hasInitializationSegment = NO;
    self.hasWrittenVideo = NO;
    self.hasWrittenAudio = NO;
    [self.segments removeAllObjects];
    self.nextSegmentSequence = 0;
    self.nextVideoFrameIndex = 0;
    self.nextAudioFrameIndex = 0;
}

@end

static CGFloat OKClamp(CGFloat value, CGFloat minValue, CGFloat maxValue) {
    return MAX(minValue, MIN(maxValue, value));
}

static CGFloat OKFontSizeForStep(NSInteger step) {
    switch ((int)OKClamp(step, -2, 2)) {
    case -2:
        return 44.0;
    case -1:
        return 54.0;
    case 1:
        return 82.0;
    case 2:
        return 96.0;
    default:
        return 68.0;
    }
}

static NSInteger OKActiveWordIndex(NSArray *words, long long adjustedMs) {
    NSInteger activeIndex = -1;
    for (NSUInteger index = 0; index < words.count; index += 1) {
        NSDictionary *word = OKDictionaryValue(words[index]);
        if (word == nil) {
            continue;
        }
        if (OKLongLongValue(word[@"timeMs"]) > adjustedMs) {
            break;
        }
        activeIndex = (NSInteger)index;
    }
    return activeIndex;
}

static CGRect OKAspectFitRect(CGSize sourceSize, CGRect bounds) {
    if (sourceSize.width <= 0.0 || sourceSize.height <= 0.0 || CGRectIsEmpty(bounds)) {
        return bounds;
    }

    CGFloat widthScale = bounds.size.width / sourceSize.width;
    CGFloat heightScale = bounds.size.height / sourceSize.height;
    CGFloat scale = MIN(widthScale, heightScale);
    CGSize fitted = CGSizeMake(sourceSize.width * scale, sourceSize.height * scale);

    return CGRectMake(
        bounds.origin.x + (bounds.size.width - fitted.width) * 0.5,
        bounds.origin.y + (bounds.size.height - fitted.height) * 0.5,
        fitted.width,
        fitted.height
    );
}

static CGRect OKTopAlignedRect(CGFloat x, CGFloat topY, CGFloat width, CGFloat height) {
    return CGRectMake(x, OKAirPlayVideoHeight - topY - height, width, height);
}

static NSString *OKStringValue(id value) {
    return [value isKindOfClass:[NSString class]] ? value : nil;
}

static NSDictionary *OKDictionaryValue(id value) {
    return [value isKindOfClass:[NSDictionary class]] ? value : nil;
}

static NSArray *OKArrayValue(id value) {
    return [value isKindOfClass:[NSArray class]] ? value : nil;
}

static long long OKLongLongValue(id value) {
    return [value respondsToSelector:@selector(longLongValue)] ? [value longLongValue] : 0;
}

static NSInteger OKIntegerValue(id value) {
    return [value respondsToSelector:@selector(integerValue)] ? [value integerValue] : 0;
}

static BOOL OKBoolValue(id value) {
    return [value respondsToSelector:@selector(boolValue)] ? [value boolValue] : NO;
}

static OKAirPlayMode OKModeValue(id value) {
    NSString *mode = OKStringValue(value);
    if (mode != nil) {
        if ([mode isEqualToString:@"lyrics"]) {
            return OKAirPlayModeLyrics;
        }
        if ([mode isEqualToString:@"cdg"]) {
            return OKAirPlayModeCdg;
        }
        return OKAirPlayModeIdle;
    }

    switch (OKIntegerValue(value)) {
    case 1:
        return OKAirPlayModeLyrics;
    case 2:
        return OKAirPlayModeCdg;
    default:
        return OKAirPlayModeIdle;
    }
}

static id OKBridgedColor(CGFloat r, CGFloat g, CGFloat b, CGFloat a) {
    return CFBridgingRelease(CGColorCreateGenericRGB(r, g, b, a));
}

static void OKSetFillColor(CGContextRef context, CGFloat r, CGFloat g, CGFloat b, CGFloat a) {
    CGContextSetRGBFillColor(context, r, g, b, a);
}

static NSDictionary *OKPresentationSpecValue(NSDictionary *scene) {
    return OKDictionaryValue(scene[@"presentationSpec"]) ?: @{};
}

static CGFloat OKSceneFloatValue(NSDictionary *dictionary, NSString *key, CGFloat fallback) {
    id value = dictionary[key];
    return [value respondsToSelector:@selector(doubleValue)] ? (CGFloat)[value doubleValue] : fallback;
}

static NSDictionary *OKColorDictionaryValue(
    NSDictionary *dictionary,
    NSString *key,
    CGFloat fallbackR,
    CGFloat fallbackG,
    CGFloat fallbackB,
    CGFloat fallbackA
) {
    NSDictionary *color = OKDictionaryValue(dictionary[key]);
    if (color == nil) {
        return @{
            @"red": @(fallbackR),
            @"green": @(fallbackG),
            @"blue": @(fallbackB),
            @"alpha": @(fallbackA),
        };
    }
    return color;
}

static id OKColorObjectFromDictionary(NSDictionary *color) {
    return OKBridgedColor(
        OKSceneFloatValue(color, @"red", 1.0),
        OKSceneFloatValue(color, @"green", 1.0),
        OKSceneFloatValue(color, @"blue", 1.0),
        OKSceneFloatValue(color, @"alpha", 1.0)
    );
}

static CTFontRef OKCreateBoldSystemFont(CGFloat size) {
    CTFontRef base = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, size, NULL);
    if (base == NULL) {
        return NULL;
    }

    CTFontRef bold = CTFontCreateCopyWithSymbolicTraits(
        base,
        size,
        NULL,
        kCTFontBoldTrait,
        kCTFontBoldTrait
    );
    if (bold != NULL) {
        CFRelease(base);
        return bold;
    }

    return base;
}

static NSDictionary *OKBaseTextAttributes(CGFloat fontSize, id color, CGFloat lineHeightMultiple) {
    CTFontRef font = OKCreateBoldSystemFont(fontSize);
    NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
    if (font != NULL) {
        attributes[(id)kCTFontAttributeName] = (__bridge id)font;
        CGFloat lineHeight = fontSize * lineHeightMultiple;
        CTTextAlignment alignment = kCTTextAlignmentCenter;
        CGFloat minimumLineHeight = lineHeight;
        CGFloat maximumLineHeight = lineHeight;
        CTParagraphStyleSetting settings[] = {
            { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment },
            { kCTParagraphStyleSpecifierMinimumLineHeight, sizeof(minimumLineHeight), &minimumLineHeight },
            { kCTParagraphStyleSpecifierMaximumLineHeight, sizeof(maximumLineHeight), &maximumLineHeight },
        };
        CTParagraphStyleRef paragraphStyle =
            CTParagraphStyleCreate(settings, sizeof(settings) / sizeof(settings[0]));
        if (paragraphStyle != NULL) {
            attributes[(id)kCTParagraphStyleAttributeName] = (__bridge id)paragraphStyle;
            CFRelease(paragraphStyle);
        }
        CFRelease(font);
    }
    attributes[(id)kCTForegroundColorAttributeName] = color;
    return attributes;
}

static NSData *OKResampleStereoPCM(const float *samples, NSUInteger frameCount, uint32_t sampleRate) {
    if (samples == NULL || frameCount == 0) {
        return [NSData data];
    }

    if (sampleRate == OKAirPlayAudioSampleRate) {
        return [NSData dataWithBytes:samples length:frameCount * OKAirPlayAudioChannels * sizeof(float)];
    }

    double ratio = (double)sampleRate / (double)OKAirPlayAudioSampleRate;
    NSUInteger outputFrames = MAX((NSUInteger)1, (NSUInteger)llround((double)frameCount / ratio));
    NSMutableData *data =
        [NSMutableData dataWithLength:outputFrames * OKAirPlayAudioChannels * sizeof(float)];
    float *output = data.mutableBytes;

    for (NSUInteger frame = 0; frame < outputFrames; frame += 1) {
        double sourcePosition = (double)frame * ratio;
        NSUInteger lowFrame = (NSUInteger)floor(sourcePosition);
        if (lowFrame >= frameCount) {
            lowFrame = frameCount - 1;
        }
        NSUInteger highFrame = MIN(lowFrame + 1, frameCount - 1);
        float fraction = (float)(sourcePosition - floor(sourcePosition));

        for (NSUInteger channel = 0; channel < OKAirPlayAudioChannels; channel += 1) {
            float lowSample = samples[lowFrame * OKAirPlayAudioChannels + channel];
            float highSample = samples[highFrame * OKAirPlayAudioChannels + channel];
            output[frame * OKAirPlayAudioChannels + channel] =
                lowSample + (highSample - lowSample) * fraction;
        }
    }

    return data;
}

@interface OKAirPlayBridge : NSObject <AVAssetWriterDelegate>

@property(nonatomic, strong) AVPlayer *player;
@property(nonatomic, strong) AVRoutePickerView *routePickerView;
@property(nonatomic, assign) OKAirPlayStateCallback stateCallback;
@property(nonatomic, assign) BOOL observingPlayer;
@property(nonatomic, strong) AVPlayerItem *observedItem;
@property(nonatomic, assign) BOOL observingCurrentItem;

@property(nonatomic, strong) dispatch_queue_t stateQueue;
@property(nonatomic, strong) dispatch_queue_t audioQueue;
@property(nonatomic, strong) dispatch_queue_t encoderQueue;
@property(nonatomic, strong) dispatch_source_t videoTimer;

@property(nonatomic, copy) NSString *streamRootPath;
@property(nonatomic, copy) NSString *playlistURLString;

@property(nonatomic, strong) OKAirPlayVariantStream *audienceStream;
@property(nonatomic, assign) BOOL realItemAttached;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *syncPoints;

@property(nonatomic, strong) NSMutableData *pendingAudienceAudioData;
@property(nonatomic, assign) NSUInteger pendingAudienceAudioOffset;
@property(nonatomic, assign) uint64_t currentAudioEpoch;

@property(nonatomic, assign) OKAirPlayMode currentMode;
@property(nonatomic, assign) OKAirPlayPhase currentPhase;
@property(nonatomic, copy) NSString *currentPhaseDetail;
@property(nonatomic, assign) uint64_t streamGeneration;
@property(nonatomic, assign) uint64_t attachedGeneration;
@property(nonatomic, strong) NSDictionary *latestSceneConfig;
@property(nonatomic, strong) NSDictionary *latestRuntimeState;
@property(nonatomic, strong) NSData *latestCdgFrame;
@property(nonatomic, strong) NSArray<NSNumber *> *plainTextPageStartIndices;
@property(nonatomic, assign) NSUInteger plainTextPageIndex;
@property(nonatomic, copy) NSString *plainTextPaginationSignature;
@property(nonatomic, assign) BOOL lastKnownExternalPlaybackActive;
@property(nonatomic, copy) NSString *lastPlaybackErrorDetail;
@property(nonatomic, assign) long long lastEmittedDisplayedPositionMs;
@property(nonatomic, assign) long long lastEmittedLatencyMs;
@property(nonatomic, assign) uint64_t lastEmittedStreamGeneration;
@property(nonatomic, assign) BOOL lastEmittedActive;
@property(nonatomic, assign) BOOL lastEmittedAudioActive;
@property(nonatomic, assign) OKAirPlayMode lastEmittedMode;
@property(nonatomic, assign) OKAirPlayPhase lastEmittedPhase;
@property(nonatomic, copy) NSString *lastEmittedDetail;

+ (instancetype)sharedBridge;
- (void)syncRoutePickerForRootView:(NSView *)rootView
                              left:(CGFloat)left
                               top:(CGFloat)top
                             width:(CGFloat)width
                            height:(CGFloat)height
                       mounted:(BOOL)mounted
                    streamRootPath:(NSString *)streamRootPath
                       playlistURL:(NSString *)playlistURL;
- (void)syncAudienceConfigWithJSON:(NSString *)configJSON;
- (void)syncAudienceRuntimeWithJSON:(NSString *)runtimeJSON
                           cdgFrame:(NSData *)cdgFrame;
- (BOOL)stepPlainTextPageWithDirection:(NSInteger)direction;
- (void)pushAudioSamples:(const float *)samples
             sampleCount:(NSUInteger)sampleCount
              sampleRate:(uint32_t)sampleRate
                channels:(uint16_t)channels
                   epoch:(uint64_t)epoch;
- (void)applyAudioEpoch:(uint64_t)epoch;
- (void)refreshPlaybackPhase;
- (BOOL)hasAudienceVideoRoute;
- (BOOL)hasRemoteAudioRoute;
- (BOOL)isAudienceStreamReadyForAttachment;

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

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _stateQueue = dispatch_queue_create("openkara.airplay.state", DISPATCH_QUEUE_SERIAL);
    dispatch_queue_set_specific(
        _stateQueue,
        OKAirPlayStateQueueSpecificKey,
        (void *)OKAirPlayStateQueueSpecificKey,
        NULL
    );
    _audioQueue = dispatch_queue_create("openkara.airplay.audio", DISPATCH_QUEUE_SERIAL);
    _encoderQueue = dispatch_queue_create("openkara.airplay.encoder", DISPATCH_QUEUE_SERIAL);
    _audienceStream =
        [[OKAirPlayVariantStream alloc] initWithPlaylistFilename:@"audience-video.m3u8"
                                                  filenamePrefix:@"audience"
                                                   includesVideo:YES];
    _syncPoints = [NSMutableArray array];
    _pendingAudienceAudioData = [NSMutableData data];
    _currentMode = OKAirPlayModeIdle;
    _currentPhase = OKAirPlayPhaseIdle;
    _currentAudioEpoch = 1;
    _streamGeneration = 1;
    _attachedGeneration = 0;
    _plainTextPageStartIndices = @[];
    _plainTextPageIndex = 0;
    _lastEmittedDisplayedPositionMs = -1;
    _lastEmittedLatencyMs = -1;
    _lastEmittedStreamGeneration = 0;
    _lastEmittedActive = NO;
    _lastEmittedAudioActive = NO;
    _lastEmittedMode = OKAirPlayModeIdle;
    _lastEmittedPhase = OKAirPlayPhaseIdle;
    return self;
}

- (void)dealloc {
    if (self.observingPlayer) {
        [self.player removeObserver:self forKeyPath:@"externalPlaybackActive"];
        [self.player removeObserver:self forKeyPath:@"timeControlStatus"];
        [self.player removeObserver:self forKeyPath:@"currentItem"];
    }
    if (self.observingCurrentItem && self.observedItem != nil) {
        [self.observedItem removeObserver:self forKeyPath:@"status"];
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:AVPlayerItemNewErrorLogEntryNotification
                                                      object:self.observedItem];
    }
}

- (void)emitState {
    if (self.stateCallback == NULL) {
        return;
    }

    BOOL active = [self hasAudienceVideoRoute] && self.currentPhase == OKAirPlayPhasePlaying;
    BOOL audioActive = [self hasRemoteAudioRoute];
    const char *detail = self.currentPhaseDetail.length > 0 ? self.currentPhaseDetail.UTF8String : NULL;
    NSNumber *displayedPositionMs = [self displayedPositionNumber];
    NSNumber *latencyMs = [self latencyNumberForDisplayedPosition:displayedPositionMs];
    BOOL changed = self.lastEmittedActive != active ||
        self.lastEmittedAudioActive != audioActive ||
        self.lastEmittedMode != self.currentMode ||
        self.lastEmittedPhase != self.currentPhase ||
        self.lastEmittedStreamGeneration != self.streamGeneration ||
        self.lastEmittedDisplayedPositionMs != (displayedPositionMs != nil ? displayedPositionMs.longLongValue : -1) ||
        self.lastEmittedLatencyMs != (latencyMs != nil ? latencyMs.longLongValue : -1) ||
        !((self.lastEmittedDetail == nil && self.currentPhaseDetail == nil) ||
          [self.lastEmittedDetail isEqualToString:self.currentPhaseDetail]);

    if (!changed) {
        return;
    }

    self.lastEmittedActive = active;
    self.lastEmittedAudioActive = audioActive;
    self.lastEmittedMode = self.currentMode;
    self.lastEmittedPhase = self.currentPhase;
    self.lastEmittedStreamGeneration = self.streamGeneration;
    self.lastEmittedDisplayedPositionMs =
        displayedPositionMs != nil ? displayedPositionMs.longLongValue : -1;
    self.lastEmittedLatencyMs = latencyMs != nil ? latencyMs.longLongValue : -1;
    self.lastEmittedDetail = self.currentPhaseDetail;

    self.stateCallback(
        active,
        audioActive,
        NULL,
        (int)self.currentMode,
        (int)self.currentPhase,
        detail,
        displayedPositionMs != nil ? displayedPositionMs.longLongValue : -1,
        self.streamGeneration,
        latencyMs != nil ? latencyMs.longLongValue : -1
    );
}

- (BOOL)isCurrentItemReadyForRouting {
    AVPlayerItem *item = self.player.currentItem;
    return self.player != nil && item != nil && item.status == AVPlayerItemStatusReadyToPlay &&
        self.currentMode != OKAirPlayModeIdle;
}

- (BOOL)hasAudienceVideoRoute {
    return [self isCurrentItemReadyForRouting] && self.player.externalPlaybackActive;
}

- (BOOL)hasRemoteAudioRoute {
    // RATIONALE: HomePod-style audio-only AirPlay remains unsupported. Keep
    // local mute and output-state reporting aligned to the same audience/video
    // route fact source that already drives TV playback.
    return [self hasAudienceVideoRoute];
}

- (void)setPhase:(OKAirPlayPhase)phase detail:(NSString *)detail {
    BOOL changed = self.currentPhase != phase ||
        !((self.currentPhaseDetail == nil && detail == nil) ||
          [self.currentPhaseDetail isEqualToString:detail]);
    self.currentPhase = phase;
    self.currentPhaseDetail = detail.length > 0 ? [detail copy] : nil;

    if (changed) {
        NSLog(
            @"OpenKara AirPlay phase -> %ld (%@)",
            (long)phase,
            self.currentPhaseDetail ?: @"no-detail"
        );
    }
}

- (NSString *)playlistURLForCurrentGeneration {
    if (self.playlistURLString.length == 0) {
        return nil;
    }
    return [NSString stringWithFormat:@"%@?generation=%llu", self.playlistURLString, self.streamGeneration];
}

- (NSString *)initializationFilenameForStream:(OKAirPlayVariantStream *)stream {
    return [NSString stringWithFormat:@"%@-init-%llu.mp4", stream.filenamePrefix, self.streamGeneration];
}

- (NSString *)segmentFilenameForStream:(OKAirPlayVariantStream *)stream
                              sequence:(NSInteger)sequence {
    return [NSString stringWithFormat:@"%@-segment-%llu-%ld.m4s",
                                      stream.filenamePrefix,
                                      self.streamGeneration,
                                      (long)sequence];
}

- (BOOL)isStreamReadyForAttachment:(OKAirPlayVariantStream *)stream {
    return stream.hasInitializationSegment && stream.segments.count > 0 &&
        stream.hasWrittenAudio && (!stream.includesVideo || stream.hasWrittenVideo);
}

- (BOOL)isAudienceStreamReadyForAttachment {
    return [self isStreamReadyForAttachment:self.audienceStream];
}

- (void)resetOutputClockTracking {
    [self.syncPoints removeAllObjects];
    self.lastEmittedDisplayedPositionMs = -1;
    self.lastEmittedLatencyMs = -1;
}

- (void)resetMediaPipelineForGeneration:(uint64_t)generation {
    if (self.streamGeneration == generation) {
        return;
    }

    self.streamGeneration = generation;
    self.attachedGeneration = 0;
    [self.audienceStream reset];
    self.realItemAttached = NO;
    self.lastPlaybackErrorDetail = nil;
    [self.pendingAudienceAudioData setLength:0];
    self.pendingAudienceAudioOffset = 0;
    [self resetOutputClockTracking];

    if (self.player != nil) {
        [self.player replaceCurrentItemWithPlayerItem:nil];
        [self syncCurrentItemObservation];
    }

    dispatch_async(self.audioQueue, ^{
        [self.pendingAudienceAudioData setLength:0];
        self.pendingAudienceAudioOffset = 0;
    });
}

- (void)restartStreamForRouteActivation {
    [self resetMediaPipelineForGeneration:self.streamGeneration + 1];
}

- (void)recordSyncPointForSourcePosition:(long long)sourcePositionMs {
    long long streamPositionMs =
        llround((double)self.audienceStream.nextVideoFrameIndex * 1000.0 /
                (double)OKAirPlayFramesPerSecond);
    [self.syncPoints addObject:@{
        @"streamMs": @(streamPositionMs),
        @"sourceMs": @(sourcePositionMs),
        @"generation": @(self.streamGeneration),
    }];
    while (self.syncPoints.count > OKAirPlayMaxSyncPoints) {
        [self.syncPoints removeObjectAtIndex:0];
    }
}

- (NSNumber *)displayedPositionNumber {
    if (![self hasAudienceVideoRoute] || self.syncPoints.count == 0) {
        return nil;
    }

    long long streamPositionMs =
        llround(CMTimeGetSeconds(self.player.currentTime) * 1000.0);
    NSDictionary *anchor = nil;
    for (NSDictionary *point in [self.syncPoints reverseObjectEnumerator]) {
        if (OKLongLongValue(point[@"generation"]) != (long long)self.streamGeneration) {
            continue;
        }
        if (OKLongLongValue(point[@"streamMs"]) <= streamPositionMs) {
            anchor = point;
            break;
        }
    }
    if (anchor == nil) {
        anchor = self.syncPoints.lastObject;
    }
    if (anchor == nil) {
        return nil;
    }

    long long anchorStreamMs = OKLongLongValue(anchor[@"streamMs"]);
    long long anchorSourceMs = OKLongLongValue(anchor[@"sourceMs"]);
    return @(MAX(0, anchorSourceMs + (streamPositionMs - anchorStreamMs)));
}

- (NSDictionary *)currentRenderScene {
    NSDictionary *config = self.latestSceneConfig ?: @{};
    NSDictionary *runtime = self.latestRuntimeState ?: @{};
    if (config.count == 0 && runtime.count == 0) {
        return @{};
    }

    NSMutableDictionary *scene = [NSMutableDictionary dictionaryWithDictionary:config];
    [scene addEntriesFromDictionary:runtime];
    return scene;
}

- (NSString *)plainTextPaginationSignatureForScene:(NSDictionary *)scene {
    if (![scene isKindOfClass:[NSDictionary class]] || !OKBoolValue(scene[@"isPlainText"])) {
        return nil;
    }

    NSDictionary *signature = @{
        @"mode": OKStringValue(scene[@"mode"]) ?: @"",
        @"songId": OKStringValue(scene[@"songId"]) ?: @"",
        @"lyricsFontStep": @(OKIntegerValue(scene[@"lyricsFontStep"])),
        @"lines": OKArrayValue(scene[@"lines"]) ?: @[],
        @"viewport": OKDictionaryValue(scene[@"viewport"]) ?: @{},
        @"presentationSpec": OKDictionaryValue(scene[@"presentationSpec"]) ?: @{},
    };
    NSError *error = nil;
    NSData *signatureData =
        [NSJSONSerialization dataWithJSONObject:signature options:0 error:&error];
    if (signatureData == nil) {
        return nil;
    }
    return [[NSString alloc] initWithData:signatureData encoding:NSUTF8StringEncoding];
}

- (NSArray<NSNumber *> *)plainTextPageStartIndicesForLayouts:(NSArray<NSDictionary *> *)layouts
                                             availableHeight:(CGFloat)availableHeight
                                                         gap:(CGFloat)gap {
    if (layouts.count == 0) {
        return @[];
    }

    NSMutableArray<NSNumber *> *pageStartIndices = [NSMutableArray array];
    NSUInteger startIndex = 0;
    while (startIndex < layouts.count) {
        [pageStartIndices addObject:@(startIndex)];

        NSUInteger endIndex = startIndex;
        CGFloat usedHeight = 0.0;
        while (endIndex < layouts.count) {
            NSDictionary *layout = layouts[endIndex];
            CGFloat lineHeight = [layout[@"height"] doubleValue];
            CGFloat candidateHeight = usedHeight + lineHeight;
            if (endIndex > startIndex) {
                candidateHeight += gap;
            }

            if (candidateHeight > availableHeight) {
                if (endIndex == startIndex) {
                    endIndex += 1;
                }
                break;
            }

            usedHeight = candidateHeight;
            endIndex += 1;
        }

        if (endIndex == startIndex) {
            endIndex = startIndex + 1;
        }
        startIndex = endIndex;
    }

    return pageStartIndices;
}

- (void)refreshPlainTextPaginationForScene:(NSDictionary *)scene {
    NSString *signature = [self plainTextPaginationSignatureForScene:scene];
    if (signature.length == 0) {
        self.plainTextPageStartIndices = @[];
        self.plainTextPageIndex = 0;
        self.plainTextPaginationSignature = nil;
        return;
    }

    if ([signature isEqualToString:self.plainTextPaginationSignature]) {
        if (self.plainTextPageStartIndices.count == 0) {
            self.plainTextPageIndex = 0;
        } else if (self.plainTextPageIndex >= self.plainTextPageStartIndices.count) {
            self.plainTextPageIndex = self.plainTextPageStartIndices.count - 1;
        }
        return;
    }

    NSArray<NSDictionary *> *layouts = [self buildLyricLineLayoutsFromScene:scene];
    NSDictionary *presentationSpec = OKPresentationSpecValue(scene);
    CGFloat gap = OKSceneFloatValue(presentationSpec, @"lineGapPx", 40.0);
    CGFloat verticalPadding =
        OKSceneFloatValue(presentationSpec, @"verticalPaddingPx", 56.0);
    CGFloat availableHeight = OKAirPlayVideoHeight - (verticalPadding * 2.0);
    self.plainTextPageStartIndices = [self plainTextPageStartIndicesForLayouts:layouts
                                                               availableHeight:availableHeight
                                                                           gap:gap];
    self.plainTextPageIndex = 0;
    self.plainTextPaginationSignature = signature;
}

- (NSRange)plainTextPageRangeForLayouts:(NSArray<NSDictionary *> *)layouts {
    if (layouts.count == 0 || self.plainTextPageStartIndices.count == 0) {
        return NSMakeRange(0, 0);
    }

    NSUInteger pageIndex = MIN(self.plainTextPageIndex, self.plainTextPageStartIndices.count - 1);
    NSUInteger startIndex = [self.plainTextPageStartIndices[pageIndex] unsignedIntegerValue];
    NSUInteger endIndex = layouts.count;
    if (pageIndex + 1 < self.plainTextPageStartIndices.count) {
        endIndex = [self.plainTextPageStartIndices[pageIndex + 1] unsignedIntegerValue];
    }
    if (startIndex >= layouts.count || endIndex <= startIndex) {
        return NSMakeRange(0, 0);
    }
    return NSMakeRange(startIndex, endIndex - startIndex);
}

- (BOOL)stepPlainTextPageWithDirection:(NSInteger)direction {
    __block BOOL didChange = NO;
    void (^stepBlock)(void) = ^{
        NSDictionary *scene = [self currentRenderScene];
        if (![scene isKindOfClass:[NSDictionary class]] || !OKBoolValue(scene[@"isPlainText"])) {
            return;
        }

        [self refreshPlainTextPaginationForScene:scene];
        if (self.plainTextPageStartIndices.count == 0) {
            return;
        }

        NSInteger nextPageIndex = (NSInteger)self.plainTextPageIndex + direction;
        if (nextPageIndex < 0 ||
            nextPageIndex >= (NSInteger)self.plainTextPageStartIndices.count) {
            return;
        }

        self.plainTextPageIndex = (NSUInteger)nextPageIndex;
        didChange = YES;
    };

    if (dispatch_get_specific(OKAirPlayStateQueueSpecificKey) == OKAirPlayStateQueueSpecificKey) {
        stepBlock();
    } else {
        dispatch_sync(self.stateQueue, stepBlock);
    }

    return didChange;
}

- (NSNumber *)latencyNumberForDisplayedPosition:(NSNumber *)displayedPositionMs {
    if (displayedPositionMs == nil || self.latestRuntimeState == nil) {
        return nil;
    }

    long long sourcePositionMs = OKLongLongValue(self.latestRuntimeState[@"positionMs"]);
    return @(MAX(0, sourcePositionMs - displayedPositionMs.longLongValue));
}

- (NSString *)waitingDetail {
    if (!self.audienceStream.hasWrittenVideo) {
        return @"waiting_for_video";
    }
    if (!self.audienceStream.hasWrittenAudio) {
        return @"waiting_for_audio";
    }
    return @"waiting_for_route";
}

- (void)refreshPlaybackPhase {
    AVPlayerItem *item = self.player.currentItem;
    NSString *detail = nil;
    OKAirPlayPhase phase = OKAirPlayPhaseIdle;

    if (self.lastPlaybackErrorDetail.length > 0) {
        phase = OKAirPlayPhaseFailed;
        detail = self.lastPlaybackErrorDetail;
    } else if (self.audienceStream.writer.status == AVAssetWriterStatusFailed) {
        phase = OKAirPlayPhaseFailed;
        detail = @"audience_writer_failed";
    } else if (item != nil && item.status == AVPlayerItemStatusFailed) {
        phase = OKAirPlayPhaseFailed;
        detail = @"player_item_failed";
    } else if (self.currentMode == OKAirPlayModeIdle) {
        phase = OKAirPlayPhaseIdle;
    } else if (item == nil || item.status == AVPlayerItemStatusUnknown) {
        phase = OKAirPlayPhaseBuffering;
        detail = [self waitingDetail];
    } else if (![self isAudienceStreamReadyForAttachment]) {
        phase = OKAirPlayPhaseBuffering;
        detail = [self waitingDetail];
    } else if (item.status == AVPlayerItemStatusReadyToPlay && [self hasRemoteAudioRoute]) {
        phase = OKAirPlayPhasePlaying;
    } else if (item.status == AVPlayerItemStatusReadyToPlay) {
        phase = OKAirPlayPhaseBuffering;
        detail = @"waiting_for_route";
    }

    [self setPhase:phase detail:detail];
    [self emitState];
}

- (void)syncCurrentItemObservation {
    AVPlayerItem *item = self.player.currentItem;
    if (self.observedItem == item) {
        return;
    }

    if (self.observingCurrentItem && self.observedItem != nil) {
        [self.observedItem removeObserver:self forKeyPath:@"status"];
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:AVPlayerItemNewErrorLogEntryNotification
                                                      object:self.observedItem];
    }

    self.observedItem = item;
    self.observingCurrentItem = NO;
    self.lastPlaybackErrorDetail = nil;

    if (item != nil) {
        [item addObserver:self
               forKeyPath:@"status"
                  options:NSKeyValueObservingOptionNew
                  context:NULL];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handlePlayerItemErrorLogEntry:)
                                                     name:AVPlayerItemNewErrorLogEntryNotification
                                                   object:item];
        self.observingCurrentItem = YES;
    }
}

- (void)ensurePlayer {
    if (self.player != nil) {
        return;
    }

    self.player = [[AVPlayer alloc] init];
    self.player.allowsExternalPlayback = YES;
    self.player.actionAtItemEnd = AVPlayerActionAtItemEndNone;
    if (@available(macOS 10.15, *)) {
        self.player.automaticallyWaitsToMinimizeStalling = NO;
    }

    [self.player addObserver:self
                  forKeyPath:@"externalPlaybackActive"
                     options:NSKeyValueObservingOptionNew
                     context:NULL];
    [self.player addObserver:self
                  forKeyPath:@"timeControlStatus"
                     options:NSKeyValueObservingOptionNew
                     context:NULL];
    [self.player addObserver:self
                  forKeyPath:@"currentItem"
                     options:NSKeyValueObservingOptionNew
                     context:NULL];
    self.observingPlayer = YES;
}

- (void)attachPlayerItemIfReady {
    if (![self isAudienceStreamReadyForAttachment] || self.playlistURLString.length == 0) {
        return;
    }

    [self ensurePlayer];

    if (self.attachedGeneration == self.streamGeneration && self.realItemAttached) {
        return;
    }

    NSURL *playlistURL = [NSURL URLWithString:[self playlistURLForCurrentGeneration]];
    if (playlistURL == nil) {
        return;
    }

    AVPlayerItem *item = [AVPlayerItem playerItemWithURL:playlistURL];
    if (@available(macOS 10.15, *)) {
        // RATIONALE: Seeking on AirPlay has to discard stale HLS buffer. A
        // persistent route is fine, but a persistent item is not; a new item
        // per stream generation is what lets TV jump to the new timeline
        // without waiting through old buffered segments.
        item.preferredForwardBufferDuration = OKAirPlayPreferredForwardBufferDuration;
        item.canUseNetworkResourcesForLiveStreamingWhilePaused = YES;
    }
    [self.player replaceCurrentItemWithPlayerItem:item];
    [self syncCurrentItemObservation];
    [self.player play];
    self.realItemAttached = YES;
    self.attachedGeneration = self.streamGeneration;
    self.lastKnownExternalPlaybackActive = self.player.externalPlaybackActive;
    NSLog(@"OpenKara AirPlay attached player item for %@ (generation %llu)", playlistURL, self.streamGeneration);
    [self refreshPlaybackPhase];
}

- (void)removeOldSegmentFilesIfNeededForStream:(OKAirPlayVariantStream *)stream {
    while (stream.segments.count > OKAirPlayPlaylistWindow) {
        OKAirPlaySegmentEntry *entry = stream.segments.firstObject;
        [stream.segments removeObjectAtIndex:0];

        if (self.streamRootPath.length == 0) {
            continue;
        }

        NSString *path = [self.streamRootPath stringByAppendingPathComponent:entry.filename];
        [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
    }
}

- (void)writePlaylistFileForStream:(OKAirPlayVariantStream *)stream {
    if (self.streamRootPath.length == 0 || !stream.hasInitializationSegment) {
        return;
    }

    NSMutableString *playlist =
        [NSMutableString stringWithString:
                               @"#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-INDEPENDENT-SEGMENTS\n"];
    NSTimeInterval maxDuration = 1.0;
    NSInteger mediaSequence = stream.segments.firstObject.sequence;
    for (OKAirPlaySegmentEntry *entry in stream.segments) {
        maxDuration = MAX(maxDuration, entry.duration);
    }

    [playlist appendFormat:@"#EXT-X-TARGETDURATION:%ld\n", (long)ceil(maxDuration)];
    [playlist appendFormat:@"#EXT-X-MEDIA-SEQUENCE:%ld\n", (long)mediaSequence];
    [playlist appendFormat:@"#EXT-X-MAP:URI=\"%@\"\n", [self initializationFilenameForStream:stream]];
    for (OKAirPlaySegmentEntry *entry in stream.segments) {
        [playlist appendFormat:@"#EXTINF:%.3f,\n%@\n", entry.duration, entry.filename];
    }

    NSString *playlistPath =
        [self.streamRootPath stringByAppendingPathComponent:stream.playlistFilename];
    [playlist writeToFile:playlistPath atomically:YES encoding:NSUTF8StringEncoding error:nil];
}

- (void)resetStreamRoot {
    if (self.streamRootPath.length == 0) {
        return;
    }

    NSFileManager *fileManager = [NSFileManager defaultManager];
    [fileManager createDirectoryAtPath:self.streamRootPath
           withIntermediateDirectories:YES
                            attributes:nil
                                 error:nil];

    NSArray<NSString *> *existingFiles =
        [fileManager contentsOfDirectoryAtPath:self.streamRootPath error:nil];
    for (NSString *filename in existingFiles) {
        NSString *path = [self.streamRootPath stringByAppendingPathComponent:filename];
        [fileManager removeItemAtPath:path error:nil];
    }
}

- (void)handleWriterFailureIfNeeded:(NSString *)context {
    if (self.audienceStream.writer.status == AVAssetWriterStatusFailed) {
        NSLog(@"OpenKara AirPlay audience writer failed during %@: %@",
              context,
              self.audienceStream.writer.error);
    }
}

- (void)startVideoTimerIfNeeded {
    if (self.videoTimer != nil) {
        return;
    }

    dispatch_source_t timer =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, self.encoderQueue);
    dispatch_source_set_timer(
        timer,
        dispatch_walltime(NULL, 0),
        (uint64_t)(NSEC_PER_SEC / OKAirPlayFramesPerSecond),
        (uint64_t)(NSEC_PER_SEC / 240)
    );

    __weak typeof(self) weakSelf = self;
    dispatch_source_set_event_handler(timer, ^{
        [weakSelf appendMediaTick];
    });
    dispatch_resume(timer);
    self.videoTimer = timer;
}

- (void)configureStreamIfNeeded {
    if (@available(macOS 11.0, *)) {
        if (self.audienceStream.writer != nil || self.streamRootPath.length == 0) {
            return;
        }

        [self resetStreamRoot];

        NSArray<OKAirPlayVariantStream *> *streams = @[ self.audienceStream ];
        NSDictionary *audioSettings = @{
            AVFormatIDKey: @(kAudioFormatMPEG4AAC),
            AVSampleRateKey: @(OKAirPlayAudioSampleRate),
            AVEncoderBitRateKey: @(192000),
            AVNumberOfChannelsKey: @(OKAirPlayAudioChannels),
        };
        NSDictionary *videoCompressionProperties = @{
            AVVideoAverageBitRateKey: @(4 * 1024 * 1024),
            AVVideoExpectedSourceFrameRateKey: @(OKAirPlayFramesPerSecond),
            AVVideoMaxKeyFrameIntervalKey:
                @(OKAirPlayFramesPerSecond * OKAirPlayPreferredSegmentIntervalSeconds),
            AVVideoProfileLevelKey: AVVideoProfileLevelH264Main31,
        };
        NSDictionary *videoSettings = @{
            AVVideoCodecKey: AVVideoCodecTypeH264,
            AVVideoWidthKey: @(OKAirPlayVideoWidth),
            AVVideoHeightKey: @(OKAirPlayVideoHeight),
            AVVideoCompressionPropertiesKey: videoCompressionProperties,
        };
        NSDictionary *pixelBufferAttributes = @{
            (NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (NSString *)kCVPixelBufferWidthKey: @(OKAirPlayVideoWidth),
            (NSString *)kCVPixelBufferHeightKey: @(OKAirPlayVideoHeight),
            (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
            (NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
        };
        UTType *contentType = [UTType typeWithIdentifier:(NSString *)AVFileTypeMPEG4];

        for (OKAirPlayVariantStream *stream in streams) {
            NSError *error = nil;
            stream.writer = [[AVAssetWriter alloc] initWithContentType:contentType];
            stream.writer.delegate = self;
            stream.writer.shouldOptimizeForNetworkUse = YES;
            stream.writer.preferredOutputSegmentInterval =
                CMTimeMake(OKAirPlayPreferredSegmentIntervalSeconds, 1);
            stream.writer.initialSegmentStartTime = kCMTimeZero;
            stream.writer.outputFileTypeProfile = AVFileTypeProfileMPEG4AppleHLS;

            if (stream.includesVideo) {
                stream.videoInput =
                    [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo
                                                       outputSettings:videoSettings];
                stream.videoInput.expectsMediaDataInRealTime = YES;
                stream.pixelBufferAdaptor =
                    [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:stream.videoInput
                                                                                 sourcePixelBufferAttributes:pixelBufferAttributes];
                if ([stream.writer canAddInput:stream.videoInput]) {
                    [stream.writer addInput:stream.videoInput];
                }
            }

            stream.audioInput =
                [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio
                                                   outputSettings:audioSettings];
            stream.audioInput.expectsMediaDataInRealTime = YES;
            if ([stream.writer canAddInput:stream.audioInput]) {
                [stream.writer addInput:stream.audioInput];
            }

            if (![stream.writer startWriting]) {
                NSLog(@"OpenKara AirPlay failed to start %@ writer: %@",
                      stream.filenamePrefix,
                      error ?: stream.writer.error);
                return;
            }

            [stream.writer startSessionAtSourceTime:kCMTimeZero];
            stream.writerStarted = YES;
        }

        self.pendingAudienceAudioOffset = 0;
        [self.pendingAudienceAudioData setLength:0];
        self.realItemAttached = NO;
        self.lastPlaybackErrorDetail = nil;
        [self resetOutputClockTracking];
        [self startVideoTimerIfNeeded];
    }
}

- (void)compactPendingAudienceAudioIfNeeded {
    NSMutableData *pendingAudioData = self.pendingAudienceAudioData;
    NSUInteger pendingAudioOffset = self.pendingAudienceAudioOffset;

    if (pendingAudioOffset == 0) {
        return;
    }

    if (pendingAudioOffset < 64 * 1024 &&
        pendingAudioOffset < (pendingAudioData.length / 2)) {
        return;
    }

    NSData *remaining =
        [pendingAudioData subdataWithRange:NSMakeRange(
                             pendingAudioOffset,
                             pendingAudioData.length - pendingAudioOffset)];
    [pendingAudioData setData:remaining];
    self.pendingAudienceAudioOffset = 0;
}

- (NSData *)dequeueAudienceAudioFrames:(NSUInteger)frameCount {
    NSUInteger bytesPerFrame = sizeof(float) * OKAirPlayAudioChannels;
    NSUInteger requestedBytes = frameCount * bytesPerFrame;
    NSMutableData *output = [NSMutableData dataWithLength:requestedBytes];
    NSMutableData *pendingAudioData = self.pendingAudienceAudioData;
    NSUInteger pendingAudioOffset = self.pendingAudienceAudioOffset;

    NSUInteger availableBytes = pendingAudioData.length - pendingAudioOffset;
    NSUInteger consumedBytes = MIN(availableBytes, requestedBytes);
    if (consumedBytes > 0) {
        memcpy(
            output.mutableBytes,
            (uint8_t *)pendingAudioData.bytes + pendingAudioOffset,
            consumedBytes
        );
        pendingAudioOffset += consumedBytes;
        self.pendingAudienceAudioOffset = pendingAudioOffset;
        [self compactPendingAudienceAudioIfNeeded];
    }

    if (consumedBytes < requestedBytes) {
        memset((uint8_t *)output.mutableBytes + consumedBytes, 0, requestedBytes - consumedBytes);
    }

    return output;
}

- (CMFormatDescriptionRef)copyAudioFormatDescription {
    AudioStreamBasicDescription asbd = {0};
    asbd.mSampleRate = OKAirPlayAudioSampleRate;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags =
        kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
    asbd.mBytesPerPacket = sizeof(float) * OKAirPlayAudioChannels;
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = sizeof(float) * OKAirPlayAudioChannels;
    asbd.mChannelsPerFrame = OKAirPlayAudioChannels;
    asbd.mBitsPerChannel = sizeof(float) * 8;

    CMFormatDescriptionRef formatDescription = NULL;
    OSStatus status = CMAudioFormatDescriptionCreate(
        kCFAllocatorDefault,
        &asbd,
        0,
        NULL,
        0,
        NULL,
        NULL,
        &formatDescription
    );
    if (status != noErr) {
        return NULL;
    }

    return formatDescription;
}

- (void)appendAudienceAudioTickForStream:(OKAirPlayVariantStream *)stream {
    if (!stream.writerStarted || stream.audioInput == nil || !stream.audioInput.readyForMoreMediaData) {
        return;
    }

    __block NSData *audioChunk = nil;
    dispatch_sync(self.audioQueue, ^{
        audioChunk = [self dequeueAudienceAudioFrames:OKAirPlayAudioFramesPerTick];
    });
    if (audioChunk.length == 0) {
        return;
    }

    CMBlockBufferRef blockBuffer = NULL;
    size_t blockLength = audioChunk.length;
    void *memory = malloc(blockLength);
    if (memory == NULL) {
        return;
    }
    memcpy(memory, audioChunk.bytes, blockLength);

    OSStatus blockStatus = CMBlockBufferCreateWithMemoryBlock(
        kCFAllocatorDefault,
        memory,
        blockLength,
        kCFAllocatorMalloc,
        NULL,
        0,
        blockLength,
        0,
        &blockBuffer
    );
    if (blockStatus != noErr || blockBuffer == NULL) {
        free(memory);
        return;
    }

    CMFormatDescriptionRef formatDescription = [self copyAudioFormatDescription];
    if (formatDescription == NULL) {
        CFRelease(blockBuffer);
        return;
    }

    CMSampleBufferRef sampleBuffer = NULL;
    CMTime presentationTime =
        CMTimeMake(stream.nextAudioFrameIndex, OKAirPlayAudioSampleRate);
    OSStatus sampleStatus = CMAudioSampleBufferCreateReadyWithPacketDescriptions(
        kCFAllocatorDefault,
        blockBuffer,
        formatDescription,
        OKAirPlayAudioFramesPerTick,
        presentationTime,
        NULL,
        &sampleBuffer
    );

    CFRelease(formatDescription);
    CFRelease(blockBuffer);

    if (sampleStatus != noErr || sampleBuffer == NULL) {
        return;
    }

    if (![stream.audioInput appendSampleBuffer:sampleBuffer]) {
        NSLog(@"OpenKara AirPlay failed to append %@ audio sample buffer: %@",
              stream.filenamePrefix,
              stream.writer.error);
    } else {
        stream.hasWrittenAudio = YES;
        stream.nextAudioFrameIndex += OKAirPlayAudioFramesPerTick;
    }

    CFRelease(sampleBuffer);
}

- (void)drawCenteredText:(NSString *)text
                fontSize:(CGFloat)fontSize
                   color:(id)color
               inContext:(CGContextRef)context
                 atPoint:(CGPoint)point {
    NSDictionary *attributes = OKBaseTextAttributes(fontSize, color, 1.0);
    NSAttributedString *attributed =
        [[NSAttributedString alloc] initWithString:text ?: @"" attributes:attributes];
    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attributed);
    if (line == NULL) {
        return;
    }

    CGFloat ascent = 0.0;
    CGFloat descent = 0.0;
    CGFloat leading = 0.0;
    double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
    CGContextSetTextPosition(context, point.x - (CGFloat)width * 0.5, point.y + ascent * 0.5);
    CTLineDraw(line, context);
    CFRelease(line);
}

- (void)drawStatusSceneInContext:(CGContextRef)context message:(NSString *)message {
    NSDictionary *presentationSpec = OKPresentationSpecValue([self currentRenderScene]);
    NSDictionary *statusColor = OKColorDictionaryValue(
        presentationSpec,
        @"statusTextColor",
        142.0 / 255.0,
        142.0 / 255.0,
        147.0 / 255.0,
        1.0
    );
    // RATIONALE: AirPlay empty/loading states are passive TV hints, not
    // interactive UI. Keep them visually small so they don't dominate the
    // audience screen or regress back to a large "background info window".
    [self drawCenteredText:message
                  fontSize:OKSceneFloatValue(presentationSpec, @"statusFontSizePx", 18.0)
                     color:OKColorObjectFromDictionary(statusColor)
                 inContext:context
                   atPoint:CGPointMake(OKAirPlayVideoWidth * 0.5, OKAirPlayVideoHeight * 0.5)];
}

- (void)drawNoLyricsSceneInContext:(CGContextRef)context messages:(NSDictionary *)messages {
    NSString *noLyrics =
        OKStringValue(messages[@"noLyrics"]) ?: @"No lyrics available for this track";
    [self drawStatusSceneInContext:context message:noLyrics];
}

- (NSArray<NSDictionary *> *)buildLyricLineLayoutsFromScene:(NSDictionary *)scene {
    NSArray *lines = OKArrayValue(scene[@"lines"]);
    if (![lines isKindOfClass:[NSArray class]] || lines.count == 0) {
        return @[];
    }

    long long adjustedMs = OKLongLongValue(scene[@"adjustedMs"]);
    NSInteger activeLineIndex = OKIntegerValue(scene[@"activeLineIndex"]);
    NSInteger lyricsFontStep = OKIntegerValue(scene[@"lyricsFontStep"]);
    BOOL isPlainText = OKBoolValue(scene[@"isPlainText"]);
    NSDictionary *presentationSpec = OKPresentationSpecValue(scene);
    CGFloat baseSize =
        OKSceneFloatValue(presentationSpec, @"fontSizePx", OKFontSizeForStep(lyricsFontStep));
    CGFloat lineHeightMultiple =
        OKSceneFloatValue(presentationSpec, @"lineHeightMultiple", 1.08);
    CGFloat activeScale = OKSceneFloatValue(presentationSpec, @"activeScale", 1.05);
    CGFloat horizontalPadding =
        OKSceneFloatValue(presentationSpec, @"horizontalPaddingPx", 64.0);
    CGFloat contentWidthRatio =
        OKSceneFloatValue(presentationSpec, @"contentWidthRatio", 0.92);
    CGFloat contentMaxWidth =
        OKSceneFloatValue(presentationSpec, @"contentMaxWidthPx", 1600.0);
    CGFloat contentWidth = MIN(
        MIN((CGFloat)OKAirPlayVideoWidth * contentWidthRatio, contentMaxWidth),
        (CGFloat)OKAirPlayVideoWidth - horizontalPadding * 2.0
    );
    NSDictionary *activeTextColor = OKColorDictionaryValue(
        presentationSpec,
        @"activeTextColor",
        1.0,
        1.0,
        1.0,
        1.0
    );
    NSDictionary *pastTextColor = OKColorDictionaryValue(
        presentationSpec,
        @"pastTextColor",
        72.0 / 255.0,
        72.0 / 255.0,
        74.0 / 255.0,
        1.0
    );
    NSDictionary *futureTextColor = OKColorDictionaryValue(
        presentationSpec,
        @"futureTextColor",
        58.0 / 255.0,
        58.0 / 255.0,
        60.0 / 255.0,
        1.0
    );
    NSDictionary *plainTextColor = OKColorDictionaryValue(
        presentationSpec,
        @"plainTextColor",
        1.0,
        1.0,
        1.0,
        1.0
    );
    NSMutableArray<NSDictionary *> *layouts = [NSMutableArray arrayWithCapacity:lines.count];

    for (NSUInteger index = 0; index < lines.count; index += 1) {
        NSDictionary *line = OKDictionaryValue(lines[index]);
        if (line == nil) {
            continue;
        }
        NSString *state = @"future";
        if (isPlainText) {
            state = @"plain";
        } else if ((NSInteger)index == activeLineIndex) {
            state = @"active";
        } else if ((NSInteger)index < activeLineIndex) {
            state = @"past";
        }

        CGFloat fontSize = [state isEqualToString:@"active"] ? baseSize * activeScale : baseSize;
        NSMutableAttributedString *text = [[NSMutableAttributedString alloc] init];
        NSArray *words = OKArrayValue(line[@"words"]);
        BOOL hasWords = [words isKindOfClass:[NSArray class]] && words.count > 0;

        if (hasWords) {
            NSInteger activeWordIndex =
                [state isEqualToString:@"active"] ? OKActiveWordIndex(words, adjustedMs) : -1;
            for (NSUInteger wordIndex = 0; wordIndex < words.count; wordIndex += 1) {
                NSDictionary *word = OKDictionaryValue(words[wordIndex]);
                if (word == nil) {
                    continue;
                }
                NSString *wordText = OKStringValue(word[@"text"]) ?: @"";
                NSString *separator = wordIndex + 1 < words.count ? @" " : @"";
                NSString *segmentText = [wordText stringByAppendingString:separator];

                NSDictionary *color = activeTextColor;
                if ([state isEqualToString:@"past"]) {
                    color = pastTextColor;
                } else if ([state isEqualToString:@"future"]) {
                    color = futureTextColor;
                } else if ([state isEqualToString:@"plain"]) {
                    color = plainTextColor;
                } else if ([state isEqualToString:@"active"]) {
                    if ((NSInteger)wordIndex < activeWordIndex) {
                        color = pastTextColor;
                    } else if ((NSInteger)wordIndex == activeWordIndex) {
                        color = activeTextColor;
                    } else {
                        color = futureTextColor;
                    }
                }

                NSDictionary *attributes = OKBaseTextAttributes(
                    fontSize,
                    OKColorObjectFromDictionary(color),
                    lineHeightMultiple
                );
                [text appendAttributedString:[[NSAttributedString alloc] initWithString:segmentText
                                                                             attributes:attributes]];
            }
        } else {
            NSString *lineText = OKStringValue(line[@"text"]) ?: @"";
            NSDictionary *color =
                [state isEqualToString:@"past"] ? pastTextColor :
                [state isEqualToString:@"future"] ? futureTextColor :
                [state isEqualToString:@"plain"] ? plainTextColor : activeTextColor;

            NSDictionary *attributes = OKBaseTextAttributes(
                fontSize,
                OKColorObjectFromDictionary(color),
                lineHeightMultiple
            );
            [text appendAttributedString:[[NSAttributedString alloc] initWithString:lineText
                                                                         attributes:attributes]];
        }

        CTFramesetterRef framesetter =
            CTFramesetterCreateWithAttributedString((__bridge CFAttributedStringRef)text);
        if (framesetter == NULL) {
            continue;
        }
        CGSize frameSize = CTFramesetterSuggestFrameSizeWithConstraints(
            framesetter,
            CFRangeMake(0, 0),
            NULL,
            CGSizeMake(contentWidth, CGFLOAT_MAX),
            NULL
        );
        CFRelease(framesetter);
        [layouts addObject:@{
            @"text": text,
            @"width": @(contentWidth),
            @"height": @(MAX(ceil(frameSize.height), ceil(fontSize * lineHeightMultiple))),
            @"state": state,
        }];
    }
    return layouts;
}

- (void)drawLyricsSceneInContext:(CGContextRef)context scene:(NSDictionary *)scene {
    NSString *songId = OKStringValue(scene[@"songId"]);
    NSDictionary *messages = OKDictionaryValue(scene[@"messages"]);
    NSArray *lines = OKArrayValue(scene[@"lines"]);
    BOOL isLoading = OKBoolValue(scene[@"isLoading"]);

    if (songId.length == 0) {
        [self drawStatusSceneInContext:context
                               message:OKStringValue(messages[@"selectSong"]) ?: @"Select a song to start"];
        return;
    }

    if (isLoading) {
        [self drawStatusSceneInContext:context
                               message:OKStringValue(messages[@"loadingLyrics"]) ?: @"Loading lyrics..."];
        return;
    }

    if (![lines isKindOfClass:[NSArray class]] || lines.count == 0) {
        [self drawNoLyricsSceneInContext:context messages:messages ?: @{}];
        return;
    }

    NSArray<NSDictionary *> *layouts = [self buildLyricLineLayoutsFromScene:scene];
    if (layouts.count == 0) {
        return;
    }

    NSDictionary *presentationSpec = OKPresentationSpecValue(scene);
    CGFloat gap = OKSceneFloatValue(presentationSpec, @"lineGapPx", 40.0);
    CGFloat verticalPadding =
        OKSceneFloatValue(presentationSpec, @"verticalPaddingPx", 56.0);
    NSDictionary *activeGlowColor = OKColorDictionaryValue(
        presentationSpec,
        @"activeGlowColor",
        1.0,
        1.0,
        1.0,
        0.8
    );
    CGFloat activeGlowBlur =
        OKSceneFloatValue(presentationSpec, @"activeGlowBlurPx", 12.0);
    BOOL isPlainText = OKBoolValue(scene[@"isPlainText"]);
    NSInteger activeLineIndex = OKIntegerValue(scene[@"activeLineIndex"]);
    CGFloat availableHeight = OKAirPlayVideoHeight - (verticalPadding * 2.0);
    CGFloat totalHeight = 0.0;
    for (NSDictionary *layout in layouts) {
        totalHeight += [layout[@"height"] doubleValue];
    }
    totalHeight += gap * MAX((NSInteger)layouts.count - 1, 0);
    CGFloat y = verticalPadding + MAX(0.0, (availableHeight - totalHeight) * 0.5);
    NSArray<NSDictionary *> *renderLayouts = layouts;
    if (isPlainText) {
        NSRange pageRange = [self plainTextPageRangeForLayouts:layouts];
        if (pageRange.length == 0) {
            return;
        }
        renderLayouts = [layouts subarrayWithRange:pageRange];
        y = verticalPadding;
    } else if (activeLineIndex >= 0 && activeLineIndex < (NSInteger)layouts.count) {
        CGFloat activeCenterWithinContent = 0.0;
        for (NSInteger index = 0; index < activeLineIndex; index += 1) {
            activeCenterWithinContent += [layouts[index][@"height"] doubleValue] + gap;
        }
        activeCenterWithinContent += [layouts[activeLineIndex][@"height"] doubleValue] * 0.5;

        CGFloat desiredY = verticalPadding + availableHeight * 0.5 - activeCenterWithinContent;
        CGFloat minY = OKAirPlayVideoHeight - verticalPadding - totalHeight;
        y = OKClamp(desiredY, minY, verticalPadding);
    }

    for (NSDictionary *layout in renderLayouts) {
        NSAttributedString *text = layout[@"text"];
        CGFloat width = [layout[@"width"] doubleValue];
        CGFloat height = [layout[@"height"] doubleValue];
        NSString *state = layout[@"state"];
        CGRect frameRect =
            OKTopAlignedRect((OKAirPlayVideoWidth - width) * 0.5, y, width, height);
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathAddRect(path, NULL, frameRect);
        CTFramesetterRef framesetter =
            CTFramesetterCreateWithAttributedString((__bridge CFAttributedStringRef)text);
        CTFrameRef frame =
            framesetter == NULL ? NULL : CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);

        CGContextSaveGState(context);
        if ([state isEqualToString:@"active"] && frame != NULL) {
            CGContextSetShadowWithColor(
                context,
                CGSizeZero,
                activeGlowBlur,
                (__bridge CGColorRef)OKColorObjectFromDictionary(activeGlowColor)
            );
        }
        if (frame != NULL) {
            CTFrameDraw(frame, context);
        }
        CGContextRestoreGState(context);
        if (frame != NULL) {
            CFRelease(frame);
        }
        if (framesetter != NULL) {
            CFRelease(framesetter);
        }
        CGPathRelease(path);

        y += height + gap;
    }
}

- (void)drawCDGFrameInContext:(CGContextRef)context {
    if (self.latestCdgFrame.length != 288 * 192 * 4) {
        return;
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider =
        CGDataProviderCreateWithCFData((__bridge CFDataRef)self.latestCdgFrame);
    CGImageRef image = CGImageCreate(
        288,
        192,
        8,
        32,
        288 * 4,
        colorSpace,
        kCGImageAlphaLast | kCGBitmapByteOrderDefault,
        provider,
        NULL,
        false,
        kCGRenderingIntentDefault
    );
    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(provider);

    if (image == NULL) {
        return;
    }

    CGRect drawRect = OKAspectFitRect(
        CGSizeMake(288.0, 192.0),
        CGRectMake(0.0, 0.0, OKAirPlayVideoWidth, OKAirPlayVideoHeight)
    );
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextDrawImage(context, drawRect, image);
    CGImageRelease(image);
}

- (CVPixelBufferRef)copyRenderedPixelBufferForScene:(NSDictionary *)scene {
    if (self.audienceStream.pixelBufferAdaptor.pixelBufferPool == NULL) {
        return NULL;
    }

    CVPixelBufferRef pixelBuffer = NULL;
    CVReturn result =
        CVPixelBufferPoolCreatePixelBuffer(
            kCFAllocatorDefault,
            self.audienceStream.pixelBufferAdaptor.pixelBufferPool,
            &pixelBuffer
        );
    if (result != kCVReturnSuccess || pixelBuffer == NULL) {
        return NULL;
    }

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    void *baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        baseAddress,
        OKAirPlayVideoWidth,
        OKAirPlayVideoHeight,
        8,
        bytesPerRow,
        colorSpace,
        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst
    );
    CGColorSpaceRelease(colorSpace);

    if (context == NULL) {
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        CFRelease(pixelBuffer);
        return NULL;
    }

    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    OKSetFillColor(context, 0.0, 0.0, 0.0, 1.0);
    CGContextFillRect(context, CGRectMake(0, 0, OKAirPlayVideoWidth, OKAirPlayVideoHeight));

    if (self.currentMode == OKAirPlayModeCdg) {
        [self drawCDGFrameInContext:context];
    } else {
        [self drawLyricsSceneInContext:context scene:scene];
    }

    CGContextRelease(context);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    return pixelBuffer;
}

- (void)appendVideoTick {
    if (!self.audienceStream.writerStarted ||
        self.audienceStream.videoInput == nil ||
        !self.audienceStream.videoInput.readyForMoreMediaData) {
        return;
    }

    NSDictionary *scene = [self currentRenderScene];
    CVPixelBufferRef pixelBuffer = [self copyRenderedPixelBufferForScene:scene];
    if (pixelBuffer == NULL) {
        return;
    }

    CMTime presentationTime =
        CMTimeMake(self.audienceStream.nextVideoFrameIndex, OKAirPlayFramesPerSecond);
    BOOL appended =
        [self.audienceStream.pixelBufferAdaptor appendPixelBuffer:pixelBuffer
                                            withPresentationTime:presentationTime];
    CFRelease(pixelBuffer);

    if (!appended) {
        NSLog(@"OpenKara AirPlay failed to append video frame: %@",
              self.audienceStream.writer.error);
        return;
    }

    if (scene.count > 0) {
        [self recordSyncPointForSourcePosition:OKLongLongValue(scene[@"positionMs"])];
    }
    self.audienceStream.hasWrittenVideo = YES;
    self.audienceStream.nextVideoFrameIndex += 1;
}

- (void)appendMediaTick {
    @autoreleasepool {
        [self configureStreamIfNeeded];
        if (!self.audienceStream.writerStarted) {
            return;
        }

        [self appendVideoTick];
        [self appendAudienceAudioTickForStream:self.audienceStream];
        [self handleWriterFailureIfNeeded:@"media tick"];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self refreshPlaybackPhase];
        });
    }
}

- (void)syncRoutePickerForRootView:(NSView *)rootView
                              left:(CGFloat)left
                               top:(CGFloat)top
                             width:(CGFloat)width
                            height:(CGFloat)height
                           mounted:(BOOL)mounted
                    streamRootPath:(NSString *)streamRootPath
                       playlistURL:(NSString *)playlistURL {
    if (streamRootPath.length > 0) {
        self.streamRootPath = streamRootPath;
    }
    if (playlistURL.length > 0) {
        self.playlistURLString = playlistURL;
    }

    if (!mounted || rootView == nil) {
        [self.routePickerView removeFromSuperview];
        return;
    }

    [self ensurePlayer];
    dispatch_async(self.encoderQueue, ^{
        [self configureStreamIfNeeded];
    });

    CGFloat hostHeight = NSHeight(rootView.bounds);
    NSRect frame = NSMakeRect(left, hostHeight - top - height, width, height);

    if (self.routePickerView == nil) {
        self.routePickerView = [[AVRoutePickerView alloc] initWithFrame:frame];
        self.routePickerView.routePickerButtonBordered = NO;
        self.routePickerView.player = self.player;
    } else {
        self.routePickerView.frame = frame;
        self.routePickerView.player = self.player;
    }

    if (self.routePickerView.superview != rootView) {
        [self.routePickerView removeFromSuperview];
        [rootView addSubview:self.routePickerView];
    }
}

- (void)syncAudienceConfigWithJSON:(NSString *)configJSON {
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.stateQueue, ^{
        NSDictionary *config = nil;
        if (configJSON.length > 0) {
            NSData *jsonData = [configJSON dataUsingEncoding:NSUTF8StringEncoding];
            config = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:nil];
        }
        weakSelf.latestSceneConfig = [config isKindOfClass:[NSDictionary class]] ? config : nil;
        [weakSelf refreshPlainTextPaginationForScene:[weakSelf currentRenderScene]];
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf refreshPlaybackPhase];
        });
    });
}

- (void)syncAudienceRuntimeWithJSON:(NSString *)runtimeJSON
                           cdgFrame:(NSData *)cdgFrame {
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.stateQueue, ^{
        NSDictionary *runtime = nil;
        if (runtimeJSON.length > 0) {
            NSData *jsonData = [runtimeJSON dataUsingEncoding:NSUTF8StringEncoding];
            runtime = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:nil];
        }

        NSDictionary *runtimeState =
            [runtime isKindOfClass:[NSDictionary class]] ? runtime : nil;
        weakSelf.latestRuntimeState = runtimeState;
        weakSelf.latestCdgFrame = [cdgFrame copy];
        weakSelf.currentMode =
            runtimeState != nil ? OKModeValue(runtimeState[@"mode"]) : OKAirPlayModeIdle;
        [weakSelf refreshPlainTextPaginationForScene:[weakSelf currentRenderScene]];
        uint64_t runtimeGeneration =
            (uint64_t)MAX(1, OKLongLongValue(runtimeState[@"streamGeneration"]));

        if (weakSelf.currentMode == OKAirPlayModeIdle) {
            dispatch_async(weakSelf.audioQueue, ^{
                [weakSelf.pendingAudienceAudioData setLength:0];
                weakSelf.pendingAudienceAudioOffset = 0;
            });
        }

        dispatch_async(weakSelf.encoderQueue, ^{
            uint64_t streamGeneration = MAX(weakSelf.streamGeneration, runtimeGeneration);
            [weakSelf resetMediaPipelineForGeneration:streamGeneration];
            dispatch_async(dispatch_get_main_queue(), ^{
                [weakSelf refreshPlaybackPhase];
            });
        });
    });
}

- (void)pushAudioSamples:(const float *)samples
             sampleCount:(NSUInteger)sampleCount
              sampleRate:(uint32_t)sampleRate
                channels:(uint16_t)channels
                   epoch:(uint64_t)epoch {
    if (samples == NULL || sampleCount == 0 || channels == 0) {
        return;
    }

    NSUInteger inputFrameCount = sampleCount / channels;
    NSMutableData *stereo = [NSMutableData dataWithLength:inputFrameCount * OKAirPlayAudioChannels * sizeof(float)];
    float *stereoSamples = stereo.mutableBytes;
    for (NSUInteger frame = 0; frame < inputFrameCount; frame += 1) {
        stereoSamples[frame * OKAirPlayAudioChannels] = samples[frame * channels];
        stereoSamples[frame * OKAirPlayAudioChannels + 1] =
            channels > 1 ? samples[frame * channels + 1] : samples[frame * channels];
    }

    NSData *resampled = OKResampleStereoPCM(stereo.bytes, inputFrameCount, sampleRate);
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.audioQueue, ^{
        if (epoch < weakSelf.currentAudioEpoch) {
            return;
        }
        if (weakSelf.currentMode == OKAirPlayModeIdle) {
            return;
        }

        [weakSelf.pendingAudienceAudioData appendData:resampled];
        NSUInteger maxBytes = OKAirPlayAudioSampleRate * OKAirPlayAudioChannels * sizeof(float);
        if (weakSelf.pendingAudienceAudioData.length > maxBytes) {
            NSUInteger overflow =
                weakSelf.pendingAudienceAudioData.length - maxBytes;
            weakSelf.pendingAudienceAudioOffset =
                MIN(weakSelf.pendingAudienceAudioOffset + overflow,
                    weakSelf.pendingAudienceAudioData.length);
            [weakSelf compactPendingAudienceAudioIfNeeded];
        }
    });
}

- (void)applyAudioEpoch:(uint64_t)epoch {
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.audioQueue, ^{
        weakSelf.currentAudioEpoch = MAX(weakSelf.currentAudioEpoch, epoch);
        [weakSelf.pendingAudienceAudioData setLength:0];
        weakSelf.pendingAudienceAudioOffset = 0;
    });
}

- (void)handlePlayerItemErrorLogEntry:(NSNotification *)notification {
    AVPlayerItem *item = notification.object;
    AVPlayerItemErrorLogEvent *lastEvent = item.errorLog.events.lastObject;
    self.lastPlaybackErrorDetail =
        lastEvent.errorComment ?: lastEvent.serverAddress ?: @"player_item_error_log";
    [self refreshPlaybackPhase];
}

- (void)assetWriter:(AVAssetWriter *)writer
 didOutputSegmentData:(NSData *)segmentData
         segmentType:(AVAssetSegmentType)segmentType
       segmentReport:(AVAssetSegmentReport *)segmentReport API_AVAILABLE(macos(11.0)) {
    if (self.streamRootPath.length == 0) {
        return;
    }

    OKAirPlayVariantStream *stream =
        writer == self.audienceStream.writer ? self.audienceStream : nil;
    if (stream == nil) {
        return;
    }

    if (segmentType == AVAssetSegmentTypeInitialization) {
        NSString *initPath =
            [self.streamRootPath stringByAppendingPathComponent:[self initializationFilenameForStream:stream]];
        [segmentData writeToFile:initPath atomically:YES];
        stream.hasInitializationSegment = YES;
        [self writePlaylistFileForStream:stream];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self attachPlayerItemIfReady];
        });
        return;
    }

    NSTimeInterval duration = 1.0;
    for (AVAssetSegmentTrackReport *track in segmentReport.trackReports) {
        if (CMTIME_IS_NUMERIC(track.duration)) {
            duration = MAX(duration, CMTimeGetSeconds(track.duration));
        }
    }

    OKAirPlaySegmentEntry *entry = [[OKAirPlaySegmentEntry alloc] init];
    entry.sequence = stream.nextSegmentSequence;
    entry.duration = duration;
    entry.filename = [self segmentFilenameForStream:stream sequence:entry.sequence];
    stream.nextSegmentSequence += 1;

    NSString *segmentPath = [self.streamRootPath stringByAppendingPathComponent:entry.filename];
    [segmentData writeToFile:segmentPath atomically:YES];
    [stream.segments addObject:entry];
    [self removeOldSegmentFilesIfNeededForStream:stream];
    [self writePlaylistFileForStream:stream];

    dispatch_async(dispatch_get_main_queue(), ^{
        [self attachPlayerItemIfReady];
    });
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                       change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
    if ([keyPath isEqualToString:@"externalPlaybackActive"] && object == self.player) {
        BOOL isAudienceVideoRouteActive = [self hasAudienceVideoRoute];
        BOOL didActivateAudienceRoute =
            isAudienceVideoRouteActive && !self.lastKnownExternalPlaybackActive &&
            self.currentMode != OKAirPlayModeIdle;
        self.lastKnownExternalPlaybackActive = isAudienceVideoRouteActive;

        if (didActivateAudienceRoute) {
            dispatch_async(self.encoderQueue, ^{
                [self restartStreamForRouteActivation];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self refreshPlaybackPhase];
                });
            });
            return;
        }
        [self refreshPlaybackPhase];
        return;
    }

    if ([keyPath isEqualToString:@"timeControlStatus"] && object == self.player) {
        [self refreshPlaybackPhase];
        return;
    }

    if ([keyPath isEqualToString:@"currentItem"] && object == self.player) {
        [self syncCurrentItemObservation];
        [self refreshPlaybackPhase];
        return;
    }

    if ([keyPath isEqualToString:@"status"] && object == self.observedItem) {
        if (self.observedItem.status != AVPlayerItemStatusFailed) {
            self.lastPlaybackErrorDetail = nil;
        }
        [self refreshPlaybackPhase];
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
    ok_airplay_dispatch_main(^{
        [OKAirPlayBridge sharedBridge].stateCallback = callback;
        [[OKAirPlayBridge sharedBridge] refreshPlaybackPhase];
    });
}

void ok_airplay_sync_route_picker(
    void *ns_view_ptr,
    double left,
    double top,
    double width,
    double height,
    bool mounted,
    const char *stream_root_path,
    const char *playlist_url
) {
    NSString *streamRootPath =
        stream_root_path == NULL ? nil : [NSString stringWithUTF8String:stream_root_path];
    NSString *playlistURL = playlist_url == NULL ? nil : [NSString stringWithUTF8String:playlist_url];

    ok_airplay_dispatch_main(^{
        NSView *rootView = (__bridge NSView *)ns_view_ptr;
        [[OKAirPlayBridge sharedBridge] syncRoutePickerForRootView:rootView
                                                              left:(CGFloat)left
                                                               top:(CGFloat)top
                                                             width:(CGFloat)width
                                                            height:(CGFloat)height
                                                           mounted:mounted
                                                    streamRootPath:streamRootPath
                                                       playlistURL:playlistURL];
    });
}

void ok_airplay_sync_audience_state(const char *config_json) {
    NSString *configJSON = config_json == NULL ? nil : [NSString stringWithUTF8String:config_json];

    [[OKAirPlayBridge sharedBridge] syncAudienceConfigWithJSON:configJSON];
}

void ok_airplay_sync_audience_runtime(
    const char *runtime_json,
    const uint8_t *cdg_frame_ptr,
    size_t cdg_frame_len
) {
    NSString *runtimeJSON = runtime_json == NULL ? nil : [NSString stringWithUTF8String:runtime_json];
    NSData *cdgFrame =
        (cdg_frame_ptr == NULL || cdg_frame_len == 0) ? nil : [NSData dataWithBytes:cdg_frame_ptr length:cdg_frame_len];

    [[OKAirPlayBridge sharedBridge] syncAudienceRuntimeWithJSON:runtimeJSON
                                                       cdgFrame:cdgFrame];
}

bool ok_airplay_step_plain_text_page(int direction) {
    return [[OKAirPlayBridge sharedBridge] stepPlainTextPageWithDirection:(NSInteger)direction];
}

void ok_airplay_push_audio_samples(
    const float *samples,
    size_t sample_count,
    uint32_t sample_rate,
    uint16_t channels,
    uint64_t epoch
) {
    [[OKAirPlayBridge sharedBridge] pushAudioSamples:samples
                                         sampleCount:sample_count
                                          sampleRate:sample_rate
                                            channels:channels
                                               epoch:epoch];
}

void ok_airplay_set_audio_epoch(uint64_t epoch) {
    [[OKAirPlayBridge sharedBridge] applyAudioEpoch:epoch];
}

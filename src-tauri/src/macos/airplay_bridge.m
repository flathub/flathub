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

typedef void (*OKAirPlayStateCallback)(bool active, const char *route_name, int mode_tag);

typedef NS_ENUM(NSInteger, OKAirPlayMode) {
    OKAirPlayModeIdle = 0,
    OKAirPlayModeLyrics = 1,
    OKAirPlayModeCdg = 2,
};

static const NSInteger OKAirPlayVideoWidth = 1280;
static const NSInteger OKAirPlayVideoHeight = 720;
static const NSInteger OKAirPlayFramesPerSecond = 30;
static const NSInteger OKAirPlayAudioSampleRate = 44100;
static const NSInteger OKAirPlayAudioChannels = 2;
static const NSInteger OKAirPlayAudioFramesPerTick =
    OKAirPlayAudioSampleRate / OKAirPlayFramesPerSecond;
static const NSInteger OKAirPlayPlaylistWindow = 3;

@interface OKAirPlaySegmentEntry : NSObject

@property(nonatomic, assign) NSInteger sequence;
@property(nonatomic, copy) NSString *filename;
@property(nonatomic, assign) NSTimeInterval duration;

@end

@implementation OKAirPlaySegmentEntry
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

static BOOL OKSceneIsPlainText(NSArray *lines) {
    if (lines.count == 0) {
        return NO;
    }

    for (NSDictionary *line in lines) {
        NSNumber *timeMs = line[@"timeMs"];
        if (timeMs == nil || timeMs.longLongValue != 0) {
            return NO;
        }
    }

    return YES;
}

static NSInteger OKActiveWordIndex(NSArray *words, long long adjustedMs) {
    NSInteger activeIndex = -1;
    for (NSUInteger index = 0; index < words.count; index += 1) {
        NSDictionary *word = words[index];
        NSNumber *timeMs = word[@"timeMs"];
        if (timeMs != nil && timeMs.longLongValue > adjustedMs) {
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

static id OKBridgedColor(CGFloat r, CGFloat g, CGFloat b, CGFloat a) {
    return CFBridgingRelease(CGColorCreateGenericRGB(r, g, b, a));
}

static void OKSetFillColor(CGContextRef context, CGFloat r, CGFloat g, CGFloat b, CGFloat a) {
    CGContextSetRGBFillColor(context, r, g, b, a);
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

static NSDictionary *OKBaseTextAttributes(CGFloat fontSize, id color) {
    CTFontRef font = OKCreateBoldSystemFont(fontSize);
    NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
    if (font != NULL) {
        attributes[(id)kCTFontAttributeName] = (__bridge id)font;
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

@property(nonatomic, strong) dispatch_queue_t mediaQueue;
@property(nonatomic, strong) dispatch_source_t videoTimer;

@property(nonatomic, copy) NSString *streamRootPath;
@property(nonatomic, copy) NSString *playlistURLString;

@property(nonatomic, strong) AVAssetWriter *writer;
@property(nonatomic, strong) AVAssetWriterInput *videoInput;
@property(nonatomic, strong) AVAssetWriterInput *audioInput;
@property(nonatomic, strong) AVAssetWriterInputPixelBufferAdaptor *pixelBufferAdaptor;
@property(nonatomic, assign) BOOL writerStarted;
@property(nonatomic, assign) BOOL realItemAttached;
@property(nonatomic, assign) BOOL hasInitializationSegment;

@property(nonatomic, strong) NSMutableArray<OKAirPlaySegmentEntry *> *segments;
@property(nonatomic, assign) NSInteger nextSegmentSequence;
@property(nonatomic, assign) int64_t nextVideoFrameIndex;
@property(nonatomic, assign) int64_t nextAudioFrameIndex;

@property(nonatomic, strong) NSMutableData *pendingAudioData;
@property(nonatomic, assign) NSUInteger pendingAudioOffset;
@property(nonatomic, assign) uint64_t audioEpoch;

@property(nonatomic, assign) OKAirPlayMode currentMode;
@property(nonatomic, strong) NSDictionary *latestScene;
@property(nonatomic, strong) NSData *latestCdgFrame;

+ (instancetype)sharedBridge;
- (void)syncRoutePickerForRootView:(NSView *)rootView
                              left:(CGFloat)left
                               top:(CGFloat)top
                             width:(CGFloat)width
                            height:(CGFloat)height
                           mounted:(BOOL)mounted
                    streamRootPath:(NSString *)streamRootPath
                       playlistURL:(NSString *)playlistURL;
- (void)syncAudienceStateWithMode:(OKAirPlayMode)mode
                        sceneJSON:(NSString *)sceneJSON
                         cdgFrame:(NSData *)cdgFrame;
- (void)pushAudioSamples:(const float *)samples
             sampleCount:(NSUInteger)sampleCount
              sampleRate:(uint32_t)sampleRate
                channels:(uint16_t)channels
                   epoch:(uint64_t)epoch;
- (void)applyAudioEpoch:(uint64_t)epoch;

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

    _mediaQueue = dispatch_queue_create("openkara.airplay.media", DISPATCH_QUEUE_SERIAL);
    _segments = [NSMutableArray array];
    _pendingAudioData = [NSMutableData data];
    _currentMode = OKAirPlayModeIdle;
    _audioEpoch = 1;
    return self;
}

- (void)dealloc {
    if (self.observingPlayer) {
        [self.player removeObserver:self forKeyPath:@"externalPlaybackActive"];
    }
}

- (void)emitState {
    if (self.stateCallback == NULL) {
        return;
    }

    BOOL active = self.realItemAttached && self.player != nil && self.player.externalPlaybackActive;
    self.stateCallback(active, NULL, (int)self.currentMode);
}

- (void)ensurePlayer {
    if (self.player != nil) {
        return;
    }

    self.player = [[AVPlayer alloc] init];
    self.player.allowsExternalPlayback = YES;
    self.player.muted = YES;
    self.player.actionAtItemEnd = AVPlayerActionAtItemEndNone;
    if (@available(macOS 10.15, *)) {
        self.player.automaticallyWaitsToMinimizeStalling = YES;
    }

    [self.player addObserver:self
                  forKeyPath:@"externalPlaybackActive"
                     options:NSKeyValueObservingOptionNew
                     context:NULL];
    self.observingPlayer = YES;
}

- (void)attachPlayerItemIfReady {
    if (self.realItemAttached || !self.hasInitializationSegment || self.segments.count == 0 ||
        self.playlistURLString.length == 0) {
        return;
    }

    [self ensurePlayer];

    NSURL *playlistURL = [NSURL URLWithString:self.playlistURLString];
    if (playlistURL == nil) {
        return;
    }

    AVPlayerItem *item = [AVPlayerItem playerItemWithURL:playlistURL];
    if (@available(macOS 10.15, *)) {
        item.preferredForwardBufferDuration = 1.0;
        item.canUseNetworkResourcesForLiveStreamingWhilePaused = YES;
    }
    [self.player replaceCurrentItemWithPlayerItem:item];
    self.player.muted = !self.player.externalPlaybackActive;
    [self.player play];
    self.realItemAttached = YES;
    [self emitState];
}

- (void)removeOldSegmentFilesIfNeeded {
    while (self.segments.count > OKAirPlayPlaylistWindow) {
        OKAirPlaySegmentEntry *entry = self.segments.firstObject;
        [self.segments removeObjectAtIndex:0];

        if (self.streamRootPath.length == 0) {
            continue;
        }

        NSString *path = [self.streamRootPath stringByAppendingPathComponent:entry.filename];
        [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
    }
}

- (void)writePlaylistFile {
    if (self.streamRootPath.length == 0 || !self.hasInitializationSegment) {
        return;
    }

    NSMutableString *playlist = [NSMutableString stringWithString:@"#EXTM3U\n#EXT-X-VERSION:7\n"];
    NSTimeInterval maxDuration = 1.0;
    NSInteger mediaSequence = self.segments.firstObject.sequence;
    for (OKAirPlaySegmentEntry *entry in self.segments) {
        maxDuration = MAX(maxDuration, entry.duration);
    }

    [playlist appendFormat:@"#EXT-X-TARGETDURATION:%ld\n", (long)ceil(maxDuration)];
    [playlist appendFormat:@"#EXT-X-MEDIA-SEQUENCE:%ld\n", (long)mediaSequence];
    [playlist appendString:@"#EXT-X-MAP:URI=\"init.mp4\"\n"];
    for (OKAirPlaySegmentEntry *entry in self.segments) {
        [playlist appendFormat:@"#EXTINF:%.3f,\n%@\n", entry.duration, entry.filename];
    }

    NSString *playlistPath = [self.streamRootPath stringByAppendingPathComponent:@"playlist.m3u8"];
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
    if (self.writer.status == AVAssetWriterStatusFailed) {
        NSLog(@"OpenKara AirPlay writer failed during %@: %@", context, self.writer.error);
    }
}

- (void)startVideoTimerIfNeeded {
    if (self.videoTimer != nil) {
        return;
    }

    dispatch_source_t timer =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, self.mediaQueue);
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
        if (self.writer != nil || self.streamRootPath.length == 0) {
            return;
        }

        [self resetStreamRoot];

        NSError *error = nil;
        UTType *contentType = [UTType typeWithIdentifier:(NSString *)AVFileTypeMPEG4];
        self.writer = [[AVAssetWriter alloc] initWithContentType:contentType];
        self.writer.delegate = self;
        self.writer.shouldOptimizeForNetworkUse = YES;
        self.writer.preferredOutputSegmentInterval = CMTimeMake(1, 1);
        self.writer.initialSegmentStartTime = kCMTimeZero;
        self.writer.outputFileTypeProfile = AVFileTypeProfileMPEG4AppleHLS;

        NSDictionary *videoCompressionProperties = @{
            AVVideoAverageBitRateKey: @(4 * 1024 * 1024),
            AVVideoExpectedSourceFrameRateKey: @(OKAirPlayFramesPerSecond),
            AVVideoMaxKeyFrameIntervalKey: @(OKAirPlayFramesPerSecond),
            AVVideoProfileLevelKey: AVVideoProfileLevelH264Main31,
        };
        NSDictionary *videoSettings = @{
            AVVideoCodecKey: AVVideoCodecTypeH264,
            AVVideoWidthKey: @(OKAirPlayVideoWidth),
            AVVideoHeightKey: @(OKAirPlayVideoHeight),
            AVVideoCompressionPropertiesKey: videoCompressionProperties,
        };

        self.videoInput =
            [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo
                                               outputSettings:videoSettings];
        self.videoInput.expectsMediaDataInRealTime = YES;

        NSDictionary *pixelBufferAttributes = @{
            (NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (NSString *)kCVPixelBufferWidthKey: @(OKAirPlayVideoWidth),
            (NSString *)kCVPixelBufferHeightKey: @(OKAirPlayVideoHeight),
            (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
            (NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
        };
        self.pixelBufferAdaptor =
            [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:self.videoInput
                                                                             sourcePixelBufferAttributes:pixelBufferAttributes];

        NSDictionary *audioSettings = @{
            AVFormatIDKey: @(kAudioFormatMPEG4AAC),
            AVSampleRateKey: @(OKAirPlayAudioSampleRate),
            AVEncoderBitRateKey: @(192000),
            AVNumberOfChannelsKey: @(OKAirPlayAudioChannels),
        };
        self.audioInput =
            [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio
                                               outputSettings:audioSettings];
        self.audioInput.expectsMediaDataInRealTime = YES;

        if ([self.writer canAddInput:self.videoInput]) {
            [self.writer addInput:self.videoInput];
        }
        if ([self.writer canAddInput:self.audioInput]) {
            [self.writer addInput:self.audioInput];
        }

        self.segments = [NSMutableArray array];
        self.nextSegmentSequence = 0;
        self.nextVideoFrameIndex = 0;
        self.nextAudioFrameIndex = 0;
        self.pendingAudioOffset = 0;
        [self.pendingAudioData setLength:0];
        self.realItemAttached = NO;
        self.hasInitializationSegment = NO;

        if (![self.writer startWriting]) {
            NSLog(@"OpenKara AirPlay failed to start writer: %@", error ?: self.writer.error);
            return;
        }
        [self.writer startSessionAtSourceTime:kCMTimeZero];
        self.writerStarted = YES;
        [self startVideoTimerIfNeeded];
    }
}

- (void)compactPendingAudioIfNeeded {
    if (self.pendingAudioOffset == 0) {
        return;
    }

    if (self.pendingAudioOffset < 64 * 1024 &&
        self.pendingAudioOffset < (self.pendingAudioData.length / 2)) {
        return;
    }

    NSData *remaining =
        [self.pendingAudioData subdataWithRange:NSMakeRange(
                                    self.pendingAudioOffset,
                                    self.pendingAudioData.length - self.pendingAudioOffset)];
    [self.pendingAudioData setData:remaining];
    self.pendingAudioOffset = 0;
}

- (NSData *)dequeueAudioFrames:(NSUInteger)frameCount {
    NSUInteger bytesPerFrame = sizeof(float) * OKAirPlayAudioChannels;
    NSUInteger requestedBytes = frameCount * bytesPerFrame;
    NSMutableData *output = [NSMutableData dataWithLength:requestedBytes];

    NSUInteger availableBytes = self.pendingAudioData.length - self.pendingAudioOffset;
    NSUInteger consumedBytes = MIN(availableBytes, requestedBytes);
    if (consumedBytes > 0) {
        memcpy(
            output.mutableBytes,
            (uint8_t *)self.pendingAudioData.bytes + self.pendingAudioOffset,
            consumedBytes
        );
        self.pendingAudioOffset += consumedBytes;
        [self compactPendingAudioIfNeeded];
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

- (void)appendAudioTick {
    if (!self.writerStarted || self.audioInput == nil || !self.audioInput.readyForMoreMediaData) {
        return;
    }

    NSData *audioChunk = [self dequeueAudioFrames:OKAirPlayAudioFramesPerTick];
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
    CMTime presentationTime = CMTimeMake(self.nextAudioFrameIndex, OKAirPlayAudioSampleRate);
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

    if (![self.audioInput appendSampleBuffer:sampleBuffer]) {
        NSLog(@"OpenKara AirPlay failed to append audio sample buffer: %@", self.writer.error);
    } else {
        self.nextAudioFrameIndex += OKAirPlayAudioFramesPerTick;
    }

    CFRelease(sampleBuffer);
}

- (void)drawCenteredText:(NSString *)text
                fontSize:(CGFloat)fontSize
                   color:(id)color
               inContext:(CGContextRef)context
                 atPoint:(CGPoint)point {
    NSDictionary *attributes = OKBaseTextAttributes(fontSize, color);
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
    [self drawCenteredText:message
                  fontSize:28.0
                     color:OKBridgedColor(0.56, 0.56, 0.58, 1.0)
                 inContext:context
                   atPoint:CGPointMake(OKAirPlayVideoWidth * 0.5, OKAirPlayVideoHeight * 0.5)];
}

- (void)drawNoLyricsSceneInContext:(CGContextRef)context messages:(NSDictionary *)messages {
    NSString *noLyrics = messages[@"noLyrics"] ?: @"No lyrics available for this track";
    NSString *addLyrics = messages[@"addLyrics"] ?: @"Add Lyrics";

    [self drawCenteredText:noLyrics
                  fontSize:28.0
                     color:OKBridgedColor(0.56, 0.56, 0.58, 1.0)
                 inContext:context
                   atPoint:CGPointMake(OKAirPlayVideoWidth * 0.5, OKAirPlayVideoHeight * 0.5 - 26.0)];

    CGFloat buttonWidth = MAX(160.0, addLyrics.length * 20.0);
    CGRect buttonRect = CGRectMake(
        (OKAirPlayVideoWidth - buttonWidth) * 0.5,
        OKAirPlayVideoHeight * 0.5 + 14.0,
        buttonWidth,
        48.0
    );
    CGPathRef path = CGPathCreateWithRoundedRect(buttonRect, 10.0, 10.0, NULL);
    CGContextSaveGState(context);
    CGContextAddPath(context, path);
    OKSetFillColor(context, 0.17, 0.17, 0.18, 1.0);
    CGContextFillPath(context);
    CGContextRestoreGState(context);
    CGPathRelease(path);

    [self drawCenteredText:addLyrics
                  fontSize:22.0
                     color:OKBridgedColor(0.92, 0.92, 0.96, 1.0)
                 inContext:context
                   atPoint:CGPointMake(CGRectGetMidX(buttonRect), CGRectGetMidY(buttonRect) - 5.0)];
}

- (NSArray<NSDictionary *> *)buildLyricLineLayoutsFromScene:(NSDictionary *)scene {
    NSArray *lines = scene[@"lines"];
    if (![lines isKindOfClass:[NSArray class]] || lines.count == 0) {
        return @[];
    }

    long long positionMs = [scene[@"positionMs"] longLongValue];
    long long offsetMs = [scene[@"offsetMs"] longLongValue];
    long long adjustedMs = positionMs - offsetMs;
    NSInteger activeLineIndex = [scene[@"activeLineIndex"] integerValue];
    NSInteger lyricsFontStep = [scene[@"lyricsFontStep"] integerValue];
    BOOL isPlainText = OKSceneIsPlainText(lines);

    CGFloat baseSize = OKFontSizeForStep(lyricsFontStep);
    NSMutableArray<NSDictionary *> *layouts = [NSMutableArray arrayWithCapacity:lines.count];

    for (NSUInteger index = 0; index < lines.count; index += 1) {
        NSDictionary *line = lines[index];
        NSString *state = @"future";
        if (isPlainText) {
            state = @"plain";
        } else if ((NSInteger)index == activeLineIndex) {
            state = @"active";
        } else if ((NSInteger)index < activeLineIndex) {
            state = @"past";
        }

        CGFloat fontSize = [state isEqualToString:@"active"] ? baseSize * 1.05 : baseSize;
        NSMutableAttributedString *text = [[NSMutableAttributedString alloc] init];
        NSArray *words = line[@"words"];
        BOOL hasWords = [words isKindOfClass:[NSArray class]] && words.count > 0;

        if (hasWords) {
            NSInteger activeWordIndex =
                [state isEqualToString:@"active"] ? OKActiveWordIndex(words, adjustedMs) : -1;
            for (NSUInteger wordIndex = 0; wordIndex < words.count; wordIndex += 1) {
                NSDictionary *word = words[wordIndex];
                NSString *wordText = word[@"text"] ?: @"";
                NSString *separator = wordIndex + 1 < words.count ? @" " : @"";
                NSString *segmentText = [wordText stringByAppendingString:separator];

                id color = OKBridgedColor(1.0, 1.0, 1.0, 1.0);
                if ([state isEqualToString:@"past"]) {
                    color = OKBridgedColor(0.28, 0.28, 0.29, 1.0);
                } else if ([state isEqualToString:@"future"]) {
                    color = OKBridgedColor(0.23, 0.23, 0.24, 1.0);
                } else if ([state isEqualToString:@"active"]) {
                    if ((NSInteger)wordIndex < activeWordIndex) {
                        color = OKBridgedColor(0.28, 0.28, 0.29, 1.0);
                    } else if ((NSInteger)wordIndex == activeWordIndex) {
                        color = OKBridgedColor(1.0, 1.0, 1.0, 1.0);
                    } else {
                        color = OKBridgedColor(0.23, 0.23, 0.24, 1.0);
                    }
                }

                NSDictionary *attributes = OKBaseTextAttributes(fontSize, color);
                [text appendAttributedString:[[NSAttributedString alloc] initWithString:segmentText
                                                                             attributes:attributes]];
            }
        } else {
            NSString *lineText = line[@"text"] ?: @"";
            id color = OKBridgedColor(1.0, 1.0, 1.0, 1.0);
            if ([state isEqualToString:@"past"]) {
                color = OKBridgedColor(0.28, 0.28, 0.29, 1.0);
            } else if ([state isEqualToString:@"future"]) {
                color = OKBridgedColor(0.23, 0.23, 0.24, 1.0);
            }

            NSDictionary *attributes = OKBaseTextAttributes(fontSize, color);
            [text appendAttributedString:[[NSAttributedString alloc] initWithString:lineText
                                                                         attributes:attributes]];
        }

        CTLineRef ctLine = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)text);
        if (ctLine == NULL) {
            continue;
        }

        CGFloat ascent = 0.0;
        CGFloat descent = 0.0;
        CGFloat leading = 0.0;
        CGFloat width = (CGFloat)CTLineGetTypographicBounds(ctLine, &ascent, &descent, &leading);
        [layouts addObject:@{
            @"line": CFBridgingRelease(ctLine),
            @"width": @(width),
            @"ascent": @(ascent),
            @"descent": @(descent),
            @"leading": @(leading),
            @"height": @(ascent + descent + leading),
            @"state": state,
        }];
    }
    return layouts;
}

- (void)drawLyricsSceneInContext:(CGContextRef)context scene:(NSDictionary *)scene {
    NSString *songId = scene[@"songId"];
    NSDictionary *messages = scene[@"messages"];
    NSArray *lines = scene[@"lines"];
    BOOL isLoading = [scene[@"isLoading"] boolValue];

    if (songId.length == 0) {
        [self drawStatusSceneInContext:context
                               message:messages[@"selectSong"] ?: @"Select a song to start"];
        return;
    }

    if (isLoading) {
        [self drawStatusSceneInContext:context
                               message:messages[@"loadingLyrics"] ?: @"Loading lyrics..."];
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

    CGFloat gap = 34.0;
    CGFloat totalHeight = 0.0;
    for (NSDictionary *layout in layouts) {
        totalHeight += [layout[@"height"] doubleValue];
    }
    totalHeight += gap * MAX((NSInteger)layouts.count - 1, 0);

    CGFloat y = (OKAirPlayVideoHeight - totalHeight) * 0.5;
    for (NSDictionary *layout in layouts) {
        CTLineRef line = (__bridge CTLineRef)layout[@"line"];
        CGFloat width = [layout[@"width"] doubleValue];
        CGFloat ascent = [layout[@"ascent"] doubleValue];
        CGFloat height = [layout[@"height"] doubleValue];
        NSString *state = layout[@"state"];

        CGContextSaveGState(context);
        if ([state isEqualToString:@"active"]) {
            CGContextSetShadowWithColor(
                context,
                CGSizeZero,
                12.0,
                (__bridge CGColorRef)OKBridgedColor(1.0, 1.0, 1.0, 0.45)
            );
        }
        CGContextSetTextPosition(context, (OKAirPlayVideoWidth - width) * 0.5, y + ascent);
        CTLineDraw(line, context);
        CGContextRestoreGState(context);

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

- (CVPixelBufferRef)copyRenderedPixelBuffer {
    if (self.pixelBufferAdaptor.pixelBufferPool == NULL) {
        return NULL;
    }

    CVPixelBufferRef pixelBuffer = NULL;
    CVReturn result =
        CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, self.pixelBufferAdaptor.pixelBufferPool, &pixelBuffer);
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

    CGContextTranslateCTM(context, 0, OKAirPlayVideoHeight);
    CGContextScaleCTM(context, 1.0, -1.0);
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    OKSetFillColor(context, 0.0, 0.0, 0.0, 1.0);
    CGContextFillRect(context, CGRectMake(0, 0, OKAirPlayVideoWidth, OKAirPlayVideoHeight));

    NSDictionary *scene = self.latestScene ?: @{};
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
    if (!self.writerStarted || self.videoInput == nil || !self.videoInput.readyForMoreMediaData) {
        return;
    }

    CVPixelBufferRef pixelBuffer = [self copyRenderedPixelBuffer];
    if (pixelBuffer == NULL) {
        return;
    }

    CMTime presentationTime = CMTimeMake(self.nextVideoFrameIndex, OKAirPlayFramesPerSecond);
    BOOL appended = [self.pixelBufferAdaptor appendPixelBuffer:pixelBuffer withPresentationTime:presentationTime];
    CFRelease(pixelBuffer);

    if (!appended) {
        NSLog(@"OpenKara AirPlay failed to append video frame: %@", self.writer.error);
        return;
    }

    self.nextVideoFrameIndex += 1;
}

- (void)appendMediaTick {
    @autoreleasepool {
        [self configureStreamIfNeeded];
        if (!self.writerStarted) {
            return;
        }

        [self appendVideoTick];
        [self appendAudioTick];
        [self handleWriterFailureIfNeeded:@"media tick"];
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
    dispatch_async(self.mediaQueue, ^{
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

- (void)syncAudienceStateWithMode:(OKAirPlayMode)mode
                        sceneJSON:(NSString *)sceneJSON
                         cdgFrame:(NSData *)cdgFrame {
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.mediaQueue, ^{
        weakSelf.currentMode = mode;
        if (sceneJSON.length > 0) {
            NSData *jsonData = [sceneJSON dataUsingEncoding:NSUTF8StringEncoding];
            NSDictionary *scene = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:nil];
            if ([scene isKindOfClass:[NSDictionary class]]) {
                weakSelf.latestScene = scene;
            }
        }
        if (cdgFrame != nil) {
            weakSelf.latestCdgFrame = [cdgFrame copy];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf emitState];
        });
    });
}

- (void)pushAudioSamples:(const float *)samples
             sampleCount:(NSUInteger)sampleCount
              sampleRate:(uint32_t)sampleRate
                channels:(uint16_t)channels
                   epoch:(uint64_t)epoch {
    if (samples == NULL || sampleCount == 0 || channels < OKAirPlayAudioChannels) {
        return;
    }

    NSUInteger inputFrameCount = sampleCount / channels;
    NSMutableData *stereo = [NSMutableData dataWithLength:inputFrameCount * OKAirPlayAudioChannels * sizeof(float)];
    float *stereoSamples = stereo.mutableBytes;
    for (NSUInteger frame = 0; frame < inputFrameCount; frame += 1) {
        stereoSamples[frame * OKAirPlayAudioChannels] = samples[frame * channels];
        stereoSamples[frame * OKAirPlayAudioChannels + 1] = samples[frame * channels + 1];
    }

    NSData *resampled = OKResampleStereoPCM(stereo.bytes, inputFrameCount, sampleRate);
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.mediaQueue, ^{
        if (epoch < weakSelf.audioEpoch) {
            return;
        }

        [weakSelf.pendingAudioData appendData:resampled];
        NSUInteger maxBytes = OKAirPlayAudioSampleRate * OKAirPlayAudioChannels * sizeof(float) * 4;
        if (weakSelf.pendingAudioData.length > maxBytes) {
            NSUInteger overflow = weakSelf.pendingAudioData.length - maxBytes;
            weakSelf.pendingAudioOffset = MIN(weakSelf.pendingAudioOffset + overflow, weakSelf.pendingAudioData.length);
            [weakSelf compactPendingAudioIfNeeded];
        }
    });
}

- (void)applyAudioEpoch:(uint64_t)epoch {
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.mediaQueue, ^{
        weakSelf.audioEpoch = MAX(weakSelf.audioEpoch, epoch);
        [weakSelf.pendingAudioData setLength:0];
        weakSelf.pendingAudioOffset = 0;
    });
}

- (void)assetWriter:(AVAssetWriter *)writer
 didOutputSegmentData:(NSData *)segmentData
         segmentType:(AVAssetSegmentType)segmentType
       segmentReport:(AVAssetSegmentReport *)segmentReport API_AVAILABLE(macos(11.0)) {
    if (writer != self.writer || self.streamRootPath.length == 0) {
        return;
    }

    if (segmentType == AVAssetSegmentTypeInitialization) {
        NSString *initPath = [self.streamRootPath stringByAppendingPathComponent:@"init.mp4"];
        [segmentData writeToFile:initPath atomically:YES];
        self.hasInitializationSegment = YES;
        [self writePlaylistFile];
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
    entry.sequence = self.nextSegmentSequence;
    entry.duration = duration;
    entry.filename = [NSString stringWithFormat:@"segment-%ld.m4s", (long)entry.sequence];
    self.nextSegmentSequence += 1;

    NSString *segmentPath = [self.streamRootPath stringByAppendingPathComponent:entry.filename];
    [segmentData writeToFile:segmentPath atomically:YES];
    [self.segments addObject:entry];
    [self removeOldSegmentFilesIfNeeded];
    [self writePlaylistFile];

    dispatch_async(dispatch_get_main_queue(), ^{
        [self attachPlayerItemIfReady];
    });
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
    if ([keyPath isEqualToString:@"externalPlaybackActive"] && object == self.player) {
        self.player.muted = !self.player.externalPlaybackActive;
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
    ok_airplay_dispatch_main(^{
        [OKAirPlayBridge sharedBridge].stateCallback = callback;
        [[OKAirPlayBridge sharedBridge] emitState];
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

void ok_airplay_sync_audience_state(
    int mode,
    const char *scene_json,
    const uint8_t *cdg_frame_ptr,
    size_t cdg_frame_len
) {
    NSString *sceneJSON = scene_json == NULL ? nil : [NSString stringWithUTF8String:scene_json];
    NSData *cdgFrame =
        (cdg_frame_ptr == NULL || cdg_frame_len == 0) ? nil : [NSData dataWithBytes:cdg_frame_ptr length:cdg_frame_len];

    [[OKAirPlayBridge sharedBridge] syncAudienceStateWithMode:(OKAirPlayMode)mode
                                                    sceneJSON:sceneJSON
                                                     cdgFrame:cdgFrame];
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

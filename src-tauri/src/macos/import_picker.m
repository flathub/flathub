#import <AppKit/AppKit.h>
#import <dispatch/dispatch.h>
#import <stdlib.h>
#import <string.h>

static void openkara_run_on_main_thread_sync(dispatch_block_t block) {
    if (block == nil) {
        return;
    }

    // RATIONALE: Tauri commands may already enter this picker on the main thread.
    // Sync-dispatching back onto the main queue from there crashes immediately.
    if ([NSThread isMainThread]) {
        block();
        return;
    }

    dispatch_sync(dispatch_get_main_queue(), block);
}

static NSArray<NSURL *> *openkara_run_import_panel(const char *defaultPath) {
    __block NSArray<NSURL *> *selectedURLs = nil;

    openkara_run_on_main_thread_sync(^{
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = YES;
        panel.allowsMultipleSelection = YES;
        panel.resolvesAliases = YES;

        if (defaultPath != NULL && defaultPath[0] != '\0') {
            NSString *path = [NSString stringWithUTF8String:defaultPath];
            if (path != nil) {
                panel.directoryURL = [NSURL fileURLWithPath:path isDirectory:YES];
            }
        }

        if ([panel runModal] == NSModalResponseOK) {
            selectedURLs = [[panel URLs] copy];
        }
    });

    return selectedURLs;
}

char **openkara_pick_import_paths(const char *defaultPath, uintptr_t *countOut) {
    if (countOut == NULL) {
        return NULL;
    }

    NSArray<NSURL *> *selectedURLs = openkara_run_import_panel(defaultPath);

    if (selectedURLs == nil || selectedURLs.count == 0) {
        *countOut = 0;
        return NULL;
    }

    char **paths = calloc(selectedURLs.count, sizeof(char *));
    if (paths == NULL) {
        *countOut = 0;
        return NULL;
    }

    NSUInteger written = 0;
    for (NSURL *url in selectedURLs) {
        const char *fileSystemPath = [[url path] fileSystemRepresentation];
        if (fileSystemPath == NULL) {
            continue;
        }

        size_t length = strlen(fileSystemPath);
        char *copiedPath = malloc(length + 1);
        if (copiedPath == NULL) {
            continue;
        }

        memcpy(copiedPath, fileSystemPath, length + 1);
        paths[written] = copiedPath;
        written += 1;
    }

    if (written == 0) {
        free(paths);
        *countOut = 0;
        return NULL;
    }

    *countOut = written;
    return paths;
}

void openkara_free_import_paths(char **paths, uintptr_t count) {
    if (paths == NULL) {
        return;
    }

    for (uintptr_t index = 0; index < count; index += 1) {
        free(paths[index]);
    }

    free(paths);
}

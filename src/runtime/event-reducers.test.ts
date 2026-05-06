import { describe, expect, test, vi } from "vitest";
import {
  createBatchSeparationClearScheduler,
  createStatusClearScheduler,
  separationErrorStatus,
  separationProgressStatus,
  uploadCompleteStatus,
  uploadErrorStatus,
  uploadProgressStatus,
} from "./event-reducers";

const error = {
  code: "internal" as const,
  message: "failed",
  retryable: true,
  fallback: "retry" as const,
};

describe("event reducers", () => {
  test("maps separation events to status snapshots", () => {
    expect(
      separationProgressStatus({ song_id: "song-1", percent: 42 }),
    ).toMatchObject({
      song_id: "song-1",
      state: "running",
      percent: 42,
      error: null,
    });

    expect(separationErrorStatus({ song_id: "song-1", error })).toMatchObject({
      song_id: "song-1",
      state: "failed",
      percent: 0,
      error,
    });
  });

  test("maps upload events to status snapshots", () => {
    expect(
      uploadProgressStatus({
        song_id: "song-1",
        percent: 35,
        remote_library_id: "remote-1",
        detail: "Uploading",
      }),
    ).toEqual({
      song_id: "song-1",
      state: "running",
      percent: 35,
      remote_library_id: "remote-1",
      detail: "Uploading",
      error: null,
    });

    expect(
      uploadCompleteStatus({
        song_id: "song-1",
        remote_library_id: "remote-1",
      }),
    ).toMatchObject({
      song_id: "song-1",
      state: "completed",
      percent: 100,
      detail: null,
      error: null,
    });

    expect(
      uploadErrorStatus({
        song_id: "song-1",
        remote_library_id: "remote-1",
        error,
      }),
    ).toMatchObject({
      song_id: "song-1",
      state: "failed",
      percent: 0,
      error,
    });
  });

  test("schedules and cancels keyed status clears", () => {
    vi.useFakeTimers();
    const clear = vi.fn();
    const scheduler = createStatusClearScheduler<string>(clear, 3000);

    scheduler.schedule("song-1");
    scheduler.schedule("song-1");
    vi.advanceTimersByTime(2999);
    expect(clear).not.toHaveBeenCalled();

    vi.advanceTimersByTime(1);
    expect(clear).toHaveBeenCalledTimes(1);
    expect(clear).toHaveBeenCalledWith("song-1");

    scheduler.schedule("song-2");
    scheduler.cancel("song-2");
    vi.advanceTimersByTime(3000);
    expect(clear).toHaveBeenCalledTimes(1);
    vi.useRealTimers();
  });

  test("batch scheduler clears terminal progress after delay", () => {
    vi.useFakeTimers();
    const clearBatch = vi.fn();
    const scheduler = createBatchSeparationClearScheduler(clearBatch, 3000);

    scheduler.scheduleAfterTerminalProgress();

    vi.advanceTimersByTime(3000);
    expect(clearBatch).toHaveBeenCalledTimes(1);
    vi.useRealTimers();
  });
});

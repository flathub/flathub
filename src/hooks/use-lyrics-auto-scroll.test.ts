// @vitest-environment jsdom

import { beforeEach, describe, expect, test, vi } from "vitest";
import { act, createElement, useRef } from "react";
import { createRoot } from "react-dom/client";
import {
  createUserScrollGuard,
  useLyricsAutoScroll,
} from "./use-lyrics-auto-scroll";

const PAUSE_MS = 3000;

function makeContainer(): HTMLDivElement {
  return document.createElement("div");
}

describe("createUserScrollGuard", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });

  test("is inactive before any user interaction", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    expect(guard.isActive()).toBe(false);

    guard.destroy();
  });

  test("becomes active immediately after a wheel event", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    container.dispatchEvent(new Event("wheel"));

    expect(guard.isActive()).toBe(true);

    guard.destroy();
  });

  test("becomes active immediately after a touchstart event", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    container.dispatchEvent(new Event("touchstart"));

    expect(guard.isActive()).toBe(true);

    guard.destroy();
  });

  test("remains active while the pause window is still open", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    container.dispatchEvent(new Event("wheel"));
    vi.advanceTimersByTime(PAUSE_MS - 1);

    expect(guard.isActive()).toBe(true);

    guard.destroy();
  });

  test("deactivates automatically after the full pause duration elapses", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    container.dispatchEvent(new Event("wheel"));
    vi.advanceTimersByTime(PAUSE_MS);

    expect(guard.isActive()).toBe(false);

    guard.destroy();
  });

  test("resets the pause timer when the user scrolls again before it expires", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    // First scroll — advances almost to the end of the window.
    container.dispatchEvent(new Event("wheel"));
    vi.advanceTimersByTime(PAUSE_MS - 500);

    // Second scroll — should restart the full 3 s window.
    container.dispatchEvent(new Event("wheel"));
    vi.advanceTimersByTime(PAUSE_MS - 1);

    // Still inside the second window, so still active.
    expect(guard.isActive()).toBe(true);

    vi.advanceTimersByTime(1);

    // Now the second window has fully elapsed.
    expect(guard.isActive()).toBe(false);

    guard.destroy();
  });

  test("stops responding to events after destroy()", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    guard.destroy();

    container.dispatchEvent(new Event("wheel"));

    expect(guard.isActive()).toBe(false);
  });

  test("clears a pending resume timer on destroy()", () => {
    const clearTimeout = vi.fn(globalThis.clearTimeout.bind(globalThis));
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS, {
      setTimeout: globalThis.setTimeout,
      clearTimeout,
    });

    container.dispatchEvent(new Event("wheel"));

    // Timer was set; destroying before it fires must clear it.
    guard.destroy();

    expect(clearTimeout).toHaveBeenCalled();
  });

  test("resets to inactive immediately on destroy() even if window was open", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    container.dispatchEvent(new Event("wheel"));
    expect(guard.isActive()).toBe(true);

    guard.destroy();

    expect(guard.isActive()).toBe(false);
  });

  test("touchstart also resets the timer when fired after wheel", () => {
    const container = makeContainer();
    const guard = createUserScrollGuard(container, PAUSE_MS);

    container.dispatchEvent(new Event("wheel"));
    vi.advanceTimersByTime(PAUSE_MS - 500);

    // Touch interaction restarts the window.
    container.dispatchEvent(new Event("touchstart"));
    vi.advanceTimersByTime(PAUSE_MS - 1);

    expect(guard.isActive()).toBe(true);

    vi.advanceTimersByTime(1);

    expect(guard.isActive()).toBe(false);

    guard.destroy();
  });
});

describe("useLyricsAutoScroll", () => {
  test("re-centers the active line when lyric layout changes without an index change", async () => {
    const host = document.createElement("div");
    document.body.append(host);
    const root = createRoot(host);
    const scrollTo = vi.fn();

    function TestHarness({ layoutVersion }: { layoutVersion: string }) {
      const ref = (node: HTMLDivElement | null) => {
        if (!node) return;
        Object.defineProperty(node, "clientHeight", {
          configurable: true,
          value: 100,
        });
        Object.defineProperty(node, "scrollHeight", {
          configurable: true,
          value: 500,
        });
        node.scrollTo = scrollTo;
      };

      const containerRef = useRef<HTMLDivElement | null>(null);
      useLyricsAutoScroll(
        containerRef,
        1,
        false,
        0,
        "standard",
        "song-1",
        layoutVersion,
      );

      return createElement(
        "div",
        {
          ref: (node: HTMLDivElement | null) => {
            containerRef.current = node;
            ref(node);
          },
        },
        createElement("div", { "data-lyrics-line-index": "1" }, "Line"),
      );
    }

    await act(async () => {
      root.render(
        createElement(TestHarness, { layoutVersion: "romanized-off" }),
      );
    });

    await act(async () => {
      root.render(
        createElement(TestHarness, { layoutVersion: "romanized-on" }),
      );
    });

    expect(scrollTo).toHaveBeenCalledTimes(2);

    root.unmount();
    host.remove();
  });
});

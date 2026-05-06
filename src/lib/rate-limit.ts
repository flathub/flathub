export interface TrailingRateLimiter<T> {
  (value: T): void;
  flush: () => void;
  cancel: () => void;
}

export function createTrailingRateLimiter<T>(
  fn: (value: T) => void,
  intervalMs: number,
): TrailingRateLimiter<T> {
  let lastRunMs = 0;
  let hasRun = false;
  let timer: ReturnType<typeof setTimeout> | null = null;
  let pending: T | undefined;
  let hasPending = false;

  const clearPendingTimer = () => {
    if (timer !== null) {
      clearTimeout(timer);
      timer = null;
    }
  };

  const run = (value: T) => {
    lastRunMs = Date.now();
    hasRun = true;
    fn(value);
  };

  const schedule = () => {
    if (timer !== null) return;
    const elapsedMs = Date.now() - lastRunMs;
    const delayMs = Math.max(0, intervalMs - elapsedMs);

    timer = setTimeout(() => {
      timer = null;
      if (!hasPending) return;
      const value = pending as T;
      pending = undefined;
      hasPending = false;
      run(value);
    }, delayMs);
  };

  const limiter = ((value: T) => {
    if (!hasRun || Date.now() - lastRunMs >= intervalMs) {
      clearPendingTimer();
      pending = undefined;
      hasPending = false;
      run(value);
      return;
    }

    pending = value;
    hasPending = true;
    schedule();
  }) as TrailingRateLimiter<T>;

  limiter.flush = () => {
    if (!hasPending) return;
    clearPendingTimer();
    const value = pending as T;
    pending = undefined;
    hasPending = false;
    run(value);
  };

  limiter.cancel = () => {
    clearPendingTimer();
    pending = undefined;
    hasPending = false;
  };

  return limiter;
}

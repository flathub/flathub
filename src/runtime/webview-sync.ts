export interface WebviewSyncChannel<T> {
  publish: (payload: T) => void;
  subscribe: (listener: (payload: T) => void) => () => void;
  close: () => void;
}

interface BroadcastChannelLike {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
}

interface WebviewSyncEnvelope<T> {
  originId: string;
  payload: T;
}

interface CreateWebviewSyncChannelOptions {
  channelFactory?: (name: string) => BroadcastChannelLike;
  originId?: string;
}

function createOriginId(): string {
  if (
    typeof crypto !== "undefined" &&
    typeof crypto.randomUUID === "function"
  ) {
    return crypto.randomUUID();
  }

  return `openkara-${Math.random().toString(36).slice(2)}`;
}

function createNoopWebviewSyncChannel<T>(): WebviewSyncChannel<T> {
  return {
    publish() {},
    subscribe() {
      return () => {};
    },
    close() {},
  };
}

export function createWebviewSyncChannel<T>(
  name: string,
  options: CreateWebviewSyncChannelOptions = {},
): WebviewSyncChannel<T> {
  const originId = options.originId ?? createOriginId();
  const channel = options.channelFactory
    ? options.channelFactory(name)
    : typeof BroadcastChannel !== "undefined"
      ? (new BroadcastChannel(name) as unknown as BroadcastChannelLike)
      : null;

  if (!channel) {
    return createNoopWebviewSyncChannel<T>();
  }

  const listeners = new Set<(payload: T) => void>();

  channel.onmessage = (event) => {
    const envelope = event.data as WebviewSyncEnvelope<T> | null;
    if (!envelope || envelope.originId === originId) {
      return;
    }

    for (const listener of listeners) {
      listener(envelope.payload);
    }
  };

  return {
    publish(payload) {
      channel.postMessage({
        originId,
        payload,
      } satisfies WebviewSyncEnvelope<T>);
    },
    subscribe(listener) {
      listeners.add(listener);
      return () => {
        listeners.delete(listener);
      };
    },
    close() {
      listeners.clear();
      channel.close();
    },
  };
}

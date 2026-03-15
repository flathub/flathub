import { create } from "zustand";

export interface Notification {
  id: string;
  type: "error" | "warning" | "success" | "info";
  title: string;
  message: string;
  retryable: boolean;
  retryAction?: () => void;
  dismissAfterMs: number | null;
  timestamp: number;
}

interface NotificationState {
  notifications: Notification[];
  addNotification: (
    notification: Omit<Notification, "id" | "timestamp">,
  ) => void;
  dismissNotification: (id: string) => void;
  clearAll: () => void;
}

const MAX_VISIBLE = 5;

export const useNotificationStore = create<NotificationState>((set) => ({
  notifications: [],

  addNotification: (notification) => {
    const id = crypto.randomUUID();
    const entry: Notification = {
      ...notification,
      id,
      timestamp: Date.now(),
    };

    set((state) => ({
      notifications: [...state.notifications, entry].slice(-MAX_VISIBLE),
    }));

    if (notification.dismissAfterMs !== null) {
      setTimeout(() => {
        set((state) => ({
          notifications: state.notifications.filter((n) => n.id !== id),
        }));
      }, notification.dismissAfterMs);
    }
  },

  dismissNotification: (id) => {
    set((state) => ({
      notifications: state.notifications.filter((n) => n.id !== id),
    }));
  },

  clearAll: () => set({ notifications: [] }),
}));

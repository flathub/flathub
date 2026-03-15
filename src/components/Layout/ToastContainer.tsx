import { X, AlertCircle, CheckCircle, Info, AlertTriangle } from "lucide-react";
import { useTranslation } from "react-i18next";
import {
  useNotificationStore,
  type Notification,
} from "@/stores/notification-store";

const ICON_MAP = {
  error: AlertCircle,
  warning: AlertTriangle,
  success: CheckCircle,
  info: Info,
} as const;

const COLOR_MAP = {
  error: "text-red-400",
  warning: "text-yellow-400",
  success: "text-green-400",
  info: "text-blue-400",
} as const;

const BORDER_MAP = {
  error: "border-red-400/30",
  warning: "border-yellow-400/30",
  success: "border-green-400/30",
  info: "border-blue-400/30",
} as const;

function Toast({ notification }: { notification: Notification }) {
  const { t } = useTranslation();
  const dismiss = useNotificationStore((s) => s.dismissNotification);
  const Icon = ICON_MAP[notification.type];

  return (
    <div
      className={`animate-slide-up flex items-start gap-2.5 rounded-lg border bg-[var(--color-sidebar)] px-3 py-2.5 shadow-lg ${BORDER_MAP[notification.type]}`}
    >
      <Icon size={14} className={`mt-0.5 shrink-0 ${COLOR_MAP[notification.type]}`} />

      <div className="min-w-0 flex-1">
        <p className="text-[12px] font-medium text-white">
          {notification.title}
        </p>
        {notification.message && (
          <p className="mt-0.5 text-[11px] text-[var(--color-text-dim)]">
            {notification.message}
          </p>
        )}
        {notification.retryable && notification.retryAction && (
          <button
            onClick={() => {
              notification.retryAction?.();
              dismiss(notification.id);
            }}
            className="mt-1.5 text-[11px] text-[var(--color-accent)] hover:underline"
          >
            {t("common.tryAgain")}
          </button>
        )}
      </div>

      <button
        onClick={() => dismiss(notification.id)}
        className="shrink-0 text-[var(--color-text-dimmer)] hover:text-[var(--color-text-dim)]"
      >
        <X size={12} />
      </button>
    </div>
  );
}

export function ToastContainer() {
  const notifications = useNotificationStore((s) => s.notifications);

  if (notifications.length === 0) return null;

  return (
    <div className="fixed right-4 bottom-16 z-[100] flex w-80 flex-col gap-2">
      {notifications.map((n) => (
        <Toast key={n.id} notification={n} />
      ))}
    </div>
  );
}

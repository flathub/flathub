import { useCallback, useRef } from "react";
import type { PointerEvent as ReactPointerEvent, ReactNode } from "react";

interface SidebarRailProps {
  visible: boolean;
  width: number;
  onResize: (width: number) => void;
  children: ReactNode;
}

const MIN_SIDEBAR_WIDTH = 240;
const MAX_SIDEBAR_WIDTH = 420;

function clampWidth(width: number): number {
  return Math.min(MAX_SIDEBAR_WIDTH, Math.max(MIN_SIDEBAR_WIDTH, width));
}

export function SidebarRail({
  visible,
  width,
  onResize,
  children,
}: SidebarRailProps) {
  const dragStateRef = useRef<{ startX: number; startWidth: number } | null>(
    null,
  );

  const handleDragStart = useCallback(
    (event: ReactPointerEvent<HTMLDivElement>) => {
      if (!visible) {
        return;
      }

      const handle = event.currentTarget;
      dragStateRef.current = {
        startX: event.clientX,
        startWidth: width,
      };

      const onPointerMove = (moveEvent: PointerEvent) => {
        const dragState = dragStateRef.current;
        if (!dragState) {
          return;
        }

        const delta = moveEvent.clientX - dragState.startX;
        onResize(clampWidth(Math.round(dragState.startWidth + delta)));
      };

      const onPointerUp = () => {
        dragStateRef.current = null;
        window.removeEventListener("pointermove", onPointerMove);
        window.removeEventListener("pointerup", onPointerUp);
      };

      window.addEventListener("pointermove", onPointerMove);
      window.addEventListener("pointerup", onPointerUp, { once: true });
      handle.setPointerCapture(event.pointerId);
    },
    [onResize, visible, width],
  );

  return (
    <div
      className={`shrink-0 overflow-hidden transition-[width] ${
        visible ? "w-[var(--window-shell-sidebar-width)]" : "w-0"
      }`}
      style={{
        transitionDuration: "var(--motion-duration-standard)",
        transitionTimingFunction: "var(--motion-ease-emphasized-out)",
      }}
    >
      <div
        className={`relative h-full w-[var(--window-shell-sidebar-width)] transition-[opacity,transform] ${
          visible ? "translate-x-0 opacity-100" : "-translate-x-3 opacity-0"
        }`}
        style={{
          transitionDuration: "var(--motion-duration-standard)",
          transitionTimingFunction: "var(--motion-ease-emphasized-out)",
        }}
      >
        {children}
        {visible ? (
          <div
            role="separator"
            aria-orientation="vertical"
            aria-label="Resize sidebar"
            onPointerDown={handleDragStart}
            className="absolute right-0 top-0 h-full w-1.5 cursor-col-resize bg-transparent hover:bg-white/10"
          />
        ) : null}
      </div>
    </div>
  );
}

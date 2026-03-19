import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import { getContextMenuPosition } from "./context-menu-position";

export interface ContextMenuItem {
  label: string;
  onClick: () => void;
  indicator?: "checked" | "mixed" | null;
}

interface ContextMenuProps {
  x: number;
  y: number;
  items: ContextMenuItem[];
  onClose: () => void;
}

export function ContextMenu({ x, y, items, onClose }: ContextMenuProps) {
  const menuRef = useRef<HTMLDivElement>(null);
  const [position, setPosition] = useState({ left: x, top: y });

  useEffect(() => {
    const handleClickOutside = (e: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
        onClose();
      }
    };
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };

    document.addEventListener("mousedown", handleClickOutside);
    document.addEventListener("keydown", handleEscape);
    return () => {
      document.removeEventListener("mousedown", handleClickOutside);
      document.removeEventListener("keydown", handleEscape);
    };
  }, [onClose]);

  useLayoutEffect(() => {
    const updatePosition = () => {
      const menu = menuRef.current;
      if (!menu) {
        return;
      }

      const rect = menu.getBoundingClientRect();
      setPosition(
        getContextMenuPosition({
          x,
          y,
          menuWidth: rect.width,
          menuHeight: rect.height,
          viewportWidth: window.innerWidth,
          viewportHeight: window.innerHeight,
        }),
      );
    };

    updatePosition();
    window.addEventListener("resize", updatePosition);
    window.addEventListener("scroll", updatePosition, true);

    return () => {
      window.removeEventListener("resize", updatePosition);
      window.removeEventListener("scroll", updatePosition, true);
    };
  }, [x, y, items]);

  if (typeof document === "undefined" || !document.body) {
    return null;
  }

  return createPortal(
    <div
      ref={menuRef}
      className="fixed z-[70] min-w-[140px] rounded-md border border-[var(--color-border)] bg-[var(--color-sidebar)] py-1 shadow-xl"
      style={{ left: position.left, top: position.top }}
    >
      {items.map((item) => (
        <button
          key={item.label}
          onClick={() => {
            item.onClick();
            onClose();
          }}
          className="flex w-full items-center px-3 py-1.5 text-left text-[13px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
        >
          <span
            className="mr-2 flex h-4 w-4 shrink-0 items-center justify-center text-[10px] text-[var(--color-accent)]"
            aria-hidden="true"
          >
            {item.indicator === "checked"
              ? "✓"
              : item.indicator === "mixed"
                ? "−"
                : ""}
          </span>
          <span>{item.label}</span>
        </button>
      ))}
    </div>,
    document.body,
  );
}

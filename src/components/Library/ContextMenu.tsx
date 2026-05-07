import { useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import { ChevronRight } from "lucide-react";
import { getContextMenuPosition } from "./context-menu-position";
import { isInSafetyZone } from "./submenu-safety-zone";
import type { Point } from "./submenu-safety-zone";

export interface ContextMenuItem {
  label: string;
  children?: ContextMenuItem[];
  onClick?: () => void;
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
  const [submenuItem, setSubmenuItem] = useState<{
    children: ContextMenuItem[];
    parentRect: DOMRect;
  } | null>(null);
  const isOverSubmenu = useRef(false);
  const hideTimeout = useRef<ReturnType<typeof setTimeout> | null>(null);
  const mousePosRef = useRef<Point>({ x: 0, y: 0 });
  const parentItemRectRef = useRef<DOMRect | null>(null);
  const submenuRectRef = useRef<DOMRect | null>(null);

  const itemCount = items.length;

  const clearHide = () => {
    if (hideTimeout.current) {
      clearTimeout(hideTimeout.current);
      hideTimeout.current = null;
    }
  };

  const scheduleHide = () => {
    clearHide();
    const mouse = mousePosRef.current;
    const parent = parentItemRectRef.current;
    const submenu = submenuRectRef.current;
    if (parent && submenu && isInSafetyZone(mouse, parent, submenu)) {
      return;
    }
    hideTimeout.current = setTimeout(() => {
      if (!isOverSubmenu.current) {
        setSubmenuItem(null);
      }
    }, 300);
  };

  useEffect(() => {
    const handleClickOutside = (e: MouseEvent) => {
      const target = e.target as Node;
      const inMenu = menuRef.current?.contains(target);
      const inSubmenu = (target as Element).closest?.("[data-context-submenu]");
      if (!inMenu && !inSubmenu) {
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

  useEffect(() => {
    return () => clearHide();
  }, []);

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
  }, [x, y, items, itemCount]);

  if (typeof document === "undefined" || !document.body) {
    return null;
  }

  return createPortal(
    <>
      <div
        ref={menuRef}
        className="fixed z-[70] min-w-[140px] rounded-md border border-[var(--color-border)] bg-[var(--color-sidebar)] py-1 shadow-xl"
        style={{ left: position.left, top: position.top }}
        onMouseMove={(e) => {
          mousePosRef.current = { x: e.clientX, y: e.clientY };
        }}
      >
        {items.map((item) => (
          <button
            key={item.label}
            onClick={() => {
              if (!item.children) {
                item.onClick?.();
                onClose();
              }
            }}
            onMouseEnter={
              item.children
                ? (e) => {
                    clearHide();
                    const btn = e.currentTarget;
                    parentItemRectRef.current = btn.getBoundingClientRect();
                    setSubmenuItem({
                      children: item.children!,
                      parentRect: btn.getBoundingClientRect(),
                    });
                  }
                : undefined
            }
            onMouseLeave={
              item.children
                ? () => {
                    scheduleHide();
                  }
                : undefined
            }
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
            <span className="flex-1">{item.label}</span>
            {item.children && <ChevronRight size={14} className="ml-3" />}
          </button>
        ))}
      </div>

      {submenuItem && (
        <SubMenu
          parentRect={submenuItem.parentRect}
          items={submenuItem.children}
          onClose={onClose}
          onSubmenuRect={(rect) => {
            submenuRectRef.current = rect;
          }}
          onMouseEnter={() => {
            isOverSubmenu.current = true;
            clearHide();
          }}
          onMouseLeave={() => {
            isOverSubmenu.current = false;
            scheduleHide();
          }}
        />
      )}
    </>,
    document.body,
  );
}

function SubMenu({
  parentRect,
  items,
  onClose,
  onSubmenuRect,
  onMouseEnter,
  onMouseLeave,
}: {
  parentRect: DOMRect;
  items: ContextMenuItem[];
  onClose: () => void;
  onSubmenuRect: (rect: DOMRect) => void;
  onMouseEnter: () => void;
  onMouseLeave: () => void;
}) {
  const menuRef = useRef<HTMLDivElement>(null);
  const [pos, setPos] = useState({
    left: parentRect.right,
    top: parentRect.top,
  });

  useLayoutEffect(() => {
    const menu = menuRef.current;
    if (!menu) return;

    const rect = menu.getBoundingClientRect();
    const vw = window.innerWidth;
    const vh = window.innerHeight;

    let left = parentRect.right;
    let top = parentRect.top;

    if (left + rect.width > vw) {
      left = parentRect.left - rect.width;
    }
    if (top + rect.height > vh) {
      top = vh - rect.height;
    }

    setPos({ left, top });
    onSubmenuRect(menu.getBoundingClientRect());
  }, [parentRect, items, onSubmenuRect]);

  return (
    <div
      ref={menuRef}
      data-context-submenu
      className="fixed z-[71] min-w-[140px] rounded-md border border-[var(--color-border)] bg-[var(--color-sidebar)] py-1 shadow-xl"
      style={{ left: pos.left, top: pos.top }}
      onMouseEnter={onMouseEnter}
      onMouseLeave={onMouseLeave}
    >
      {items.map((item) => (
        <button
          key={item.label}
          onClick={() => {
            item.onClick?.();
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
    </div>
  );
}

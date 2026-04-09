import {
  cloneElement,
  useEffect,
  useLayoutEffect,
  useReducer,
  useRef,
  useState,
  useId,
  isValidElement,
  type ReactElement,
  type ReactNode,
} from "react";
import { createPortal } from "react-dom";
import { getTooltipPosition, tooltipVisibilityReducer } from "./Tooltip.utils";

interface TooltipProps {
  children: ReactNode;
  label: string;
  shortcut?: string;
}

export function Tooltip({ children, label, shortcut }: TooltipProps) {
  const anchorRef = useRef<HTMLSpanElement>(null);
  const tooltipRef = useRef<HTMLDivElement>(null);
  const [open, dispatch] = useReducer(tooltipVisibilityReducer, false);
  const tooltipId = useId();
  const [position, setPosition] = useState<{
    left: number;
    top: number;
  } | null>(null);

  const describedChildren = (() => {
    if (!open) {
      return children;
    }

    if (!isValidElement(children)) {
      return children;
    }

    const existing = (children.props as { "aria-describedby"?: string })?.[
      "aria-describedby"
    ];
    const merged = existing
      ? existing.split(/\s+/).includes(tooltipId)
        ? existing
        : `${existing} ${tooltipId}`
      : tooltipId;

    return cloneElement(
      children as ReactElement,
      {
        "aria-describedby": merged,
      } as Record<string, unknown>,
    );
  })();

  useLayoutEffect(() => {
    if (
      !open ||
      !anchorRef.current ||
      !tooltipRef.current ||
      typeof window === "undefined"
    ) {
      return;
    }

    const updatePosition = () => {
      if (!anchorRef.current || !tooltipRef.current) {
        return;
      }

      setPosition(
        getTooltipPosition(
          anchorRef.current.getBoundingClientRect(),
          {
            width: tooltipRef.current.offsetWidth,
            height: tooltipRef.current.offsetHeight,
          },
          {
            width: window.innerWidth,
            height: window.innerHeight,
          },
        ),
      );
    };

    updatePosition();
    window.addEventListener("resize", updatePosition);
    window.addEventListener("scroll", updatePosition, true);

    return () => {
      window.removeEventListener("resize", updatePosition);
      window.removeEventListener("scroll", updatePosition, true);
    };
  }, [open]);

  useEffect(() => {
    if (!open) {
      return;
    }

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        dispatch({ type: "escape" });
      }
    };

    document.addEventListener("keydown", handleKeyDown);
    return () => {
      document.removeEventListener("keydown", handleKeyDown);
    };
  }, [open]);

  return (
    <>
      <span
        ref={anchorRef}
        className="inline-flex"
        onMouseEnter={() => dispatch({ type: "pointer-enter" })}
        onMouseLeave={() => dispatch({ type: "pointer-leave" })}
        onFocusCapture={() => dispatch({ type: "focus" })}
        onBlurCapture={(event) => {
          if (anchorRef.current?.contains(event.relatedTarget as Node)) {
            return;
          }
          dispatch({ type: "blur" });
        }}
      >
        {describedChildren}
      </span>

      {open && typeof document !== "undefined"
        ? createPortal(
            <div
              ref={tooltipRef}
              role="tooltip"
              id={tooltipId}
              className="app-panel-surface pointer-events-none fixed z-[80] flex items-center gap-2 rounded-xl border border-[color-mix(in_srgb,var(--color-border)_88%,transparent)] bg-[color-mix(in_srgb,var(--color-sidebar)_96%,transparent)] px-3 py-1.5 text-[11px] font-medium text-white shadow-[0_18px_36px_rgba(0,0,0,0.34)]"
              style={
                position
                  ? position
                  : {
                      left: 0,
                      top: 0,
                      opacity: 0,
                    }
              }
            >
              <span>{label}</span>
              {shortcut ? (
                <span className="rounded-md bg-[var(--color-ghost-hover)] px-1.5 py-0.5 text-[10px] font-semibold text-[var(--color-text-dim)]">
                  {shortcut}
                </span>
              ) : null}
            </div>,
            document.body,
          )
        : null}
    </>
  );
}

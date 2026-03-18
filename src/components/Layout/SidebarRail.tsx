import type { ReactNode } from "react";

interface SidebarRailProps {
  visible: boolean;
  children: ReactNode;
}

export function SidebarRail({ visible, children }: SidebarRailProps) {
  return (
    <div
      className={`shrink-0 overflow-hidden transition-[width] ${
        visible ? "w-[260px]" : "w-0"
      }`}
      style={{
        transitionDuration: "var(--motion-duration-standard)",
        transitionTimingFunction: "var(--motion-ease-emphasized-out)",
      }}
    >
      <div
        className={`h-full w-[260px] transition-[opacity,transform] ${
          visible ? "translate-x-0 opacity-100" : "-translate-x-3 opacity-0"
        }`}
        style={{
          transitionDuration: "var(--motion-duration-standard)",
          transitionTimingFunction: "var(--motion-ease-emphasized-out)",
        }}
      >
        {children}
      </div>
    </div>
  );
}

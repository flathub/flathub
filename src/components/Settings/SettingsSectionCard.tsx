import type { ReactNode } from "react";

interface SettingsSectionCardProps {
  title: string;
  description?: string;
  tone?: "default" | "danger";
  children: ReactNode;
}

export function SettingsSectionCard({
  title,
  description,
  tone = "default",
  children,
}: SettingsSectionCardProps) {
  const isDanger = tone === "danger";

  return (
    <section
      className={`space-y-3 rounded-lg border p-5 ${
        isDanger
          ? "border-red-500/30 bg-[var(--color-sidebar)]"
          : "border-[var(--color-border)] bg-[var(--color-sidebar)]"
      }`}
    >
      <div className="space-y-1">
        <label
          className={`text-[12px] font-medium uppercase ${
            isDanger ? "text-red-400" : "text-[var(--color-text-dim)]"
          }`}
        >
          {title}
        </label>
        {description && (
          <p className="text-[12px] text-[var(--color-text-dim)]">
            {description}
          </p>
        )}
      </div>

      {children}
    </section>
  );
}

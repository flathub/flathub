package util

import (
	"fmt"
	"math"
	"os"
	"path"
	"strings"
	"time"
)

const (
	tmpDir      = "/tmp/.mpris-timer"
	width       = 256
	height      = 256
	padding     = 16
	strokeWidth = 32
)

func init() {
	_ = os.MkdirAll(tmpDir, 0755)
}

func MakeProgressCircle(progress float64) (string, error) {
	progress = math.Max(0, math.Min(100, progress))
	filename := path.Join(tmpDir, fmt.Sprintf("_c.%.1f.svg", progress))

	if _, err := os.Stat(filename); err == nil {
		return filename, nil
	}

	centerX := width / 2
	centerY := height / 2
	radius := float64(width)/2 - float64(strokeWidth) - float64(padding)
	baseWidth := strokeWidth * 0.25
	circumference := 2 * math.Pi * radius
	dashOffset := circumference * (1 - progress/100)

	svg := fmt.Sprintf(`<svg width="%d" height="%d">
		<circle
			cx="%d"
			cy="%d"
			r="%.1f"
			fill="none"
			stroke="#535353"
			stroke-width="%d"
		/>
		<circle
			cx="%d"
			cy="%d"
			r="%.1f"
			fill="none"
			stroke="#2190a4"
			stroke-width="%d"
			stroke-dasharray="%.1f"
			stroke-dashoffset="%.1f"
			transform="rotate(-90 %d %d)"
		/>
	</svg>`, width, height, centerX, centerY, radius, baseWidth,
		centerX, centerY, radius, strokeWidth, circumference, dashOffset, centerX, centerY)

	err := os.WriteFile(filename, []byte(strings.TrimSpace(svg)), 0644)
	if err != nil {
		return "", fmt.Errorf("failed to write SVG file: %w", err)
	}

	return filename, nil
}

func FormatDuration(d time.Duration) string {
	d = d.Round(time.Second)
	h := d / time.Hour
	d -= h * time.Hour
	m := d / time.Minute
	d -= m * time.Minute
	s := d / time.Second

	if h > 0 {
		return fmt.Sprintf("%02d:%02d:%02d", h, m, s)
	}

	return fmt.Sprintf("%02d:%02d", m, s)
}

package util

import (
	"bytes"
	"fmt"
	"github.com/diamondburned/gotk4/pkg/gdk/v4"
	"log"
	"math"
	"os"
	"path"
	"strings"
	"text/template"
	"time"
)

const (
	tmpDir        = "/tmp/.mpris-timer"
	width         = 256
	height        = 256
	padding       = 16
	strokeWidth   = 32
	fgStrokeColor = "#535353"
	bgStrokeColor = "#2190a4"
)

type svgParams struct {
	Width         int
	Height        int
	CenterX       int
	CenterY       int
	Radius        float64
	FgStrokeColor string
	BaseWidth     int
	BgStrokeColor string
	StrokeWidth   int
	Circumference float64
	DashOffset    float64
}

const svgTemplate = `
<svg width="{{.Width}}" height="{{.Height}}">
    <circle cx="{{.CenterX}}" cy="{{.CenterY}}" r="{{.Radius}}" fill="none" stroke="{{.FgStrokeColor}}" stroke-width="{{.BaseWidth}}" />
    <circle cx="{{.CenterX}}" cy="{{.CenterY}}" r="{{.Radius}}" fill="none" stroke="{{.BgStrokeColor}}" stroke-width="{{.StrokeWidth}}" stroke-dasharray="{{.Circumference}}" stroke-dashoffset="{{.DashOffset}}" transform="rotate(-90 {{.CenterX}} {{.CenterY}})" />
</svg>
`

func init() {
	_ = os.MkdirAll(tmpDir, 0755)
}

func ParseKeyval(keyval uint) string {
	return strings.ReplaceAll(gdk.KeyvalName(keyval), "KP_", "")
}

func IsGdkKeyvalNumber(keyval uint) bool {
	return (keyval >= gdk.KEY_0 && keyval <= gdk.KEY_9) || (keyval >= gdk.KEY_KP_0 && keyval <= gdk.KEY_KP_9)
}

func NumToLabelText(num int) string {
	if num > 59 || num < 0 {
		log.Fatalf("NumToLabelText: num must be between 0 and 59")
	}

	return fmt.Sprintf("%02d", num)
}

func MakeProgressCircle(progress float64) (string, error) {
	progress = math.Max(0, math.Min(100, progress))
	filename := path.Join(tmpDir, fmt.Sprintf("_f4g.%.1f.svg", progress))

	if _, err := os.Stat(filename); err == nil {
		return filename, nil
	}

	centerX := width / 2
	centerY := height / 2
	radius := float64(width)/2 - float64(strokeWidth) - float64(padding)
	baseWidth := int(math.Round(strokeWidth * 0.25))
	circumference := 2 * math.Pi * radius
	dashOffset := circumference * (1 - progress/100)

	data := svgParams{
		Width:         width,
		Height:        height,
		CenterX:       centerX,
		CenterY:       centerY,
		Radius:        radius,
		BaseWidth:     baseWidth,
		StrokeWidth:   strokeWidth,
		FgStrokeColor: fgStrokeColor,
		BgStrokeColor: bgStrokeColor,
		Circumference: circumference,
		DashOffset:    dashOffset,
	}

	tmpl, err := template.New("svg").Parse(svgTemplate)
	if err != nil {
		return "", err
	}

	var svgBuffer bytes.Buffer
	err = tmpl.Execute(&svgBuffer, data)
	if err != nil {
		return "", err
	}

	err = os.WriteFile(filename, svgBuffer.Bytes(), 0644)
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

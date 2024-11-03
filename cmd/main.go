package main

import (
	"fmt"
	"log"
	"math"
	"os"
	"os/signal"
	"path"
	"strconv"
	"strings"
	"time"

	"github.com/godbus/dbus/v5"
)

const (
	baseFPS      = 25
	baseInterval = time.Second / baseFPS
	tmpDir       = "/tmp/.mpris-timer"
)

type MPRISPlayer struct {
	serviceName    string
	objectPath     dbus.ObjectPath
	conn           *dbus.Conn
	name           string
	duration       time.Duration
	startTime      time.Time
	isPaused       bool
	pausedAt       time.Time
	pausedFor      time.Duration
	tickerDone     chan struct{}
	playbackStatus string
}

func NewMPRISPlayer(seconds int, name string) (*MPRISPlayer, error) {
	if seconds <= 0 {
		return nil, fmt.Errorf("duration must be positive")
	}

	if err := os.MkdirAll(tmpDir, 0755); err != nil {
		return nil, fmt.Errorf("failed to create temp directory: %w", err)
	}

	return &MPRISPlayer{
		name:           name,
		duration:       time.Duration(seconds) * time.Second,
		objectPath:     "/org/mpris/MediaPlayer2",
		playbackStatus: "Playing",
		tickerDone:     make(chan struct{}),
	}, nil
}

func (p *MPRISPlayer) Init() error {
	conn, err := dbus.SessionBus()
	if err != nil {
		return fmt.Errorf("failed to connect to session bus: %w", err)
	}

	p.conn = conn
	p.serviceName = fmt.Sprintf("org.mpris.MediaPlayer2.mpris-timer-%d", time.Now().UnixNano())

	reply, err := conn.RequestName(p.serviceName, dbus.NameFlagDoNotQueue)
	if err != nil {
		return fmt.Errorf("failed to request name: %w", err)
	}

	if reply != dbus.RequestNameReplyPrimaryOwner {
		return fmt.Errorf("name already taken")
	}

	if err := p.exportInterfaces(); err != nil {
		return fmt.Errorf("failed to export interfaces: %w", err)
	}

	p.startTime = time.Now()
	go p.tick()

	return nil
}

func (p *MPRISPlayer) exportInterfaces() error {
	if err := p.conn.Export(p, p.objectPath, "org.mpris.MediaPlayer2"); err != nil {
		return err
	}

	if err := p.conn.Export(p, p.objectPath, "org.mpris.MediaPlayer2.Player"); err != nil {
		return err
	}

	if err := p.conn.Export(p, p.objectPath, "org.freedesktop.DBus.Properties"); err != nil {
		return err
	}

	return nil
}

func (p *MPRISPlayer) tick() {
	ticker := time.NewTicker(baseInterval)
	defer ticker.Stop()

	for {
		select {
		case <-p.tickerDone:
			return
		case <-ticker.C:
			if p.isPaused {
				continue
			}

			elapsed := time.Since(p.startTime) - p.pausedFor
			progress := math.Min(100, (float64(elapsed)/float64(p.duration))*100)

			if progress >= 100 {
				os.Exit(0)
			}

			timeLeft := p.duration - elapsed
			progressImg, err := makeProgressCircle(progress)
			if err != nil {
				log.Printf("Failed to create progress circle: %v", err)
				continue
			}

			metadata := map[string]dbus.Variant{
				"mpris:trackid": dbus.MakeVariant(dbus.ObjectPath("/track/1")),
				"xesam:title":   dbus.MakeVariant(p.name),
				"xesam:artist":  dbus.MakeVariant([]string{formatDuration(timeLeft)}),
				"mpris:artUrl":  dbus.MakeVariant("file://" + progressImg),
			}

			p.emitPropertiesChanged("org.mpris.MediaPlayer2.Player", map[string]dbus.Variant{
				"Metadata":       dbus.MakeVariant(metadata),
				"PlaybackStatus": dbus.MakeVariant(p.playbackStatus),
			})
		}
	}
}

func (p *MPRISPlayer) emitPropertiesChanged(iface string, changed map[string]dbus.Variant) {
	err := p.conn.Emit(p.objectPath, "org.freedesktop.DBus.Properties.PropertiesChanged",
		iface, changed, []string{})
	if err != nil {
		log.Printf("Failed to emit properties changed: %v", err)
	}
}

func (p *MPRISPlayer) Raise() *dbus.Error { return nil }
func (p *MPRISPlayer) Quit() *dbus.Error  { os.Exit(0); return nil }

func (p *MPRISPlayer) PlayPause() *dbus.Error {
	if p.isPaused {
		p.pausedFor += time.Since(p.pausedAt)
	} else {
		p.pausedAt = time.Now()
	}
	p.isPaused = !p.isPaused
	p.playbackStatus = map[bool]string{true: "Paused", false: "Playing"}[p.isPaused]

	p.emitPropertiesChanged("org.mpris.MediaPlayer2.Player", map[string]dbus.Variant{
		"PlaybackStatus": dbus.MakeVariant(p.playbackStatus),
	})
	return nil
}

func (p *MPRISPlayer) Previous() *dbus.Error {
	p.startTime = time.Now()
	p.pausedFor = 0
	p.isPaused = false
	p.playbackStatus = "Playing"
	return nil
}

func (p *MPRISPlayer) Next() *dbus.Error { os.Exit(1); return nil }
func (p *MPRISPlayer) Stop() *dbus.Error { os.Exit(1); return nil }

func (p *MPRISPlayer) Get(iface, prop string) (dbus.Variant, *dbus.Error) {
	switch iface {
	case "org.mpris.MediaPlayer2":
		switch prop {
		case "Identity":
			return dbus.MakeVariant("MPRIS Timer"), nil
		case "DesktopEntry":
			return dbus.MakeVariant(path.Join(os.Getenv("PWD"), "mpris-timer.desktop")), nil
		}
	case "org.mpris.MediaPlayer2.Player":
		switch prop {
		case "PlaybackStatus":
			return dbus.MakeVariant(p.playbackStatus), nil
		case "CanGoNext":
			return dbus.MakeVariant(true), nil
		case "CanGoPrevious":
			return dbus.MakeVariant(true), nil
		case "CanPlay":
			return dbus.MakeVariant(true), nil
		case "CanPause":
			return dbus.MakeVariant(true), nil
		case "CanSeek":
			return dbus.MakeVariant(false), nil
		case "CanControl":
			return dbus.MakeVariant(true), nil
		}
	}
	return dbus.Variant{}, nil
}

func (p *MPRISPlayer) GetAll(iface string) (map[string]dbus.Variant, *dbus.Error) {
	props := make(map[string]dbus.Variant)
	switch iface {
	case "org.mpris.MediaPlayer2":
		props["Identity"] = dbus.MakeVariant("MPRIS Timer")
		props["DesktopEntry"] = dbus.MakeVariant(path.Join(os.Getenv("PWD"), "mpris-timer.desktop"))
	case "org.mpris.MediaPlayer2.Player":
		props["PlaybackStatus"] = dbus.MakeVariant(p.playbackStatus)
		props["CanGoNext"] = dbus.MakeVariant(true)
		props["CanGoPrevious"] = dbus.MakeVariant(true)
		props["CanPlay"] = dbus.MakeVariant(true)
		props["CanPause"] = dbus.MakeVariant(true)
		props["CanSeek"] = dbus.MakeVariant(false)
		props["CanControl"] = dbus.MakeVariant(true)
	}
	return props, nil
}

func (p *MPRISPlayer) Set(iface, prop string, value dbus.Variant) *dbus.Error {
	return nil
}

func makeProgressCircle(progress float64) (string, error) {
	progress = math.Max(0, math.Min(100, progress))
	filename := path.Join(tmpDir, fmt.Sprintf("_c.%.1f.svg", progress))

	if _, err := os.Stat(filename); err == nil {
		return filename, nil
	}

	const (
		width       = 256
		height      = 256
		padding     = 16
		strokeWidth = 32
	)

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

func formatDuration(d time.Duration) string {
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

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: mpris-timer <seconds> [<title>]")
		os.Exit(1)
	}

	seconds, err := strconv.Atoi(os.Args[1])
	if err != nil {
		log.Fatalf("Invalid duration: %v", err)
	}

	name := "Timer"
	if len(os.Args) > 2 {
		name = os.Args[2]
	}

	player, err := NewMPRISPlayer(seconds, name)
	if err != nil {
		log.Fatalf("Failed to create player: %v", err)
	}

	if err := player.Init(); err != nil {
		log.Fatalf("Failed to initialize player: %v", err)
	}

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt)

	select {
	case <-sigChan:
		os.Exit(0)
	}
}

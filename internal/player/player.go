package player

import (
	"fmt"
	"github.com/godbus/dbus/v5"
	"log"
	"math"
	"mpris-timer/internal/util"
	"os"
	"path"
	"time"
)

const (
	baseFPS      = 25
	baseInterval = time.Second / baseFPS
)

// RequestNames for possible instances
var RequestNames = []string{
	"org.mpris.MediaPlayer2.io.github.efogdev.mpris-timer",
	"org.mpris.MediaPlayer2.io.github.efogdev.mpris-timer-0",
	"org.mpris.MediaPlayer2.io.github.efogdev.mpris-timer-1",
	"org.mpris.MediaPlayer2.io.github.efogdev.mpris-timer-2",
	"org.mpris.MediaPlayer2.io.github.efogdev.mpris-timer-3",
	"org.mpris.MediaPlayer2.io.github.efogdev.mpris-timer-4",
}

type MPRISPlayer struct {
	Done           chan struct{}
	Name           string
	serviceName    string
	objectPath     dbus.ObjectPath
	conn           *dbus.Conn
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

	return &MPRISPlayer{
		Name:           name,
		duration:       time.Duration(seconds) * time.Second,
		objectPath:     "/org/mpris/MediaPlayer2",
		playbackStatus: "Playing",
		tickerDone:     make(chan struct{}),
		Done:           make(chan struct{}, 1),
	}, nil
}

func (p *MPRISPlayer) Start() error {
	conn, err := dbus.SessionBus()
	if err != nil {
		return fmt.Errorf("failed to connect to session bus: %w", err)
	}

	p.conn = conn

	for _, name := range RequestNames {
		reply, err := conn.RequestName(name, dbus.NameFlagDoNotQueue)
		if err == nil && reply == dbus.RequestNameReplyPrimaryOwner {
			p.serviceName = name
			break
		}
	}

	if p.serviceName == "" {
		return fmt.Errorf("could not find free service name")
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
			p.Done <- struct{}{}
			return
		case <-ticker.C:
			if p.isPaused {
				continue
			}

			elapsed := time.Since(p.startTime) - p.pausedFor
			progress := math.Min(100, (float64(elapsed)/float64(p.duration))*100)

			if progress >= 100 {
				p.Done <- struct{}{}
				return
			}

			timeLeft := p.duration - elapsed
			progressImg, err := util.MakeProgressCircle(progress)
			if err != nil {
				log.Printf("Failed to create progress circle: %v", err)
				continue
			}

			metadata := map[string]dbus.Variant{
				"mpris:trackid": dbus.MakeVariant(dbus.ObjectPath("/track/1")),
				"xesam:title":   dbus.MakeVariant(p.Name),
				"xesam:artist":  dbus.MakeVariant([]string{util.FormatDuration(timeLeft)}),
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

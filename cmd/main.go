package main

import (
	"flag"
	"github.com/diamondburned/gotk4-adwaita/pkg/adw"
	"log"
	"mpris-timer/internal/player"
	"mpris-timer/internal/ui"
	"os"
	"os/signal"
)

var (
	useUI     = flag.Bool("ui", false, "Show timepicker UI")
	autoStart = flag.Bool("start", false, "Start immediately")
	title     = flag.String("title", "Timer", "Name/title of the timer")
	duration  = flag.Int("duration", 0, "Duration in seconds")
)

func main() {
	flag.Parse()

	// sanity check
	if *useUI && *autoStart {
		log.Fatalf("UI can't be used with --start")
	}

	// use UI by default
	if !*useUI && !*autoStart {
		*useUI = true
	}

	// show UI
	if *useUI {
		ui.Init(func(app *adw.Application) {
			ui.NewTimePicker(app, duration)
		})
	}

	// check duration
	if *duration <= 0 {
		log.Fatalf("duration must be a positive integer")
	}

	name := "Timer"
	if title != nil {
		name = *title
	}

	// start timer
	timer, err := player.NewMPRISPlayer(*duration, name)
	if err != nil {
		log.Fatalf("Failed to create player: %v", err)
	}

	if err := timer.Init(); err != nil {
		log.Fatalf("Failed to initialize player: %v", err)
	}

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt)

	select {
	case <-sigChan:
		os.Exit(0)
	}
}

package main

import (
	"flag"
	"log"
	"mpris-timer/internal/player"
	"mpris-timer/internal/ui"
	"os"
	"os/signal"
)

var (
	useUI    bool
	duration int
	title    string
)

func main() {
	flag.BoolVar(&useUI, "ui", false, "Show timepicker UI (default)")
	flag.IntVar(&duration, "start", 0, "Start the timer immediadety")
	flag.StringVar(&title, "title", "Timer", "Name/title of the timer")
	flag.Parse()

	// sanity check
	if useUI && duration > 0 {
		log.Fatalf("UI can't be used with --start")
	}

	// use UI by default
	if duration <= 0 && !useUI {
		useUI = true
	}

	// show UI
	if useUI {
		ui.Init(&duration)
	}

	// start timer
	timer, err := player.NewMPRISPlayer(duration, title)
	if err != nil {
		log.Fatalf("Failed to create player: %v", err)
	}

	if err := timer.Start(); err != nil {
		log.Fatalf("Failed to initialize player: %v", err)
	}

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt)

	select {
	case <-sigChan:
		os.Exit(0)
	}
}

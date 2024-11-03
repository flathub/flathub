package main

import (
	"fmt"
	"log"
	"mpris-timer/internal/player"
	"os"
	"os/signal"
	"strconv"
)

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

	timer, err := player.NewMPRISPlayer(seconds, name)
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

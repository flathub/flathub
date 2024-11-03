package ui

import (
	"fmt"
	"log"
	"strconv"
	"time"
)

const defaultPreset = "02:30"

var DefaultPresets = []string{
	"00:30",
	"01:00",
	"01:30",
	"02:00",
	"02:30",
	"03:00",
	"05:00",
	"07:00",
	"10:00",
	"15:00",
	"20:00",
	"30:00",
}

func fromPreset(preset string) time.Time {
	result, err := time.Parse("04:05", preset)
	if err != nil {
		log.Fatalf("failed to parse preset %s: %v", preset, err)
	}

	return result
}

func fromParts(hours int, minutes int, seconds int) time.Time {
	result, err := time.Parse("15:04:05", fmt.Sprintf("%02d:%02d:%02d", hours, minutes, seconds))
	if err != nil {
		log.Fatalf("failed to parse parts %d %d %d: %v", hours, minutes, seconds, err)
	}

	return result
}

func fromStringParts(hours string, minutes string, seconds string) time.Time {
	hoursInt, err := strconv.Atoi(hours)
	if err != nil {
		log.Fatalf("failed to parse hours %s: %v", hours, err)
	}

	minutesInt, err := strconv.Atoi(minutes)
	if err != nil {
		log.Fatalf("failed to parse minutes %s: %v", minutes, err)
	}

	secondsInt, err := strconv.Atoi(seconds)
	if err != nil {
		log.Fatalf("failed to parse seconds %s: %v", seconds, err)
	}

	return fromParts(hoursInt, minutesInt, secondsInt)
}

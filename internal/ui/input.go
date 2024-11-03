package ui

import (
	"github.com/diamondburned/gotk4/pkg/gdk/v4"
	"github.com/diamondburned/gotk4/pkg/gtk/v4"
	"log"
	"mpris-timer/internal/util"
	"slices"
	"strconv"
)

func setupTimeEntry(entry *gtk.Entry, next *gtk.Widget, maxVal int) {
	if maxVal <= 0 {
		maxVal = 59
	}

	entry.SetSensitive(true)
	entry.SetCanFocus(true)
	entry.SetCanTarget(true)
	entry.SetMaxWidthChars(2)
	entry.SetWidthChars(2)
	entry.SetOverflow(gtk.OverflowHidden)
	entry.AddCSSClass("timer-entry")
	entry.SetHExpand(false)
	entry.SetHAlign(gtk.AlignCenter)
	entry.SetVAlign(gtk.AlignCenter)
	entry.SetAlignment(.5)
	entry.SetText("00")
	entry.SelectRegion(0, -1)

	focusCtrl := gtk.NewEventControllerFocus()
	focusCtrl.SetPropagationPhase(gtk.PhaseTarget)
	focusCtrl.ConnectLeave(func() {
		val := entry.Text()
		log.Printf("val: %s", val)

		if len(val) == 0 {
			entry.SetText("00")
		}

		if len(val) == 1 {
			entry.SetText("0" + val)
		}

		entry.SelectRegion(0, 0)
	})

	ctrl := gtk.NewEventControllerKey()
	ctrl.SetPropagationPhase(gtk.PhaseCapture)
	ctrl.SetPropagationLimit(gtk.LimitNone)
	ctrl.ConnectKeyPressed(func(keyval, keycode uint, state gdk.ModifierType) (ok bool) {
		// allow some basic keys
		allowedKeyvals := []uint{
			gdk.KEY_Tab,
			gdk.KEY_ISO_Left_Tab,
			gdk.KEY_3270_BackTab,
			gdk.KEY_ISO_Enter,
			gdk.KEY_3270_Enter,
			gdk.KEY_KP_Enter,
			gdk.KEY_BackSpace,
			gdk.KEY_Delete,
			gdk.KEY_KP_Delete,
			gdk.KEY_Left,
			gdk.KEY_Right,
			gdk.KEY_Up,
			gdk.KEY_Down,
			gdk.KEY_Home,
			gdk.KEY_KP_Home,
			gdk.KEY_End,
			gdk.KEY_KP_End,
		}

		type shortcut struct {
			keyval []uint
			mask   gdk.ModifierType
			fn     func()
		}

		// allow some (unhandled) shortcuts
		allowedShortcuts := []shortcut{
			{
				keyval: []uint{gdk.KEY_a}, // ^A = select all
				mask:   gdk.ControlMask,
			},
		}

		for _, cfg := range allowedShortcuts {
			if slices.Contains(cfg.keyval, keyval) && cfg.mask == state {
				return false
			}
		}

		isNumber := util.IsGdkKeyvalNumber(keyval)
		if !isNumber && !slices.Contains(allowedKeyvals, keyval) {
			return true
		}

		// now we are interested only in numbers
		if !isNumber {
			return false
		}

		val := entry.Text()
		_, _, selectionPresent := entry.SelectionBounds()
		if len(val) >= 2 && !selectionPresent {
			return true
		}

		if len(val) != 1 {
			return false
		}

		// section finished
		newVal, err := strconv.Atoi(val + util.ParseKeyval(keyval))
		if err != nil {
			log.Printf("Error converting keyval to int: %v", err)
			return true
		}

		if newVal > maxVal {
			entry.SetText(util.NumToLabelText(maxVal))
			return true
		}

		return false
	})

	entry.ConnectChanged(func() {
		val := entry.Text()

		if len(val) == 2 {
			next.GrabFocus()
		}
	})

	entry.AddController(ctrl)
	entry.AddController(focusCtrl)
}

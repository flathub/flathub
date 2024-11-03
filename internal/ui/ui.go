package ui

import (
	"github.com/diamondburned/gotk4-adwaita/pkg/adw"
	"github.com/diamondburned/gotk4/pkg/gio/v2"
	"github.com/diamondburned/gotk4/pkg/gtk/v4"
	"log"
	"os"
)

const (
	minWidth      = 400
	minHeight     = 200
	collapseWidth = 450
	defaultWidth  = 550
	defaultHeight = 435
	defaultPreset = "02:30"
)

var (
	DefaultPresets = []string{
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
)

func Init(callback func(app *adw.Application)) {
	log.Println("Started time picker UI")

	app := adw.NewApplication("com.github.diamondburned.gotk4-examples.gtk4.simple", gio.ApplicationDefaultFlags)
	app.ConnectActivate(func() { callback(app) })

	if code := app.Run(nil); code > 0 {
		os.Exit(code)
	}
}

func NewTimePicker(app *adw.Application, result *int) {
	win := adw.NewApplicationWindow(&app.Application)
	box := gtk.NewBox(gtk.OrientationVertical, 0)
	header := adw.NewHeaderBar()
	body := adw.NewOverlaySplitView()

	box.Append(header)
	box.Append(body)

	win.SetContent(&box.Widget)
	win.SetTitle("MPRIS Timer")
	win.SetSizeRequest(minWidth, minHeight)
	win.SetDefaultSize(defaultWidth, defaultHeight)

	bp := adw.NewBreakpoint(adw.NewBreakpointConditionLength(adw.BreakpointConditionMaxWidth, collapseWidth, adw.LengthUnitSp))
	bp.AddSetter(body, "collapsed", true)
	win.AddBreakpoint(bp)

	body.SetVExpand(true)
	body.SetHExpand(true)
	body.SetSidebarPosition(gtk.PackEnd)
	body.SetSidebar(NewSidebar())
	body.SetContent(NewContent())
	body.SetSidebarWidthFraction(.35)

	win.SetVisible(true)
	*result = 0
}

func NewSidebar() *adw.NavigationPage {
	sidebar := adw.NewNavigationPage(gtk.NewBox(gtk.OrientationVertical, 0), "Presets")
	flowBox := gtk.NewFlowBox()

	flowBox.SetVAlign(gtk.AlignCenter)
	flowBox.SetColumnSpacing(16)
	flowBox.SetRowSpacing(16)
	flowBox.SetMarginStart(24)
	flowBox.SetMarginEnd(24)
	flowBox.SetMarginTop(24)
	flowBox.SetMarginBottom(24)

	for _, preset := range DefaultPresets {
		label := gtk.NewLabel(preset)
		label.SetCanFocus(true)
		label.SetSensitive(true)
		label.SetCanTarget(true)
		label.SetMarginStart(8)
		label.SetMarginEnd(8)
		label.SetMarginTop(8)
		label.SetMarginBottom(8)
		label.SetHAlign(gtk.AlignCenter)
		label.SetVAlign(gtk.AlignCenter)

		flowBox.Append(label)

		if preset == defaultPreset {
			label.GrabFocus()
		}
	}

	scrolledWindow := gtk.NewScrolledWindow()
	scrolledWindow.SetVExpand(true)
	scrolledWindow.SetOverlayScrolling(true)
	scrolledWindow.SetMinContentHeight(minHeight)
	scrolledWindow.SetChild(flowBox)

	sidebar.SetChild(scrolledWindow)

	return sidebar
}

func NewContent() *adw.NavigationPage {
	clamp := adw.NewClamp()
	box := gtk.NewBox(gtk.OrientationVertical, 0)
	content := adw.NewNavigationPage(clamp, "New timer")

	clamp.SetChild(box)

	return content
}

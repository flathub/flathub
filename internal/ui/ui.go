package ui

import (
	_ "embed"
	"fmt"
	"github.com/diamondburned/gotk4-adwaita/pkg/adw"
	"github.com/diamondburned/gotk4/pkg/gdk/v4"
	"github.com/diamondburned/gotk4/pkg/gio/v2"
	"github.com/diamondburned/gotk4/pkg/gtk/v4"
	"log"
	"os"
)

//go:embed style.css
var cssString string

const (
	minWidth      = 400
	minHeight     = 200
	collapseWidth = 460
	defaultWidth  = 550
	defaultHeight = 250
)

var (
	win          *adw.ApplicationWindow
	hoursLabel   *gtk.Label
	minutesLabel *gtk.Label
	secondsLabel *gtk.Label
	startBtn     *gtk.Button
)

func Init(result *int) {
	log.Println("started time picker UI")

	app := adw.NewApplication("com.efogdev.mpris-timer", gio.ApplicationDefaultFlags)
	app.ConnectActivate(func() {
		prov := gtk.NewCSSProvider()
		prov.ConnectParsingError(func(sec *gtk.CSSSection, err error) {
			log.Printf("CSS error: %v", err)
		})

		prov.LoadFromString(cssString)
		gtk.StyleContextAddProviderForDisplay(gdk.DisplayGetDefault(), prov, gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

		NewTimePicker(app, result)
	})

	if code := app.Run(nil); code > 0 {
		os.Exit(code)
	}
}

func NewTimePicker(app *adw.Application, result *int) {
	if result == nil {
		log.Fatalf("invalid result pointer")
	}

	win = adw.NewApplicationWindow(&app.Application)
	box := gtk.NewBox(gtk.OrientationVertical, 0)
	header := adw.NewHeaderBar()
	body := adw.NewOverlaySplitView()

	box.Append(header)
	box.Append(body)

	win.SetContent(box)
	win.SetTitle("MPRIS Timer")
	win.SetSizeRequest(minWidth, minHeight)
	win.SetDefaultSize(defaultWidth, defaultHeight)

	bp := adw.NewBreakpoint(adw.NewBreakpointConditionLength(adw.BreakpointConditionMaxWidth, collapseWidth, adw.LengthUnitSp))
	bp.AddSetter(body, "collapsed", true)
	win.AddBreakpoint(bp)

	body.SetVExpand(true)
	body.SetHExpand(true)
	body.SetSidebarPosition(gtk.PackEnd)
	body.SetContent(NewContent(result))
	body.SetSidebar(NewSidebar(result))
	body.SetSidebarWidthFraction(.35)

	win.SetVisible(true)
	*result = 0
}

func NewSidebar(result *int) *adw.NavigationPage {
	sidebar := adw.NewNavigationPage(gtk.NewBox(gtk.OrientationVertical, 0), "Presets")
	flowBox := gtk.NewFlowBox()

	flowBox.SetSelectionMode(gtk.SelectionBrowse)
	flowBox.SetVAlign(gtk.AlignCenter)
	flowBox.SetColumnSpacing(16)
	flowBox.SetRowSpacing(16)
	flowBox.AddCSSClass("flow-box")

	for idx, preset := range DefaultPresets {
		label := gtk.NewLabel(preset)
		label.SetCursorFromName("pointer")
		label.AddCSSClass("preset-lbl")
		label.SetHAlign(gtk.AlignCenter)
		label.SetVAlign(gtk.AlignCenter)

		flowBox.Append(label)

		child := flowBox.ChildAtIndex(idx)
		child.ConnectActivate(func() {
			time := fromPreset(preset)
			*result = time.Minute()*60 + time.Second()

			if hoursLabel == nil || minutesLabel == nil || secondsLabel == nil {
				return
			}

			hoursLabel.SetText(NumToLabelText(0))
			minutesLabel.SetText(NumToLabelText(time.Minute()))
			secondsLabel.SetText(NumToLabelText(time.Second()))
			startBtn.SetCanFocus(true)
			startBtn.GrabFocus()
		})

		if preset == defaultPreset {
			flowBox.SelectChild(child)
		}
	}

	scrolledWindow := gtk.NewScrolledWindow()
	scrolledWindow.SetVExpand(true)
	scrolledWindow.SetOverlayScrolling(true)
	scrolledWindow.SetMinContentHeight(minHeight)
	scrolledWindow.SetChild(flowBox)
	sidebar.SetChild(scrolledWindow)
	sidebar.SetReceivesDefault(true)

	return sidebar
}

func NewContent(result *int) *adw.NavigationPage {
	vBox := gtk.NewBox(gtk.OrientationVertical, 8)
	hBox := gtk.NewBox(gtk.OrientationHorizontal, 8)
	clamp := adw.NewClamp()
	content := adw.NewNavigationPage(clamp, "New timer")

	clamp.SetChild(vBox)
	vBox.Append(hBox)

	hoursLabel = gtk.NewLabel("00")
	minutesLabel = gtk.NewLabel("00")
	secondsLabel = gtk.NewLabel("00")

	hoursLabel.AddCSSClass("timer-lbl")
	minutesLabel.AddCSSClass("timer-lbl")
	secondsLabel.AddCSSClass("timer-lbl")

	hBox.Append(hoursLabel)
	hBox.Append(gtk.NewLabel(":"))
	hBox.Append(minutesLabel)
	hBox.Append(gtk.NewLabel(":"))
	hBox.Append(secondsLabel)

	hBox.SetVAlign(gtk.AlignCenter)
	hBox.SetHAlign(gtk.AlignCenter)
	hBox.SetVExpand(true)
	hBox.SetHExpand(true)

	btnContent := adw.NewButtonContent()
	btnContent.SetHExpand(false)
	btnContent.SetLabel("Start")
	btnContent.SetIconName("play")

	startBtn = gtk.NewButton()
	startBtn.SetCanFocus(false)
	startBtn.SetChild(btnContent)
	startBtn.SetHExpand(false)
	startBtn.AddCSSClass("control-btn")
	startBtn.ConnectActivate(func() {
		time := fromStringParts(hoursLabel.Label(), minutesLabel.Label(), secondsLabel.Label())
		*result = time.Hour()*60*60 + time.Minute()*60 + time.Second()
		win.Close()
	})

	footer := gtk.NewBox(gtk.OrientationHorizontal, 8)
	footer.SetVAlign(gtk.AlignCenter)
	footer.SetHAlign(gtk.AlignCenter)
	footer.Append(startBtn)
	vBox.Append(footer)

	return content
}

func NumToLabelText(num int) string {
	if num > 60 || num < 0 {
		log.Fatalf("NumToLabelText: num must be between 0 and 60")
	}

	return fmt.Sprintf("%02d", num)
}

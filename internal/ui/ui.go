package ui

import (
	_ "embed"
	"github.com/diamondburned/gotk4-adwaita/pkg/adw"
	"github.com/diamondburned/gotk4/pkg/gdk/v4"
	"github.com/diamondburned/gotk4/pkg/gio/v2"
	"github.com/diamondburned/gotk4/pkg/gtk/v4"
	"log"
	"mpris-timer/internal/util"
	"os"
)

//go:embed style.css
var cssString string

const (
	minWidth      = 400
	minHeight     = 210
	collapseWidth = 500
	defaultWidth  = 550
	defaultHeight = 210
)

var (
	win           *adw.ApplicationWindow
	initialPreset *gtk.FlowBoxChild
	hoursLabel    *gtk.Entry
	minutesLabel  *gtk.Entry
	secondsLabel  *gtk.Entry
	startBtn      *gtk.Button
)

func Init(result *int) {
	log.Println("started time picker UI")

	app := adw.NewApplication("io.github.efogdev.mpris-timer", gio.ApplicationNonUnique)
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

	*result = 0
	win = adw.NewApplicationWindow(&app.Application)
	handle := gtk.NewWindowHandle()
	body := adw.NewOverlaySplitView()
	handle.SetChild(body)

	escCtrl := gtk.NewEventControllerKey()
	escCtrl.SetPropagationPhase(gtk.PhaseCapture)
	escCtrl.ConnectKeyPressed(func(keyval, keycode uint, state gdk.ModifierType) (ok bool) {
		if keyval != gdk.KEY_Escape {
			return false
		}

		win.Close()
		os.Exit(0)
		return true
	})

	win.AddController(escCtrl)
	win.SetContent(handle)
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
	body.SetEnableShowGesture(true)
	body.SetEnableHideGesture(true)

	win.SetVisible(true)
	initialPreset.Activate()
	minutesLabel.SetText("00")
	secondsLabel.SetText("00")

	initialPreset.GrabFocus()
}

func NewSidebar(_ *int) *adw.NavigationPage {
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

		onActivate := func() {
			time := fromPreset(preset)

			if hoursLabel == nil || minutesLabel == nil || secondsLabel == nil {
				return
			}

			hoursLabel.SetText(util.NumToLabelText(0))
			minutesLabel.SetText(util.NumToLabelText(time.Minute()))
			secondsLabel.SetText(util.NumToLabelText(time.Second()))
			startBtn.SetCanFocus(true)
			startBtn.GrabFocus()
		}

		mouseCtrl := gtk.NewGestureClick()
		mouseCtrl.ConnectPressed(func(nPress int, x, y float64) {
			onActivate()
		})

		child := flowBox.ChildAtIndex(idx)
		child.ConnectActivate(onActivate)
		child.AddController(mouseCtrl)

		if preset == defaultPreset {
			flowBox.SelectChild(child)
			initialPreset = child
		}
	}

	scrolledWindow := gtk.NewScrolledWindow()
	scrolledWindow.SetVExpand(true)
	scrolledWindow.SetOverlayScrolling(true)
	scrolledWindow.SetMinContentHeight(minHeight)
	scrolledWindow.SetChild(flowBox)

	kbCtrl := gtk.NewEventControllerKey()
	kbCtrl.SetPropagationPhase(gtk.PhaseBubble)
	kbCtrl.ConnectKeyPressed(func(keyval, keycode uint, state gdk.ModifierType) (ok bool) {
		isNumber := util.IsGdkKeyvalNumber(keyval)
		if !isNumber {
			return false
		}

		minutesLabel.SetText(util.ParseKeyval(keyval))
		minutesLabel.Activate()
		minutesLabel.GrabFocus()
		minutesLabel.SelectRegion(1, 1)

		return true
	})

	sidebar.SetChild(scrolledWindow)
	sidebar.AddController(kbCtrl)

	return sidebar
}

func NewContent(result *int) *adw.NavigationPage {
	startBtn = gtk.NewButton()

	vBox := gtk.NewBox(gtk.OrientationVertical, 8)
	hBox := gtk.NewBox(gtk.OrientationHorizontal, 8)
	clamp := adw.NewClamp()
	content := adw.NewNavigationPage(clamp, "New timer")

	clamp.SetChild(vBox)
	vBox.Append(hBox)

	hoursLabel = gtk.NewEntry()
	hoursLabel.AddCSSClass("entry")

	minutesLabel = gtk.NewEntry()
	minutesLabel.AddCSSClass("entry")

	secondsLabel = gtk.NewEntry()
	secondsLabel.AddCSSClass("entry")

	setupTimeEntry(hoursLabel, &minutesLabel.Widget, 23)
	setupTimeEntry(minutesLabel, &secondsLabel.Widget, 59)
	setupTimeEntry(secondsLabel, &startBtn.Widget, 59)

	scLabel1 := gtk.NewLabel(":")
	scLabel1.AddCSSClass("sc-label")

	scLabel2 := gtk.NewLabel(":")
	scLabel2.AddCSSClass("sc-label")

	hBox.Append(hoursLabel)
	hBox.Append(scLabel1)
	hBox.Append(minutesLabel)
	hBox.Append(scLabel2)
	hBox.Append(secondsLabel)

	hBox.SetVAlign(gtk.AlignCenter)
	hBox.SetHAlign(gtk.AlignCenter)
	hBox.SetVExpand(true)
	hBox.SetHExpand(true)

	btnContent := adw.NewButtonContent()
	btnContent.SetHExpand(false)
	btnContent.SetLabel("Start")
	btnContent.SetIconName("play")

	startBtn.SetCanFocus(false)
	startBtn.SetChild(btnContent)
	startBtn.SetHExpand(false)
	startBtn.AddCSSClass("control-btn")
	startBtn.AddCSSClass("suggested-action")

	startFn := func() {
		time := fromStringParts(hoursLabel.Text(), minutesLabel.Text(), secondsLabel.Text())
		seconds := time.Hour()*60*60 + time.Minute()*60 + time.Second()
		if seconds > 0 {
			*result = seconds
			win.Close()
			return
		}

		os.Exit(1)
	}

	startBtn.ConnectClicked(startFn)
	startBtn.ConnectActivate(startFn)

	footer := gtk.NewBox(gtk.OrientationHorizontal, 8)
	footer.SetVAlign(gtk.AlignCenter)
	footer.SetHAlign(gtk.AlignCenter)
	footer.SetMarginBottom(16)
	footer.Append(startBtn)
	vBox.Append(footer)

	return content
}

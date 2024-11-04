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
	minHeight     = 200
	collapseWidth = 460
	defaultWidth  = 550
	defaultHeight = 250
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

	*result = 0
	win = adw.NewApplicationWindow(&app.Application)
	box := gtk.NewBox(gtk.OrientationVertical, 0)
	header := adw.NewHeaderBar()
	body := adw.NewOverlaySplitView()

	box.Append(header)
	box.Append(body)

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
	initialPreset.Activate()
	initialPreset.GrabFocus()
}

func NewSidebar(_ *int) *adw.NavigationPage {
	sidebar := adw.NewNavigationPage(gtk.NewBox(gtk.OrientationVertical, 0), "Presets")
	presetsFlowBox := gtk.NewFlowBox()

	presetsFlowBox.SetSelectionMode(gtk.SelectionBrowse)
	presetsFlowBox.SetVAlign(gtk.AlignCenter)
	presetsFlowBox.SetColumnSpacing(16)
	presetsFlowBox.SetRowSpacing(16)
	presetsFlowBox.AddCSSClass("flow-box")

	for idx, preset := range DefaultPresets {
		label := gtk.NewLabel(preset)
		label.SetCursorFromName("pointer")
		label.AddCSSClass("preset-lbl")
		label.SetHAlign(gtk.AlignCenter)
		label.SetVAlign(gtk.AlignCenter)
		presetsFlowBox.Append(label)

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

		child := presetsFlowBox.ChildAtIndex(idx)
		child.ConnectActivate(onActivate)
		child.AddController(mouseCtrl)

		if preset == defaultPreset {
			presetsFlowBox.SelectChild(child)
			initialPreset = child
		}
	}

	scrolledWindow := gtk.NewScrolledWindow()
	scrolledWindow.SetVExpand(true)
	scrolledWindow.SetOverlayScrolling(true)
	scrolledWindow.SetMinContentHeight(minHeight)
	scrolledWindow.SetChild(presetsFlowBox)

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

	startBtn.SetCanFocus(false)
	startBtn.SetChild(btnContent)
	startBtn.SetHExpand(false)
	startBtn.AddCSSClass("control-btn")

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
	footer.Append(startBtn)
	vBox.Append(footer)

	return content
}

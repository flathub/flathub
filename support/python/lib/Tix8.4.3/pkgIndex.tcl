if {[catch {package require Tcl 8.4}]} return
package ifneeded Tix 8.4.3 \
    [list load [file join $dir libTix8.4.3.a] Tix]

#!/usr/bin/env python3
"""
Currency Converter — GTK4 desktop app
API: https://xe.otek.today/FROM/TO/1
Cache: 6 hours per pair
"""

import gi
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, GLib

import urllib.request
import json
import time
import threading
import os

# ── Cache stored in ~/.cache/currency_converter.json ──────────────────────────
CACHE_FILE = os.path.expanduser("~/.cache/currency_converter.json")
CACHE_TTL  = 6 * 3600   # 6 hours in seconds
API_BASE   = "https://xe.otek.today"

# 1. SORT CURRENCIES ALPHABETICALLY BY CODE
CURRENCIES = sorted([
("AED","United Arab Emirates dirham"),
("AFN","Afghan afghani"),
("ALL","Albanian lek"),
("AMD","Armenian dram"),
("AOA","Angolan kwanza"),
("ARS","Argentine peso"),
("AUD","Australian dollar"),
("AWG","Aruban florin"),
("AZN","Azerbaijani manat"),
("BAM","Bosnia and Herzegovina convertible mark"),
("BBD","Barbados dollar"),
("BDT","Bangladeshi taka"),
("BHD","Bahraini dinar"),
("BIF","Burundian franc"),
("BMD","Bermudian dollar"),
("BND","Brunei dollar"),
("BOB","Boliviano"),
("BOV","Bolivian Mvdol"),
("BRL","Brazilian real"),
("BSD","Bahamian dollar"),
("BTN","Bhutanese ngultrum"),
("BWP","Botswana pula"),
("BYN","Belarusian ruble"),
("BZD","Belize dollar"),
("CAD","Canadian dollar"),
("CDF","Congolese franc"),
("CHE","WIR euro"),
("CHF","Swiss franc"),
("CHW","WIR franc"),
("CLF","Unidad de Fomento"),
("CLP","Chilean peso"),
("CNY","Renminbi"),
("COP","Colombian peso"),
("COU","Unidad de Valor Real"),
("CRC","Costa Rican colon"),
("CUP","Cuban peso"),
("CVE","Cape Verdean escudo"),
("CZK","Czech koruna"),
("DJF","Djiboutian franc"),
("DKK","Danish krone"),
("DOP","Dominican peso"),
("DZD","Algerian dinar"),
("EGP","Egyptian pound"),
("ERN","Eritrean nakfa"),
("ETB","Ethiopian birr"),
("EUR","Euro"),
("FJD","Fiji dollar"),
("FKP","Falkland Islands pound"),
("GBP","Pound sterling"),
("GEL","Georgian lari"),
("GHS","Ghanaian cedi"),
("GIP","Gibraltar pound"),
("GMD","Gambian dalasi"),
("GNF","Guinean franc"),
("GTQ","Guatemalan quetzal"),
("GYD","Guyanese dollar"),
("HKD","Hong Kong dollar"),
("HNL","Honduran lempira"),
("HTG","Haitian gourde"),
("HUF","Hungarian forint"),
("IDR","Indonesian rupiah"),
("ILS","Israeli new shekel"),
("INR","Indian rupee"),
("IQD","Iraqi dinar"),
("IRR","Iranian rial"),
("ISK","Icelandic króna"),
("JMD","Jamaican dollar"),
("JOD","Jordanian dinar"),
("JPY","Japanese yen"),
("KES","Kenyan shilling"),
("KGS","Kyrgyzstani som"),
("KHR","Cambodian riel"),
("KMF","Comoro franc"),
("KPW","North Korean won"),
("KRW","South Korean won"),
("KWD","Kuwaiti dinar"),
("KYD","Cayman Islands dollar"),
("KZT","Kazakhstani tenge"),
("LAK","Lao kip"),
("LBP","Lebanese pound"),
("LKR","Sri Lankan rupee"),
("LRD","Liberian dollar"),
("LSL","Lesotho loti"),
("LYD","Libyan dinar"),
("MAD","Moroccan dirham"),
("MDL","Moldovan leu"),
("MGA","Malagasy ariary"),
("MKD","Macedonian denar"),
("MMK","Myanmar kyat"),
("MNT","Mongolian tögrög"),
("MOP","Macanese pataca"),
("MRU","Mauritanian ouguiya"),
("MUR","Mauritian rupee"),
("MVR","Maldivian rufiyaa"),
("MWK","Malawian kwacha"),
("MXN","Mexican peso"),
("MXV","Mexican Unidad de Inversion"),
("MYR","Malaysian ringgit"),
("MZN","Mozambican metical"),
("NAD","Namibian dollar"),
("NGN","Nigerian naira"),
("NIO","Nicaraguan córdoba"),
("NOK","Norwegian krone"),
("NPR","Nepalese rupee"),
("NZD","New Zealand dollar"),
("OMR","Omani rial"),
("PAB","Panamanian balboa"),
("PEN","Peruvian sol"),
("PGK","Papua New Guinean kina"),
("PHP","Philippine peso"),
("PKR","Pakistani rupee"),
("PLN","Polish złoty"),
("PYG","Paraguayan guaraní"),
("QAR","Qatari riyal"),
("RON","Romanian leu"),
("RSD","Serbian dinar"),
("RUB","Russian ruble"),
("RWF","Rwandan franc"),
("SAR","Saudi riyal"),
("SBD","Solomon Islands dollar"),
("SCR","Seychelles rupee"),
("SDG","Sudanese pound"),
("SEK","Swedish krona"),
("SGD","Singapore dollar"),
("SHP","Saint Helena pound"),
("SLE","Sierra Leonean leone"),
("SOS","Somalian shilling"),
("SRD","Surinamese dollar"),
("SSP","South Sudanese pound"),
("STN","São Tomé and Príncipe dobra"),
("SVC","Salvadoran colón"),
("SYP","Syrian pound"),
("SZL","Swazi lilangeni"),
("THB","Thai baht"),
("TJS","Tajikistani somoni"),
("TMT","Turkmenistan manat"),
("TND","Tunisian dinar"),
("TOP","Tongan paʻanga"),
("TRY","Turkish lira"),
("TTD","Trinidad and Tobago dollar"),
("TWD","New Taiwan dollar"),
("TZS","Tanzanian shilling"),
("UAH","Ukrainian hryvnia"),
("UGX","Ugandan shilling"),
("USD","United States dollar"),
("USN","United States dollar - next day"),
("UYI","Uruguay Peso en Unidades Indexadas"),
("UYU","Uruguayan peso"),
("UYW","Unidad previsional"),
("UZS","Uzbekistani sum"),
("VED","Venezuelan digital bolívar"),
("VES","Venezuelan sovereign bolívar"),
("VND","Vietnamese đồng"),
("VUV","Vanuatu vatu"),
("WST","Samoan tala"),
("XAD","Arab Accounting Dinar"),
("XAF","CFA franc BEAC"),
("XAG","Silver - one troy ounce"),
("XAU","Gold - one troy ounce"),
("XBA","European Composite Unit - EURCO - bond market unit"),
("XBB","European Monetary Unit - E.M.U.-6- bond market unit"),
("XBC","European Unit of Account 9 - E.U.A.-9- bond market unit"),
("XBD","European Unit of Account 17 - E.U.A.-17- bond market unit"),
("XCD","East Caribbean dollar"),
("XCG","Caribbean guilder"),
("XDR","Special drawing rights"),
("XOF","CFA franc BCEAO"),
("XPD","Palladium - one troy ounce"),
("XPF","CFP franc - franc Pacifique"),
("XPT","Platinum - one troy ounce"),
("XSU","SUCRE"),
("XTS","Code reserved for testing"),
("XUA","ADB Unit of Account"),
("XXX","No currency"),
("YER","Yemeni rial"),
("ZAR","South African rand"),
("ZMW","Zambian kwacha"),
("ZWG","Zimbabwe Gold"),
], key=lambda x: x[0])

# ── Cache helpers ──────────────────────────────────────────────────────────────

def load_cache():
    try:
        with open(CACHE_FILE, "r") as f:
            return json.load(f)
    except Exception:
        return {}

def save_cache(cache):
    try:
        with open(CACHE_FILE, "w") as f:
            json.dump(cache, f)
    except Exception:
        pass

def get_rate(frm, to, callback):
    """Fetch rate in a background thread, call callback(rate_or_None) on main thread."""
    if frm == to:
        GLib.idle_add(callback, 1.0)
        return

    cache = load_cache()
    key   = f"{frm}_{to}"
    now   = time.time()

    if key in cache and (now - cache[key]["ts"]) < CACHE_TTL:
        GLib.idle_add(callback, cache[key]["rate"])
        return

    def fetch():
        try:
            url = f"{API_BASE}/{frm}/{to}/1"
            req = urllib.request.Request(
                url,
                headers={"User-Agent": "Mozilla/5.0 (X11; Linux x86_64)"}
            )
            with urllib.request.urlopen(req, timeout=8) as r:
                body = r.read().decode().strip()
            rate = float(body)
            cache[key] = {"rate": rate, "ts": now}
            save_cache(cache)
            GLib.idle_add(callback, rate)
        except Exception as e:
            print(f"[currency_converter] fetch error: {e}")
            GLib.idle_add(callback, None)

    threading.Thread(target=fetch, daemon=True).start()

# ── GTK Application ────────────────────────────────────────────────────────────

class CurrencyConverter(Gtk.ApplicationWindow):

    def __init__(self, app):
        super().__init__(application=app, title="Currency Converter")
        self.set_default_size(680, 400)
        self.set_resizable(True)

        self._rate       = None
        self._updating   = False   # guard against recursive signals

        self._build_ui()
        self._apply_css()
        self._fetch_rate()

    # ── UI construction ────────────────────────────────────────────────────────

    def _build_ui(self):
        root = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        root.set_margin_top(32)
        root.set_margin_bottom(32)
        root.set_margin_start(32)
        root.set_margin_end(32)
        self.set_child(root)

        # Title
        title = Gtk.Label(label="Currency Converter")
        title.add_css_class("title")
        root.append(title)

        # Spacer
        spacer = Gtk.Box()
        spacer.set_vexpand(True)
        root.append(spacer)

        # ── Row 1: dropdowns ──────────────────────────────────────────────────
        dd_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        dd_row.set_halign(Gtk.Align.CENTER)
        root.append(dd_row)

        self.dd_from = self._make_dropdown()
        self.dd_to   = self._make_dropdown()

        self._set_dropdown_by_code(self.dd_from, "USD")
        self._set_dropdown_by_code(self.dd_to,   "TRY")

        dd_row.append(self.dd_from)
        dd_row.append(self.dd_to)

        # ── Swap button ───────────────────────────────────────────────────────
        swap_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
        swap_row.set_halign(Gtk.Align.CENTER)
        swap_row.set_margin_top(10)
        swap_row.set_margin_bottom(10)
        root.append(swap_row)

        self.btn_swap = Gtk.Button(label="⇄  Swap")
        self.btn_swap.add_css_class("swap-btn")
        self.btn_swap.connect("clicked", self._on_swap)
        swap_row.append(self.btn_swap)

        # ── Row 2: amount fields ──────────────────────────────────────────────
        amt_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=32)
        amt_row.set_halign(Gtk.Align.CENTER)
        root.append(amt_row)

        self.entry_from = self._make_entry()
        self.entry_to   = self._make_entry()
        self.entry_from.set_text("1.0000")

        amt_row.append(self.entry_from)
        amt_row.append(self.entry_to)

        # ── Status label ──────────────────────────────────────────────────────
        self.lbl_status = Gtk.Label(label="")
        self.lbl_status.add_css_class("status")
        self.lbl_status.set_margin_top(14)
        self.lbl_status.set_halign(Gtk.Align.CENTER)
        root.append(self.lbl_status)

        # Spacer
        spacer2 = Gtk.Box()
        spacer2.set_vexpand(True)
        root.append(spacer2)

        # ── Connect signals ───────────────────────────────────────────────────
        self.dd_from.connect("notify::selected", self._on_dropdown_changed)
        self.dd_to.connect("notify::selected",   self._on_dropdown_changed)
        self.entry_from.connect("changed", self._on_from_changed)
        self.entry_to.connect("changed",   self._on_to_changed)

    def _make_dropdown(self):
        codes = Gtk.StringList()
        for code, name in CURRENCIES:
            codes.append(f"{code} — {name}")
        
        dd = Gtk.DropDown(model=codes)
        
        # 2. ENABLE NATIVE GTK4 SEARCH CAPABILITY
        dd.set_enable_search(True)
        # Tell the search algorithm to evaluate the item's string representation
        expression = Gtk.PropertyExpression.new(Gtk.StringObject, None, "string")
        dd.set_expression(expression)

        dd.set_size_request(300, 44)
        dd.add_css_class("currency-drop")
        return dd

    def _make_entry(self):
        e = Gtk.Entry()
        e.set_size_request(220, 44)
        e.set_input_purpose(Gtk.InputPurpose.NUMBER)
        e.set_alignment(0.5)
        e.add_css_class("amount-entry")
        return e

    def _set_dropdown_by_code(self, dd, code):
        for i, (c, _) in enumerate(CURRENCIES):
            if c == code:
                dd.set_selected(i)
                return

    def _selected_code(self, dd):
        return CURRENCIES[dd.get_selected()][0]

    # ── CSS ────────────────────────────────────────────────────────────────────

    def _apply_css(self):
        css = b"""
        window {
            background-color: #f5f5f5;
        }
        label.title {
            font-size: 22px;
            font-weight: 500;
            color: #222;
            margin-bottom: 8px;
        }
        .currency-drop {
            border-radius: 8px;
            min-height: 44px;
        }
        .swap-btn {
            background: #eeeef8;
            color: #3d3d8f;
            border-radius: 20px;
            padding: 6px 24px;
            font-size: 14px;
            border: none;
            box-shadow: none;
        }
        .swap-btn:hover {
            background: #ddddf0;
        }
        .amount-entry {
            font-size: 20px;
            border: none;
            border-bottom: 2px solid #ccc;
            border-radius: 0;
            background: transparent;
            padding: 4px 8px;
            min-width: 180px;
        }
        .amount-entry:focus {
            border-bottom-color: #3d3d8f;
        }
        label.status {
            font-size: 12px;
            color: #888;
        }
        label.status.error {
            color: #cc3333;
        }
        """
        provider = Gtk.CssProvider()
        provider.load_from_data(css)
        Gtk.StyleContext.add_provider_for_display(
            self.get_display(),
            provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    # ── Logic ──────────────────────────────────────────────────────────────────

    def _fetch_rate(self):
        frm = self._selected_code(self.dd_from)
        to  = self._selected_code(self.dd_to)
        self.lbl_status.set_text("Fetching rate…")
        self.lbl_status.remove_css_class("error")
        get_rate(frm, to, self._on_rate_ready)

    def _on_rate_ready(self, rate):
        if rate is None:
            self.lbl_status.set_text("⚠ Could not fetch rate. Check your connection.")
            self.lbl_status.add_css_class("error")
            self._rate = None
            return

        self._rate = rate
        frm = self._selected_code(self.dd_from)
        to  = self._selected_code(self.dd_to)
        self.lbl_status.set_text(f"1 {frm} = {rate:.6f} {to}  •  rates cached 6h")
        self.lbl_status.remove_css_class("error")
        self._recalc_from()

    def _recalc_from(self):
        """Re-compute 'to' field based on 'from' field value and current rate."""
        if self._rate is None:
            return
        try:
            amount = float(self.entry_from.get_text().replace(",", "."))
        except ValueError:
            return
        self._updating = True
        self.entry_to.set_text(f"{amount * self._rate:.4f}")
        self._updating = False

    def _recalc_to(self):
        """Re-compute 'from' field based on 'to' field value and current rate."""
        if self._rate is None or self._rate == 0:
            return
        try:
            amount = float(self.entry_to.get_text().replace(",", "."))
        except ValueError:
            return
        self._updating = True
        self.entry_from.set_text(f"{amount / self._rate:.4f}")
        self._updating = False

    # ── Signal handlers ────────────────────────────────────────────────────────

    def _on_dropdown_changed(self, *_):
        self._fetch_rate()

    def _on_from_changed(self, *_):
        if not self._updating:
            self._recalc_from()

    def _on_to_changed(self, *_):
        if not self._updating:
            self._recalc_to()

    def _on_swap(self, *_):
        fi = self.dd_from.get_selected()
        ti = self.dd_to.get_selected()
        self._updating = True
        self.dd_from.set_selected(ti)
        self.dd_to.set_selected(fi)
        self._updating = False
        self._fetch_rate()


# ── Entry point ────────────────────────────────────────────────────────────────

class App(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="today.otek.currency_converter")

    def do_activate(self):
        win = CurrencyConverter(self)
        win.present()


if __name__ == "__main__":
    App().run()
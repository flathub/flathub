# window.py
#
# Copyright 2024 omerkurt
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

import codecs

import gi

gi.require_version("Gst", "1.0")
gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Adw
from gi.repository import Gtk, Gst, Gio, GObject, GLib, GdkPixbuf, Gdk
import threading
import asyncio
import aiohttp
import json
import re
import random
import ast  # Güvenli bir şekilde string'den Python nesnesine dönüşüm için
import requests
from io import BytesIO
from bs4 import BeautifulSoup
from collections import namedtuple
from datetime import datetime, timedelta
import os
import subprocess

# class KelimeYonetici:
#     def __init__(self):
#         self.data_dir = os.getenv('XDG_DATA_HOME', os.path.join(os.path.expanduser('~'), '.local', 'share'))
#         self.file_path = os.path.join(self.data_dir, 'kelimeler.json')
#         self.data = {
#             "kelimeler": []
#         }
#         self.dosyayi_yukle()
#         print(self.data_dir)
#     def dosyayi_yukle(self):
# Eğer dosya varsa, içeriğini yükle
#         try:
#             with open(self.file_path, 'r', encoding='utf-8') as file:
#                 self.data = json.load(file)
#         except FileNotFoundError:
#             pass

#     def kelime_ekle(self, kelime):
# Kelimeyi listeye ekle
#         self.data["kelimeler"].insert(0,kelime)
# Değişiklikleri dosyaya kaydet
#         self.dosyayi_kaydet()

#     def dosyayi_kaydet(self):
# Veriyi JSON dosyasına yaz
#         os.makedirs(self.data_dir, exist_ok=True)
#         with open(self.file_path, 'w', encoding='utf-8') as file:
#             json.dump(self.data, file, ensure_ascii=False, indent=4)

# Örnek kullanım


# yonetici.kelime_ekle("yeni_kelimse")

Word = namedtuple("Word", ["word", "word2", "word3", "mp3_link"])
BASE_API_URL = "https://tureng.com/tr/turkce-ingilizce/"


class Word(GObject.Object):
    word = GObject.Property(type=str)
    word1 = GObject.Property(type=str)  # Second word column, identical to the first
    word2 = GObject.Property(type=str)  # Third word column, identical to the first
    word3 = GObject.Property(type=str)  # Third word column, identical to the first
    mp3_link = GObject.Property(type=str)  # Third word column, identical to the first

    def __str__(self):
        return f"Word: {self.word}, Word1: {self.word1}, Word2: {self.word2}, Word3: {self.word3}, MP3 Link: {self.mp3_link}"

    def __repr__(self):
        return (
            f"{self.__class__.__name__}(word={self.word}, word1={self.word1}, "
            f"word2={self.word2}, word3={self.word3}, mp3_link={self.mp3_link})"
        )


async def fetch_words_async(search_term):
    # `filtered_data` ve `mp3_links` değişkenlerinin başlangıçta boş olduğunu varsayarak
    filtered_data = []
    mp3_links = {}
    try:
        search_url = f"{BASE_API_URL}{search_term}"
        async with aiohttp.ClientSession() as session:
            headers = {
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.150 Safari/537.36"
            }
            async with session.get(search_url, headers=headers) as response:
                if response.status == 200:
                    html_content = await response.text()
                    soup = BeautifulSoup(html_content, "html.parser")
                    table = soup.find("table")
                    data = []
                    if table:
                        for row in table.find_all("tr"):
                            cols = row.find_all("td")
                            if len(cols) == 0:
                                cols = row.find_all("th")
                            cols = [ele.text.strip() for ele in cols]
                            data.append([ele for ele in cols if ele])
                        data = data[1:] if data else []
                        filtered_data = [item for item in data if len(item) >= 3]

                    mp3_links = {
                        element.get_text(strip=True): element.find("source", src=True)[
                            "src"
                        ]
                        for element in soup.select(".tureng-voice")
                        if element.find("source", src=True)
                        and element.find("source", src=True)["src"].endswith(".mp3")
                    }

                else:
                    print(f"Failed to fetch data, status code: {response.status}")
    except Exception as e:
        print(f"An error occurred: {e}")

    # `filtered_data` ve `mp3_links` başlangıçta tanımlandığı için, bu noktada kullanıma hazırdırlar.
    words = []
    for row in filtered_data:
        word_text = " ".join(row[:4])
        mp3_link = mp3_links.get(word_text, None)
        words.append(
            Word(
                word=row[0], word1=row[1], word2=row[2], word3=row[3], mp3_link=mp3_link
            )
        )
    return words, mp3_links


def fetch_words_column(search_term):
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    words = loop.run_until_complete(fetch_words_async(search_term))
    loop.close()
    return words


class AudioPlayer:
    def __init__(self):
        # Initialize GStreamer
        Gst.init(None)
        self.player = Gst.ElementFactory.make("playbin")

    def play_url(self, url):
        if not url:
            return
        # Stop any currently playing sound
        self.player.set_state(Gst.State.NULL)
        # Set the new URL to play
        self.player.set_property("uri", url)
        # Start playing
        self.player.set_state(Gst.State.PLAYING)


@Gtk.Template(resource_path="/dev/omerkurt/Tureng/window.ui")
class TurengvocabularyWindow(Adw.ApplicationWindow):
    __gtype_name__ = "TurengvocabularyWindow"
    t1 = Gtk.Template.Child()
    t2 = Gtk.Template.Child()
    t3 = Gtk.Template.Child()
    t4 = Gtk.Template.Child()
    t5 = Gtk.Template.Child()
    t6 = Gtk.Template.Child()
    t7 = Gtk.Template.Child()
    t8 = Gtk.Template.Child()
    t9 = Gtk.Template.Child()
    us_play = Gtk.Template.Child()
    uk_play = Gtk.Template.Child()
    au_play = Gtk.Template.Child()

    entry_search = Gtk.Template.Child()
    find_icon = Gtk.Template.Child()
    idiom_image = Gtk.Template.Child()
    coined_image = Gtk.Template.Child()
    toolbar_view = Gtk.Template.Child()
    page3 = Gtk.Template.Child()
    column_view = Gtk.Template.Child()
    col1 = Gtk.Template.Child()
    factory1 = Gtk.Template.Child()
    col2 = Gtk.Template.Child()
    factory2 = Gtk.Template.Child()
    col3 = Gtk.Template.Child()
    factory3 = Gtk.Template.Child()
    col4 = Gtk.Template.Child()
    factory4 = Gtk.Template.Child()
    stack = Gtk.Template.Child()
    label_en = Gtk.Template.Child()
    label_tr = Gtk.Template.Child()
    label_ex = Gtk.Template.Child()
    label_en_example = Gtk.Template.Child()
    label_tr_example = Gtk.Template.Child()
    list_box = Gtk.Template.Child()
    search_status = Gtk.Template.Child()
    svg_logo = Gtk.Template.Child()
    list_box_editable = Gtk.Template.Child()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.audio_player = AudioPlayer()
        self.model = Gio.ListStore(item_type=Word)
        self.initialize_ui()
        self.find_icon.connect("clicked", self.on_button_clicked)

        fetch = self.fetch_words()
        trending_words_fetch = self.trending_words(fetch)
        words_day_fetch = self.words_day(fetch)

        guest = self.guest_list(fetch)
        newlyAddedTerms = self.newlyAddedTerms(fetch)

        self.populate_list_box(newlyAddedTerms)
        self.list_box.connect("row-selected", self.on_row_selected)
        self.list_box_editable.connect("row-selected", self.on_row_selected)
        self.populate_images(fetch)
        self.populate_trending_words(trending_words_fetch)
        self.setup_date_related_ui(words_day_fetch)
        self.configure_style_manager()
        self.populate_list_box_editable([])

    def populate_list_box(self, items):
        for item in items:
            label = Gtk.Label(label=item)
            label.set_margin_top(10)
            label.set_margin_bottom(10)
            row = Gtk.ListBoxRow()
            row.set_child(label)
            self.list_box.append(row)

    def remove_last_prepended_item(self):
        # ListBox'taki en üstteki öğeyi (index 0) al
        row = self.list_box_editable.get_row_at_index(0)
        if row is not None:
            # Eğer öyle bir öğe varsa, onu ListBox'tan kaldır
            self.list_box_editable.remove(row)

    def populate_list_box_editable(self, items):
        for item in items:
            label = Gtk.Label(label=item)
            label.set_margin_top(10)
            label.set_margin_bottom(10)
            row = Gtk.ListBoxRow()
            row.set_child(label)
            self.list_box_editable.prepend(row)

    def populate_images(self, b):
        coined_urls, idiom_urls = self.categorize_urls(b)
        if coined_urls:
            random_coined_url = random.choice(coined_urls)
            self.load_image_from_url(random_coined_url, self.coined_image)
        if idiom_urls:
            random_idiom_url = random.choice(idiom_urls)
            self.load_image_from_url(random_idiom_url, self.idiom_image)

    def populate_trending_words(self, trending_words):
        selected_words = random.sample(trending_words, min(9, len(trending_words)))
        for i, word in enumerate(selected_words):
            button = getattr(self, f"t{i+1}")
            button.get_child().set_text(word)
            button.connect("clicked", self.on_button_clickeds)

    def setup_date_related_ui(self, words_by_day):
        current_date = datetime.now()

        # Iterate over the last 5 days including today
        for i in range(10):
            formatted_date = (current_date - timedelta(days=i)).strftime("%Y-%m-%d")
            result_array = self.get_array_by_date(words_by_day, formatted_date)
            # If data is found for the calculated date, break out of the loop
            if result_array is not None:
                break

        # If result_array is still None after checking the past 5 days, handle the no data case
        if result_array is None:
            utf8_text = "No data available for the past 5 days."
        else:
            # Convert the result_array to UTF-8 text
            utf8_text = self.utf8_decode_list(result_array)

        # Set the UI text to the appropriate message or data
        self.set_date_related_text(utf8_text)

    def set_date_related_text(self, utf8_text):
        self.label_en.set_text(utf8_text[4])
        self.label_tr.set_text(utf8_text[1])
        self.label_ex.set_text(utf8_text[5])
        self.label_en_example.set_text(utf8_text[3])
        self.label_tr_example.set_text(utf8_text[6])

    def configure_style_manager(self):
        style_manager = Adw.StyleManager.get_default()
        current_color_scheme = style_manager.get_color_scheme()
        style_manager.set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)

    # def write_sample_file(self):
    #     data_dir = os.getenv(
    #         "XDG_DATA_HOME", os.path.join(os.path.expanduser("~"), ".local", "share")
    #     )
    #     file_path = os.path.join(data_dir, "history.json")
    #     try:
    #         os.makedirs(data_dir, exist_ok=True)
    #         with open(file_path, "w") as file:
    #             file.write("Merhaba, bu bir test mesajıdır!\n")
    #         print(f"'{file_path}' dizinine yazıldı.")
    #     except Exception as e:
    #         print(f"Dosyaya yazma işlemi sırasında hata oluştu: {e}")

    def on_row_selected(self, listbox, row):
        if row is not None:
            label = row.get_child()
            dizi = [label.get_text()]
            self.entry_search.set_text(label.get_text())
            self.entry_search.emit("activate")

    def on_play_clickeds(self, button, mp3url):
        mp3_url = mp3url
        self.audio_player.play_url(mp3_url)

    def utf8_decode_list(self, text_list):
        # Define a regular expression pattern to match hexadecimal escape sequences
        pattern = r"\\x([0-9a-f]{2})"

        # Define a function to decode each hexadecimal escape sequence
        def decode_hex(match):
            hex_value = match.group(1)  # Get the hexadecimal value
            return chr(int(hex_value, 16))  # Convert hexadecimal to Unicode character

        decoded_text_list = []  # Initialize an empty list to store decoded strings
        for text in text_list:
            # Use re.sub() with a lambda function to apply the decoding to each string
            decoded_text = re.sub(pattern, decode_hex, text)
            decoded_text_list.append(decoded_text)

        return decoded_text_list

    def on_button_clickeds(self, widget):
        trend_word = widget.get_child().get_text()
        self.entry_search.set_text(trend_word)
        self.entry_search.emit("activate")

    def initialize_ui(self):
        self.entry_search.set_placeholder_text("Enter a word to search")
        self.entry_search.connect("activate", self.on_entry_activate)
        self.selection_model = Gtk.SingleSelection(model=self.model)
        self.factory1.connect(
            "setup", lambda fac, list_item: list_item.set_child(Gtk.Label())
        )
        self.factory1.connect(
            "bind",
            lambda fac, list_item: list_item.get_child().set_label(
                list_item.get_item().word
            ),
        )

        self.factory2.connect(
            "setup", lambda fac, list_item: list_item.set_child(Gtk.Label())
        )
        self.factory2.connect(
            "bind",
            lambda fac, list_item: list_item.get_child().set_label(
                list_item.get_item().word1
            ),
        )

        self.factory3.connect(
            "setup", lambda fac, list_item: list_item.set_child(Gtk.Label())
        )
        self.factory3.connect(
            "bind",
            lambda fac, list_item: list_item.get_child().set_label(
                list_item.get_item().word2
            ),
        )

        self.factory4.connect(
            "setup", lambda fac, list_item: list_item.set_child(Gtk.Label())
        )
        self.factory4.connect(
            "bind",
            lambda fac, list_item: list_item.get_child().set_label(
                list_item.get_item().word3
            ),
        )

        self.column_view.set_model(model=self.selection_model)

    def on_entry_activate(self, entry):
        search_term = entry.get_text()
        self.populate_list_box_editable([search_term])
        self.stack.set_visible_child_name("page3")
        if search_term:
            threading.Thread(
                target=self.update_words, args=(search_term,), daemon=True
            ).start()

    def update_words(self, search_term):
        words, mp3_links = fetch_words_column(search_term)
        if len(words) == 0:
            self.search_status.set_title("Not found!")
            self.remove_last_prepended_item()

        else:
            self.search_status.set_title(search_term)

        if mp3_links:
            self.col3.set_title("English")
            self.col4.set_title("Turkish")
            self.us_play.set_visible(True)
            self.uk_play.set_visible(True)
            self.au_play.set_visible(True)
        else:
            self.col3.set_title("Turkish")
            self.col4.set_title("English")
            self.us_play.set_visible(False)
            self.uk_play.set_visible(False)
            self.au_play.set_visible(False)
        self.us_play.connect(
            "clicked",
            lambda button: self.on_play_clickeds(
                button, "https:" + mp3_links["Play ENTRENus"]
            )
            if mp3_links.get("Play ENTRENus")
            else None,
        )
        self.uk_play.connect(
            "clicked",
            lambda button: self.on_play_clickeds(
                button, "https:" + mp3_links["Play ENTRENuk"]
            )
            if mp3_links.get("Play ENTRENuk")
            else None,
        )
        self.au_play.connect(
            "clicked",
            lambda button: self.on_play_clickeds(
                button, "https:" + mp3_links["Play ENTRENau"]
            )
            if mp3_links.get("Play ENTRENau")
            else None,
        )
        GLib.idle_add(lambda: self.model.splice(0, len(self.model), words))

    def on_button_clicked(self, button):
        # Etiketin mevcut görünürlüğünün tersini ayarla
        is_visible = self.entry_search.get_visible()
        self.entry_search.set_visible(not is_visible)

    async def fetch_page_async(self):
        async with aiohttp.ClientSession() as session:
            headers = {
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.150 Safari/537.36"
            }
            async with session.get(
                "https://landing.tureng.com/asset/landing-en-tr.js", headers=headers
            ) as resp:
                if resp.status == 200:
                    js_content = (
                        await resp.text()
                    )  # JavaScript içeriğini metin olarak al
                    return js_content

    def fetch_words(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        words = loop.run_until_complete(self.fetch_page_async())
        loop.close()
        return words

    def trending_words(self, js_content):
        pattern = r"commonTerms=(\[.*?\])"  # Düzenli ifade deseni
        matches = re.search(
            pattern, js_content, re.DOTALL
        )  # JavaScript içeriğinde arama yap
        if matches:
            terms_str = matches.group(1)
            # Güvenli bir şekilde string ifadeyi Python listesine dönüştür
            terms_list = ast.literal_eval(terms_str)
            # Her bir terimi döndürmek yerine, tüm listeyi döndür
            return [term.strip('"') for term in terms_list]
        else:
            print("No match found")
            return []

    def words_day(self, js_content):
        pattern = r"word_of_the_day_list=(\[.*?\])"  # Regular expression pattern
        # Modified pattern to ignore nested quotes
        quote_pattern = r'"([^"\\]*(?:\\.[^"\\]*)*)"'
        try:
            matches = re.search(
                pattern, js_content, re.DOTALL
            )  # Search in JavaScript content
            if not matches:
                raise ValueError("word_of_the_day_list variable not found.")

            terms_str = matches.group(1)
            # Find all expressions inside quotes, ignoring nested quotes
            all_quotes = re.findall(quote_pattern, terms_str, re.DOTALL)
            if not all_quotes:
                raise ValueError("Expressions inside quotes not found.")
            # Grouping into chunks of seven
            chunks = [all_quotes[i : i + 7] for i in range(0, len(all_quotes), 7)]
            return chunks
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            return []

    def guest_list(self, js_content):
        pattern = r"slider_guest_list=(\[.*?\])"  # Regular expression pattern
        quote_pattern = r'"([^"\\]*(?:\\.[^"\\]*)*)"'
        try:
            matches = re.search(
                pattern, js_content, re.DOTALL
            )  # Search in JavaScript content
            if not matches:
                raise ValueError("slider_guest_list variable not found.")

            terms_str = matches.group(1)
            # Find all expressions inside quotes
            all_quotes = re.findall(quote_pattern, terms_str, re.DOTALL)
            if not all_quotes:
                raise ValueError("Expressions inside quotes not found.")

            # Grouping into chunks of three
            chunks = [all_quotes[i : i + 3] for i in range(0, len(all_quotes), 3)]
            return chunks
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            return []  # Return an empty list in case of error

    def newlyAddedTerms(self, js_content):
        pattern = r"commonTerms=(\[.*?\])"  # Düzenli ifade deseni
        matches = re.search(
            pattern, js_content, re.DOTALL
        )  # JavaScript içeriğinde arama yap
        if matches:
            terms_str = matches.group(1)
            # Güvenli bir şekilde string ifadeyi Python listesine dönüştür
            terms_list = ast.literal_eval(terms_str)
            # Her bir terimi döndürmek yerine, tüm listeyi döndür
            return [term.strip('"') for term in terms_list]
        else:
            print("No match found")
            return []

    def categorize_urls(self, js_content):
        # Regex pattern for matching URLs
        pattern = r"https?:\/\/[-a-zA-Z0-9@:%._\+~#=]{2,256}\.[a-z]{2,6}\b[-a-zA-Z0-9@:%_\+.~#?&\/=]*"

        # Find all URLs in the content
        urls = re.findall(pattern, js_content)

        # Initialize lists to hold categorized URLs
        coined_urls = []
        idiom_urls = []

        # Categorize the URLs based on the presence of "coined" or "idiom"
        for url in urls:
            if "coined" in url:
                coined_urls.append(url)
            elif "idiom" in url:
                idiom_urls.append(url)

        return coined_urls, idiom_urls

    def get_array_by_date(self, data, desired_date):
        for sublist in data:
            if sublist[0] == desired_date:
                return sublist
        return None

    def load_image_from_url(self, url, gtk_image_widget):
        """Load an image from a URL and display it in the specified Gtk.Image widget using GTK4."""

        def update_image(data):
            try:
                # Convert data to GLib.Bytes
                gbytes = GLib.Bytes.new(data)
                # Create a Gdk.Texture from GBytes
                texture = Gdk.Texture.new_from_bytes(gbytes)

                # Update the Gtk.Image widget to display the new image
                if isinstance(gtk_image_widget, Gtk.Picture):
                    gtk_image_widget.set_paintable(texture)
                elif isinstance(gtk_image_widget, Gtk.Image):
                    gtk_image_widget.set_from_paintable(texture)
            except Exception as e:
                print(f"Error processing image: {e}")

        def download_image():
            try:
                response = requests.get(url)
                response.raise_for_status()  # Raises an HTTPError for bad responses
                GLib.idle_add(update_image, response.content)
            except requests.RequestException as e:
                print(f"Error downloading image: {e}")

        # Start download in a separate thread to avoid freezing the GUI
        from threading import Thread
        download_thread = Thread(target=download_image)
        download_thread.start()

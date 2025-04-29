# Put this in the same folder as whisper.cpp
"""
Audio-to-Text GUI for whisper.cpp – complete version
• Drag-and-drop, multi-select, Ctrl+A, Del / Backspace, click-away deselect
• Model menu with Install / Delete / Cancel Download (live MB counter)
• Drop-down updates the instant a model finishes downloading or is deleted
"""

import gi, os, subprocess, threading, math
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib, Gio, Gdk

MB = 1024 * 1024

MODEL_SIZE_MB = {
    "tiny":   75,
    "base":   142,
    "small":  466,
    "medium": 1536,   # 1.5 GiB ≈ 1536 MiB
    "large":  2960    # 2.9 GiB ≈ 2960 MiB
}

class WhisperWindow(Gtk.Window):
    # ───────────────────────── initialisation ──────────────────────────────
    def __init__(self):
        super().__init__(title="Audio-To-Text Transcriber")
        Gtk.Settings.get_default().set_property(
            "gtk-application-prefer-dark-theme", True
        )
        self.set_default_size(700, 580); self.set_border_width(8)

        # paths / state ------------------------------------------------------
        sd = os.path.abspath(os.path.dirname(__file__))
        self.repo_dir        = os.path.join(sd, "whisper.cpp")
        self.bin_path        = os.path.join(self.repo_dir, "build", "bin", "whisper-cli")
        self.download_script = os.path.join(self.repo_dir, "models", "download-ggml-model.sh")
        
        # Models Directory (flatpak compatible)
        data_dir = os.getenv(
            "AUDIO_TO_TEXT_TRANSCRIBER_DATA_DIR",
            os.path.join(GLib.get_user_data_dir(), "AudioToTextTranscriber")
        )
        os.makedirs(data_dir, exist_ok=True)
        self.models_dir = os.path.join(data_dir, "models")
        os.makedirs(self.models_dir, exist_ok=True)

        # Current State Variables
        self.display_to_core = {}      # UI label → model core
        self.dl_info         = None    # holds dict while download runs
        self.cancel_flag     = False   # transcription cancel
        self.current_proc    = None

        # list of models shown in drop-down
        self.desired_models = ["tiny","tiny.en","base","base.en","small","small.en",
                               "medium","medium.en","large-v1","large-v2","large-v3",
                               "large-v3-turbo"]

        # ─── UI layout ------------------------------------------------------
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6); self.add(box)
        self._add_model_controls(box)
        self._add_audio_selector(box)
        self._add_output_selector(box)

        self.ts_check   = Gtk.CheckButton(label="Include timestamps")
        box.pack_start(self.ts_check, False, False, 0)

        self.trans_btn  = Gtk.Button(label="Transcribe"); self._green(self.trans_btn)
        self.trans_btn.connect("clicked", self.on_transcribe)
        box.pack_start(self.trans_btn, False, False, 6)

        self.status_lbl = Gtk.Label(label="Idle"); box.pack_start(self.status_lbl, False, False, 0)

        self.notebook   = Gtk.Notebook(); self.notebook.set_scrollable(True)
        self.notebook.set_hexpand(True); self.notebook.set_vexpand(True)
        box.pack_start(self.notebook, True, True, 0)

        self.connect("destroy", Gtk.main_quit)
        self._setup_dnd()
        self._update_model_btn()

    # ─────────────────── MODEL DROPDOWN + ACTION BUTTON ────────────────────
    def _add_model_controls(self, parent):
        parent.pack_start(Gtk.Label(label="Model:"), False, False, 0)

        self.model_combo = Gtk.ComboBoxText()
        self._refresh_model_menu()                       # initial fill
        self.model_combo.connect("changed", lambda *_: self._update_model_btn())
        parent.pack_start(self.model_combo, False, False, 0)

        self.model_btn = Gtk.Button(); parent.pack_start(self.model_btn, False, False, 4)
        self.model_btn.connect("clicked", self.on_model_btn)

    def _refresh_model_menu(self):
        """Rebuild the drop-down to reflect which models are present."""
        try:
            current_core = self.display_to_core[self.model_combo.get_active_text()]
        except Exception:
            current_core = None

        self.model_combo.remove_all()
        self.display_to_core.clear()

        size = {"tiny":"Smallest","base":"Smaller","small":"Small",
                "medium":"Medium","large":"Large"}
        lang = {"en":"English","fr":"French","es":"Spanish","de":"German"}

        for core in self.desired_models:
            size_key, lang_key = (core.split(".",1)+[None])[:2]
            if lang_key:
                label = f"{size.get(size_key,size_key.title())} {lang.get(lang_key,lang_key.upper())}"
            else:
                label = f"{size.get(size_key,size_key.title())}"
            if not os.path.isfile(self._model_target_path(core)):
                label += " (download)"
            self.model_combo.append_text(label)
            self.display_to_core[label] = core

            # restore previous selection if possible
            if core == current_core:
                self.model_combo.set_active(len(self.display_to_core)-1)

        if self.model_combo.get_active() == -1:
            self.model_combo.set_active(0)

    # helper
    def _model_target_path(self, core): return os.path.join(self.models_dir, f"ggml-{core}.bin")

    # update install/delete/cancel label & Transcribe enable
    def _update_model_btn(self):
        active = self.model_combo.get_active_text()
        if active is None:            # nothing selected yet
            self.trans_btn.set_sensitive(False)
            return

        if self.dl_info:                               # download running
            done = os.path.getsize(self.dl_info["target"]) // MB if os.path.isfile(self.dl_info["target"]) else 0
            tot  = self.dl_info["total_mb"] or "?"
            self.model_btn.set_label(f"Cancel Download  {done} / {tot} MB")
            self.trans_btn.set_sensitive(False)
            return

        core   = self.display_to_core[active]
        exists = os.path.isfile(self._model_target_path(core))
        self.model_btn.set_label("Delete Model" if exists else "Install Model")
        self.trans_btn.set_sensitive(exists)

    # ----------------------------------------------------------------------
    def on_model_btn(self, _):
        # cancel ongoing download
        if self.dl_info:
            self.dl_info["proc"].terminate()
            self.status_lbl.set_text("Cancelling download…")
            return

        core   = self.display_to_core[self.model_combo.get_active_text()]
        target = self._model_target_path(core)

        if os.path.isfile(target):                     # delete
            if self._yes_no(f"Delete model “{core}”?"):
                try: os.remove(target)
                except Exception as e: self._error(str(e))
            self._refresh_model_menu(); self._update_model_btn()
            return

        # start install
        self._start_download(core)

    # ───────────────────────── download management ─────────────────────────
    def _start_download(self, core):
        target = self._model_target_path(core)
        # strip suffixes like ".en", "-q5_1", "-v3-turbo" to find the family name
        family = core.split(".", 1)[0].split("-")[0]
        # look up the disk size; fallback to a HEAD request if we don’t have it
        if family in MODEL_SIZE_MB:
            total_mb = MODEL_SIZE_MB[family]
        else:
            total_bytes = self._remote_size_bytes(core)
            total_mb    = round(total_bytes/MB) if total_bytes else None

        cmd  = ["sh", self.download_script, core, self.models_dir]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        self.dl_info = {"core": core, "proc": proc, "target": target, "total_mb": total_mb}

        GLib.timeout_add(500, self._update_download_progress)
        threading.Thread(target=self._drain_output, args=(proc,), daemon=True).start()
        self._update_model_btn()

    def _update_download_progress(self):
        if not self.dl_info: return False
        proc, target = self.dl_info["proc"], self.dl_info["target"]

        if proc.poll() is None:                         # in progress
            self._update_model_btn(); return True

        # finished ----------------------------------------------------------------
        success = (proc.returncode == 0 and os.path.isfile(target))
        if not success:
            if os.path.isfile(target): os.remove(target)
            self._error(f"Failed to download model “{self.dl_info['core']}”.")
        else:
            self.status_lbl.set_text(f"Model “{self.dl_info['core']}” installed.")

        self.dl_info = None
        self._refresh_model_menu(); self._update_model_btn()
        return False

    def _drain_output(self, proc):
        for line in proc.stdout:
            tx = line.strip()
            if tx: GLib.idle_add(self.status_lbl.set_text, tx[:120])

    # HEAD request to get remote size
    def _remote_size_bytes(self, core):
        src="https://huggingface.co/ggerganov/whisper.cpp"; pfx="resolve/main/ggml"
        if "tdrz" in core: src="https://huggingface.co/akashmjn/tinydiarize-whisper.cpp"
        url=f"{src}/{pfx}-{core}.bin"
        for tool in (["curl","-sIL",url], ["wget","--spider","-S",url]):
            try:
                out=subprocess.check_output(tool,stderr=subprocess.STDOUT,text=True,timeout=8)
                for ln in out.splitlines():
                    if "Content-Length" in ln: return int(ln.split()[-1])
            except Exception: continue
        return None

    # ───────────────────────── audio selector ─────────────────────────────
    def _add_audio_selector(self, parent):
        parent.pack_start(Gtk.Label(label="Audio files:"), False, False, 0)
        h = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6); parent.pack_start(h, True, True, 0)

        self.audio_store = Gtk.ListStore(str)
        tv = Gtk.TreeView(model=self.audio_store)
        tv.append_column(Gtk.TreeViewColumn("Path", Gtk.CellRendererText(), text=0))

        sel = tv.get_selection(); sel.set_mode(Gtk.SelectionMode.MULTIPLE)
        tv.connect("key-press-event", self._on_audio_key)
        
        scr = Gtk.ScrolledWindow(); scr.set_vexpand(True); scr.add(tv); h.pack_start(scr, True, True, 0)

        vb  = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4); h.pack_start(vb, False, False, 0)
        add = Gtk.Button(label="Add Audio…"); add.connect("clicked", self.on_add_audio); vb.pack_start(add, False, False, 0)
        self.remove_btn = Gtk.Button(label="Remove Selected"); vb.pack_start(self.remove_btn, False, False, 0)
        self.remove_btn.connect("clicked", self.on_remove_audio)

        self.audio_view, self.add_btn = tv, add   # store for focus tests

    def _on_audio_key(self, widget, event):
        sel = widget.get_selection()
        if event.keyval in (Gdk.KEY_Delete, Gdk.KEY_BackSpace):
            self.on_remove_audio(None); return True
        if (event.keyval in (Gdk.KEY_a, Gdk.KEY_A)) and (event.state & Gdk.ModifierType.CONTROL_MASK):
            sel.select_all(); return True
        return False

    # add / remove / browse --------------------------------------------------
    def on_add_audio(self, _):
        dlg = Gtk.FileChooserDialog(
            "Select audio files or folders", self, Gtk.FileChooserAction.OPEN,
            (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            "Add", Gtk.ResponseType.OK)
        )
        dlg.set_select_multiple(True)

        # show both audio files and directories
        f = Gtk.FileFilter(); f.set_name("Audio or Folders")
        for ext in ("*.mp3","*.wav","*.flac","*.m4a","*.ogg","*.opus"): f.add_pattern(ext)
        f.add_mime_type("inode/directory")          # let folders appear
        dlg.add_filter(f)

        if dlg.run() == Gtk.ResponseType.OK:
            new_paths = self._collect_audio_files(dlg.get_filenames())
            for fn in new_paths:
                self.audio_store.append((fn,))
        dlg.destroy()

    # ─── helper placed anywhere in the class (e.g. right before on_add_audio) ──
    def _collect_audio_files(self, paths):
        """Return a list of unique audio files beneath the given path(s)."""
        audio_ext = (".mp3", ".wav", ".flac", ".m4a", ".ogg", ".opus")
        found = []
        seen  = set(r[0] for r in self.audio_store)        # already in list

        def _add_if_ok(p):
            if p.lower().endswith(audio_ext) and p not in seen:
                found.append(p);  seen.add(p)

        for p in paths:
            if os.path.isfile(p):
                _add_if_ok(p)
            elif os.path.isdir(p):
                for root, _, files in os.walk(p):
                    for f in files:
                        _add_if_ok(os.path.join(root, f))
        return found


    def on_remove_audio(self, _):
        tv   = self.audio_view
        sel  = tv.get_selection()
        model, paths = self.audio_view.get_selection().get_selected_rows()
        for p in reversed(paths):
            model.remove(model.get_iter(p))
        GLib.idle_add(sel.unselect_all)

    def _add_output_selector(self, parent):
        parent.pack_start(Gtk.Label(label="Output folder:"), False, False, 0)
        h=Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        self.out_entry = Gtk.Entry(); self.out_entry.set_editable(False); self.out_entry.set_can_focus(False)
        h.pack_start(self.out_entry, True, True, 0)
        b=Gtk.Button(label="Browse…"); b.connect("clicked", self._browse_out); h.pack_start(b, False, False, 0)
        parent.pack_start(h, False, False, 0)
        d=os.path.expanduser("~/Downloads")
        if os.path.isdir(d): self.out_entry.set_text(d)

    def _browse_out(self, _):
        dlg=Gtk.FileChooserDialog("Select folder",self,Gtk.FileChooserAction.SELECT_FOLDER,
                                  (Gtk.STOCK_CANCEL,Gtk.ResponseType.CANCEL,"Choose",Gtk.ResponseType.OK))
        if dlg.run()==Gtk.ResponseType.OK: self.out_entry.set_text(dlg.get_filename())
        dlg.destroy()

    # ─────────────────── drag-and-drop for audio files ─────────────────────
    def _setup_dnd(self):
        t=[Gtk.TargetEntry.new("text/uri-list",0,0)]
        for w in (self, self.audio_view, self.audio_view.get_parent()):
            w.drag_dest_set(Gtk.DestDefaults.ALL, t, Gdk.DragAction.COPY)
            w.connect("drag-data-received", self._dnd_received)

    def _dnd_received(self,w,dc,x,y,sel,info,time_):
        for uri in sel.get_data().decode().splitlines():
            if uri.startswith("file://"):
                path,_=GLib.filename_from_uri(uri)
                if path.lower().endswith((".mp3",".wav",".flac",".m4a",".ogg",".opus")) \
                        and not any(r[0]==path for r in self.audio_store):
                    self.audio_store.append((path,))
        dc.finish(True,False,time_)

    # ─────────────────────────── transcribe ────────────────────────────────
    def on_transcribe(self, _):
        if self.trans_btn.get_label()=="Cancel":
            self.cancel_flag=True
            if self.current_proc:
                try:self.current_proc.terminate()
                except:pass
            self._gui_status("Cancelling…"); return

        core=self.display_to_core[self.model_combo.get_active_text()]
        model_path=self._model_target_path(core)
        if not os.path.isfile(model_path): return self._error("Model not installed.")

        # clear old tabs
        for i in reversed(range(self.notebook.get_n_pages())):
            self.notebook.remove_page(i)

        files=[r[0] for r in self.audio_store]
        out_dir=self.out_entry.get_text().strip() or None
        if not files: return self._error("No audio files selected.")
        if not out_dir: return self._error("Choose an output folder.")
        
        # ensure we have a whisper-cli; build it if missing
        if not os.path.isfile(self.bin_path):
            self.trans_btn.set_sensitive(False)
            # run build in a background thread to avoid freezing the UI
            def _build_and_continue():
                success = self._ensure_whisper_cli()
                if success:
                    # once built, call on_transcribe again on the main thread
                    GLib.idle_add(self.on_transcribe, _)
                GLib.idle_add(self.trans_btn.set_sensitive, True)
            threading.Thread(target=_build_and_continue, daemon=True).start()
            return

        self.cancel_flag=False; self.trans_btn.set_label("Cancel"); self._red(self.trans_btn)
        threading.Thread(target=self._worker,args=(model_path,files,out_dir),daemon=True).start()

    def _ensure_whisper_cli(self):
        """If whisper-cli is missing, run cmake and build until it appears."""
        if os.path.isfile(self.bin_path):
            return True

        # update UI
        GLib.idle_add(self.status_lbl.set_text, "Building whisper-cli (~2 min)…")
        try:
            # 1) generate build files
            res1 = subprocess.run(
                ["cmake", "-B", "build"],
                cwd=self.repo_dir,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            if res1.returncode != 0:
                raise RuntimeError(res1.stderr)

            # 2) compile in Release
            res2 = subprocess.run(
                ["cmake", "--build", "build", "--config", "Release"],
                cwd=self.repo_dir,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            if res2.returncode != 0:
                raise RuntimeError(res2.stderr)

            # check again
            if not os.path.isfile(self.bin_path):
                raise FileNotFoundError(f"{self.bin_path} still missing after build")

        except Exception as e:
            GLib.idle_add(self._error, f"Build failed:\n{e}")
            return False

        GLib.idle_add(self.status_lbl.set_text, "Build complete.")
        return True


    # ───────────────── background transcription worker ─────────────────────
    def _worker(self, model_path, files, out_dir):
        total=len(files)
        for idx,p in enumerate(files,1):
            if self.cancel_flag: break
            name=os.path.basename(p)
            self._gui_status(f"{idx}/{total} – {name}")

            buf_holder={}; lbl_holder={}
            def _mk_tab():
                buf=Gtk.TextBuffer(); view=Gtk.TextView(buffer=buf,editable=False,monospace=True)
                scr=Gtk.ScrolledWindow(); scr.add(view); scr.show_all()
                lbl=Gtk.Label(label=f"⏳ {name}")
                self.notebook.append_page(scr,lbl); self.notebook.set_current_page(-1)
                buf_holder["b"]=buf; lbl_holder["l"]=lbl
            GLib.idle_add(_mk_tab,priority=GLib.PRIORITY_HIGH_IDLE)
            while not buf_holder: pass
            buf,lbl=buf_holder["b"],lbl_holder["l"]

            cmd=[self.bin_path,"-m",model_path,"-f",p]
            if not self.ts_check.get_active(): cmd.append("-nt")
            self._gui_log(buf,"transcribing …")
            self.current_proc=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
            for line in self.current_proc.stdout:
                if self.cancel_flag:
                    try: self.current_proc.terminate()
                    except: pass
                    GLib.idle_add(self._gui_tab_title, lbl, f"❌ {name}")
                    break
                self._gui_log(buf, line.rstrip())
            self.current_proc.stdout.close(); self.current_proc.wait()

            self.current_proc.stdout.close(); self.current_proc.wait()

            if self.cancel_flag: GLib.idle_add(self._gui_tab_title,lbl,f"❌ {name}"); break

            if self.current_proc.returncode!=0:
                err=self.current_proc.stderr.read().strip(); self.current_proc.stderr.close()
                self._gui_log(buf,f"ERROR: {err}")
            else:
                self.current_proc.stderr.close()
                dest=os.path.join(out_dir,os.path.splitext(name)[0]+".txt")
                def _save():
                    txt=buf.get_text(buf.get_start_iter(),buf.get_end_iter(),False)
                    with open(dest,"w",encoding="utf-8") as f:f.write(txt)
                    # buf.insert(buf.get_end_iter(),f"\nSaved → {dest}\n"); 
                    return False
                GLib.idle_add(_save); GLib.idle_add(self._gui_tab_title,lbl,f"✅ {name}")

        self._gui_status("Cancelled" if self.cancel_flag else "Done"); GLib.idle_add(self._reset_btn)

    # ─────────────────────── misc small helpers ────────────────────────────
    def _green(self,b): ctx=b.get_style_context(); ctx.remove_class("destructive-action"); ctx.add_class("suggested-action")
    def _red  (self,b): ctx=b.get_style_context(); ctx.remove_class("suggested-action"); ctx.add_class("destructive-action")
    def _gui_log(self,buf,txt): GLib.idle_add(lambda:(buf.insert(buf.get_end_iter(),txt+"\n"),False)[1])
    def _gui_status(self,msg): GLib.idle_add(self.status_lbl.set_text,msg)
    def _gui_tab_title(self,lbl,txt): GLib.idle_add(lbl.set_text,txt)
    def _reset_btn(self): self.trans_btn.set_label("Transcribe"); self._green(self.trans_btn)
    def _yes_no(self,msg):
        dlg=Gtk.MessageDialog(self,Gtk.DialogFlags.MODAL,Gtk.MessageType.QUESTION,0,msg)
        dlg.add_button("Cancel",Gtk.ResponseType.CANCEL); dlg.add_button("OK",Gtk.ResponseType.OK)
        res=dlg.run(); dlg.destroy(); return res==Gtk.ResponseType.OK
    def _error(self,msg):
        dlg=Gtk.MessageDialog(self,Gtk.DialogFlags.MODAL,Gtk.MessageType.ERROR,Gtk.ButtonsType.CLOSE,msg)
        dlg.run(); dlg.destroy()

# ───────────────────────────────────────────────────────────────────────────
if __name__=="__main__":
    WhisperWindow().show_all(); Gtk.main()

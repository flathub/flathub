# -*- coding: utf-8 -*-

import sys
import ui_main
import ui_options
import ui_donate
import ui_about

import os
import shlex
import subprocess
import webbrowser
from PyQt5 import QtWidgets
from pymediainfo import MediaInfo



# -------------------------------------------- Main program ----------------------------------------------------------

class ExampleApp(QtWidgets.QMainWindow, ui_main.Ui_MainWindow):
    def __init__(self, parent=None):  # init ui_main
        super().__init__(parent)
        self.setupUi(self)  # init design

# ------------------------------------------- Main Menu Buttons ------------------------------------------------------

        self.actionOpen.triggered.connect(self.open_file)
        self.actionSave_As.triggered.connect(self.save_file)
        self.actionOptions.triggered.connect(self.options)
        self.actionAbout.triggered.connect(self.about)
        self.actionDonate_0_7_for_project.triggered.connect(self.donate)

# ---------------------------------------- Status and Buttons ---------------------------------------------------------

        self.toolButton_1.clicked.connect(self.open_file)  # execute open_file
        self.toolButton_2.clicked.connect(self.save_file)  # execute save_file
        self.toolButton_3.clicked.connect(self.make_preset)  # make preset
        self.comboBox_1.currentTextChanged.connect(self.settings_menu)
        self.comboBox_3.currentTextChanged.connect(self.settings_menu_bit_depth)
        self.comboBox_4.currentTextChanged.connect(self.settings_menu_mode)
        self.comboBox_8.currentTextChanged.connect(self.settings_menu_hdr_signals)
        self.comboBox_12.currentTextChanged.connect(self.settings_menu_audio)

# ----------------------------------------------- Dialogs -------------------------------------------------------------

    def open_file(self):
        self.lineEdit_1.clear()  # Clear elements
        self.lineEdit_2.clear()  # Clear elements
        self.textBrowser_1.clear()  # Clear elements
        self.textBrowser_2.clear()  # Clear elements
        self.label_53.setText("Progress:")
        self.progressBar.setProperty("value", 0)
        file_name_open = QtWidgets.QFileDialog.getOpenFileName(self, "Open File", "Untitled", ("Video Files: *.avi, "
                                                                                               "*m2ts, *.m4v, *.mkv, "
                                                                                               "*.mov, *.mp4, *.mpeg, "
                                                                                               "*.mpg, *.mxf, *.ts, "
                                                                                               "*.webm (*.avi *.m2ts "
                                                                                               "*.m4v *.mkv *.mov "
                                                                                               "*.mp4 *.mpeg *.mpg "
                                                                                               "*.mxf *.ts *.webm)"))
        file_name = file_name_open[0]
        self.lineEdit_1.setText(file_name)  # Open filename to listEdit_1

        try:
            cmd = f'mediainfo --Language=raw "{file_name}"'
            args = shlex.split(cmd)
            process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            data = process.communicate()[0]
            data_line = data.decode('utf8')
            self.textBrowser_1.setText(data_line)

        except:
            self.textBrowser_1.setText("!!! An error occured !!!\nPossible reasons for the error:\n - can't get "
                                       "information about this file or file is not supported")

    def save_file(self):
        self.lineEdit_2.clear()  # Clear elements
        prefix = self.comboBox_2.currentText()
        file_name_save = QtWidgets.QFileDialog.getSaveFileName(self, "Save As", "Untitled." + prefix, "All Files (*.*)")
        self.lineEdit_2.setText(file_name_save[0])  # Output filename to listEdit_2

    def options(self):
        self.optionsapp = OptionsApp()
        self.optionsapp.show()
        self.optionsapp.toolButton_4.clicked.connect(self.optionsapp.close)
        self.optionsapp.toolButton_5.clicked.connect(self.optionsapp.close)

    def about(self):
        self.aboutapp = AboutApp()
        self.aboutapp.show()
        self.aboutapp.toolButton_6.clicked.connect(self.aboutapp.close)
        self.aboutapp.label_11.setText('<a href="https://github.com/CineEncoder/cine-encoder"> https://github.com/CineEncoder/cine-encoder </a>')
        self.aboutapp.label_11.setOpenExternalLinks(True)
        self.aboutapp.label_12.setText('<a href="https://github.com/CineEncoder/cine-encoder/blob/master/LICENSE"> License: GNU Lesser General Public License Version 3 </a>')
        self.aboutapp.label_12.setOpenExternalLinks(True)

    def donate(self):
        self.donateapp = DonateApp()
        self.donateapp.show()
        self.donateapp.toolButton_7.clicked.connect(lambda: webbrowser.open('https://paypal.me/KozhukharenkoOleg?locale.x=ru_RU'))
        self.donateapp.toolButton_8.clicked.connect(lambda: webbrowser.open('https://blockchain.com/btc/payment_request?address=14ukiWCK2f8vFNPP3qKbo2vfaSGRMN63qL&amount=0.00005448&message=Cine Encoder donation'))


# -------------------------------------------- Settings menu ----------------------------------------------------------

    def settings_menu(self):
        self.comboBox_2.setEnabled(True)
        self.comboBox_3.setEnabled(True)
        self.comboBox_4.setEnabled(True)
        self.comboBox_12.setEnabled(True)
        self.comboBox_2.clear()
        self.comboBox_3.clear()
        self.comboBox_4.clear()
        self.comboBox_12.clear()

        if self.comboBox_1.currentText() == "H265 NVENC" or self.comboBox_1.currentText() == "H265":
            self.comboBox_2.addItems(["mkv"])
            self.comboBox_3.addItems(["10 bit", "8 bit"])
            self.comboBox_4.addItems(["Variable bitrate", "Constant quality"])
            self.comboBox_12.addItems(["From source", "AAC", "AC3"])
            self.comboBox_2.setEnabled(False)

        if self.comboBox_1.currentText() == "H264 NVENC" or self.comboBox_1.currentText() == "H264":
            self.comboBox_2.addItems(["mkv"])
            self.comboBox_3.addItems(["8 bit"])
            self.comboBox_4.addItems(["Variable bitrate", "Constant quality"])
            self.comboBox_12.addItems(["From source", "AAC", "AC3"])
            self.comboBox_2.setEnabled(False)
            self.comboBox_3.setEnabled(False)

        if self.comboBox_1.currentText() == "VP9":
            self.comboBox_2.addItems(["mkv"])
            self.comboBox_3.addItems(["10 bit"])
            self.comboBox_4.addItems(["Variable bitrate"])
            self.comboBox_12.addItems(["From source", "Vorbis", "Opus"])
            self.comboBox_2.setEnabled(False)
            self.comboBox_3.setEnabled(False)
            self.comboBox_4.setEnabled(False)

        if self.comboBox_1.currentText() == "ProRes HQ 4:2:2" or self.comboBox_1.currentText() == "DNxHR HQX 4:2:2":
            self.comboBox_2.addItems(["mov"])
            self.comboBox_3.addItems(["10 bit"])
            self.comboBox_4.addItems(["Auto"])
            self.comboBox_12.addItems(["PCM"])
            self.comboBox_2.setEnabled(False)
            self.comboBox_3.setEnabled(False)
            self.comboBox_4.setEnabled(False)
            self.comboBox_8.setEnabled(False)
            self.comboBox_12.setEnabled(False)

    def settings_menu_bit_depth(self):
        self.comboBox_8.setEnabled(True)
        self.comboBox_8.clear()
        if self.comboBox_3.currentText() == "8 bit":
            self.comboBox_8.addItems(["Disable"])
            self.comboBox_8.setEnabled(False)
        if self.comboBox_3.currentText() == "10 bit":
            self.comboBox_8.addItems(["Disable", "Enable"])

    def settings_menu_mode(self):
        self.comboBox_5.setEnabled(True)
        self.comboBox_6.setEnabled(True)
        self.comboBox_7.setEnabled(True)
        self.comboBox_5.clear()
        self.comboBox_6.clear()
        self.comboBox_7.clear()
        if self.comboBox_4.currentText() == "Auto":
            self.comboBox_5.addItems(["Auto"])
            self.comboBox_6.addItems(["Auto"])
            self.comboBox_7.addItems(["Auto"])
            self.comboBox_5.setEnabled(False)
            self.comboBox_6.setEnabled(False)
            self.comboBox_7.setEnabled(False)
        if self.comboBox_4.currentText() == "Constant quality":
            self.comboBox_5.addItems(["Auto"])
            self.comboBox_6.addItems(["Auto"])
            self.comboBox_7.addItems(["22", "25", "24", "23", "21", "20", "19", "18", "17", "16", "15"])
            self.comboBox_5.setEnabled(False)
            self.comboBox_6.setEnabled(False)
        if self.comboBox_4.currentText() == "Variable bitrate":
            self.comboBox_5.addItems(["45M", "40M", "35M", "30M", "25M", "20M", "15M", "10M", "5M"])
            self.comboBox_6.addItems(["50M", "45M", "40M", "35M", "30M", "25M", "20M", "15M", "10M", "5M"])
            self.comboBox_7.addItems(["Auto"])
            self.comboBox_7.setEnabled(False)

    def settings_menu_hdr_signals(self):
        self.comboBox_9.setEnabled(True)
        self.comboBox_10.setEnabled(True)
        self.comboBox_11.setEnabled(True)
        self.comboBox_9.clear()
        self.comboBox_10.clear()
        self.comboBox_11.clear()
        if self.comboBox_8.currentText() == "Disable":
            self.comboBox_9.addItems(["bt709"])
            self.comboBox_10.addItems(["bt709"])
            self.comboBox_11.addItems(["bt709"])
            self.comboBox_9.setEnabled(False)
            self.comboBox_10.setEnabled(False)
            self.comboBox_11.setEnabled(False)
        if self.comboBox_8.currentText() == "Enable":
            self.comboBox_9.addItems(["bt2020"])
            self.comboBox_10.addItems(["bt2020nc"])
            self.comboBox_11.addItems(["smpte2084 (PQ)", "arib-std-b67 (HLG)"])

    def settings_menu_audio(self):
        self.comboBox_13.setEnabled(True)
        self.comboBox_13.clear()
        if self.comboBox_12.currentText() == "From source":
            self.comboBox_13.addItems(["From source"])
            self.comboBox_13.setEnabled(False)
        if self.comboBox_12.currentText() == "AAC":
            self.comboBox_13.addItems(["384k", "256k", "128k"])
        if self.comboBox_12.currentText() == "AC3":
            self.comboBox_13.addItems(["640k", "448k", "384k", "256k"])
        if self.comboBox_12.currentText() == "Vorbis":
            self.comboBox_13.addItems(["448k", "384k", "256k", "128k", "96k", "64k"])
        if self.comboBox_12.currentText() == "Opus":
            self.comboBox_13.addItems(["448k", "384k", "256k", "128k", "96k", "64k"])
        if self.comboBox_12.currentText() == "PCM":
            self.comboBox_13.addItems(["Auto"])
            self.comboBox_13.setEnabled(False)

# ---------------------------------------------- Encode File ---------------------------------------------------------

    def make_preset(self):
        global preset
        global preset_0
        global trc
        vbitrate = str(self.comboBox_5.currentText())
        maxrate = str(self.comboBox_6.currentText())
        quality = str(self.comboBox_7.currentText())
        colorprim = str(self.comboBox_9.currentText())
        colormatrix = str(self.comboBox_10.currentText())
        if self.comboBox_3.currentText() == "8 bit":
            pxfmt = "yuv420p"
            prof = "main"
        if self.comboBox_3.currentText() == "10 bit":
            pxfmt = "yuv420p10le"
            prof = "main10"
        if self.comboBox_11.currentText() == "smpte2084 (PQ)":
            transfer = "smpte2084"
            trc = "16"
        if self.comboBox_11.currentText() == "arib-std-b67 (HLG)":
            transfer = "arib-std-b67"
            trc = "18"

        abitrate = str(self.comboBox_13.currentText())
        if self.comboBox_12.currentText() == "From source":
            apreset = "-c:a copy"
        if self.comboBox_12.currentText() == "AAC":
            apreset = f'-c:a aac -b:a {abitrate} '
        if self.comboBox_12.currentText() == "AC3":
            apreset = f'-c:a ac3 -b:a {abitrate} '
        if self.comboBox_12.currentText() == "Vorbis":
            apreset = f'-c:a libvorbis -b:a {abitrate} '
        if self.comboBox_12.currentText() == "Opus":
            apreset = f'-c:a libopus -b:a {abitrate} '
        if self.comboBox_12.currentText() == "PCM":
            apreset = "-c:a pcm_s16le"


        if self.comboBox_1.currentText() == "H265 NVENC" and self.comboBox_8.currentText() == "Enable": #-----H265 NVENC-HDR-10bit
            if self.comboBox_4.currentText() == "Constant quality":
                preset = str(f'-pix_fmt yuv420p10le -c:v hevc_nvenc -preset slow -rc constqp -qp {quality} -qmin 0 -qmax 51 -rc-lookahead 32 -sei hdr -profile:v main10 -color_primaries {colorprim} -color_trc {transfer} -colorspace {colormatrix} -color_range tv -flags -global_header -max_muxing_queue_size 1024 {apreset} -f matroska ')
            if self.comboBox_4.currentText() == "Variable bitrate":
                preset = str(f'-pix_fmt yuv420p10le -b:v {vbitrate} -maxrate:v {maxrate} -c:v hevc_nvenc -preset slow -rc vbr_hq -2pass 1 -qmin 0 -qmax 51 -rc-lookahead 32 -sei hdr -profile:v main10 -color_primaries {colorprim} -color_trc {transfer} -colorspace {colormatrix} -color_range tv -flags -global_header -max_muxing_queue_size 1024 {apreset} -f matroska ')
            preset_0 = "-hide_banner -threads:v 4 -threads:a 8 -hwaccel cuvid "
            self.encode_mux_file()
        if self.comboBox_1.currentText() == "H265" and self.comboBox_8.currentText() == "Enable": #-------H265-HDR-10bit
            if self.comboBox_4.currentText() == "Constant quality":
                preset = str(f'-pix_fmt yuv420p10le -c:v libx265 -preset slow -crf {quality} -rc-lookahead 32 -sei hdr -profile:v main10 -color_primaries {colorprim} -color_trc {transfer} -colorspace {colormatrix} -color_range tv -max_muxing_queue_size 1024 {apreset} -f matroska ')
            if self.comboBox_4.currentText() == "Variable bitrate":
                preset = str(f'-pix_fmt yuv420p10le -b:v {vbitrate} -maxrate:v {maxrate} -c:v libx265 -preset slow -rc vbr_hq -rc-lookahead 32 -sei hdr -profile:v main10 -color_primaries {colorprim} -color_trc {transfer} -colorspace {colormatrix} -color_range tv -max_muxing_queue_size 1024 {apreset} -f matroska ')
            preset_0 = "-hide_banner "
            self.encode_mux_file()
        if self.comboBox_1.currentText() == "VP9" and self.comboBox_8.currentText() == "Enable": #---------VP9-HDR-10bit
            preset = str(f'-b:v {vbitrate} -speed 4 -pix_fmt yuv420p10le -color_primaries 9 -color_trc {trc} -colorspace 9 -color_range 1 -maxrate {maxrate} -minrate 8040000 -profile:v 2 -vcodec libvpx-vp9 {apreset} -f matroska ')
            preset_0 = "-hide_banner "
            self.encode_mux_file()
        if self.comboBox_1.currentText() == "H265 NVENC" and self.comboBox_8.currentText() == "Disable": #---H265 NVENC-8-10bit
            if self.comboBox_4.currentText() == "Constant quality":
                preset = str(f'-pix_fmt {pxfmt} -c:v hevc_nvenc -preset slow -rc constqp -qp {quality} -qmin 0 -qmax 51 -rc-lookahead 32 -profile:v {prof} -max_muxing_queue_size 1024 {apreset} -f matroska ')
            if self.comboBox_4.currentText() == "Variable bitrate":
                preset = str(f'-pix_fmt {pxfmt} -b:v {vbitrate} -maxrate:v {maxrate} -c:v hevc_nvenc -preset slow -rc vbr_hq -2pass 1 -qmin 0 -qmax 51 -rc-lookahead 32 -profile:v {prof} -max_muxing_queue_size 1024 {apreset} -f matroska ')
            preset_0 = "-hide_banner -hwaccel cuvid "
            self.encode_file()
        if self.comboBox_1.currentText() == "H265" and self.comboBox_8.currentText() == "Disable": #----- --H265-8-10bit
            if self.comboBox_4.currentText() == "Constant quality":
                preset = str(f'-pix_fmt {pxfmt} -c:v libx265 -preset slow -crf {quality} -rc-lookahead 32 -profile:v {prof} -max_muxing_queue_size 1024 {apreset} -f matroska ')
            if self.comboBox_4.currentText() == "Variable bitrate":
                preset = str(f'-pix_fmt {pxfmt} -b:v {vbitrate} -maxrate:v {maxrate} -c:v libx265 -preset slow -rc vbr_hq -rc-lookahead 32 -profile:v {prof} -max_muxing_queue_size 1024 {apreset} -f matroska ')
            preset_0 = "-hide_banner "
            self.encode_file()
        if self.comboBox_1.currentText() == "VP9" and self.comboBox_8.currentText() == "Disable": #-------- ---VP9-10bit
            preset = str(f'-b:v {vbitrate} -speed 4 -pix_fmt yuv420p10le -maxrate {maxrate} -minrate 8040000 -profile:v 2 -vcodec libvpx-vp9 {apreset} -f matroska ')
            preset_0 = "-hide_banner "
            self.encode_file()
        if self.comboBox_1.currentText() == "H264 NVENC": #---------------------------------------------H264 NVENC-8bit
            if self.comboBox_4.currentText() == "Constant quality":
                preset = str(f'-pix_fmt yuv420p -c:v h264_nvenc -preset slow -rc constqp -qp {quality} -qmin 0 -qmax 51 -rc-lookahead 32 -profile:v high -max_muxing_queue_size 1024 {apreset} -f matroska ')
            if self.comboBox_4.currentText() == "Variable bitrate":
                preset = str(f'-pix_fmt yuv420p -b:v {vbitrate} -maxrate:v {maxrate} -c:v h264_nvenc -preset slow -rc vbr_hq -2pass 1 -qmin 0 -qmax 51 -rc-lookahead 32 -profile:v high -max_muxing_queue_size 1024 {apreset} -f matroska ')
            preset_0 = "-hide_banner -hwaccel cuvid "
            self.encode_file()
        if self.comboBox_1.currentText() == "H264": #---------------------------------------------------------H264-8bit
            if self.comboBox_4.currentText() == "Constant quality":
                preset = str(f'-pix_fmt yuv420p -c:v libx264 -preset slow -crf {quality} -qmin 0 -qmax 51 -rc-lookahead 32 -profile:v main -max_muxing_queue_size 1024 {apreset} -f matroska ')
            if self.comboBox_4.currentText() == "Variable bitrate":
                preset = str(f'-pix_fmt yuv420p -b:v {vbitrate} -maxrate:v {maxrate} -c:v libx264 -preset slow -rc vbr_hq -2pass 1 -qmin 0 -qmax 51 -rc-lookahead 32 -profile:v main -max_muxing_queue_size 1024 {apreset} -f matroska ')
            preset_0 = "-hide_banner "
            self.encode_file()
        if self.comboBox_1.currentText() == "ProRes HQ 4:2:2": #--------------------------------------------ProRes-10bit
            preset = str(f'-c:v prores_ks -profile:v 3 -vtag apch -c:a pcm_s16le ')
            preset_0 = "-hide_banner "
            self.encode_file()
        if self.comboBox_1.currentText() == "DNxHR HQX 4:2:2": #--------------------------- -----------------DNxHR-10bit
            preset = str(f'-c:v dnxhd -profile:v dnxhr_hqx -pix_fmt yuv422p10le -c:a pcm_s16le ')
            preset_0 = "-hide_banner "
            self.encode_file()

    def encode_mux_file(self):
        self.statusbar.clearMessage()
        self.textBrowser_2.clear()  # Clear elements
        self.label_53.setText("Encoding:")
        input_file = self.lineEdit_1.text()
        output_file = self.lineEdit_2.text()
        temp_folder = output_file + "_temp"
        temp_file = temp_folder + "/temp.mkv"
        try:
            os.mkdir(temp_folder)
        except:
            self.textBrowser_2.append("Please delete previous temporary folder or rename output file")
        else:
            self.textBrowser_2.append("Start encoding ... \nButtons will be disable after start encoding.\n")
            percent = 0
            frame = 0
            media_info = MediaInfo.parse(input_file)
            dur = 0.001*float(media_info.tracks[0].duration)
            fps = float(media_info.tracks[0].frame_rate)
            dur_mod = round(dur, 2)
            fps_mod = round(fps, 2)
            fr_count = int(dur_mod*fps_mod)
            try:
                cmd = f'ffmpeg {preset_0}-i "{input_file}" {preset} -y "{temp_file}" '
                args = shlex.split(cmd)
                process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                for line in process.stdout:
                    line_mod1 = line.replace('   ', ' ')
                    line_mod2 = line_mod1.replace('  ', ' ')
                    line_mod3 = line_mod2.replace('= ', '=')
                    pos_st = line_mod3.find('frame=') + 6
                    pos_end = line_mod3.find(' fps')
                    if pos_st != 5:
                        frame = int(line_mod3[pos_st:pos_end])
                    percent = (frame*100)/fr_count
                    self.progressBar.setProperty("value", percent)
                    self.statusbar.showMessage("  " + line_mod3)
            except:
                self.textBrowser_2.setText("!!! An error occured !!!\nPossible reasons for the error:\n - input or "
                                           "output file not defined,\n - codec may not be supported by program,"
                                           "\n - attempt to overwrite an existing file.")
            else:
                self.label_53.setText("Muxing:")
                self.textBrowser_2.append("Start muxing ... \nButtons will be disable after start muxing.\n")
                try:
                    cmd = f'mkvmerge -o "{output_file}" --colour-matrix 0:9 --colour-range 0:1 ' \
                          f'--colour-transfer-characteristics 0:{trc} --colour-primaries 0:9 --max-content-light 0:1000 ' \
                          f'--max-frame-light 0:300 --max-luminance 0:1000 --min-luminance 0:0.01 ' \
                          f'--chromaticity-coordinates 0:0.68,0.32,0.265,0.690,0.15,0.06 --white-colour-coordinates ' \
                          f'0:0.3127,0.3290 "{temp_file}" '
                    args = shlex.split(cmd)
                    process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                    for line in process.stdout:
                        line_mod1 = line.replace('   ', ' ')
                        line_mod2 = line_mod1.replace('  ', ' ')
                        self.statusbar.showMessage("  " + line_mod2)
                except:
                    self.textBrowser_2.setText("!!! An error occured !!!\nPossible reasons for the error:\n - input "
                                               "or output file not defined,\n - codec may not be supported by "
                                               "program,\n - attempt to overwrite an existing file.")
                else:
                    self.textBrowser_2.append("\nTask completed successfully!")
                    try:
                        os.replace(temp_file)
                        os.rmdir(temp_folder)
                    except:
                        self.textBrowser_2.append("Please delete temporary folder manually.")

    def encode_file(self):
        self.statusbar.clearMessage()
        self.textBrowser_2.clear()  # Clear elements
        self.label_53.setText("Encoding:")
        input_file = self.lineEdit_1.text()
        output_file = self.lineEdit_2.text()
        self.textBrowser_2.append("Start encoding ... \nButtons will be disable after start encoding.\n")
        percent = 0
        frame = 0
        try:
            media_info = MediaInfo.parse(input_file)
            dur = 0.001*float(media_info.tracks[0].duration)
            fps = float(media_info.tracks[0].frame_rate)
            dur_mod = round(dur, 2)
            fps_mod = round(fps, 2)
            fr_count = int(dur_mod*fps_mod)
        except:
            self.textBrowser_2.setText("Select first input and output file!")
        else:
            try:
                cmd = f'ffmpeg {preset_0}-i "{input_file}" {preset} -y "{output_file}" '
                args = shlex.split(cmd)
                process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                for line in process.stdout:
                    line_mod1 = line.replace('   ', ' ')
                    line_mod2 = line_mod1.replace('  ', ' ')
                    line_mod3 = line_mod2.replace('= ', '=')
                    pos_st = line_mod3.find('frame=') + 6
                    pos_end = line_mod3.find(' fps')
                    if pos_st != 5:
                        frame = int(line_mod3[pos_st:pos_end])
                    percent = (frame*100)/fr_count
                    self.progressBar.setProperty("value", percent)
                    self.statusbar.showMessage("  " + line_mod3)
            except:
                self.textBrowser_2.setText("!!! An error occured !!!\nPossible reasons for the error:\n - input or "
                                           "output file not defined,\n - codec may not be supported by program,"
                                           "\n - attempt to overwrite an existing file.")
            else:
                self.textBrowser_2.append("\nTask completed successfully!")

# --------------------------------------------------------------------------------------------------------------------

class OptionsApp(QtWidgets.QMainWindow, ui_options.Ui_Options):
    def __init__(self):  # init
        super().__init__()
        self.setupUi(self)  # init

class DonateApp(QtWidgets.QMainWindow, ui_donate.Ui_Donate):
    def __init__(self):  # init
        super().__init__()
        self.setupUi(self)  # init

class AboutApp(QtWidgets.QMainWindow, ui_about.Ui_About):
    def __init__(self):  # init
        super().__init__()
        self.setupUi(self)  # init



def main():
    app = QtWidgets.QApplication(sys.argv)
    window = ExampleApp()
    window.show()
    app.exec_()


if __name__ == '__main__':  # if run direct
    main()  # run main()

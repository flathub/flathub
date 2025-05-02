# Copyright (c) 2017-2021, Md Imam Hossain (emamhd at gmail dot com)
# see LICENSE.txt for details

import os
import tkinter

class FolderData():

    def __init__(self, location):
        self.location = os.path.abspath(location)
        self.name = os.path.dirname(self.location)
        self.files = os.listdir(self.location)
        self.folders = []
        self.number_of_files = len(self.files)
        self.location_label_text =  tkinter.StringVar()
        self.folders_number_label_text = tkinter.StringVar()
        self.location_label_text.set("Current folder location: " + self.location)

    def update(self, location):
        self.location = os.path.abspath(location)
        self.name = os.path.dirname(self.location)
        self.files = os.listdir(self.location)
        self.folders.clear()
        self.number_of_files = len(self.files)
        self.location_label_text.set("Current folder location: " + self.location)

class FoldersWindow():

    def __init__(self, parent, background_color='#FFFFFF', icons_background_color='#FFFFFF', location='\\'):
        self.folders_frame = tkinter.Frame(parent, background=icons_background_color)
        self.scrolled_window = tkinter.Canvas(self.folders_frame, background=icons_background_color)
        self.scrolled_window.pack(fill=tkinter.BOTH, expand=tkinter.TRUE, side=tkinter.LEFT)
        self.scrolled_window_scrollbar = tkinter.Scrollbar(self.folders_frame, troughcolor=icons_background_color, activebackground=background_color, background=background_color, relief=tkinter.RIDGE, orient=tkinter.VERTICAL, command=self.scrolled_window.yview)
        self.scrolled_window_scrollbar.pack(fill=tkinter.Y, side=tkinter.RIGHT)
        self.scrolled_window.config(yscrollcommand=self.scrolled_window_scrollbar.set)
        self.scrolled_window.bind('<Configure>', self.scrolled_window_on_configure)
        self.folders_window = tkinter.Frame(self.scrolled_window, background=icons_background_color)
        #self.folders_window.pack(fill=tkinter.BOTH, expand=tkinter.TRUE, anchor=tkinter.NW)
        self.folders_window_scrolled_window_id = self.scrolled_window.create_window((0, 0), window=self.folders_window, anchor=tkinter.NW)
        self.folders_window.bind('<Configure>', self.folders_window_on_configure)

    def scrolled_window_on_configure(self, event):

        self.scrolled_window.itemconfigure(self.folders_window_scrolled_window_id, width=self.scrolled_window.winfo_width())

    def folders_window_on_configure(self, event):

        size = (self.folders_window.winfo_reqwidth(), self.folders_window.winfo_reqheight())
        self.scrolled_window.config(scrollregion="0 0 %s %s" % size)

        #self.scrolled_window.config(scrollregion=self.scrolled_window.bbox(tkinter.ALL))

class Folder():

    def __init__(self, parent, parent_of_parent, parent_folder_data, icon=None, name="Folder", location='\\', background_color='#FFFFFF', foreground_color='#252525', font_size=12):

        self.folder_frame = tkinter.Frame(parent, background=background_color)
        self.folder_frame.bind('<Double-Button-1>', self.double_click_callback)
        self.icon = tkinter.Label(self.folder_frame, image=icon, background=background_color)
        self.icon.bind('<Double-Button-1>', self.double_click_callback)
        self.icon.pack(side=tkinter.LEFT)
        self.label = tkinter.Label(self.folder_frame, text=name, background=background_color, foreground=foreground_color, font=(None, font_size))
        self.label.bind('<Double-Button-1>', self.double_click_callback)
        self.label.pack(side=tkinter.LEFT)
        self.folder_location = location
        self.name = name
        self.container = parent
        self.container_of_parent = parent_of_parent
        self.parent_folder_data = parent_folder_data
        self.icon_data = icon
        self.background_color = background_color
        self.foreground_color = foreground_color

    def double_click_callback(self, event):

        for widget in self.container.winfo_children():
            widget.destroy()

        self.parent_folder_data.update(os.path.abspath(os.path.join(self.folder_location, self.name)))

        self.parent_folder_data.files.sort()

        for file in self.parent_folder_data.files:
            if os.path.isdir(os.path.abspath(os.path.join(self.parent_folder_data.location, file))):
                Folder(self.container, self.container_of_parent, self.parent_folder_data, background_color=self.background_color, foreground_color=self.foreground_color ,icon=self.icon_data, name=file, location=self.parent_folder_data.location).folder_frame.pack(anchor=tkinter.NW)
                self.parent_folder_data.folders.append(file)

        self.parent_folder_data.folders_number_label_text.set(str(len(self.parent_folder_data.folders)) + ' folders and ' + str(len(self.parent_folder_data.files) - len(self.parent_folder_data.folders)) + ' files are found in current folder')

        self.container_of_parent.xview_moveto(0)
        self.container_of_parent.yview_moveto(0)

class FoldersSelectionWindow():

    def __init__(self, parent, location, folder_icon, window_background_color='#F6F4F2', window_foreground_color='#000000', window_button_background_color='#F4F2F0', window_button_focus_background_color='#FCFAF8', window_button_foreground_color='#242424', icons_background_color='#FFFFFF', icons_foreground_color='#252525'):
        self.window_background_color = window_background_color
        self.window_foreground_color = window_foreground_color
        self.window_button_background_color = window_button_background_color
        self.window_button_focus_background_color = window_button_focus_background_color
        self.window_button_foreground_color = window_button_foreground_color
        self.icons_background_color = icons_background_color
        self.icons_foreground_color = icons_foreground_color
        self.folders_selection_window_frame = tkinter.Frame(parent, background=window_background_color)
        self.folders = FoldersWindow(self.folders_selection_window_frame, background_color=window_background_color, icons_background_color=icons_background_color)
        self.folders.folders_frame.pack(fill=tkinter.BOTH, expand=tkinter.TRUE, side=tkinter.TOP)
        self.folder_icon = tkinter.PhotoImage(file=folder_icon)
        self.listing_directory = FolderData(location)
        self.listing_directory.files.sort()

        for file in self.listing_directory.files:
            if os.path.isdir(os.path.abspath(os.path.join(self.listing_directory.location, file))):
                Folder(self.folders.folders_window, self.folders.scrolled_window, self.listing_directory, background_color=self.icons_background_color, foreground_color=self.icons_foreground_color, icon=self.folder_icon, name=file, location=self.listing_directory.location).folder_frame.pack(anchor=tkinter.NW)
                self.listing_directory.folders.append(file)

        self.listing_directory.folders_number_label_text.set(str(len(self.listing_directory.folders)) + ' folders and ' + str(len(self.listing_directory.files)-len(self.listing_directory.folders)) + ' files')

        self.location_label = tkinter.Label(self.folders_selection_window_frame, font=(None, 10), background=window_background_color, foreground=window_foreground_color, textvariable=self.listing_directory.location_label_text).pack(fill=tkinter.X, side=tkinter.TOP)
        self.folders_number_label = tkinter.Label(self.folders_selection_window_frame, font=(None, 10), background=window_background_color, foreground=window_foreground_color, textvariable=self.listing_directory.folders_number_label_text).pack(fill=tkinter.X, side=tkinter.TOP)
        self.parent_button = tkinter.Button(self.folders_selection_window_frame, font=(None, 10), activeforeground=window_button_foreground_color, activebackground=window_button_focus_background_color, highlightbackground=window_background_color, background=window_button_background_color, foreground=window_button_foreground_color, text="Go to parent folder", command=self.parent_button_callback).pack(fill=tkinter.X, side=tkinter.TOP)

    def parent_button_callback(self):

        for widget in self.folders.folders_window.winfo_children():
            widget.destroy()

        self.listing_directory.update(os.path.abspath(os.path.join(self.listing_directory.location, '..')))

        self.listing_directory.files.sort()

        for file in self.listing_directory.files:
            if os.path.isdir(os.path.abspath(os.path.join(self.listing_directory.location, file))):
                Folder(self.folders.folders_window, self.folders.scrolled_window, self.listing_directory, background_color=self.icons_background_color, foreground_color=self.icons_foreground_color, icon=self.folder_icon, name=file, location=self.listing_directory.location).folder_frame.pack(side=tkinter.TOP, anchor=tkinter.W)
                self.listing_directory.folders.append(file)

        self.listing_directory.folders_number_label_text.set(str(len(self.listing_directory.folders)) + ' folders and ' + str(len(self.listing_directory.files) - len(self.listing_directory.folders)) + ' files')

        self.folders.scrolled_window.xview_moveto(0)
        self.folders.scrolled_window.yview_moveto(0)

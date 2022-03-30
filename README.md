# Needly (Version 2.5)

*Needly* is an openQA needle editor written in Python. It creates or modifies needles for the openQA tests. 

The openQA needle consists of two files, a *png* screenshot and the *json* definition file. The definition file provides various information to the openQA engine, including the *tags*, *types*, and *areas* to look for. Users can use this application to edit (or create) all the necessary information quickly and draw the areas using the built-in WYSIWYG editor. When the needle is saved, it is fully compatible with the openQA engine. 

The advantage of the editor is that it does not need openQA to be installed on the system. The needle files are all you need to work with them.

The editor only supports *png* screenshots. To open the needle, you can either load that *png* file or the *json* definition file. Both ways will open the needle in the editor. 

## Requirements to run the application

* Python 3
* Tkinter
* the Pillow library

## Recommended requirements to have better user experience

* Libvirt (to run VMs that can be controllable with `virsh`)
* ImageMagick (to work with screenshots) 

## Using the editor

### Starting the editor

Currently, you can run the editor from the console.

* Running `./needly.py` starts the editor without any image loaded. You can open an image using the **Ctrl-O** combination. Or you can load the entire directory into a front using **Ctrl-D**. 
* Running `./needly.py needle.png` starts the editor with that particular image loaded which can be useful to edit an existing needle quickly.

### Reading the images

#### Editing multiple files in a directory

You can open an entire directory and navigate through images one after another and edit their needles. To use this approach:

1. Click **File > Open directory** or use **Ctrl-D**.
2. Use the dialogue to select a directory from which the screenshots will be loaded.
3. Circle over the images using **Load next** (**Ctrl-N**) or **Load previous** (**Ctrl-P**) from the **File** menu.

#### Editing a single file

You can also open a particular file and edit its needle. To use this approach:

1. Click the **File > Open file** or use the **Ctrl-O** key.
2. Use the dialogue to locate the file you want to edit.

### Working with needles

#### Loading the needle information

In the current version, when you have loaded a picture, the definition file is not loaded automatically so that you can recreate the needle from scratch which is useful when reneedling test suites where bigger changes are to expected. However, if you would rather want to edit the current definiton file, you need to load it using **Needle > Load** (**Ctrl-L**).  

You can reload the needle again anytime and restore all the original information until you save the definition file.

#### Reading the needle information.

When the needle is loaded, you can see all needle information in the right part of the program window.
Among others:

* the name of the active image
* needle properties
* needle tags
* active area coordinates
* number of areas in the needle
* the content of the needle json file

#### Updating the needle information

You can manually update the following fields:

* the coordinates
* the properties
* the tags
* the area type

#### Redrawing the area

The needle area can be updated using several techniqes:

1. You can use the mouse to draw a new needle area. 
2. You can manually update the coordinates in the coordinate fields on the right.
3. You can use keys to change the size of the area. 

When using the keyboard:

* Using **Left**, **Right**, **Up**, and **Down** arrows changes the coordinates of the lower right corner in steps of 1 px each.
* Using the **Shift** key combined with arrows changes the coordinates of the upper left corner in steps of 1 px each.
* Holding the **Ctrl** key when pressing arrows increases one step to 5 pxs.
* Holding the **Alt** key when pressing arrows increases one step to 25 pxs.

**Note**: When you have updated the area, you have to click **Area > Modify area** (**Ctrl-M**) 
to update the actual needle. 

#### Saving a needle

If you want to store the needle information permanently, you have to save it. To do so:

1. Click on the **Needle > Save**, or use the **Ctrl-S** shortcut to save the needle.

When saving, the editor overwrites the definiton file and the original needle cannot be restored.

#### Creating a new needle from scratch

When you create a needle can do it for an existing image or you can take a screenshot from a connected virtual machine (see later). To create a new needle for an existing image:

1. Open the existing image.
2. Fill in all needed info:
   * needle tag
   * needle area
   * needle type (match, ocr, or exclude)
   * needle properties (not compulsory)
3. Draw a rectangle around the area or use any of the approaches from **Redrawing the area** section.
4. Click **Area > Add area** button or press the **Ctrl-A**  to add the area to the needle. 
5. If you wish to add another area (the needle can have more areas), just draw a new area and repeat **Step 3** to add it to the needle.
6. Click the **Needle > Save** (**Ctrl-S**) to save the needle permanently.

### Working with areas

#### Add an area to the needle

In order to have an area on the needle, you have to add it to it:

1. Press the **Area > Add area** button (**Ctrl-A**) to add the area to the needle. 
2. Repeat for another area.

You can see the number of areas in the field in the lower right part of the window.

#### Removing an active area

When your area is still active (that means that you have not added a new area yet), it can be removed
from the needle again:

1. Click on the **Area > Remove area** button (**Ctrl-R**) to remove it from the needle. 

When removing the area from the needle, the active area falls back to the previous area 
(which becomes active) and the rectangle will show its current position. You can repeat the action, until all areas are deleted.

If, however, you need to remove an area added earlier but you do not want to remove the later areas, you need to save the needle first, then reload it, which puts you to the earliest area and makes it active. Then you can move to later areas using **Area > Go to next area** (**Ctrl-G**).

#### Showing next area

When the needle has more than one area (you can see the number in the lower right part of the
program window), only the first area is shown. To see the next area:

1. Click on the **Area > Go to next area** (**Ctrl-G**). 

This will show the next area in the needle and makes it active. You can update the area or remove it.

**Warning**: In this version, you cannot navigate in areas. You only can move to the next ones.
However, if you remove the area from the needle, the editor will fall back to the previous area and 
make it active again so you can update or remove it.

If you need to change the first area without removing the next area, use the following workaround:

1. Save the needle (**Ctrl-S**).
2. Load the needle (**Ctrl-L**).
3. Now, the editor makes the very first area active and you can modify it (**Ctrl-M**).
4. Move to the next area (**Ctrl-G**) to modify it.
5. Repeat until you have modified required areas.
6. Save the needle (**Ctrl-S**).

#### Taking screenshots from a VM

If you do not have any screenshot to be used as a needle, you can create one by taking a screenshot from an existing and running virtual machine:

1. Click on the **vMachine > Connect a VM** to open a dialogue to connect to a running VM. Now, the application will be able to take screenshots from that virtual machine. When you need to connect to a different VM, repeate this step.
2. When the application is connected to a VM (indicated in the lower right part), you can take a screenshot any time using **vMachine > Take screenshot** (**Ctrl-T**). The screenshot will be saved as `screenshot.png` and displayed in the application.
3. In order to protect the screenshot from being overwritten with another press of the key shortcut, type in the tag name, and use the **File > Set name from tag** (**Ctrl-X**) to rename the *png* file.

**Note:** To have this functionality, you have to have `libvirt`, `virsh`, and `ImageMagick` installed. 

**Warning:** If your virtual machine does not run in user space, it might not be visible by the application. There are several ways to fix it: 

1. In `virt-manager` add a new, user related, connection to libvirt. Click **File > Add connection** and then choose **QEMU/KVM user relation** as the hypervisor. Now, any VM created under this connection will be available to be used in the application. (recommended) 
2. Fix the rights of the user to be able to manipulate the system VMs via `virsh` (out of scope of this document)
3. Run the editor with `sudo` (not-recommended).

## Reporting a problem

If you experience a problem, open an issue. Or help with the development. Yup, it is opensource!

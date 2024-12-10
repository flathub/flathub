from tkinter import *
import os
import subprocess
from tkinter import messagebox
from tkinter import filedialog as fd



file_icona = ""

user_home_directory = os.path.expanduser("~/")
path = os.path.expanduser("~/.config/SonneWebApp/")
if os.path.exists("~/.config/SonneWebApp") == False:
    os.makedirs(path, exist_ok=True)
if os.path.exists("{path}icons") == False:
    os.makedirs(f"{path}icons", exist_ok=True)
    


with open(f"{path}start_web_app.py", "w") as file:
    file.write('''import sys
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWebEngineWidgets import *
app = QtWidgets.QApplication(sys.argv)
name = sys.argv[1]
site = sys.argv[2]
zoom = sys.argv[3]
zoom = float(zoom)
w = QWebEngineView()
w.load(QtCore.QUrl(site))
w.setWindowTitle(name)
w.resize(1200, 800) 
w.setZoomFactor(zoom)  
w.show()
app.exec_()'''
        )
    


def send_new_app_command():
    global file_icona
    
    name = e1.get()    
    if name == "" or "/" in name or "." in name:
        messagebox.showinfo("Error", "Error: Insert a valid name please (Invalid character: / . \ )")
        return None
       
    site = e2.get()
    if site == "":
        messagebox.showinfo("Error", "Error: Insert a valid web site please")
        return None

    scale = e3.get()
    if scale == "":
        messagebox.showinfo("Error", "Error: Insert a valid scale please (Example: 1.7, 1.8, 2.0)")
        return None

    
    if file_icona != "preferences-desktop":     
        os.system(f"cp {file_icona} {path}icons/{name}.ico")
        file_icona = f"{path}icons/{name}.ico"
    
        
    with open(f"{user_home_directory}.local/share/applications/SonneWebApp_{name}.desktop", "w") as file:
        file.write(f'''[Desktop Entry]
Type=Application
Name={name}
Exec=python3 {path}start_web_app.py "{name}" "{site}" "{scale}"
Icon={file_icona}
Comment=App created by SonneWebApp.py
Keywords=sonne;
NoDisplay=false
'''
            )   
    messagebox.showinfo("Site added", "Web app added successfully!")
    add_app.destroy()     


def icon_name():    
    global file_icona
    file_icona = fd.askopenfilename()
    print(file_icona)

def add_web_app():
    global e1, e2, e3, add_app, file_icona 
      
    add_app = Tk()
    add_app.geometry("1000x1000")
    add_app.title("Add an app")
    Label(add_app, text="Name").grid(row=0)
    Label(add_app, text="Site").grid(row=1)
    Label(add_app, text="Scale").grid(row=2)
    e1 = Entry(add_app)
    e2 = Entry(add_app)
    e3 = Entry(add_app)

    e1.grid(row=0, column=1)
    e2.grid(row=1, column=1)
    e3.grid(row=2, column=1)
    add_icon= Button(add_app, text="Add Icon", command=icon_name)
    add_icon.grid(row=4, column=1)

    send_new_app = Button(add_app, text="Add site", command=send_new_app_command)
    send_new_app.grid(row=5, column=2)
    add_app.mainloop()


def remove_app_button():
    name = r1.get()
    try:
        os.remove(f"{user_home_directory}.local/share/applications/SonneWebApp_{name}.desktop")
        messagebox.showinfo("Site removed", "Web app removed successfully!")
        remove_app.destroy()
        
        
        
    except:
        messagebox.showinfo("Error during site removing", "Error: WebApp not removed, did you write the correct name?")
        remove_app.destroy()

def remove_web_app():
    global remove_app, r1
    remove_app = Tk()
    remove_app.geometry("1000x1000")
    remove_app.title("Remove Web app")
    Label(remove_app, text="Insert the name of the site").grid(row=0)
    r1 = Entry(remove_app)
    r1.grid(row=0, column=1)

    send_remove = Button(remove_app, text="Remove", command=remove_app_button)
    send_remove.grid(row = 2)
    remove_app.mainloop()
    

window = Tk()
window.geometry("1000x1000")
window.title("Web To App")
botton = Button(window, text = "Add web app", command=add_web_app)
botton.pack()

icon = PhotoImage(file='icon.png')
window.iconphoto(True, icon)


remove_button = Button(window, text="Remove a site", command=remove_web_app)
remove_button.pack()

try:
    desktop_files = subprocess.check_output(f"ls {user_home_directory}.local/share/applications | grep SonneWebApp_", shell=True)

    desktop_files = desktop_files.decode("utf-8").strip()
    desktop_files = desktop_files.replace("SonneWebApp_", "")
    desktop_files = desktop_files.replace(".desktop", "")

except:
    desktop_files = "Nothing here!"
show_exist_site = Label(text="Existing site: \n" + desktop_files)
show_exist_site.pack()
window.mainloop()
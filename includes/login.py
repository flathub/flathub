from PySide6.QtWidgets import QMainWindow
from ui import login_ui
from discord_integration import login, load_token
import platformdirs


class LoginUI(QMainWindow, login_ui.Ui_MainWindow):
    def __init__(self, switcher, parent=None):
        super().__init__(parent)

        self.ui = login_ui.Ui_MainWindow()
        self.ui.setupUi(self)
        self.ui.pushButton.clicked.connect(self.switch)
        self.switcher = switcher

        self.ui.password.returnPressed.connect(self.switch)
    
    def switch(self):
        email = self.ui.email.text()
        password = self.ui.password.text()
        valid = email and password

        self.ui.email.setText("")
        self.ui.password.setText("")

        if valid:
            _token = login(email, password)
            print(_token, "is our token")

            if _token:
                with open(platformdirs.user_config_dir("QTCord") + "/discordauth.txt", "w") as f:
                    f.write(_token)

                load_token()

                self.switcher.setCurrentIndex(self.switcher.currentIndex() + 1)

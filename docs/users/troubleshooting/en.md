# Troubleshooting this Flatpak

If your situation is not listed here, you have been unable to follow the instructions, or the solution proposed here has not worked for you, please head to our [issues][issues] section with your problem, or contact us on [our Matrix chat][matrix-chat].


## Failure to call AutoFirma from the browser

This is most likely because the CA certificate installation could not be performed automatically.

To fix this, you will find different—though similar—solutions depending on your browser.

<details>
    <summary>🦊 Solution for Firefox, Waterfox, Icecat, LibreWolf, etc.</summary>

1. Open the browser and go to **Settings** (or "Options"/"Preferences").
2. In the sidebar, go to the **Privacy & Security** section.
3. Scroll down almost to the bottom to find the **Certificates** section and click the **View Certificates...** button.
4. Ensure you are on the **Authorities** tab.
5. Click **Import...** and select the file located in your home folder: 
   `~/.afirma/Autofirma/Autofirma_ROOT.cer`
   *(Note: If you cannot see the hidden `.afirma` folder, press `Ctrl + H` in the file picker).*
6. **IMPORTANT:** When importing it, a pop-up window will appear asking about trust. You must tick the box:
   - [x] **Trust this CA to identify websites.**
7. Accept and restart the browser.
</details>

<details>
    <summary>🔵 Solution for Chromium, Google Chrome, etc.</summary>

1. Open the browser settings. You can do this by pasting the following into the address bar:
   `chrome://settings/certificates`
   *(Or by navigating to: Settings > Privacy and security > Security > Manage certificates).*
2. Select the **Authorities** tab.
3. Click the **Import** button.
4. Select the file located in your home folder:
   `~/.afirma/Autofirma/Autofirma_ROOT.cer`
   *(Note: If you cannot see the hidden `.afirma` folder in the Linux file picker, it is usually enough to right-click and enable "Show hidden files" or press `Ctrl + H`).*
5. **IMPORTANT:** Upon selection, a trust dialogue box will appear. You must tick:
   - [x] **Trust this certificate for identifying websites.**
6. Accept and restart the browser.
</details>


[issues]: https://github.com/flathub/es.gob.afirma/issues
[matrix-chat]: https://matrix.to/#/#autofirma-flatpak:matrix.org

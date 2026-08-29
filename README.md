# Guía Oficial para Publicar Yoru Reader en Flathub

Flathub es la tienda principal de aplicaciones para Linux (Steam Deck, Ubuntu, Fedora, Arch, etc.).
Flathub gestiona las aplicaciones a través de GitHub mediante Pull Requests.

### Pasos para publicar:

1. **Hacer Fork del repositorio oficial de Flathub**:
   Visita: https://github.com/flathub/flathub y haz clic en **Fork**.

2. **Crear una rama con los archivos de Yoru Reader**:
   Crea una rama llamada `add-yoru-reader`.
   Sube los archivos contenidos en `packaging/flathub/`:
   - `com.yorureader.app.yml`
   - `com.yorureader.app.metainfo.xml`
   - `com.yorureader.app.desktop`
   - `icon.png`

3. **Abrir Pull Request (PR)**:
   Abre un Pull Request hacia `flathub/flathub:master` con el título:
   `Add com.yorureader.app`

4. **Publicación Automática**:
   El bot de Flathub compilará y validará la aplicación. Una vez aprobado por los revisores de Flathub, se creará el repositorio `https://github.com/flathub/com.yorureader.app` y estará disponible mundialmente para instalar con:
   ```bash
   flatpak install flathub com.yorureader.app
   ```

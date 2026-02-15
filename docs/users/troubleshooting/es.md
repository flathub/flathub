# Solución de problemas con este Flatpak

Si tu situación no se encuentra aquí,
no has sido capaz de seguir las instrucciones
o no te ha funcionado la solución aquí propuesta,
por favor acude a nuestra sección de [issues][issues] con tu problema, o contáctanos en [nuestro chat de Matrix][matrix-chat].


## Fallo al llamar a Autofirma desde el navegador

Muy probablemente esto se debe a que no se ha podido realizar la instalación del certificado CA de forma automática. 

Para solucionarlo, dependiendo de tu navegador, tendrás soluciones diferentes, aunque son similares entre ellas.

<details>
    <summary>🦊 Solución para Firefox, Waterfox, Icecat, Librewolf, etc.</summary>

1. Abre el navegador y dirígete a **Ajustes** (o "Opciones"/"Preferencias").
2. En el menú lateral, ve a la sección **Privacidad y seguridad**.
3. Baja casi hasta el final hasta encontrar el apartado **Certificados** y haz clic en el botón **Ver certificados...**
4. Asegúrate de estar en la pestaña **Autoridades** (o "Entidades").
5. Haz clic en **Importar...** y selecciona el archivo situado en tu carpeta de usuario: 
   `~/.afirma/Autofirma/Autofirma_ROOT.cer`
   *(Nota: Si no ves la carpeta oculta `.afirma`, presiona `Ctrl + H` en el selector de archivos).*
6. **IMPORTANTE:** Al importarlo, aparecerá una ventana emergente preguntando por la confianza. Debes marcar la casilla:
   - [x] **Confiar en esta CA para identificar sitios web.**
7. Acepta y reinicia el navegador.
</details>

<details>
    <summary>🔵 Solución para Chromium, Google Chrome, etc</summary>

1. Abre la configuración del navegador. Puedes hacerlo pegando esto en la barra de direcciones:
   `chrome://settings/certificates`
   *(O navegando a: Configuración > Privacidad y seguridad > Seguridad > Gestionar certificados).*
2. Selecciona la pestaña **Autoridades**.
3. Haz clic en el botón **Importar**.
4. Selecciona el archivo situado en tu carpeta de usuario:
   `~/.afirma/Autofirma/Autofirma_ROOT.cer`
   *(Nota: Si no ves la carpeta oculta `.afirma` en el selector de Linux, suele bastar con hacer clic derecho y activar "Mostrar archivos ocultos" o presionar `Ctrl + H`).*
5. **IMPORTANTE:** Al seleccionarlo, aparecerá un cuadro de diálogo de confianza. Debes marcar:
   - [x] **Confiar en este certificado para identificar sitios web.**
6. Acepta y reinicia el navegador.
</details>


[issues]: https://github.com/flathub/es.gob.afirma/issues
[matrix-chat]: https://matrix.to/#/#autofirma-flatpak:matrix.org

# Autofirma Flatpak

[Read in English](README.en.md)

---

## Acciones necesarias para la mayoria de navegadores

Autofirma requiere instalar un certificado en el navegador que vayas a utilizar. Recomendamos leer el [FAQ oficial] para instalar los certificados manualmente.

Puedes encontrar el archivo del certificado a instalar en la siguiente ubicacion:

```
~/.var/app/es.gob.afirma/.afirma/Autofirma/Autofirma_ROOT.cer
```

Puedes comprobar tu configuracion en las siguientes paginas: 
- https://valide.redsara.es/valide/
- https://www.sededgsfp.gob.es/es/Paginas/TestAutofirma.aspx

## FAQ

> ¿Por qué necesito instalar manualmente el certificado? 
> ¿Autofirma no hace esto automatico en Windows?

No realmente, el código de auto-instalación de certificados de Autofirma no es robusto.
Solo es capaz de detectar los dos principales navegadores (Chrome/Chromium y Firefox) incluso en Windows, y solo es capaz de detectarlos en
las siguientes configuraciones automaticamente sin permisos adicionales:
- Chrome Nativo (gestor de paquetes)
- Otros que comprueben el almacen NSS compartido (`~/.pki/nssdb`)
Y los siguientes con permisos adicionales:
- Firefox Snap
- Firefox (gestor de paquetes)
- Chromium Snap

También, dar permiso a estos ultimos abriría una brecha de ataque grande a tu navegador, por ello no lo habilitamos por defecto ni documentamos como hacerlo aquí.

## Contribuir

Cualquier contribución es bien recibida.
Para resolver dudas o cuestiones al respecto estamos disponibles en [Matrix]: [#autofirma-flatpak:matrix.org][matrix-chat]

[Matrix]: https://matrix.org
[matrix-chat]: https://matrix.to/#/#autofirma-flatpak:matrix.org
[FAQ oficial]:https://github.com/ctt-gob-es/clienteafirma/wiki/Faq-autofirma-execution#no-se-abre-autofirma-al-ejecutar-firmas-desde-el-navegador

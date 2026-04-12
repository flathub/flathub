# AutoFirma Flatpak

[Leer en español](README.md)

---

## Required actions for most browsers

Autofirma requires you to install its self-signed certificate on the browser you use it with.
We recommend reading the [official FAQ] for instructions to manually install it.

You can find the certificate file on the following path:
```
~/.var/app/es.gob.afirma/.afirma/Autofirma/Autofirma_ROOT.cer
```

You can check if your setup is working on the following websites: 
- https://valide.redsara.es/valide/?lang=en
- https://www.sededgsfp.gob.es/es/Paginas/TestAutofirma.aspx (Spanish Only)

## FAQ

 > Doesn't Autofirma works out-of-the-box on Windows?
 > Why do I need to manually install a certificate?

Autofirma's certificate auto-installation code is not robust. 
Only detects the two main browsers (Firefox and Chrome/Chromium) even on Windows,
and its only able to detect them on the following setups without additional 
permissions:
- Chrome (package manager)
- Other browsers that check NSS's shared database (`~/.pki/nssdb`)

And the following ones with additional permissions:
- Firefox Snap
- Firefox (package manager)
- Chromium Snap

Also, giving permission to these last ones would open a big attack surface to your
browser, so we don't enable them by default and don't document how to do it here.

## Contributing

Any contributions are welcomed.

For solving questions, we are available in [Matrix]: at [#autofirma-flatpak:matrix.org][matrix-chat]

Please be aware that most conversation takes place in spanish.

[Matrix]: https://matrix.org
[matrix-chat]: https://matrix.to/#/#autofirma-flatpak:matrix.org
[official FAQ]: https://github.com/ctt-gob-es/clienteafirma/wiki/Faq-autofirma-execution-en-US

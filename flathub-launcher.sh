#!/bin/sh
# Lanzador de IngePresupuestos (edición Flathub).
# PySide6 lo aporta el base-app io.qt.PySide.BaseApp; las demás dependencias
# Python están en /app/lib/pythonX.Y/site-packages (módulo python3-requirements).
# No se fuerza la plataforma Qt: Qt auto-detecta Wayland (WAYLAND_DISPLAY) o
# X11 (DISPLAY). Escape INGEPPTO_FORCE_XCB=1 disponible (lo maneja main.py).
#
# En esta edición NO hay acceso al host (regla Flathub): los reportes ODT/ODS
# —que dependen de LibreOffice del sistema— muestran un aviso en vez de generarse;
# PDF/Word/Excel funcionan normalmente.
PYVER=$(python3 -c 'import sys; print("%d.%d" % sys.version_info[:2])')
export PYTHONPATH="/app/lib/python${PYVER}/site-packages:${PYTHONPATH}"
cd /app/ingepresupuestos || exit 1
exec python3 main.py "$@"

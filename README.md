# Flatpak QGIS packaging

Just flatpak packaging for QGIS.

## I need more python modules!
Often you will need extra python modules to run extra plugins. We can't possibly package them all with
the application. To get them in your system, you can install them locally as follows:

```
flatpak run --command=pip3 org.qgis.qgis install scipy --user
```

Where `scipy` is the package you want, replace it with whatever you think it might be necessary.

If you feel like it definitely should be bundled, don't hesitate to report an issue in this repository.

Artisan uses many libraries, and it is important that they function correctly.
Some packages have built-in tests that can be run.

Note that this is especially important to do when building OpenBLAS, NumPy and/or SciPy from source,
and upgrading one of them, as they strongly depend on each other, and may not always be compatible
(e.g. [scipy#19831](https://github.com/scipy/scipy/issues/19831))

The Flatpak does by default not include everything to run these tests, so you'll
need to build an adapted Flatpak to run them.

## numpy & scipy

[numpy](https://github.com/numpy/numpy) and [scipy](https://github.com/scipy/scipy)
have built-in tests.

It requires the dependencies `pytest`, `hypothesis` and `pooch` as well as its own `tests`.
Build the flatpak

```sh
flatpak-pip-generator pytest hypothesis pooch -o dep-python3-tests
echo '  - dep-python3-tests.json' >>org.artisan_scope.artisan.yml
sed -i 's/- \(python3 -m pip uninstall\)/- echo -- do not \1/' org.artisan_scope.artisan.yml
sed -i 's/- find .* -name tests.*$/- echo skip tests/' org.artisan_scope.artisan.yml
sed -i 's/\(proto,help,uic,misc\)/\1,test/' org.artisan_scope.artisan.yml
flatpak-builder build-dir org.artisan_scope.artisan.yml --force-clean --install --user
```

Then run them with

```sh
flatpak run --devel --command=python org.artisan_scope.artisan -c "import numpy, sys; sys.exit(numpy.test() is False)"
flatpak run --devel --command=python org.artisan_scope.artisan -c "import scipy, sys; sys.exit(scipy.test() is False)"
```

## matplotlib

https://matplotlib.org/devdocs/devel/testing.html#testing-released-versions-of-matplotlib

add the the matplotlib install commands (adapt Python version if needed):
```
"tar --wildcards --strip-components=2 --directory=${FLATPAK_DEST}/lib/python3.11/site-packages -xzf matplotlib*.tar.gz */lib/matplotlib/tests/baseline_images",
"tar --wildcards --strip-components=2 --directory=${FLATPAK_DEST}/lib/python3.11/site-packages -xzf matplotlib*.tar.gz */lib/mpl_toolkits/*/tests/baseline_images"
```

then run

```sh
flatpak run --command=pytest org.artisan_scope.artisan --pyargs matplotlib.tests
```

these tests will always fail, however, because they are tightly bound to the Freetype version, and we (want to) use the system's.
https://github.com/matplotlib/matplotlib/issues/8796

It can still be useful to run the tests, as most tests do actually succeed, and it is good to know that nothing would be crashing Python.

## hardware interfaces

Some Python packages need native libraries, sometimes they are part of the source package, sometimes part of the wheel,
sometimes not at all. Make sure it can find and load the native libraries.

- phidgets
- snap7
- yoctopuce

TODO finish how to test this

## Artisan

Run the tests with

```
flatpak run --cwd=/app --env=PYTHONPATH=/app --command=pytest org.artisan_scope.artisan --pyargs test
```


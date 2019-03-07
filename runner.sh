#!/bin/sh
pushd /app/invesalius/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/app/lib/python3.5/site-packages/skimage/_shared/
echo $LD_LIBRARY_PATH
python3 app.py
popd

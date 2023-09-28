#!/bin/sh
tar -xf /app/extra/amplide.linux64.tgz
rm -f /app/extra/amplide.linux64.tgz
mv /app/extra/ampl.linux-intel64/* /app/extra/
rm -r /app/extra/ampl.linux-intel64
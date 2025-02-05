#!/bin/bash

cd /app
export LD_LIBRARY_PATH=usr/lib
exec ./server_box

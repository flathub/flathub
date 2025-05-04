#!/bin/bash

ulimit -c 0
exec /app/reqable/reqable "$@"
#!/bin/bash
# Wrapper script for ElecWhat to use zypak for proper sandboxing
exec zypak-wrapper /app/elecwhat "$@"
#!/bin/bash
base64 -d | xxd -p | paste -s | tr -d '\t'

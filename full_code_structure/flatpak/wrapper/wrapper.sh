#!/bin/bash
echo -ne "\033]0;Scripted Journeys\007"
/app/bin/first_run.sh
python3 /app/bin/__main__.py

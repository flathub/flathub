#!/bin/bash -e

ln -sfn ~/.technic/cache/ ~/.var/app/net.technicpack.TechnicLauncher/data/cache

java -jar /app/bin/TechnicLauncher.jar

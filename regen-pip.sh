# Increase this on each version update and rerun the script
wget https://raw.githubusercontent.com/pgadmin-org/pgadmin4/REL-8_11/requirements.txt

# https://github.com/flatpak/flatpak-builder-tools/issues/365
cat requirements.txt | grep -v "<= '3.9'" | grep -v "<= '3.10'" | grep -v sys_platform==\"win32\" > requirements_filtered.txt
sed -i "1 i tomli" requirements_filtered.txt # psycopg-c requires tomli to build

flatpak-pip-generator --yaml -r requirements_filtered.txt --ignore-pkg bcrypt==4.2.* cryptography==43.0.*

rm requirements.txt
rm requirements_filtered.txt
wget https://raw.githubusercontent.com/pgadmin-org/pgadmin4/REL-8_4/requirements.txt

# https://github.com/flatpak/flatpak-builder-tools/issues/365
cat requirements.txt | grep -v "<= '3.7'" | grep -v "<= '3.10'" | grep -v sys_platform==\"win32\" > requirements_filtered.txt
sed -i "1 i tomli" requirements_filtered.txt # psycopg-c requires tomli to build

flatpak-pip-generator --yaml -r requirements_filtered.txt --ignore-pkg bcrypt==4.0.* cryptography==42.0.*

rm requirements.txt
rm requirements_filtered.txt
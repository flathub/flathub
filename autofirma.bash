#!/bin/bash
#
# Wrapper script for Autofirma Flatpak.
# Based on the AUR script from Oscar Garcia Amor (https://ogarcia.me)
# to handle certificate generation and trust before launching Autofirma.
#
_autofirma_dir="${HOME}/.afirma/Autofirma"
_autofirma_ca="${_autofirma_dir}/AutoFirma_ROOT.cer"
_autofirma_pfx="${_autofirma_dir}/autofirma.pfx"
_cert_days="3650"
_cert_cn="AutoFirma ROOT"
_firefox_profiles_ini="${HOME}/.mozilla/firefox/profiles.ini"
_firefox_flatpak_profiles_ini="${HOME}/.var/app/org.mozilla.firefox/.mozilla/firefox/profiles.ini"
_nssdb="sql:${HOME}/.pki/nssdb"


function _make_ca_config {
  cat << EOF > "${_temp_dir}/openssl.cnf"
[ ca ]
default_ca=CA_autofirma
[ CA_autofirma ]
dir=${_temp_dir}
new_certs_dir=\$dir
database=\$dir/index.txt
serial=\$dir/serial
crlnumber=\$dir/crlnumber
default_days=${_cert_days}
default_crl_days=30
default_md=sha256
preserve=no
x509_extensions=usr_cert
email_in_dn=no
copy_extensions=copy
[ policy_ca ]
countryName=optional
stateOrProvinceName=optional
localityName=optional
organizationName=optional
organizationalUnitName=optional
commonName=supplied
emailAddress=optional
[ req ]
default_bits=4096
x509_extensions=v3_ca
distinguished_name=req_distinguished_name
[ req_distinguished_name ]
commonName_default=${_cert_cn}
[ usr_cert ]
basicConstraints=CA:FALSE
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer:always
subjectAltName=IP:127.0.0.1
[ v3_ca ]
basicConstraints=critical,CA:TRUE
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer:always
keyUsage=cRLSign,digitalSignature,keyCertSign,keyEncipherment,dataEncipherment
extendedKeyUsage=serverAuth,clientAuth,anyExtendedKeyUsage
EOF
touch "${_temp_dir}/index.txt"
echo "01" > "${_temp_dir}/crlnumber"
}


function process_firefox_profiles {
  local profiles_ini="$1"
  local base_dir="$2"

  if [ ! -r "${profiles_ini}" ]; then
    return
  fi

  local profile_paths=($(grep Path "${profiles_ini}"))
  for profile_path in ${profile_paths[@]}; do
    profile_path="${profile_path##*=}"
    # Check if profile path is absolute or relative
    [ ! -d "${profile_path}" ] && profile_path="${base_dir}/${profile_path}"
    # Add CA in current firefox profile
    if [ -d "${profile_path}" ]; then
      certutil -d "${profile_path}" -D -n "${_cert_cn}" > /dev/null 2>&1
      certutil -d "${profile_path}" -A -i "${_autofirma_ca}" -n "${_cert_cn}" -t C,,
    fi
  done
}

function trust_ca {
  # We must check if the .cer file actually exists before trying to trust it
  if [ ! -f "${_autofirma_ca}" ]; then
    echo "AutoFirma CA not found, skipping trust."
    return
  fi

  echo "Updating system and Firefox trust stores..."

  # Add CA in shared user database
  certutil -d "${_nssdb}" -D -n "${_cert_cn}" > /dev/null 2>&1
  certutil -d "${_nssdb}" -A -i "${_autofirma_ca}" -n "${_cert_cn}" -t C,,

  # Add CA in all native firefox profiles (if any)
  process_firefox_profiles "${_firefox_profiles_ini}" "${HOME}/.mozilla/firefox"

  # Add CA in all Firefox Flatpak profiles (if any)
  process_firefox_profiles "${_firefox_flatpak_profiles_ini}" "${HOME}/.var/app/org.mozilla.firefox/.mozilla/firefox"

  unset _autofirma_ca _autofirma_pfx _cert_cn _nssdb _firefox_profiles_ini _firefox_flatpak_profiles_ini
  echo "Trust update complete."
}


function do_init {
  echo "Generating new AutoFirma certificates..."
  mkdir -p "${_autofirma_dir}"
  _temp_dir="$(mktemp -d)"
  _ca="openssl ca -config ${_temp_dir}/openssl.cnf"
  _req="openssl req -config ${_temp_dir}/openssl.cnf"
  rm -f "${_autofirma_ca}" "${_autofirma_pfx}"
  _make_ca_config
  openssl rand -base64 48 > "${_temp_dir}/randomkey.txt"
  # Make local CA
  ${_req} -new -passout file:"${_temp_dir}/randomkey.txt" \
    -keyout "${_temp_dir}/autofirma.key" \
    -subj "/CN=${_cert_cn}" \
    -out "${_temp_dir}/autofirma.csr"
  ${_ca} -batch -create_serial -notext -selfsign \
    -extensions v3_ca \
    -policy policy_ca \
    -out "${_autofirma_ca}" \
    -days ${_cert_days} \
    -passin file:"${_temp_dir}/randomkey.txt" \
    -keyfile "${_temp_dir}/autofirma.key" \
    -infiles "${_temp_dir}/autofirma.csr"
  # Make user certificate and key
  ${_req} -new -passout file:"${_temp_dir}/randomkey.txt" \
    -keyout "${_temp_dir}/user.key" \
    -subj "/CN=127.0.0.1" \
    -out "${_temp_dir}/user.csr"
  ${_ca} -batch -notext \
    -extensions usr_cert \
    -policy policy_ca \
    -out "${_temp_dir}/user.cer" \
    -cert "${_autofirma_ca}" \
    -keyfile "${_temp_dir}/autofirma.key" \
    -passin file:"${_temp_dir}/randomkey.txt" \
    -infiles "${_temp_dir}/user.csr"
  # Make user pfx from certificate and key
  openssl pkcs12 -export -passin file:"${_temp_dir}/randomkey.txt" \
    -inkey "${_temp_dir}/user.key" \
    -certfile "${_autofirma_ca}" \
    -in "${_temp_dir}/user.cer" \
    -name "socketautofirma" \
    -passout pass:654321 \
    -out "${_autofirma_pfx}"
  rm -rf ${_temp_dir}
  unset _ca _req _temp_dir
  echo "Certificate generation complete."
}


# If any required cert or key is missing, rebuild it
if [ ! -r "${_autofirma_ca}" ] || [ ! -r "${_autofirma_pfx}" ]; then
  do_init
fi
unset _autofirma_dir _cert_days

# Always update CA in profiles.
# Run in background to not delay app startup.
trust_ca &

# Execute the real Autofirma launcher
exec /app/bin/autofirma.real "$@"

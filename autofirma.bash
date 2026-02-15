#!/bin/bash
#
# Wrapper script for Autofirma Flatpak.
# Partially based on the AUR script from Oscar Garcia Amor (https://ogarcia.me)
# (https://aur.archlinux.org/cgit/aur.git/plain/autofirma?h=autofirma-bin)
# but remodelled to instead use more upstream code.
#
_autofirma_dir="${HOME}/.afirma/Autofirma"
_autofirma_ca="${_autofirma_dir}/Autofirma_ROOT.cer"
_autofirma_pfx="${_autofirma_dir}/autofirma.pfx"
_script_sh="${_autofirma_dir}/script.sh"
_uninstall_sh="${_autofirma_dir}/uninstall.sh"
_cert_cn="SocketAutoFirma"

function do_init {
  echo "Generating new AutoFirma certificates..."
  mkdir -p "${_autofirma_dir}"

  # Copy the configurator jar to the autofirma directory (writable location)
  # The jar will generate certificates in the same directory where it's located
  cp /app/lib/autofirmaConfigurador.jar "${_autofirma_dir}/"

  # Run the configurator from the autofirma directory
  # This generates: autofirma.pfx, AutoFirma_ROOT.cer, script.sh, uninstall.sh
  java -jar "${_autofirma_dir}/autofirmaConfigurador.jar"

  # Remove the jar copy (we don't need it anymore)
  rm -f "${_autofirma_dir}/autofirmaConfigurador.jar"

  # Verify that certificates and scripts were generated
  if [ ! -f "${_autofirma_ca}" ] || [ ! -f "${_autofirma_pfx}" ]; then
    echo "ERROR: Certificate generation failed!"
    return 1
  fi

  if [ ! -f "${_script_sh}" ]; then
    echo "ERROR: script.sh not generated!"
    return 1
  fi

  echo "Certificate generation complete."

  # Uninstall old certificates if uninstall script exists
  if [ -f "${_uninstall_sh}" ]; then
    echo "Removing old certificates..."
    source "${_uninstall_sh}" 2>/dev/null || true
  fi

  # Execute the installation script
  echo "Installing certificates..."
  source "${_script_sh}"

  echo "Certificate installation complete."
}


# If any required cert or key is missing, regenerate everything
if [ ! -r "${_autofirma_ca}" ] || [ ! -r "${_autofirma_pfx}" ]; then
  do_init
fi

unset _autofirma_dir _autofirma_ca _autofirma_pfx _script_sh _uninstall_sh _cert_cn

# Execute the real Autofirma launcher
exec /app/bin/autofirma.real "$@"

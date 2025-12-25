flatpak remove -y com.komect.soho.cloudpc
flatpak run --command=flathub-build org.flatpak.Builder com.komect.soho.cloudpc.yaml
flatpak install --user -y ./repo com.komect.soho.cloudpc
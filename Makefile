all: dingtalk.flatpak

dingtalk.flatpak: com.alibabainc.dingtalk.yaml
	sudo flatpak-builder --force-clean --repo=repo --install-deps-from=flathub --install build com.alibabainc.dingtalk.yaml
	flatpak build-bundle repo dingtalk.flatpak com.alibabainc.dingtalk --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo

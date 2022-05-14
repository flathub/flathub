# Creating flatpak package manually

Execute build.sh and install.sh (remove previous installations first)

# Update to new tuxemon version
- Update tag in flatpak manifest: org.tuxemon.Tuxemon.yaml
- Update tag in updateRequirements.sh script
- Update version of portmidi if exists
- Execute script updateRequirements.sh to regenerate the requirements
- Add build-options again to python3-pygame in python3-requirements.json

Build options for pygame copied from:https://github.com/flathub/com.katawa_shoujo.KatawaShoujo/blob/74e5f93c4a668789f6464ba13017a9737c12764b/pygame/pygame-1.9.6.json#L10-L41

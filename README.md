# Virtual Machine Manager (virt-manager)

**NOTE:** This Flatpak does not include the libvirt daemon. You must install it
on your host or a remote system and connect to it.

For a more integrated experience, look at
[Boxes](https://flathub.org/apps/org.gnome.Boxes).

## Known issues or untested features

As the feature set of virt-manager is quite large, we have not yet tested that
all functionnalities work with the Flatpak. Please report issues in this repo
if you find something that does not work.

### USB redirection

This Flatpak currently has access to all USB devices.

* Appears to be working without workarounds. Are workaround are still needed?
* udev rules and group membership for user?
* an suid helper on the host and flatpak-spawn wrapper in sandbox? polkit?
* polkit rules can't work in the sandbox, need to document all the polkit
  helpers, those probably should be available on the host with their polkit
  rules,
* supply hardlinked binaries to run on the host? rules can be dropped into /etc
* handling usbredirserver calls? or spice-client-glib-usb-acl-helper from
  spice-gtk?

### Full access to SSH configuration

This Flatpak currently has access to `~/.ssh`.

* ssh configuration dir access, includes config, known_hosts, and etc.
* not xdg dirs spec compliant, so without the proper filesystem access
  permission the ssh configuration won't be saved between running sessions.
* enabled by default, if you don't like giving the sandbox this access then
  override it with flatpak-override or flatseal and add a `--persist=.ssh` to
  keep the ssh config dir in HOME/.var/app/org.virt_manager.virt-manager/.ssh
* using persist also means that the sandbox won't have access to your private
  key files so it won't be able to change them, if this is even something to be
  concerned about instead of giving access to the keys, give access to the
  ssh-agent

### Access to host's SSH agent

This Flatpak currently has access to the host's SSH agent.

* access to the running ssh-agent on the host
* it's presumed that SSH_AGENT_PID and SSH_AUTH_SOCK are known in the sandbox,
  and that SSH_AUTH_SOCK is accessible in the sandbox
* the default, `--socket=ssh-auth`, bind-mounts the xdg-run/ssh-auth, only used
  by gnome-keyring's ssh agent but it's a good location as any to put the
  ssh-agent file socket
* if you're not using gnome-keyring's ssh agent then you gonna need to force
  your ssh-agent to create the socket in this mentioned dir, that you will also
  need to create before starting the app, or in another location, and then you
  should add the suitable filesystem permission. just make sure to only
  bind-mount an existing folder, and don't directly bind-mount the socket.
  custom ssh socket location --filesystem=xdg-run/ssh.
* see example in the ssh-agents section in the Arch Wiki's ssh keys page
  https://wiki.archlinux.org/index.php/SSH_keys#SSH_agents

## Links

- [Website](https://virt-manager.org/)
- [Sources](https://github.com/virt-manager/virt-manager)

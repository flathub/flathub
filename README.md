# kcli

[kcli](https://github.com/karmab/kcli) is a tool is meant to interact with existing virtualization providers (libvirt, KubeVirt, oVirt, OpenStack, VMware vSphere, AWS, Azure, GCP, IBM cloud and Hcloud) and to easily deploy and customize VMs from cloud images.

You can also interact with those VMs (list, info, ssh, start, stop, delete, console, serialconsole, add/delete disk, add/delete nic, ...).

Furthermore, you can deploy VMs using predefined profiles, several at once using plan files or entire products for which plans were already created for you.

Refer to kcli [documentation](https://kcli.readthedocs.io/) for more information.

>**NOTE:** This Flatpak includes the libvirt daemon and QEMU. There is no need to install other applications, extensions or anything else in the host.
> libvirt is used from its session interface (`qemu:///session`), which runs VMs completely isolated from their host’s storage and network resources. Session VMs run completely rootless, by default, with user mode networking and user's files as virtual disks.
> The main use case for this flatpak is to support **rootless headless virtual machines**.

## Quick start

Instruct kcli to use `qemu:///session`:
```bash
echo 'default:
  session: true
  vnc: false
  protocol: ssh
  user: root
  enableroot: true
  tunnelport: 22
  host: 127.0.0.1
  insecure: true
  start: true
  wait: false
  networkwait: 0' > ~/.kcli/config.yml 
```

Create a default pool on your home directory:
```bash
mkdir -p ~/.kcli/images
flatpak run io.github.karmab.kcli create pool -p ~/.kcli/images default
```

Deploy your first vm with:
```bash
flatpak run io.github.karmab.kcli create vm -i centos8stream -P usermode_backend=slirp myvm
flatpak run io.github.karmab.kcli list vm
# wait 5-10 seconds for vm to grab an ip
flatpak run io.github.karmab.kcli ssh myvm
flatpak run io.github.karmab.kcli delete vm
```

## Use with CLI

You can open the sandboxed Bash environment with the following command:
```bash
flatpak run --command=sh io.github.karmab.kcli
```

This launches a shell within the Flatpak sandbox for kcli, allowing you to run CLI tools like `virsh`, `virt-admin` or execute other commands related to virtual machine management.

## Known issues or untested features

As the feature set of kcli is quite large, we have not yet tested that all functionalities work with the Flatpak. The main use case for this flatpak is to support **rootless headless virtual machines**.

Please report issues in this repo if you find something that does not work.

## Manual build/install

```bash
org.flatpak.Builder build-dir io.github.karmab.kcli.yaml --force-clean
org.flatpak.Builder --user --install --force-clean build-dir io.github.karmab.kcli.yaml
```

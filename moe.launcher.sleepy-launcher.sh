#!/bin/sh

# Modify /etc/hosts to block logging servers
# This is possible because /etc is a writable tmpfs in flatpak
if readlink /etc/hosts > /dev/null; then
    # /etc/hosts is a symlink by default, if it is, copy the original and modify it
    # Otherwise, we already modified it
    original=$(readlink /etc/hosts)
    rm /etc/hosts
    cp $original /etc/hosts
    cat <<EOF >> /etc/hosts
# Global
# Genshin logging servers (do not remove!)
0.0.0.0 overseauspider.yuanshen.com
0.0.0.0 log-upload-os.hoyoverse.com
0.0.0.0 log-upload-os.mihoyo.com
0.0.0.0 sg-public-data-api.hoyoverse.com
0.0.0.0 apm-log-upload-os.hoyoverse.com
0.0.0.0 zzz-log-upload-os.hoyoverse.com

# China
# Genshin logging servers (do not remove!)
0.0.0.0 log-upload.mihoyo.com
0.0.0.0 uspider.yuanshen.com
0.0.0.0 public-data-api.mihoyo.com
0.0.0.0 apm-log-upload-os.hoyoverse.com
0.0.0.0 zzz-log-upload-os.hoyoverse.com
EOF

    # If NO_BLOCK_PROXY is set, don't block the proxy/cdn servers
    if [ -z "$NO_BLOCK_PROXY" ]; then
        cat <<EOF >> /etc/hosts
# Optional Unity proxy/cdn servers
0.0.0.0 prd-lender.cdp.internal.unity3d.com
0.0.0.0 thind-prd-knob.data.ie.unity3d.com
0.0.0.0 thind-gke-usc.prd.data.corp.unity3d.com
0.0.0.0 cdp.cloud.unity3d.com
0.0.0.0 remote-config-proxy-prd.uca.cloud.unity3d.com
EOF
    fi
fi

export PATH=$PATH:/usr/lib/extensions/vulkan/gamescope/bin

exec sleepy-launcher "$@"

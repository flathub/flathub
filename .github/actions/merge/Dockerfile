FROM quay.io/fedora/fedora:38

RUN dnf install -y flatpak-builder git /usr/bin/pip && \
    dnf clean all

ADD requirements.txt /requirements.txt
RUN pip install -r /requirements.txt

ADD entrypoint.py /entrypoint.py
ENTRYPOINT ["/entrypoint.py"]

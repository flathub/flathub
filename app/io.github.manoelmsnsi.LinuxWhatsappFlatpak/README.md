# WhatsApp GTK

WhatsApp GTK é um cliente leve e nativo para **WhatsApp Web**, desenvolvido em **Python**, **GTK 3** e **WebKit2**.  
O objetivo do projeto é oferecer uma experiência integrada ao desktop Linux, com sandbox, bom desempenho e isolamento de dados.

> ⚠️ **Aviso legal**  
> Este aplicativo **não é oficial**, não é afiliado, patrocinado ou endossado pela Meta ou pelo WhatsApp.

---

## ✨ Funcionalidades

- Interface nativa GTK
- Baseado em WhatsApp Web
- Isolamento de cookies e cache
- Integração com notificações do sistema
- Links externos abertos no navegador padrão
- Funciona em Wayland e X11
- Distribuído como Flatpak (sandboxed)

---

## 📦 Instalação

### Flatpak (recomendado)

```bash
flatpak install flathub io.github.manoelmsnsi.LinuxWhatsappFlatpak
```

Executar:

```bash
flatpak run io.github.manoelmsnsi.LinuxWhatsappFlatpak
```

---

## 🔧 Build manual (desenvolvimento)

### Dependências

- Flatpak
- flatpak-builder
- org.gnome.Platform
- org.gnome.Sdk

### Instalação das dependências

Adicione o repositório Flathub (se ainda não tiver):

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

Instale o runtime e SDK do GNOME:

```bash
flatpak install flathub org.gnome.Platform//48 org.gnome.Sdk//48
```

### Build e instalação local

Clone o repositório:

```bash
git clone https://github.com/manoelmsnsi/linux-whatsapp-flatpak.git
cd linux-whatsapp-flatpak
```

Build e instale:

```bash
flatpak-builder --user --install --force-clean build-dir manifest.yml
```

Execute o aplicativo:

```bash
flatpak run io.github.manoelmsnsi.LinuxWhatsappFlatpak
```

---

## 🧱 Tecnologias utilizadas

- Python 3
- GTK 3
- WebKit2GTK
- Flatpak

---

## 🔐 Privacidade

- Nenhum dado é coletado
- Cookies e cache ficam isolados dentro do sandbox do Flatpak
- Nenhuma comunicação além do WhatsApp Web

---

## 🐞 Problemas e sugestões

Relate bugs ou sugestões em:

https://github.com/manoelmsnsi/linux-whatsapp-flatpak/issues

---

## 📄 Licença

GPL-3.0-or-later

---

## 🙌 Autor

Manoel Messias  
https://github.com/manoelmsnsi
# linux-whatsapp-flatpak

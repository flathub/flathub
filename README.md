# Adalet E-İmza

Adalet Bakanlığı'nın, tarayıcı tabanlı UYAP sistemlerinde akıllı kart veya USB token
üzerindeki sertifika ile imza atmayı sağlayan yardımcı uygulaması Adalet E-İmza'nın
Flatpak paketi.

## Kullanım

```bash
flatpak install flathub tr.gov.adalet.eimza-tray
flatpak run tr.gov.adalet.eimza-tray
```

- Sistemde `pcscd` servisi çalışıyor ve kart okuyucunuzun sürücüsü kurulu olmalıdır.
- Uygulama tarayıcıya `127.0.0.1:5975` adresinden hizmet verir. Önce bakanlığın deb
  paketini veya AppImage'ını kaldırın, yoksa ikisi de aynı portu kullanmaya çalışır.
- Oturum açılışında kendiliğinden başlamaz. İmzalamadan önce uygulamayı başlatın.

## Derleme

```bash
git clone --recurse-submodules https://github.com/flathub/tr.gov.adalet.eimza-tray.git
cd tr.gov.adalet.eimza-tray
flatpak run --command=flathub-build org.flatpak.Builder tr.gov.adalet.eimza-tray.yaml
flatpak install --user -y ./repo tr.gov.adalet.eimza-tray
```

## Güncelleme

Uygulama, bakanlığın paketlediği başlatıcı ve Java yerine Flathub'ın OpenJDK'sı ile
çalışır; bu yüzden `platform/adalet-eimza-tray.sh` başlatıcının JVM seçeneklerini
tekrarlar. Sürüm yükseltmesini birleştirmeden önce seçeneklerin değişmediğini
kontrol edin:

```bash
tools/launcher-options.sh | diff tools/launcher-options.txt -
```

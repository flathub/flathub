from pathlib import Path
import sys

if sys.argv[1] == "wrapper":
    p = Path("wrapper-lite-qemu.cpp")
    text = p.read_text()
    old = 'std::string argsFile = assetDir + "/.lite-qemu-args";'
    assert old in text
    text = text.replace(old, 'std::string argsFile = g_dataImg + ".args";')
    p.write_text(text)
else:
    for root in (Path("src"), Path("data"), Path("po")):
        for p in list(root.rglob("*")):
            if not p.is_file():
                continue
            try:
                text = p.read_text()
            except UnicodeDecodeError:
                continue
            text = text.replace("app.marchfields.Polyphony", "app.marchfields.Polyphony.Local")
            text = text.replace("/app/marchfields/Polyphony", "/app/marchfields/Polyphony/Local")
            if p.name == "main.py":
                text = text.replace('"Polyphony"', '"Polyphony (Local)"').replace("application_name='Polyphony'", "application_name='Polyphony (Local)'")
            if p.suffix == ".py":
                text = text.replace("8129", "18129")
                text = text.replace("127.0.0.1:8080", "127.0.0.1:18080")
                text = text.replace("LITE_HOST_PORT = 8080", "LITE_HOST_PORT = 18080")
            if p.name.endswith(".desktop.in"):
                text = text.replace("Name=Polyphony", "Name=Polyphony (Local)")
            p.write_text(text)
    for p in sorted(Path("data").rglob("*"), key=lambda x: len(x.parts), reverse=True):
        if "app.marchfields.Polyphony" in p.name:
            p.rename(p.with_name(p.name.replace("app.marchfields.Polyphony", "app.marchfields.Polyphony.Local")))

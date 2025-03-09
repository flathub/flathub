from setuptools import setup

setup(
    name="scummvm_wrapper",
    description="ScummVM wrapper for Flatpak",
    py_modules=["scummvm_wrapper"],
    version="1.0.0",
    entry_points={
        "console_scripts": [
            "scummvm_wrapper = scummvm_wrapper:main"
        ]
    }
)

from setuptools import setup

setup(
    name='webp_converter',
    version='1.0.0',
    description='Convert images to WebP format using a GTK interface.',
    author='Michael Scuteri',
    author_email='michaelscuteridev@gmail.com',
    license='MIT',
    scripts=['webp_converter.py'],
    data_files=[
        ('share/applications', ['desktop/webp_converter.desktop']),
        ('share/icons/hicolor/256x256/apps', ['data/icons/webp_converter.png']),
    ],
    classifiers=[
        'Environment :: X11 Applications :: GTK',
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
    ],
)


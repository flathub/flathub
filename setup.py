import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="cine-encoder-helg1980", # Replace with your own username
    version="1.0",
    author="Oleg Kozhukharenko",
    author_email="depositmail@mail.ru",
    description="Encoder",
    long_description=long_description,
    long_description_content_type="video",
    url="https://github.com/CineEncoder/cine-encoder",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Python Software Foundation License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)

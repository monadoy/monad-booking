import click
import requests
import platform
from zipfile import ZipFile
import io
import tarfile
import os
import shutil


@click.group(chain=True)
def cli():
    pass


MKLITTLEFS_MINGW = "https://github.com/earlephilhower/mklittlefs/releases/download/3.0.0/x86_64-w64-mingw32-mklittlefs-295fe9b.zip"
MKLITTLEFS_APPLE = "https://github.com/earlephilhower/mklittlefs/releases/download/3.0.0/x86_64-apple-darwin14-mklittlefs-295fe9b.tar.gz"
MKLITTLEFS_GNU = "https://github.com/earlephilhower/mklittlefs/releases/download/3.0.0/x86_64-linux-gnu-mklittlefs-295fe9b.tar.gz"


@cli.command()
def littlefs():
    system = platform.system()
    url = ""
    if system == "Windows":
        url = MKLITTLEFS_MINGW
    elif system == "Darwin":
        url = MKLITTLEFS_APPLE
    elif system == "Linux":
        url = MKLITTLEFS_GNU
    else:
        click.echo("Unsupported system: " + system, err=True)
        return

    res = requests.get(url)

    if system == "Windows":
        zip = ZipFile(io.BytesIO(res.content))
        zip.extractall("./")
    else:
        tar = tarfile.open(fileobj=io.BytesIO(res.content))
        tar.extractall("./")

    click.echo("Done")


@cli.command()
def frontend():
    os.chdir(
        os.path.dirname(os.path.realpath(__file__)) + "/webPortalFrontend")

    os.system("npm install")
    os.system("npm run build")
    os.chdir("..")

    if os.path.exists("./data/webroot"):
        shutil.rmtree("./data/webroot")

    shutil.copytree("./webPortalFrontend/dist", "./data/webroot")

    click.echo("Done")


if __name__ == "__main__":
    cli()

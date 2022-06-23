import click
import numpy as np
import png
import os
from PIL import Image


@click.command()
@click.argument("image_path_or_folder")
def main(image_path_or_folder):
    if os.path.isdir(image_path_or_folder):
        for file in os.listdir(image_path_or_folder):
            if file.endswith(".png") and not file.endswith("_4bpp.png"):
                convert(file)
    else:
        convert(image_path_or_folder)


def convert(image_path):
    click.echo("Converting image: " + image_path)
    # Convert to grayscale with transparency
    orig = Image.open(image_path).convert("LA")

    # Add a white background
    image = Image.new("LA", orig.size, "WHITE")
    image.paste(orig, mask=orig)

    # Convert to grayscale with no transparency
    image = image.convert("L")

    # Quantize
    im = np.array(image, dtype=np.float64)
    im /= 17
    im = np.rint(im).astype(np.uint8)

    # Save as 4 bit grayscale non-transparent png
    png.from_array(im, "L;4").save(image_path.replace(".png", "_4bpp.png"))

    click.echo("Done")


if __name__ == "__main__":
    main()

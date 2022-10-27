import click
import numpy as np
import png
import os
from PIL import Image


def quantize(im, levels):
    im = im.astype(np.float64)
    # values that are exactly 1 will overflow without this fraction
    im /= 255.00000001
    im *= levels
    return im.astype(np.uint8)


@click.command()
@click.argument("image_path_or_folder")
@click.option("--four-colours", is_flag=True)
@click.option("--uniform/--no-uniform", default=True)
def main(image_path_or_folder, four_colours, uniform):
    if os.path.isdir(image_path_or_folder):
        for root, _dirs, files in os.walk(image_path_or_folder):
            for file in files:
                if file.endswith(".png") and not file.endswith("_4bpp.png"):
                    convert(root + "/" + file, four_colours, uniform)
    else:
        convert(image_path_or_folder, four_colours, uniform)


def convert(image_path, four_colours, uniform):
    click.echo("Converting image: " + image_path)
    # Convert to grayscale with transparency
    orig = Image.open(image_path).convert("LA")

    # Add a white background
    image = Image.new("LA", orig.size, "WHITE")
    image.paste(orig, mask=orig)

    # Convert to grayscale with no transparency
    image = image.convert("L")

    if four_colours:
        if uniform:
            im = np.array(image, dtype=np.uint32)
            im = quantize(im, 4) * 5
        else:
            image = image.quantize(4)
            im = np.array(image, dtype=np.uint8)
            im = (3 - im) * 5
    else:
        if uniform:
            im = np.array(image, dtype=np.uint32)
            im = quantize(im, 16)
        else:
            image = image.quantize(16)
            im = np.array(image, dtype=np.uint8)
            im = (15 - im)

    # Save as 4 bit grayscale non-transparent png
    png.from_array(im, "L;4").save(
        image_path.replace(".png",
                           f"{'_4col' if four_colours else ''}_4bpp.png"))

    click.echo("Done")


if __name__ == "__main__":
    main()

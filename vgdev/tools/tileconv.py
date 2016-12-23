import click
from itertools import islice
import png

def read_png(stream):
    w, h, rows, info = list(png.Reader(stream).read())
    for ty in range(0, h // 8):
        row = list(islice(rows, 0, 8))
        for tx in range(0, w // 8):
            x = tx * 8
            yield list(map(lambda p: p[x:x+8], row)), tx, ty

def write_1bpp(stream, tile, shift=0):
    for row in tile:
        bp0 = 0
        for pixel in row:
            bp0 = (bp0 << 1) | (pixel >> shift & 1)
        stream.write(bytes((bp0,)))

def write_2bpp(stream, tile):
    for row in tile:
        bp0 = 0
        bp1 = 0
        for pixel in row:
            bp0 = (bp0 << 1) | (pixel >> 0 & 1)
            bp1 = (bp1 << 1) | (pixel >> 1 & 1)
        stream.write(bytes((bp0, bp1)))

def write_3bpp(stream, tile):
    write_2bpp(stream, tile)
    write_1bpp(stream, tile, shift=3)

BLANK_TILE = [[0] * 8] * 8

WRITERS = {
    '1bpp':write_1bpp,
    '2bpp':write_2bpp,
    '3bpp':write_3bpp,
}

@click.command()
@click.argument('output', type=click.File('wb'))
@click.argument('inputs', type=click.File('rb'), nargs=-1)
@click.option('-f', '--format', type=click.Choice(WRITERS.keys()), required=True)
@click.option('-0', '--blank', is_flag=True,
    help="Fill the first tile slot with a blank tile")
def tileconv(output, inputs, format, blank):
    write = WRITERS[format]
    if blank:
        write(output, BLANK_TILE)
    for input in inputs:
        for tile, tx, ty in read_png(input):
            write(output, tile)

if __name__ == '__main__':
    tileconv()

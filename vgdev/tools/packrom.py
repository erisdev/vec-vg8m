import click
import struct

MAGIC = b'VGDMP100'
HEADER_FMT = '<8sHHHHHHH'
HEADER_LEN = struct.calcsize(HEADER_FMT)

CHUNK_SIZE = 512
def iter_chunks(f):
    while True:
        chunk = f.read(CHUNK_SIZE)
        if chunk:
            yield chunk
        else:
            return

def append_file(output, input):
    start = output.tell()
    for chunk in iter_chunks(input):
        output.write(chunk)
    return start, output.tell() - start

@click.command()
@click.argument('output', type=click.File('wb'))
@click.argument('program', type=click.File('rb'))
@click.option('--mappat', type=click.File('rb'),
    help='map pattern data')
@click.option('--sprpat', type=click.File('rb'),
    help='sprite pattern data')
def packrom(output, program, mappat, sprpat):
    flags = 0
    mappat_offset, mappat_size = 0, 0
    sprpat_offset, sprpat_size = 0, 0

    output.seek(HEADER_LEN)

    prog_offset, prog_size = append_file(output, program)
    if mappat:
        pappat_offset, mappat_size = append_file(output, mappat)
    if sprpat:
        mappat_offset, sprpat_size = append_file(output, sprpat)

    output.seek(0)
    output.write(struct.pack(HEADER_FMT,
        MAGIC,
        flags,
        prog_offset,   prog_size,
        mappat_offset, mappat_size,
        sprpat_offset, sprpat_size))

if __name__ == '__main__':
    packrom()

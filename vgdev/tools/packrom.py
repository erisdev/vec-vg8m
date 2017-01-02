import click
from struct import Struct
import os

MAGIC = b'VGDMP100'
FILE_HEADER = Struct('<8sHH')
BANK_HEADER = Struct('<HHBBH')

SLOT_ID = {
    'prog': 0x00,
    'ext':  0x01,
    'bg':   0x02,
    'spr':  0x03,

    'bios': 0xF0,
    'txt':  0xF1,
}

SLOT_SIZE = {
    'prog': 0x4000,
    'ext':  0x4000,
    'bg':   0x4000,
    'spr':  0x3000,

    'bios': 0x0800,
    'txt':  0x0800,
}

BankSlot = click.Choice(SLOT_ID.keys())

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
@click.option('--bank', 'banks', multiple=True, type=(BankSlot, click.File('rb')))
def packrom(output, banks):
    mappat_offset, mappat_size = 0, 0
    sprpat_offset, sprpat_size = 0, 0

    output.seek(FILE_HEADER.size)

    id_counter = dict(map(lambda x: (x, 0), SLOT_ID.keys()))
    for (slot, input) in banks:
        id = id_counter[slot]
        id_counter[slot] += 1

        # skip bank header & write the bank data first
        output.seek(BANK_HEADER.size, 1)
        bank_offset, bank_size = append_file(output, input)

        # now that bank size is known, jump back to the header and write it
        output.seek(bank_offset - BANK_HEADER.size)
        output.write(BANK_HEADER.pack(bank_size, 0, SLOT_ID[slot], id, 0))

        # finally, jump to the end of the bank data and continue
        output.seek(bank_offset + bank_size)

        if bank_size > SLOT_SIZE[slot]:
            print("rom bank {:02x}:{:02x} is larger than 0x{:04x} bytes".format(
                slot, id, SLOT_SIZE[slot]), file=os.stderr)


    output.seek(0)
    output.write(FILE_HEADER.pack(MAGIC, 0, len(banks)))

if __name__ == '__main__':
    packrom()

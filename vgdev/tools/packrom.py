import click
from struct import Struct

BankSlot = click.Choice(('prog', 'ext', 'bg', 'spr'))

MAGIC = b'VGDMP100'
FILE_HEADER = Struct('<8sHH')
BANK_HEADER = Struct('<HHBBH')

SLOT_ID = {'prog': 0, 'ext': 1, 'bg': 2, 'spr': 3}

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

    id_counter = {'prog': 0, 'ext': 0, 'bg': 0, 'spr': 0};
    for (slot, input) in banks:
        # skip bank header & write the bank data first
        output.seek(BANK_HEADER.size, 1)
        bank_offset, bank_size = append_file(output, input)

        # now that bank size is known, jump back to the header and write it
        output.seek(bank_offset - BANK_HEADER.size)
        output.write(BANK_HEADER.pack(bank_size, 0, SLOT_ID[slot], id_counter[slot], 0))
        id_counter[slot] += 1

        # finally, jump to the end of the bank data and continue
        output.seek(bank_offset + bank_size)


    output.seek(0)
    output.write(FILE_HEADER.pack(MAGIC, 0, len(banks)))

if __name__ == '__main__':
    packrom()

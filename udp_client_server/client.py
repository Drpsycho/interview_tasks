import socket
import struct
import random
import argparse
from pathlib import Path
from random import randint

#external depends
import crc32c

Port = 9000
IP = "127.0.0.1"

def randomID():
    return (randint(1, 99999999)).to_bytes(8,byteorder='big')

def client(filepath):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
    chunks=[]
    filename = filepath
    chunksize = 1455

    crc_whole_file = 0
    with open(filename, "rb") as f:
        chunk_number = 0
        while True:
            chunk = f.read(chunksize)
            if chunk:
                crc_whole_file =  crc32c.crc32c(chunk, crc_whole_file)
                chunks.append((chunk,chunk_number))
                chunk_number += 1
            else:
                break  

    random.shuffle(chunks)
    total_packet = len(chunks)
    
    already_sent = 0
    id = randomID()
    PUT = 1
    for temp_chunk in chunks:
        chunk_data = temp_chunk[0]
        chunk_number = temp_chunk[1]
        crc = crc32c.crc32c(chunk_data, 0)

        data = struct.pack(f'<llB8s{len(chunk_data)}s', chunk_number, total_packet, PUT, id, chunk_data)

        s.sendto(data, (IP, Port))
        already_sent += 1
        data, address = s.recvfrom(4096)

        seq_number, seq_total, type, id, crc_from_server = list(struct.unpack('<llB8s4s', data))
        crc_from_server = int.from_bytes(crc_from_server,"little")
        if(seq_total != total_packet):
            pass
            print('crc32 from client = {:#010x}'.format(crc))
            print('crc32 from server = {:#010x}'.format(crc_from_server))
            print(f"\nsent {seq_total} from {total_packet}\npacket number is {chunk_number}")
        else:
            print("The checksum of the entire file is")
            print('crc32 from client = {:#010x}'.format(crc_whole_file))
            print('crc32 from server = {:#010x}'.format(crc_from_server))
            print("done")

    s.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Client for the UDP server.')
    parser.add_argument("filepath", type=Path)
    p = parser.parse_args()
    client(p.filepath)

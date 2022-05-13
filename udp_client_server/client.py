import socket
import struct
import random
import argparse
from pathlib import Path

#external depends
import crc32c

Port = 9000
IP = "127.0.0.1"

def client(filepath):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)

    hello = bytes("Hello!", 'utf-8')
    data = struct.pack('<llB8s1455s', 3, 2, 0, b'00000001', hello)

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

    print("file crc = " + str(crc_whole_file))
    random.shuffle(chunks)
    print(len(chunks))
    total_packet = len(chunks)
    crc = 0
    already_sent = 0
    for temp_chunk in chunks:
        chunk_data = temp_chunk[0]
        chunk_number = temp_chunk[1]
        crc = 0
        crc = crc32c.crc32c(chunk_data, crc)
        print('crc32 client = {:#010x}'.format(crc))

        data = struct.pack(f'<llB8s{len(chunk_data)}s', chunk_number, total_packet, 1, (1).to_bytes(8,byteorder='big'), chunk_data)

        s.sendto(data, (IP, Port))
        already_sent += 1
        # print("\n\n 1. Client Sent : ", send_data, "\n\n")
        data, address = s.recvfrom(4096)
        # 4+4+1+8+4
        seq_number, seq_number, type, id, crc_from_server = list(struct.unpack('<llB8s4s', data))
        crc_from_server = int.from_bytes(crc_from_server,"little")
        print('crc32 from server = {:#010x}'.format(crc_from_server))
        print(f"sent {already_sent} from {total_packet}")
        if crc_from_server != crc:
            print('crc32 whole file = {:#010x}'.format(crc_whole_file))
            print('crc32 from server = {:#010x}'.format(crc_from_server))
            print('crc32 from client = {:#010x}'.format(crc))
            exit(0)
        # print("\n\n 2. Client received : ", data.decode('utf-8'), "\n\n")
    print("done")
    print("crc of the file\nfrom client: "+str(crc_whole_file)+"\nfrom server: " +str(crc)+"\n")
    s.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Client for the UDP server.')
    parser.add_argument("filepath", type=Path)
    p = parser.parse_args()
    client(p.filepath)

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

def readfile_and_get_checksum(filename):
    chunksize = 1455
    chunks=[]
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
    return chunks, crc_whole_file

def send_package(client_socket, chunk_data, chunk_number, id, total_packet) :
    PUT = 1
    data = struct.pack(f'<llB8s{len(chunk_data)}s', chunk_number, total_packet, PUT, id, chunk_data)
    return client_socket.sendto(data, (IP, Port)) == len(data) 

def recive_package(client_socket):    
    data, address = client_socket.recvfrom(4096)
    seq_number, seq_total, type, id, crc_from_server = list(struct.unpack('<llB8s4s', data))
    crc_from_server = int.from_bytes(crc_from_server,"little")
    return seq_total, crc_from_server

def client(path_to_file):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)    
    chunks, crc_whole_file = readfile_and_get_checksum(path_to_file)
    random.shuffle(chunks)
    total_packet = len(chunks)
    
    already_sent = 0
    id = randomID()
    
    for chunk_data, chunk_number  in chunks:
        crc = crc32c.crc32c(chunk_data, 0)

        while(True):
            send_package(client_socket, chunk_data, chunk_number, id, total_packet)
            seq_total, crc_from_server = recive_package(client_socket)
            if crc == crc_from_server:
                # print('crc32 from client = {:#010x}'.format(crc))
                # print('crc32 from server = {:#010x}'.format(crc_from_server))
                # print(f"\nsent {seq_total} from {total_packet}\npacket number is {chunk_number}")
                break
            if total_packet == seq_total:
                print(f"\nDone, sent {seq_total} packets!\nThe checksum of the entire file is")
                print('crc32 from client = {:#010x}'.format(crc_whole_file))
                print('crc32 from server = {:#010x}'.format(crc_from_server))
                break
        already_sent += 1
    client_socket.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Client for the UDP server.')
    parser.add_argument("filepath", type=Path)
    p = parser.parse_args()
    client(p.filepath)
    exit(0)

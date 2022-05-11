/*
    Simple udp server
*/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>

#include <stdexcept>
#include <iostream>
#include <array>
#include <vector>
#include <memory_pull.hpp>
#include <thread>
#include <map>
#include <unordered_map>
#include <fstream>

constexpr auto BUFLEN = 512;	//Max length of buffer
constexpr auto PORT = 9889;	//The port on which to listen for incoming data
constexpr auto max_size_udp_packet = 1472; //byte
constexpr auto payload_size = max_size_udp_packet-(4+4+1+8);

enum packet_type{
    ACK = 0,
    PUT = 1
};

#pragma pack(push, 1)
struct udp_packet{
    uint32_t seq_number;                            // номер пакета
    uint32_t seq_total;                             // количество пакетов с данными
    uint8_t  type;                                  // тип пакета: 0 == ACK, 1 == PUT
    uint8_t  id[8];                                 // 8 байт - идентификатор, отличающий один файл от другого
    uint8_t  data[payload_size];   // после заголовка и до конца UDP пакета идут данные
};
#pragma pack(pop)

#define POLY 0x82f63b78 
uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
{
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
    }
    return ~crc;
}

class server_udp{
    int sock_descriptor;
    sockaddr_in sock_addres = {};
    sockaddr_in si_other = {};
    memory_pool _mempool;

    std::unordered_map<uint64_t, std::map<uint32_t, std::vector<uint8_t>> > temporary_storage;

    public:
    server_udp(): _mempool(){

        if( (sock_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            std::cout << "socket descriptor  = " << sock_descriptor << " errno = "<< strerror(errno)<< "\n"; 
            throw std::runtime_error("can't create socket!"); 
        }
        std::cout << "socket descriptor = " << sock_descriptor << "\n";
        
        sock_addres.sin_family = AF_INET;
        sock_addres.sin_port = htons(PORT);
        sock_addres.sin_addr.s_addr = htonl(INADDR_ANY);

        if( auto result = bind(sock_descriptor , (struct sockaddr*)&sock_addres, sizeof(sock_addres)); result == -1){
            std::cout << "bind result  = " << result << " errno = "<< strerror(errno)<< "\n"; 
            throw std::runtime_error("can't bind socket!");
        }
    }

    void handler(){
        for(;;){
            auto buf = _mempool.pop();
            if (buf.first == nullptr){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            {
                // if(buf.first->byte_used == sizeof(udp_packet)){
                    udp_packet _udp_packet{};
                    auto payload_size = buf.first->byte_used - sizeof(uint8_t)*(4+4+1+8);
                    std::cout << "byte used: " << buf.first->byte_used << "\n";
                    memcpy(&_udp_packet, buf.first->pointer(), buf.first->byte_used);

                    // {
                    //     std::cout << "sequnce number: " << _udp_packet.seq_number << "\n";
                    //     std::cout << "total byte: " << _udp_packet.seq_total << "\n";
                    //     std::cout << "packet type: " << (_udp_packet.type ? "PUT" : "ACK")  << "\n";
                    //     std::cout << "id: " << _udp_packet.id[7] << "\n";
                    //     std::cout << "msg: " << _udp_packet.data[1] <<"\n";
                    // }
                    
                    uint64_t id = (uint64_t)(_udp_packet.id[7]) | 
                    (uint64_t)(_udp_packet.id[6]) << 8  |
                    (uint64_t)(_udp_packet.id[5]) << 16 |
                    (uint64_t)(_udp_packet.id[4]) << 24 |
                    (uint64_t)(_udp_packet.id[3]) << 32 |
                    (uint64_t)(_udp_packet.id[2]) << 40 |
                    (uint64_t)(_udp_packet.id[1]) << 48 |
                    (uint64_t)(_udp_packet.id[0]) << 56;

                    {
                        if(temporary_storage.find(id) == temporary_storage.end()){
                            temporary_storage[id] = {{_udp_packet.seq_number, {_udp_packet.data, _udp_packet.data+payload_size}}};
                        }else{
                            temporary_storage.at(id)[_udp_packet.seq_number] = {_udp_packet.data, _udp_packet.data+payload_size};
                        }
                    }

                    uint32_t crc = 0;
                    if( temporary_storage.at(id).size() == _udp_packet.seq_total){
                        std::ofstream fs("example.txt", std::ios::out | std::ios::binary | std::ios::app);

                        for(auto it : temporary_storage.at(id)){
                            crc = crc32c(crc, it.second.data(), it.second.size());
                            fs.write((char *)it.second.data(), it.second.size());
                        }
                        fs.close();
                        temporary_storage.at(id).clear();
                    } else {
                        crc = crc32c(crc, _udp_packet.data, payload_size);
                    }
                    _udp_packet.type = ACK;
                    memcpy(&_udp_packet.data, &crc, sizeof(crc));
                
                    if (sendto(sock_descriptor, &_udp_packet, (4+4+1+8+4), 0, &buf.second, sizeof(sockaddr)) == -1){
                        std::cout << "Can't send to: " << "\n";
                    }
                    std::cout << "crc is " << std::hex << crc << "\n";
            
            }
            _mempool.free(buf.first);
        }
    }

    void reciver(){
        sockaddr socket_from = {};
        auto socket_from_len = sizeof(socket_from);
        ssize_t recv_len = 0;
        for(;;)
        {
            auto buf = _mempool.get();
            if(!buf){
                std::cout << "error! memory pull is empty\n";
            }

            //handling big message
            if ((recv_len = recvfrom(sock_descriptor, buf->pointer(), buf->size(), 0, &socket_from,(socklen_t *) &socket_from_len)) == -1)
                throw std::logic_error("can't recive data from socket!");
            buf->byte_used = recv_len;

            // {
            //     std::string s(buf->pointer(), buf->byte_used);
            //     std::cout << "reciver Incoming data: " << s << "\n";
            // }
            _mempool.commit(buf, socket_from);
            //print details of the client/peer and the data received
            // {
            //     printf("Received packet from %s:%d\n", inet_ntoa(socket_from.sin_addr), ntohs(socket_from.sin_port));
            // }
        }
    }
};

int main(void)
{
    server_udp _server_udp;
    std::thread worker (&server_udp::handler, &_server_udp);
    _server_udp.reciver();

    return 0;
}

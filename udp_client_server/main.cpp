#include <thread>
#include <udp_server.hpp>

int main(void)
{
    server_udp _server_udp;
    std::thread worker (&server_udp::handler, &_server_udp);
    _server_udp.reciver();

    return 0;
}

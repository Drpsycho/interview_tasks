# Udp client server

This applications demonstrates sending any files divided into chunks in any order via UDP.
The server is able to accept all these chunks and assemble the source file back.

For the server used the standard C++ library and part of the standard libraries from Linux (no **ASIO** or etc)

For the client used the standard Python 3 and third party library for the check sum calculating (crc32c fast)

# Usage
## server
udp_server bind by default 9000 port. Start without any argumets.

```./udp_server```


## client 

depends from the module "crc32c"

```pip3 install crc32c```

```client.py path_to_file``` 

### helpers:

* build.sh script for building  udp_server
* run.sh demo with udp_server and client.py
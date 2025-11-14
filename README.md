# Online Awale XTrem Experience
**[4IF INSA Lyon]** *Online Awale game using sockets*

## Building
To build the project launch the following command in a terminal while beign in the root directory of this project.
```sh
make all
```

## Launching
There are two parts to this project, Client and Serveur.
The server part must be launched before any client connects.
```sh
./bin/server
```

To connect as a client to a running server use :
```sh
./bin/client [server_ip] [username]
```
Example : 
```sh
./bin/client 127.0.0.1 Baudouin
```
#ifndef PENK_NETWORK_CPP

#define PENK_NETWORK_CPP

#include "penk.cpp"

#include <enet/enet.h>

enum PenkNetworkType { CLIENT, SERVER };

bool initialized = false;

void NetworkInit() {
    penkAssert(enet_initialize() == 0, "unable to initialize ENet! (networking)");
    atexit(enet_deinitialize);
    initialized = true;
}

struct Client {
    bool error;
    ENetHost* client;
    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;
};

Client GenerateClientError(const char *error) {
    printf("[PENK.NETWORK] %s!\n", error);
    return Client{true};
}

Client CreateClient(const char* host, int port) {
    if(!initialized) {
        return generateClientError("Not initialized");
    }

    Client output_client;
    output_client.error = false;

    output_client.client = enet_host_create(NULL, 1, 1, 0, 0);
    
    if(output_client.client == NULL) {
        return generateClientError("Failed to create a host");
    }

    enet_address_set_host(&output_client.address, host);
    output_client.address.port = port;
    output_client.peer = enet_host_connect(output_client.client, &output_client.address, 1, 0);
    
    if(output_client.peer == NULL) {
        return generateClientError("No available peers");
    }

    if(enet_host_service(output_client.client, &output_client.event, 5000) > 0 && output_client.event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("[PENK.NETWORK] Connected to %s:%i successfully\n", host, port);
    } else {
        enet_peer_reset(output_client.peer);
        return generateClientError("Unable to connect");
    }

    return output_client;
}

void UpdateClient(Client client, bool debug = false, int sleep = 20) {
    while(enet_host_service(client.client, &client.event, sleep) > 0) {
        switch(client.event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                if(debug) {
                    printf("[PENK.NETWORK] A packet of length %u (%s) was received from %x:%u on channel %u:\n",
                        client.event.packet -> dataLength,
                        client.event.packet -> data,
                        client.event.peer -> address.host,
                        client.event.peer -> address.port,
                        client.event.channelID);
                }
                break;
        }
    }
}

void DestroyClient(Client client) {
    enet_peer_disconnect(client.peer, 0);
    while(enet_host_service(client.client, &client.event, 250) > 0) {
        switch(client.event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(client.event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("[PENK.NETWORK] Disconnected\n");
                break;
        }
    }
}

struct Server {
    bool error;
    ENetHost* server;
    ENetAddress address;
    ENetEvent event;
};

Server GenerateServerError(const char *error) {
    printf("[PENK.NETWORK] %s!\n", error);
    return Server{true};
}

Server CreateServer(int port, int player_limit) {
    if(!initialized) {
        return GenerateServerError("Not initialized");
    }

    Server output_server;
    output_server.error = false;
    output_server.address.host = ENET_HOST_ANY;
    output_server.address.port = port;

    output_server.server = enet_host_create(&output_server.address, player_limit, 1, 0, 0);

    if(output_server.server == NULL) {
        return GenerateServerError("Unable to create server");
    }

    return output_server;
}

void UpdateServer(Server server, bool debug = false, int sleep = 200) {
    while(enet_host_service(server.server, &server.event, sleep) > 0) {
        switch(server.event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if(debug) printf("[PENK.NETWORK] New connection from %x:%u\n", 
                    server.event.peer -> address.host,
                    server.event.peer -> address.port);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if(debug) {
                    printf("[PENK.NETWORK] A packet of length %u (%s) was received from %x:%u on channel %u:\n",
                        server.event.packet -> dataLength,
                        server.event.packet -> data,
                        server.event.peer -> address.host,
                        server.event.peer -> address.port,
                        server.event.channelID);
                }
                break;    

            case ENET_EVENT_TYPE_DISCONNECT:
                if(debug) printf("[PENK.NETWORK] %x:%u disconnected\n",
                    server.event.peer -> address.host,
                    server.event.peer -> address.port);
            break;
        }
    }
}

void DestroyServer(Server server) {
    enet_host_destroy(server.server);
}

#endif // PENK_NETWORK_CPP
#ifndef PENK_NETWORK_CPP

#define PENK_NETWORK_CPP

#include "penk.cpp"
#include "penkGraphics.cpp"
#include "RakNetTypes.h"
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "MessageIdentifiers.h"

struct ServerData {
    char* password;
    int timeoutTime;
    int maxConnections;
};

struct Server {
    RakNet::RakPeerInterface *server;
    RakNet::RakNetStatistics *stats;
    RakNet::Packet *packet;
    
    ServerData serverData;
    unsigned char packetIdentifier;
    RakNet::SystemAddress clientID = RakNet::UNASSIGNED_SYSTEM_ADDRESS;

    RakNet::SocketDescriptor socketDescriptor[1];

    DataStructures::List<RakNet::RakNetSocket2*> sockets;

    bool fail;

    bool Init(int port) {
        server = RakNet::RakPeerInterface::GetInstance();
        server->SetIncomingPassword(serverData.password, (int)strlen(serverData.password));
        server->SetTimeoutTime(serverData.timeoutTime, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

        socketDescriptor[0].port = port;
        socketDescriptor[0].socketFamily = AF_INET;

        fail = server->Startup(serverData.maxConnections, socketDescriptor, 1) != RakNet::RAKNET_STARTED;
        server->SetMaximumIncomingConnections(serverData.maxConnections);

        if(fail) {
            return false;
        }

        server->SetOccasionalPing(true);
        server->SetUnreliableTimeout(1000);

        server->GetSockets(sockets);
        return true;
    }

    unsigned char _GetPacketIdentifier(RakNet::Packet *packet) {
        if (packet == 0) return 255;

        if ((unsigned char)packet->data[0] == ID_TIMESTAMP) {
            RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
            return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
        }
        else return (unsigned char)packet->data[0];
    }


    void Broadcast(char message[]) {
        server->Send(message, (const int)strlen(message) + 1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
    }

    const char* Receive() {
        for (packet = server->Receive(); packet; server->DeallocatePacket(packet), packet = server->Receive())
        {
            packetIdentifier = _GetPacketIdentifier(packet);

            switch (packetIdentifier)
            {
                case ID_DISCONNECTION_NOTIFICATION:
                    return TextFormat("%s disconnected.", packet->systemAddress.ToString(true));
                    break;


                case ID_NEW_INCOMING_CONNECTION:
                    clientID = packet->systemAddress;
                    return TextFormat("New incoming connection from %s", packet->systemAddress.ToString(true));
                    break;

                case ID_INCOMPATIBLE_PROTOCOL_VERSION:
                    return "Incompatible protocol version packet received";
                    break;

                case ID_CONNECTED_PING:
                case ID_UNCONNECTED_PING:
                    return TextFormat("Ping from %s\n", packet->systemAddress.ToString(true));
                    break;

                case ID_CONNECTION_LOST:
                    return TextFormat("Connection lost (from %s)", packet->systemAddress.ToString(true));
                    break;

                default:
                    server->Send((const char*)packet->data, (const int)strlen((const char*)packet->data)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);

                    return "TextFormat('.%s', (const char*)packet->data)";
                    break;
            }
        }
        return "NULL";
    }

    void Destroy() {
        server->Shutdown(300);
        RakNet::RakPeerInterface::DestroyInstance(server);
    }
};

#endif // PENK_NETWORK_CPP
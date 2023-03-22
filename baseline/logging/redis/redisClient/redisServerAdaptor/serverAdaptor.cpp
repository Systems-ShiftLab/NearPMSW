#include <iostream>
#include <map>
using namespace std;
#include "../redisAdaptorCommon/common.h"
#include "../socketHandler/socketHandler.h"

#define CIRCULAR_BUFFER_SIZE 16



struct requestBufferEntry{
    uint32_t seq_no;
    size_t request_size;
    char request[5000];
};

void parsErr(){
    cerr << "Parsing error, exiting" << endl;
}



int main(int argc, char* argv[]){
    int pmSwitchPort = PMSWITCH_PORT;
    char* useErrorMsg = "Use: ./redisServerAdaptor PMSwitch_port\n";
    if(argc>1){
        if(argv[1][0]>'9'||argv[1][0]<'0'){
            cerr << useErrorMsg << endl;
            exit(1);
        }
        pmSwitchPort = atoi(argv[1]);
    }

    std::map<uint32_t, struct requestBufferEntry> reorderBuffer;
    // UDP socket for PMSwitch downstream;
    // Temp implementation, do not use this, it's shitty.
    int PMSwitchupStreamUDPSock = socketHandler_listen(pmSwitchPort, DATAGRAM, BLOCKING);

    int serverSock_fd = socketHandler_connect("127.0.0.1", 6379, STREAM, BLOCKING);
    if(serverSock_fd==NULL){
        std::cerr << "Cannot connect to the server." << endl;
    }
    int ret=0;
    char pmSwitchBuff[5000];
    char toServerBuff[5000];
    int seqNumber = 0;
    while(1){
        struct sockaddr_in addressStruct;
        size_t addr_struct_Size = sizeof(addressStruct);
        size_t recvSize = 0;
        size_t sendSize = 0;
        ret = socketHandler_recv_bytes_from(PMSwitchupStreamUDPSock, pmSwitchBuff, sizeof(pmSwitchBuff), &addressStruct, &addr_struct_Size);
        if(ret==0){
            cerr << "Socket closed, exiting";
            exit(0);
        }
        recvSize = ret;
        int port = ntohs(addressStruct.sin_port);
        char src_ip[40];
        inet_ntop(AF_INET, (void*)&addressStruct.sin_addr.s_addr, src_ip, sizeof(src_ip));
        // cerr << "recved from client" << endl;
        struct pmswitchHeader pmswitch_hds;
        parseHeader(pmSwitchBuff, &pmswitch_hds, recvSize);
        int requestType = pmswitch_hds.type;
	// Just serve the request for now.
        if(0&&pmswitch_hds.seq_no > seqNumber){
            // There is a gap in sequence number. Either the packet arrives out of order or the packet is missing.
            // The server adaptor needs to request the recovery from the client or the switch.

            // To be implemented

            assert(0);

            continue;
        }else{
            while(1){
                // Process current request.
                // This will do for now.
                size_t requestSize = recvSize;
                ////////////////////////
                size_t payload_size = stripHeader(toServerBuff, pmSwitchBuff, requestSize);
                socketHandler_send_bytes(serverSock_fd, toServerBuff, payload_size);
                // cerr << "sent to server" << endl;

                size_t server_response_size = socketHandler_recv_bytes(serverSock_fd, toServerBuff, sizeof(toServerBuff));
                // cerr << "recved from server" << endl;
                // cerr << requestType << endl;
                
                // The server always responses with PMSWITCH_OPCODE_REPONSE.

                // if(requestType==PMSWITCH_OPCODE_PERSIST_NEED_ACK){
                //     // Just send ACK
                //     sendSize = pmSwitchEncapsulate(pmSwitchBuff, PMSWITCH_OPCODE_ACK, pmswitch_hds.session_id, pmswitch_hds.seq_no, NULL, 0);
                // }else{
                //     // requestType is PMSWITCH_OPCODE_PERSIST_NO_ACK
                //     // Need to encapsulate the response.
                //     sendSize = pmSwitchEncapsulate(pmSwitchBuff, PMSWITCH_OPCODE_REPONSE, pmswitch_hds.session_id, pmswitch_hds.seq_no, toServerBuff, server_response_size);
                // }
                
                sendSize = pmSwitchEncapsulate(pmSwitchBuff, PMSWITCH_OPCODE_REPONSE, pmswitch_hds.session_id, pmswitch_hds.seq_no, toServerBuff, server_response_size);
                socketHandler_send_bytes_to(PMSwitchupStreamUDPSock, pmSwitchBuff, sendSize, (const char*)src_ip, port);
                // cerr << "sent to client" << endl;

                seqNumber++;


                // Process requests in the reorder buffer.

                // To be implemented
                if(1){
                    break;
                }
                // DO NOT forget to populate pmswitch_hds with valid header.
            }
        }





        // socketHandler_send_bytes(serverSock_fd, pmSwitchBuff, ret);
        // cerr << "sent to server" << endl;
        // ret = socketHandler_recv_bytes(serverSock_fd, pmSwitchBuff, sizeof(pmSwitchBuff));
        // cerr << "recved from server" << endl;
        // socketHandler_send_bytes_to(PMSwitchupStreamUDPSock, pmSwitchBuff, ret, (const char*)src_ip, port);
        // cerr << "sent to client" << endl;
        
    }


}

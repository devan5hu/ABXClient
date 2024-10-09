/* Author : devanshuC @CodeForces */

#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

// Constants
#define ZERO 0x0
#define ONE 0x1
#define TWO 0x2

// Setup
#pragma comment(lib, "Ws2_32.lib")

// Struct to hold packet information
struct Packet {
    std::string symbol;
    char indicatorBuySell;
    int quantity;
    int price;
    int packetSequence;

    std::string toJson() const {
        std::ostringstream oss;
        oss << "\t{"
            << "\"symbol\": \"" << symbol << "\", "
            << "\"buySell\": \"" << indicatorBuySell << "\", "
            << "\"quantity\": " << quantity << ", "
            << "\"price\": " << price << ", "
            << "\"packetSequence\": " << packetSequence
            << "}";
        return oss.str();
    }
};

int handleSendError(SOCKET &ConnectSocket, addrinfo *result) {
    printf("Error sending request\n");
    closesocket(ConnectSocket);
    freeaddrinfo(result);
    return 1;
}

int sendData(SOCKET &ConnectSocket, const char *data, int length, addrinfo *result) {
    int bytes_sent = send(ConnectSocket, data, length, 0);
    
    if (bytes_sent == -1) {
        return handleSendError(ConnectSocket, result); 
    }
    
    return bytes_sent;
}

bool isValidPacket(const Packet &packet) {
    return !packet.symbol.empty() && 
           (packet.indicatorBuySell == 'B' || packet.indicatorBuySell == 'S') && 
           (packet.quantity > 0) && 
           (packet.price >= 0) && 
           (packet.packetSequence > 0);
}

void writeJsonToFile(std::vector<Packet> &data) {
    std::ofstream outFile("packets.json"); // Open or create the JSON file
    int dataSize = data.size(); 
    if(dataSize == 0){
        outFile.close();
        return;
    } 
    if (!outFile) {
        printf("Error creating JSON file!\n");
        return;
    }
    outFile << "[\n"; 
    for(int i=0; i<dataSize; i++){
        if(i == dataSize-1) outFile << data[i].toJson() << '\n';
        else outFile << data[i].toJson() << ',' << '\n';
    }
    outFile << "]"; 
    outFile.close();
    printf("Data written to packets.json\n");
}

void receiveData(SOCKET sockfd, std::queue<int> &missingPackets, int requestType, std::vector<Packet> &data) {
    std::vector<char> buffer(1024);
    int bytesReceived;
    int packetSize = 17; // 4 + 1 + 4 + 4 + 4
    int sequence = 1;

    while ((bytesReceived = recv(sockfd, buffer.data(), 1024, 0)) > 0) {
        int totalReceived = 0;
        while (totalReceived < bytesReceived) {
            if (bytesReceived - totalReceived < packetSize) {
                break;
            }

            Packet packet;
            packet.symbol = std::string(buffer.data() + totalReceived, 4);  // symbol
            packet.indicatorBuySell = buffer[totalReceived + 4];    // buysell 
            packet.quantity = ntohl(*(int*)(buffer.data() + totalReceived + 5)); // quantity
            packet.price = ntohl(*(int*)(buffer.data() + totalReceived + 9));  // price
            packet.packetSequence = ntohl(*(int*)(buffer.data() + totalReceived + 13)); // packetSequence
            
            totalReceived += packetSize;

            if(isValidPacket(packet)){
                data.push_back(packet);
            } else {
                printf("Invalid packet received");
            }

            if (requestType == 1) {
                int totalPacketsMissed = packet.packetSequence - sequence;
                if (totalPacketsMissed > 0) {
                    for (int iter = 0; iter < totalPacketsMissed; iter++) {
                        missingPackets.push(sequence + iter);
                    }
                }
                sequence = packet.packetSequence + 1;
            }
        }
        if(requestType == 2){
            break;
        }
    }
    
}

bool isSocketValid(SOCKET socket) {
    if (socket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        return 0;
    }
    return 1;
}

bool validateConnection(int iResult, SOCKET &connectSocket){
    if (iResult == SOCKET_ERROR) {
        printf("Unable to connect to server: %ld\n", WSAGetLastError());
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
        WSACleanup();
        return true; // Return true on error
    }
    printf("Connection established successfully.\n");
    return false; // Return false on success
}

void sortData(std::vector <Packet> &data){
    sort(data.begin(), data.end(), [](const Packet& a, const Packet& b){
        return a.packetSequence < b.packetSequence;
    });
}

int main() {
    WSADATA wsaData;
    int iResult;
    std::string api_endpoint = "/";
    std::string hostname = "127.0.0.1";
    std::string port = "3000";
    
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct addrinfo* result = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;            // IPv4
    hints.ai_socktype = SOCK_STREAM;      // TCP

    iResult = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (!isSocketValid(ConnectSocket)) {
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if(validateConnection(iResult, ConnectSocket)){
        return 1;
    }

    printf("Connected to server.\n");
    
    std::queue<int> missingPackages; 
    std::vector<Packet> data;
    
    char requestType1[2] = { ONE, ZERO };
    int bytes_sent = send(ConnectSocket, requestType1, 2, 0);
    printf("Request sent to server.\n");

    receiveData(ConnectSocket, missingPackages, 1, data);

    if (bytes_sent == -1) {
        return handleSendError(ConnectSocket, result); 
    }

    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (!isSocketValid(ConnectSocket)) {
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if(validateConnection(iResult, ConnectSocket)){
        return 1;
    }

    while (!missingPackages.empty()) {
        char reqPacket = missingPackages.front();
        // printf("Requested Packet: %d\n", missingPackages.front());
        missingPackages.pop();
        char requestType2[2] = { TWO, reqPacket };
        int bytes_sent2; 
        try {
            bytes_sent2 = send(ConnectSocket, requestType2, 2, 0);
            // printf("Request for missing packet sent: %d\n", reqPacket);

            if (bytes_sent == -1) {
                return handleSendError(ConnectSocket, result);  
            }
        } catch (std::runtime_error& e) {
            printf("Error sending request for missing packet\n");
        }

        if (ConnectSocket != INVALID_SOCKET) {
            if (bytes_sent == -1) {
                return handleSendError(ConnectSocket, result);  
            }
            try {
                receiveData(ConnectSocket, missingPackages, 2, data);
            } catch (std::runtime_error& e) {
                printf("Error receiving missing packet data\n");
            }
        } else {
            printf("Invalid Socket Connection\n");
        }
    }

    printf("Data received successfully. Total packets received: %zu\n", data.size());

    sortData(data);
    
    // Writing the JSON
    writeJsonToFile(data);
    
    freeaddrinfo(result);
    closesocket(ConnectSocket);
    printf("Server Disconnected.\n");

    // WSACleanup();
    return 0;
}

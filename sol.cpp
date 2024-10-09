/* Author: devanshuC @Codeforces */

// Imports 
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <queue>

//constants
#define ZERO 0x0
#define ONE 0x1
#define TWO 0x2

// Setup
#pragma comment(lib, "Ws2_32.lib")

void receiveData(SOCKET sockfd , std::queue < int > &missingPackets , int requestType) {
    std::queue < int > q;
    std::vector<char> buffer(INT_MAX);
    int bytesReceived;
    int packetSize = 17; // 4 + 1 + 4 + 4 + 4
    int sequence = 1;
    while ((bytesReceived = recv(sockfd, buffer.data(), INT_MAX - 1, 0)) > 0) {
        int totalReceived = 0;

        while (totalReceived < bytesReceived) {
            if (bytesReceived - totalReceived < packetSize) {
                break;
            }
            std::string symbol(buffer.data() + totalReceived, 4);  // symbol
            char indicatorBuySell = buffer[totalReceived + 4];    // buysell 
            int quantity =  ntohl(*(int*)(buffer.data() + totalReceived + 5)); // quantity
            int price = ntohl(*(int*)(buffer.data() + totalReceived + 9));  // price
            int packetSequence = ntohl(*(int*)(buffer.data() + totalReceived + 13)); // packetSequence
            
            // Total Packets Missed Logic            
            int totalPacketsMissed = packetSequence - sequence;
            if(totalPacketsMissed > 0){
                for(int iter = 1; iter <= totalPacketsMissed; iter++){
                    missingPackets.push(sequence - 1 + iter);
                }
            }
            sequence = packetSequence;
            sequence++;
            
            printf("Symbol: %.*s ", 4, symbol.c_str());
            printf("Buy/Sell: %c " , indicatorBuySell);
            printf("Quantity: %d " , quantity);
            printf("Price: %d " , price);
            printf("Packet Sequence: %d \n", packetSequence);

            totalReceived += packetSize;
        }
    }
    if (bytesReceived < 0) {
        throw std::runtime_error("Error receiving data from socket.");
    }
}

int main() {
    WSADATA wsaData;
    int iResult;
    std::string api_endpoint = "/";
    std::string hostname = "127.0.0.1";
    std::string port = "3000";

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct addrinfo *result = NULL, hints;

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
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("Unable to connect to server: %ld\n", WSAGetLastError());
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
        WSACleanup();
        return 1;
    }

    printf("Connected to server. :) \n");
    char requestType1[2] = {ONE , ZERO};
    int bytes_sent = send(ConnectSocket, requestType1 , 2 , 0);
    std::queue < int > missingPackages; 
    receiveData(ConnectSocket , missingPackages, 2);
    if (bytes_sent == -1) {
        std::cerr << "Error sending request" << std::endl;
        closesocket(ConnectSocket);
        freeaddrinfo(result);
        return 1;
    }
    int i = 1;
    int missed = -1;
    while(!missingPackages.empty()){
        printf("Here is the %dth missing package %d \n" , i , missingPackages.front());
        if(missed == -1){
            missed = missingPackages.front(); 
        }
        missingPackages.pop();
        i++;
    }

    char requestType2[2] = {TWO , 0x5};
    int bytes_sent2; 
    try{
        printf("SENT 22222 \n");
        bytes_sent2 = send(ConnectSocket, requestType2 , 2 , 0);
        printf("SENT 22222 \n");
    }
    catch(std::runtime_error& e){
        std::cerr << e.what() << '\n';
    }
    if(ConnectSocket != INVALID_SOCKET){
        if (bytes_sent == -1) {
            std::cerr << "Error sending request" << std::endl;
            closesocket(ConnectSocket);
            freeaddrinfo(result);
            return 1;
        }
        else{
            try{
                receiveData(ConnectSocket , missingPackages, 2);
            }
            catch(std::runtime_error& e){
                std::cerr << e.what() << "\n";
            }
        }
    }
    else{
        std::cerr << "INVALID SOCKET CONNECTION " << "\n";
    }

    freeaddrinfo(result);
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

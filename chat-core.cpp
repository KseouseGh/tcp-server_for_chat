#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <Winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#define SERVICE_PORT 1500
//new
#define MAX_CLIENTS 5
#define MAX_BUFFER_SIZE 1024
#include <string>
#define MAX_USERNAME_LEN 100
#define MAX_MESSAGE_LEN 1000

using namespace std;
#include <vector>

struct ClientInf {
	char ip[16]; // IP-����� �������
	SOCKET s;
};
ClientInf clients[MAX_CLIENTS];
struct Message {
    char sender[MAX_USERNAME_LEN];
    char text[MAX_MESSAGE_LEN];
    time_t time;
};
vector<Message> messages;
int lastMessageIndex = -1;
int send_str(SOCKET s, const char* sString) {
    int len = strlen(sString);
    return send(s, sString, len + 1, 0);
}
void broadcastMessage(const char* message) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strlen(clients[i].ip) > 0 && clients[i].s != INVALID_SOCKET) {
            send(clients[i].s, message, strlen(message) + 1, 0);
        }
    }
}
void receiveMessage(SOCKET s) {
    char buffer[MAX_BUFFER_SIZE];

    while (true) {
        int bytesRecv = recv(s, buffer, MAX_BUFFER_SIZE, 0);

        if (bytesRecv == SOCKET_ERROR) {
            fprintf(stderr, "Error receiving message\n");
            break;
        }
        else if (bytesRecv == 0) {
            // ���������� ������� ��������
            printf("Server disconnected\n");
            break;
        }
        else {
            // ��������� ��� ������������ � ����� ���������
            string username, messageText;
            size_t pos = string(buffer).find(":");
            if (pos != string::npos) {
                username = string(buffer).substr(0, pos);
                messageText = string(buffer).substr(pos + 1);
            }

            // ��������� ��������� � ������
            Message newMessage;
            strcpy(newMessage.sender, username.c_str());
            strcpy(newMessage.text, messageText.c_str());
            newMessage.time = time(NULL);
            messages.push_back(newMessage);

            // ���������� ��������� ���� ��������
            string prefixedMessage = username + ":" + messageText;
            broadcastMessage(prefixedMessage.c_str());
        }
    }
}

int main() {
    SOCKET S; // ���������� ��������������� ������
    SOCKET NS; // ���������� ��������������� ������
    sockaddr_in serv_addr;
    WSADATA wsadata;
    char sName[128];
    bool bTerminate = false;
    // �������������� ���������� �������
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    // ��������� ��������� ������ �������
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVICE_PORT);
    // ������� �����
    S = socket(AF_INET, SOCK_STREAM, 0);
    if (S == INVALID_SOCKET) {
        fprintf(stderr, "Can't create socket\n");
        exit(1);
    }
    // ����������� ����� � ������
    if (bind(S, (sockaddr*)&serv_addr, sizeof(serv_addr)) == INVALID_SOCKET) {
        fprintf(stderr, "Can't bind\n");
        exit(1);
    }
    // ��������� ����� � ����� �������������
    if (listen(S, MAX_CLIENTS) != INVALID_SOCKET) {
        printf("Server listen on %s:%d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        // �������� ���� ��������� ����������� ��������
        while (!bTerminate) {
            printf("Wait for connections.....\n");
            sockaddr_in clnt_addr;
            int addrlen = sizeof(clnt_addr);
            memset(&clnt_addr, 0, sizeof(clnt_addr));
            // ��������� ������ � ����� �������� ������� �� ����������.
            // ����� ����������, �.�. ���������� ���������� ������ ���
            // ����������� ������� ��� ������
            NS = accept(S, (sockaddr*)&clnt_addr, &addrlen);
            if (NS == INVALID_SOCKET) {
                fprintf(stderr, "Can't accept connection\n");
                break;
            }
            // ���������, ���� �� ����� ��� ������ �������
            bool clientAdded = false;

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (strlen(clients[i].ip) == 0) {
                    strcpy(clients[i].ip, inet_ntoa(clnt_addr.sin_addr));
                    clientAdded = true;
                    clients[i].s = NS;
                    break;
                }
            }
            if (!clientAdded) {
                send_str(NS, "There is not enough place for new IP-adress.\r\n");
                closesocket(NS);
                continue;
            }
            // ������ ������ ��� ��������� ��������� �� �������
            thread clientThread(receiveMessage, NS);
            clientThread.detach();
            printf("Client connected: %s\n", inet_ntoa(clnt_addr.sin_addr));
        }
        // ��������� �����
        closesocket(S);
        // ��������� ������ ���������� �������
        WSACleanup();
        return 0;
    }
}
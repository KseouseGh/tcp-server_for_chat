/*
    Простой TCP-сервер
    Поучает текстовую строку, преобразует ее в верхний регистр и
    возвращает клиенту результат.

    Файл: server.cpp

    Пример компиляции:
    bcc32 -oserver.exe server.cpp Ws2_32.lib
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <Winsock2.h>
#include <stdio.h>
#include <stdlib.h>
// TCP-порт сервера
#define SERVICE_PORT 1500
//new
#define MAX_CLIENTS 5
#define MAX_ATTEMPTS 3
#define BLOCK_TIME 300000 // 5 минут в миллисекундах
#include <fstream>
#include <string>
struct ClientInfo {
    char ip[16]; // IP-адрес клиента
    int attempts; // количество попыток ввода пароля
    long block_time; // время блокировки
};
ClientInfo clients[MAX_CLIENTS];
std::ifstream passwordFile("password.txt");
std::string correctPassword;
//new

int send_string(SOCKET s, const char* sString);
//
int main10(void)
{
    SOCKET  S;  //дескриптор прослушивающего сокета
    SOCKET  NS; //дескриптор присоединенного сокета

    sockaddr_in serv_addr;
    WSADATA     wsadata;
    char        sName[128];
    bool        bTerminate = false;

    // Инициализируем библиотеку сокетов
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    // Пытаемся получить имя текущей машины
    gethostname(sName, sizeof(sName));
    printf("\nServer host: %s\n", sName);

    // Создаем сокет
    // Для TCP-сокета указываем параметр SOCK_STREAM
    // Для UDP - SOCK_DGRAM 
    if ((S = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't create socket\n");
        exit(1);
    }
    // Заполняем структуру адресов 
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // Разрешаем работу на всех доступных сетевых интерфейсах,
    // в частности на localhost
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    // обратите внимание на преобразование порядка байт
    serv_addr.sin_port = htons((u_short)SERVICE_PORT);

    // Связываем сокет с заданным сетевым интерфесом и портом
    if (bind(S, (sockaddr*)&serv_addr, sizeof(serv_addr)) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't bind\n");
        exit(1);
    }

    // Переводим сокет в режим прослушивания заданного порта
    // с максимальным количеством ожидания запросов на соединение 5
    if (listen(S, 5) == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't listen\n");
        exit(1);
    }

    printf("Server listen on %s:%d\n",
        inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

    // Основной цикл обработки подключения клиентов 
    while (!bTerminate)
    {
        printf("Wait for connections.....\n");

        sockaddr_in clnt_addr;
        int addrlen = sizeof(clnt_addr);
        memset(&clnt_addr, 0, sizeof(clnt_addr));

        // Переводим сервис в режим ожидания запроса на соединение.
        // Вызов синхронный, т.е. возвращает управление только при 
        // подключении клиента или ошибке 
        NS = accept(S, (sockaddr*)&clnt_addr, &addrlen);
        if (NS == INVALID_SOCKET)
        {
            fprintf(stderr, "Can't accept connection\n");
            break;
        }
        //new2
        if (passwordFile.is_open()) {
            getline(passwordFile, correctPassword);
            passwordFile.close();
        }
        int i = -1;
        bool blocked = false;
        for (int j = 0; j < MAX_CLIENTS; ++j) {
            if (strcmp(clients[j].ip, inet_ntoa(clnt_addr.sin_addr)) == 0) {
                i = j;
                break;
            }
        }
        bool clientAdded = false;// Если клиент не найден в списке, идёт его добавление, или отключение в случае если нет места для ещё одного адреса
        if (i == -1) {
            for (int j = 0; j < MAX_CLIENTS; ++j) {
                if (strlen(clients[j].ip) == 0) {
                    strcpy(clients[j].ip, inet_ntoa(clnt_addr.sin_addr));
                    clientAdded = true;
                    i = j;
                    break;
                }
            }
        }
        if (!clientAdded) {
            send_string(NS, "There is not enough place for new IP-adress.\r\n");
            closesocket(NS);
            continue;
        }
        else {
            while (true) {
                send_string(NS, "Enter password: ");
                char entered_password[100];
                int nReaded = recv(NS, entered_password, sizeof(entered_password) - 1, 0);
                if (nReaded <= 0) {
                    closesocket(NS);
                    continue; // Обработка ошибки при получении пароля
                }
                if(strcmp(entered_password, "\r\n") == 0){
                    send_string(NS, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                    continue;
                }
                entered_password[nReaded] = '\0';
                if (strcmp(entered_password, correctPassword.c_str()) != 0) {
                    clients[i].attempts++;
                    if (clients[i].attempts >= MAX_ATTEMPTS) {
                        clients[i].block_time = GetTickCount() + BLOCK_TIME;
                        clients[i].attempts = 0;
                        blocked = true;
                    }
                    else {
                        send_string(NS, "Invalid password.\r\n");
                    }
                }
                else {
                    break;
                }
                if (blocked) {
                    send_string(NS, "You are blocked. Try again later.\r\n");
                    closesocket(NS);
                    continue;
                }
            }
        }
        //new2
        // Получаем параметры присоединенного сокета NS и
        // информацию о клиенте
        addrlen = sizeof(serv_addr);
        getsockname(NS, (sockaddr*)&serv_addr, &addrlen);
        // Функция inet_ntoa возвращает указатель на глобальный буффер, 
        // поэтому использовать ее в одном вызове printf не получится
        printf("Accepted connection on %s:%d ",
            inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        printf("from client %s:%d\n",
            inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

            // Отсылаем вводную информацию о сервере
            send_string(NS, "* * * Welcome to simple UPCASE TCP-server * * *\r\n");
        //
        char    sReceiveBuffer[1024] = { 0 };
        // Получаем и обрабатываем данные от клиента
        while (true)
        {
            int nReaded = recv(NS, sReceiveBuffer, sizeof(sReceiveBuffer) - 1, 0);
            // В случае ошибки (например, отсоединения клиента) выходим
            if (nReaded <= 0) break;
            // Мы получаем поток байт, поэтому нужно самостоятельно 
            // добавить завержающий 0 для ASCII строки 
            sReceiveBuffer[nReaded] = 0;
            // Отбрасываем символы превода строк
            for (char* pPtr = sReceiveBuffer; *pPtr != 0; pPtr++)
            {
                if (*pPtr == '\n' || *pPtr == '\r')
                {
                    *pPtr = 0;
                    break;
                }
            }
            // Пропускаем пустые строки
            if (sReceiveBuffer[0] == 0) continue;

            printf("Received data: %s\n", sReceiveBuffer);

            // Анализируем полученные команды или преобразуем текст в верхний регистр
            if (strcmp(sReceiveBuffer, "info") == 0)
            {
                send_string(NS, "Test TCP-server.\r\n");
            }
            else if (strcmp(sReceiveBuffer, "exit") == 0)
            {
                send_string(NS, "Bye...\r\n");
                printf("Client initialize disconnection.\r\n");
                break;
            }
            else if (strcmp(sReceiveBuffer, "shutdown") == 0)
            {
                send_string(NS, "Server go to shutdown.\r\n");
                Sleep(200);
                bTerminate = true;
                break;
            }
            else
            {
                // Преобразовываем строку в верхний регистр
                char sSendBuffer[1024];
                _snprintf(sSendBuffer, sizeof(sSendBuffer), "Server reply: %s\r\n",
                    _strupr(sReceiveBuffer));
                send_string(NS, sSendBuffer);
            }
        }
        // закрываем присоединенный сокет
        closesocket(NS);
        printf("Client disconnected.\n");
    }
    // Закрываем серверный сокет
    closesocket(S);
    // освобождаем ресурсы библиотеки сокетов
    WSACleanup();
    return 0;
}
// Функция отсылки текстовой ascii строки клиенту
int send_string(SOCKET s, const char* sString)
{
    return send(s, sString, strlen(sString), 0);
}
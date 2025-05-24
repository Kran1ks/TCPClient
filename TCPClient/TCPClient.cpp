#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include <regex>

#pragma comment(lib, "ws2_32.lib")

bool isValidMove(const std::string& move) {
    std::regex movePattern(R"(^\s*([0-2])\s+([0-2])\s*$)");
    return std::regex_match(move, movePattern);
}

int main() {
    setlocale(LC_ALL, "Russian");
    std::cout << "TCP-клиент запущен\n";

    WSAData wsad;
    if (WSAStartup(MAKEWORD(2, 2), &wsad) != 0) {
        std::cerr << "Ошибка инициализации Winsock: " << WSAGetLastError() << std::endl;
        return 1;
    }

    SOCKET clientsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientsock == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    int flag = 1;
    if (setsockopt(clientsock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) == SOCKET_ERROR) {
        std::cerr << "Ошибка настройки TCP_NODELAY: " << WSAGetLastError() << std::endl;
        closesocket(clientsock);
        WSACleanup();
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(306);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        std::cerr << "Неверный адрес сервера\n";
        closesocket(clientsock);
        WSACleanup();
        return 2;
    }

    if (connect(clientsock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка подключения к серверу: " << WSAGetLastError() << std::endl;
        closesocket(clientsock);
        WSACleanup();
        return 2;
    }

    std::cout << "Подключение к серверу успешно\n";

    char buffer[512];
    std::string accumulatedMessage;
    bool gameEnded = false;

    while (true) {
        int bytes_read = recv(clientsock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            if (!accumulatedMessage.empty()) {
                std::cout << accumulatedMessage;
                if (accumulatedMessage.find("Игра окончена") != std::string::npos) {
                    gameEnded = true;
                }
                accumulatedMessage.clear();
            }
            std::cout << "Соединение с сервером разорвано\n";
            break;
        }
        buffer[bytes_read] = '\0';
        accumulatedMessage += buffer;

        // Выводим накопленное сообщение
        std::cout << accumulatedMessage;
        if (accumulatedMessage.find("Игра окончена") != std::string::npos) {
            gameEnded = true;
        }

        if (accumulatedMessage.find("ваш ход") != std::string::npos && !gameEnded) {
            std::string move;
            bool validMove = false;
            do {
                std::cout << "Введите ваш ход (ряд столбец, например, 1 1): ";
                std::getline(std::cin, move);
                if (isValidMove(move)) {
                    validMove = true;
                }
                else {
                    std::cout << "Неверный формат хода! Используйте два числа от 0 до 2 (например, '1 1').\n";
                }
            } while (!validMove);

            move += "\n";
            if (send(clientsock, move.c_str(), move.size(), 0) == SOCKET_ERROR) {
                std::cerr << "Ошибка отправки хода: " << WSAGetLastError() << std::endl;
                break;
            }
        }
        accumulatedMessage.clear();
    }

    // Дополнительная попытка получить остатки данных
    if (!gameEnded) {
        int bytes_read = recv(clientsock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::cout << buffer;
        }
    }

    shutdown(clientsock, SD_BOTH);
    closesocket(clientsock);
    WSACleanup();
    std::cout << "Клиент отключен\n";

    return 0;
}
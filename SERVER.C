#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOGDI
#define NOUSER
#define WIN32_LEAN_AND_MEAN

#include "raylib.h"

#include <winsock2.h>
#include <windows.h>

extern int __stdcall GetAsyncKeyState(int vKey);

#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_RETURN 0x0D

typedef struct {
    float bolaX;
    float bolaY;
    float bolaVelocidadeX;
    float bolaVelocidadeY;
    float player1Y; 
    float player2Y;
    int placar1;
    int placar2;
} DadosCompartilhados;

typedef struct {
    char tipo[20];          
    int placar1;
    int placar2;
    char mensagem[100];
} Mensagem;

void error_handling(char *message) {
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    WSACleanup();
    exit(1);
}

void enviarMensagem(SOCKET sock, Mensagem *msg) {
    int enviados = send(sock, (char*)msg, sizeof(Mensagem), 0);
    if (enviados == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        fprintf(stderr, "Erro ao enviar mensagem\n");
    }
}

int receberMensagem(SOCKET sock, Mensagem *msg) {
    int recebidos = recv(sock, (char*)msg, sizeof(Mensagem), 0);
    if (recebidos == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            return 0;
        }
        return -1;
    }
    return recebidos;
}

int main() {
    const int larguraTela = 640;
    const int alturaTela = 720;

    printf("=== SERVIDOR PONG - Player 1 (Esquerda) ===\n");
    printf("Controle: W/S | ENTER para iniciar\n");
    
    WSADATA winsocketsDados;
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        error_handling("Falha ao inicializar o Winsock");
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        error_handling("Erro ao criar o socket");
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(51171);
    
    if (bind(serverSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        error_handling("Erro ao associar o socket");
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        error_handling("Erro ao colocar o socket em estado de escuta");
    }
    printf("Aguardando Player 2 conectar...\n\n");

    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    
    if (clientSocket == INVALID_SOCKET) {
        error_handling("Erro ao aceitar a conexão");
    }
    printf("Player 2 conectado! Iniciando jogo...\n\n");

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
    
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    float playerX = 10;
    float playerY = alturaTela / 2 - 50;
    float playerLargura = 25;
    float playerAltura = 100;
    float playerVelocidade = 10;

    HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DadosCompartilhados), "Local\\PongShared");
    if (!hMap) {
        error_handling("CreateFileMapping falhou");
    }
    DadosCompartilhados *shared = (DadosCompartilhados*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!shared) {
        CloseHandle(hMap);
        error_handling("MapViewOfFile falhou");
    }

    HANDLE hMutex = CreateMutexA(NULL, FALSE, "Local\\PongSharedMutex");
    if (!hMutex) {
        UnmapViewOfFile(shared);
        CloseHandle(hMap);
        error_handling("CreateMutex falhou");
    }

    DadosCompartilhados init = {
        larguraTela, alturaTela / 2.0f,
        0.0f, 0.0f,
        playerY,
        alturaTela / 2 - 50,
        0, 0,
        0
    };
    WaitForSingleObject(hMutex, INFINITE);
    memcpy(shared, &init, sizeof(DadosCompartilhados));
    ReleaseMutex(hMutex);

    InitWindow(larguraTela, alturaTela, "PONG - Player 1 (Servidor) - Esquerda");
    SetTargetFPS(60);

    DadosCompartilhados dados;
    Mensagem msgEnvio, msgRecebida;
    bool jogoIniciado = false;

    while (!WindowShouldClose()) {
       
        if (GetAsyncKeyState('W') & 0x8000) {
            if (playerY > 0) {
                playerY -= playerVelocidade;
            }
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            if (playerY + playerAltura < alturaTela) {
                playerY += playerVelocidade;
            }
        }

        
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            WaitForSingleObject(hMutex, INFINITE);
            if (!jogoIniciado && shared->placar1 < 10 && shared->placar2 < 10) {
                jogoIniciado = true;
                shared->bolaVelocidadeX = 5.0f;
                shared->bolaVelocidadeY = 5.0f;
                printf("[SERVIDOR] Jogo iniciado!\n");
            } else if (shared->placar1 >= 10 || shared->placar2 >= 10) {
                
                shared->placar1 = 0;
                shared->placar2 = 0;
                shared->bolaX = larguraTela / 2.0f;
                shared->bolaY = alturaTela / 2.0f;
                shared->bolaVelocidadeX = 0;
                shared->bolaVelocidadeY = 0;
                jogoIniciado = false;

                strcpy_s(msgEnvio.tipo, sizeof(msgEnvio.tipo), "REINICIO");
                enviarMensagem(clientSocket, &msgEnvio);
                printf("[SERVIDOR] Jogo reiniciado!\n");
            }
            ReleaseMutex(hMutex);
            Sleep(200);
        }

       
        WaitForSingleObject(hMutex, INFINITE);
        shared->player1Y = playerY;
        ReleaseMutex(hMutex);

        
        if (jogoIniciado) {
            WaitForSingleObject(hMutex, INFINITE);

            
            shared->bolaX += shared->bolaVelocidadeX;
            shared->bolaY += shared->bolaVelocidadeY;

            
            if (shared->bolaY <= 0 || shared->bolaY >= alturaTela - 15) {
                shared->bolaVelocidadeY *= -1;
            }

            
            Rectangle player1Rect = {playerX, playerY, playerLargura, playerAltura};
            Rectangle bolaRect = {shared->bolaX, shared->bolaY, 15, 15};
            if (CheckCollisionRecs(player1Rect, bolaRect) && shared->bolaVelocidadeX < 0) {
                shared->bolaVelocidadeX *= -1.05f;
                shared->bolaX = playerX + playerLargura + 1;
            }

            
            float p2GlobalX = larguraTela + (larguraTela - 35);
            Rectangle player2Rect = {p2GlobalX, shared->player2Y, playerLargura, playerAltura};
            if (CheckCollisionRecs(player2Rect, bolaRect) && shared->bolaVelocidadeX > 0) {
                shared->bolaVelocidadeX *= -1.05f;
                shared->bolaX = player2Rect.x - 16;
            }

            
            if (shared->bolaX >= larguraTela * 2) {
                shared->placar1 += 1;
                shared->bolaX = larguraTela / 2.0f;
                shared->bolaY = alturaTela / 2.0f;
                shared->bolaVelocidadeX = 0;
                shared->bolaVelocidadeY = 0;
                jogoIniciado = false;

                printf("[SERVIDOR] Player 1 pontuou! Placar: %d x %d\n", shared->placar1, shared->placar2);

                
                strcpy_s(msgEnvio.tipo, sizeof(msgEnvio.tipo), "PONTO");
                msgEnvio.placar1 = shared->placar1;
                msgEnvio.placar2 = shared->placar2;
                strcpy_s(msgEnvio.mensagem, sizeof(msgEnvio.mensagem), "Player 1 pontuou!");
                enviarMensagem(clientSocket, &msgEnvio);
            }

            
            if (shared->bolaX <= 0) {
                shared->placar2 += 1;
                shared->bolaX = larguraTela / 2.0f;
                shared->bolaY = alturaTela / 2.0f;
                shared->bolaVelocidadeX = 0;
                shared->bolaVelocidadeY = 0;
                jogoIniciado = false;

                printf("[SERVIDOR] Player 2 pontuou! Placar: %d x %d\n", shared->placar1, shared->placar2);

                
                strcpy_s(msgEnvio.tipo, sizeof(msgEnvio.tipo), "PONTO");
                msgEnvio.placar1 = shared->placar1;
                msgEnvio.placar2 = shared->placar2;
                strcpy_s(msgEnvio.mensagem, sizeof(msgEnvio.mensagem), "Player 2 pontuou!");
                enviarMensagem(clientSocket, &msgEnvio);
            }

            ReleaseMutex(hMutex);
        }

        
        WaitForSingleObject(hMutex, INFINITE);
        memcpy(&dados, shared, sizeof(DadosCompartilhados));
        ReleaseMutex(hMutex);

        
        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangle(playerX, playerY, playerLargura, playerAltura, WHITE);

        if (dados.bolaX < larguraTela) {
            DrawRectangle(dados.bolaX, dados.bolaY, 15, 15, WHITE);
        }

        DrawText(TextFormat("P1: %d", dados.placar1), 10, 10, 20, WHITE);
        DrawText(TextFormat("P2: %d", dados.placar2), larguraTela - 80, 10, 20, WHITE);

        if (dados.placar1 >= 10 || dados.placar2 >= 10) {
            const char *msg = (dados.placar1 >= 10) ? "Player 1 Venceu!" : "Player 2 Venceu!";
            DrawText(msg, larguraTela/2 - MeasureText(msg, 30)/2, alturaTela/2 - 40, 30, WHITE);
            DrawText("Enter para reiniciar", larguraTela/2 - 100, alturaTela/2, 20, WHITE);
        }

        if (!jogoIniciado && dados.placar1 < 10 && dados.placar2 < 10) {
            DrawText("Pressione ENTER para iniciar", larguraTela/2 - 150, alturaTela/2, 20, WHITE);
        }

        EndDrawing();
    }

    UnmapViewOfFile(shared);
    CloseHandle(hMap);
    CloseHandle(hMutex);

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    CloseWindow();
    return 0;
}

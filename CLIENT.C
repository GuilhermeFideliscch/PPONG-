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
    float playerY;
    int ativo;
} Mensagem;

void error_handling(char *message) {
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    WSACleanup();
    exit(1);
}

int main() {
    const int larguraTela = 640;
    const int alturaTela = 720;

    printf("=== CLIENTE PONG - Player 2 (Direita) ===\n");
    printf("Controle: Seta Cima/Baixo\n");
    
    WSADATA winsocketsDados;
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        error_handling("Falha ao inicializar o Winsock");
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        error_handling("Erro ao criar o socket");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(51171);
    
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        error_handling("Erro ao conectar ao servidor");
    }
    printf("Conectado ao servidor! Iniciando jogo...\n\n");

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
    
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    float playerX = larguraTela - 35;
    float playerY = alturaTela / 2 - 50;
    float playerLargura = 25;
    float playerAltura = 100;
    float playerVelocidade = 10;

    InitWindow(larguraTela, alturaTela, "PONG - Player 2 (Cliente) - Direita");
    SetTargetFPS(60);

    DadosCompartilhados dados;

    HANDLE hMap = NULL;
    int tentativas = 0;
    while (tentativas < 200 && !hMap) {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Local\\PongShared");
        if (!hMap) {
            Sleep(50);
            tentativas++;
        }
    }
    if (!hMap) {
        fprintf(stderr, "Não foi possível abrir o memory-mapped file compartilhado\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    DadosCompartilhados *shared = (DadosCompartilhados*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!shared) {
        CloseHandle(hMap);
        error_handling("MapViewOfFile falhou no cliente");
    }

    HANDLE hMutex = CreateMutexA(NULL, FALSE, "Local\\PongSharedMutex");
    if (!hMutex) {
        UnmapViewOfFile(shared);
        CloseHandle(hMap);
        error_handling("CreateMutex falhou no cliente");
    }

    while (!WindowShouldClose()) {
        // INPUT P2: Seta Cima/Baixo com GetAsyncKeyState (sem precisar clicar)
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (playerY > 0) {
                playerY -= playerVelocidade;
            }
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            if (playerY + playerAltura < alturaTela) {
                playerY += playerVelocidade;
            }
        }

        
        WaitForSingleObject(hMutex, INFINITE);
        shared->player2Y = playerY;
        ReleaseMutex(hMutex);

        
        WaitForSingleObject(hMutex, INFINITE);
        memcpy(&dados, shared, sizeof(DadosCompartilhados));
        ReleaseMutex(hMutex);
 
        
        BeginDrawing();
        ClearBackground(BLACK);
 
       
        DrawRectangle(playerX, playerY, playerLargura, playerAltura, WHITE);
        
        
        if (dados.bolaX >= larguraTela) {
           
            float bolaLocalX = dados.bolaX - larguraTela;
            DrawRectangle(bolaLocalX, dados.bolaY, 15, 15, WHITE);
        }
 
        // Placar (branco)
        DrawText(TextFormat("P1: %d", dados.placar1), 10, 10, 20, WHITE);
        DrawText(TextFormat("P2: %d", dados.placar2), larguraTela - 80, 10, 20, WHITE);

        if (dados.placar1 >= 10 || dados.placar2 >= 10) {
            const char *msg = (dados.placar1 >= 10) ? "Player 1 Venceu!" : "Player 2 Venceu!";
            DrawText(msg, larguraTela/2 - MeasureText(msg, 30)/2, alturaTela/2 - 40, 30, WHITE);
            DrawText("Servidor reinicia o jogo", larguraTela/2 - 120, alturaTela/2, 20, WHITE);
        }

        EndDrawing();
    }

    UnmapViewOfFile(shared);
    CloseHandle(hMap);
    CloseHandle(hMutex);

    closesocket(clientSocket);
    WSACleanup();
    CloseWindow();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOGDI
#define NOUSER
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>

#include "raylib.h"

typedef struct {
    float bolaX;
    float bolaY;
    float bolaVelocidadeX;
    float bolaVelocidadeY;
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

    printf("=== CLIENTE PONG - Player 2 ===\n");
    
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
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
    serverAddr.sin_port = htons(51171);
    
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        error_handling("Erro ao conectar ao servidor");
    }
    printf("Iniciando jogo...\n\n");

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
    
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    float playerX = larguraTela - 35;
    float playerY = alturaTela / 2 - 50;
    float playerLargura = 25;
    float playerAltura = 100;
    float playerVelocidade = 10;
    Rectangle player = {playerX, playerY, playerLargura, playerAltura};

    float oponenteX = 10;
    float oponenteY = alturaTela / 2 - 50;
    Rectangle oponente = {oponenteX, oponenteY, playerLargura, playerAltura};

    Rectangle bola = {larguraTela / 2, alturaTela / 2, 15, 15};

    InitWindow(larguraTela, alturaTela, "PONG - Player 2 (Cliente)");
    SetTargetFPS(60);

    DadosCompartilhados dados;
    Mensagem msgEnviar, msgReceber;
    
    FILE *fileShared = NULL;
    int tentativas = 0;
    while (tentativas < 50 && !fileShared) {
        fileShared = fopen("pong_shared.dat", "rb");
        if (!fileShared) {
            WaitTime(0.1);
            tentativas++;
        }
    }

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
            player.y -= playerVelocidade;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
            player.y += playerVelocidade;
        }

        if (player.y < 0) player.y = 0;
        if (player.y + playerAltura > alturaTela) player.y = alturaTela - playerAltura;

        msgEnviar.playerY = player.y;
        msgEnviar.ativo = 1;
        send(clientSocket, (char*)&msgEnviar, sizeof(Mensagem), 0);

        int bytesReceived = recv(clientSocket, (char*)&msgReceber, sizeof(Mensagem), 0);
        if (bytesReceived > 0) {
            oponente.y = msgReceber.playerY;
        }

        if (fileShared) {
            fseek(fileShared, 0, SEEK_SET);
            fread(&dados, sizeof(DadosCompartilhados), 1, fileShared);
        }

        bola.x = dados.bolaX - larguraTela;
        bola.y = dados.bolaY;

        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleRec(player, WHITE);
        
        if (dados.bolaX >= larguraTela) {
            DrawRectangleRec(bola, WHITE);
        }

        DrawText(TextFormat("P1: %d", dados.placar1), 10, 10, 20, WHITE);
        DrawText(TextFormat("P2: %d", dados.placar2), larguraTela - 80, 10, 20, WHITE);

        if (dados.placar1 >= 10 || dados.placar2 >= 10) {
            const char *msg = (dados.placar1 >= 10) ? "Player 1 Venceu!" : "Player 2 Venceu!";
            DrawText(msg, larguraTela/2 - MeasureText(msg, 30)/2, alturaTela/2 - 40, 30, WHITE);
            DrawText("Servidor reinicia o jogo", larguraTela/2 - 120, alturaTela/2, 20, GRAY);
        }

        EndDrawing();
    }

    if (fileShared) fclose(fileShared);
    closesocket(clientSocket);
    WSACleanup();
    CloseWindow();
    return 0;
}

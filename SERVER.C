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

    printf("=== SERVIDOR PONG - Player 1 ===\n");
    
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

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    float playerX = 10;
    float playerY = alturaTela / 2 - 50;
    float playerLargura = 25;
    float playerAltura = 100;
    float playerVelocidade = 10;
    Rectangle player = {playerX, playerY, playerLargura, playerAltura};

    float oponenteX = larguraTela - 35;
    float oponenteY = alturaTela / 2 - 50;
    Rectangle oponente = {oponenteX, oponenteY, playerLargura, playerAltura};

    Rectangle bola = {larguraTela / 2, alturaTela / 2, 15, 15};

    FILE *f = fopen("pong_shared.dat", "wb");
    if (f) {
        DadosCompartilhados dados = {
            larguraTela / 2.0f, alturaTela / 2.0f,
            0.0f, 0.0f, 0, 0
        };
        fwrite(&dados, sizeof(DadosCompartilhados), 1, f);
        fclose(f);
    }

    InitWindow(larguraTela, alturaTela, "PONG - Player 1 (Servidor)");
    SetTargetFPS(60);

    DadosCompartilhados dados;
    Mensagem msgEnviar, msgReceber;
    bool jogoIniciado = false;

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
        int bytesSent = send(clientSocket, (char*)&msgEnviar, sizeof(Mensagem), 0);

        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        
        if (select(0, &readfds, NULL, NULL, &timeout) > 0) {
            int bytesReceived = recv(clientSocket, (char*)&msgReceber, sizeof(Mensagem), 0);
            if (bytesReceived > 0) {
                oponente.y = msgReceber.playerY;
            }
        }

        FILE *fRead = fopen("pong_shared.dat", "rb");
        if (fRead) {
            fread(&dados, sizeof(DadosCompartilhados), 1, fRead);
            fclose(fRead);
        }

        if (!jogoIniciado && IsKeyPressed(KEY_ENTER)) {
            jogoIniciado = true;
            dados.bolaVelocidadeX = 5;
            dados.bolaVelocidadeY = 5;
        }

        if (jogoIniciado) {
            dados.bolaX += dados.bolaVelocidadeX;
            dados.bolaY += dados.bolaVelocidadeY;

            if (dados.bolaY >= alturaTela || dados.bolaY <= 0) {
                dados.bolaVelocidadeY *= -1;
            }

            Rectangle bolaRect = {dados.bolaX, dados.bolaY, 15, 15};
            if (CheckCollisionRecs(player, bolaRect) && dados.bolaVelocidadeX < 0) {
                dados.bolaVelocidadeX *= -1;
                dados.bolaX = player.x + playerLargura + 1;
            }

            if (CheckCollisionRecs(oponente, bolaRect) && dados.bolaVelocidadeX > 0) {
                dados.bolaVelocidadeX *= -1;
                dados.bolaX = oponente.x - 16;
            }

            if (dados.bolaX >= larguraTela) {
                dados.placar1 += 1;
                dados.bolaX = larguraTela / 2;
                dados.bolaY = alturaTela / 2;
                dados.bolaVelocidadeX = 0;
                dados.bolaVelocidadeY = 0;
                jogoIniciado = false;
            }
            if (dados.bolaX <= 0) {
                dados.placar2 += 1;
                dados.bolaX = larguraTela / 2;
                dados.bolaY = alturaTela / 2;
                dados.bolaVelocidadeX = 0;
                dados.bolaVelocidadeY = 0;
                jogoIniciado = false;
            }
        }

        if ((dados.placar1 >= 10 || dados.placar2 >= 10) && IsKeyPressed(KEY_ENTER)) {
            dados.placar1 = 0;
            dados.placar2 = 0;
            dados.bolaX = larguraTela / 2;
            dados.bolaY = alturaTela / 2;
            dados.bolaVelocidadeX = 0;
            dados.bolaVelocidadeY = 0;
            jogoIniciado = false;
        }

        FILE *fWrite = fopen("pong_shared.dat", "wb");
        if (fWrite) {
            fwrite(&dados, sizeof(DadosCompartilhados), 1, fWrite);
            fclose(fWrite);
        }

        bola.x = dados.bolaX;
        bola.y = dados.bolaY;

        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleRec(player, WHITE);
        DrawRectangleRec(oponente, GRAY);
        DrawRectangleRec(bola, WHITE);

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

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    CloseWindow();
    return 0;
}
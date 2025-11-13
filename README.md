🕹️ # PPONG!!

Um remake moderno e experimental do clássico Pong, agora com comunicação entre processos e renderização com Raylib.
Dois programas distintos — servidor e cliente — trocam informações em tempo real, simulando uma partida em rede local.

⚙️ Descrição

O projeto demonstra:

Envio e recebimento de mensagens entre processos usando memória compartilhada e sockets.

Sincronização de estado do jogo entre dois lados (placar, posição da bola, movimentos das raquetes).

Uso da Raylib para toda a parte gráfica.

Um servidor que mantém o estado global e um cliente que o atualiza em tempo real.

Em outras palavras: é o Pong, mas com cérebro de sistema operacional e alma de engenheiro de rede.

🧠 Conceitos Envolvidos

IPC (Inter-Process Communication)

Sockets TCP/UDP

Raylib
Framework C para desenvolvimento de jogos 2D e 3D, responsável por toda a parte visual.


🚀 Execução

Baixe o Server.exe e o Client.exe, e execute-os da mesma forma.

caso queira compilar e/ou ver os codigos.

veja este tutorial de como ajustar o raylib para a sua maquina: https://youtu.be/-F6THkPkF2I?si=MkOPns4eJw27FFlc

codigo de compilacao server: gcc SERVER.c -o server.exe -IC:\raylib\raylib\src -LC:\raylib\raylib\src -lraylib -lws2_32 -lopengl32 -lgdi32 -lwinmm
codigo de compilacao client: gcc CLIENT.c -o client.exe -IC:\raylib\raylib\src -LC:\raylib\raylib\src -lraylib -lws2_32 -lopengl32 -lgdi32 -lwinmm

(Não é necessário nenhum arquivo de configuração — a comunicação é direta e autônoma.)

🧩 Dependências
Raylib

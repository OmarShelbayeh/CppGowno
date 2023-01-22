#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define SUCCESS 1001
#define FAILURE 1002
#define SERVER_PORT 2137
#define WHITE 0
#define RED 1

using namespace std;

int gameOver = 0;
bool gameStarted = false;

struct cln
{
    int cfd;
    struct sockaddr_in caddr;
};

struct Game
{
    cln player1;
    cln player2;
    int currentPlayer; // 0 = player 1, 1 = player 2
};

struct msg
{
    int ponStart;
    int ponEnd;
    int ponSkipped[];
};

Game game{0, 0, 0};

void handleCommunication(cln currentPlayer, cln opponent)
{
    char buf[32];
    recv(currentPlayer.cfd, &buf, sizeof(buf), 0);

    if (strcmp(buf, "Game Over") == 0)
    {
        gameOver = 1;
    }
    send(opponent.cfd, buf, strlen(buf), 0);
    cout << "Message from client: " << buf << endl;
}

int waitForConnections(int serverSocket, sockaddr_storage serverStorage, socklen_t addr_size)
{
    struct sockaddr_in client1addr;
    struct sockaddr_in client2addr;

    // Connection with client 1
    int cln1Soc = accept(serverSocket, (struct sockaddr *)&client1addr,
                         &addr_size);

    cout << "Connection with client 1" << endl;

    // Connection with client 2
    int cln2Soc = accept(serverSocket, (struct sockaddr *)&client2addr,
                         &addr_size);

    if (cln1Soc == -1 || cln2Soc == -1)
    {
        cout << "Error when accepting connection" << endl;
        return FAILURE;
    }

    cout << "Connection with client 2" << endl;

    cln client1{cln1Soc, client1addr};
    cln client2{cln2Soc, client2addr};

    char white[32] = "WHITE";
    char red[32] = "RED";
    send(client1.cfd, white, strlen(white), 0);
    send(client2.cfd, red, strlen(red), 0);

    game.player1 = client1;
    game.player2 = client2;
    game.currentPlayer = WHITE;

    return SUCCESS;
}

int mainLoop(int serverSocket, sockaddr_storage serverStorage, socklen_t addr_size)
{
    cln currentPlayer = game.currentPlayer == WHITE ? game.player1 : game.player2;
    cln opponent = game.currentPlayer == WHITE ? game.player2 : game.player1;

    if (!gameStarted)
    {
        if (waitForConnections(serverSocket, serverStorage, addr_size) == SUCCESS)
        {
            gameStarted = true;
        }
        else
        {
            gameOver = 1;
            exit(1);
        }
    }
    else
    {
        handleCommunication(currentPlayer, opponent);
        game.currentPlayer = game.currentPlayer == WHITE ? RED : WHITE;
    }
    return 1;
}

int main()
{
    int serverSocket, cln1Soc, cln2Soc;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    srand(time(NULL));

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    const int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    if (listen(serverSocket, SERVER_PORT) == 0)
    {
        cout << "Server is listening" << endl;
    }
    else
    {
        perror("Error when listeting");
    }
    addr_size = sizeof(serverStorage);

    while (gameOver == 0)
    {
        mainLoop(serverSocket, serverStorage, addr_size);
    }

    close(game.player1.cfd);
    close(game.player2.cfd);
    return 1;
}

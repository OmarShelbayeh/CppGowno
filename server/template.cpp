#include <sys/types.h>
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

struct cln
{
  int cfd;
  struct sockaddr_in caddr;
};

void childend(int signc)
{
  wait(NULL);
  printf("Zakonczono proces potomny\n");
}


int sendData(int cfd, char buf[], int dataLength)       // Funkcja sluzaca wysylaniu danych do momentu napotkania znaku konca danych.
                                                        // Zwraca ilsoc wyslanych danych w razie powodzenia oraz 0 w razie bledu
{
	int allDataSend = 0;                                // Czy wyslano wszystkie dane
	int totalDataSend = 0;                              // Ilosc danych wyslana dotychczas

	while(allDataSend == 0)
	{

		int writeResult = write(cfd, buf + totalDataSend, dataLength - totalDataSend);  // Wysylamy dane przesuwajac sie po buforze

		if(writeResult == 0 || writeResult == -1) {     // Obsluga bledow
			printf("%s\n", strerror(errno));
			return 0;
		}
		else {
			totalDataSend = totalDataSend + writeResult;

			if (buf[totalDataSend-1] == 'X')            // Jezeli osiagnieto znak konca danych
				allDataSend = 1;                        // Koniec wysylania
		}
	}

	return totalDataSend;
}

int receiveData(int cfd, char buf[], int dataLength)    // Funkcja analogiczna do sendData tylko sluzaca odbieraniu danych
{
	memset(buf, '*', 66);

	int allDataReceive = 0;
	int totalDataReceive = 0;

	while(allDataReceive == 0)
	{

		int readResult = read(cfd, buf + totalDataReceive, dataLength - totalDataReceive);

		if(readResult == 0 || readResult == -1) {
			return 0;
		}
		else {
			totalDataReceive = totalDataReceive + readResult;

			if (buf[totalDataReceive-1] == 'X')
				allDataReceive = 1;


		}
	}

	return totalDataReceive;
}

void *th(void* arg[])                           // funkcja obsulujaca gre
{
  char buf[66];
  struct cln* client1 = (struct cln*)arg[0];
  struct cln* client2 = (struct cln*)arg[1];
     
  printf("New connection: %s, Port: %d\n", inet_ntoa((struct in_addr)client1->caddr.sin_addr), client1->caddr.sin_port);
  printf("New connection: %s, Port: %d\n", inet_ntoa((struct in_addr)client2->caddr.sin_addr), client2->caddr.sin_port);
  
  int gameOver = 0;                             // Czy koniec gry
  int opponentConnError = 0;                    // Czy gracz ma problem z polaczeniem

  int rStatus;                                  // Status odczytu danych
  int wStatus;                                  // Status wysylania danych



  wStatus = sendData(client1->cfd, "wX", 2);

  if(wStatus == 0)                              // Jezeli blad to oznacza ze gracz wylaczyl klienta nim drugi gracz zaczal szukac gry
  {
        sendData(client2->cfd, "EX",2);         // Informujemy drugiego gracza o tym fakcie
        close(client1->cfd);                    // Zwalniamy zasoby
        close(client2->cfd);
        free(client1);
        free(client2);
        return EXIT_SUCCESS;

  }
  else {
      wStatus = sendData(client2->cfd, "bX", 2);

      if(wStatus == 0)
      {
          sendData(client1->cfd, "EX",2);
          close(client1->cfd);
          close(client2->cfd);
          free(client1);
          free(client2);
          return EXIT_SUCCESS;
      }

  }



  while(gameOver == 0)                                  // Glowna petla w ktorej obslugiwani sa gracze naprzemiennie az do zakonczenia gry
	{

		rStatus = receiveData(client1->cfd,buf,66);     // Czekamy na dane o wykonanym ruchu od pierwszego gracza

		if(rStatus == 0) {                              // Jezeli gracz sie rozlaczyl
			opponentConnError = 1;
			gameOver = 1;
		}


		// Kazdy z ponizszych przypadkow oznacza dla serwera tyle ze musi przeslac wiadomosc do gracza drugiego i moze juz zakonczyc ten watek

		if(buf[0] == 'L') // Koniec gry
			gameOver = 1;

		if(buf[0] == 'S') // Surrender
			gameOver = 1;

		if(buf[0] == 'T') // Time out
			gameOver = 1;

		if(opponentConnError == 1)              // Ustawienie odpowiedniej informacji dla gracza aby poinformowac go o tym ze przeciwnik wyszedl z gry
		{
			buf[0] = 'E';
			buf[65] = 'X';
		}

		wStatus = sendData(client2->cfd, buf, 66);

		if(wStatus == 0) {                      // Jezeli ten gracz sie rozlaczyl
			if(opponentConnError == 0)          // Jezeli drugi gracz jest ok to trzeba mu przekazac informacje
				opponentConnError = 1;
			else
				gameOver = 1;                   // jezeli nie to konczymy
		}


		if(gameOver == 1)
			break;



		if(opponentConnError == 0) {                        // Ponizej analogicznie tylko z drugim graczem
			rStatus = receiveData(client2->cfd, buf, 66);
			if (rStatus == 0) {
				opponentConnError = 1;
				gameOver = 1;
			}
		}

		if(buf[0] == 'L') // Koniec gry
			gameOver = 1;

		if(buf[0] == 'S') // Surrender
			gameOver = 1;

		if(buf[0] == 'T') // Time out
			gameOver = 1;

		if(opponentConnError == 1)
		{
			buf[0] = 'E';
			buf[65] = 'X';
			gameOver = 1;
		}


		wStatus = sendData(client1->cfd, buf, 66);
		if(wStatus == 0) {

			buf[0] = 'E';
			buf[65] = 'X';
			wStatus = sendData(client2->cfd, buf, 66);
			gameOver = 1;

		}



	}
        
  close(client1->cfd);          //Zwolnienie zasobow po zakonczonej grze
  close(client2->cfd);
  free(client1);
  free(client2);


  return EXIT_SUCCESS;
  
}

int room = -1;                  // ilosc graczy w pokoju
struct cln * clientsInfo[2];    // tablica przechowujaca informacje o graczach grajacych ze soba

int main(int argc, char * argv[])
{
  pthread_t tid;
 	socklen_t sl;
	int sfd, on = 1;
	struct sockaddr_in saddr;//, caddr;

	signal(SIGCHLD, childend);
	
	saddr.sin_family = PF_INET;
  	saddr.sin_port = htons(1234);
  	saddr.sin_addr.s_addr = INADDR_ANY;
	
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr));
	listen(sfd, 5);

	while(1)
	{
	  struct cln* c = malloc(sizeof(struct cln));
	  sl = sizeof(c->caddr);
        
	  c->cfd = accept(sfd, (struct sockaddr*)&c->caddr, &sl);
	  printf("New player\n");
	  room = room + 1;
	  clientsInfo[room] = c;
     	  
	if(room == 1)
	{
	  printf("Creating new room\n");
	  pthread_create(&tid, NULL,th,clientsInfo);
	  pthread_detach(tid);
	  room = -1;
	}
	
	}
	
	close(sfd);
	
	return 1;
}

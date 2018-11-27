#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	LE_PIFIE_FEO 1

void armar_mensaje(char *p);
/*********************************************************
 * funcion MAIN
 * Orden Parametros: IP destino, puerto , Cantidad de lecturas , TiempoEntre Lecturas
 *
 *********************************************************/
void armar_mensaje(char *p)
{
	pid_t pid;
	//int i;
	//const char mensaje[]={"Hola, soy PID 12345"};
	//14 15 16 17
	pid=getpid();
	*(p-6)='0'+ pid/10000;
	pid=pid%10000;
	*(p-5)='0'+ pid/1000;
	pid=pid%1000;
	*(p-4)='0'+ pid/100;
	pid=pid%100;
	*(p-3)='0'+ pid/10;
	pid=pid%10;
	*(p-2)='0'+ pid;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in origen,destino;
	int sockfd,i,aux;
	char mensaje[]={"Hola, soy PID 12345"};
	unsigned char respuesta[200];

    struct dato_memoria {
        int temperatura;
        unsigned long N_lectura;
        // hora de lectura o algo asi
    }*pSdatomemoria;


	if (argc != 5)
	{
		printf("USO: \n");
		printf("	cliente IP-Destino Puerto Nlecturas TiempoEntreLecturas\n");
		return LE_PIFIE_FEO;
	}

	//creamos el socket que manejara esta conexión
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
	{
		printf("Error al crear el socket\n");
		return LE_PIFIE_FEO;
	}
	// ponemos a 0 las estructuras con las direcciones
	memset(&origen,0,sizeof(struct sockaddr_in));
	memset(&destino,0,sizeof(struct sockaddr_in));

	// cargamos la estructura con las direcciones de rigen
	origen.sin_family = AF_INET;
	origen.sin_port = htons(0); //Asigna un puerto disponible de la maquina
	origen.sin_addr.s_addr = htonl(INADDR_ANY); //Asigna una IP de la maquina

	// Ahora a conectar el socket con este programa (asignamos el puerto)
	// en el cliente no hace falta
	if (bind(sockfd,(struct sockaddr*)&origen, sizeof(origen)) < 0)
	{
		printf("Error al hacer el bind\n");
		close(sockfd);
		return LE_PIFIE_FEO;
	}

	// Ahora preparamos la estructura con las direcciones de destino
	destino.sin_family = AF_INET;
	destino.sin_addr.s_addr = inet_addr(*(argv+1));	// dir IP destino
	destino.sin_port = htons( atoi(*(argv+2)) );		// puerto destino

	// Intentams la conexion con el destino
    if (connect(sockfd, (struct sockaddr*)&destino, sizeof(destino)) < 0)
    {
		perror("Error en connect");
		close(sockfd);
		return(LE_PIFIE_FEO);
	}

	//printf("sizeof=%d sterlen=%d\n",sizeof(mensaje),strlen(mensaje));
	// sizeof = 20 y strlen=19
	armar_mensaje(mensaje+sizeof(mensaje));
	// Listo ahora a pedir conexiones
	for(i=0;i<atoi(*(argv+3));i++)
	{
		// Enviamos el mensaje "estoy vivo"
		if(send(sockfd, mensaje, sizeof(mensaje), 0) < 0)
		{
			perror("Error al enviar");
			close(sockfd);
			return(LE_PIFIE_FEO);
		}

		printf("\n->Enviando: %s, a: %s en el puerto: %s \n",mensaje, *(argv+1), *(argv+2));

		// Recibe la respuesta
		if((aux=recv(sockfd, &respuesta, sizeof(respuesta),0)) > 0)
		{
			printf("<-Recibido: N bytes %d\n",aux);
			pSdatomemoria=(struct dato_memoria*) respuesta;
			pSdatomemoria->temperatura=ntohl(pSdatomemoria->temperatura);
			pSdatomemoria->N_lectura=ntohl(pSdatomemoria->N_lectura);

			printf("Temperatura leida %d,%d ºC ... Nº lectura %lu\n",
					pSdatomemoria->temperatura/10,pSdatomemoria->temperatura%10,
					pSdatomemoria->N_lectura);
		}
		else
		{
			perror("reciv devolvio <=0 revisar");
			close(sockfd);
			//return(LE_PIFIE_FEO);
		}
		sleep(atoi(*(argv+4)));
	}
	// Se cierra la conexion (socket)
	close(sockfd);
	return 0;
}

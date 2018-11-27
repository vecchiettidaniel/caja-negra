#ifndef INC_CODEHIJOS_H_
#define INC_CODEHIJOS_H_

#include "includes.h"
#include "globales.h"
#include "ipcs.h"
#include <sys/types.h>
#include <sys/socket.h> // recv send
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h> 	//exit
#include <unistd.h>		//close
#include <string.h>		//memset
#include <fcntl.h>		//open
#include <stdio.h>		// perror
#include <errno.h>		//errno
#include <netinet/in.h> // htol, dirs ets

#define TIEMPO_MUERTO 5     // segundos que esper antes de cerrar conexion

int codigo_pibes(int sfd,int mi_memoria,int mi_semaforo);
int chequear_clave(char *p);


int chequear_clave(char *p)
{
	const char *clave={"Hola, soy PID"};
	int i,resultado;

	i=0;
	resultado=1;
    while( ( i < (sizeof(clave) -1) ) && resultado )
    {
        resultado=(*(p+i)==*(clave+i))?1:0;
        i++;
    }
	// res=0 si no es igual
	// res=1 si las claves coinciden
	return resultado;
}


/* =======================
 *
 * =======================*/
int codigo_pibes(int sfd,int mi_memoria,int mi_semaforo)
{
	char mensaje[TAMANO_BUFFER];
	int aux,error_detectado=0,totalenviados=0;

    fd_set readfds;
    struct timeval timeout;

    struct dato_memoria DatoaEnviar,*pLeerTempsAca;

	// Atachamos la memria compartida
	pLeerTempsAca=(struct dato_memoria *)shmat(mi_memoria,NULL,0);
	if(pLeerTempsAca==((struct dato_memoria *)-1))
	{
		perror("Erro al Atachear la memoria\n");
        close(sfd);
		exit(LE_PIFIE_FEO);
	}

	printf("Hijo: mi pid %d el de papa: %d\n",getpid(),getppid());
    aux=1;          // sale porque el clente termino la conexion
    fin_programa=1; // sale por error o por señal

	while(aux && fin_programa)
    {
        FD_ZERO(&readfds);
        FD_SET(sfd,&readfds);
        timeout.tv_sec=TIEMPO_MUERTO;
        timeout.tv_usec=0;

        aux=select(sfd+1, &readfds, NULL, NULL, &timeout);
        if(aux==0)
        {   //time out alcanzado
            printf("Hijo %d: Timeout alcanzado\n",getpid());
            fin_programa=0;
        }

        if(aux<0)
        {
            if(errno!=EINTR)
            {   // erro en el select
        		perror("Error en SELCT");
                fin_programa=0;
                error_detectado=1;
            }
            else
            {   // el select salio por una señal, las condiciones se setean
                // en el handler de la señal
                printf("Hijo %d: Select salio por señal\n",getpid());
            }
        }

        if(aux>0) // del select
        {   // todo ok ahra a recibir, no se deberia bloquear ya que select
            // me dejo todo OK para recibir el dato
            aux=recv(sfd, mensaje, sizeof(mensaje),0);
            if(aux<0)
            {//recv dio error (que no devuelva -1)
                // si errno es EINTR => recivi una señal!!!! ojo aca
        		perror("Erro en el recv\n");
                fin_programa=0;
                error_detectado=1;
            }

        	if(aux > 0) //del recv
        	{  //recibi el mensaje
				//printf("\nHijo %d Recibi: %s\n",getpid(),mensaje);
				// comparo lo recibido con un patron para ver que el cliente sepa
                // usar el servicio, chequear devuelve 1 si todo OK
				aux=chequear_clave(mensaje);
                if(aux)
				{// clave OK
	        		// Ahora tengo que mandarle la temperatura que leyeron del sensor
					if(leer_dato (mi_semaforo, pLeerTempsAca,&DatoaEnviar))
					{
						printf("HIJO: %d eror al leer el dato\n",getpid());
	                    fin_programa=0;
	                    error_detectado=1;
					}

					else
					{// todo OK, envio el dato
		        		DatoaEnviar.temperatura=htonl(DatoaEnviar.temperatura);
		                DatoaEnviar.N_lectura=htonl(DatoaEnviar.N_lectura);

		        		if (send(sfd, &DatoaEnviar,sizeof(struct dato_memoria), 0) == -1)
		        		{
		        			perror("Error en send\n");
		                    fin_programa=0;
		                    error_detectado=1;
		        		}
					}
				}//if(aux) aux=chequear_clave(mensaje);
				else //clave incorrecta
                {
                    printf("Hijo %d: Error en mensaje cerrando conexion\n",getpid());
                	fin_programa=0;
                }

        		// despues de esto me quedo bloquead en el select hasta que me escriban
        		// de guelta... si se corta la comunicacion => recv devuelve 0
        		totalenviados++;	// veces que mande algo

            }// if(aux > 0) //del recv
            // si aux=0 el cliente termino l aconexion
    	}// if(aux<0) del select

    }//while(aux && fin_programa)

    if(error_detectado)
    {// ocurrio algun error en algun punto del programa
        printf("Hijo %d: Error detectado, cerrando conexiones y saliendo\n",getpid());
    }
    else
    {
        if(fin_programa)
        {
            printf("Hijo %d: todo OK cerrando conexion, envie %d veces \n",getpid(),totalenviados);
        }
        else
        {
            printf("Hijo %d: programa terminado, envie %d veces \n",getpid(),totalenviados);
        }
    }
	desatachar_memoria((const void *) pLeerTempsAca);
	close(sfd);
	exit(error_detectado);

    /* Pa entender las condiciones: si error detectado es =1 => fin programa es 0
     * Si error_detectado es = 0 => el programa termino por alguna de las siguientes
     * Fin_Programa=0 => o timeOUT, o cliente envio mensaje malo o papa me mato
     * Fin_programa=1 => cliente cerro comunicacion (sale por aux=0)*/
}

#endif //#ifndef INC_CODEHIJOS_H_

#include <getopt.h>
#include <dirent.h> 
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

void atender_cliente(int connfd);
void list_files(int connfd);
void send_file(int connfd, char *filename);
void print_help(char *command);
void print_error(char *command);

bool seguir = true;

int main(int argc, char **argv) {
	int opt;

	//Sockets
	int listenfd, connfd;
	unsigned int clientlen;
	
	//Direcciones y puertos
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp, *port, *ipaddress, *path;

	while ((opt = getopt (argc, argv, "h")) != -1){
		switch(opt)
		{
			case 'h':
				print_help(argv[0]);
				return 0;
			default:
				print_error(argv[0]);
				return 1;
		}
	}

	if(argc != 4){
		fprintf(stderr, "Cantidad inválida de argumentos ingresados.\n");
		print_error(argv[0]);
		return 1;
	}

	//Captura de argumentos ingresados por consola.
	ipaddress = argv[1];
	port = argv[2];
	path = argv[3];

	//Validación del puerto.
	int port_n = atoi(port);
	if(port_n <= 0 || port_n > USHRT_MAX){
		fprintf(stderr, "Puerto: %s invalido. Ingrese un número entre 1 y %d.\n", port, USHRT_MAX);
		return 1;
	}

	//Cambio de work directory.
	if (chdir(path)) {
		fprintf(stderr, "'%s' NO es un directorio válido.\n", path);
		return 1;
	}
	printf("Directorio de trabajo cambiado a '%s'\n", path);
  
	//Apertura de un socket de escucha en port
	listenfd = open_listenfd(port);
	if (listenfd < 0)
		connection_error(listenfd);
	printf("server escuchando en:\nDirección IP:\t%s\nPuerto:\t\t%s\n...\n", ipaddress, port);

	while (seguir) {
		clientlen = sizeof(clientaddr);
		connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

		/* Determine the domain name and IP address of the client */
		hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
					sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);

		printf("server conectado a %s (%s)\n", hp->h_name, haddrp);
		atender_cliente(connfd);
		printf("server desconectando a %s (%s)\n", hp->h_name, haddrp);

		close(connfd);
	}

	return 0;
}

void atender_cliente(int connfd) {
	int n;
	char buff_input[MAXLINE] = {0};//Buffer del cliente.

	while(1) {
		n = read(connfd, buff_input, MAXLINE);
		if(n <= 0)
			return;

		//Detecta "exit" y se desconecta del cliente
		if(strcmp(buff_input, "exit\n") == 0){
			return;
		}

		//Detecta "kill", se desconecta del cliente y termina
		if(strcmp(buff_input, "kill\n") == 0){
			seguir = false;
			return;
		}
		
		if (strcmp(buff_input, "_listar_\n") == 0) {
			//Listar archivos disponibles.
			printf("Se ha recibido una solicitud de listar archivos.\n");
			printf("Listando archivos...\n");
			list_files(connfd);
		} else {
			//Envío de archivo.
			buff_input[n-1] = '\0';
			printf("Se ha recibido una solicitud para enviar archivo '%s'.\n", buff_input);
			send_file(connfd, buff_input);
		}
		memset(buff_input, 0, MAXLINE);
	}
}


void print_help(char *command) {
	printf("Servidor de archivos.\n");
	printf("uso:\n\t%s <ip address> <puerto> <directorio de archivos>\n", command);
	printf("\t%s -h\n", command);
	printf("Opciones:\n");
	printf("\t-h\t\t\tAyuda, muestra este mensaje\n");
}


void print_error(char *command) {
	fprintf(stderr, "uso:\n\t%s <ip address> <puerto> <directorio de archivos>\n", command);
	fprintf(stderr, "\t%s -h\n", command);
}


void list_files(int connfd) {
	struct dirent *dir;
	char buff_out[MAXLINE] = {0};
	char buff_in[3] = {0};
	DIR *d = opendir(".");
	int n;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			strcpy(buff_out, dir->d_name);
			n = write(connfd, buff_out, strlen(buff_out));
			if(n <= 0)
				break;
			read(connfd, buff_in, 3);
			memset(buff_in, 0, 3);
			memset(buff_out, 0, n);
		}
		write(connfd, "_fin", 4);
	}
	closedir(d);
}


void send_file(int connfd, char *filename) {
	//Verificar existencia y tamaño de archivo.
	struct stat mi_stat;
	if(stat(filename, &mi_stat) < 0){
		write(connfd, "no_existe", 9);
		return;
	}

	printf("Enviando archivo %s...\n", filename);
	int n, fd_read = open(filename, O_RDONLY, 0);
	void *buff_read = calloc(MAXLINE, sizeof(BYTE));

	//Mensaje inicial.
	char str[256];
	char buff_in[3] = {0};
	sprintf(str, "%ld", mi_stat.st_size);
	write(connfd, str, 256);

	while (true) {
		n = read(fd_read, buff_read, MAXLINE);
		write(connfd, buff_read, n);
		if(n <= 0)
			break;
		read(connfd, buff_in, 3);
		memset(buff_in, 0, 3);
		memset(buff_read, 0, n);
	}
	write(connfd, "_fin", 4);
	close(fd_read);
	free(buff_read);
}


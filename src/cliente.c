#include <getopt.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

void get_from_console(char *linea_consola);
void print_menu();
void print_help(char *command);
void print_error(char *command);

int main(int argc, char **argv) {
	int opt;

	//Socket
	int clientfd;
	//Direcciones y puertos
	char *hostname, *port;

	//Lectura desde consola
	char *linea_consola;
	char read_buffer[MAXLINE] = {0};
	ssize_t n;

	while ((opt = getopt (argc, argv, "h")) != -1){
		switch(opt) {
			case 'h':
				print_help(argv[0]);
				return 0;
			default:
				print_error(argv[0]);
				return 1;
		}
	}

	if(argc != 3){
		print_error(argv[0]);
		return 1;
	} else {
		hostname = argv[1];
		port = argv[2];
	}

	//Validación del puerto.
	int port_n = atoi(port);
	if(port_n <= 0 || port_n > USHRT_MAX){
		fprintf(stderr, "Puerto: %s invalido. Ingrese un número entre 1 y %d.\n", port, USHRT_MAX);
		return 1;
	}

	//Conexión al servidor retornando un socket conectado
	clientfd = open_clientfd(hostname, port);
	if(clientfd < 0)
		connection_error(clientfd);
	printf("Conectado exitosamente a %s en el puerto %s.\n", hostname, port);

	linea_consola = (char *) calloc(1, MAXLINE);
	while(true){
		//Leer desde consola.
		get_from_console(linea_consola);
		
		//Envia al servidor
		n = write(clientfd, linea_consola, strlen(linea_consola));
		if(n<=0)
			break;

		memset(read_buffer, 0, MAXLINE);
		if (strcmp(linea_consola, "_listar_\n") == 0) {
			while (true) {
				n = read(clientfd, read_buffer, MAXLINE);
				if (strcmp(read_buffer, "_fin") == 0)
					break;
				write(clientfd, "ok", 2);
				printf("%s\n", read_buffer);
				memset(read_buffer, 0, n);
			}
			printf("\n");
		} else if (strcmp(linea_consola, "kill\n") == 0 || strcmp(linea_consola, "exit\n") == 0)
			break;
		else {
			linea_consola[n-1] = '\0';
			char output_file[100] = {0} ;
			strcpy(output_file, linea_consola);

			//Presentar la cantidad de bytes.
			n = read(clientfd, read_buffer, MAXLINE);
			if (strcmp(read_buffer, "no_existe") == 0)
				printf("No existe el archivo %s.\n", output_file);
			else {
				printf("Tamaño del archivo:\t%s\n", read_buffer);
				int newFile = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
				
				void *aux = calloc(MAXLINE, sizeof(BYTE));
				while (true) {
					n = read(clientfd, aux, MAXLINE);
					if (strcmp(aux, "_fin") == 0)
						break;
					write(newFile, aux, n);
					write(clientfd, "ok", 2);
					memset(aux, 0, n);
				}
				close(newFile);
			}
			linea_consola[n-1] = '\n';
		}
		memset(read_buffer,0,MAXLINE); //Encerar el buffer
		memset(linea_consola,0,MAXLINE); //Encerar el buffer
	}

	printf("Desconectando...\n");
	free(linea_consola);
	close(clientfd);
	return 0;
}

void get_from_console(char *linea_consola){
	size_t max = MAXLINE;
	print_menu();
	printf(">");
	getline(&linea_consola, &max, stdin);
	bool badin = false;

	//Cambio de lo ingresado a comandos internos.
	do {
		if (strcmp(linea_consola, "1\n") == 0) {
			memcpy(linea_consola, "_listar_\n", 9);
			badin = false;
		} else if (strcmp(linea_consola, "2\n") == 0) {
			printf("Nombre del archivo a descargar:\t>");
			getline(&linea_consola, &max, stdin);
			badin = false;
		} else if (strcmp(linea_consola, "kill\n") == 0 || strcmp(linea_consola, "exit\n") == 0)
			badin = false;
		else {
			badin = true;
			fprintf(stderr, "Se ha ingresado un opción inválida.");
			printf("\nSeleccione una opción (número):\t");
			getline(&linea_consola, &max, stdin);
		}
	} while (badin);
}


void print_menu() {
	printf("Menú.\n");
	printf("1.\tListar archivos a descargar.\n");
	printf("2.\tDescargar archivo.\n");
	printf("\nSeleccione una opción (número):\t");
}


void print_help(char *command) {
	printf("Cliente para solicitar archivos.\n");
	printf("uso:\n\t%s <ip address> <puerto>\n", command);
	printf("\t%s -h\n", command);
	printf("Opciones:\n");
	printf("\t-h\t\t\tAyuda, muestra este mensaje\n");
}


void print_error(char *command) {
	fprintf(stderr, "uso:\n\t%s <ip address> <puerto>\n", command);
	fprintf(stderr, "\t%s -h\n", command);
}



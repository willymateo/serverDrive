.PHONY: all debug sanitize clean
all: bin/servidor bin/cliente bin/listar_dir

bin/servidor: bin/servidor.o bin/common.o src/common.h
	gcc -Wall -o bin/servidor bin/servidor.o bin/common.o

bin/cliente: bin/cliente.o bin/common.o src/common.h
	gcc -Wall -o bin/cliente bin/cliente.o bin/common.o

bin/listar_dir:
	gcc -Wall -o bin/listar_dir ejemplos/listar_contenido_directorio.c

bin/%.o: src/%.c src/common.h
	gcc -Wall -c -o $@ $< -iquote src/

run_servidor:
	./bin/servidor 127.0.0.1 9500 ./data/

run_cliente:
	./bin/cliente 127.0.0.1 9500

.PHONY: clean
clean:
	mkdir -p bin/
	rm -f bin/*

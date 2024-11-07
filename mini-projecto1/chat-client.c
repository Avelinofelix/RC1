//chat-client.C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <host> <porta>\n", argv[0]);
        return -1;
    }

    // Resolver endereço do servidor
    struct addrinfo hints, *resultado;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(argv[1], argv[2], &hints, &resultado);
    if (status != 0) {
        fprintf(stderr, "Erro no getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // Criar socket
    int cliente_socket = socket(resultado->ai_family, resultado->ai_socktype, resultado->ai_protocol);
    if (cliente_socket < 0) {
        perror("Erro ao criar socket");
        freeaddrinfo(resultado);
        return -1;
    }

    // Conectar ao servidor
    if (connect(cliente_socket, resultado->ai_addr, resultado->ai_addrlen) < 0) {
        perror("Erro ao conectar");
        close(cliente_socket);
        freeaddrinfo(resultado);
        return -1;
    }

    freeaddrinfo(resultado);

    fd_set read_fds;
    int max_fd;
    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(cliente_socket, &read_fds);
        max_fd = (STDIN_FILENO > cliente_socket) ? STDIN_FILENO : cliente_socket;

        // Esperar por entrada do usuário ou mensagem do servidor
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("Erro no select");
            break;
        }

        // Verificar entrada do usuário
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            ssize_t bytes_lidos = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
            if (bytes_lidos <= 0) {
                break;  // EOF (Ctrl+D) ou erro
            }
            buffer[bytes_lidos] = '\0';

            // Enviar mensagem para o servidor
            if (send(cliente_socket, buffer, bytes_lidos, 0) < 0) {
                perror("Erro ao enviar mensagem");
                break;
            }
        }

        // Verificar mensagens do servidor
        if (FD_ISSET(cliente_socket, &read_fds)) {
            ssize_t bytes_lidos = recv(cliente_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_lidos <= 0) {
                break;  // Conexão fechada pelo servidor ou erro
            }
            buffer[bytes_lidos] = '\0';

            // Imprimir mensagem recebida
            printf("%s", buffer);
        }
    }

    close(cliente_socket);
    return 0;
}
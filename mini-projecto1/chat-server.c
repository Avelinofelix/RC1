//chat-server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 4096

// Estrutura para armazenar informações dos clientes
typedef struct {
    int socket;
    char ip[INET_ADDRSTRLEN];
    int port;
} Cliente;

// Variáveis globais
Cliente clientes[MAX_CLIENTS];
int num_clientes = 0;
int servidor_socket;
volatile sig_atomic_t servidor_rodando = 1;

// Função para tratar o sinal SIGINT
void manipular_sigint(int sig) {
    servidor_rodando = 0;
}

// Função para enviar mensagem para todos os clientes exceto o remetente
void broadcast(char *mensagem, int remetente_socket) {
    for (int i = 0; i < num_clientes; i++) {
        if (clientes[i].socket != remetente_socket) {
            send(clientes[i].socket, mensagem, strlen(mensagem), 0);
        }
    }
}

// Função para remover um cliente
void remover_cliente(int socket) {
    char mensagem[BUFFER_SIZE];
    int idx = -1;

    // Encontrar o índice do cliente
    for (int i = 0; i < num_clientes; i++) {
        if (clientes[i].socket == socket) {
            idx = i;
            break;
        }
    }

    if (idx != -1) {
        // Enviar mensagem de saída
        snprintf(mensagem, BUFFER_SIZE, "%s:%d left.\n", 
                clientes[idx].ip, clientes[idx].port);
        broadcast(mensagem, socket);

        // Fechar socket e remover cliente
        close(socket);
        
        // Mover últimos clientes uma posição para trás
        for (int i = idx; i < num_clientes - 1; i++) {
            clientes[i] = clientes[i + 1];
        }
        num_clientes--;
    }
}

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        return -1;
    }

    // Configurar manipulador de sinal
    signal(SIGINT, manipular_sigint);

    // Criar socket do servidor
    servidor_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_socket < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    // Configurar endereço do servidor
    struct sockaddr_in servidor_addr;
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_port = htons(atoi(argv[1]));
    servidor_addr.sin_addr.s_addr = INADDR_ANY;

    // Permitir reutilização do endereço
    int opt = 1;
    if (setsockopt(servidor_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Erro no setsockopt");
        return -1;
    }

    // Vincular socket
    if (bind(servidor_socket, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr)) < 0) {
        perror("Erro no bind");
        return -1;
    }

    // Escutar conexões
    if (listen(servidor_socket, MAX_CLIENTS) < 0) {
        perror("Erro no listen");
        return -1;
    }

    fd_set read_fds;
    int max_fd;

    while (servidor_rodando) {
        FD_ZERO(&read_fds);
        FD_SET(servidor_socket, &read_fds);
        max_fd = servidor_socket;

        // Adicionar sockets dos clientes ao conjunto
        for (int i = 0; i < num_clientes; i++) {
            FD_SET(clientes[i].socket, &read_fds);
            if (clientes[i].socket > max_fd) {
                max_fd = clientes[i].socket;
            }
        }

        // Esperar por atividade em algum socket
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("Erro no select");
            break;
        }

        // Verificar nova conexão
        if (FD_ISSET(servidor_socket, &read_fds)) {
            struct sockaddr_in cliente_addr;
            socklen_t addr_len = sizeof(cliente_addr);
            int novo_socket = accept(servidor_socket, (struct sockaddr*)&cliente_addr, &addr_len);

            if (novo_socket < 0) {
                perror("Erro no accept");
                continue;
            }

            if (num_clientes >= MAX_CLIENTS) {
                close(novo_socket);
                continue;
            }

            // Adicionar novo cliente
            clientes[num_clientes].socket = novo_socket;
            inet_ntop(AF_INET, &cliente_addr.sin_addr, clientes[num_clientes].ip, INET_ADDRSTRLEN);
            clientes[num_clientes].port = ntohs(cliente_addr.sin_port);

            // Enviar mensagem de entrada
            char mensagem[BUFFER_SIZE];
            snprintf(mensagem, BUFFER_SIZE, "%s:%d join.\n", 
                    clientes[num_clientes].ip, clientes[num_clientes].port);
            broadcast(mensagem, -1);  // -1 para enviar para todos

            num_clientes++;
        }

        // Verificar mensagens dos clientes
        for (int i = 0; i < num_clientes; i++) {
            if (FD_ISSET(clientes[i].socket, &read_fds)) {
                char buffer[BUFFER_SIZE];
                int bytes_lidos = recv(clientes[i].socket, buffer, BUFFER_SIZE - 1, 0);

                if (bytes_lidos <= 0) {
                    remover_cliente(clientes[i].socket);
                    i--;  // Ajustar índice após remoção
                    continue;
                }

                buffer[bytes_lidos] = '\0';
                char mensagem[BUFFER_SIZE];
                snprintf(mensagem, BUFFER_SIZE, "%s:%d %s", 
                        clientes[i].ip, clientes[i].port, buffer);
                broadcast(mensagem, clientes[i].socket);
            }
        }
    }

    // Limpar e fechar todos os sockets
    for (int i = 0; i < num_clientes; i++) {
        close(clientes[i].socket);
    }
    close(servidor_socket);

    return 0;
}
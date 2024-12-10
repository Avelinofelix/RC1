#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "packet_format.h"

#define MAX_WINDOW_SIZE 32

typedef struct {
    data_pkt_t packet;
    int received;
} window_recv_t;

void usage() {
    fprintf(stderr, "Uso: ./file-receiver <ficheiro_saida> <porta> <tamanho_janela>\n");
    exit(-1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) usage();

    char *output_filename = argv[1];
    int port = atoi(argv[2]);
    int window_size = atoi(argv[3]);

    if (window_size < 1 || window_size > MAX_WINDOW_SIZE) {
        fprintf(stderr, "Tamanho da janela inválido. Deve estar entre 1 e %d\n", MAX_WINDOW_SIZE);
        exit(-1);
    }

    // Abrir ficheiro de saída
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Erro ao criar ficheiro de saída");
        exit(-1);
    }

    // Criar socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro na criação do socket");
        exit(-1);
    }

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro no bind do socket");
        exit(-1);
    }

    // Inicializar janela de recepção
    window_recv_t recv_window[MAX_WINDOW_SIZE];
    memset(recv_window, 0, sizeof(recv_window));

    int window_base = 1;
    socklen_t client_len = sizeof(client_addr);
    int file_completed = 0;

    while (!file_completed) {
        data_pkt_t recv_packet;
        ssize_t recv_size = recvfrom(sockfd, &recv_packet, sizeof(data_pkt_t), 0, 
                                     (struct sockaddr *)&client_addr, &client_len);

        if (recv_size < 0) {
            perror("Erro na recepção de pacote");
            continue;
        }

        uint32_t seq_num = ntohl(recv_packet.seq_num);

        // Verificar se o pacote está dentro da janela de recepção
        if (seq_num >= window_base && seq_num < window_base + window_size) {
            // Armazenar pacote na janela de recepção
            recv_window[seq_num % window_size].packet = recv_packet;
            recv_window[seq_num % window_size].received = 1;
        }

        // Preparar acknowledgement
        ack_pkt_t ack;
        ack.seq_num = htonl(window_base);

        // Criar máscara de ACKs seletivos
        ack.selective_acks = 0;
        for (int i = 0; i < window_size; i++) {
            int check_seq = window_base + i + 1;
            if (recv_window[check_seq % window_size].received) {
                ack.selective_acks |= (1 << i);
            }
        }

        // Enviar acknowledgement
        if (sendto(sockfd, &ack, sizeof(ack_pkt_t), 0, 
                   (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Erro ao enviar acknowledgement");
        }

        // Avançar a janela de recepção
        while (recv_window[window_base % window_size].received) {
            // Escrever pacote no ficheiro
            size_t data_size = (recv_size == sizeof(data_pkt_t)) ? 1000 : recv_size - sizeof(uint32_t);
            fseek(output_file, (window_base - 1) * 1000, SEEK_SET);
            fwrite(recv_window[window_base % window_size].packet.data, 1, data_size, output_file);

            // Marcar pacote como processado
            recv_window[window_base % window_size].received = 0;
            window_base++;

            // Verificar se o ficheiro está completo (pacote final com menos de 1000 bytes)
            if (data_size < 1000) {
                file_completed = 1;
                break;
            }
        }
    }

    fclose(output_file);
    close(sockfd);
    return 0;
}
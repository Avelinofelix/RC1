#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include "packet_format.h"

#define MAX_WINDOW_SIZE 32
#define TIMEOUT_SECONDS 1

typedef struct {
    data_pkt_t packet;
    int sent;
    int acknowledged;
} window_packet_t;

void usage() {
    fprintf(stderr, "Uso: ./file-sender <ficheiro> <host> <porta> <tamanho_janela>\n");
    exit(-1);
}

int main(int argc, char *argv[]) {
    if (argc != 5) usage();

    char *filename = argv[1];
    char *hostname = argv[2];
    int port = atoi(argv[3]);
    int window_size = atoi(argv[4]);

    if (window_size < 1 || window_size > MAX_WINDOW_SIZE) {
        fprintf(stderr, "Tamanho da janela inválido. Deve estar entre 1 e %d\n", MAX_WINDOW_SIZE);
        exit(-1);
    }

    // Abrir ficheiro para leitura
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir ficheiro");
        exit(-1);
    }

    // Configurar socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro na criação do socket");
        exit(-1);
    }

    struct hostent *server = gethostbyname(hostname);
    if (!server) {
        herror("Erro ao resolver hostname");
        exit(-1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Configurar timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SECONDS;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Inicializar janela de envio
    window_packet_t send_window[MAX_WINDOW_SIZE];
    memset(send_window, 0, sizeof(send_window));

    int window_base = 0;
    int next_seq_num = 1;
    int total_sent = 0;
    int timeout_count = 0;

    while (1) {
        // Enviar pacotes dentro da janela
        while (next_seq_num < window_base + window_size && !feof(file)) {
            send_window[next_seq_num % window_size].packet.seq_num = htonl(next_seq_num);
            size_t bytes_read = fread(send_window[next_seq_num % window_size].packet.data, 1, 1000, file);

            if (sendto(sockfd, &send_window[next_seq_num % window_size].packet, 
                       sizeof(data_pkt_t), 0, 
                       (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Erro ao enviar pacote");
                exit(-1);
            }

            send_window[next_seq_num % window_size].sent = 1;
            send_window[next_seq_num % window_size].acknowledged = 0;

            next_seq_num++;
            total_sent++;
        }

        // Receber acknowledgements
        ack_pkt_t ack;
        socklen_t server_len = sizeof(server_addr);
        ssize_t recv_size = recvfrom(sockfd, &ack, sizeof(ack_pkt_t), 0, 
                                     (struct sockaddr *)&server_addr, &server_len);

        if (recv_size < 0) {
            // Timeout
            timeout_count++;
            if (timeout_count >= 3) {
                fprintf(stderr, "Erro: 3 timeouts consecutivos\n");
                exit(-1);
            }

            // Retransmitir todos os pacotes não confirmados
            for (int i = window_base; i < next_seq_num; i++) {
                if (!send_window[i % window_size].acknowledged) {
                    if (sendto(sockfd, &send_window[i % window_size].packet, 
                               sizeof(data_pkt_t), 0, 
                               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                        perror("Erro ao retransmitir pacote");
                        exit(-1);
                    }
                }
            }
        } else {
            timeout_count = 0;
            uint32_t base_seq = ntohl(ack.seq_num);
            uint32_t selective_acks = ntohl(ack.selective_acks);

            // Atualizar janela de envio
            while (window_base < base_seq) {
                send_window[window_base % window_size].acknowledged = 1;
                window_base++;
            }

            // Processar ACKs seletivos
            for (int i = 0; i < 32; i++) {
                if (selective_acks & (1 << i)) {
                    int seq_num = base_seq + i + 1;
                    send_window[seq_num % window_size].acknowledged = 1;
                }
            }
        }

        // Verificar se todos os pacotes foram enviados e confirmados
        if (feof(file) && window_base == next_seq_num) {
            break;
        }
    }

    fclose(file);
    close(sockfd);
    return 0;
}
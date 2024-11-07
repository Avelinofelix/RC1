Sistema de Chat Cliente/Servidor

Este projeto implementa um sistema de chat básico usando sockets TCP, desenvolvido como parte do mini-projeto 1 da disciplina RC-I 2024-2025 no ISPTEC.
Feito por: Avelino Félix Adelino

++Características

- Comunicação via sockets TCP
- Suporte para múltiplos clientes (até 1000)
- Servidor single-threaded usando select()
- Identificação de clientes por IP:PORTA
- Notificações de entrada e saída de usuários
- Broadcast de mensagens para todos os clientes

++Requisitos

- GCC (GNU Compiler Collection)
- Make
- Sistema operacional Unix/Linux

++Compilação

Para compilar o projeto, execute:

make

Isto irá gerar dois executáveis:

*chat-server
*chat-client


Execução
Servidor
Para iniciar o servidor:

./chat-server <porta>
Exemplo:

./chat-server 1234

Cliente
Para conectar um cliente:


./chat-client <host> <porta>
Exemplo:
./chat-client localhost 1234


++Limpeza
Para limpar os arquivos compilados:


make clean
Estrutura do Projeto


.
├── chat-server.c      # Código fonte do servidor
├── chat-client.c      # Código fonte do cliente
├── Makefile          # Script de compilação
└── README.md         # Este arquivo

++Protocolo de Mensagens
Entrada de usuário: <IP>:<PORTA> join.\n
Saída de usuário: <IP>:<PORTA> left.\n
Mensagem de chat: <IP>:<PORTA> <mensagem>\n


++Limitações
Tamanho máximo de mensagem: 4096 caracteres
Máximo de 1000 clientes simultâneos
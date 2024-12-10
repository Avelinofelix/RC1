# Projeto de Transferência de Dados Confiável

## Descrição
Este projeto implementa um sistema de transferência de arquivos usando UDP com protocolo ARQ (Automatic Repeat reQuest) para garantir confiabilidade.

## Requisitos
- Vagrant
- VirtualBox
- gcc
- make

## Configuração e Execução

### Usando Vagrant
1. Instale Vagrant e VirtualBox
2. Clone o repositório
3. Execute:
   ```bash
   vagrant up
   vagrant ssh
   ```

### Dentro da VM
1. Navegue até o diretório do projeto
   ```bash
   cd /vagrant/projeto
   ```

2. Compilar o projeto
   ```bash
   make
   ```

3. Executar receptor
   ```bash
   ./file-receiver arquivo_saida.txt 1234 5
   ```

4. Executar transmissor
   ```bash
   ./file-sender arquivo_entrada.txt localhost 1234 5
   ```

## Depuração
- Use `LD_PRELOAD="./log-packets.so"` para injeção de falhas
- Consulte `generate-msc.sh` para geração de gráficos de sequência

## Testes
Execute `./test-submission2.sh` para verificação básica do projeto.

## Autor
Avelino Félix Adelino

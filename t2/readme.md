Lucas Pereira Pacheco - 12543930

## Como Compilar

Para compilar para produção (otimizado e sem informações de debug):
`g++ -O2 -DNDEBUG -o mainProd projeto.cpp`

Para compilar para depuração (com símbolos de debug e sem otimização):
`g++ -g -O0 -o mainDbg projeto.cpp`

---

## Explicação do Projeto

Este projeto implementa um cliente UDP utilizando o protocolo SLOW. A estrutura do código é modular, com funções e structs de nomes autoexplicativos que facilitam o entendimento.

![image](https://github.com/user-attachments/assets/6af53ce6-6605-4ac7-880f-7cffe26a063b)

### Estruturas de Dados

-   **`SlowHeader`**: Define o formato do cabeçalho do protocolo SLOW, incluindo `UUIDv8` para o ID da sessão, flags empacotadas (`MB`, `A/R`, `ACK`, `R`, `C`), `Session TTL` (STTL), números de sequência e reconhecimento (`seqNum`, `ackNum`), tamanho da janela (`window`), e identificadores de fragmentação (`fid`, `fo`). Contém funções *helper* para acessar e modificar as flags e o STTL. Os "números mágicos" em `setSttlValue` são máscaras bit a bit para manipular os 27 bits do STTL.

### Funções Auxiliares

-   **`printSlowHeader`**: Função de depuração que exibe o conteúdo de um `SlowHeader`. É compilada apenas em modo debug (quando `NDEBUG` não está definido).
-   **`generateNilUUID`**: Gera um UUID nulo (todos os bytes zerados), usado no início do *three-way handshake*.
-   **`setupUDP`**: Inicializa o socket UDP necessário para a comunicação SLOW.
-   **`sendSlowPacket`** e **`receiveSlowPacket`**: Funções fundamentais para o envio e recebimento de pacotes SLOW, respectivamente. Elas encapsulam a lógica de serialização/desserialização do cabeçalho e dos dados.
-   **`sendConnect`**, **`handleSetup`** e **`sendData`**: Funções essenciais para a execução do *three-way handshake* inicial.
-   **`sendData`**: Além do handshake, é utilizada continuamente para a comunicação de dados com a central.
-   **`handleAck`**: Processa as mensagens de reconhecimento (ACK) recebidas da central, atualizando o controle de sequência e a janela deslizante. Esta função também gerencia a remontagem de mensagens fragmentadas, utilizando os campos `fid`, `fo` e a flag `MB` do cabeçalho SLOW.
-   **`sendDisconnect`**: Inicia o processo de encerramento da sessão com a central.

### A Função `main`

A função `main` orquestra o fluxo principal do cliente:

1.  Configura o socket UDP.
2.  Executa o *three-way handshake* para estabelecer a conexão com a central.
3.  Entra em um loop para enviar mensagens de texto digitadas pelo usuário para a central, utilizando a lógica de fragmentação e controle de janela. O loop continua até que o usuário digite "sair".
4.  Após o loop ou em caso de falha no envio de dados, envia um pacote de desconexão para a central.
5.  Aguarda o ACK da central para confirmar o encerramento da sessão.

Em caso de falha em qualquer etapa crítica (exceto `sendData` durante o loop de mensagens, que apenas interrompe o envio), a aplicação encerra com código de erro `1`. Se `sendData` falhar, o loop é interrompido e o programa procede para a fase de desconexão.

Lucas Pereira Pacheco - 12543930

Pra buildar pra prod:

`g++ -O2 -DNDEBUG -o mainProd projeto.cpp`

Pra buildar pra debug:

`g++ -g -O0 -o mainDbg main.cpp`


# Explicação do projeto:

Olhando por cima, temos essas funçoes e strcuts com nomes e parâmetros bem auto explicativos:
![image](https://github.com/user-attachments/assets/6af53ce6-6605-4ac7-880f-7cffe26a063b)

### structs

A struct `SlowHeader` possui o formato do Header Slow e algumas funções helpers para getters e setters para as flags e sstl. Dentro da função `setSttlValue` existem alguns números mágicos que são as máscaras para extrair e modificar apenas os 27 bits do `uint8_t` que representam o sttl.

### Funções Auxiliares

- `printSlowHeader` serve apenas para debug e só é executada quando compilado sem a flag NDEBUG.

- `generateNilUUID` gera um UUID nulo para ser usado no connect do three way handshake.

- `setupUDP` abre o socket UDP que vamos usar como base para implementar o SLOW.

- `sendSlowPacket` e a `receivesSlowPacket` enviam e recebem, respectivamente, pacotes slow.

- `sendConnect`, `handleSetup` e `sendData` São utilizada nos three way handshake.

- `sendData` continua sendo usada após o three way handshake para se comunicar com a central, até a `sendDisconnect` ser acionada.

### A função `main`:
- sobe o socket UDP;
- faz o three way handshake;
- envia mensagens para a central até receber a string "sair";
- envia um disconnect para a central;
- espera o ACK da central para confirmar o disconnect.

Se qualquer etapa dessas falhar, exceto a `sendData`, ela dá `return 1`encerrando a aplicação com erro.
Caso a `sendData` falhe, o loop é quebrado e ela segue o fluxo de disconnect sem esperar a string "sair".

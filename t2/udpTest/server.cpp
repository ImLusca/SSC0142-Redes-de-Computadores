#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 7033
#define MAX_BUFFER_SIZE 1472

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    
    // 1. Criar o socket UDP
    // AF_INET para IPv4, SOCK_DGRAM para UDP, 0 para protocolo padrão
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    // Limpar a estrutura do endereço do servidor
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Preencher informações do endereço do servidor
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY; // Ouve em qualquer interface de rede disponível
    servaddr.sin_port = htons(PORT); // Converte a porta para ordem de bytes de rede

    // 2. Vincular o socket ao endereço e porta
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Erro ao vincular socket à porta");
        close(sockfd);
        return 1;
    }

    std::cout << "Servidor UDP ouvindo na porta " << PORT << std::endl;
    std::cout << "Aguardando mensagens..." << std::endl;

    // Buffer para armazenar os dados recebidos
    std::vector<char> buffer(MAX_BUFFER_SIZE);

    while (true) {
        len = sizeof(cliaddr); // Tamanho da estrutura do endereço do cliente

        // 3. Receber dados
        // recvfrom retorna o número de bytes recebidos
        int n_bytes = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                               (struct sockaddr *)&cliaddr, &len);
        if (n_bytes < 0) {
            perror("Erro ao receber dados");
            continue; // Tentar novamente
        }

        // Adicionar um terminador nulo para garantir que seja uma string válida para exibição
        if (n_bytes < MAX_BUFFER_SIZE) {
            buffer[n_bytes] = '\0';
        } else {
            // Se o buffer estiver cheio, o último byte pode não ser um terminador nulo
            // Cuidado ao imprimir, pode truncar ou imprimir lixo
            buffer.back() = '\0';
        }

        std::cout << "Mensagem recebida de "
                  << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port)
                  << " (" << n_bytes << " bytes): " << buffer.data() << std::endl;

        // Opcional: Enviar uma resposta simples ao cliente
        const char *response_msg = "Mensagem recebida!";
        sendto(sockfd, response_msg, strlen(response_msg), 0,
               (const struct sockaddr *)&cliaddr, len);
    }

    // 4. Fechar o socket (este loop é infinito, então não será alcançado normalmente)
    close(sockfd);
    return 0;
}

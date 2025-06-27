#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 7033
#define SERVER_IP "142.93.184.175" //IP de slow.gmelodie.com
#define MAX_BUFFER_SIZE 1472
#define UUIDv8 std::array<uint8_t, 16>

#pragma pack(push, 1)           // Alinhando a struct
struct SlowHeader {
    UUIDv8 sid;                 // 16 bytes

    uint32_t sttl         : 27; // Session TTL (bits 0-26)
    uint32_t flag_connect : 1;  // Bit 27 (C)
    uint32_t flag_revive  : 1;  // Bit 28 (R)
    uint32_t flag_ack     : 1;  // Bit 29 (ACK)
    uint32_t flag_ar      : 1;  // Bit 30 (A/R)
    uint32_t flag_mb      : 1;  // Bit 31 (MB)

    uint32_t flags_and_sttl;    // uint32_t para conter o sttl e as flags empacotadas.

    uint32_t seqnum;            // 4 bytes
    uint32_t acknum;            // 4 bytes

    uint16_t window;            // 2 bytes
    uint8_t fid;                // 1 byte
    uint8_t fo;                 // 1 byte

    void set_sttl_value(uint32_t value) {
        // 0x1F = 0000 0000 0000 0000 0000 0000 0001 1111       -> Preserva as flags e limpa sttl ao realizar AND
        // 0x07FFFFFF = 0000 0111 1111 1111 1111 1111 1111 1111 -> extrai os 27 bits menos significativos do uint32_t ao realizar o AND
        flags_and_sttl = (flags_and_sttl & 0x1F) | ((value & 0x07FFFFFF) << 5); // Joga pra esquerda e da OR para encaixar com as flags
    }

    uint32_t get_sttl_value() const {
        return (flags_and_sttl >> 5); // descarta as flags e retorna os 27 bits de sttl
    }

    void set_flag(uint8_t bit_pos, bool value) {
        if(bit_pos > 4) return;

        if (value) 
            flags_and_sttl |= (1U << bit_pos);
        else 
            flags_and_sttl &= ~(1U << bit_pos);
    }

    bool get_flag(uint8_t bit_pos) const {
        return (flags_and_sttl >> bit_pos) & 1U;
    }

    void set_flag_more_bits(bool value) { set_flag(0, value); }     
    void set_flag_accept_reject(bool value) { set_flag(1, value); } 
    void set_flag_ack(bool value) { set_flag(2, value); }           
    void set_flag_revive(bool value) { set_flag(3, value); }        
    void set_flag_connect(bool value) { set_flag(4, value); }       

    bool get_flag_more_bits() const { return get_flag(0); }
    bool get_flag_accept_reject() const { return get_flag(1); }
    bool get_flag_ack() const { return get_flag(2); }
    bool get_flag_revive() const { return get_flag(3); }
    bool get_flag_connect() const { return get_flag(4); }
};
#pragma pack(pop) // Para alinhar a struct

bool setupUDP(int& sockfd, sockaddr_in& servaddr){

    // Abre socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return false;
    }

    // Limpando serverAddr
    memset(&servaddr, 0, sizeof(servaddr));

    // setando variaveis
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_port = htons(PORT); 

    // Converte o endereço IP do servidor de string para formato binário
    if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("Endereço IP inválido/não suportado");
        close(sockfd);
        return false;
    }

    // Tudo certo
    std::cout << "Cliente UDP pronto para enviar mensagens para " << SERVER_IP << ":" << PORT << std::endl;

    return true;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    if(!setupUDP(sockfd,servaddr)) return 1;

    std::string message;
    std::vector<char> buffer(MAX_BUFFER_SIZE);

    while (true) {
        std::cout << "Digite uma mensagem (ou 'sair' para encerrar): ";
        std::getline(std::cin, message);

        if (message == "sair") break;

        // sendto envia a mensagem para o endereço especificado
        int n_bytes_sent = sendto(sockfd, message.c_str(), message.length(), 0,(const struct sockaddr *)&servaddr, sizeof(servaddr));

        if (n_bytes_sent < 0) {
            perror("Erro ao enviar mensagem");
            continue;
        } 

        std::cout << "Mensagem enviada (" << n_bytes_sent << " bytes)." << std::endl;        
    }

    // 3. Fechar o socket
    close(sockfd);
    return 0;
}

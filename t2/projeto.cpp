#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <random>
#include <iomanip> 
#include <algorithm> 

#define PORT 7033
#define SERVER_IP "142.93.184.175" // IP de slow.gmelodie.com
#define MAX_BUFFER_SIZE 1472

using UUIDv8 = std::array<uint8_t, 16>;

#pragma pack(push, 1)         // evita paddings do compilador
struct SlowHeader {
  UUIDv8 sid;                 // 16 bytes

  // Bit 0 (MB)
  // Bit 1 (A/R)
  // Bit 2 (ACK)
  // Bit 3 (R)
  // Bit 4 (C)
  // Session TTL (bits 5-31)

  uint32_t flagsAndSttl;      // uint32_t para conter o sttl e as flags empacotadas.

  uint32_t seqNum;            // 4 bytes
  uint32_t ackNum;            // 4 bytes

  uint16_t window;            // 2 bytes
  uint8_t fid;                // 1 byte
  uint8_t fo;                 // 1 byte

  void setSttlValue(uint32_t value) {
    // O 0x1F (0000 0000 0000 0000 0000 0000 0001 1111) é usado para mascarar e preservar os 5 bits das flags.
    // O 0x07FFFFFF (0000 0111 1111 1111 1111 1111 1111 1111) é usado para limpar os 5 bits mais significativos do STTL antes do shift e OR.
    flagsAndSttl = (flagsAndSttl & 0x1F) | ((value & 0x07FFFFFF) << 5);
  }

  uint32_t getSttlValue() const {
    return (flagsAndSttl >> 5);
  }

  void setFlag(uint8_t bitPos, bool value) {
    if(bitPos > 4) return;

    if (value) {
      flagsAndSttl |= (1U << bitPos); // Define o bit.
    } else {
      flagsAndSttl &= ~(1U << bitPos); // Limpa o bit.
    }
  }

  bool getFlag(uint8_t bitPos) const {
    return (flagsAndSttl >> bitPos) & 1U;
  }

  // Funções helper para definir as flags
  void setFlagMoreBits(bool value) { setFlag(0, value); }
  void setFlagAcceptReject(bool value) { setFlag(1, value); }
  void setFlagAck(bool value) { setFlag(2, value); }
  void setFlagRevive(bool value) { setFlag(3, value); }
  void setFlagConnect(bool value) { setFlag(4, value); }

  // Funções helper para obter as flags
  bool getFlagMoreBits() const { return getFlag(0); }
  bool getFlagAcceptReject() const { return getFlag(1); }
  bool getFlagAck() const { return getFlag(2); }
  bool getFlagRevive() const { return getFlag(3); }
  bool getFlagConnect() const { return getFlag(4); }
};
#pragma pack(pop) // evita paddings do compilador


void printSlowHeader(const SlowHeader& header, const std::string& prefix = "Header", const std::vector<char>* data = nullptr) {

  #ifdef NDEBUG
    return;
  #endif

  std::cout << "--- " << prefix << " Contents ---" << std::endl;
  std::cout << "  SID: ";
  for (uint8_t byte : header.sid) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  std::cout << std::dec << std::endl; 

  std::cout << "  Flags & STTL: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.flagsAndSttl << std::dec << std::endl;
  std::cout << "    More Bits (MB): " << (header.getFlagMoreBits() ? "true" : "false") << std::endl;
  std::cout << "    Accept/Reject (A/R): " << (header.getFlagAcceptReject() ? "true" : "false") << std::endl;
  std::cout << "    ACK: " << (header.getFlagAck() ? "true" : "false") << std::endl;
  std::cout << "    Revive (R): " << (header.getFlagRevive() ? "true" : "false") << std::endl;
  std::cout << "    Connect (C): " << (header.getFlagConnect() ? "true" : "false") << std::endl;
  std::cout << "    Session TTL (STTL): " << header.getSttlValue() << " ms" << std::endl;

  std::cout << "  Seq Num: " << header.seqNum << std::endl;
  std::cout << "  Ack Num: " << header.ackNum << std::endl;
  std::cout << "  Window: " << header.window << std::endl;
  std::cout << "  FID: " << static_cast<int>(header.fid) << std::endl;
  std::cout << "  FO: " << static_cast<int>(header.fo) << std::endl;

  if (data && !data->empty()) {
    std::cout << "  Tamanho do Payload: " << data->size() << " bytes" << std::endl;
    std::cout << "  Payload (Primeiros 16 bytes hex): ";
    for (size_t i = 0; i < std::min((size_t)16, data->size()); ++i) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>((*data)[i]) << " ";
    }
    std::cout << std::dec << std::endl; 
  } else {
    std::cout << "  Sem Payload." << std::endl;
  }
  std::cout << "--------------------------" << std::endl;
}

UUIDv8 generateNilUUID() {
  UUIDv8 nilUuid;
  nilUuid.fill(0);
  return nilUuid;
}

bool sendSlowPacket(int sockfd, const sockaddr_in& destAddr, const SlowHeader& header, const std::vector<char>& data = {}) {

  std::vector<char> packetBuffer(sizeof(SlowHeader) + data.size());

  memcpy(packetBuffer.data(), &header, sizeof(SlowHeader));
  if (!data.empty())
  {
    memcpy(packetBuffer.data() + sizeof(SlowHeader), data.data(), data.size());
  }

  // Chamar printSlowHeader antes de enviar, passando os dados
  printSlowHeader(header, "Sending Header", &data);

  int nBytesSent = sendto(sockfd, packetBuffer.data(), packetBuffer.size(), 0, (const struct sockaddr*)&destAddr, sizeof(destAddr));

  if (nBytesSent < 0)
  {
    perror("Erro ao enviar pacote SLOW");
    return false;
  }

  std::cout << "Pacote SLOW enviado (" << nBytesSent << " bytes)." << std::endl;
  return true;
}

bool receiveSlowPacket(int sockfd, sockaddr_in& senderAddr, SlowHeader& header, std::vector<char>& dataBuffer) {
  socklen_t len = sizeof(senderAddr);
  std::vector<char> buffer(MAX_BUFFER_SIZE);
  int nBytesReceived = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&senderAddr, &len);

  if (nBytesReceived < 0)
  {
    perror("Erro ao receber pacote SLOW");
    return false;
  }

  if (nBytesReceived < sizeof(SlowHeader)) {
    std::cerr << "Pacote SLOW recebido muito pequeno para conter o cabeçalho." << std::endl;
    return false;
  }

  memcpy(&header, buffer.data(), sizeof(SlowHeader));
  dataBuffer.assign(buffer.begin() + sizeof(SlowHeader), buffer.begin() + nBytesReceived);

  // Chamar printSlowHeader depois de receber, passando os dados
  printSlowHeader(header, "Received Header", &dataBuffer);

  std::cout << "Pacote SLOW recebido (" << nBytesReceived << " bytes)." << std::endl;
  return true;
}

bool setupUDP(int& sockfd, sockaddr_in& servAddr) {
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Erro ao criar socket");
    return false;
  }

  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, SERVER_IP, &servAddr.sin_addr) <= 0) {
    perror("Endereço IP inválido/não suportado");
    close(sockfd);
    return false;
  }

  std::cout << "Cliente UDP pronto para enviar mensagens para " << SERVER_IP << ":" << PORT << std::endl;
  return true;
}

bool sendConnect(int sockfd, const sockaddr_in& servAddr) {
  SlowHeader connectHeader;
  connectHeader.sid = generateNilUUID();               // UUID nulo
  connectHeader.setSttlValue(0);                       // TTL zerado
  connectHeader.setFlagConnect(true);                  // flag de connect true
  connectHeader.seqNum = 0;                            // seqnum zerado
  connectHeader.ackNum = 0;                            // acknum 0
  connectHeader.window = MAX_BUFFER_SIZE - sizeof(SlowHeader);
  connectHeader.fid = 0;                               // Fragment ID 0
  connectHeader.fo = 0;                                // Fragment Offset 0
  // Sem data

  std::cout << "Enviando Connect..." << std::endl;
  return sendSlowPacket(sockfd, servAddr, connectHeader);
}

bool handleSetup(int sockfd, sockaddr_in& centralAddr, SlowHeader& setupHeader, UUIDv8& sessionId, uint32_t& sessionSttl, uint32_t& centralSeqNum) {
  std::vector<char> setupData;
  std::cout << "Aguardando Setup..." << std::endl;

  if (!receiveSlowPacket(sockfd, centralAddr, setupHeader, setupData)) return false;

  if (setupHeader.getFlagAcceptReject()) {

    std::cout << "Conexão aceita pelo Central." << std::endl;
    sessionId = setupHeader.sid;                            // extrai SID
    sessionSttl = setupHeader.getSttlValue();               // extrai TTL
    centralSeqNum = setupHeader.seqNum;                     // extrai seqnum

    std::cout << "ID da sessão: ";
    for (uint8_t byte : sessionId) {
      std::cout << std::hex << (int)byte;
    }

    std::cout << std::dec << std::endl;
    std::cout << "TTL da sessão: " << sessionSttl << " ms" << std::endl;
    std::cout << "seqnum da central: " << centralSeqNum << std::endl;
    return true;

  }

  std::cerr << "Conexão rejeitada pelo Central." << std::endl;
  return false;

}

bool sendData(int sockfd, const sockaddr_in& servAddr, const UUIDv8& sessionId, uint32_t sessionSttl, uint32_t seqNum, uint32_t ackNum, const std::string& payload) {
  SlowHeader dataHeader;
  dataHeader.sid = sessionId;
  dataHeader.setSttlValue(sessionSttl);             // TTL da sessão
  dataHeader.setFlagAck(true);                      // ACK flag true
  dataHeader.setFlagConnect(false);                 // Flag Connect em false
  dataHeader.seqNum = seqNum;                       // seq da sessãp
  dataHeader.ackNum = ackNum;                       // acknum da sessão
  dataHeader.window = MAX_BUFFER_SIZE - sizeof(SlowHeader); // Window size
  dataHeader.fid = 0;                               // fid zerado
  dataHeader.fo = 0;                                // fo zerado
  

  std::vector<char> dataPayload(payload.begin(), payload.end());

  std::cout << "Enviando Data..." << std::endl;
  return sendSlowPacket(sockfd, servAddr, dataHeader, dataPayload);
}

bool handleAck(int sockfd, sockaddr_in& centralAddr, uint32_t expectedAckNum) {
  SlowHeader ackHeader;
  std::vector<char> ackData;
  std::cout << "Aguardando Ack..." << std::endl;

  if (!receiveSlowPacket(sockfd, centralAddr, ackHeader, ackData)) return false;

  if (ackHeader.getFlagAck() && ackHeader.ackNum == expectedAckNum) {
    std::cout << "Ack recebido da Central." << std::endl;
    std::cout << "seqnum da central: " << ackHeader.seqNum << std::endl;
    std::cout << "window da central: " << ackHeader.window << std::endl;
    return true;
  }

  std::cerr << "deu ruim no ACK." << std::endl;
  return false;

}

bool sendDisconnect(int sockfd, const sockaddr_in& servAddr, const UUIDv8& sessionId, uint32_t sessionSttl, uint32_t seqNum, uint32_t ackNum) {
  SlowHeader disconnectHeader;
  disconnectHeader.sid = sessionId;             // UID da sessão
  disconnectHeader.setSttlValue(sessionSttl);   // TTL da sessão
  disconnectHeader.setFlagAck(true);            // Ack true
  disconnectHeader.setFlagConnect(true);        // connect true
  disconnectHeader.setFlagRevive(true);         // revive true
  // Connect e revive true juntos indica solicitação de disconnect
  disconnectHeader.seqNum = seqNum;             // seqnum da sessão
  disconnectHeader.ackNum = ackNum;             // acknum da sessão
  disconnectHeader.window = 0;                  // Window size zerado
  disconnectHeader.fid = 0;                     // fid zerado
  disconnectHeader.fo = 0;                      // fo zerado
  // sem data

  std::cout << "Enviando Disconnect..." << std::endl;
  return sendSlowPacket(sockfd, servAddr, disconnectHeader);
}


bool threeWayHandshake(int sockfd, sockaddr_in servAddr, UUIDv8& sessionId, uint32_t& sessionTtl, uint32_t& lastSeq, uint32_t& peripheralSeqNum, sockaddr_in& centralAddr){

  SlowHeader setupHeader;

  // solicita conexão
  if (!sendConnect(sockfd, servAddr)) {
    perror("Erro ao solicitar conexão");
    return false;
  }

  // espera setup da central
  if (!handleSetup(sockfd, centralAddr, setupHeader, sessionId, sessionTtl, lastSeq)) {
    perror("Erro ao receber setup");
    return false;
  }

  peripheralSeqNum = lastSeq + 1;

  // manda um salve pra central
  std::string initialPayload = "Faaaaala cruzao";
  if (!sendData(sockfd, servAddr, sessionId, sessionTtl, peripheralSeqNum, lastSeq, initialPayload)) {
    perror("Erro ao enviar mensagem");
    return false;
  }

  // espera o ACK da central
  lastSeq = peripheralSeqNum;
  if (!handleAck(sockfd, centralAddr, lastSeq)) {
    perror("Erro ao receber ACK da mensagem");
    return false;
  }

  return true;
}

int main() {
  int sockfd;
  sockaddr_in servAddr;
  sockaddr_in centralAddr;

  if (!setupUDP(sockfd, servAddr)) return 1;


  UUIDv8 currentSessionId;
  uint32_t currentSessionSttl = 0;
  uint32_t peripheralSeqNum = 0;
  uint32_t centralLastSeqNum = 0;


  if(!threeWayHandshake(sockfd,servAddr,currentSessionId,currentSessionSttl,centralLastSeqNum,peripheralSeqNum, centralAddr)){
    close(sockfd);
    return 1;
  }


  std::cout << "Handshake 3-way concluído com sucesso!" << std::endl;

  std::string message;
  while (true) {
    std::cout << "Digite uma mensagem para enviar (ou 'sair' para encerrar a conexão): ";
    std::getline(std::cin, message);

    if (message == "sair") break;

    peripheralSeqNum++;
    if (!sendData(sockfd, servAddr, currentSessionId, currentSessionSttl, peripheralSeqNum, centralLastSeqNum, message)) {
      break;
    }
  }

  // Disconnect pra quando cansar de conversar
  peripheralSeqNum++;
  if (!sendDisconnect(sockfd, servAddr, currentSessionId, currentSessionSttl, peripheralSeqNum, centralLastSeqNum)) {
    close(sockfd);
    return 1;
  }

  // espera Ack do Disconnect
  centralLastSeqNum = peripheralSeqNum;
  if (!handleAck(sockfd, centralAddr, centralLastSeqNum)) {
    close(sockfd);
    return 1;
  }

  std::cout << "Boua noite." << std::endl;

  close(sockfd);
  return 0;
}
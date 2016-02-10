// Internet Protocols course 2016, exercise 2.2 by Eino Virtanen 296665

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L // Newest POSIX definition for everything
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#define DEBUG 1 // Can be set to 0 for normal usage

int PrimarySocket, SecondarySocket;
struct sockaddr_in PrimaryAddress;
char *ReceiveBuffer, *SendBuffer;

int tcpSend(int sockfd, char *data, size_t bytes) {

  size_t Written = write(sockfd, data, bytes);
  int Return = 0;

  if (Written == bytes) {
    Return = 1;
    printf("SENT: |");
  } else {
    printf("Failed to send: |");
  }

  for (int i = 0; i < bytes; i++)
    printf("%c", data[i]);
  printf("|\n");

  return Return;
}

int tcpRead(int sockfd) {

  ReceiveBuffer = malloc(1460); // TCP payload size in most settings
  size_t Read;

  Read = read(sockfd, ReceiveBuffer, 1460);

  if (Read == 0) {
    printf("Failed to read from socket!\n");
    return 0;
  } else {
    printf(" GOT: |");
    for (int i = 0; i < Read; i++)
      printf("%c", ReceiveBuffer[i]);
    printf("|\n");
    return 1;
  }
}

int sendInitData() {

  const char InitData[] = "296665\n2.2-names\n";
  char InitIP[15];

  FILE *fp;
  fp = fopen(".info", "r");
  fscanf(fp, "%s", InitIP);

  if (DEBUG)
    printf("Trying to send init data to the server.\n");

  if ((PrimarySocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Failed to initialize PrimarySocket!\n");
    return 0;
  }

  PrimaryAddress.sin_family = AF_INET;
  PrimaryAddress.sin_port = htons(5000);

  if (inet_pton(AF_INET, InitIP, &PrimaryAddress.sin_addr) <= 0) {
    printf("Inet_pton error for %s!\n", InitIP);
    return 0;
  }

  if (connect(PrimarySocket, (struct sockaddr *) &PrimaryAddress,
      sizeof(PrimaryAddress)) < 0) {
    printf("Connect error!\n");
    return 0;
  }

  if (DEBUG)
    printf("Sending initialization data.\n");

  if (!tcpSend(PrimarySocket, (char *) InitData, sizeof(InitData)))
    return 0;

  return 1;
}

struct addrinfo* getIP(char *DomainName, char *Port,
    struct addrinfo *AddressInfo) {

  struct addrinfo AddressFilter;

  memset(AddressInfo, 0, sizeof(struct addrinfo));
  memset(&AddressFilter, 0, sizeof(struct addrinfo));

  if (DEBUG)
    printf("Trying to get IP of %s:%s.\n", DomainName, Port);

  AddressFilter.ai_family = AF_UNSPEC;
  AddressFilter.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(DomainName, Port, &AddressFilter, &AddressInfo) != 0) {
    printf("Failed to get IP!\n");
    return NULL;
  }

  return AddressInfo;
}

struct sockaddr_in getOwnIP(int Socket, struct sockaddr_in LocalAddress) {

  memset(&LocalAddress, 0, sizeof(struct sockaddr_in));

  socklen_t LocalAddressSize = sizeof(LocalAddress);

  if (getsockname(Socket, (struct sockaddr *) &LocalAddress, &LocalAddressSize)
      < 0) {
    printf("Error getting local address.\n");
  }

  return LocalAddress;
}

struct sockaddr_in6 getOwnIP6(int Socket, struct sockaddr_in6 Local6Address) {

  memset(&Local6Address, 0, sizeof(struct sockaddr_in6));

  socklen_t LocalAddressSize = sizeof(Local6Address);

  if (getsockname(Socket, (struct sockaddr *) &Local6Address, &LocalAddressSize)
      < 0) {
    printf("Error getting local address.\n");
  }

  return Local6Address;
}

// Course material as basis
struct addrinfo *tcpConnect(struct addrinfo *AddressInfo) {

  int NthAddress = 1;

  do {
    if (DEBUG)
      printf("Trying to connect to the %d. address.\n", NthAddress++);

    SecondarySocket = socket(AddressInfo->ai_family, AddressInfo->ai_socktype,
        AddressInfo->ai_protocol);

    if (SecondarySocket < 0)
      printf("Failed to open SecondarySocket!\n");

    if (SecondarySocket < 0)
      continue;

    if (connect(SecondarySocket, AddressInfo->ai_addr, AddressInfo->ai_addrlen)
        == 0)
      break;

    printf("Connect failed.\n");

    close(SecondarySocket);

  } while ((AddressInfo = AddressInfo->ai_next) != NULL);

  if (SecondarySocket < 0) {
    printf("Failed to find IP to open a socket to.\n");
    return NULL;
  } else {
    printf("Succesfully connected to an IP address.\n");
    return AddressInfo;
  }

}

int main() {

  char *DomainName, *Port, *FirstSpacePosition, *SecondSpacePosition,
      *NewLinePosition, LocalAddressString[100];
  struct addrinfo *AddressInfo = malloc(sizeof(struct addrinfo)),
      *AddressInfoPoller;
  struct sockaddr_in LocalAddress;
  struct sockaddr_in6 Local6Address;

  printf("Running exercise 2.2 by Eino.\n");

  if (DEBUG)
    printf("Debug mode on.\n");

  if (!sendInitData())
    return 0;

  while (1) {

    memset(LocalAddressString, '\0', 100);
    memset(&LocalAddress, 0, sizeof(struct sockaddr_in));
    memset(&Local6Address, 0, sizeof(struct sockaddr_in6));

    if (!tcpRead(PrimarySocket))
      return 0;

    FirstSpacePosition = strchr(ReceiveBuffer, ' ');
    SecondSpacePosition = strchr(FirstSpacePosition + 1, ' ');
    NewLinePosition = strchr(SecondSpacePosition, '\n');

    if (DEBUG)
      printf(
          "ReceiveBuffer == %p\nFirstSpacePosition == %p\nSecondSpacePosition == %p\nNewLinePosition == %p\n",
          ReceiveBuffer, FirstSpacePosition, SecondSpacePosition,
          NewLinePosition);

    if (!DomainName) {
      DomainName = malloc(
          sizeof(char) * (SecondSpacePosition - FirstSpacePosition));
    } else {
      DomainName = realloc(DomainName,
          sizeof(char) * (SecondSpacePosition - FirstSpacePosition));
    }
    memset(DomainName, '\0', SecondSpacePosition - FirstSpacePosition);
    memcpy(DomainName, FirstSpacePosition + 1,
        SecondSpacePosition - FirstSpacePosition - 1);

    if (!Port) {
      Port = malloc(sizeof(char) * (NewLinePosition - SecondSpacePosition));
    } else {
      Port = realloc(Port,
          sizeof(char) * (NewLinePosition - SecondSpacePosition));
    }
    memset(Port, '\0', NewLinePosition - SecondSpacePosition);
    memcpy(Port, SecondSpacePosition + 1,
        NewLinePosition - SecondSpacePosition - 1);

    if (DEBUG)
      printf("Calling getIP with %s:%s\n", DomainName, Port);

    AddressInfo = getIP(DomainName, Port, AddressInfo);

    if (!AddressInfo)
      return 0;

    AddressInfoPoller = tcpConnect(AddressInfo);

    if (!AddressInfoPoller)
      return 0;

    SendBuffer = malloc(1024);
    SendBuffer = memset(SendBuffer, '\0', 1024);

    if (AddressInfoPoller->ai_family == AF_INET) {

      printf("AddressInfo->ai_family is IPv4.\n");
      LocalAddress = getOwnIP(SecondarySocket, LocalAddress);

      inet_ntop(AF_INET, &LocalAddress.sin_addr, LocalAddressString,
          sizeof(LocalAddressString));
      sprintf(SendBuffer, "ADDR %s %d 296665\n", LocalAddressString,
          ntohs(LocalAddress.sin_port));

    } else {
      printf("AddressInfo->ai_family is IPv6.\n");
      Local6Address = getOwnIP6(SecondarySocket, Local6Address);

      inet_ntop(AF_INET6, &Local6Address.sin6_addr, LocalAddressString,
          sizeof(LocalAddressString));
      sprintf(SendBuffer, "ADDR %s %d 296665\n", LocalAddressString,
          ntohs(Local6Address.sin6_port));
    }

    printf("SendBuffer == %s, strlen(SendBuffer) == %zu\n", SendBuffer,
        strlen(SendBuffer));

    tcpSend(SecondarySocket, SendBuffer, strlen(SendBuffer));

    freeaddrinfo(AddressInfo);

  }

  return 0;
}

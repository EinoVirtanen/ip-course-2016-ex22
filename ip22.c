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
unsigned char *ReceiveBuffer;

int tcpSend(int sockfd, unsigned char *data, size_t bytes) {

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

  free(ReceiveBuffer);
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
  const char InitIP[] = "195.148.124.76";

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

  if (!tcpSend(PrimarySocket, (unsigned char *) InitData, sizeof(InitData)))
    return 0;

  return 1;
}

int getIP(unsigned char *DomainName, unsigned char *Port,
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
    return 0;
  }

  printf("AddressInfo->ai_next == %p\n", AddressInfo->ai_next);
  /*
   if (DEBUG) {
   printf("Got IPs: ");
   for(struct addrinfo *AddressPointer = AddressInfo; AddressPointer != NULL; AddressPointer = AddressPointer->ai_next) {
   printf("%d, ", AddressPointer->ai_addr->sa_data);
   }
   printf("\n");
   }
   */
  return 1;
}

int getOwnIP(int Socket, struct sockaddr_in LocalAddress) {

  memset(&LocalAddress, 0, sizeof(LocalAddress));

  size_t LocalAddressSize = sizeof(LocalAddress);

  if (getsockname(Socket, (struct sockaddr *) &LocalAddress, &LocalAddressSize)
      < 0) {
    printf("Error getting local address.\n");
    return 0;
  }

  return 1;
}

// Course material as basis
int tcpConnect(struct addrinfo *AddressInfo) {

  printf("AddressInfo->ai_next == %p\n", AddressInfo->ai_next);
  int NthAddress = 1;

  do {
    if (DEBUG)
      printf("Trying to connect to the %d. address.\n", NthAddress++);

    if (DEBUG)
      printf(
          "AddressInfo->ai_next == %p (Why is this (nil) if there seems to be 2 addresses?)\n",
          AddressInfo->ai_next);

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
    return 0;
  } else {
    printf("Succesfully connected to an IP address.\n");
    return 1;
  }

}

int main() {

  unsigned char *DomainName, *Port, *FirstSpacePosition, *SecondSpacePosition,
      *NewLinePosition;
  struct addrinfo *AddressInfo = malloc(sizeof(struct addrinfo));
  struct sockaddr_in LocalAddress;

  printf("Running exercise 2.2 by Eino.\n");

  if (DEBUG)
    printf("Debug mode on.\n");

  if (!sendInitData())
    return 0;

  if (!tcpRead(PrimarySocket))
    return 0;

  FirstSpacePosition = strchr(ReceiveBuffer, ' ');
  SecondSpacePosition = strchr(FirstSpacePosition + 1, ' ');
  NewLinePosition = strchr(SecondSpacePosition, '\n');

  if (DEBUG)
    printf(
        "ReceiveBuffer == %d\nFirstSpacePosition == %d\nSecondSpacePosition == %d\nNewLinePosition == %d\n",
        (int) ReceiveBuffer, (int) FirstSpacePosition,
        (int) SecondSpacePosition, (int) NewLinePosition);

  DomainName = malloc(
      sizeof(char) * (SecondSpacePosition - FirstSpacePosition - 1));
  memcpy(DomainName, FirstSpacePosition + 1,
      SecondSpacePosition - FirstSpacePosition - 1);

  Port = malloc(sizeof(char) * (NewLinePosition - SecondSpacePosition - 1));
  memcpy(Port, SecondSpacePosition + 1,
      NewLinePosition - SecondSpacePosition - 1);

  if (!getIP(DomainName, Port, AddressInfo))
    return 0;

  printf("AddressInfo->ai_next == %p\n", AddressInfo->ai_next);

  if (!tcpConnect(AddressInfo))
    return 0;

  return 0;
}

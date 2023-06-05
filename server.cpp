/*
 * Sockets-server - 
 * Simple server program to demonstrate sockets usage
 * CSS 503
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

const int BUFFSIZE = 1500;
const int NUM_CONNECTIONS = 5;
int TOTAL_READS = 0;

struct ThreadArgs {
   int clientSD;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *serviceConnect(void *arg){
   
   ThreadArgs *threadArgs = (ThreadArgs*) arg;
   int clientSD = threadArgs->clientSD;
   delete threadArgs;

   char databuff[BUFFSIZE];
   bzero(databuff, BUFFSIZE);

   //read iterations from client
   int iterations;
   int bytesRead = read(clientSD, &iterations, sizeof(iterations));
   if (bytesRead < 0) {
        cerr << "Read failed" << endl;
        close(clientSD);
        pthread_exit(NULL);
   }

   int numReadCalls = 0;
   for(int i = 0; i < iterations; i++){
      int totalBytes = 0;
      while(totalBytes < BUFFSIZE){
         
         int bytes = read(clientSD, databuff + totalBytes, BUFFSIZE - totalBytes);
         TOTAL_READS++;
         if (bytes < 0)
         {
            cerr << "Read Failed" << endl;
            close(clientSD);
            pthread_exit(NULL);
         }
         totalBytes += bytes;
         numReadCalls++;
      }
   }

    //Send the number of read calls made back
    int bytesWritten = write(clientSD, &numReadCalls, sizeof(numReadCalls));
    if (bytesWritten < 0){

      cerr << "Write Failed" << endl;
      close(clientSD);
      pthread_exit(NULL);
    }

    close(clientSD);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int serverPort;

    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " port" << endl;
        return -1;
    }

    serverPort = atoi(argv[1]);
    /* 
     * Build address
     */
    sockaddr_in acceptSocketAddress;
    bzero((char *)&acceptSocketAddress, sizeof(acceptSocketAddress));
    acceptSocketAddress.sin_family = AF_INET;
    acceptSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSocketAddress.sin_port = htons(serverPort);

    /*
     *  Open socket and bind
     */
    int serverSD = socket(AF_INET, SOCK_STREAM, 0);
    const int on = 1;
    setsockopt(serverSD, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));
    cout << "Socket #: " << serverSD << endl;
   
    int rc = bind(serverSD, (sockaddr *)&acceptSocketAddress, sizeof(acceptSocketAddress));
    if (rc < 0)
    {
        cerr << "Bind Failed" << endl;
        return -1;
    }

    /*
     *  listen and accept
     */
    listen(serverSD, NUM_CONNECTIONS);       //setting number of pending connections
    cout << "Waiting for connection..." << endl;
    pthread_t threads[NUM_CONNECTIONS];
    int threadCount = 0;
    
    while(true){

      sockaddr_in newSockAddr;
      socklen_t newSockAddrSize = sizeof(newSockAddr);
      int newSD = accept(serverSD, (sockaddr *) &newSockAddr, &newSockAddrSize);
      cout << "Accepted Socket #: " << newSD <<endl;

      ThreadArgs *threadArgs = new ThreadArgs;
      threadArgs->clientSD = newSD;

      //critical section
      pthread_mutex_lock(&mutex);
      
      int tid = pthread_create(&threads[threadCount], NULL, serviceConnect, (void*) threadArgs);
      if(tid != 0){
         cerr << "Error creating thread: " << tid << endl;
         close(newSD);
         delete threadArgs;
         continue;
      }

      threadCount++;
      pthread_mutex_unlock(&mutex);
      //end critical section

      //join threads
      if(threadCount >= NUM_CONNECTIONS){
         for(int i = 0; i < NUM_CONNECTIONS; i++){
            tid = pthread_join(threads[i], NULL);
            if(tid != 0){
               cerr << "Error joining threads" << endl;
            }
         }
         threadCount = 0;
      }
    }

    pthread_mutex_destroy(&mutex);
    close(serverSD);

    return 0;
}

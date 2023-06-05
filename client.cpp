/*
 * Sockets-client - client program to demonstrate sockets usage
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
#include <chrono>

using namespace std;
using namespace std::chrono;

const int BUFFSIZE=1500;

int main(int argc, char *argv[])
{
    char *serverName; //name of server
    char port[6]; //server port
    int repetitions; //repetition of sending a set of data buffers
    int nBufs;     //number of data buffers
    int bufSize;     //size of the buffer
    int type;        //type of transfer scenario

    
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    int clientSD = -1;


    /*
     *  Argument validation
     */
    if (argc != 7)
    {
       cerr << "Usage: " << argv[0] << "serverName" << endl;
       return -1;
    }

    /*
     * Use getaddrinfo() to get addrinfo structure corresponding to serverName / Port
	 * This addrinfo structure has internet address which can be used to create a socket too
     */
    serverName = argv[1];
    strcpy(port, argv[2]);
    repetitions = atoi(argv[3]);
    nBufs = atoi(argv[4]);
    bufSize = atoi(argv[5]);
    type = atoi(argv[6]);
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;					/* Allow IPv4 or IPv6*/
    hints.ai_socktype = SOCK_STREAM;					/* TCP */
    hints.ai_flags = 0;							/* Optional Options*/
    hints.ai_protocol = 0;						/* Allow any protocol*/

    int rc = getaddrinfo(serverName, port, &hints, &result);
    if (rc != 0)
    {
       cerr << "ERROR: " << gai_strerror(rc) << endl;
       exit(EXIT_FAILURE);
    }

    /*
     * Iterate through addresses and connect
     */
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        clientSD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (clientSD == -1)
		{
            continue;
      }
		/*
		* A socket has been successfully created
		*/
        rc = connect(clientSD, rp->ai_addr, rp->ai_addrlen);
        if (rc < 0)
        {
            cerr << "Connection Failed" << endl;
            close(clientSD);
            return -1;
        }
        else	//success
        {
            break;
        }
    }

    if (rp == NULL)
    {
        cerr << "No valid address" << endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Client Socket: " << clientSD << endl;
    }
    freeaddrinfo(result);

    /*
     *  Write and read data over network
     */
    char databuf[nBufs][bufSize];

    //send number of repetitions
    int bytesWritten = write(clientSD, &repetitions, sizeof(int));
    if(bytesWritten < 0)
    {
      cerr << "Write Failed" << endl;
      close(clientSD);
      return -1;
    }

    auto start = high_resolution_clock::now();  
    for (int i = 0; i < repetitions; i++) {

        switch(type) 
        {
         case 1: 

            for(int j = 0; j < nBufs; j++){
               int bytesWritten = write(clientSD, databuf[j], bufSize);
                if(bytesWritten < 0)
               {
                  cerr << "Write Failed" << endl;
                  close(clientSD);
                  return -1;
               }
            }
            break;

         case 2:{
            struct iovec vector[nBufs];
            for (int j = 0; j < nBufs; j++) 
            {
               vector[j].iov_base = databuf[j];
               vector[j].iov_len = bufSize;
            }

            int bytesWritten = writev(clientSD, vector, nBufs);
            if(bytesWritten < 0)
            {
                  cerr << "Write Failed" << endl;
                  close(clientSD);
                  return -1;
            }
            break;
         }

         case 3: 
            int bytesWritten = write(clientSD, databuf, nBufs * bufSize);
            if(bytesWritten < 0)
            {
               cerr << "Write Failed" << endl;
               close(clientSD);
               return -1;
            }
            break;
      }
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    int numReads;
    read(clientSD, &numReads, sizeof(int));
    double throughput = (nBufs * bufSize * 8.0) / (duration / 1000000.0); // Calculate throughput in Gbps

    cout << "Test (" << type << "): time = " << duration << " usec, #reads = " 
         << numReads << ", throughput " << throughput << " Gbps" << endl;
    

    close(clientSD);
    return 0;
}

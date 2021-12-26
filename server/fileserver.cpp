#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include <ctype.h>

#include "../include/filexfer.h"

void* SocketHandler(void*);
void* TCPfiled(void* p);

int complete_send(int sock, uint8_t* buffer, int size )
{
    int sent = 0;

    while( sent < size )
    {
        int count = send(sock, buffer + sent , size - sent, 0 );

        if (count <0 )
        {
                printf("send failed %d\r\n", count);
                return 0;
        }

        sent += count;
    }

    return sent;
}

int complete_receive(int sock, uint8_t* buffer, int size)
{
    int receive  = 0;
        while( receive < size )
        {
                int count = recv(sock, buffer + receive, size - receive, 0 );

                if (count <0 )
                {
                        printf("recv failed %d\r\n", count);
                        return 0;
                }

		printf("count = %d\r\n", count);
                receive += count;
        }

        return receive;
}

// LinServer -p port-number
int main(int argc, char** argv)
{
    int port_number= 30000;
    int c;

    while(( c= getopt(argc, argv, "p:" )) != -1)
    {
        switch(c)
        {
            case 'p':
                port_number = atoi( optarg);
                break;
            default:
                printf("Unknown option %c\r\n", c);
                break;
        }
    }

    struct sockaddr_in my_addr;

    int hsock;
    int * p_int ;
    int err;

    socklen_t addr_size = 0;
    int* csock;
    sockaddr_in sadr;
    pthread_t thread_id=0;

    hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock == -1){
        printf("Error initializing socket %d\n", errno);
        goto FINISH;
    }
    
    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;
        
    if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
        (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
        printf("Error setting options %d\n", errno);
        free(p_int);
        goto FINISH;
    }
    free(p_int);

    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(port_number);
    
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY ;
    
    if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
        fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n", errno);
        goto FINISH;
    }
    if(listen( hsock, 10) == -1 ){
        fprintf(stderr, "Error listening %d\n",errno);
        goto FINISH;
    }

    //Now lets do the server stuff
    printf("Listen on port %d\r\n", port_number);

    addr_size = sizeof(sockaddr_in);
    
    while(true)
	{
        printf("waiting for a connection\n");
        csock = (int*)malloc(sizeof(int));
        if((*csock = accept( hsock, (sockaddr*)&sadr, &addr_size))!= -1)
        {
            printf("---------\nReceived connection from %s\n", inet_ntoa(sadr.sin_addr));
//            pthread_create(&thread_id,0,&SocketHandler, (void*)csock );
            pthread_create(&thread_id,0,&TCPfiled, (void*)csock );

            pthread_detach(thread_id);
        }
        else{
            fprintf(stderr, "Error accepting %d\n", errno);
        }

    }
    
FINISH:
;
}

void* SocketHandler(void* lp)
{
    int *csock = (int*)lp;

    char buffer[1024];
    int buffer_len = 1024;
    int bytecount;

    memset(buffer, 0, buffer_len);
    if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1)
    {
        fprintf(stderr, "Error receiving data %d\n", errno);
        goto FINISH;
    }

    printf("received bytes %d\nReceived string \"%s\"", bytecount, buffer);
    strcat(buffer,"SERVER ECHO" );

    if((bytecount = send(*csock, buffer, strlen(buffer), 0))== -1){
        fprintf(stderr,"rror sending data %d\n", errno);
        goto FINISH;
    }
    
    printf("Sent bytes %d\n", bytecount);

FINISH:
    free(csock);
    return 0;
}


void* TCPfiled(void* lp)
{

    int *csock = (int*)lp;
    int fd = *csock;
    int     cc;
    free(csock);

    struct filexfer filexfer;
	

    int headersize = complete_receive(fd,(uint8_t*) &filexfer, sizeof (struct filexfer)) ;
    if ( headersize != sizeof (struct filexfer) )
    {
        printf("Receive header info count failed %d\r\n", headersize);          
        return 0;
    }

    if ( FILEXFER_PREFIX != ntohl(filexfer.prefix))
    {
        printf("Prefix not match %08x %08x\r\n", FILEXFER_PREFIX, ntohl(filexfer.prefix) );             
        return 0;
    }

    printf("[T] receive file %s\r\n",  filexfer.filename);
    printf("[T] receive size %d\r\n",  ntohl(filexfer.filelength));
	
    FILE * pFile = fopen( filexfer.filename,"w" );

    if( NULL == pFile )
    {
        printf( "Open File Failure %s\r\n", filexfer.filename);
        return 0;
    }


    uint32_t filesize = ntohl(filexfer.filelength);

    if ( filesize > FILEXFER_MAXSIZE)
    {
        printf("filelength too big  %08x\r\n",  filesize );             
        return 0;
    }

    uint8_t * buffer = (uint8_t *) malloc( filesize );
    memset(buffer, 0, filesize);
    uint32_t received = 0;

#if 0
    while( received < filesize )
    {
        int frame = filesize - received > 4096 ? 4096 : filesize - received;
        int count = read(fd, buffer + received, frame);

        if ( received == 0 && count > 0 )
            printf("buffer= %02x,%02x,\r\n", buffer[0], buffer[1]);

        if ( count <= 0)
            break;

        received += count;
    }
#else

	 received = complete_receive(fd, buffer,  filesize);
	 
   
#endif

    printf("Received %d\r\n", received);
        
    fwrite(buffer,1, received ,pFile);
    fclose(pFile);

    free( buffer);

	char ACK[4] = "BYE";
	complete_send(fd,(uint8_t*) &ACK[0], 4);
    printf("DONE\r\n");
        
    return 0;
}



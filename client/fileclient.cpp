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

#include <sys/stat.h>
#include <ctype.h>

#include "../include/filexfer.h"

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

		receive += count;
	}

	return receive;
}



int main(int argc, char** argv)
{

//    int host_port= 1101;
//    char* host_name= "127.0.0.1";
    int port_number = 30000;
    const char* host_name= "192.168.251.85";
    const char* file_name= "client";
    int c;

    while(( c = getopt(argc, argv, "p:h:f:")) != -1)
    {
        switch(c)
        {
            case 'p':
                port_number = atoi( optarg);
                break;
            case 'h':
                host_name = optarg;
                break;
            case 'f':
                file_name = optarg;
                break;
            default:
                printf("Unknown option %c\r\n",c);
                break;
        }
    }

    struct sockaddr_in my_addr;

    int bytecount;
    int buffer_len=0;

    int hsock;
    int * p_int;
    int err;

    hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock == -1){
        printf("Error initializing socket %d\n",errno);
        
//        goto FINISH;
        return 3;
    }
    
    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;
        
    if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
        (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
        printf("Error setting options %d\n",errno);
        free(p_int);
        return 2;
//        goto FINISH;
    }

    free(p_int);

    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(port_number);
    
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = inet_addr(host_name);

    if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
    {
        if((err = errno) != EINPROGRESS)
        {
            fprintf(stderr, "Error connecting socket %d\n", errno);
            close(hsock);
            return 1;
//            goto FINISH;
            
        }
    }

    //Now lets do the client related stuff
#if 0

    char buffer[1024];

    buffer_len = 1024;

    memset(buffer, '\0', buffer_len);

    printf("Enter some text to send to the server (press enter)\n");
    fgets(buffer, 1024, stdin);
    buffer[strlen(buffer)-1]='\0';
    
    if( (bytecount=send(hsock, buffer, strlen(buffer),0))== -1){
        fprintf(stderr, "Error sending data %d\n", errno);
        goto FINISH;
    }
    printf("Sent bytes %d\n", bytecount);

    if((bytecount = recv(hsock, buffer, buffer_len, 0))== -1){
        fprintf(stderr, "Error receiving data %d\n", errno);
        goto FINISH;
    }
    printf("Recieved bytes %d\nReceived string \"%s\"\n", bytecount, buffer);

    close(hsock);
#else
    struct filexfer filexfer;
        
    memset(&filexfer,0, sizeof(struct filexfer));

    struct stat st;
    stat(file_name, &st);
    int size = st.st_size;

    filexfer.prefix = htonl( FILEXFER_PREFIX); 
    strcpy( filexfer.filename, file_name);			
     filexfer.filelength = htonl( (int) st.st_size);	

    if ( size == 0 )
    {
        fprintf(stderr, "Open File Error %s, %d\n",file_name, errno);
        close(hsock);
        return 1;
    }


    if( (bytecount=send(hsock,&filexfer,sizeof(struct filexfer ) ,0))== -1)
    {
        fprintf(stderr, "Error sending data %d\n", errno);
	return 0;		
//        goto FINISH;
    }



                 
    uint8_t* buffer = (uint8_t*) malloc(size);

    FILE* fd = fopen(file_name, "r");
    fread(buffer, 1, size, fd);
    fclose(fd);


#if 0    

    int sent = complete_send(hsock, buffer,size);
	
    if (  sent != size )
	{
		printf("Send Failed, %d != %d \r\n", sent, size);
	}
	

   char ack[4];
   memset(ack, 0, sizeof(ack));
   int nack = 	complete_receive(hsock,(uint8_t*) & ack[0] , 4);

   printf( "nACK = %d\r\n", nack);
   printf( "ack  = %s\r\n", ack);

   if (( nack != 4 ) || (strcmp(ack, "BYE") != 0 ))
   {
	    printf("Sent failed\n");
   }
   else
   {
	    printf("Sent success\n");

   }
#else

	int filesize = size; 
   int sent = 0;	

	while( sent < filesize )
	{
		int data_to_send = filesize - sent ;
		int block_to_send = MIN(FILEXFER_MAX_BLOCK, data_to_send);
		int data_sent = complete_send(hsock, buffer +sent, block_to_send);

		if ( data_sent <= 0 )
		{
			printf("Receive Failed\r\n");
			free(buffer);
			return 0;
		}	

		sent +=  data_sent;
		char ack[4];
		memset(ack, 0, sizeof(ack));
		int nack = 	complete_receive(hsock,(uint8_t*) & ack[0] , 4);

		if ( nack == 4 )
		{
			if ( strcmp(ack, "BYE") == 0 )
			{
				if (  sent == filesize)
					printf("Sent compolete\n");
				else
					printf("Sent not compolete\n");
				break;
			}
			if ( strcmp(ack, "ACK") == 0 )
			{
				printf("More data to send \n");
				continue;		
			}
		}

		printf( "ACK Failed= %d,%s\r\n", nack, ack);
		break;
   }
#endif

	free( buffer);
    close(hsock);

#endif    
FINISH:
    ;
}

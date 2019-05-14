//  UDPingerClinet.C
//  Samuel Fu, 5/13/19
//  CS176A_hw3
//  Reference:
//  UPD Client Skeleton - https://github.com/Shippo7/UDP-Pinger/blob/master/udp_sender.c

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define TIMEOUT 60
#define BUFSIZE 256

int main(int argc, char **argv){
    struct sockaddr_in myaddr;	//my address
    struct sockaddr_in remaddr;	//remote address
    socklen_t slen=sizeof(remaddr);
    int fd; //my socket
    int port; //port number
    char buf[BUFSIZE]; //receive buffer
    int times; //number of messages send
    char host_to_contact[50];
    struct hostent *hp, *gethostbyname();
    char ip[100];
    struct hostent *he;
    struct in_addr **addr_list;
    struct timeval starttime, endtime;//init the clock
    struct timeval timeout={TIMEOUT,0}; //set timeout
    int i;
    
    /* Get host name, port number, loop times */
    if(argc==3){
        strncpy(host_to_contact, argv[1], sizeof(host_to_contact));
        port=atoi(argv[2]);
        times = 10;
    }
    else{
        printf("Usage: %s <host name> <port number> \n", argv[0]);
        exit(1);
    }
    
    /* Create a UDP socket */
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        exit(1);
    }
    
    /* Get IP address from Hostname */
    
    he = gethostbyname(host_to_contact);
    
    addr_list = (struct in_addr **) he->h_addr_list;
    
    for(i = 0; addr_list[i] != NULL; i++)
    {
        strcpy(ip , inet_ntoa(*addr_list[i]) );
    }
    
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);
    
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    
    memset((char *) &remaddr, 0, sizeof(remaddr));
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(port);
    if (inet_aton(ip, &remaddr.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    
    /* Set receive UDP message timeout */
    
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
    
    /* Send the messages and calculate RTT */
    int loss_count = 0;
    double min = -1;
    double max = -1;
    double total = 0.0;

    for (i=1; i < times+1; i++) {
        /* form the message and send it */

        // char send_buf[] = "ping";
      
        char pingString[256];
        char timeString[256];
        char buffer[256];

        time_t rawtime;
        struct tm * timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        sprintf(pingString, "PING %d ", i);
        strftime(timeString, sizeof(timeString), "%H:%M:%S", timeinfo);
        strcpy(buffer, pingString);
        strcat(buffer, timeString);

        if (sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&remaddr, slen)==-1) {
            perror("sendto");
            continue;
        }

        
        /* Waiting message come back */
        gettimeofday(&starttime,0);
        int recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &slen);
        if (recvlen >= 0) {
            buf[recvlen] = 0;
            gettimeofday(&endtime,0);

            double timeuse = 1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec;
            timeuse /=1000;

            if(min == -1){
                min = timeuse;
            }
            if(max == -1){
                max = timeuse;
            }
            if(min > timeuse){
                min = timeuse;
            }
            if(max < timeuse){
                max = timeuse;
            }
            total += timeuse;

            printf("PING received from %s: ", argv[1]);
            printf("seq#=%i ", i);
            printf("time=%.*f ms\n", 3, timeuse);
            bzero(buffer, 256);
        }
        else{
            printf("Timeout!!!!!!!\n");
            loss_count += 1;
            continue;
        }
    }

    int received = 10 - loss_count;
    double percent = (loss_count / 10.0) * 100.0;
    printf("--- ping statistics ---\n");
    printf("10 packets transmitted, %i packets received, %.*f%% packet loss\n", received, 0, percent);
    double avg = 0;
    if(received != 0) {
        avg = total / received;
    }
    printf("round-trip min/avg/max = %.*f/%.*f/%.*f ms\n", 3, min, 3, avg, 3, max);


    close(fd);

    return 0;
}
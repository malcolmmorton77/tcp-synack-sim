/*Name:    Malcolm M
 *Section: 3530.002
 *Date:    4/19/2022
 *Descr:   server side of tcp segment project
 *Note:    only works on linux
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdbool.h>

#define MSS 512

int main(){
    char portno[7];
    int numport;
    int listenfd;
    struct sockaddr_in servaddr;
    int *serversize = (socklen_t*) sizeof(servaddr);

    unsigned short int tempdest;
    unsigned short int tempsource;
    unsigned int sack;

    struct tcpsegment{ //define the same tcpsegment structure on the server side
        unsigned short int sourceport;
        unsigned short int destport;
        unsigned int seqnum;
        unsigned int acknum;
        unsigned short int headflag;
        unsigned short int rwnd;
        unsigned short int chksum;
        unsigned short int urg;
        char payload[MSS];
    }tcpseg;

    printf("Port: ");
    fflush(stdout);
    scanf("%s", portno);
    numport = atoi(portno);

    //place this before setting all the values
    bzero(&servaddr, sizeof(servaddr));
     
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(numport);
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);

    // creating socket file descriptor with tcp connection
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	    perror("socket creation failed");
	    exit(EXIT_FAILURE);
    }

    //assign servaddr to listenfd socket
    if( bind( listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind error\n");
        exit(EXIT_FAILURE);
    }

    //listen with backlog of 10
    if( listen( listenfd, 10 ) == -1){
        printf("listen error\n");
        exit(EXIT_FAILURE);
    }

    //accept the connection, generating an error if it doesn't work
	if ((listenfd = accept( listenfd, (struct sockaddr*)NULL, NULL )) < 0){ //make sure listenfd is used throughout to generate a connection
		printf("accept error\n");
		exit(EXIT_FAILURE);
	}

    //server accepts the SYN request
    read(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
    //compute checksum for the ACK packet; Credit:Dr. Robin
    unsigned short int cksum_arr[266];
    unsigned int i, sum=0, cksum, wrap, calc_cksum;
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        //printf("Element %3d: 0x%04X\n", i, cksum_arr[i]); removed for cleanliness
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = cksum;
    
    printf("Source Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt

    if(calc_cksum == tcpseg.chksum && tcpseg.headflag == 2){ //requesting connection
        //bzeros the segment and recreates the segment on the server side by flipping the source and dest ports
        sack = tcpseg.seqnum + 1;
        tempsource = tcpseg.sourceport;
        tempdest = tcpseg.destport;
        bzero(&tcpseg, sizeof(tcpseg));
        tcpseg.acknum = sack;
        tcpseg.sourceport = tempdest;
        tcpseg.destport = tempsource;

        //random number for sequence number /*since this is run on linux, I used /dev/random to generate a random number*/
        unsigned int randval;
        FILE *f;
        f = fopen("/dev/random", "r");
        fread(&randval, sizeof(randval), 1, f);
        fclose(f);
        tcpseg.seqnum = abs(randval);

        //set any values that you dont use to zero*******
        bzero(tcpseg.payload, 512); //set the payload of the segment as NULL
        tcpseg.urg = 0;
        tcpseg.rwnd = 0;

        //set flags
        tcpseg.headflag ^= 1UL << 1; //set the SYN bit to 1
        tcpseg.headflag ^= 1UL << 4; //set the ACK bit to 1

        //compute checksum for the packet; Credit:Dr. Robin
        unsigned short int cksum_arr[266];
        unsigned int i, sum=0, cksum, wrap, calc_cksum;
        cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
        tcpseg.chksum = 0;     //set chksum to 0 for calculation
        memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

        for (i=0;i<266;i++)            // Compute sum
        {
            sum = sum + cksum_arr[i];
        }

        wrap = sum >> 16;             // Get the carry
        sum = sum & 0x0000FFFF;       // Clear the carry bits
        sum = wrap + sum;             // Add carry to the sum

        wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
        sum = sum & 0x0000FFFF;
        calc_cksum = wrap + sum;
        calc_cksum = 0xFFFF^calc_cksum;
        tcpseg.chksum = calc_cksum;

        sleep(1);
        write(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg)); //write SYNACK
        printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
    }
    else{
        close(listenfd);
        exit(1);
    }

    //server accepts the ACK segment
    bzero(&tcpseg, sizeof(tcpseg));
    read(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
    printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
    
    //compute checksum for the ACK packet; Credit:Dr. Robin
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    wrap = 0, sum = 0;     //re-initialize wrap and sum
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = cksum;

    if(calc_cksum == cksum){ //if no errors, server checks the flags
        if(tcpseg.headflag == 16){  //if client acked the synack segment, establish connection
            printf("\nConnection Established.\n");
            printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
        }
    }
    else{
        close(listenfd);
        exit(1);
    }

    //server accepts the close request
    bzero(&tcpseg, sizeof(tcpseg));
    read(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
    printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
    
    //compute checksum for the ACK packet; Credit:Dr. Robin
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    wrap = 0, sum = 0;
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = cksum;

    if(calc_cksum == cksum){    //initiate the close requests
        sack = tcpseg.seqnum + 1;
        tempdest = tcpseg.destport;
        tempsource = tcpseg.sourceport;
        bzero(&tcpseg, sizeof(tcpseg));
        tcpseg.destport = tempsource;
        tcpseg.sourceport = tempdest;
        tcpseg.seqnum = 1024;           //set sequence number to 1024
        tcpseg.acknum = sack;           //set acknum to sequence number + 1
        tcpseg.headflag ^= 1UL << 4; //set ackbit to 1

        //set any values that you dont use to zero*******
        bzero(tcpseg.payload, 512); //set the payload of the segment as NULL
        tcpseg.urg = 0;
        tcpseg.rwnd = 0;

        //compute checksum for the ACK packet; Credit:Dr. Robin
        wrap = 0, sum = 0;
        tcpseg.chksum = 0;     //set chksum to 0 for calculation
        memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

        for (i=0;i<266;i++)            // Compute sum
        {
            sum = sum + cksum_arr[i];
        }

        wrap = sum >> 16;             // Get the carry
        sum = sum & 0x0000FFFF;       // Clear the carry bits
        sum = wrap + sum;             // Add carry to the sum

        wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
        sum = sum & 0x0000FFFF;
        calc_cksum = wrap + sum;
        calc_cksum = 0xFFFF^calc_cksum;
        tcpseg.chksum = calc_cksum;

        sleep(1);
        write(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
        printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt

        //send close acknowledgement, keeping everything the same except the header flags
        bzero(&tcpseg.headflag, sizeof(tcpseg.headflag)); //zero the header flags
        tcpseg.headflag ^= 1UL << 0; //set FIN bit to 1

        //compute checksum for the ACK packet; Credit:Dr. Robin
        wrap = 0, sum = 0;
        tcpseg.chksum = 0;     //set chksum to 0 for calculation
        memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

        for (i=0;i<266;i++)            // Compute sum
        {
            sum = sum + cksum_arr[i];
        }

        wrap = sum >> 16;             // Get the carry
        sum = sum & 0x0000FFFF;       // Clear the carry bits
        sum = wrap + sum;             // Add carry to the sum

        wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
        sum = sum & 0x0000FFFF;
        calc_cksum = wrap + sum;
        calc_cksum = 0xFFFF^calc_cksum;
        tcpseg.chksum = calc_cksum;

        sleep(1);
        write(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
        printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
    }
    else{
        close(listenfd);
        exit(1);
    }

    read(listenfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));

    //compute checksum for the ACK packet; Credit:Dr. Robin
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    wrap = 0, sum = 0;
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = cksum;

    if(calc_cksum == cksum && tcpseg.headflag == 16){
        printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
        printf("\nConnection Closed\n");
    }
    else{
        close(listenfd);
        exit(1);
    }

    //set the acknum to received sequence number from client + 1
    //the syn and ack bits need to be set to 1 when returning the packet to client
    //bzero the segment when received from client and create a new one for returning the packet
    close(listenfd);
    return 0;
}
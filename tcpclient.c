/*Name:    Malcolm M
 *Section: 3530.002
 *Date:    4/19/2022
 *Descr:   client side of tcp segment project
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
#include <math.h>

#define MSS 512

int main(){
    char portno[7];
    unsigned short int numport, sockfd = 0;
    struct sockaddr_in servaddr;
    socklen_t serversize = sizeof(servaddr);

    unsigned short int tempsource;
    unsigned short int tempdest;
    unsigned int sack;

    struct tcpsegment{
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

    //create socket for use and error check
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	    printf("socket error");
	    exit(EXIT_FAILURE);
    }

    printf("Port: ");
    scanf("%s", portno);
    numport = atoi(portno);

    bzero(&servaddr, sizeof(servaddr));
     
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(numport);
    inet_pton(AF_INET,"129.120.151.96",&(servaddr.sin_addr));

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
	    printf("connect error\n");
	    exit(EXIT_FAILURE);
    }

    //call getsockname after connect call for source port number
    getsockname(sockfd, (struct sockaddr*) &servaddr, &serversize); //getsockname() returns the size of the socket when no errors
    tcpseg.sourceport = htons(servaddr.sin_port);//populate sourceport with the htons result after getsockname
    
    //destination port is given by the user
    tcpseg.destport = numport;
    //put a zero as ack number because you don't know what the server will respond with when initializing the connection
    tcpseg.acknum = 0;

    //random number for sequence number /*since this is run on linux, I used /dev/random to generate a random number*/
    unsigned int randval;
    FILE *f;
    f = fopen("/dev/random", "r");
    fread(&randval, sizeof(randval), 1, f);
    fclose(f);
    tcpseg.seqnum = abs(randval);

    //initialize header flags to all 0 for use
    tcpseg.headflag = 0;
    tcpseg.headflag ^= 1UL << 1; //set the SYN bit to 1
    //set any values that you dont use to zero*******
    bzero(tcpseg.payload, 512); //set the payload of the segment as NULL
    tcpseg.urg = 0;
    tcpseg.rwnd = 0;

    //compute checksum for the packet; Credit:Dr. Robin
    unsigned short int cksum_arr[266];
    unsigned int i, sum=0, cksum, wrap, calc_cksum;
    tcpseg.chksum = 0;
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bytes

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
    cksum = wrap + sum;

    tcpseg.chksum = 0xFFFF^cksum;
    //print values for checking
    printf("Source Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum);
    
    //write the SYN request to server
    write(sockfd, (struct tcpsegment*)&tcpseg, sizeof(tcpseg));

    //read the response
    bzero(&tcpseg, sizeof(tcpseg));
    read(sockfd, (struct tcpsegment*) &tcpseg, sizeof(tcpseg)); //read the SYNACK segment from server
    printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt of 
    //compute checksum for the ACK packet; Credit:Dr. Robin
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    wrap = 0, sum = 0;     //re-initialize wrap and sum
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        //printf("Element %3d: 0x%04X\n", i, cksum_arr[i]); //removed for cleanliness
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = cksum;          //reassign to cksum   
    
    if(calc_cksum == cksum && tcpseg.headflag == 18){
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
        f = fopen("/dev/urandom", "r"); //changed for quicker simulation
        fread(&randval, sizeof(randval), 1, f);
        fclose(f);
        tcpseg.seqnum = abs(randval); //sometimes the program pauses as it calculates extra bits

        //set any values that you dont use to zero*******
        bzero(tcpseg.payload, 512); //set the payload of the segment as NULL
        tcpseg.urg = 0;
        tcpseg.rwnd = 0;

        //set flags
        tcpseg.headflag ^= 1UL << 4; //set the ACK bit to 1

        //compute checksum for the packet; Credit:Dr. Robin
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
        tcpseg.chksum = calc_cksum;

        sleep(1);                       //sleep before sending
        write(sockfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
        printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum);
    }
    else{
        close(sockfd);
        exit(1);
    }

    //CLOSE
    tempsource = tcpseg.sourceport;
    tempdest = tcpseg.destport;
    bzero(&tcpseg, sizeof(tcpseg));
    tcpseg.sourceport = tempsource;
    tcpseg.destport = tempdest;
    tcpseg.seqnum = 2046;       //assign sequence number to 2046 and acknowledgement to 1024 for close request
    tcpseg.acknum = 1024;
    tcpseg.headflag ^= 1UL << 0;
    //set any values that you dont use to zero*******
    bzero(tcpseg.payload, 512); //set the payload of the segment as NULL
    tcpseg.urg = 0;
    tcpseg.rwnd = 0;

    //compute checksum for the ACK packet; Credit:Dr. Robin
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    wrap = 0, sum = 0;     //re-initialize wrap and sum
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        //printf("Element %3d: 0x%04X\n", i, cksum_arr[i]); //removed for cleanliness
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = calc_cksum;          //reassign to cksum   

    sleep(1);       //directions didn't say to sleep, but I added one to aid in the flow
    write(sockfd, (struct tcpsegment*)&tcpseg, sizeof(tcpseg));
    printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt

    //read the response from server
    bzero(&tcpseg, sizeof(tcpseg));
    read(sockfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg)); //read the close ack
    unsigned int tempack = tcpseg.seqnum + 1;

    printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
    
    //compute checksum for the ACK packet; Credit:Dr. Robin
    cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
    wrap = 0, sum = 0;     //re-initialize wrap and sum
    tcpseg.chksum = 0;     //set chksum to 0 for calculation
    memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

    for (i=0;i<266;i++)            // Compute sum
    {
        //printf("Element %3d: 0x%04X\n", i, cksum_arr[i]); //removed for cleanliness
        sum = sum + cksum_arr[i];
    }

    wrap = sum >> 16;             // Get the carry
    sum = sum & 0x0000FFFF;       // Clear the carry bits
    sum = wrap + sum;             // Add carry to the sum

    wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
    sum = sum & 0x0000FFFF;
    calc_cksum = wrap + sum;
    calc_cksum = 0xFFFF^calc_cksum;
    tcpseg.chksum = calc_cksum;          //reassign to cksum

    if(calc_cksum == cksum && tcpseg.headflag == 16){
        read(sockfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
        //compute checksum for the ACK packet; Credit:Dr. Robin
        cksum = tcpseg.chksum; //temp variable to hold the current checksum for comparison later
        wrap = 0, sum = 0;     //re-initialize wrap and sum
        tcpseg.chksum = 0;     //set chksum to 0 for calculation
        memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

        for (i=0;i<266;i++)            // Compute sum
        {
            //printf("Element %3d: 0x%04X\n", i, cksum_arr[i]); //removed for cleanliness
            sum = sum + cksum_arr[i];
        }

        wrap = sum >> 16;             // Get the carry
        sum = sum & 0x0000FFFF;       // Clear the carry bits
        sum = wrap + sum;             // Add carry to the sum

        wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
        sum = sum & 0x0000FFFF;
        calc_cksum = wrap + sum;
        calc_cksum = 0xFFFF^calc_cksum;
        tcpseg.chksum = cksum;          //reassign to cksum
        if(calc_cksum == cksum && tcpseg.headflag == 1){
            printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
            tempsource = tcpseg.sourceport;
            tempdest = tcpseg.destport;
            sack = tcpseg.seqnum + 1;
            bzero(&tcpseg.seqnum, sizeof(tcpseg.seqnum));
            bzero(&tcpseg.acknum, sizeof(tcpseg.acknum));
            tcpseg.seqnum = sack;
            tcpseg.acknum = tempack;
            bzero(&tcpseg.headflag, sizeof(tcpseg.headflag));
            tcpseg.headflag ^= 1UL << 4; //set ACK bit to 1

            //compute checksum for the ACK packet; Credit:Dr. Robin
            wrap = 0, sum = 0;     //re-initialize wrap and sum
            tcpseg.chksum = 0;     //set chksum to 0 for calculation
            memcpy(cksum_arr, &tcpseg, 532); // Copying 532 bits into cksum_arr

            for (i=0;i<266;i++)            // Compute sum
            {
                //printf("Element %3d: 0x%04X\n", i, cksum_arr[i]); //removed for cleanliness
                sum = sum + cksum_arr[i];
            }

            wrap = sum >> 16;             // Get the carry
            sum = sum & 0x0000FFFF;       // Clear the carry bits
            sum = wrap + sum;             // Add carry to the sum

            wrap = sum >> 16;             // Wrap around once more as the previous sum could have generated a carry
            sum = sum & 0x0000FFFF;
            calc_cksum = wrap + sum;
            calc_cksum = 0xFFFF^calc_cksum;
            tcpseg.chksum = calc_cksum;          //reassign to cksum

            sleep(1);
            write(sockfd, (struct tcpsegment *)&tcpseg, sizeof(tcpseg));
            printf("\nSource Port: %d; Destination Port: %d\nSequence number: %d; Acknowledgment number: %d\nHeader flags: %d (0x%x)\nChecksum: %d (0x%x)\n", tcpseg.sourceport, tcpseg.destport, tcpseg.seqnum, tcpseg.acknum, tcpseg.headflag, tcpseg.headflag, tcpseg.chksum, tcpseg.chksum); //print receipt
        }
        else{
            close(sockfd);
            exit(1);
        }
    }
    else{
        close(sockfd);
        exit(1);
    }
    sleep(2);
    close(sockfd);
    return 0;
}
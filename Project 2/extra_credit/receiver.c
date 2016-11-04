/*
Client side/ Receiver Side
Input: 
Server/Sender's Hostname
Server/Sender's Port Number
Filename
probability of packet lost - example: .2
probability of packet corruption - example: .1
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <float.h>
#include "common.h"
#include "winvector.h"


int main(int argc, char *argv[])
{
    FILE *fp;
    fp = fopen("test.txt", "ab");
    srand(time(NULL)+2);
    clock_t begin, end;
    begin = clock();


    int sockfd; //Socket descriptor
    int portno, n_send, n_rec;
    socklen_t servlen;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    float pkt_loss = strtod(argv[4], NULL);
    float pkt_corrupt = strtod(argv[5], NULL);

    char buffer[BUFFER_SIZE];
    //Check that there are enough arguments to run client
    if (argc < 3) {
       printf("Not enough arguments\n");
       exit(0);
    }
    //convert 2nd argument into port number of server
    portno = atoi(argv[2]);

    //CREATE SOCKET
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new UDP socket
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
    printf("Socket was created\n");


    //GET IP AND OTHER INFO FROM HOSTNAME
    server = gethostbyname(argv[1]); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    memset(buffer,0, BUFFER_SIZE);
    printf("The file requested is: %s\n", argv[3]);
    memcpy(buffer, argv[3], strlen(argv[3]));

    //SEND REQUEST TO SERVER
    servlen = sizeof(serv_addr);
    n_send = sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &serv_addr, servlen); //write to the socket
    if (n_send < 0) {
         error("ERROR writing to socket");
     }

    //READ RESPONSE FROM SERVER
    struct Packet * pkt = (struct Packet *) malloc (sizeof(struct Packet));
    int n_pre = recvfrom(sockfd,pkt, sizeof(*pkt), 0, (struct sockaddr *) &serv_addr, &servlen); //read from the socket
    if (n_pre < 0) {
        error("ERROR reading from socket");
    }
    printf("Received PREAMBLE Packet from sender.\n");
    char *tkn;
    const char d[2] = "|";
    tkn = strtok(pkt->data, d);
    int num_pcks_exptd = atoi(tkn);
    printf("The number of packets expected are: %i\n", num_pcks_exptd);

    tkn = strtok(NULL, d);
    int cwnd = 200;
    printf("The window size is: %i\n", cwnd);
    int num_pcks_acked = 0;

    if(num_pcks_exptd == 0 && cwnd == 0) {
        printf("File requested does not exist.\n");
    }
    else {
        //initalize window struct, counter, and ack/rtr arrays
        WinVector window_status;
                vector_init(&window_status, cwnd);
        vector_double_capacity_if_needed(&window_status, cwnd);

        int counter1 = 0;
        int counter2 = 0;
        int i;
        int *ack_array = (int *) malloc(sizeof(int)*window_status.size);
        int *rtr_array = (int *) malloc(sizeof(int)*window_status.size);
        for(i=0; i<window_status.size; i++){
            ack_array[i] = -1;
            rtr_array[i] = -1; 
        }
        //initialize int array of sequence numbers expected (30KB max sequence number)
        int *seq_num_to_expt = (int *) malloc(sizeof(int)*num_pcks_exptd);
        int a = 0;
        int loop = 0;
        while (loop<num_pcks_exptd){
            seq_num_to_expt[loop] = a % MAX_SEQ_NUM;
            a += BUFFER_SIZE;
            loop++;
        }
        //initialized window with sequence numbers we are expecting and ACK = 0
        int j;
        for(j = 0; j<window_status.size; j++){
            window_status.SEQ[j]=seq_num_to_expt[j];
            window_status.ACK[j]= 0;
            window_status.PKT[j].type = 'n';
        }
        int marker_for_seq_arr = window_status.size;
        int move_over_value=0;
        //initialized array to hold packets received in order
        //keep receiving file until the whole file is received
        int processing = 1;
        while(processing) {
            while( move_over_value < window_status.size){
                struct Packet * pkt = (struct Packet *) malloc (sizeof(struct Packet));
                int n_resp = recvfrom(sockfd, pkt, sizeof(*pkt), 0, (struct sockaddr *) &serv_addr, &servlen);
                if (n_resp < 0){
                    error("ERROR in reading from socket");
                }
                else {
                    if(pkt->type == 'w') {


                        //window_status.size = atoi(pkt->data);
                        printf("Window expanded to  %i.\n", window_status.size);
                    }

                    double randomL = rand_num_gen_0_1();
                    double randomC = rand_num_gen_0_1();
                    if(randomL < pkt_loss){
                        printf("Packet %i is LOST.\n", pkt->seq_num);
                    }
                    else if (randomC < pkt_corrupt) {
                        printf("Packet %i is CORRUPTED.\n", pkt->seq_num);
                        int i;
                        for (i = 0; i < window_status.size; i++) {
                            if (window_status.SEQ[i] == pkt->seq_num && window_status.ACK[i] != 1) {
                                rtr_array[counter1] = pkt->seq_num;
                                counter1=counter1+1; 
                            }      
                        }
                    } 
                    else{
                        printf("Received Packet %i of type DATA\n", pkt->seq_num);             
                        int i;
                        int trig = 1; //turn trigger on in order to search history if needed
                        for (i = 0; i < window_status.size; i++) {
                            if (window_status.SEQ[i] == pkt->seq_num && window_status.ACK[i] != 1) {
                                trig = 0;
                                set_packet(&window_status, i,  *pkt);
                                ack_array[counter2] = pkt->seq_num;
                                counter2=counter2+1;
                                num_pcks_acked++;
                                window_status.ACK[i] = 1; 
                            }
                            else if (window_status.SEQ[i] == pkt->seq_num) {
                                trig = 0;
                                move_over_value = window_status.size;
                                ack_array[counter2] = pkt->seq_num;
                                printf("Found duplicate of Packet %i. Will send ACK anyway.\n", pkt->seq_num);
                            }
                        } //finished checking which packets need to be acked (do not send ack if already have packet or packet not in window)
                        if(trig){ //if we didn't receive a packet in our window we have to resend
                            int iter;
                            for(iter= 0; iter<marker_for_seq_arr; iter++){
                                if(seq_num_to_expt[iter]==pkt->seq_num){
                                    ack_array[counter2] = pkt->seq_num;
                                    counter2=counter2+1;
                                    move_over_value = window_status.size;
                                    //printf("Found duplicate of Packet %i. Will send ACK anyway.\n", pkt->seq_num);
                                }
                            }
                        }
                    } //end of else for not corrupted packet
                    int s;
                    int sum = 0;
                    for(s=0; s<window_status.size; s++){
                        sum=sum+window_status.ACK[s]; //see if the whole window has been acked
                    }
                    if((counter1+sum)==window_status.size || sum==window_status.size){
                        break;
                    }
                    move_over_value++;
                    if(num_pcks_acked+counter1 == num_pcks_exptd){
                        move_over_value = window_status.size;
                    }
                }
            } //end stationary window while loop
            counter1 = 0;
            counter2=0;
            struct Packet ack_pkt;
            int y;
            for (y = 0; y < window_status.size; y++) {
                if (ack_array[y] != -1) {
                    //wait to send ack packet each time you send
                    time_delay(200);
                    ack_pkt.seq_num = ack_array[y];
                    ack_pkt.type = 'a';
                    n_send = sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *) &serv_addr, servlen);
                    if (n_send < 0) {
                        error("ERROR writing to socket");
                    }
                    printf("Sent an ACK for Packet %i\n", ack_array[y]);
                    ack_array[y] = -1;
                    if (num_pcks_acked == num_pcks_exptd) {
                        processing = 0;
                        break;
                    }
                }
            } //end of for loop for ACK section
            
            struct Packet rtr_pkt;
            for (i = 0; i < window_status.size; i++) {
                if (rtr_array[i] != -1) {
                    //wait to send rtr packet each time you send
                    time_delay(200);
                    rtr_pkt.seq_num = rtr_array[i];
                    rtr_pkt.type = 'r';
                    n_send = sendto(sockfd, &rtr_pkt, sizeof(rtr_pkt), 0, (struct sockaddr *) &serv_addr, servlen);
                    if (n_send < 0) {
                        error("ERROR writing to socket");
                    }
                    printf("Sent back a RTR for Packet %i\n", rtr_array[i]);
                    rtr_array[i] = -1;
                }
            } // end of for loop for RTR section
                
            int j = 0;
            move_over_value = 0;
            // if the leftmost packet in the window is acknowledged, then move the window
             /*     printf("%s\n", "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
                  print_data(&window_status,'S');
                  print_data(&window_status,'T');
                  print_data(&window_status,'A');
                  printf("%s\n", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");*/
            if (window_status.ACK[0] == 1) {
                //append to file
                while(window_status.ACK[j] == 1 && j < num_pcks_exptd) {
                    if(fputs(window_status.PKT[j].data, fp) < 0){
                         error("ERROR writing to file.");
                    }
                    j++;
                } //write file window packets until a packet has not been received in the sequence we need.
                //Then we keep the place we left off with j and move the window over from there
                //move window down
                int k;
                for (k=j; k<window_status.size; k++) {
                    window_status.ACK[k-j] = window_status.ACK[k];
                    window_status.SEQ[k-j] = window_status.SEQ[k];
                    window_status.PKT[k-j] = window_status.PKT[k];
                    move_over_value = j;
                } 
                //resent empty spots in window
                for (k=window_status.size-j; k<window_status.size; k++) {
                    window_status.ACK[k] = 0;
                    if(marker_for_seq_arr >= num_pcks_exptd){
                        window_status.SEQ[k] = -1;
                    }
                    else{
                        window_status.SEQ[k] = seq_num_to_expt[marker_for_seq_arr];
                        marker_for_seq_arr++;
                    }
                    window_status.PKT[k].type = 'n';
                }
            } // end of if which moves window
            if (j > 0) {
                printf("############## WINDOW MOVED BY %i SPACES ACKED PACKETS %i ##############\n", j, num_pcks_acked);
            }
            if(marker_for_seq_arr >= window_status.size + num_pcks_exptd) {
                break;
            }
            
        } //end of while which does processing
        if (num_pcks_acked == num_pcks_exptd) {
            struct Packet fin_packet;
            fin_packet.type = 'f';
            fin_packet.seq_num = -1;
            n_send = sendto(sockfd, &fin_packet, sizeof(fin_packet), 0, (struct sockaddr *) &serv_addr, servlen);
            if (n_send < 0) {
                error("ERROR writing to socket");
            }
            printf("Sent FIN to sender\n");
        } //end of if which sends FIN
        printf("File successfully received!\n"); 
    } // end of else if file existed  
    close(sockfd); //close socket
    fclose(fp);
    end = clock();
     float transfer_len = timediff(begin, end);
    printf("The whole transfer lasted for: %4.4f sec\n", transfer_len/1000 );
    return 0;
}

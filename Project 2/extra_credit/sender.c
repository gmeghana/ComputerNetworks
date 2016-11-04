/* 
Server side/ Sender side
Input:
Port Number - example: 15000
Window Size - example: 5 
probability of packet loss - example: .2
probability of packet corruption - example: .1 
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <float.h>
#include "common.h"
#include "winvector.h"

static const char large_serverFile [] = "large.txt";
static const char small_serverFile [] = "small.txt";
static const int timer = 200; 


int main(int argc, char *argv[])
{  
  char buffer[BUFFER_SIZE];
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  WinVector window_status;
  fd_set live_fd_set; //holds socket that we are reading and writing to
  struct timeval timeout;
    timeout.tv_sec = 0; 
    timeout.tv_usec = timer*100; 
  srand(time(NULL));

  //Check that all arguments are inputted correctly
  if (argc < 5) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
   vector_init(&window_status, atoi(argv[2]));

  vector_double_capacity_if_needed(&window_status, atoi(argv[2]));
  print_data(&window_status,'S');
  print_data(&window_status,'T');
  print_data(&window_status,'A');
  float pkt_loss = strtod(argv[3], NULL);
  float pkt_corrupt = strtod(argv[4], NULL);

  //CREATING SOCKET
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//create UDP socket
  if (sockfd < 0) {
    error("ERROR opening socket");
  }
  printf("Socket created\n");

  memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
  //fill in address info
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
     
  //BINDING SOCKET
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ 
    error("ERROR on binding");
  }
  printf("Socket is binding\n");
  printf("Timer value is set for %i seconds.\n", timer);

  clilen = sizeof(cli_addr);

  //READ FILE REQUEST 
  int n_recv = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
  if (n_recv < 0){
    error("ERROR in reading from socket");
  } 
  printf("File name requested is: %s\n", buffer);
     	 
  //WRITE file if the file exists in server
  if(strncmp(buffer, large_serverFile, strlen(large_serverFile))==0 || strncmp(buffer, small_serverFile, strlen(small_serverFile))==0){
    int n_sent;
    long header_size = 9;

    //setting the socket's timer on the socket
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    FD_ZERO(&live_fd_set);
    FD_SET(sockfd, &live_fd_set);

    //open file
    FILE *fp = fopen(buffer, "rb");
    if(fp == 0){
      error("The file cannot be opened");
    }
    //go to the very end of the file to determine the file size
    if(fseek(fp, 0, SEEK_END) == -1){
      error("The file had problems with the seek function");
    }
    //determine the file size
    long file_size = ftell(fp);
    if(file_size == -1){
      error("The file size was not retrieved");
    }
    //set the pointer to the beginning
    rewind(fp);

    //Initialize buffer for data
    char pckdata[BUFFER_SIZE-header_size];
    char *msg = (char *) malloc(sizeof(char)*file_size);
    int progress = 0;
    bzero(msg, file_size);

    //read piece of the file into buffer "msg"
    int read_bytes = fread(msg, file_size, 1, fp);
    if(read_bytes < 1){
      error("The file was not read");
    }
    fclose(fp);

    //Send preamble packet containing number of packets to receive 
    int n_packets_to_send = 1 + ((file_size - 1)/(BUFFER_SIZE-header_size));
    if (window_status.size > n_packets_to_send) {
      window_status.size = n_packets_to_send;
    }
    struct Packet preamble_packet;
    preamble_packet.type = 'p';
    preamble_packet.seq_num = -1;
    sprintf(preamble_packet.data, "%i | %i", n_packets_to_send, window_status.size);
    n_sent = sendto(sockfd, &preamble_packet, sizeof(preamble_packet), 0, (struct sockaddr *) &cli_addr, clilen);
    if (n_sent < 0) {
      error("ERROR writing to socket");
    }
    printf("Sent the following preamble: %s\n", preamble_packet.data);
    int prev_seq = 0;
    int prev_pkt_size = 0;
    int n_pkts_sent = 0;
    int trigger = 0;
    int num_pkts_acked = 0;


    int processing = 1;
    while(processing){
      //Send file in pieces to receiver with header
      int total_b_sent = 0;
      int pkt_num = 0;
      time_t sec;
      while(pkt_num < window_status.size) {
        if(get_data(&window_status, 'T', pkt_num)==0 && file_size != 0){
          bzero(pckdata, BUFFER_SIZE-header_size);
          // checking when we are at the last packet
          if (file_size < BUFFER_SIZE-header_size) {
            memcpy(pckdata, msg + progress, file_size);
            progress += file_size;
            file_size = file_size - file_size;
            trigger = 1;
          }
          else {
            memcpy(pckdata, msg + progress, BUFFER_SIZE-header_size);
            progress += BUFFER_SIZE-header_size;
            file_size = file_size - (BUFFER_SIZE-header_size);
          }
          set_data(&window_status, 'S', pkt_num, (prev_seq + prev_pkt_size) % MAX_SEQ_NUM);
          struct Packet *pkt = (struct Packet *) malloc (sizeof(struct Packet));
          pkt->seq_num = (prev_seq + prev_pkt_size) % MAX_SEQ_NUM;
          pkt->type = 'd';
          strcpy(pkt->data, pckdata);

          n_sent = sendto(sockfd, pkt, sizeof(*pkt), 0, (struct sockaddr *) &cli_addr, clilen);
          if (n_sent < 0) {
            error("ERROR writing to socket");
          }
          n_pkts_sent++;
          //printf("Just sent DATA packet %i\n", window_status.SEQ[pkt_num]);
          clock_t current = clock();
          set_data(&window_status, 'T', pkt_num,  current);
          set_data(&window_status, 'A', pkt_num, 0);
          set_packet(&window_status, pkt_num, *pkt);
          prev_seq = get_data(&window_status, 'S', pkt_num);
          prev_pkt_size = strlen(pckdata) + header_size;

          if(trigger){
            break;
          }
        } //if this is a null slot in the window, then fill it with a packet
        pkt_num++;
      } //end window max while loop

      //RECEIVING ACK PACKETS
      while (get_data(&window_status, 'A', 0) == 0) {
        //if window sent then look for confirmations
        if(select(sockfd+1, &live_fd_set, NULL, NULL, &timeout) < 0){
            error("ERROR with select function");
        }
        if(FD_ISSET(sockfd, &live_fd_set)){
          struct Packet * ar_pkt = (struct Packet *) malloc (sizeof(struct Packet));
          int n_resp = recvfrom(sockfd, ar_pkt, sizeof(*ar_pkt), 0, (struct sockaddr *) &cli_addr, &clilen);
          if (n_resp < 0){
            error("ERROR in reading from socket");
          }
          // if fin is received, consider all other waiting ACKS as ACKED 
          if(ar_pkt->type == 'f') {
            processing = 0;
	    printf("Received FIN.");
            break;
          }

          //if loss
          double randomL = rand_num_gen_0_1();
          //if corrupted
          double randomC =  rand_num_gen_0_1();
          if(randomL < pkt_loss){
            printf("Packet %i is LOST.\n", ar_pkt->seq_num);
            ar_pkt->type = 'l';
          }
          else if(randomC < pkt_corrupt){ //if corrupted
            printf("Packet %i is CORRUPTED.\n", ar_pkt->seq_num);
            ar_pkt->type = 'c';
          }
          if(ar_pkt->type == 'a'){
            int i;
            for (i = 0; i<window_status.size; i++) {
              if (ar_pkt->seq_num == get_data(&window_status, 'S', i)) {
                set_data(&window_status, 'A', i, 1);
                printf("Received ACK Packet %i.\n", ar_pkt->seq_num); 
                num_pkts_acked++;
              }
            }
          } // end of ack packet section

          struct Packet temp;
          if (ar_pkt->type == 'r') {
            int i;
            for (i = 0; i<window_status.size; i++) {
              if (ar_pkt->seq_num == get_data(&window_status, 'S', i)) {
                printf("Received RTR packet %i.\n", ar_pkt->seq_num);
                temp = window_status.PKT[i];
                n_sent = sendto(sockfd, &temp, sizeof(temp), 0, (struct sockaddr *) &cli_addr, clilen);
                if (n_sent < 0) {
                  error("ERROR writing to socket");
                }
                printf("Retransmitted CORRUPTED Packet %i.\n", window_status.PKT[i].seq_num);
                //clock_t current = clock();
                set_data(&window_status, 'T', i, clock());                  
              }
            }
          } //end of rtr packet section
        } //end of if statement for selecting feed of socket
        else {
          struct Packet t;
          int m;
          for ( m = 0; m<window_status.size; m++) {
            //printf("elapsed: %ld ms\n", timediff(window_status.TTL[m], clock() ));
            if(window_status.ACK[m] == 0 && timediff(window_status.TTL[m], clock() ) >= timer && window_status.PKT[m].type != 'n') {
         
              t = window_status.PKT[m];
              n_sent = sendto(sockfd, &t, sizeof(t), 0, (struct sockaddr *) &cli_addr, clilen);
              if (n_sent < 0) {
                error("ERROR writing to socket");
              }
              printf("Packet %i timedout - Retransmitting.\n", window_status.PKT[m].seq_num);
              //clock_t * current = clock();
              set_data(&window_status, 'T', m, clock());
            }
          } //end search for all packets that need to be retransmitted in window scope
          
          //find min TTL amongst our window
          int n;
          long min = 100000000000000;
          for(n = 0; n<window_status.size; n++){
            if((window_status.TTL[n]+timer - clock()<min && (window_status.TTL[n] + timer - clock()))>0 && window_status.ACK[n] == 0){
              min = window_status.TTL[n] + timer - clock();
            }
          }
          if(min == 100000000000000){
            min = timer;
          }
          timeout.tv_sec =  0; 
          timeout.tv_usec = timer*100;
          FD_ZERO(&live_fd_set);
          FD_SET(sockfd, &live_fd_set);

        } // end of else that checks timers
      } //end of stationary window while loop
      print_data(&window_status,'S');
      print_data(&window_status,'T');
      print_data(&window_status,'A');
      int move = 0;
      int offset = window_status.size;
      int j;
      for (j=0; j<window_status.size; j++) {
        if(window_status.ACK[j] == 0) {
          offset = j;
          break;
        }
      }
      int k;
      for (k=offset; k<window_status.size; k++) {
        set_data(&window_status,'A', move, get_data(&window_status,'A', k));
        set_data(&window_status,'S', move, get_data(&window_status,'S', k));
        set_data(&window_status,'T', move, get_data(&window_status,'T', k));
        window_status.PKT[move] = window_status.PKT[k];
        move++;
      } 
      for (k=move; k<window_status.size; k++) {
        set_data(&window_status, 'A', k, 0);
        set_data(&window_status, 'S', k, 0);
        set_data(&window_status, 'T', k, 0);
        window_status.PKT[k].type = 'n';
      }

      printf("%s\n", "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
      print_data(&window_status,'S');
      print_data(&window_status,'T');
      print_data(&window_status,'A');
      printf("%s\n", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
      if (offset > 0) {
        printf("############## WINDOW MOVED BY %i SPACES ##############\n", offset);
        if (offset == window_status.size)
        {
          if(window_status.size > atoi(argv[5])) vector_double_capacity_if_needed(&window_status, offset+1);
            else vector_double_capacity_if_needed(&window_status, offset*2);
          printf("        ->Expanded window size to  %i  \n", window_status.size);
          printf("%s\n", "---->>>>>vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
          print_data(&window_status,'S');
          print_data(&window_status,'T');
          print_data(&window_status,'A');
          printf("%s\n", "---->>>>>^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
          struct Packet preamble_packet;
          preamble_packet.type = 'w';
          preamble_packet.seq_num = -1;
          sprintf(preamble_packet.data, "%i",  window_status.size);
          n_sent = sendto(sockfd, &preamble_packet, sizeof(preamble_packet), 0, (struct sockaddr *) &cli_addr, clilen);
          if (n_sent < 0) {
            error("ERROR writing to socket");
          }
          printf("Sent the following preamble: %s\n", preamble_packet.data);


        }
      }

    }
    printf("File successfully sent!\n");
  }
  else{
    struct Packet preamble_packet;
    preamble_packet.type = 'p';
    preamble_packet.seq_num = -1;
    sprintf(preamble_packet.data, "%i | %i", 0, 0);
    sendto(sockfd, &preamble_packet, sizeof(preamble_packet), 0, (struct sockaddr *) &cli_addr, clilen);
    printf("Client requested file that the server does not have.\n");
  }
  close(sockfd);//close connection
   
  return 0; 
}


#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED


static const int BUFFER_SIZE = 1024;
static const int WINDOW_SIZE = 5;
static const long header_size = 9; 
static const int MAX_SEQ_NUM = 30000;


struct Packet {
  int   seq_num;
  char  data [BUFFER_SIZE - header_size]; //9 bits for header, so 1024 - 9 = 1015
  char  type;  // d = data, a = ack, p = preamble, r = retransmission

};

void error(char *msg)
{
    perror(msg);
    exit(0);
}

double rand_num_gen_0_1()
{
    return (double) rand() / (double) RAND_MAX ;

}

long timediff(clock_t t1, clock_t t2) {
    long elapsed;
    elapsed = ((double)t2 - t1) / CLOCKS_PER_SEC * 100000;
    return elapsed;
}

void time_delay(int timer_len){
    clock_t t1, t2;
    int i;
    float x = 2.7182;
    long elapsed;

    t1 = clock();

    while (elapsed < timer_len){
    	t2 = clock();
		elapsed = timediff(t1, t2);
    	
    }
    
    //printf("elapsed: %ld ms\n", elapsed);


}

#endif
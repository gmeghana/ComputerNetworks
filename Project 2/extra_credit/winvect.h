// vector.h
#include "common.h"
#define VECTOR_INITIAL_CAPACITY 15

// Define a vector type
typedef struct {
  int size;      // slots used so far
  int capacity;  // total available slots
  int *Seq;     // array of integers we're storing
  int *TTL;     // array of integers we're storing
  int *ACK;     // array of integers we're storing
  struct Packet *PKT;
} WinVector;

void vector_init(WinVector *wv) {
  // initialize size and capacity
  wv->size = 0;
  wv->capacity = VECTOR_INITIAL_CAPACITY;

  // allocate memory for vector->data
  wv->SEQ = malloc(sizeof(int) * wv->capacity);
  wv->TTL = malloc(sizeof(int) * wv->capacity);
  wv->ACK = malloc(sizeof(int) * wv->capacity);
  wv->PKT = malloc(sizeof(Packet) * wv->capacity);
}
//void vector_append(WinVector *wv, int value);

int get_data(WinVector *wv, int arr_num, int index){
	
	if (index >= wv->size || index < 0) {
    	printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
    	exit(1);
  	}

	switch(arr_num) {
      case 'SEQ' :
         return vw->SEQ[index];
         
      case 'TTL' :
      	return vw->TTL[index];
      case 'ACK' :
         return vw->ACK[index];
      default :
         printf("Invalid grade\n" );
         return 0;
     }


  	
}


void vector_set(WinVector *wv, int index, char arrtype, int value);

void vector_double_capacity_if_full(WinVector *wv);

void vector_free(WinVector *wv);
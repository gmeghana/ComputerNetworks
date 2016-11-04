// vector.h
#ifndef WINVECTOR_H_INCLUDED
#define WINVECTOR_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#define VECTOR_INITIAL_CAPACITY 15

// Define a vector type
typedef struct {
  int size;      // slots used so far
  int capacity;  // total available slots
  int *SEQ;     // array of integers we're storing
  int *TTL;     // array of integers we're storing
  int *ACK;     // array of integers we're storing
  struct Packet *PKT;
} WinVector;

void vector_init(WinVector *wv, int size) {
  // initialize size and capacity
  wv->size = size;
  wv->capacity = VECTOR_INITIAL_CAPACITY;
  while(size > wv->capacity){
      wv->capacity = wv->capacity * 2;
  }

  // allocate memory for vector->data
  wv->SEQ = malloc(sizeof(int) * wv->capacity);
  wv->TTL = malloc(sizeof(int) * wv->capacity);
  wv->ACK = malloc(sizeof(int) * wv->capacity);
  wv->PKT = malloc(sizeof(struct Packet) * wv->capacity);
  
  struct Packet null;
  null.type = 'n';
  int k;
  for (k=0; k < wv->capacity; k++) {
      wv->ACK[k] = 0;
      wv->SEQ[k] = 0;
      wv->TTL[k] = 0;
      wv->PKT[k] = null;
  }


}
//void vector_append(WinVector *wv, int value);

int get_data(WinVector *wv, char arr_type, int index){
	
	if (index >= wv->size || index < 0) {
    	printf("Index %d out of bounds for vector of size %d\n", index, wv->size);
    	exit(1);
  	}

	switch(arr_type) {
      case 'S' :
         return wv->SEQ[index];
      case 'T' :
      	return wv->TTL[index];
      case 'A' :
         return wv->ACK[index];
      default :
         printf("Invalid arr_type\n" );
         return 0;
     }


  	
}


void set_data(WinVector *wv, char arr_type, int index, int value){
	if (index >= wv->size || index < 0) {
    	printf("Index %d out of bounds for vector of size %d\n", index, wv->size);
    	exit(1);
  	}

	switch(arr_type) {
      case 'S' :
          	wv->SEQ[index] = value;
            break;
      case 'T' :
      	 	wv->TTL[index] = value;
          break;
      case 'A' :
          	wv->ACK[index] = value;
            break;
      default :
         printf("Invalid arr_type\n" );
         break;
     }

}

void set_packet(WinVector *wv, int index, struct Packet pkt)
{
    if (index >= wv->size || index < 0) {
      printf("Index %d out of bounds for vector of size %d\n", index, wv->size);
      exit(1);
    }

    wv->PKT[index] = pkt;
}

void print_data(WinVector *wv, char arr_type){
  int k;
  if (wv->size != 0){
    switch(arr_type) {
        case 'S' :
              for(k=0; k < wv->size; k++) printf("SEQ[%i]: %i\n",k, wv->SEQ[k]);
              break;
        case 'T' :
              for(k=0; k < wv->size; k++) printf("TTL[%i]: %i\n",k, wv->TTL[k]);
              break;
        case 'A' :
              for(k=0; k < wv->size; k++) printf("ACK[%i]: %i\n",k, wv->ACK[k]);
              break;
        default :
           printf("Invalid arr_type\n" );
       }
  }
}

void vector_free(WinVector *wv) {
  free(wv->SEQ);
  free(wv->TTL);
  free(wv->ACK);
  free(wv->PKT);
}

void vector_double_capacity_if_needed(WinVector *wv, int size_needed){
  if (size_needed > wv->capacity && size_needed > wv->size) {

    // double vector->capacity and resize the allocated memory accordingly

    WinVector temp;
    vector_init(&temp, size_needed);

    int i;
    for(i =0; i<wv->size; i++)  temp.SEQ[i] = wv->SEQ[i];
    for(i =0; i<wv->size; i++)  temp.TTL[i] = wv->TTL[i];
    for(i =0; i<wv->size; i++)  temp.ACK[i] = wv->ACK[i];
    for(i =0; i<wv->size; i++)  temp.PKT[i] = wv->PKT[i];

    vector_free(wv);
    wv->capacity = temp.capacity;
    
    // allocate memory for vector->data
    wv->SEQ = malloc(sizeof(int) * wv->capacity);
    wv->TTL = malloc(sizeof(int) * wv->capacity);
    wv->ACK = malloc(sizeof(int) * wv->capacity);
    wv->PKT = malloc(sizeof(struct Packet) * wv->capacity);
    for(i =0; i<temp.capacity; i++) wv->SEQ[i] = temp.SEQ[i];
    for(i =0; i<temp.capacity; i++) wv->TTL[i] = temp.TTL[i];
    for(i =0; i<temp.capacity; i++) wv->ACK[i] = temp.ACK[i];
    for(i =0; i<temp.capacity; i++) wv->PKT[i] = temp.PKT[i];
    
    wv->size = size_needed;
    vector_free(&temp);
    vector_double_capacity_if_needed(wv, size_needed);

  }
  else if(size_needed > wv->size)wv->size = size_needed;
}




#endif
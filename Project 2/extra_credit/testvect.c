#include "winvector.h"

int main(){

WinVector win;
vector_init(&win, 5);


set_data(&win,'A',0,1);

print_data(&win,'T');
printf("%s\n", "\nDone\n");

}
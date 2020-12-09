#include <tinyos.h>

typedef struct procinfo_control_block{
    procinfo info;
    int PCB_cursor;
}procinfo_cb;

int info_read(void* , char *, unsigned int);
int info_close(void *);
int info_dummy_write(void* , const char *, unsigned int);
void * info_dummy_open(unsigned int);

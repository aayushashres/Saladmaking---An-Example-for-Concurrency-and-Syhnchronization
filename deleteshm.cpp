#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h> 
# include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include <stdlib.h>

#include "structures.h"
#define SHMSIZE sizeof(structure) //size of shared memory

int main(int argc, char** argv){
    int shmid;
    for (int i=0; i<argc-1; i++) //reading the command line to determine arduments (what follows each flag)
	{
		if (strcmp((argv[i]),"-s")==0 && (i+1<argc)) 
		{
            shmid=atoi(argv[i+1]);
		}
    }
    int err;
	structure* shm;

	shm = (structure*) shmat (shmid , NULL,0); /* Attach the segment */
    err = shmdt (shm); /* Detach segment */
    err = shmctl (shmid , IPC_RMID , NULL); 
	if (err == -1) 
		perror ("Removal Error.");
	else 
		printf ("Removed. %d\n", (int)(err));
    return 0;
}
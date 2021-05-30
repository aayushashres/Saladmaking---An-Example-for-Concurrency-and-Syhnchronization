#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h> 
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/stat.h>
#include <stdio.h> 
#include <stdlib.h>
#include "structures.h"
using namespace std;
#define SHMSIZE sizeof(structure) //size of shared memory

//reference: page 1201 in The Linux Programming inTerface A Linux and UNIX® System Programming Handbook by Michael Kerrisk
ssize_t
readLine(int fd, char *buffer, size_t n)
{
    ssize_t numRead; //the number of bytes of the last read
    size_t totRead; //the total bytes read
    char *buf;
    char ch;

    if (n<=0 || buffer==NULL){
        errno =EINVAL;
        return -1;
    }
    buf = buffer;
    totRead=0;
    for(;;){
        numRead=read(fd,&ch,1);

        if (numRead== -1){ //restarts the read
            if(errno==EINTR){
                continue;
            }else{
                return -1;
            }
        }else if(numRead==0){ //reached the end of file
            if(totRead==0){
                return 0;
            }
            else{
                break; //goes to \0 so end of line is added
            }
        }else{
            if (totRead<n-1){ //adding to buf
                totRead++;
                *buf++=ch;
            }
            if(ch=='\n'){  //delimiter 
                break;
            }
        }
    }
    *buf='\0';
    return totRead;
}

int main(int argc, char** argv)
{
    
    int salads_needed;
    char cheftimestring[1024];
    int cheftime;
    srand (time(NULL));
   
    for (int i=0; i<argc-1; i++) //reading the command line to determine arguments (what follows each flag)
	{
		if (strcmp((argv[i]),"-n")==0 && (i+1<argc)) 
		{
            salads_needed=atoi(argv[i+1]);
		}
        if (strcmp((argv[i]),"-s")==0 && (i+1<argc)) 
		{
            cheftime=atoi(argv[i+1]);
            // printf("cheftime: %f \n", cheftime);
        }
	}

    //creating shared memory space, referenced labs from recitation
    int shmid;
    key_t key;
    structure* shm;
    int err=0;

    key = 9999; //some arbitary key value to uniquely identify the shared memory segment
  
    //Create the shared memory segment----------------------------------------------------
    shmid = shmget(key, SHMSIZE , IPC_CREAT | 0666);
    printf("Shared memory ID: %d\n", shmid);
  
    //check for faiure
    if (shmid < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    //Attach the segment to the data space
    shm = (structure*) (shmat(shmid , NULL , 0));
    //check for failure
    if (shm == (structure*) -1)
    {
        perror("shmat failure");
        exit(1);
    }

    FILE* concurrencylog;
    char concurrencylogstring[1024];

    // FIFO STUFF
    int fd;
    // printf("GETS HERE BEFORE FIFO\n");
    char* myfifo = "myfifo"; 
    mkfifo(myfifo, 0777); 
    int LENGTH = 600;
    char str1[LENGTH]; 
    fd = open(myfifo, O_RDONLY); //chef only reads the info put my saladmakers from fifo
    // printf("READ ONLY\n");

    shm->done=false; // boolean in shared memory to be used by saladmaker processes 

    while (shm->count<3){
        //wait for creation of all saladmakers
    }
    printf("All salad makers are active.\n");

    //chef semaphore to ensure no one "reads" the table when chef is putting ingredients
    sem_t* chef;
    chef = sem_open("chef_semaphore", O_CREAT, 0666, 1);
    
    //semaphores for each saladmaker to ensure only one saladmaker can access the table at one time. They are intiialized to zero
    //and they are posted to be active by the chef when the combination of veggies they need becomes available on table
    sem_t* onion_infinite = sem_open("onion saladmaker semaphore", O_CREAT, 0666, 0);
    sem_t* tomato_infinite= sem_open("tomato saladmaker semaphore", O_CREAT, 0666, 0);
    sem_t* pepper_infinite=sem_open("pepper saladmaker semaphore", O_CREAT, 0666, 0);

    //semaphore to protect count of numver of salads produced in shard memory
    sem_t* saladcount_sem=sem_open("saladcount", O_CREAT, 0666, 1);

    int combo_type; //combo type denotes which 2 vegetables are being put on the table
    int chef_wait_time; //chef wait time depends on what we get from terminal



    //loop until required number of salads have been made
    while (shm->num_of_salads_produced<salads_needed){
        // waittime - what if we start here and end right before sem_posting on salmaker

        combo_type = rand() % 3 + 1;  //generating a random number between 1 and 3 to decide which combination of veggies chef puts on table
        chef_wait_time = (rand() % (int)(0.5 * cheftime) + (int)(0.5 * cheftime)); //chef wait time is between 50% of chef time to cheftime received from terminal
        usleep(chef_wait_time*1000); //chef sleeps
        // https://man7.org/linux/man-pages/man3/usleep.3.html
        
        sem_wait(chef); //wait on chef to make sure no one has access to table when chef is putting veggies on table
        if (shm->num_of_salads_produced<=salads_needed){
            printf("\n Total salads made: %d\n", shm->num_of_salads_produced);
        }
        

        //for each combination, the pertinent vegetables are generated as such:
        /*
            tomatoes in the range of 64 to 96 (amount required by saladmaker is 80)
            onions in the range of 24 to 36 (amount required by saladmaker is 30)
            peppers in the range of 40 to 60 (amount required by saladmaker is 50)
        */

        /*
            for each combination, the right saladmaker is posted after chef puts veggies on the table so they can "read" the veggies
        */

        if (combo_type==1){ //combo 1 is onion and pepper
            printf("...Generating combo 1 (onions and peppers)...\n");
            shm->onion = rand() % 12 + 24;
            shm->pepper = rand() % 20 + 40;

            sem_post(tomato_infinite);
        }
        else if (combo_type==2){ //combo 2 is tomato and pepper
            printf("...Generating combo 2 (tomatoes and peppers)...\n");
            shm->pepper = rand() % 20 + 40;
            shm->tomato = rand() % 32 + 64;
            sem_post(onion_infinite);
        }
        else if (combo_type==3){ //combo 3 is onion and tomato
            printf("...Generating combo 3 (onions and tomatoes)...\n");
            shm->onion = rand() % 12 + 24;
            shm->tomato = rand() % 32 + 64;
            sem_post(pepper_infinite);
        }
        

    }


    shm->done=true; //once out of loop, set to true
    printf("\nALL SALADS WERE MADE\n");

    
    // printing out parallel processing times to terminal from logged file
    int myline;
    concurrencylog = fopen("parallel_proc", "r");
    
    // ref: https://www.poftut.com/fopen-function-usage-in-c-and-cpp-with-examples/
    // https://www.tutorialspoint.com/c_standard_library/c_function_fgetc.htm
    while(1) {
      myline = fgetc(concurrencylog);
      if( feof(concurrencylog) ) {
         break ;
      }
      printf("%c", myline);
    }

    fclose(concurrencylog);


     //variables to store data from myfifo
    int index=0;
    char saladmakernumber[1024];
    char totaltomatoes[1024];
    char totalonions[1024];
    char totalpeppers[1024];

    //reading from fifo, referncing lab2
    while(index<3){
        if (readLine(fd, str1, LENGTH)==0){
            continue;
        }
        sscanf(str1, "%s %s %s %s", saladmakernumber, totalonions, totaltomatoes, totalpeppers);
        printf("Saladmaker [%s] used [%s]g of onions [%s]g of tomatoes and [%s]g of peppers in total\n\n", saladmakernumber, totalonions, totaltomatoes, totalpeppers);
        index++;
    }
    close(fd);
    
    //printing the total prepare and wait times for each saladmaker
    printf("-----------------------------------------------------------------------\n");
    printf("PREPARE TIMES:\n  Saladmaker 1: %f milliseconds\n  Saladmaker 2: %f milliseconds \n  Saladmaker 3: %f milliseconds\n", shm->salmaker1_preparetime, shm->salmaker2_preparetime, shm->salmaker3_preparetime);
    printf("-----------------------------------------------------------------------\n");
    printf("WAIT TIMES:\n  Saladmaker 1: %f milliseconds\n  Saladmaker 2: %f milliseconds\n  Saladmaker 3: %f  milliseconds\n", shm->salmaker1_waittime, shm->salmaker2_waittime, shm->salmaker3_waittime);
    //post on all saladmakers - this may been some ongoing saladmaking will be abrupted, but it's fine since we've reached the required number
    //post also makes sure we're not waiting for anything and allows us to terminate the program
    sem_post(onion_infinite);
    sem_post(tomato_infinite);
    sem_post(pepper_infinite);
    sem_post(saladcount_sem);
    
    //closing all semaphores and unlinking
    sem_close(tomato_infinite);
    sem_unlink("tomato saladmaker semaphore");
    sem_close(onion_infinite);
    sem_unlink("onion saladmaker semaphore");
    sem_close(pepper_infinite);
    sem_unlink("pepper saladmaker semaphore");
    sem_close(saladcount_sem);
    sem_unlink("saladcount");
    sem_close(chef);
    // printf("DELETING CHEFSEM\n");
    sem_unlink("chef_semaphore\n");
    
    
    //detach from shared memory  
    err = shmdt (shm); /* Detach segment */
	if (err == -1) 
		perror (" Detachment .");
	else 
		printf (" Detachment from shared memory.%d\n", err );
    //delete shared memory segment
    err = shmctl (shmid , IPC_RMID , NULL); 
	if (err == -1) 
		perror ("Removal.");
	else 
		printf ("Removed shared memory %d\n", (int)(err));
    return EXIT_SUCCESS;
}
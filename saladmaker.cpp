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
#include <sys/stat.h>
#include <time.h>
#include <stdio.h> 
#include <ctime>
#include <chrono>
#include <stdlib.h>
#include <sys/time.h>
#include <ctime>
#include "structures.h"
#include <fstream>
#include <fcntl.h>


using namespace std;

int main(int argc, char** argv){

    double salad_make_time;
    int shmid;
    int vegetable;

   
    for (int i=0; i<argc-1; i++) //reading the command line to determine arduments (what follows each flag)
	{
		if (strcmp((argv[i]),"-s")==0 && (i+1<argc)) 
		{
            shmid=atoi(argv[i+1]);
            printf ("Shared memory ID: %d\n", shmid);
		}
        if (strcmp((argv[i]),"-m")==0 && (i+1<argc)) 
		{
            salad_make_time=atof(argv[i+1]);
            // printf("cheftime: %f \n", cheftime);
        }
	}


    // Accessing Shared Memory 
    int err;
	structure* shm;
    int salad_maker_wait_time;

	shm = (structure*) shmat (shmid , NULL,0); /* Attach the segment */
	
	if (shm == (structure*) -1)
    {
        perror("shmat failure");
        exit(1);
    }
  
	
    //wrap in mutex semaphore to set what veg the saladmaker has infinite of
    // sem_t * mutex;
    // setbuf(stderr, NULL);

    // mutex = sem_open("mutex_for_stderr", O_CREAT, 0666, 1);
    // sem_wait(mutex);
    shm->count++;
    vegetable=shm->count;
    //checking which "vegetable" the saladmaker got as their infinite vegetable
    if(vegetable==1){
        printf("Saladmaker has a basket of tomatoes\n");
    }
	else if(vegetable==2){
        printf("Saladmaker has a basket of onions\n");
    }
    else if(vegetable==3){
        printf("Saladmaker has a basket of peppers\n");
    }
    // sem_post(mutex);
    // //close semaphore
    // sem_close(mutex);
    // unlink 
    // sem_unlink("mutex_for_stderr");

  
    sem_t* chef;
    chef = sem_open("chef_semaphore", O_CREAT, 0666, 1);
    sem_t* onion_infinite = sem_open("onion saladmaker semaphore", O_CREAT, 0666, 0);
    sem_t* tomato_infinite= sem_open("tomato saladmaker semaphore", O_CREAT, 0666, 0);
    sem_t* pepper_infinite=sem_open("pepper saladmaker semaphore", O_CREAT, 0666, 0);
    sem_t* saladcount_sem=sem_open("saladcount", O_CREAT, 0666, 1);
    sem_t* concurrent_count_sem=sem_open("concurrentcount", O_CREAT, 0666, 1);
  

    // opening fifo to write
    int fd; 
    char * myfifo = "myfifo"; 
    mkfifo(myfifo, 0777); 
    char* total_weight_string_inchar = new char[600];
    fd = open(myfifo, O_WRONLY);
    // printf("WRITE ONLY \n");

    //weight of vegetables saldmaker has in grams
    int myonion=0;
    int mytomato=0;
    int mypepper=0;
    int mysaladcount=0;
    
    //time variables to monitor prepare time
    struct timeval time_now;
    time_t c0;
    time_t c1;
    
    //time variables to monitor wait time
    time_t wait_time_end;
    time_t wait_time_start;

    //time variables to monitor when parallel processing start and ends 
    //https://www.programiz.com/cpp-programming/library-function/ctime/time
    time_t conc_start;
    time_t conc_end;

    double waittime=0;
    double preparetime=0;

    //filedirectory for logfiles for each saladmaker
    FILE* filedirectory;
    char filestring[1024];

    // filedirectory for log of concurrent processes
    FILE* concurrencylog;
    char concurrencylogstring[1024];
    
    //while condition untill all required salads are made (using bool because num of salads in shared memory is being used in loop)
    while (shm->done==false){

        //breaktime for salad maker, ranges between 80% of time to time we get from terminal

        salad_maker_wait_time = (rand() % (int)(0.2 * salad_make_time) + (int)(0.8 * salad_make_time));

        //wait time starts here because we want to wait until we get the right amount of veggies to proceed making salad.
        gettimeofday(&time_now, nullptr);
        wait_time_start = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);


        /*
            Each saladmaker has a "vegetable" number associated to them. vegetable 1 means the saladmaker has inifinte tomtoees, 2 for onions, 3 for peppers
            the actions are the same for each if condition, i.e. each saladmaker, expect change in variables like logfile name or vegetables
            i will comment on only the first one for cleaner code, the same logic applies to the remaining two
        */
        if (vegetable==1){
            sem_wait(tomato_infinite); //ensures only this saladmakers can access table

            //writing into log file upon accessing the table (shared memory) and getting vegetables
            filedirectory = fopen("saladmaker1_log", "a");
            fprintf(filedirectory, "Received %d onions %d peppers from chef\n", shm->onion, shm->pepper);
            fclose(filedirectory);
            
            // adding received vegetables to how much the saladmaker has. It starts with 0, but keeps adding up. 
            // adding veggies up to prevent "waste" and also for reduced wait times
            // printf("GETS HERE\n");
            myonion += shm->onion;
            mypepper += shm->pepper;

            // logging how much veggies saladmaker has after adding what they got from table
            filedirectory = fopen("saladmaker1_log", "a");
            fprintf(filedirectory, "Saladmaker has infinite tomatoes, %d onions %d peppers\n", myonion, mypepper);
            fclose(filedirectory);

            // "clearing" the table
            shm->onion = 0;
            shm->pepper = 0;
            
            sem_post(chef); //this means chef can have access to the table again

            //this is when the saladmaker process is working on its own so we start checking if processes(other salad makers) are working in parallel
            sem_wait(concurrent_count_sem);
            shm->num_concurrent_processes++; //increasing the number of active processes

            // if more than 1 process is active we log the start time into log file. the timestamp here is the current calendar as time
            // using a semaphore because all processess access the file
            if(shm->num_concurrent_processes>1){
                conc_start = time(nullptr);
                concurrencylog = fopen("parallel_proc", "a");
                fprintf(concurrencylog, "%d processes running in parallel starting at [%ld]\n", shm->num_concurrent_processes, conc_start);
                fclose(concurrencylog);

            }
            sem_post(concurrent_count_sem);
            
          
            //checking if we have enough vegetables to make salad
            if (myonion>=30 && mypepper>=50){
                 //  if we have enough veggies, it means we are done waiting for enough veggies, so we calculate and add onto wait time which is in shared memory
                 // we dont need a semaphore here because each saladmaker has own varibale in shared memory
                 gettimeofday(&time_now, nullptr);
                 wait_time_end = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                 waittime = (wait_time_end - wait_time_start);
                 shm->salmaker1_waittime+=waittime;

                 //logging time when salad maker starts prepating salad  
                 gettimeofday(&time_now, nullptr);
                 c0 = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                 filedirectory = fopen("saladmaker1_log", "a");
                 fprintf(filedirectory, "started working on salad at time %ld s \n", time(nullptr));
                 fclose(filedirectory);
                 usleep(salad_maker_wait_time*1000); //saladmaker sleep time

                 if(shm->done==false){ //again checking if we need to increase the number of salads incase required number has been reached
                    //semaohore to change shared variabled in shared mem to increase saladcount
                    sem_wait(saladcount_sem);
                    if(shm->done==false){
                        shm->num_of_salads_produced++;
                        mysaladcount++;
                    }
                    sem_post(saladcount_sem);
                    myonion -= 30;
                    mypepper -= 50;
                    //now that the salad is made, we reduce the amount used in preparing it. Preparation is over so we log end of prepare time
                    gettimeofday(&time_now, nullptr);
                    c1 = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                    shm->salmaker1_preparetime += (c1 - c0) ;

                    filedirectory = fopen("saladmaker1_log", "a");
                    fprintf(filedirectory, "finished making salad at %ld s \n", time(nullptr));
                    fclose(filedirectory);
                    filedirectory = fopen("saladmaker1_log", "a");
                    fprintf(filedirectory, "Saladmaker made a salad and total salads made by saladmaker is %d salads\n\n", mysaladcount);
                    fclose(filedirectory);
                    
                    
                 }
               
            }
            

        //now we are done with our process, so we reduce the number of processes active.
        //if this results in only one process being active, we say that parallell processing has stopped
        // in the beginning, we will get a message saying parallel processing has stopped (when we initate saladmakers)

        sem_wait(concurrent_count_sem);
        shm->num_concurrent_processes--;
        if(shm->num_concurrent_processes<=1){
            conc_end = time(nullptr);
            concurrencylog = fopen("parallel_proc", "a");
            fprintf(concurrencylog, "Paralell proccessing stopped at[%ld]. Only one active process. \n", conc_end);
            fclose(concurrencylog);
        }
        sem_post(concurrent_count_sem);
            
        }
        if (vegetable==2){
            
            sem_wait(onion_infinite);

            filedirectory = fopen("saladmaker2_log", "a");
            fprintf(filedirectory, "Received %d tomatos %d peppers from chef\n", shm->tomato, shm->pepper);
            fclose(filedirectory);

            // printf("GETS HERE\n");
            mytomato += shm->tomato; //to accumulate the produce, so it doesnt go to waste
            mypepper +=shm->pepper;

            filedirectory = fopen("saladmaker2_log", "a");
            fprintf(filedirectory, "Saladmaker has infinite onions, %d tomatoes %d peppers\n", mytomato, mypepper);
            fclose(filedirectory);
            
            shm->tomato = 0;
            shm->pepper = 0;
            
            sem_post(chef);

            sem_wait(concurrent_count_sem);
            shm->num_concurrent_processes++;
            if(shm->num_concurrent_processes>1){
                conc_start = time(nullptr);
                concurrencylog = fopen("parallel_proc", "a");
                fprintf(concurrencylog, "%d processes running in parallel starting at [%ld]\n", shm->num_concurrent_processes, conc_start);
                fclose(concurrencylog);
            }
            sem_post(concurrent_count_sem);
                        
            if (mytomato>=80 && mypepper>=50){
                 gettimeofday(&time_now, nullptr);
                 wait_time_end = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                 waittime = (wait_time_end - wait_time_start);
                 shm->salmaker2_waittime+=waittime;

                 gettimeofday(&time_now, nullptr);
                 c0 = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

                 filedirectory = fopen("saladmaker2_log", "a");
                 fprintf(filedirectory, "started working on salad at time %ld s \n", time(nullptr));
                 fclose(filedirectory);
                 usleep(salad_maker_wait_time*1000);
                 if(shm->done==false){
                    
                    sem_wait(saladcount_sem);
                    if(shm->done==false){
                        shm->num_of_salads_produced++;
                        mysaladcount++;
                    }
                    sem_post(saladcount_sem);
                
                    mytomato -=80;
                    mypepper-=50;

                    gettimeofday(&time_now, nullptr);
                    c1 = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                    shm->salmaker2_preparetime += (c1 - c0) ;

                    filedirectory = fopen("saladmaker2_log", "a");
                    fprintf(filedirectory, "finished making salad at %ld s \n", time(nullptr));
                    fclose(filedirectory);
                    filedirectory = fopen("saladmaker2_log", "a");
                    fprintf(filedirectory, "Saladmaker made a salad and total salads made by saladmaker is %d salads\n\n", mysaladcount);
                    fclose(filedirectory);
                    
                 }
                 
            }
            sem_wait(concurrent_count_sem);
            shm->num_concurrent_processes--;
            if(shm->num_concurrent_processes<=1){
                conc_end = time(nullptr);
                concurrencylog = fopen("parallel_proc", "a");
                fprintf(concurrencylog, "Paralell proccessing stopped at[%ld]. Only one active process. \n", conc_end);
                fclose(concurrencylog);
            }
            sem_post(concurrent_count_sem);
                
    
        }
        if (vegetable==3){
            
            sem_wait(pepper_infinite);

            filedirectory = fopen("saladmaker3_log", "a");
            fprintf(filedirectory, "Received %d tomatos %d onions from chef\n", shm->tomato, shm->onion);
            fclose(filedirectory);

            mytomato += shm->tomato;
            myonion+= shm->onion;
    
            filedirectory = fopen("saladmaker3_log", "a");
            fprintf(filedirectory, "Saladmaker has infinite peppers, %d tomatoes %d onions\n", mytomato, myonion);
            fclose(filedirectory);

            shm->tomato = 0;
            shm->onion = 0;
            
            sem_post(chef);

            sem_wait(concurrent_count_sem);
            shm->num_concurrent_processes++;
            if(shm->num_concurrent_processes>1){
                conc_start = time(nullptr);
                concurrencylog = fopen("parallel_proc", "a");
                fprintf(concurrencylog, "%d processes running in parallel starting at [%ld]\n", shm->num_concurrent_processes, conc_start);
                fclose(concurrencylog);
                
            }
            sem_post(concurrent_count_sem);
            

            if (mytomato>=80 && myonion>=30){
                 gettimeofday(&time_now, nullptr);
                 wait_time_end = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                 waittime = (wait_time_end - wait_time_start);
                 shm->salmaker3_waittime+=waittime;

                 gettimeofday(&time_now, nullptr);
                 c0 = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

                 filedirectory = fopen("saladmaker3_log", "a");
                 fprintf(filedirectory, "started working on salad at time %ld s \n", time(nullptr));
                 fclose(filedirectory);
                 usleep(salad_maker_wait_time*1000);
                 if(shm->done==false){
                    
                    sem_wait(saladcount_sem);
                    if(shm->done==false){
                        shm->num_of_salads_produced++;
                        mysaladcount++;
                    }
                        
                    sem_post(saladcount_sem);
                    
                    mytomato -= 80;
                    myonion -= 30;

                    gettimeofday(&time_now, nullptr);
                    c1 = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
                    shm->salmaker3_preparetime += (c1 - c0) ;
                    filedirectory = fopen("saladmaker3_log", "a");
                    fprintf(filedirectory, "finished making salad at %ld s \n", time(nullptr));
                    fclose(filedirectory);
                    filedirectory = fopen("saladmaker3_log", "a");
                    fprintf(filedirectory, "Saladmaker made a salad and total salads made by saladmaker is %d salads\n\n", mysaladcount);
                    fclose(filedirectory);
                    
                 }
                 
                 
            }
            

            sem_wait(concurrent_count_sem);
            shm->num_concurrent_processes--;
            if(shm->num_concurrent_processes<=1){
                conc_end = time(nullptr);
                concurrencylog = fopen("parallel_proc", "a");
                fprintf(concurrencylog, "Paralell proccessing stopped at[%ld]. Only one active process. \n", conc_end);
                fclose(concurrencylog);
            }
            sem_post(concurrent_count_sem);

        }
        
    }

    //calculating total amount vegetables used by multipling weight of ingredients by number of salads made
    int totalonion = 30 * mysaladcount;
    int totaltomato = 80 * mysaladcount;
    int totalpepper = 50 * mysaladcount;

    //writing to my fifo, referenced lab2
    string onions_weight = to_string(totalonion);
    string tomato_weight = to_string(totaltomato);
    string pepper_weight = to_string(totalpepper);
    string salad_maker_number = to_string(vegetable);

    string total_weight_string =  salad_maker_number + " " + onions_weight + " " + tomato_weight + " " + pepper_weight +"\n";
    total_weight_string_inchar = const_cast<char*>(total_weight_string.c_str());    
    
    write(fd, total_weight_string_inchar, strlen(total_weight_string_inchar));
    close(fd);

    // making sure no process is stuck on a semaphore
    sem_post(onion_infinite);
    sem_post(tomato_infinite);
    sem_post(pepper_infinite);
    sem_post(saladcount_sem);

    // closing and unlinking all semaphores used
    sem_close(tomato_infinite);
    sem_unlink("tomato saladmaker semaphore");
    sem_close(onion_infinite);
    sem_unlink("onion saladmaker semaphore");
    sem_close(pepper_infinite);
    sem_unlink("pepper saladmaker semaphore");
    sem_close(saladcount_sem);
    sem_unlink("saladcount");
    sem_close(concurrent_count_sem);
    sem_unlink("concurrentcount\n");
    sem_close(chef);
    sem_unlink("chef_semaphore\n");
    
    err = shmdt (shm); /* Detach segment */
	
	if (err == -1) 
		perror (" Detachment .");
	else 
		printf (" Detachment from shared memory.%d\n", err );

    return 0;
}
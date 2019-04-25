#include <stdio.h>
#include "writeOutput.h"
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>

//enum miner_type {IRON = 0, COPPER = 1, COAL = 2};

typedef struct miner{
    int id;
    int capacity;
    int mine_capacity;
    int ore_type;
    int time_interval;
}miner;

typedef struct transporter
{
    int id;
    int time_interval;
}transporter;

typedef struct smelter
{
    int id;
    int time_interval;
    int capacity;
    int ore_type;
}smelter;

typedef struct foundary{
    int id;
    int time_interval;
    int capacity;
}foundary;

int number_of_miners;
int number_of_transporters;
int number_of_smelters;
int number_of_foundaries;

miner *miners;
transporter *transporters;
smelter *smelters;
foundary *foundaries;

pthread_t *miner_threads;
pthread_t *transporter_threads;
pthread_t *smelter_threads;
pthread_t *foundary_threads;

sem_t *semaphores;
pthread_mutex_t *locks;

void getInput(void){
    scanf("%d",&number_of_miners);
    //printf("Number of miners = %d \n", number_of_miners);
    miners = (miner*)malloc(number_of_miners * sizeof(miner));
    //fflush(stdout);
    int temp1,temp2,temp3,temp4,i;
    for(i=0;i<number_of_miners;i++){
        scanf("%d %d %d %d",&temp1,&temp2,&temp3,&temp4);
        (miners+i)->id = i+1;
        (miners+i)->time_interval = temp1;
        (miners+i)->capacity = temp2;
        (miners+i)->ore_type = temp3;
        (miners+i)->mine_capacity = temp4;
        //printf("Miner %d created. %d %d %d %d\n", (miners+i)->id,(miners+i)->time_interval,(miners+i)->capacity,(miners+i)->ore_type,(miners+i)->mine_capacity);
    }
    scanf("%d",&number_of_transporters);
    transporters = (transporter*)malloc(number_of_transporters * sizeof(transporter));
    //printf("Number of transporters = %d \n", number_of_transporters);
    //fflush(stdout);
    for(i=0;i<number_of_transporters;i++){
        scanf("%d",&temp1);
        (transporters+i)->id = i+1;
        (transporters+i)->time_interval = temp1;
    }
    scanf("%d",&number_of_smelters);
    smelters = (smelter*)malloc(number_of_smelters * sizeof(smelter));
    //printf("Number of smelters = %d \n", number_of_smelters);
    //fflush(stdout);
    for(i=0;i<number_of_smelters;i++){
        scanf("%d %d %d",&temp1,&temp2,&temp3);
        (smelters+i)->id = i+1;
        (smelters+i)->time_interval = temp1;
        (smelters+i)->capacity = temp2;
        (smelters+i)->ore_type = temp3;
    }
    scanf("%d",&number_of_foundaries);
    foundaries = (foundary*)malloc(number_of_foundaries * sizeof(foundary));
    //printf("Number of foundaries = %d \n", number_of_foundaries);
    //fflush(stdout); 
    for(i=0;i<number_of_foundaries;i++){
        scanf("%d %d",&temp1,&temp2);
        (foundaries+i)->id = i+1;
        (foundaries+i)->time_interval = temp1;
        (foundaries+i)->capacity = temp2;
    }
    return;
}

void createSemaphores(void){
    semaphores = (sem_t*) malloc(number_of_miners * 2 * sizeof(sem_t));
    return;

}

void createThreads(void){
    // Allocate thread arrays to store threadids for each thread.
    miner_threads = (pthread_t*)malloc(number_of_miners * sizeof(pthread_t));
    transporter_threads = (pthread_t*)malloc(number_of_transporters * sizeof(pthread_t));
    smelter_threads = (pthread_t*)malloc(number_of_smelters * sizeof(pthread_t));
    foundary_threads = (pthread_t*)malloc(number_of_foundaries * sizeof(pthread_t));
}
/*
void *miner_thread(void *ID)
{
	int id = *(int *)ID;
    int mine_limit = miners[id].mine_capacity;
	int ore_count = 0 ;
    FillMinerInfo(MinerInfo,miners[id].id,miners[id].ore_type,miners[id].capacity,ore_count);
    WriteOutput(MinerInfo,NULL,NULL,NULL,MINER_CREATED);
    while(mine_limit--){
        sem_wait(&semaphores[id][0]); // wait for open position for new ore
        WriteOutput(MinerInfo, NULL, NULL, NULL,MINER_STARTED);
        lock(&(locks[id]));
        usleep(miner[id].time_interval);
        ore_count++;
        unlock(&(locks[id]));
        sem_post(&semaphores[id][1]);
        FillMinerInfo(MinerInfo,miners[id].id,miners[id].ore_type,miners[id].capacity,ore_count);
        WriteOutput(MinerInfo, NULL, NULL, NULL,MINER_FINISHED);
        usleep(miner[id].time_interval);
    }
    FillMinerInfo(MinerInfo,miners[id].id,miners[id].ore_type,miners[id].capacity,ore_count);
    WriteOutput(MinerInfo, NULL, NULL, NULL,MINER_STOPPED);
    pthread_exit(0);
}
*/

int main(void){
    getInput();
    createSemaphores();
    createThreads();
    waitThreads();
    return 0;
}
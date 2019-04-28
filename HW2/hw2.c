#include <stdio.h>
#include "writeOutput.h"
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>

//enum miner_type {IRON = 0, COPPER = 1, COAL = 2};

void *miner_thread(void*);
void *transporter_thread(void*);
int find_miner(void);

typedef struct miner{
    int id;
    int capacity;
    int mine_capacity;
    OreType ore_type;
    int time_interval;
    int ore_count;
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

sem_t *miner_semaphores;
pthread_mutex_t *miner_locks;

int miner_iterator=0;

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
        (miners+i)->ore_count = 0;
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
    miner_semaphores = (sem_t*) malloc(number_of_miners * sizeof(sem_t));
    miner_locks = (pthread_mutex_t*) malloc(number_of_miners*sizeof(pthread_mutex_t));
    int i;
    for(i=0;i<number_of_miners;i++){
            sem_init(miner_semaphores+i, 0, miners[i].capacity); //semaphore for signaling to miners
    }
    for(i=0;i<number_of_miners;i++){
        pthread_mutex_init(miner_locks+i, NULL);
    }
    return;

}

void createThreads(void){
    // Allocate thread arrays to store threadids for each thread.
    miner_threads = (pthread_t*)malloc(number_of_miners * sizeof(pthread_t));
    //printf("Allocated miner threads.\n");
    //fflush(stdout);
    transporter_threads = (pthread_t*)malloc(number_of_transporters * sizeof(pthread_t));
    //printf("Allocated transporter threads.\n");
    //fflush(stdout);
    smelter_threads = (pthread_t*)malloc(number_of_smelters * sizeof(pthread_t));
    //printf("Allocated smelter  threads.\n");
    //fflush(stdout);
    foundary_threads = (pthread_t*)malloc(number_of_foundaries * sizeof(pthread_t));
    int i;
    //printf("Allocated foundary threads.\n");
    //fflush(stdout);
    for(i=0;i<number_of_miners;i++){
        int *temp = (int*)malloc(sizeof(int*));
        *temp = i;
        pthread_create(miner_threads+i, NULL, miner_thread,(void*) temp);
    }
    
    for(i=0;i<number_of_transporters;i++){
        int *temp = (int*)malloc(sizeof(int*));
        *temp = i;
        pthread_create(transporter_threads+i, NULL, transporter_thread, temp);
    }
    /*
    for(i=0;i<number_of_smelters;i++){
        pthread_create(smelter_threads+i, NULL, smelter_thread, NULL);
    }
    for(i=0;i<number_of_foundaries;i++){
        pthread_create(foundary_threads+i, NULL, foundary_thread, NULL);
    }
    */
    return;
}

void *miner_thread(void *ID)
{
	int id = *(int *)ID;
    free(ID);
    int mine_limit = miners[id].mine_capacity;
    MinerInfo miner_info;
    FillMinerInfo(&miner_info,miners[id].id,miners[id].ore_type,miners[id].capacity,miners[id].ore_count);
    WriteOutput(&miner_info,NULL,NULL,NULL,MINER_CREATED);
    while(mine_limit--){
        sem_wait(miner_semaphores+id); // wait for open position for new ore
        //WriteOutput(&miner_info, NULL, NULL, NULL,MINER_STARTED);
        pthread_mutex_lock(miner_locks+id);
        usleep(miners[id].time_interval);
        (miners[id].ore_count)++;
        pthread_mutex_unlock(miner_locks+id);
        FillMinerInfo(&miner_info,miners[id].id,miners[id].ore_type,miners[id].capacity,miners[id].ore_count);
        //WriteOutput(&miner_info, NULL, NULL, NULL,MINER_FINISHED);
        usleep(miners[id].time_interval);
        //printf("Mine created t c %d\n",id);
        //fflush(stdout);
    }
    FillMinerInfo(&miner_info,miners[id].id,miners[id].ore_type,miners[id].capacity,miners[id].ore_count);
    WriteOutput(&miner_info, NULL, NULL, NULL,MINER_STOPPED);
    pthread_exit(0);
}

void *transporter_thread(void *ID){
    int id = *(int *)ID;
    free(ID);
    TransporterInfo transporter_info;
    MinerInfo miner_info;
    OreType *ore;
    FillTransporterInfo(&transporter_info,transporters[id].id,NULL);
    WriteOutput(NULL, &transporter_info, NULL, NULL,TRANSPORTER_CREATED);
    while(1){
        ore = NULL;
        int target_miner = find_miner();
        printf("%d \n",target_miner);
        // Miner routine starts
        FillMinerInfo(&miner_info, target_miner+1, 0, 0, 0);
        ore = &(miners[target_miner].ore_type);
        FillTransporterInfo(&transporter_info,transporters[id].id,NULL);
        WriteOutput(&miner_info, &transporter_info, NULL, NULL,TRANSPORTER_TRAVEL);
        pthread_mutex_lock(miner_locks+target_miner);
        miners[target_miner].ore_count--;
        pthread_mutex_unlock(miner_locks+target_miner);
        FillMinerInfo(&miner_info,miners[target_miner].id,miners[target_miner].ore_type,miners[target_miner].capacity,miners[target_miner].ore_count);
        FillTransporterInfo(&transporter_info,transporters[id].id,ore);
        //WriteOutput(&miner_info, &transporter_info, NULL, NULL,TRANSPORTER_TAKE_ORE);
        usleep(transporters[id].time_interval);
        sem_post(miner_semaphores+target_miner);
    }
    pthread_exit(0);

}

int find_miner(void){
    int i;
    pthread_mutex_t* temp;
    int flag = 0;
    printf("Miner iterator: %d \n",miner_iterator);
    if(miner_iterator == 1){
        miner_iterator = -1;
    }
    for(i=miner_iterator+1;i<number_of_miners;i++){
        temp = miner_locks+miner_iterator;
        pthread_mutex_lock(temp);
        int temp_ore_count = miners[i].ore_count;
        if(temp_ore_count){
            miner_iterator = i;
            flag = 1;
        }
        if(i == number_of_miners-1){
            i = 0;
        }
        if(flag){
            pthread_mutex_unlock(temp);
            break;
        }
        pthread_mutex_unlock(temp);
    }
    //printf("%d \n",i);
    return i;
}

void waitThreads(void){
    int i;
    for(i=0;i<number_of_miners;i++){
        pthread_join(miner_threads[i], NULL);
    }
    return;
}


int main(void){
    getInput();
    //printf("Creating semaphores.\n");
    //fflush(stdout);
    createSemaphores();
    //printf("Creating threads.\n");
    //fflush(stdout);
    InitWriteOutput();
    createThreads();
    //printf("Created threads\n");
    //fflush(stdout);
    waitThreads();
    
    return 0;
}
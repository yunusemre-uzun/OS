#include <stdio.h>
#include "writeOutput.h"
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
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

typedef struct foundry{
    int id;
    int time_interval;
    int capacity;
    int waiting_iron_count;
    int waiting_coal_count;
    int produced_ignot_count;
}foundry;

int number_of_miners;
int number_of_transporters;
int number_of_smelters;
int number_of_foundaries;

miner *miners;
transporter *transporters;
smelter *smelters;
foundry *foundaries;

pthread_t *miner_threads;
pthread_t *transporter_threads;
pthread_t *smelter_threads;
pthread_t *foundry_threads;

sem_t *miner_semaphores;
sem_t *foundry_semaphores;
sem_t *waiter_semaphores;
pthread_mutex_t *miner_locks;
pthread_mutex_t *foundry_locks;

int miner_iterator=0;
int all_miners_are_stopped = 0;

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
    foundaries = (foundry*)malloc(number_of_foundaries * sizeof(foundry));
    //printf("Number of foundaries = %d \n", number_of_foundaries);
    //fflush(stdout); 
    for(i=0;i<number_of_foundaries;i++){
        scanf("%d %d",&temp1,&temp2);
        (foundaries+i)->id = i+1;
        (foundaries+i)->time_interval = temp1;
        (foundaries+i)->capacity = temp2;
        (foundaries+i)->waiting_coal_count = 0;
        (foundaries+i)->waiting_iron_count = 0;
        (foundaries+i)->produced_ignot_count = 0;
    }
    return;
}

void createSemaphores(void){
    miner_semaphores = (sem_t*) malloc(number_of_miners * sizeof(sem_t));
    foundry_semaphores = (sem_t *) malloc(number_of_foundaries*2*sizeof(sem_t));
    waiter_semaphores = (sem_t *) malloc(number_of_foundaries*sizeof(sem_t));
    miner_locks = (pthread_mutex_t*) malloc(number_of_miners*sizeof(pthread_mutex_t));
    foundry_locks = (pthread_mutex_t *) malloc(number_of_foundaries*sizeof(pthread_mutex_t));
    int i;
    for(i=0;i<number_of_miners;i++){
        sem_init(miner_semaphores+i, 0, miners[i].capacity); //semaphore for signaling to miners
    }
    for(i=0;i<number_of_miners;i++){
        pthread_mutex_init(miner_locks+i, NULL);
    }
    for(i=0;i<number_of_foundaries;i++){
        pthread_mutex_init(foundry_locks+i, NULL);
    }
    for(i=0;i<number_of_foundaries*2;i++){
        sem_init(foundry_semaphores+i, 0, 0);
    }
    
    return;

}

void createThreads(void){
    // Allocate thread arrays to store threadids for each thread.
    miner_threads = (pthread_t *)malloc(number_of_miners * sizeof(pthread_t));
    //printf("Allocated miner threads.\n");
    //fflush(stdout);
    transporter_threads = (pthread_t*)malloc(number_of_transporters * sizeof(pthread_t));
    //printf("Allocated transporter threads.\n");
    //fflush(stdout);
    smelter_threads = (pthread_t*)malloc(number_of_smelters * sizeof(pthread_t));
    //printf("Allocated smelter  threads.\n");
    //fflush(stdout);
    foundry_threads = (pthread_t*)malloc(number_of_foundaries * sizeof(pthread_t));
    //printf("Allocated foundry threads.\n");
    //fflush(stdout);
    int i;
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
        pthread_create(foundry_threads+i, NULL, foundry_thread, NULL);
    }
    */
    return;
}

void *miner_thread(void *ID){
	int index = *(int *)ID; // index is the place for miner in the miners array
    free(ID);
    miner *temp_miner = miners+index;
    int mine_limit = temp_miner->mine_capacity;
    MinerInfo miner_info;
    FillMinerInfo(&miner_info,temp_miner->id,temp_miner->ore_type,temp_miner->capacity,temp_miner->ore_count);
    WriteOutput(&miner_info,NULL,NULL,NULL,MINER_CREATED);
    while(mine_limit--){
        sem_wait(miner_semaphores+index); // wait for open position for new ore
        WriteOutput(&miner_info, NULL, NULL, NULL,MINER_STARTED);
        pthread_mutex_lock(miner_locks+index);
        usleep(temp_miner->time_interval);
        (temp_miner->ore_count)++;
        pthread_mutex_unlock(miner_locks+index);
        FillMinerInfo(&miner_info,temp_miner->id,temp_miner->ore_type,temp_miner->capacity,temp_miner->ore_count);
        WriteOutput(&miner_info, NULL, NULL, NULL,MINER_FINISHED);
        usleep(temp_miner->time_interval);
    }
    FillMinerInfo(&miner_info,temp_miner->id,temp_miner->ore_type,temp_miner->capacity,temp_miner->ore_count);
    WriteOutput(&miner_info, NULL, NULL, NULL,MINER_STOPPED);
    pthread_exit(0);
}

void *transporter_thread(void *ID){
    int id = *(int *)ID;
    free(ID);
    TransporterInfo transporter_info;
    MinerInfo miner_info;
    OreType *ore = NULL;
    FillTransporterInfo(&transporter_info,transporters[id].id,ore);
    WriteOutput(NULL, &transporter_info, NULL, NULL,TRANSPORTER_CREATED);
    while(1){
        int target_miner_index = find_miner();
        if(target_miner_index != -1){ // Miner routine starts
            miner *target_miner = miners+target_miner_index;
            FillMinerInfo(&miner_info, target_miner->id, 0, 0, 0);
            ore = &(miners[target_miner_index].ore_type);
            FillTransporterInfo(&transporter_info,transporters[id].id,NULL);
            WriteOutput(&miner_info, &transporter_info, NULL, NULL,TRANSPORTER_TRAVEL);
            pthread_mutex_lock(miner_locks+target_miner_index);
            target_miner->ore_count--;
            pthread_mutex_unlock(miner_locks+target_miner_index);
            FillMinerInfo(&miner_info,target_miner->id,target_miner->ore_type,target_miner->capacity,target_miner->ore_count);
            FillTransporterInfo(&transporter_info,transporters[id].id,ore);
            WriteOutput(&miner_info, &transporter_info, NULL, NULL,TRANSPORTER_TAKE_ORE);
            usleep(transporters[id].time_interval);
            sem_post(miner_semaphores+target_miner_index);
        }
        else{
            break;
        }
    }
    pthread_exit(0);
}

void *wait_thread(void* inp){
    int index = *(int *)inp; // index is the place for miner in the foundaries array
    free(inp);
    sem_wait(foundry_semaphores+index*2);
    sem_wait(foundry_semaphores+index*2+1);
    sem_post(waiter_semaphores+index);
    pthread_exit(0);
}

void *foundry_thread(void *ID){
	int index = *(int *)ID; // index is the place for miner in the foundaries array
    pthread_t waiter_thread_id;
    pthread_create(&waiter_thread_id, NULL, wait_thread,(void*) ID);
    free(ID);
    int flag = 0;
    clock_t start, end;
    struct timespec ts;
    foundry *temp_foundry = foundaries+index;
    FoundryInfo foundry_info;
    TransporterInfo transporter_info;
    FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
    WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_CREATED);
    start = clock();
    while(1){
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        if(sem_timedwait(waiter_semaphores+index, &ts)==-1) break;
        pthread_mutex_lock(foundry_locks+index);
        temp_foundry->waiting_iron_count--;
        temp_foundry->waiting_coal_count--;
        FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
        WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_STARTED);
        usleep(temp_foundry->time_interval);
        temp_foundry->produced_ignot_count++;
        FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
        WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_FINISHED);
        pthread_mutex_unlock(foundry_locks+index);
    }
    FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
    WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_STOPPED);
    
}

int find_miner(void){
    int i;
    pthread_mutex_t* temp;
    int flag = 0;
    if(miner_iterator == 2)
        miner_iterator = 0;
    for(i=miner_iterator;i<number_of_miners;i++){
        if(flag >= 2 && all_miners_are_stopped){
            return -1;
        }
        temp = miner_locks+miner_iterator;
        pthread_mutex_lock(temp);
        int temp_ore_count = miners[i].ore_count;
        if(temp_ore_count > 0){
            pthread_mutex_unlock(temp);
            miner_iterator = i+1;
            break;
        }
        pthread_mutex_unlock(temp);
        if(i == number_of_miners-1)
            i = -1;
            flag++;
    }
    return i;
}

void waitThreads(void){
    int i;
    for(i=0;i<number_of_miners;i++){
        pthread_join(miner_threads[i], NULL);
    }
    all_miners_are_stopped = 1;
    for(i=0;i<number_of_transporters;i++){
        pthread_join(transporter_threads[i], NULL);
    }
    return;
}


int main(void){
    getInput();
    createSemaphores();
    InitWriteOutput();
    createThreads();
    waitThreads();
    
    return 0;
}
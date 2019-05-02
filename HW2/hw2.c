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
void *foundry_thread(void*);
void *smelter_thread(void*);
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
    int waiting_ore_count;
    int produced_ignot_count;
}smelter;

typedef struct foundry{
    int id;
    int time_interval;
    int capacity;
    int waiting_iron_count;
    int waiting_coal_count;
    int produced_ignot_count;
}foundry;

typedef struct target_producer{
    int index;
    int producer_type; // 0 for smelter, 1 for foundry
}target_producer;

int number_of_miners;
int number_of_transporters;
int number_of_smelters;
int number_of_foundries;

miner *miners;
transporter *transporters;
smelter *smelters;
foundry *foundries;

pthread_t *miner_threads;
pthread_t *transporter_threads;
pthread_t *smelter_threads;
pthread_t *foundry_threads;

sem_t *miner_semaphores;
sem_t *foundry_semaphores;
sem_t *smelter_semaphores;
sem_t *transporter_semaphore;
sem_t *waiter_foundry_semaphores;
sem_t *waiter_smelter_semaphores;
pthread_mutex_t *miner_locks;
pthread_mutex_t *foundry_locks;
pthread_mutex_t *smelter_locks;
pthread_mutex_t *transporter_lock;

int miner_iterator=0;
int all_miners_are_stopped = 0;
int all_smelters_are_stopped = 0;
int all_foundries_are_stopped = 0;
int *miners_alive;
int *smelters_alive;
int *foundries_alive;

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
        (smelters+i)->produced_ignot_count = 0;
        (smelters+i)->waiting_ore_count = 0;
    }
    scanf("%d",&number_of_foundries);
    foundries = (foundry*)malloc(number_of_foundries * sizeof(foundry));
    //printf("Number of foundries = %d \n", number_of_foundries);
    //fflush(stdout); 
    for(i=0;i<number_of_foundries;i++){
        scanf("%d %d",&temp1,&temp2);
        (foundries+i)->id = i+1;
        (foundries+i)->time_interval = temp1;
        (foundries+i)->capacity = temp2;
        (foundries+i)->waiting_coal_count = 0;
        (foundries+i)->waiting_iron_count = 0;
        (foundries+i)->produced_ignot_count = 0;
    }
    return;
}

void createSemaphores(void){
    miner_semaphores = (sem_t*) malloc(number_of_miners * sizeof(sem_t));
    foundry_semaphores = (sem_t *) malloc(number_of_foundries*2*sizeof(sem_t));
    smelter_semaphores = (sem_t *) malloc(number_of_smelters*sizeof(sem_t));
    transporter_semaphore = (sem_t *) malloc(sizeof(sem_t));
    waiter_foundry_semaphores = (sem_t *) malloc(number_of_foundries*sizeof(sem_t));
    waiter_smelter_semaphores = (sem_t *) malloc(number_of_smelters*sizeof(sem_t));
    miner_locks = (pthread_mutex_t*) malloc(number_of_miners*sizeof(pthread_mutex_t));
    foundry_locks = (pthread_mutex_t *) malloc(number_of_foundries*sizeof(pthread_mutex_t));
    transporter_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    smelter_locks = (pthread_mutex_t *) malloc(number_of_smelters*sizeof(pthread_mutex_t));
    int i;
    for(i=0;i<number_of_miners;i++){
        sem_init(miner_semaphores+i, 0, miners[i].capacity); //semaphore for signaling to miners
    }
    for(i=0;i<number_of_miners;i++){
        pthread_mutex_init(miner_locks+i, NULL); //locks for locking miners
    }
    for(i=0;i<number_of_foundries;i++){
        pthread_mutex_init(foundry_locks+i, NULL); //locks for locking foundries
    }
    for(i=0;i<number_of_foundries*2;i++){
        sem_init(foundry_semaphores+i, 0, 0); // semaphores for foundries
    }
    for(i=0;i<number_of_foundries;i++){
        sem_init(waiter_foundry_semaphores+i, 0, 0);
    }
    for(i=0;i<number_of_smelters;i++){
        sem_init(waiter_smelter_semaphores+i, 0, 0);
    }
    for(i=0;i<number_of_smelters;i++){
        sem_init(smelter_semaphores+i, 0, 0);
    }
    for(i=0;i<number_of_smelters;i++){
        pthread_mutex_init(smelter_locks+i, NULL);
    }
    int total_capacity=0;
    for(i=0;i<number_of_foundries;i++){
        total_capacity += (foundries+i)->capacity;
    }
    for(i=0;i<number_of_smelters;i++){
        total_capacity += (smelters+i)->capacity;
    }
    sem_init(transporter_semaphore, 0, total_capacity); 
    //smaphore for signaling transporters, initial value is the total capacity of smelter and foundries
    pthread_mutex_init(transporter_lock, NULL); //locks for locking transporters
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
    foundry_threads = (pthread_t*)malloc(number_of_foundries * sizeof(pthread_t));
    //printf("Allocated foundry threads.\n");
    //fflush(stdout);
    miners_alive = (int *) malloc(number_of_miners*sizeof(int));
    smelters_alive = (int *) malloc(number_of_smelters*sizeof(int));
    foundries_alive = (int *) malloc(number_of_foundries*sizeof(int));
    int i;
    for(i=0;i<number_of_miners;i++){
        int *temp = (int*)malloc(sizeof(int*));
        *temp = i;
        pthread_create(miner_threads+i, NULL, miner_thread,(void*) temp);
        miners_alive[i] = 1;
    }
    
    for(i=0;i<number_of_transporters;i++){
        int *temp = (int*)malloc(sizeof(int*));
        *temp = i;
        pthread_create(transporter_threads+i, NULL, transporter_thread, temp);
    }
    for(i=0;i<number_of_foundries;i++){
        int *temp = (int*)malloc(sizeof(int*));
        *temp = i;
        pthread_create(foundry_threads+i, NULL, foundry_thread, temp);
        foundries_alive[i] = 1;
    }
    for(i=0;i<number_of_smelters;i++){
        int *temp = (int*)malloc(sizeof(int*));
        *temp = i;
        pthread_create(smelter_threads+i, NULL, smelter_thread, temp);
        smelters_alive[i] = 1;
    }
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
        usleep(temp_miner->time_interval-((temp_miner->time_interval)*0.01) + (rand()%(int)(temp_miner->time_interval*0.02)));
        (temp_miner->ore_count)++;
        pthread_mutex_unlock(miner_locks+index);
        FillMinerInfo(&miner_info,temp_miner->id,temp_miner->ore_type,temp_miner->capacity,temp_miner->ore_count);
        WriteOutput(&miner_info, NULL, NULL, NULL,MINER_FINISHED);
        usleep(temp_miner->time_interval-((temp_miner->time_interval)*0.01) + (rand()%(int)(temp_miner->time_interval*0.02)));
    }
    FillMinerInfo(&miner_info,temp_miner->id,temp_miner->ore_type,temp_miner->capacity,temp_miner->ore_count);
    WriteOutput(&miner_info, NULL, NULL, NULL,MINER_STOPPED);
    miners_alive[index] = 0;
    pthread_exit(0);
}

target_producer find_target_producer(OreType ore){
    target_producer ret;
    int i;
    int flag = 0;
    for(i=0;i<number_of_smelters;i++){
        pthread_mutex_lock(smelter_locks+i); //wait for the lock of specific smelter
        if(!flag && (smelters+i)->waiting_ore_count==1 && ore==((smelters+i)->ore_type) && smelters_alive[i]){ 
            // if flag is not set and there is an ore in the store of the smelter and ore type of the smelter is the same with transporter
            ret.index = i;
            ret.producer_type = 0; //smelter
            flag = 1;
        }
        pthread_mutex_unlock(smelter_locks+i); //release for the lock of specific smelter
    }
    if(flag)
        return ret;
    for(i=0;i<number_of_foundries;i++){
        pthread_mutex_lock(foundry_locks+i); //wait for the lock of specific smelter
        if(!flag && (foundries+i)->waiting_coal_count>=1 && ore==IRON && foundries_alive[i]){ 
            //if flag is not set and there is coal in the store of the foundary and transporter carries iron
            ret.index = i;
            ret.producer_type = 1; //foundry
            flag = 1;
        }
        pthread_mutex_unlock(foundry_locks+i); //wait for the lock of specific smelter
    }
    if(flag)
        return ret;
    for(i=0;i<number_of_foundries;i++){
        pthread_mutex_lock(foundry_locks+i); //wait for the lock of specific smelter
        if(!flag && (foundries+i)->waiting_iron_count>=1 && ore==COAL && foundries_alive[i]){ 
            //if flag is not set and there is iron in the store of the foundary and transporter carries coal
            ret.index = i;
            ret.producer_type = 1; //foundry
            flag = 1;
        }
        pthread_mutex_unlock(foundry_locks+i); //wait for the lock of specific smelter
    }
    if(flag)
        return ret;
    for(i=0;i<number_of_smelters;i++){
        pthread_mutex_lock(smelter_locks+i); //wait for the lock of specific smelter
        if(!flag && (smelters+i)->waiting_ore_count == 0 && ore==(smelters+i)->ore_type && smelters_alive[i]){
            ret.index = i;
            ret.producer_type = 0; //smelter
            flag = 1;
        }
        pthread_mutex_unlock(smelter_locks+i); //wait for the lock of specific smelter
    }
    if(flag)
        return ret;
    for(i=0;i<number_of_foundries;i++){
        pthread_mutex_lock(foundry_locks+i); //wait for the lock of specific smelter
        if(!flag && (foundries+i)->waiting_coal_count == 0 && (foundries+i)->waiting_iron_count == 0 
            && (ore==IRON || ore==COAL) && foundries_alive[i]){
            ret.index = i;
            ret.producer_type = 1; //foundary
            flag = 1;
        }
        pthread_mutex_unlock(foundry_locks+i); //wait for the lock of specific smelter
    }
    if(flag)
        return ret;
    for(i=0;i<number_of_smelters;i++){
        pthread_mutex_lock(smelter_locks+i); //wait for the lock of specific smelter
        if(!flag && ((smelters+i)->waiting_ore_count != (smelters+i)->capacity) && ore==(smelters+i)->ore_type && smelters_alive[i]){
            ret.index = i;
            ret.producer_type = 1; //foundary
            flag = 1;
        }
        pthread_mutex_unlock(smelter_locks+i); //wait for the lock of specific smelter
    }
    if(flag)
        return ret;
    for(i=0;i<number_of_foundries;i++){
        pthread_mutex_lock(foundry_locks+i); //wait for the lock of specific smelter
        if(!flag && ((foundries+i)->waiting_coal_count+(foundries+i)->waiting_iron_count != (foundries+i)->capacity) 
            && (ore==IRON || ore==COAL) && foundries_alive[i]){
            ret.index = i;
            ret.producer_type = 1; //foundary
            flag = 1;
        }
        pthread_mutex_unlock(foundry_locks+i); //wait for the lock of specific smelter
    }
    if(flag)
        return ret;
    ret.index = -1;
    ret.producer_type = -1;
    return ret;
}

void *transporter_thread(void *ID){
    int id = *(int *)ID;
    free(ID);
    TransporterInfo transporter_info;
    MinerInfo miner_info;
    OreType *ore = NULL;
    int flag=0;
    FillTransporterInfo(&transporter_info,transporters[id].id,ore);
    WriteOutput(NULL, &transporter_info, NULL, NULL,TRANSPORTER_CREATED);
    while(1){
        if(all_miners_are_stopped && all_foundries_are_stopped && all_smelters_are_stopped)
            break;
        if(flag)
            ore = NULL;
        flag = 0;
        int target_miner_index = find_miner();
        if(target_miner_index != -1){ // Miner routine starts
            miner *target_miner = miners+target_miner_index;
            FillMinerInfo(&miner_info, target_miner->id, 0, 0, 0);
            ore = &(miners[target_miner_index].ore_type);
            FillTransporterInfo(&transporter_info,transporters[id].id,NULL);
            //printf("%d \n",target_miner->id);
            WriteOutput(&miner_info, &transporter_info, NULL, NULL,TRANSPORTER_TRAVEL);
            pthread_mutex_lock(miner_locks+target_miner_index);
            target_miner->ore_count--;
            FillMinerInfo(&miner_info,target_miner->id,target_miner->ore_type,target_miner->capacity,target_miner->ore_count);
            pthread_mutex_unlock(miner_locks+target_miner_index);
            FillTransporterInfo(&transporter_info,transporters[id].id,ore);
            WriteOutput(&miner_info, &transporter_info, NULL, NULL,TRANSPORTER_TAKE_ORE);
            usleep(transporters[id].time_interval - (transporters[id].time_interval*0.01) + (rand()%(int)(transporters[id].time_interval*0.02)));
            sem_post(miner_semaphores+target_miner_index);
        }
        else{
            continue;
        }
        //printf("anna \n");
        sem_wait(transporter_semaphore); // if all stores of producers are full
        target_producer target_producer_temp;
        target_producer_temp = find_target_producer(*ore);
        //printf("carenina \n");
        //printf("Target producer type:%d, id:%d \n",target_producer_temp.producer_type,target_producer_temp.producer_type);
        //sem_post(transporter_semaphore);
        //ore = NULL;
        //printf("carenina \n");
        if(target_producer_temp.index != -1 && target_producer_temp.producer_type == 0){
            SmelterInfo smelter_info;
            int smelter_index = target_producer_temp.index;
            FillSmelterInfo(&smelter_info, (smelters+smelter_index)->id, 0, 0, 0,0);
            FillTransporterInfo(&transporter_info,transporters[id].id,ore);
            WriteOutput(NULL, &transporter_info, &smelter_info, NULL,TRANSPORTER_TRAVEL);
            usleep(transporters[id].time_interval - (transporters[id].time_interval*0.01) + (rand()%(int)(transporters[id].time_interval*0.02)));
            pthread_mutex_lock(smelter_locks+target_producer_temp.index);
            (smelters+smelter_index)->waiting_ore_count++;
            FillSmelterInfo(&smelter_info,(smelters+smelter_index)->id,(smelters+smelter_index)->ore_type,(smelters+smelter_index)->capacity,
                            (smelters+smelter_index)->waiting_ore_count,(smelters+smelter_index)->produced_ignot_count);
            FillTransporterInfo(&transporter_info,transporters[id].id,ore);
            WriteOutput(NULL, &transporter_info, &smelter_info, NULL,TRANSPORTER_DROP_ORE);
            usleep(transporters[id].time_interval - (transporters[id].time_interval*0.01) + (rand()%(int)(transporters[id].time_interval*0.02)));
            sem_post(smelter_semaphores+smelter_index);
            pthread_mutex_unlock(smelter_locks+target_producer_temp.index);
            flag = 1;
        }
        else if(target_producer_temp.index != -1 && target_producer_temp.producer_type == 1){
            FoundryInfo foundry_info;
            int foundry_index = target_producer_temp.index;
            FillFoundryInfo(&foundry_info, (foundries+foundry_index)->id, 0, 0, 0,0);
            FillTransporterInfo(&transporter_info,transporters[id].id,ore);
            WriteOutput(NULL, &transporter_info, NULL, &foundry_info,TRANSPORTER_TRAVEL);
            usleep(transporters[id].time_interval - (transporters[id].time_interval*0.01) + (rand()%(int)(transporters[id].time_interval*0.02)));
            pthread_mutex_lock(foundry_locks+target_producer_temp.index);
            if(*ore==IRON)
                (foundries+foundry_index)->waiting_iron_count++;
            else
                (foundries+foundry_index)->waiting_coal_count++;
            FillFoundryInfo(&foundry_info,(foundries+foundry_index)->id,(foundries+foundry_index)->capacity,(foundries+foundry_index)->waiting_iron_count,
                            (foundries+foundry_index)->waiting_coal_count,(foundries+foundry_index)->produced_ignot_count);
            FillTransporterInfo(&transporter_info,transporters[id].id,ore);
            WriteOutput(NULL, &transporter_info,NULL, &foundry_info,TRANSPORTER_DROP_ORE);
            usleep(transporters[id].time_interval - (transporters[id].time_interval*0.01) + (rand()%(int)(transporters[id].time_interval*0.02)));
            if(*ore==IRON) {  
                sem_post(foundry_semaphores+foundry_index*2);}
            else{
                sem_post(foundry_semaphores+foundry_index*2+1);}
            pthread_mutex_unlock(foundry_locks+target_producer_temp.index);
            flag = 1;
        }
    }
    FillTransporterInfo(&transporter_info,transporters[id].id,ore);
    WriteOutput(NULL, &transporter_info,NULL, NULL,TRANSPORTER_STOPPED);
    pthread_exit(0);
}

void *wait_foundry_thread(void* inp){
    int index = *(int *)inp; // index is the place for miner in the foundries array
    free(inp);
    while(1)
    {
        sem_wait(foundry_semaphores+index*2);
        sem_wait(foundry_semaphores+index*2+1);
        sem_post(waiter_foundry_semaphores+index);
    }
    pthread_exit(0);
}

void *foundry_thread(void *ID){
	int index = *(int *)ID; // index is the place for foundrt in the foundries array
    free(ID);
    int *temp = (int*) malloc(sizeof(int));
    *temp = index;
    int flag = 0;
    pthread_t waiter_thread_id;
    pthread_create(&waiter_thread_id, NULL, wait_foundry_thread,(void*) temp);
    //int iter = 0;
    clock_t start, end;
    struct timespec ts;
    foundry *temp_foundry = foundries+index;
    FoundryInfo foundry_info;
    TransporterInfo transporter_info;
    FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
    WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_CREATED);
    start = clock();
    while(1){
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        if(sem_timedwait(waiter_foundry_semaphores+index, &ts)==-1) break;
        //printf("Foundry iteration: %d \n",++iter);
        pthread_mutex_lock(foundry_locks+index);
        temp_foundry->waiting_iron_count--;
        temp_foundry->waiting_coal_count--;
        FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
        WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_STARTED);
        usleep(temp_foundry->time_interval-((temp_foundry->time_interval)*0.01) + (rand()%(int)(temp_foundry->time_interval*0.02)));
        temp_foundry->produced_ignot_count++;
        FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
        WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_FINISHED);
        pthread_mutex_unlock(foundry_locks+index);
        sem_post(transporter_semaphore);
        sem_post(transporter_semaphore);
    }
    FillFoundryInfo(&foundry_info,temp_foundry->id,temp_foundry->capacity,temp_foundry->waiting_iron_count,temp_foundry->waiting_coal_count,temp_foundry->produced_ignot_count);
    WriteOutput(NULL, NULL, NULL, &foundry_info,FOUNDRY_STOPPED);
    pthread_cancel(waiter_thread_id);
    foundries_alive[index] = 0;
    pthread_exit(0);
}

void *wait_smelter_thread(void* inp){
    int index = *(int *)inp; // index is the place for miner in the foundries array
    free(inp);
    while(1){
        sem_wait(smelter_semaphores+index);
        sem_wait(smelter_semaphores+index);
        sem_post(waiter_smelter_semaphores+index);
    }
    pthread_exit(0);
}

void *smelter_thread(void *ID){
	int index = *(int *)ID; // index is the place for foundrt in the foundries array
    free(ID);
    int *temp = (int*) malloc(sizeof(int));
    *temp = index;
    pthread_t waiter_thread_id;
    pthread_create(&waiter_thread_id, NULL, wait_smelter_thread,(void*) temp);
    int flag = 0;
    clock_t start, end;
    struct timespec ts;
    smelter *temp_smelter = smelters+index;
    SmelterInfo smelter_info;
    TransporterInfo transporter_info;
    FillSmelterInfo(&smelter_info,temp_smelter->id,temp_smelter->ore_type,temp_smelter->capacity,temp_smelter->waiting_ore_count,temp_smelter->produced_ignot_count);
    WriteOutput(NULL, NULL, &smelter_info, NULL,SMELTER_CREATED);
    start = clock();
    while(1){
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        if(sem_timedwait(waiter_smelter_semaphores+index, &ts)==-1) break;
        pthread_mutex_lock(smelter_locks+index);
        temp_smelter->waiting_ore_count--;
        temp_smelter->waiting_ore_count--;
        FillSmelterInfo(&smelter_info,temp_smelter->id,temp_smelter->ore_type,temp_smelter->capacity,temp_smelter->waiting_ore_count,temp_smelter->produced_ignot_count);
        WriteOutput(NULL, NULL, &smelter_info,NULL ,SMELTER_STARTED);
        usleep(temp_smelter->time_interval-((temp_smelter->time_interval)*0.01) + (rand()%(int)(temp_smelter->time_interval*0.02)));
        temp_smelter->produced_ignot_count++;
        FillSmelterInfo(&smelter_info,temp_smelter->id,temp_smelter->ore_type,temp_smelter->capacity,temp_smelter->waiting_ore_count,temp_smelter->produced_ignot_count);
        WriteOutput(NULL, NULL, &smelter_info,NULL,SMELTER_FINISHED);
        pthread_mutex_unlock(smelter_locks+index);
        sem_post(transporter_semaphore);
        sem_post(transporter_semaphore);
    }
    FillSmelterInfo(&smelter_info,temp_smelter->id,temp_smelter->ore_type,temp_smelter->capacity,temp_smelter->waiting_ore_count,temp_smelter->produced_ignot_count);
    WriteOutput(NULL, NULL, &smelter_info,NULL,SMELTER_STOPPED);
    pthread_cancel(waiter_thread_id);
    smelters_alive[index] = 0;
    pthread_exit(0);
}

void printAllMiners(void){
    int i=0;
    for(i=0;i<number_of_miners;i++){
        if(miners_alive[i]){
            printf("Miner %d %d %d %d %d\n", (miners+i)->id,(miners+i)->time_interval,(miners+i)->capacity,(miners+i)->ore_type,(miners+i)->mine_capacity);
        }
        else{
            printf("Miner %d stopped. \n", (miners+i)->id);
        }
    }
}

int find_miner(void){
    //printAllMiners();
    int i;
    pthread_mutex_t* temp;
    int flag = 0;
    if(miner_iterator == number_of_miners)
        miner_iterator = 0;
    for(i=miner_iterator;i<number_of_miners;i++){
        if(flag >= 2 && all_miners_are_stopped){
            return -1;
        }
        if(miners_alive[i]==0 && miners[i].ore_count==0){
            goto end;
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
        end:
            if(i == number_of_miners-1)
                i = -1;
                flag++;
    }
    //printf("%d %d \n",i,(miners+2)->id);
    return i;
}

void waitThreads(void){
    int i;
    for(i=0;i<number_of_miners;i++){
        pthread_join(miner_threads[i], NULL);
    }
    all_miners_are_stopped = 1;
    for(i=0;i<number_of_foundries;i++){
       pthread_join(foundry_threads[i], NULL);
    }
    all_foundries_are_stopped = 1;
    for(i=0;i<number_of_smelters;i++){
        pthread_join(smelter_threads[i], NULL);
    }
    all_smelters_are_stopped = 1;
    //printf("%d %d %d \n",all_miners_are_stopped,all_foundries_are_stopped,all_smelters_are_stopped);
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>



sem_t* Imm_queue;
sem_t* Imm_regis;
sem_t* Judge_out;
sem_t* Certificate;
sem_t* Write;


typedef struct SharedMemory {
    int A; //counter
    int NE; // entered
    int NC; //registered
    int NB; //inBuilding
    int I;  //Intern indicator

    int PI; // number of processes
    int IG; //Imigrant generate rate
    int JG; // Judge wants to enter building
    int IT; // Certificate pickup time
    int JT; // Judge meeting time

    int IMM_certified;

    FILE* print;
}SharedMemory;


void mysleep(int time)
{
  if(time > 0)
    usleep((rand() % (time + 1)) * 1000);
}


void clean(SharedMemory* SharedMem)
{
    fclose(SharedMem->print);
    munmap(SharedMem ,sizeof(SharedMemory));
    sem_close(Imm_queue);
    sem_unlink("xkvasn14_semaphore_imigrant_queue");
    sem_close(Imm_regis);
    sem_unlink("xkvasn14_semaphore_imigrant_register");
    sem_close(Judge_out);
    sem_unlink("xkvasn14_semaphore_judge_out");
    sem_close(Certificate);
    sem_unlink("xkvasn14_semaphore_certificate");
    sem_close(Write);
    sem_unlink("xkvasn14_semaphore_write");
}








int Imigrants_gen(int PI, int IT, int IG, SharedMemory* SharedMem, int killID)
{
    for (int i = 1; i <= PI; i++)
    {   

        int pid = fork();
        if(pid<0)
        {
            fprintf(stderr,"Fork error");
            clean(SharedMem);
            kill(killID,SIGKILL);
        }
        else if(pid == 0)
        {
            
            //BEGIN
            sem_wait(Write);
            SharedMem->A++;
            sem_post(Write);
            sem_wait(Write);
            fprintf(SharedMem->print, "%d: IMM %d: starts\n",SharedMem->A,i);
            sem_post(Write);
            sem_wait(Imm_queue);
            

            //ENTERING BUILDING
            sem_wait(Judge_out);
            sem_post(Judge_out);
            sem_wait(Write);
            SharedMem->A++;
            SharedMem->NE++;
            SharedMem->NB++;
            sem_post(Write);
            sem_wait(Write);
            fprintf(SharedMem->print, "%d: IMM %d: enters: %d: %d: %d\n",SharedMem->A,i,SharedMem->NE,SharedMem->NC,SharedMem->NB);
            sem_post(Write);
            

            //REGISTRATION
            sem_post(Imm_queue);
            sem_wait(Imm_regis);
            sem_wait(Write);
            SharedMem->A++;
            SharedMem->NC++;
            sem_post(Write);
            sem_wait(Write);
            fprintf(SharedMem->print, "%d: IMM %d: checks: %d: %d: %d\n",SharedMem->A,i,SharedMem->NE,SharedMem->NC,SharedMem->NB);
            sem_post(Write);
            sem_post(Imm_regis);


            //CERTIFICATION
            sem_wait(Certificate);
            sem_post(Certificate);
            sem_wait(Write);
            SharedMem->A++;
            sem_post(Write);
            sem_wait(Write);
            fprintf(SharedMem->print, "%d: IMM %d: wants certificate: %d: %d: %d\n",SharedMem->A,i,SharedMem->NE,SharedMem->NC,SharedMem->NB);
            sem_post(Write);

            //GETTING CERTIFICATE
            mysleep(IT);
            sem_wait(Write);
            SharedMem->A++;
            sem_post(Write);
            sem_wait(Write);
            fprintf(SharedMem->print, "%d: IMM %d: got certificate: %d: %d: %d\n",SharedMem->A,i,SharedMem->NE,SharedMem->NC,SharedMem->NB);
            sem_post(Write);


            //LEAVING THE BUILDING
            sem_wait(Judge_out);
            sem_post(Judge_out);
            sem_wait(Write);
            SharedMem->A++;
            SharedMem->NB--;
            sem_post(Write);
            sem_wait(Write);
            fprintf(SharedMem->print, "%d: IMM %d: leaves: %d: %d: %d\n",SharedMem->A,i,SharedMem->NE,SharedMem->NC,SharedMem->NB);
            sem_post(Write);

            exit(0);
        }
        
        mysleep(IG);
    }

return 0;
}


int Judge_gen(int PI, int JG, int JT, SharedMemory* SharedMem)
{

do
{
    mysleep(JG);

    //JUDGE ENTERS
    sem_wait(Write);
    SharedMem->A++;
    sem_post(Write);
    sem_wait(Write);
    fprintf(SharedMem->print, "%d: JUDGE: wants to enter\n",SharedMem->A);
    sem_post(Write);
    sem_wait(Judge_out);
    sem_wait(Write);
    SharedMem->A++;
    sem_post(Write);
    sem_wait(Write);
    fprintf(SharedMem->print, "%d: JUDGE: enters: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    sem_post(Write);


    //JUDGE WAITS
    if(SharedMem->NE != SharedMem->NC)
    {
        sem_wait(Write);
        SharedMem->A++;
        sem_post(Write);
        sem_wait(Write);
        fprintf(SharedMem->print, "%d: JUDGE: waits for IMM: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
        sem_post(Write);
        while(SharedMem->NE != SharedMem->NC)
        {mysleep(2);}
    }

    //HOLDS MEETING
    sem_wait(Write);
    SharedMem->A++;
    sem_post(Write);
    sem_wait(Write);
    fprintf(SharedMem->print, "%d: JUDGE: starts confirmation: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    sem_post(Write);

    mysleep(JT);

    sem_wait(Write);
    SharedMem->A++;
    SharedMem->IMM_certified += SharedMem->NC;
    SharedMem->NE = 0;
    SharedMem->NC = 0;
    sem_post(Write);
    sem_wait(Write);
    fprintf(SharedMem->print, "%d: JUDGE: ends confirmation: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    sem_post(Write);
    sem_post(Certificate);

    mysleep(JT);


    sem_wait(Write);
    SharedMem->A++;
    sem_post(Write);
    sem_wait(Write);
    fprintf(SharedMem->print, "%d: JUDGE: leaves: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    sem_post(Write);
    sem_trywait(Certificate);
    sem_post(Judge_out);

}while(SharedMem->IMM_certified < PI);

    sem_wait(Write);
    SharedMem->A++;
    sem_post(Write);
    sem_wait(Write);
    fprintf(SharedMem->print, "%d: JUDGE: finishes\n",SharedMem->A);
    sem_post(Write);

    return 0;
}


/*
int fce(SharedMemory* SharedMem)
{
    while(SharedMem->IMM_certified != SharedMem->PI)
    {

    sleep(2);
    SharedMem->A++;
    printf("%d JUDGE : wants to enter\n",SharedMem->A);
    sem_wait(Judge_out);
    SharedMem->A++;
    printf("%d: JUDGE: enters: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    
    printf("%d: JUDGE: starts confirmation: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    mysleep(SharedMem->JG);
    printf("%d: JUDGE: ends confirmation: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    SharedMem->IMM_certified += SharedMem->NC;
    SharedMem->NC = 0;
    SharedMem->NE = 0;
    sem_post(Certificate);
    mysleep(SharedMem->JT);
    printf("%d: JUDGE: leaves: %d: %d: %d\n",SharedMem->A,SharedMem->NE,SharedMem->NC,SharedMem->NB);
    sem_trywait(Certificate);
    sem_post(Judge_out);
    sem_post(Judge_out);
    sleep(1);
    }
    return 0;
}

*/








int main(int argc, char** argv)
{


///// CHECKING CORRECT NUMBER OF ARGS ///////
  if(argc != 6)
  {
    fprintf(stderr,"Unvalid Input Arguments, use ./proj2 PI IG JG IT JT");
    exit(1);
  }




  //Init Shared Memory
  SharedMemory* SharedMem;
  SharedMem = mmap( NULL , sizeof(SharedMemory) , PROT_READ|PROT_WRITE , MAP_SHARED|MAP_ANONYMOUS, -1 , 0 );
    if(SharedMem == MAP_FAILED)
      {
        fprintf(stderr,"mmap allocate failure");
        return 1;
      }


    //init semaphoressemaphore_imigran
    Imm_queue = sem_open("xkvasn14_semaphore_imigrant_queue",O_CREAT,0666,1);
    Imm_regis = sem_open("xkvasn14_semaphore_imigrant_register",O_CREAT,0666,1);
    Judge_out = sem_open("xkvasn14_semaphore_judge_out",O_CREAT,0666,1);
    Certificate = sem_open("xkvasn14_semaphore_certificate",O_CREAT,0666,0);
    Write = sem_open("xkvasn14_semaphore_write",O_CREAT,0666,1);
    sem_trywait(Certificate);



    SharedMem->PI = atoi(argv[1]);
    SharedMem->IG = atoi(argv[2]);
    SharedMem->JG = atoi(argv[3]);
    SharedMem->IT = atoi(argv[4]);
    SharedMem->JT = atoi(argv[5]);



    SharedMem->print = fopen("proj2.out","w");




////////// CHECKING VALID ARGUMENTS INTERVALS ///////////

    if(SharedMem->PI < 0)
      {
        clean(SharedMem);
        fprintf(stderr,"PI is lower then 0!");
        exit(1);
      }
    if(SharedMem->IG > 2000 || SharedMem->IG < 0)
    {
        clean(SharedMem);
        fprintf(stderr,"IG has to be <0;2000>!");
        exit(1);
    }
    if(SharedMem->JG > 2000 || SharedMem->JG < 0)
    {
        clean(SharedMem);
        fprintf(stderr,"JG has to be <0;2000>!");
        exit(1);
    }
    if(SharedMem->IT > 2000 || SharedMem->IT < 0)
    {
        clean(SharedMem);
        fprintf(stderr,"IT has to be <0;2000>!");
        exit(1);
    }
    if(SharedMem->JT > 2000 || SharedMem->JT < 0)
    {
        clean(SharedMem);
        fprintf(stderr,"JG has to be <0;2000>!");
        exit(1);
    }





/////////////////////////
//Imigrants

    int iid = fork();
    if(iid < 0)
    {
        fprintf(stderr,"Fork error");
        clean(SharedMem);
        return 1;
    }
    else if(iid == 0)
    {
        int result;
        result = Imigrants_gen(SharedMem->PI,SharedMem->IT,SharedMem->IG,SharedMem,iid);
        exit(result);
    }



/*

    int pid = fork();
    if(pid < 0)
    {
        fprintf(stderr,"Fork error");
        clean(SharedMem);
        return 1;
    }
    else if(pid == 0)
    {
        int result;
        result = Judge_gen(SharedMem->PI,SharedMem->JG,SharedMem->JT,SharedMem);
        exit(result);
    }

*/

















    
   
    //////////////////////
    //Judge

        int jid = fork();
        if(jid < 0)
        {
            fprintf(stderr,"Fork error");
            int state;
            wait(&state);
            clean(SharedMem);
            return 1;
        }
        else if(jid == 0)
        {
            int result;
            result = Judge_gen(SharedMem->PI,SharedMem->JG,SharedMem->JT,SharedMem);
            exit(result);
        }
       
        
            //parent
            int state;
            wait(&state);
            wait(&state);
       
   
    


clean(SharedMem);
return 0;
} // MAIN END
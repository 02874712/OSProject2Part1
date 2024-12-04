#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define LOOP 25
#define TIME_MAX_COUNT 5
#define D_MAX_AMOUNT 100
#define S_MAX_AMOUNT 50

void  PoorStudentProcess(int []);
void  DearDadProcess(int []);

sem_t *mutex, *turn;


int  main(int  argc, char *argv[])
{
     int    ShmID;
     int    *ShmPTR;
     pid_t  pid;
     int    status;

     ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          exit(1);
     }
     // printf("Process has received a shared memory of two integers...\n");

     ShmPTR = (int *) shmat(ShmID, NULL, 0);
     if (*ShmPTR == -1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
     }

     ShmPTR[0] = 0;      //   BankAccount

     /* create, initialize semaphore as bankAccess 'Bank Account Access'*/
     if ((mutex = sem_open("bankAccess", O_CREAT, 0644, 1)) == SEM_FAILED) {
          perror("sem_open for bank access failed");
          exit(1);
     }
       /* create, initialize semaphore as cpTurn 'Consumer/Producer Turn' */
     if ((turn = sem_open("cpTurn", O_CREAT, 0644, 1)) == SEM_FAILED) {
          perror("sem_open for turn failed");
          exit(1);
     }

     printf("Orig Bank Account = %d\n", ShmPTR[0]);

     pid = fork();
     if (pid < 0) { 
          printf("*** fork error (server) ***\n");
          exit(1);
     }
     else if (pid == 0) {
          PoorStudentProcess(ShmPTR);
          exit(0);
     }
     else{
          DearDadProcess(ShmPTR);
     }

     wait(&status);
     printf("Process has detected the completion of its child...\n");
     shmdt((void *) ShmPTR);
     printf("Process has detached its shared memory...\n");
     shmctl(ShmID, IPC_RMID, NULL);
     printf("Process has removed its shared memory...\n");
     printf("Process exits...\n");
     exit(0);
}

void  PoorStudentProcess(int  SharedMem[])
{    
  //   Initialize variables for Poor Student Process
  int deposit = 0;
  char student_prompt[] = "Poor Student: ";

  //   Initialize SharedMem variables for account
  int account = 0;
  int turn_val, attempt;

  // Loop Indefinitely   
  while(1){

     //   Randomize time the PS Process sleeps up to TIME_MAX_COUNT
     srand(time(NULL));
     sleep(rand()% (TIME_MAX_COUNT + 1));
     
     printf("Poor Student: Attempting to Check Balance\n");


     while (sem_getvalue(turn, &turn_val) == 0 && turn_val != 1);   // while it is not PS turn keep looping
     // printf("Grabbing lock...\n");
     sem_wait(turn);


     sem_wait(mutex);  // enters critical section
     account = SharedMem[0];     // when it is PS turn retrieve BankAccount contents

     //   gets a random number for the balance amount
     attempt = rand();
     deposit = rand()%(S_MAX_AMOUNT + 1);
     printf("Poor Student needs $%d\n", deposit);


     if(attempt%2==0)   //   attempt random number is even, withdraw!
     {    
          if (deposit <= account){
               account -= deposit;
               printf("%sWithdraws $%d / Balance = $%d\n", student_prompt, deposit, account);
          }
         
     } else if(deposit > account)  //   balance is > account, not enough!
     {
          printf("%sNot Enough Cash ($%d)\n", student_prompt, account );
     }else{
          printf("Poor Student: Last Checking Balance = $%d\n", account);
     }

     //   copy new values back to shared memory
     SharedMem[0] = account;
     // printf("Releasing lock...\n");
     sem_post(mutex);
     sem_post(turn);     

   }

}

void DearDadProcess(int SharedMem[])
{    
  //   Initialize variables for Dear Old Dad Process
  int deposit = 0;
  char dad_prompt[] = "Dear Old Dad: ";

  //   Initialize SharedMem variables into account and turn
  int account = 0;
  int turn_val, attempt;

  //   Loop Indefinitely
  while(1){

    //   Randomize time the DOD Process sleeps up to TIME_MAX_COUNT
    srand(time(NULL));
    sleep(rand()% (TIME_MAX_COUNT + 1));

    printf("Dear Old Dad: Attempting to Check Balance\n");

      while (sem_getvalue(turn, &turn_val) == 0 && turn_val != 1);   // while it is not DOD turn keep looping
     //  printf("Grabbing lock...\n");
      sem_wait(turn);
      //gets a random number for the balance amount
      attempt = rand();
      deposit = rand()%(D_MAX_AMOUNT + 1);

      sem_wait(mutex);    // enters critical section
      account = SharedMem[0];     // when it is DOD turn retrieve BankAccount contents     


      if(attempt%2==0)  // attempt - random number generated is even 
      {

        //deposit money if deposit is < 100 - enough cash if odd
        if(account<100){ 
             
             // deposit money if deposit is even - doesn't have money if odd
             if(deposit%2==0){
               account += deposit;
               printf("%sDeposits $%d / Balance = $%d\n", dad_prompt, deposit, account);
             } else {
               printf("%sDoesn't have any money to give\n", dad_prompt);
             } 
             
        }
        else {
              printf("%sThinks Student has enough Cash ($%d)\n", dad_prompt, account);
        }
        
      } else {  //   deposit is odd
          printf("%sLast Checking Balance = $%d\n", dad_prompt, account);
      }

      //   copy new values back to shared memory
      SharedMem[0] = account;
     //  printf("Releasing lock...\n");
      sem_post(mutex);
      sem_post(turn);        

  }
}

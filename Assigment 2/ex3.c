/*
 * CSE_OS Exercise 3
 * ID1: 313561813
 * ID2: 316147719
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

// defines parmeters:
#define N 5
#define FIN_PROB 0.1
#define MIN_INTER_ARRIVAL_IN_NS 8000000
#define MAX_INTER_ARRIVAL_IN_NS 9000000
#define INTER_MOVES_IN_NS		100000
#define SIM_TIME 2

//================================================================
// Traffic Block struct that will define a block in the TrafficCircle.
typedef struct TrafficBlock{
	char isFull;
	int isNew;
	pthread_mutex_t blockMutex;
}TrafficBlock ;
TrafficBlock * TrafficCircle;
//=================================================================



// declaration on the functions
void InitializeTrafficCircle();
void * generator(void * position);
void PrintTrafficCircle();
void * drive(void * position);
int DestroyTrafficCircle();
void sigTermination(int signal);


pthread_t M_thread;
int * flag;
//--------- create signal hender to finish the program------
void sigTermination(int signal){
	*flag=1;	
	if(pthread_self()!=M_thread){return;}//chack if this is the main thread 
	sleep(1);//the main thread sleep for some time till the other threads finish
	DestroyTrafficCircle(); //call the func which destroy the simulation
}

//-----------MAIN FUNCTION------------------------
int main(){
	int i;
	flag=(int*)malloc(sizeof(int)); 
	*flag=0;
	signal(SIGINT,sigTermination); 
	InitializeTrafficCircle();//initial the traffic circle 
	M_thread=pthread_self();
	for(i=0; i<10; i++){//print 10 snapshot of the traffic circle 
		usleep(SIM_TIME*100000);
		PrintTrafficCircle();
	}	
	kill(getpid(),SIGINT);
	return 0;
}
//----- initial the traffic circle whit 4 generator and fill the struct whit mutex and condition (new)------

void InitializeTrafficCircle(){	
	int i, corners[4],l;
	l = 4*(N-1);
	pthread_t Generators[4];
	TrafficCircle = malloc(sizeof( TrafficBlock)*(l));//initial denamic allocation for the traffic circle
	for(i=0; i<l; i++){
		TrafficCircle[i].isFull= ' ';//initial each cell as empty cell
		TrafficCircle[i].isNew = 0;//ititial the condition about the the car (is new ?)
		pthread_mutex_init(&TrafficCircle[i].blockMutex,NULL);//initial mutex for each cell
	}

	for(i=0; i<4; i++){//for loop that initial the generator in each corners of the circle
		corners[i] = i*(N-1);
		if(pthread_create(&Generators[i],NULL,generator,&corners[i]) != 0) {//chack if we can creat tread for each generator 
			printf("fail to create generator");
			kill(getpid(),SIGINT);
		}
	}
}
void * generator(void * position){	
	int pos = *(int*)position;
	int err1,err2;             //error indicators 
	int l = 4*(N-1);          //length of the traffic circle
	unsigned int timer;       
	pthread_t car;            //car thread


    //------------------- generating car iterations -----------------------//
	while(!(*flag)){  //running until the flag is up
		//random uniform calculation for the sleeping time 
		timer = (MIN_INTER_ARRIVAL_IN_NS + rand() / (RAND_MAX / (MAX_INTER_ARRIVAL_IN_NS - MIN_INTER_ARRIVAL_IN_NS +1) +1))/1000; 
		
		usleep(timer);		//sleep
		if(TrafficCircle[pos].isFull == ' ' && TrafficCircle[(pos+l-1)%l].isFull == ' '){ //check if the currnet and the previous blocks are free
			err1=pthread_mutex_trylock(&TrafficCircle[((pos+l-1) % l)].blockMutex); //try to catch the previous block 
			if(err1 == EBUSY){ continue; }                                        //the block is busy try in the next time
			err2=pthread_mutex_trylock(&TrafficCircle[pos].blockMutex);         //try to catch the current block 
			if(err2 == EBUSY){ continue; }                                       //the block is busy try in the next time
			else if (err1 != 0 || err2 != 0) {
				printf("Unknown Error locking blocks for the generator\n");
				kill(getpid(),SIGINT);                                          //terminate the program
			} 
			else{                                                               //succseed locking the blocks
				TrafficCircle[pos].isFull = '*';                                //mark the block as full
				TrafficCircle[pos].isNew = 1;                                   //the block has a new car that didn't completed a move
				//create a thread for the car in the drive function if there is an error terminate the program
				if(pthread_create(&car,NULL,drive,&pos) != 0) {printf("Unknown Error creating a car thread\n"); kill(getpid(),SIGINT);}
				pthread_mutex_unlock(&TrafficCircle[pos].blockMutex);   //unlock the blocks in the end of the driving
				pthread_mutex_unlock(&TrafficCircle[(pos+l-1) % l].blockMutex);
			}
		}
	}
	return NULL;
}
void * drive(void * position){
	int pos = *(int*)position;
	int l = 4*(N-1);
	int nextPos = (pos + 1)%l;
	int Err,randval;

	if(*flag == 1){ pthread_exit(NULL); }    //check if the flag is up, if so exit the thread
	pthread_mutex_lock(&TrafficCircle[pos].blockMutex); 

    //------------------- cars iterations -----------------------//
	while(!(*flag)){
		//sleep before each iteration
		usleep(INTER_MOVES_IN_NS/1000);
		/*if the car next to a sink point and it didn't just created 
		exit the traffic circle in a random probability*/
		if(pos % (N-1) == 0 && TrafficCircle[pos].isNew == 0){ 
			randval=rand() % 10000;
			if(randval < FIN_PROB*10000){ // We want to exit!
				TrafficCircle[pos].isFull = ' ';
				pthread_mutex_unlock(&TrafficCircle[pos].blockMutex);
				pthread_exit(NULL);
			}
		}
		//if the car didn't left the traffic circle check if the next block is open and try to move forward
		Err=pthread_mutex_trylock(&TrafficCircle[(pos+1) % l].blockMutex);
		if(Err == EBUSY){ continue; }  //the block is busy try in the next time
		else if (Err != 0) {
			printf("Unknown Error locking the next block\n");
			kill(getpid(),SIGINT);                             //terminate the program
		}
		//
		nextPos = (pos + 1)%l;
		TrafficCircle[pos].isFull = ' ';    //mark the currnet block to be empty
		TrafficCircle[pos].isNew = 0;       //mark the car as not new because it just moved
		TrafficCircle[nextPos].isFull = '*';  //mark the next block to be full
		TrafficCircle[nextPos].isNew = 0;      //mark the car as not new because it just moved
		pos = nextPos;

		pthread_mutex_unlock(&TrafficCircle[(pos+l-1) % l].blockMutex);  //unlock the old block
	}
	return NULL;

}

void PrintTrafficCircle(){// the function print the traffic circle 
	int l;
	l = 4*N-4;
	for(int i=0;i!=3*N-4;i=(i+(l)-1)%(l)){
		printf("%c",TrafficCircle[i].isFull);
	} printf("\n");
	
	for(int j=1;j<N-1;j++){
		printf("%c",TrafficCircle[j].isFull);
		for(int k=1;k<N-1;k++) printf("@");
		printf("%c",TrafficCircle[3*N-j].isFull);
		printf("\n");
	}
	
	for(int i=N-1;i!=2*N-1;i++){
		printf("%c",TrafficCircle[i].isFull);
		} printf("\n");
	printf("\n\n");
}

int DestroyTrafficCircle(){
	for(int i=0; i<4*N-4; i++){		
		pthread_mutex_destroy(&TrafficCircle[i].blockMutex);//destroy each mutex for each cell in the circle 
	}
	//free all the momory allocation which we use in the code
	free(flag);
	free(TrafficCircle);	
	exit(0);
}






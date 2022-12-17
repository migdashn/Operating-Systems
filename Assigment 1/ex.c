

/*cse-os exresice 1 
ID1: 316147719
ID2: 313561813*/



#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <fcntl.h>

#include <wait.h>

#include <pthread.h>


///decleration of the functions

pid_t my_fork();

void print_pids(int fd, short unsigned int N, short unsigned int G);

void count_lines(short unsigned int G);

void *printme(void* id);

void print_threads(short unsigned int N);





int main (int argc ,char* argv[]){
	//main function saving all the argument and calling the funcions

    int N = atoi(argv[1]), G = atoi(argv[2]);                //input = argv[1] , argv[2]

    int fd;


    if ((fd = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {      //open the out.txt file

        perror("Error in open file\n");

        exit(EXIT_FAILURE);

    }


    print_pids(fd,N,G);

    close(fd);

    count_lines(G);

    print_threads(N);
}

    /////////////      ex1      /////////////////

    pid_t my_fork(void){
		//call a fork and return a non-negative value if succeeds.
		//else, print an error message.

        pid_t f = fork();

        if(f<0) {

            perror("Fork error");

            exit(0);

        }

        return f;

    }




    ///////////      ex2      /////////////////



void print_pids(int fd, short unsigned int N, short unsigned int G) {
	//print lines by the following rules:
	//for each generation g print  N^g lines where the first process g=0 and the last g=G
	//lines will be printed the file out.txt
	int g = 0, n = 0, status, buff_size ;
	char buff[100];
        pid_t pid;

        while(n<N && g<=G) {
		pid = my_fork();
		if(!pid){      //check if it is the child 
			n = 0;
			g++;
		}else{
			waitpid(pid,&status,0);   //wait for all the children to finish
			if(n==N-1 || n == N){
				buff_size = sprintf(buff,"My pid id %d. My generation is %d.\n", getpid(), g);
				write(fd,buff,buff_size);    //write to the file
			}
			n++;    //it is the father 
		}
	}
	if(g) exit(0);
}



////////////////////      ex3      ////////////////////////



void count_lines(short unsigned int G) {
	/*count the number of lines for each
	generation in the file out.txt calling grep
	and print the number of lines in the terminal*/
    int i, status;
	char buff[100];
	pid_t pid;	
	for(i=0;i<G;i++){
		pid = my_fork();
		if(pid > 0){             
			waitpid(pid,&status,0);        //wait for all the children to finish
		}else{
			/*create a command with system() function to count all the lines in out.txt with wc*/
			sprintf(buff, "My generation is %d \\.", G - i); //the text we search
			printf("Number of lines by proccesses of generation %d is ",G - i);
			fflush(stdout);
			sprintf(buff,"cat out.txt | grep ' %d.$' | wc -l\n",G - i);
			system(buff);
			exit(0);

		}
	} 
}

/////////////////// ex4       /////////////////////////

void *printme(void* id){
	//printing function
	int *i;
	i = (int*)id;
	printf("Hi, I'm thread number %d \n",*i);
	return NULL;
}

void print_threads(short unsigned int N){
	//create N threads and print them in ascending order
	int i, vals[N];
	pthread_t tids[N];                                        //array with all the threads id 
	void* retval;
	for(i = 0; i<N;i++){
		vals[i] = i;
		pthread_create(&(tids[i]),NULL,printme,vals+i);    //creating threads and the father waits until the children will finish
		pthread_join(tids[i],&retval);
	}
	pthread_exit(NULL);
}


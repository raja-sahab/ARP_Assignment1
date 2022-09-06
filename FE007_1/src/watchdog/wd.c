/* Whatchdog: it checks the other processes, and sends a reset command in case all processes 
do nothing (no computation, no motion, no input/output) for 60 seconds. */

/* LIBRARIES */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
/* Defining CHECK() tool. By using this methid the code results ligher and cleaner */
#define CHECK(X) ({int __val = (X); (__val == -1 ? ({fprintf(stderr,"ERROR (" __FILE__ ":%d) -- %s\n",__LINE__,strerror(errno)); exit(-1);-1;}) : __val); })

/* CONSTANTS */
#define PERIOD 60     // After this period, it launchs the reset signal.

/* GLOBAL VARIABLES */
int timer = PERIOD;   // Timer.
FILE * log_file;      // Log file.

/* FUNCTION HEADERS */
void signal_handler( int sig );
void logPrint ( char * string );

/* FUNCTIONS */
void signal_handler( int sig ) {
    /* Function to handle the SIGTSTP signal. */

    if(sig==SIGTSTP){
        timer=PERIOD; // Restart the timer variable.
    }   
}

void logPrint ( char * string ) {
    /* Function to print on log file adding time stamps. */

    time_t ltime = time(NULL);
    fprintf( log_file, "%.19s: %s", ctime( &ltime ), string );
    fflush(log_file);
}

/* MAIN */
int main(int argc, char * argv[]){

    /* Acquires the motors' PIDs. */
    int pid_motor_h=atoi(argv[1]);
    int pid_motor_v=atoi(argv[2]);

    log_file = fopen("../log_file/Log.txt","a"); // Open the log file

    logPrint("wd        : Watchdog started.\n");

    /* Signals that the process can receive. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler =&signal_handler;
    sa.sa_flags=SA_RESTART;

    /* sigaction for SIGTSTP */
    CHECK(sigaction(SIGTSTP,&sa,NULL));

    while(1){

        while (timer >= 0){     //waiting...
            sleep(1);           // Sleep for one second.
            timer--;            // Decrease timer variable.
        }

        //if 60 seconds are ellapsed, then send the reset signal to the motors.
        CHECK(kill(pid_motor_h, SIGUSR2));
        CHECK(kill(pid_motor_v, SIGUSR2));

        logPrint("wd        : Reset for time ellapsing.\n");

        timer = PERIOD; //update timer variable
    }

    logPrint("wd        : Watchdog ended.\n");

    fclose(log_file); // Close log file.

    return 0;
}

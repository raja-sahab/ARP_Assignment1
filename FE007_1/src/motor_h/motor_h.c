/* LIBRARIES */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
/* Defining CHECK() tool. By using this methid the code results ligher and cleaner */
#define CHECK(X) ({int __val = (X); (__val == -1 ? ({fprintf(stderr,"ERROR (" __FILE__ ":%d) -- %s\n",__LINE__,strerror(errno)); exit(-1);-1;}) : __val); })

/* CONSTANTS */
#define X_UB 9.9   // Upper bound of x axes.
#define X_LB 0     // Lower bound of x axes.
#define STEP 0.01  // Velocity of the motor.

/* GLOBAL VARIABLES */
float h_position = X_LB;                        // Real position of the h motor.
float est_pos_h = X_LB;                         // Estimated position of the h motor.
int command = 0;                                // Command received.
bool resetting = false;                         // Boolean variable for reset the motor.
bool stop_pressed = false;                      // Boolean variable for stop the motor.
FILE *log_file;                                 // Log file.
char * fifo_h = "/tmp/fifo_command_to_mot_h";   //File path
char * fifo_est_pos_h = "/tmp/fifo_est_pos_h";  //File path

/* FUNCTIONS HEADERS */
void signal_handler( int sig );
float float_rand( float min, float max );
void logPrint ( char * string );

/* FUNCTIONS */
void signal_handler( int sig ) {
    /* Function to handle stop and reset signals. */

    if (sig == SIGUSR1) { // SIGUSR1 is the signal to stop the motor.
        command = 6; //stop command
        stop_pressed = true;
    }

    if (sig == SIGUSR2){ // SIGUSR2 is the signal to reset the motor.
        stop_pressed = false;
        resetting = true;
    }
}

float float_rand( float min, float max ) {
    /* Function to generate a randomic error. */

    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min); // [min, max]
}

void logPrint ( char * string ) {
    /* Function to print on log file adding time stamps. */

    time_t ltime = time(NULL);
    fprintf( log_file, "%.19s: %s", ctime( &ltime ), string );
    fflush(log_file);
}

/* MAIN */

int main() {

    int fd_h, fd_inspection_h; //file descriptors
    int ret;                   //select() system call return value
    char str[80];              // String to print on log file.

    /* Signals that the process can receive. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;

    /* sigaction for SIGUSR1 & SIGUSR2 */
    CHECK(sigaction(SIGUSR1,&sa,NULL));
    CHECK(sigaction(SIGUSR2,&sa,NULL));  

    log_file = fopen("../log_file/Log.txt", "a"); // Open the log file.

    logPrint("motor_h   : Motor h started.\n");

    fd_set rset; //ready set of file descriptors

    /* The select() system call does not wait for file descriptors to be ready. */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    /* Open pipes. */
    fd_h = CHECK(open(fifo_h, O_RDONLY));
    fd_inspection_h=CHECK(open(fifo_est_pos_h, O_WRONLY));

    while (1) {

        /* Initialize the set of file descriptors for the select system call. */
        FD_ZERO(&rset);
        FD_SET(fd_h, &rset);

        ret = CHECK(select(FD_SETSIZE, &rset, NULL, NULL, &tv));

        if (FD_ISSET(fd_h, &rset) != 0) { // There is something to read!
            CHECK(read(fd_h, &command, sizeof(int))); // Update the command.

            sprintf(str, "motor_h   : command received = %d.\n", command);
            logPrint(str);
        }

        if (!resetting) { // The system is not resetting.
            
            if (command == 3) { // Increase h position.

                if (h_position > X_UB) { // Upper X limit of the work envelope reached.
                    command = 6; //stop command
                }
                else {
                    h_position += STEP; //go right
                }
            }
            
            if (command == 4) { // Decrease h position.

                if (h_position < X_LB) { //Lower X limit of the work envelope reached
                    command = 6; //stop command

                } else {
                    h_position -= STEP; //go left
                }
            }

            if (command == 6) { //stop command
                //h_position must not change
            }

        } else { // The motor is resetting. The only usable command during reset is STOP command.

            if ( (!stop_pressed) && (h_position > X_LB) ) {
                h_position -= STEP; // Returns to zero position.
            
            } else { // The stop command occours or the reset ended.
                resetting = false;
                command = 0;
            }

            stop_pressed = false;
        }

    /*  If the system is resetitng we continue sending (to the inspection konsole)
        the hoist position with an error because the measurement uncertainty continues 
        to exists even if the system is resetting!  */
    /*  Send the estimate position to the inspection console. */
        est_pos_h = h_position + float_rand(-0.005, 0.005); //compute the estimated position
        CHECK(write(fd_inspection_h, &est_pos_h, sizeof(float)));  //send to inspection konsole

        sprintf(str, "motor_h   : h_position = %f\n", h_position);
        logPrint(str);

        /* Sleeps. If the command does not change, repeats again the same command. */
        usleep(20000); //sleep for 0,2 second

    } // End of the while cycle.

    /* Close pipes. */
    CHECK(close(fd_h));
    CHECK(close(fd_inspection_h));

    logPrint("motor_h   : Motor h ended.\n");

    fclose(log_file); // Close log file.

    return 0;
}

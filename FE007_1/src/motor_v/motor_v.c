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
#include <stdbool.h>
/* Defining CHECK() tool. By using this methid the code results ligher and cleaner */
#define CHECK(X) ({int __val = (X); (__val == -1 ? ({fprintf(stderr,"ERROR (" __FILE__ ":%d) -- %s\n",__LINE__,strerror(errno)); exit(-1);-1;}) : __val); })

/* CONSTANTS */
#define Z_UB 9.9   // Upper bound of z axes.
#define Z_LB 0     // Lower bound of z axes.
#define STEP 0.01  // Velocity of the motor.

/* GLOBAL VARIABLES */
float v_position = Z_LB;                        // Real position of the v motor.
float est_pos_v = Z_LB;                         // Estimated position of the v motor.
int command = 0;                                // Command received.
bool resetting = false;                         // Boolean variable for reset the motor.
bool stop_pressed = false;                      // Boolean variable for stop the motor.
FILE *log_file;                                 // Log file.
char * fifo_v = "/tmp/fifo_command_to_mot_v";   // File path
char * fifo_est_pos_v = "/tmp/fifo_est_pos_v";  // File path

/* FUNCTIONS HEADERS */
void signal_handler( int sig );
float float_rand( float min, float max );
void logPrint ( char * string );

/* FUNCTIONS */
void signal_handler( int sig ) {
    /* Function to handle stop and reset signals. */

    if (sig == SIGUSR1) { // SIGUSR1 is the signal to stop the motor.
        command = 5; //stop command
        stop_pressed = true;
    }

    if (sig == SIGUSR2) { // SIGUSR2 is the signal to reset the motor.
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

    int fd_v, fd_inspection_v; //file descriptors
    int ret;                   //select() system call return value
    char str[80];              // String to print on log file.
    
    /* Signals that the process can receive. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;

    /* sigaction for SIGUSR1 & SIGUSR2 */
    CHECK(sigaction(SIGUSR1, &sa, NULL));
    CHECK(sigaction(SIGUSR2, &sa, NULL));

    log_file = fopen("../log_file/Log.txt", "a"); // Open the log file

    logPrint("motor_v   : Motor v started.\n");

    fd_set rset; //ready set of file descriptors

    /* The select() system call does not wait for file descriptors to be ready. */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    /* Open pipes. */
    fd_v = CHECK(open(fifo_v, O_RDONLY));
    fd_inspection_v = CHECK(open(fifo_est_pos_v, O_WRONLY));

    while (1) {

        /* Initialize the set of file descriptors for the select system call. */
        FD_ZERO(&rset);
        FD_SET(fd_v, &rset);
        ret = CHECK(select(FD_SETSIZE, &rset, NULL, NULL, &tv));

        if (FD_ISSET(fd_v, &rset) != 0) { // There is something to read!
            CHECK(read(fd_v, &command, sizeof(int))); // Update the command.

            sprintf(str, "motor_v   : command received = %d.\n", command);
            logPrint(str);
        }

        if (!resetting) { // The system is not resetting.
            
            if (command == 1) { // Increase v position.

                if (v_position > Z_UB) { // Upper Z limit of the work envelope reached
                    command = 5; //stop command
                }
                else {
                    v_position += STEP; //go upwards
                }
            }

            if (command == 2) { // Decrease z position.

                if (v_position < Z_LB) { //Lower Z limit of the work envelope reached
                    command = 5; //stop command

                } else {
                    v_position -= STEP; //go downwards
                }
            }

            if (command == 5) { //stop command
                // v_position must not change
            }

        } else { // The motor is resetting. // The motor is resetting. The only usable command during reset is STOP command.

            if (!stop_pressed && v_position > Z_LB) {
                v_position -= STEP; // Returns to zero position.
            
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
        est_pos_v = v_position + float_rand(-0.005, 0.005); //compute the estimated position
        CHECK(write(fd_inspection_v, &est_pos_v, sizeof(float)));  //send to inspection konsole

        sprintf(str, "motor_v   : v_position = %f\n", v_position);
        logPrint(str);

        /* Sleeps. If the command does not change, repeats again the same command. */
        usleep(20000);

    } // End of the while cycle.

    /* Close pipes. */
    CHECK(close(fd_v));
    CHECK(close(fd_inspection_v));

    logPrint("motor_x   : Motor x ended.\n");

    fclose(log_file); // Close log file.

    return 0;
}

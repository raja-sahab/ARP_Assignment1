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
#include <termios.h>
#include <time.h>
#include <stdbool.h>

/* Defining CHECK() tool. By using this method the code results lighter and cleaner. */
#define CHECK(X) ({int __val = (X); (__val == -1 ? ({fprintf(log_file,"ERROR (" __FILE__ ":%d) -- %s\n",__LINE__,strerror(errno)); exit(-1);-1;}) : __val); })

/* COLORS */
#define RESET "\033[0m"
#define BHRED "\e[1;91m"
#define BHGRN "\e[1;92m"
#define BHYEL "\e[1;93m"
#define BHMAG "\e[1;95m"
#define BHCYN "\e[1;96m"

/* GLOBAL VARIABLES */
const int cmd_increase_v = 1;                       // This represents the "increase V" command.
const int cmd_decrease_v = 2;                       // This represents the "decrease V" command.
const int cmd_increase_h = 3;                       // This represents the "increase H" command.
const int cmd_decrease_h = 4;                       // This represents the "decrease H" command.
const int cmd_stop_v = 5;                           // This represents the "STOP V" command.
const int cmd_stop_h = 6;                           // This represents the "STOP H" command.
int fd_h, fd_v, fd_pid;                             // File descriptors.
pid_t command_pid, pid_wd;                          // Process IDs.
bool resetting = false;                             // Boolean variable for reset the motors.
FILE *log_file;                                     // Log file.
char * fifo_v = "/tmp/fifo_command_to_mot_v";       // File path.
char * fifo_h = "/tmp/fifo_command_to_mot_h";       // File path.
char * fifo_inspection = "/tmp/command_to_in_pid";  // File path.

/* FUNCTIONS HEADERS */
void signal_handler( int sig );
int interpreter();
void setup_terminal ();
void logPrint ( char * string );
void helpPrint ();

/* FUNCTIONS */
void signal_handler( int sig ) {
    /* Function to handle stop and reset signals. */

    if (sig == SIGUSR1) { // SIGUSR1 is the signal to stop the motors.
        resetting = false;
        printf(BHRED "... MOTORS STOPPED ..." RESET "\n");
        fflush(stdout);
    }

    if (sig == SIGUSR2) { // SIGUSR2 is the signal to reset motors.
        resetting = true;
        printf(BHRED "...   RESETTING    ..." RESET "\n");
        fflush(stdout);
    }
}

int interpreter(){
    /* Function to receive input from the keyboard and convert it into commands.
    It also writes on the correct pipe to control motors.*/

    int c, c1, c2;   // Input characters are treated as integers, with ASCII conversion.
    c = getchar();   // Wait for the keyboard input.

    if (c != 104 && c != 118 && c != 27 && c != 97 && c!= 101) { // The input is not a command.
        printf(BHMAG " --> Invalid command, press 'h' for help." RESET "\n");

    } else { // The input is valid.
        
        kill(pid_wd, SIGTSTP); // Send a signal to let the watchdog know that an input occurred.

        time_t ltime = time(NULL);
        fprintf(log_file, "%.19s: command   : Input received.\n", ctime(&ltime));
        fflush(log_file);

        if ( c == 101) { // ASCII number for 'e' keyboard key.
            return 1; //no errors occurred. User just quitted the aplication. 
        }

        if ( c == 97) { // ASCII number for 'a' keyboard key.
            helpPrint();
        }

        if (!resetting) { // When the motors are resetting, this part is not executed.

            /* Arrow command inputs. Arrows are a combination of three different characters */
            if (c == 27) { // The first ASCII numbers for arrows is 27.
                c1 = getchar(); // If c is 27, it reads another character.

                if (c1 == 91) { // The second ASCII numbers for arrows is 27.
                    c2 = getchar(); // Reads another character.

                    if (c2 == 65) { //third ASCII nummber for upwards arrow.
                        printf("\n" BHYEL "Decrease V" RESET "\n");
                        CHECK(write(fd_v, &cmd_decrease_v, sizeof(int)));
                    }
                    if (c2 == 66) { //third ASCII nummber for downwards arrow.
                        printf("\n" BHYEL "Increase V" RESET "\n");
                        CHECK(write(fd_v, &cmd_increase_v, sizeof(int)));
                    }
                    if (c2 == 67) { //third ASCII nummber for right arrow.
                        printf("\n" BHMAG "Increase H" RESET "\n");
                        CHECK(write(fd_h, &cmd_increase_h, sizeof(int)));
                    }
                    if (c2 == 68) { //third ASCII nummber for left arrow.
                        printf("\n" BHMAG "Decrease H" RESET "\n");
                        CHECK(write(fd_h, &cmd_decrease_h, sizeof(int)));
                    }
                }
            }
            else
            {
                if (c == 104) { // ASCII number for 'h' keyboard key.
                    printf("\n" BHRED "horizontally stopped" RESET "\n");
                    CHECK(write(fd_h, &cmd_stop_h, sizeof(int)));
                }
                if (c == 118) { // ASCII number for 'v' keyboard key.
                    printf("\n" BHRED "z stop" RESET "\n");
                    CHECK(write(fd_v, &cmd_stop_v, sizeof(int)));
                }
            }

        } else { // The motors are resetting!

            if ( c == 27 ) {
                getchar();
                getchar();
            }

            printf(BHRED "  Please, wait the end of the resetting." RESET "\n");
            fflush(stdout);
        }
    }
    return 0;
}

void setup_terminal (){
    /* Function to setup the terminal in order to receive immediately input key commands,
    without waiting the ESC key. */

    static struct termios newt;
    /* tcgetattr gets the parameters of the current terminal
    STDIN_FILENO will tell tcgetattr that it should write the settings of stdin to newt. */
    tcgetattr(STDIN_FILENO, &newt);

    /* ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL. */
    newt.c_lflag &= ~(ICANON);

    /* Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf(" ### COMMAND CONSOLE ### \n \n");
    fflush(stdout);
    helpPrint();
}

void logPrint ( char * string ) {
    /* Function to print on log file adding time stamps. */

    time_t ltime = time(NULL);
    fprintf( log_file, "%.19s: %s", ctime( &ltime ), string );
    fflush(log_file);
}

void helpPrint () {
    /* Function to print the help command message. */

    printf(BHGRN "Please, use the following commands: " RESET "\n");
    printf(BHMAG "Press the right or left arrows to move the hoist horizontally." RESET "\n");
    printf(BHMAG "Press the up or down arrows to move the hoist vertically." RESET "\n");
    printf(BHYEL "Press 'h' to cease the horizontal movement." RESET "\n");
    printf(BHYEL "Press 'v' to cease the vertical movement." RESET "\n");
    printf(BHCYN "Press 'a' to again display this message." RESET "\n");
    printf(BHCYN "Press 'e' to exit." RESET "\n");
    fflush(stdout);
}

/*MAIN()*/
int main(int argc, char *argv[]) {

    /* Collects PIDs. */
    pid_wd = atoi(argv[1]);
    command_pid = getpid();

    /* Signals that the process can receive. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;

    /* sigaction for SIGUSR1 & SIGUSR2 */
    CHECK(sigaction(SIGUSR1, &sa, NULL));
    CHECK(sigaction(SIGUSR2, &sa, NULL));

    log_file = fopen("../log_file/Log.txt", "a"); // Opens the log file.

    logPrint("command   : Command console started\n");

    /* Opening pipes */
    fd_v = CHECK(open(fifo_v, O_WRONLY));
    fd_h = CHECK(open(fifo_h, O_WRONLY));
    fd_pid = CHECK(open(fifo_inspection, O_WRONLY));

    /* Send the Command PID to the Inspection*/
    CHECK(write(fd_pid, &command_pid, sizeof(int)));

    setup_terminal();

    while (1)
    {
        //if the interpreter function returns 1, then the quit button has been pressed and the process can termitates.
        if (interpreter()) break; 
    }

    /* Close pipes */
    CHECK(close(fd_h));
    CHECK(close(fd_v));
    CHECK(close(fd_pid));

    logPrint("command   : Command console ended\n");

    fclose(log_file); // Close log file.
    return 0;
}

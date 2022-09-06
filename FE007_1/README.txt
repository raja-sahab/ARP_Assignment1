First Assignment of ARP class
---------------------------------------------------------------------

This project aims to reproduce a realistic behavior of a simple hoist, characterized by two degrees of freedom: vertical and horizontal.
We designed the system to have a working area of 10x10 meters, and it can reach both the limits of the work envelope in 23 seconds. Therefore, both motors provide the hoist with a linear velocity of 0.43 meters per second on each ax.
The simulator is composed of six concurrent processes:

> master
> command console
> motor x
> motor z
> inspection console
> watchdog

All data are collected in a log file in the 'log_file' folder during each execution.
We implemented a simple graphic interface to visualize the hoist movements during the execution.


#### INSTALLATION ####

You can install the program with the 'install.sh' bash script. For doing this, insert on the terminal:

$ ./install.sh <pathname>

Where <pathname> is the destination directory where all files will be copied. If it doesn't exist, the script will create it after asking the user. Then, the installation procedure will start, and the needed ncurses library will be installed if there's not yet.
Furthermore, it will compile all programs, placing the executables into the "executables" directory.


#### RUN ####

You can run the program with:

$ ./run.sh


#### HELP ####

If you need more information about each process of the hoist simulator, you can ask for help with:

$ ./help.sh
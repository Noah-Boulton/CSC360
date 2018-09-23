CSC360 Assignment 1 - Noah Boulton V00803598

Note:
	No fork bomb occured in the final testing of this program, however it is recommended to run 
	ulimit -u 30 before execution to be safe.

Compilation:
	To compile the program, use make command.

Running:
	Run the program with ./PMan

Supported Commands:
	bg: If you type bg followed by a program in the directory and a list of arguments, PMan will run the program in the background.
		Usage: 	bg inf hello 100 will run the program inf passing hello and 100 as arguments.
			bg ./inf hello 100 will run the program inf in the current directory, passing in the arguments as above.
			bg /path/to/inf hello 100 will run the program found in the specified directory, passing in the arguments as above.
		Errors: If the fork fails or execvp fails to run the program PMan notifies the user.
	bglist: PMan will list all programs currently running in the background.
		Usage: bglist 
		       123: inf
		       456: foo
         	       Total background jobs: 2
		Errors: If the pid is not found then PMan notifies the user.
	bgkill: If you type bgkill followed by a pid, PMan will kill the process with the specified pid.
		Usage: 	bgkill 1234 will kill the process with pid 1234.
		Errors: If the pid is not found then PMan notifies the user.
	bgstop: If you type bgstop followed by a pid, PMan will stop the process with the specified pid.
		Usage: 	bgstop 1234 will stop the process with pid 1234.
		Errors: If the pid is not found then PMan notifies the user.
	bgstart: If you type bgstart followed by a pid, PMan will start the process with the specified pid.
		Usage: 	bgstart 1234 will start the process with pid 1234 in the background.
		Errors: If the pid is not found then PMan notifies the user.
	pstat: If you type pstat followed by a pid, PMan will list the filename, state, utime, stime, sesident set size, 
	       number of voluntary context switches, and number of involuntary context switches.
		Usage: 	pstat 1234
		       	comm:	inf
		       	state:	sleeping
		       	utime:	40
		       	stime:	40
		       	rss:	127
		       	voluntary_ctxt_switches:	3
		       	nonvoluntary_ctxt_switches: 	3
		Errors: If the pid is not found then PMan notifies the user.
	

CSC360 Assignment 2 - Noah Boulton V00803598

Compilation:
        To compile the program, use make command.

Running:
        Run the program with ./ACS <intput text file>

Input File Format:
	1. The first line specifies the number of customer threads.
	2. The first character specifies the unique id of the customer.
	3. A colon follows the customer id.
	4. Following the colon is an integer cooresponding to the customer arrival time measured in 10ths of a second.
	5. A comma follows the arrival time.
	6. Following the comma is an integer cooresponding to the customer service time also measured in 10ths of a second.
	7. A newline ends the line
	
	This example specifies 7 customers:
	7
     	1:2,60
     	2:4,70
     	3:5,50
     	4:7,30
     	5:7,40
     	6:8,50
     	7:10,30

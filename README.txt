README FILE

Name: UniverseWanderer

Work: 
Make socket programming project. 
Code 5 source files to accomplish server/client communication.


File: 
client.c 	Send command and numbers to request computation
aws.c 		Receive data from client and calculate distributedly in 3 servers
serverA.c 	A server to help AWS to do the computation
serverB.c 	B server to help AWS to do the computation
serverC.c 	C server to help AWS to do the computation


Run Notice:
Please ensure the nums.csv file is located in the same 
folder with executable files.


Messages exchanged:
In my project, I assigned a CODE for every function and 
sent the code as the first together with all other data.

From Client to AWS 
packet contains the function code at first and a sequence 
of integers data cast into an int array. The return packet
only have one integer which is the result of reduction.

From AWS to all Servers
The data has been seperated into three arrays of same amount 
of numbers. Every packets contains the function code as the 
first data and all other data assigned to it. The return 
packet only have one integer which is the result of reduction.


Reused Code: 
I learnt a lot from the Beej's Guide and also tailored some of
the examples to my own project. The code I referenced from Beej
is denoted in my source code files.


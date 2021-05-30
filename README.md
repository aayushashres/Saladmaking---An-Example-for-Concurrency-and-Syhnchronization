# Saladmaking---An-Example-for-Concurrency-and-Syhnchronization
Use of semaphores and shared memory to implement concurrency and synchronization as multiple processes attempt to make "salads". For class on Operating Systems. 

The program creates applications chef and saladmaker that run concurrently to create salads.
The program uses shared memory in order to share information, such as weight of vegetables and number of salads made.
The user can specify the number of salads they want to generate, and also how long a "break" they want to give to the chef and saladmakers.
The user is able to get a log of the activities through created log files. 

(A more detailed description of the assignment can be found on the document for projet 3)

The program runs on macOS.

For safety, make clean before making the executables to ensure that previous log files are not there.

Run make to make the executables.

After the executables are made, open 4 separate terminals.
On the first terminal, we can invoke the chef program as 
./chef -n 20 -s 1000
The number that follows -n is the number of salads we want made. The number that follows -s is the amount of time we want the chef to take a break.

Similarly, on the rest of the 3 terminals, we can envoke saladmakers programs as:
./saladmaker1 -m 1000 -s 5439488
./saladmaker2 -m 1000 -s 5439488
./saladmaker3 -m 1000 -s 5439488
The number that follows -m is the amount of time we want the saladmakers to spend chopping vegetables. The number that follows -s is the shared memory id which will be displayed on terminal of chef program. 

The program releases any shared memory used when it exits.

Note: the helper code deleteshm can be invoked as ./deleteshm -s <sharedmemoryid>

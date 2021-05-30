all: chef saladmaker1 saladmaker2 saladmaker3 deleteshm


structures.o: structures.cpp
	g++ -c structures.cpp 

chef: chef.o 
	g++ chef.o -o chef

chef.o: chef.cpp structures.h
	g++ -c chef.cpp

saladmaker1: saladmaker.o 
	g++ saladmaker.o -o saladmaker1

saladmaker2: saladmaker.o 
	g++ saladmaker.o -o saladmaker2

saladmaker3: saladmaker.o 
	g++ saladmaker.o -o saladmaker3

saladmaker.o: saladmaker.cpp structures.h
	g++ -c saladmaker.cpp 

deleteshm: deleteshm.o
	g++ deleteshm.o -o deleteshm

deleteshm.o: deleteshm.cpp
	g++ -c deleteshm.cpp

clean:
	rm *.o chef saladmaker1 saladmaker2 saladmaker3 deleteshm myfifo saladmaker1_log saladmaker2_log saladmaker3_log parallel_proc

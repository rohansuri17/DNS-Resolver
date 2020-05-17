multi-lookup.c source file for resolver and requester
multi-lookup.h header file
names1..5.txt domain names 
performance.py performance file for DNS lookup
multi-lookup.h header file for resolver and requester
Makefile how to compile and link

Run:
gcc multi-lookup.c -o main -lpthread
./main names1.txt...results.txt

default:
	gcc -g -std=gnu99 -l c -o project -Wall -Wextra -Wno-unused-parameter -pthread cmpsc473mr.c cmpsc473mr.h
clean:
	rm *.o output.txt


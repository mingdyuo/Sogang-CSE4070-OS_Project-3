/* additional.c
	
   when argc is 1, execute fibonacci calculating
   when argc is 4, get max of four numbers 		*/
   

#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>

int
main (int argc, char *argv[]) 
{
	switch(argc){
		case 2:
		{
			int result = fibonacci(atoi(argv[1]));
			printf("%d\n", result);
			return EXIT_SUCCESS;
			break;
		}
		case 5:
			max_of_four_int(
					atoi(argv[1]), atoi(argv[2]),
					atoi(argv[3]), atoi(argv[4]));
			return EXIT_SUCCESS;
			break;
		default:
			break;
	}

    return EXIT_FAILURE;
}


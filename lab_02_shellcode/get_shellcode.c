#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	char l = 1;
	unsigned char buf;
	int fd;

	// Open program from CLI arguments
	fd = open(argv[1], 0, S_IRUSR);

	while(read(fd, &buf, 1))
	{
		// Ignore null bytes
		if (buf == 0 && l == 1) {
			printf(" \n");
			l = 0;
		}

		// Print formatted shellcode
		else if (buf) {
			printf("\\x%02x", buf);
			l = 1;
		}
	}

	// close file handler
	close(fd);
}

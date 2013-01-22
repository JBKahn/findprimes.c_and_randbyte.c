#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

extern int generatenum();
extern int putrandbyte(int fd);

int main(int argc, char **argv)
{
	int d;
	for(d=1;d<argc; d++) {
		int fd;
		if ((fd = open(argv[d], O_RDONLY)) < 0) {
			perror(argv[d]);
			return(1);
		}
		// Sends the file descriptor of the open file to putrandbyte.
     	putrandbyte(fd);
	}
	if (argc == 1) {
		// Sends the file descriptor 0, stdin to putrandbyte.
		putrandbyte(0);
	}
	return(0);
}

// generatenum generates a random num based on reading /dev/urandom.
int generatenum() {
	int fd;
	unsigned long randint;
	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		perror("/dev/urandom");
		return(1);
	}
	read(fd, &randint, sizeof (unsigned long));
	close(fd);
	return(randint);
}

// putrandbyte prints a random byte from an open file.
int putrandbyte(int fd) {
    unsigned long randint;
    char c;
	int size;
	struct stat sb;

	randint = generatenum();

	if (fstat(fd, &sb) < 0) {
    	perror("fstat");
    	return(1);
    }
    size = sb.st_size;
    // If the file has a size of 0, return without printing anything.
    if (size == 0)
    	return(0);
    
    // The modulus prevents the file from reading a byte the file doesn't
    // contain and the abs prevents the random byte from being negative when
    // passed to lseek.
	if (lseek(fd, abs(randint % size), SEEK_SET) < 0) {
		perror("lseek");
		return(1);
	
}	int err;
	if ((err = read(fd, &c, 1)) <= 0) {
		perror("read");
		return(1);
	}
	putchar(c);
	return(0);
}

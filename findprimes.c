#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NPROC 5
#define GOAL 5000

int assignmentpipe[NPROC][2], donepipe[NPROC][2];

extern void doit(int from, int to);
extern void spawnall(), be_master();
extern void be_child(int fd_from_master, int fd_to_master);
extern void sendpipe(int topipe, int value);
extern int readfrompipe(int frompipe);

int main(int argc, char **argv)
{
    if (argc != 2) {
	   fprintf(stderr, "usage: %s dir\n", argv[0]);
	return(1);
    }

    // If the directory already exists exit.
    if (mkdir(argv[1], 0777)) {
	   perror(argv[1]);
	   return(1);
    }
    // If I'm unable to cd into the directory exit.
    if (chdir(argv[1])) {
	  perror(argv[1]);
	  return(1);
    }

    doit(2, 10);
    spawnall();
    be_master(10);
    return(0);
}


void spawnall()
{
    int i;
    for (i = 0; i < NPROC; i++) {
        if (pipe(assignmentpipe[i])) {
            perror("pipe");
            exit(0);
        }
        if (pipe(donepipe[i])) {
            perror("pipe");
            exit(0);
        }
        switch (fork()) {
            case -1:
                perror("fork");
                exit(1);
            case 0:
                // close the appropriate pipes for the child.
                close(assignmentpipe[i][1]);
                close(donepipe[i][0]);
                be_child(assignmentpipe[i][0], donepipe[i][1]);
                exit(0);
                break;
            default:
                // close the appropriate pipes for the parent.
                close(assignmentpipe[i][0]);
                close(donepipe[i][1]);
                break;
        }
    }
}


void be_child(int frommaster, int tomaster)  /* does not return */
{
    // Sends 0 to the parent twice through the pipe.
    int to,from;
    sendpipe(tomaster,0);
    sendpipe(tomaster,0);

    // Repeatedly grabs a to integer and a from integer from the pipe.
    // It calculates the primes between them and then notifies the master
    // that it has completed these checks through the pipe.
    while (1) {
        from = readfrompipe(frommaster);
        to = readfrompipe(frommaster);
        doit(from,to);
        sendpipe(tomaster,from);
        sendpipe(tomaster,to);
    }
}


void be_master(int done_so_far)
{
    // Keeps track of the to and from value of the children.
    int to[NPROC];
    int from[NPROC];

    // j keeps track of which child it's talking to and todo stores the
    // number of primes to calcualte per child on a given run.
    int j,todo;

    while (done_so_far < GOAL) {
        // The parent recives the to and from calculated by the child.
        for (j = 0; j < NPROC; j++) {
            from[j] = readfrompipe(donepipe[j][0]);
            to[j] = readfrompipe(donepipe[j][0]);
            // If more primes have been calcualted and no numbers have been
            // skipped than done_so_far is undated to the newest number.
            if (done_so_far < to[j] && done_so_far + 1 == from[j])
                done_so_far = to[j];
        }

        if (done_so_far == GOAL)
            return;

        // todo calculates the number of primes for each child to calcualte.
        todo = (((done_so_far * done_so_far) - done_so_far)/NPROC);

        if (todo*NPROC > GOAL)
            todo = ((GOAL - done_so_far)/NPROC);

        // each child is sent the prime ranger for it to check.
        for (j = 0; j < NPROC; j++) {
            sendpipe(assignmentpipe[j][1],(done_so_far + j*todo + 1));
            sendpipe(assignmentpipe[j][1],(done_so_far + (j+1)*todo));
        }
    }
}


void doit(int from, int to)
{
    DIR * dp;
    struct dirent * currentdir;

    int i, tempfile,fail;

    // When the pipe is empty it will send a -1 as both to and from.
    // The from is changed to 0 and so this will do nothing in this case.
    if (from < 0)
        from = 0;
    for (i=from; i <= to; i++) {
        if ((dp = opendir(".")) == NULL) {
            perror(".");
            return;
        }
        fail = 0;
        while ((currentdir = readdir(dp)) && currentdir) {
            // Skip over . and ..
            if (atoi(currentdir->d_name) == 0)
                continue;
            // If the number isn't prime, mark fail as 1.
            if (i % atoi(currentdir->d_name) == 0) 
                fail = 1;
        }
        closedir(dp);

        if (fail == 0) {
            char buf[20];
            sprintf(buf, "%d", i);
            tempfile = open(buf, O_CREAT, 0666);
            close(tempfile);
        }
    }
}

int readfrompipe(int frompipe) {
    int x,len;
    if ((len = read(frompipe, &x, sizeof x)) != sizeof x) {
        if (len < 0)
            perror("read");
        else if (len > 0)
            perror("too small");
        return(-1);
    } else {
        return(x);
    }
}

void sendpipe(int topipe, int value) {
    if (write(topipe, &value, sizeof value) != sizeof value) {
        perror("write");
        exit(1);
    }
}
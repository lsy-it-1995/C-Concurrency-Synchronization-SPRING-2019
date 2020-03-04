#include "concurrency.h"

pthread_mutex_t lock[HISTSIZE];

void *FG_num(void *file){	
	int ch, rv;
	
	while((rv = Read(*(int*)file, &ch, 1)) != 0) {
        ch -= '0';
        // if the byte is out of range, skip it
        if (ch < 0 || ch >= HISTSIZE) {
            fprintf(stderr, "skipping %c\n", ch);
            continue;
        }
        pthread_mutex_lock(&lock[(int)ch]);
        histogram[(int)ch]++;
        pthread_mutex_unlock(&lock[(int)ch]);
    }
    // cleanup
    Close(*(int*)file);
	return NULL;
}


void concurrent_fg(char *dirname) {
	// your code here
    // try to change to directory specified in args
    Chdir(dirname);
    // open directory stream
    DIR *dp = Opendir("./");
    
    struct dirent *ep;
    // start by assuming there will be only 8 files
    // reallocate if this guess is too small
    // allocate space for file information
	int numfds = 8, *fds = Malloc(numfds*sizeof(int)), count = 0;

    while ((ep = readdir(dp)) != NULL) {
        // make sure file exists
        struct stat sb;
        Stat(ep->d_name, &sb);
        // make sure that its a "regular" file (i.e not a directory, link etc.)
        if ((sb.st_mode & S_IFMT) != S_IFREG) {
            continue;
        }
        // open file
        fds[count] = Open(ep->d_name, O_RDONLY);
        count++;
        // if the number of files is more than what we guessed
        // ask for more space
        if (count == numfds) {
            numfds = numfds * 2;
            fds = Realloc(fds, numfds*sizeof(int));
        }
    }
    // cleanup
    Closedir(dp);
	    
    int i=0;
    pthread_t pthread_arr[count];
	for(i = 0; i < HISTSIZE; i++){
		histogram[i] = 0;
	}
	
    // go through all the files
    for (i = 0; i < count; i++) {
        pthread_create(&pthread_arr[i], NULL, FG_num, &fds[i]);
    }
    for( i = 0 ; i < count ; i++)
    {
        pthread_join(pthread_arr[i], NULL);
    }
    // cleanup
    free(fds);
}

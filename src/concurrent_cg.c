#include "concurrency.h"

/*

creates a peer thread per file in directory. 
each thread processes its assigned file keeping the counnt of the occurences 
as a local histogram array variable. upon completion of processing the file, 
the thread obtains the lock to the shard global histogram array to update with
its local counts. once all files have been processed, the concurrent_CG function
return to the main thread. the main thread signals the display thread via
the cmdpipe which then prints the histogram and stat
*/


void *CG_num(void *file){
	static pthread_mutex_t lock;
	int localGram[HISTSIZE], ch, rv, i;
	
	for(i=0; i < HISTSIZE; i++){
		localGram[i]=0;
	}
	
	while((rv = Read( (*(int *)file), &ch, 1)) != 0) {
		ch -= '0';
		// if the byte is out of range, skip it
		if (ch < 0 || ch >= HISTSIZE) {
			fprintf(stderr, "skipping %c\n", ch);
			continue;
		}
	   localGram[(int)ch]++;           
	}

	pthread_mutex_lock(&lock);
	for(i = 0; i < HISTSIZE; i++){
		histogram[i] += localGram[i];

	}
	pthread_mutex_unlock(&lock);
    Close(*(int*)file);
	return NULL;
}


void concurrent_cg(char *dirname) {
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

    for(i = 0; i< count; i++)
    {
		pthread_arr[i] = 0;
        pthread_create(&pthread_arr[i], NULL, CG_num, &fds[i]);
    }
    for(i = 0; i< count; i++)
    {
        pthread_join(pthread_arr[i], NULL);
    }
    // cleanup
    free(fds);
}

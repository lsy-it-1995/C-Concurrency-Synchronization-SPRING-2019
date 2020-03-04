#include "concurrency.h"

pthread_mutex_t lock;

void *RW_findNUM(void *file){
    int ch, rv;
    
	while((rv = Read( (*(int *)file), &ch, 1)) != 0) {
		ch -= '0';
		// if the byte is out of range, skip it
		if (ch < 0 || ch >= HISTSIZE) {
			fprintf(stderr, "skipping %c\n", ch);
			continue;
		}
		pthread_mutex_lock(&lock);
		histogram[(int)ch]++;
		pthread_mutex_unlock(&lock);           
	}

	Close(*(int *)file);
	return NULL;
}

void snapshot_stats() {
    int i, sum = 0;
    int cfreq[HISTSIZE];
    statsnap s;
    s.n = 0;
    
    // part 2 put your locks here
	pthread_mutex_lock(&lock);
    s.mode = -1;
    int maxfreq = -1;
    for (i = 0; i < HISTSIZE; i++) {
        s.n += histogram[i];
        cfreq[i] = s.n;
        sum += i * histogram[i];        
        if (maxfreq < histogram[i]) {
            s.mode = i;
            maxfreq = histogram[i];
        }
    }

    // part 2 put your locks here
	pthread_mutex_unlock(&lock);

    s.mean = calc_mean_median(sum, s.n, cfreq, &s.median);
    // send stats
    Write(statpipe[1], &s, sizeof(statsnap));
}

void snapshot_histogram() {
    int i;
    histsnap s;
    s.n = 0;
    
    // part 2 put your locks here
    pthread_mutex_lock(&lock);
    for (i = 0; i < HISTSIZE; i++) {
        s.n += histogram[i];
        s.hist[i] = histogram[i];
    }
    
    // part 2 put your locks here
    pthread_mutex_unlock(&lock);

    // send n and histogram   
    Write(histpipe[1], &s, sizeof(histsnap));
}

void *readerwriter_stat_task(void *data) {
    while(1) {
        usleep(READSLEEP);
        snapshot_stats();
    }
    return NULL;
}

void *readerwriter_hist_task(void *data) {
    while(1) {
        usleep(READSLEEP);
        snapshot_histogram();
    }
    return NULL;
}

void start_readers(pthread_t *readers) {
    pthread_create(&readers[0], NULL, readerwriter_stat_task, NULL);
    pthread_create(&readers[1], NULL, readerwriter_hist_task, NULL);

}

void readerwriter(char *dirname) {
        // start reader threads
    pthread_t readers[2];
    start_readers(readers);

    // your code here
      Chdir(dirname);
    // open directory stream
    DIR *dp = Opendir("./");

    struct dirent *ep;
    // start by assuming there will be only 8 files
    // reallocate if this guess is too small
    int numfds = 8;
    // allocate space for file information
    int *fds = Malloc(numfds*sizeof(int));
    int count = 0;
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
    int i;
    pthread_t thread_array [count];
    // go through all the files
    for (i = 0; i < count; i++) {
        pthread_create(&thread_array[i],NULL,RW_findNUM,&fds[i]);   
    }

    for(i = 0; i < count; i++){
        pthread_join(thread_array[i],NULL);
    }

    // remember to cancel the reader threads when you are done processing all the files
    pthread_cancel(readers[0]);
    pthread_cancel(readers[1]);
    free(fds);
}

#include "../include/server.h"

// /********************* [ Helpful Global Variables ] **********************/
int num_dispatcher = 0; //Global integer to indicate the number of dispatcher threads   
int num_worker = 0;  //Global integer to indicate the number of worker threads
FILE *logfile;  //Global file pointer to the log file
int queue_len = 0; //Global integer to indicate the length of the queue
pthread_t worker_thread[MAX_THREADS];
pthread_t dispatcher_thread[MAX_THREADS];
int counter = 0;

/* TODO: Intermediate Submission
  TODO: Add any global variables that you may need to track the requests and threads
  [multiple funct]  --> How will you track the p_thread's that you create for workers?
  [multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
  [multiple funct]  --> Might be helpful to track the ID's of your threads in a global array
  What kind of locks will you need to make everything thread safe? [Hint you need multiple]
  What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
  How will you track the number of images in the database?
  How will you track the requests globally between threads? How will you ensure this is thread safe? Example: request_t req_entries[MAX_QUEUE_LEN]; 
  [multiple funct]  --> How will you update and utilize the current number of requests in the request queue?
  [worker()]        --> How will you track which index in the request queue to remove next? 
  [dispatcher()]    --> How will you know where to insert the next request received into the request queue?
  [multiple funct]  --> How will you track the p_thread's that you create for workers? TODO
  How will you store the database of images? What data structure will you use? Example: database_entry_t database[100]; 
*/
//Large structures 
database_entry_t database[1000]; // Just randomly guess that we won't do more than 1000 images
request_t req_entries[MAX_QUEUE_LEN];
//locks
pthread_mutex_t request_queue_lock = PTHREAD_MUTEX_INITIALIZER;
//condition vars
pthread_cond_t req_queue_notfull = PTHREAD_COND_INITIALIZER;
pthread_cond_t req_queue_notempty = PTHREAD_COND_INITIALIZER;
//queue head and tail
int queue_head = 0;
int queue_tail = 0;



//TODO: Implement this function
/**********************************************
 * image_match
   - parameters:
      - input_image is the image data to compare
      - size is the size of the image data
   - returns:
       - database_entry_t that is the closest match to the input_image
************************************************/
//just uncomment out when you are ready to implement this function
database_entry_t image_match(char *input_image, int size)
{
  const char *closest_file     = NULL;
	int         closest_distance = 10;
  int closest_index = 0;
  for(int i = 0; i < counter; i++)/* replace with your database size, originally 0*/ 
	{
		const char *current_file = database[i].buffer; /* TODO: assign to the buffer from the database struct*/
		int result = memcmp(input_image, current_file, size);
		if(result == 0)
		{
			return database[i];
		}

		else if(result < closest_distance)
		{
			closest_distance = result;
			closest_file     = current_file;
      closest_index = i;
		}
	}

	if(closest_file != NULL)
	{
    return database[closest_index];
	}
  else
  {
    printf("No closest file found.\n");
  }
  
  
}

//TODO: Implement this function
/**********************************************
 * LogPrettyPrint
   - parameters:
      - to_write is expected to be an open file pointer, or it 
        can be NULL which means that the output is printed to the terminal
      - All other inputs are self explanatory or specified in the writeup
   - returns:
       - no return value
************************************************/
void LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, char * file_name, int file_size){
   if (to_write == NULL) {
        printf("Thread: %d, Request #: %d, File: '%s', Size: %d\n", threadId, requestNumber, file_name, file_size);
    } else {
        fprintf(to_write, "Thread: %d, Request #: %d, File: '%s', Size: %d\n", threadId, requestNumber, file_name, file_size);
    }
}


/*
  TODO: Implement this function for Intermediate Submission
  * loadDatabase
    - parameters:
        - path is the path to the directory containing the images
    - returns:
        - no return value
    - Description:
        - Traverse the directory and load all the images into the database
          - Load the images from the directory into the database
          - You will need to read the images into memory
          - You will need to store the file name in the database_entry_t struct
          - You will need to store the file size in the database_entry_t struct
          - You will need to store the image data in the database_entry_t struct
          - You will need to increment the number of images in the database
*/
/***********/

void loadDatabase(char *path){struct dirent *entry; 
  DIR *dir = opendir(path);
  if (dir == NULL)
  {
    perror("Opendir ERROR");
    exit(0);
  }
  while ((entry = readdir(dir)) != NULL)
  {
    if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".DS_Store") != 0)
    {
      sprintf(database[counter].file_name, "%s/%s", path, entry->d_name);
      FILE *fp1;
      unsigned char *buffer1 = NULL;
      long fileLength1;

      // Open the image files
      fp1 = fopen(database[counter].file_name, "rb");
      if (fp1 == NULL) {
          perror("Error: Unable to open image files.\n");
          exit(1);
      }

      // Get the length of file 1
      fseek(fp1, 0, SEEK_END);
      fileLength1 = ftell(fp1);
      rewind(fp1);

      // Allocate memory to store file contents
      buffer1 = (unsigned char*)malloc(fileLength1 * sizeof(unsigned char));
    
      if (buffer1 == NULL) 
      {
        printf("Error: Memory allocation failed.\n");
        fclose(fp1);
        if (buffer1 != NULL) free(buffer1);
      }

      // Read file contents into memory buffers
      fread(buffer1, sizeof(unsigned char), fileLength1, fp1);
      database[counter].buffer = buffer1;
      database[counter].file_size = fileLength1;
      counter++;
    }
  }

  closedir(dir);
}


void * dispatch(void *thread_id) 
{   
  while (1) 
  {
    size_t file_size = 0;
    request_detials_t request_details;

    /* TODO: Intermediate Submission
    *    Description:      Accept client connection
    *    Utility Function: int accept_connection(void)
    */
    int client_fd;
    if ((client_fd = accept_connection()) == -1) {
      perror("Error in accept_connection()");
      continue;
    }

    /* TODO: Intermediate Submission
    *    Description:      Get request from client
    *    Utility Function: char * get_request_server(int fd, size_t *filelength)
    */
    char *filename;
    if ((filename = get_request_server(client_fd, &file_size)) == NULL) {
      close(client_fd);
      continue;
    }

   /* TODO
    *    Description:      Add the request into the queue
        //(1) Copy the filename from get_request_server into allocated memory to put on request queue
        
        //(2) Request thread safe access to the request queue

        //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull

        //(4) Insert the request into the queue
        
        //(5) Update the queue index in a circular fashion (i.e. update on circular fashion which means when the queue is full we start from the beginning again) 

        //(6) Release the lock on the request queue and signal that the queue is not empty anymore
   */
    pthread_mutex_lock(&request_queue_lock); //2

    //3
    while (queue_len >= MAX_QUEUE_LEN) {
      pthread_cond_wait(&req_queue_notfull, &request_queue_lock);
    }
    //4 - Not sure if i set this right?
    req_entries[queue_tail].buffer = filename;  
    req_entries[queue_tail].file_size = file_size;
    req_entries[queue_tail].file_descriptor = client_fd;
    //5
    queue_tail = (queue_tail + 1) % MAX_QUEUE_LEN;
    queue_len++;

    //clean up (6)
    pthread_cond_signal(&req_queue_notempty);
    pthread_mutex_unlock(&request_queue_lock);
  }
    // return NULL;
}

void * worker(void *thread_id) {

  // You may use them or not, depends on how you implement the function
  int num_request = 0;                                    //Integer for tracking each request for printing into the log file
  int fileSize    = 0;                                    //Integer to hold the size of the file being requested
  void *memory    = NULL;                                 //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char *mybuf;                                  //String to hold the contents of the file being requested


  /* TODO : Intermediate Submission 
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  int id = *((int*)thread_id);
  while (1) {
    /* TODO
    *    Description:      Get the request from the queue and do as follows
      //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock
      //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised

      //(3) Now that you have the lock AND the queue is not empty, read from the request queue

      //(4) Update the request queue remove index in a circular fashion

      //(5) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock  
      */
    pthread_mutex_lock(&request_queue_lock); //1

    while (queue_len == 0) {//2
      pthread_cond_wait(&req_queue_notempty, &request_queue_lock);
    }
    //3?
    char *image_buffer = req_entries[queue_head].buffer;
    fileSize = req_entries[queue_head].file_size;
    int fd = req_entries[queue_head].file_descriptor;
    //4
    queue_head = (queue_head + 1) % MAX_QUEUE_LEN;
    queue_len--;
    //5
    pthread_cond_signal(&req_queue_notfull);
    pthread_mutex_unlock(&request_queue_lock);
      
    /* TODO
    *    Description:       Call image_match with the request buffer and file size
    *    store the result into a typeof database_entry_t
    *    send the file to the client using send_file_to_client(int fd, char * buffer, int size)              
    */
    database_entry_t matched_image = image_match(image_buffer, fileSize);
    send_file_to_client(fd, matched_image.buffer, matched_image.file_size);

    /* TODO
    *    Description:       Call LogPrettyPrint() to print server log
    *    update the # of request (include the current one) this thread has already done, you may want to have a global array to store the number for each thread
    *    parameters passed in: refer to write up
    */
    LogPrettyPrint(logfile, id, num_request, matched_image.file_name, matched_image.file_size);
    num_request++;  //Where to declare this? printing requests needed
  }

}

int main(int argc , char *argv[])
{
   if(argc != 6){
    printf("usage: %s port path num_dispatcher num_workers queue_length \n", argv[0]);
    return -1;
  }


  int port            = -1;
  char path[BUFF_SIZE] = "no path set\0";
  num_dispatcher      = -1;                               //global variable
  num_worker          = -1;                               //global variable
  queue_len           = -1;                               //global variable
 

  /* TODO: Intermediate Submission
  *    Description:      Get the input args --> (1) port (2) database path (3) num_dispatcher (4) num_workers  (5) queue_length
  */
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len = atoi(argv[5]);

  /* TODO: Intermediate Submission
  *    Description:      Open log file
  *    Hint:             Use Global "File* logfile", use "server_log" as the name, what open flags do you want?
  */
  logfile = fopen("server_log", "w");
 

  /* TODO: Intermediate Submission
  *    Description:      Start the server
  *    Utility Function: void init(int port); //look in utils.h 
  */
  init(port);

  /* TODO : Intermediate Submission
  *    Description:      Load the database
  *    Function: void loadDatabase(char *path); // prototype in server.h
  */
  loadDatabase(path);

  /* TODO: Intermediate Submission
  *    Description:      Create dispatcher and worker threads 
  *    Hints:            Use pthread_create, you will want to store pthread's globally
  *                      You will want to initialize some kind of global array to pass in thread ID's
  *                      How should you track this p_thread so you can terminate it later? [global]
  */
  int thread_id[MAX_THREADS];
  for (int i = 0; i < MAX_THREADS; i++){
    thread_id[i] = i;
  }


  for (int k = 0; k < num_dispatcher; k++) { // should be one big global array? no.
    if (pthread_create(&dispatcher_thread[k], NULL, dispatch, &thread_id[k]) != 0){
      perror("thread creation failed\n");
    }

    else{
      printf("dispatch %d created successfully\n", k);
    }

  } 
  for (int j = 0; j < num_worker; j++) {

    if (pthread_create(&worker_thread[j], NULL, worker, &thread_id[j]) != 0){
      perror("thread creation failed\n");
    }
    else{
      printf("worker %d created successfully\n", j);
    }

  }



  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  int i;
  for(i = 0; i < num_dispatcher; i++){
    fprintf(stderr, "JOINING DISPATCHER %d \n",i);
    if((pthread_join(dispatcher_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join dispatcher thread %d.\n", i);
    }
  }
  for(i = 0; i < num_worker; i++){
   // fprintf(stderr, "JOINING WORKER %d \n",i);
    if((pthread_join(worker_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join worker thread %d.\n", i);
    }
  }
  fprintf(stderr, "SERVER DONE \n");  // will never be reached in SOLUTION
  fclose(logfile);//closing the log files

}
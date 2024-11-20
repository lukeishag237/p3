#include "../include/client.h"



int port = 0;

pthread_t worker_thread[100];
int worker_thread_id = 0;
char output_path[1028];

processing_args_t req_entries[100];

/* TODO: implement the request_handle function to send the image to the server and recieve the processed image
* 1. Open the file in the read-binary mode (i.e. "rb" mode) - Intermediate Submission
* 2. Get the file length using the fseek and ftell functions - Intermediate Submission
* 3. set up the connection with the server using the setup_connection(int port) function - Intermediate Submission
* 4. Send the file to the server using the send_file_to_server(int fd, FILE *file, int size) function - Intermediate Submission
* 5. Receive the processed image from the server using the receive_file_from_server(int socket, char *file_path) function
* 6. receive_file_from_server saves the processed image in the output directory, so pass in the right directory path
* 7. Close the file
*/
void * request_handle(void * img_file_path)
{
    char *file_path = (char*)img_file_path;
    FILE *imgPath = fopen(file_path, "rb");
    if (imgPath == NULL) {
        perror("File open error within request_handle\n");
        return NULL;
    }
    if (fseek(imgPath, 0, SEEK_END) != 0) {
        perror("fseek error in request_handle\n");
        fclose(imgPath);
        return NULL;
    }
    long file_size = ftell(imgPath);
    if (file_size == -1) {
        perror("ftell error\n");
        fclose(imgPath);
        return NULL;
    }
    int file_size_int = file_size;
    int fd = setup_connection(port);
    // ; part 5 and part 6 same line?
    if (send_file_to_server(fd, imgPath, file_size_int) < 0) {
        perror("error sending to server\n");
        fclose(imgPath);
        close(fd);
        return NULL;
    }

    char out_paths[2048];
    sprintf(out_paths, "%s/%s", "output", file_path);

    if (receive_file_from_server(fd, out_paths) < 0) { //double check all good, recieves image
        perror("error receiving file froms server\n");
        fclose(imgPath);
        close(fd);
        return NULL;
    }

    close(fd);
    fclose(imgPath);
    return NULL;
}

/* Directory traversal function is provided to you. */
void directory_trav(char * img_directory_path)
{
    char dir_path[BUFF_SIZE]; 
    strcpy(dir_path, img_directory_path);
    struct dirent *entry; 
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("Opendir ERROR");
        exit(0);
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".DS_Store") != 0)
        {
            sprintf(req_entries[worker_thread_id].file_name, "%s/%s", dir_path, entry->d_name);
            printf("New path: %s\n", req_entries[worker_thread_id].file_name);
            pthread_create(&worker_thread[worker_thread_id], NULL, request_handle, (void *) req_entries[worker_thread_id].file_name);
            worker_thread_id++;
        }
    }
    closedir(dir);
    for(int i = 0; i < worker_thread_id; i++)
    {
        pthread_join(worker_thread[i], NULL);
    }
}


int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: ./client <directory path> <Server Port> <output path>\n");
        exit(-1);
    }
    /*TODO:  Intermediate Submission
    * 1. Get the input args --> (1) directory path (2) Server Port (3) output path
    */
    char directory_path[strlen(argv[1])];
    strcpy(directory_path, argv[1]);
    port = atoi(argv[2]);
    strcpy(output_path, argv[3]);
    /*TODO: Intermediate Submission
    * Call the directory_trav function to traverse the directory and send the images to the server
    */
    directory_trav(directory_path);
    return 0;  
}
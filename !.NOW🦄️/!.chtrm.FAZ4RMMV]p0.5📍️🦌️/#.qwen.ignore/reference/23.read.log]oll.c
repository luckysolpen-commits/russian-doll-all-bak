#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp = fopen("log.txt", "r+");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    int old_bufferSize = 0;
    int new_bufferSize = 0;
    int read_count = 0;

    while (1) {
        fseek(fp, 0, SEEK_END); // seek to end of file
        new_bufferSize = ftell(fp); // get current file size

        if (new_bufferSize > old_bufferSize) { // new data was appended
            fseek(fp, old_bufferSize, SEEK_SET); // go back to start of new data
            char buffer[new_bufferSize - old_bufferSize]; // allocate buffer for new data
            fread(buffer, 1, new_bufferSize - old_bufferSize,fp); 
// read new data

          
          
             printf("New token: %s\n", buffer);
            printf("New buff-size: %d\n", new_bufferSize);
              printf("readcount: %d\n", read_count);
              
                old_bufferSize = new_bufferSize; // update old buffer size
            read_count++;

        }

        usleep(1000); // poll forever (adjust the delay as needed)
    }

    fclose(fp);
    return 0;
}


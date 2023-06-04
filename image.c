// this contains function to open and close to file that holds the file system image
#include <fcntl.h>
#include <unistd.h>
#include "image.h"

// global variables
int image_fd;

// open the image file of the given name, create it if it doesn't exist, and truncate
// to 0 size if truncate is true. use open() to create the file
int image_open(char *filename, int truncate){
    if(truncate){
        image_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
    } else {
        image_fd = open(filename, O_RDWR | O_CREAT, 0600);
    }
    return image_fd;
}

// close the image file. use close() to close the file
int image_close(void){
    return close(image_fd);
}
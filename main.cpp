#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>

//socketimports
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace boost::interprocess;

int order = 10;
int samples = 10;

int sockfd;
int portno = 51717;
int n;

struct sockaddr_in serv_addr;
struct hostent *server = gethostbyname("localhost");

char buffer[256];

void error(const char *msg) {
    perror(msg);
    exit(0);
}

//
//  Sets all elements of a matrix to value.
//  The elements on the main diagonal are set to 0.
//
void fillMatrix(long *array, long value, int order) {
    for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
            if (i == j)
                array[order * i + j] = 0;
            else
                array[order * i + j] = value;
        }
    }
}

// Fills all matrices of dimension ORDER which are stored in array.
// The number of matrices is assumend to be NUM_SAMPLES
// The i-th matrix in the array i initialized with value i, and 0
// on the main diagonal
//
void fillMatrices(long *matrix, int numSamples, int order) {
    for (int i = 0; i < numSamples; i++) {
        fillMatrix(matrix + i * order * order, i, order);
    }
}

void writeFifo(char *file, int fifonr, char *message) {
    int proc;

    mkfifo(file, static_cast<__mode_t>(fifonr));

    proc = open(file, O_WRONLY);
    std::cout << "send message: " << message << std::endl;
    write(proc, message, sizeof(message));
    close(proc);
    unlink(file);
}

void printSharedMemoryMat(long *arrayptr) {

    for (int i = 1; i <= order * order * samples; i++) {

        if (i % (order * order) == 0) {
            std::cout << arrayptr[i] << std::endl;
        } else {
            std::cout << arrayptr[i];
        }
    }
}

void printSharedMemoryRes(long *arrayptr) {

    for (int i = 0; i < samples; i++) {

        std::cout << arrayptr[i] << "  " << std::endl;
    }
}

void initSocket() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    else
        std::cout << "connected to daemon" << std::endl;

}

void writeToSocket(char *message) {

    std::cout << "sending to daemon: " << message << std::endl;

    bzero(buffer, 256);
    n = write(sockfd, message, strlen(message));
    if (n < 0)
        error("ERROR writing to socket");
}

char *readFromSocket() {
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0) {
        error("ERROR reading from socket");
        return "error";
    } else {
        std::cout << "received from daemon: " << buffer << std::endl;
        return buffer;
    }

}

int main(int argc, char *argv[]) {

    std::cout << "huhu" << std::endl;

    unsigned long sizeMatrixElements = order * order * samples * sizeof(long);
    unsigned long sizeResultsElements = samples * sizeof(long);

    //connect to daemon
    initSocket();

/*
    //Remove shared memory on construction and destruction
    struct shm_remove {
        shm_remove() {
            shared_memory_object::remove("MatSharedMemory");
            shared_memory_object::remove("ResultsSharedMemory");
        }

        ~shm_remove() {
            //shared_memory_object::remove("MatSharedMemory");
            //shared_memory_object::remove("ResultsSharedMemory");
        }
    } remover;*/



    //Create a shared memory object for matricies and results
    shared_memory_object matriciesSHM(open_or_create, "MatSharedMemory", read_write);
    shared_memory_object resultSHM(open_or_create, "ResultsSharedMemory", read_write);

    //Set size
    matriciesSHM.truncate(sizeMatrixElements);
    resultSHM.truncate(sizeResultsElements);


    //Map region for Matricies to send
    mapped_region matRegion(matriciesSHM, read_write);
    long *matricesptr = (long *) matRegion.get_address();

    //Map region for Results to collect
    mapped_region resultsRegion(resultSHM, read_write);
    long *resultsptr = (long *) resultsRegion.get_address();


    //fill matricies in shared memory
    fillMatrices(matricesptr, samples, order);
    printSharedMemoryMat(matricesptr);


    //send start command to daemon
    writeToSocket("start");

    //writeToSocket("samples 5");

    //get finish command from daemon
    char *message = readFromSocket();
    std::string msg(message);


    if (msg.compare("finish") == 0) {
        printSharedMemoryRes(resultsptr);
    }

    std::cout << "ende" << std::endl;

    return 0;
}

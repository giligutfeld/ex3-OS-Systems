#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "ex3.h"

// Reads the configuration file, create the list of producers and return the number of Co-Editors.
int readConfigurationFile(std::vector<Producer*> &producers, char* path, int &semaphores) {
    FILE *f;
    char *line;
    size_t len = 0;
    int producerNumber, numberOfProducts, queueSize;

    // Open the configuration file and check it works.
    f = fopen(path, "r");
    if (f == NULL) {
        exit(-1);
    }

    // Read each line from the configuration file.
    while (getline(&line, &len, f) != -1) {
        // The first line is the producer number.
        producerNumber = std::stoi(line);
        getline(&line, &len, f);

        // If the second line is empty, so we finished read the file and the last line was the Co-Editor queue size.
        if (strcmp(line, "\n") == 0) {
            break;
        }

        // The second line is the number of products.
        numberOfProducts = std::stoi(line);
        getline(&line, &len, f);

        // The third line is the queue size.
        queueSize = std::stoi(line);
        getline(&line, &len, f);

        // Create another producer.
        semaphores++;
        producers.push_back(new Producer(producerNumber, queueSize, numberOfProducts, semaphores));
    }

    // Close the file and return the Co-Editor queue size.
    fclose(f);
    return producerNumber;
}

// Create strings from the producer
void *runProducer(void *producer) {
    Producer p = (Producer *) producer;
    int flag = 1;

    // Create strings until it done
    while (flag) {
        flag = p.createString();
    }
    return NULL;
}

// Get the strings from the producers and transfer them to the co-editors queue.
void *runDispatcher(void *dispatcher) {
    Dispatcher *d = (Dispatcher *) dispatcher;

    // Create number of the current producer, an array of finished producers and a counter of the finished producers.
    int current = -1, finishedProducers[d->_numOfProducers], counter = 0, type;

    // Read strings until all the producers finished.
    while (counter != d->_numOfProducers) {

        // Sleep 50 milliseconds and continue to the next producer.
        usleep(50000);
        current++;

        // After arriving to the last producer, move back to the first one.
        if (current == d->_numOfProducers) {
            current = 0;
        }

        // Check the producer still creates strings.
        if (finishedProducers[current]) {
            continue;
        }

        // Get the next string from the producer's queue.
        std::string str = d->_producers[current]->_buffer->remove();

        // If the queue is empty, continue to the next producer.
        if (str == "empty")
            continue;

        // When the producer finish change the value in the compatible index in the array and add 1 to the counter.
        if (str == DONE) {
            finishedProducers[current] = 1;
            counter++;
            continue;
        }

        // Get the type of the string and push it to the compatible queue.
        type = 0;
        if (str.find("NEWS") != std::string::npos)
            type = 1;
        else if (str.find("WEATHER") != std::string::npos)
            type = 2;
        d->_buffers[type]->insert(str);
    }

    // When finished, push "DONE" to the queues.
    d->_buffers[0]->insert(DONE);
    d->_buffers[1]->insert(DONE);
    d->_buffers[2]->insert(DONE);
    return NULL;
}

// Get each string in the compatible category from the dispatcher and transfer it to the buffer.
void *runCoEditor(void* coEditor) {
    CoEditor *c = (CoEditor *) coEditor;
    std::string str;

    // While the string isn't done it continues the loop.
    while ((str = c->_unBoundedBuffer->remove()) != DONE) {

        // Sleep 50 milliseconds
        usleep(50000);

        // Check the queue is not empty.
        if (str == "empty") {
            continue;
        }

        // Wait until the buffer is not full.
        while (c->_boundedBuffer->isFull()) {
            usleep(50000);
        }

        // Push the string to the co-editor buffer.
        c->_boundedBuffer->insert(str);
    }

    // When finished, push "DONE" to the buffer.
    while (c->_boundedBuffer->isFull()) {
        usleep(50000);
    }
    c->_boundedBuffer->insert(DONE);
    return NULL;
}

// Get each string from the co-editors shared queue and print it.
void runScreenManager(BoundedBuffer *coEditorsSharedQueue) {
    int count = 0;

    // Get strings until we get "DONE" from all the co-editors.
    while (count != NUM_CO_EDITORS) {
        std::string m = coEditorsSharedQueue->remove();

        // Check the queue is not empty.
        if (m == "empty")
            continue;

        // Count the number of finished co-editors.
        if (m == DONE) {
            count++;
            continue;
        }
        std::cout << m << std::endl;
    }
}

int main(int argc, char** argv) {

    // Checks there are 2 arguments.
    if (argc != 2) {
        return -1;
    }

    std::vector<Producer*> producers;
    int semaphores = 0, i;

    // Reads the configuration file.
    int bufferSize = readConfigurationFile(producers, argv[1], semaphores);
    int numberOfProducers = semaphores;

    // Create pthread_t arrays.
    pthread_t producerThread[numberOfProducers];
    pthread_t dispatcherThread;
    pthread_t coEditorThread[NUM_CO_EDITORS];

    // Create a bounded buffer for the Co-Editors shared queue.
    BoundedBuffer *coEditorsSharedQueue = new BoundedBuffer(bufferSize, ++semaphores);

    // Create unbounded buffers for the dispatcher.
    std::vector<UnBoundedBuffer*> dispatcherBuffers;
    for (i = 0; i < NUM_CO_EDITORS; i++) {
        dispatcherBuffers.push_back(new UnBoundedBuffer(++semaphores));
    }

    // Create the Dispatcher and the Co-Editors.
    Dispatcher* dispatcher = new Dispatcher(producers, dispatcherBuffers, numberOfProducers);
    std::vector<CoEditor*> coEditors;
    for (i = 0; i < NUM_CO_EDITORS; i++) {
        coEditors.push_back(new CoEditor(dispatcherBuffers[i], coEditorsSharedQueue));
    }

    // Create threads for each of the producers, dispatcher and co-editors
    if (pthread_create(&dispatcherThread, NULL, runDispatcher, (void *) dispatcher) != 0) {
        exit(1);
    }
    for (i = 0; i < numberOfProducers; i++) {
        if (pthread_create(&producerThread[i], NULL, runProducer, (void *) producers[i]) != 0) {
            exit(1);
        }
    }
    for (i = 0; i < NUM_CO_EDITORS; i++) {
        if (pthread_create(&coEditorThread[i], NULL, runCoEditor, (void *) coEditors[i]) != 0) {
            exit(1);
        }
    }

    runScreenManager(coEditorsSharedQueue);
    return 0;
}

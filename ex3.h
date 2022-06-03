//
// Created by Gili Gutfeld on 03/06/2022.
//

#ifndef EX3_EX3_H
#define EX3_EX3_H

#include <queue>
#include <random>
#include <sys/sem.h>

#define DONE "DONE"
#define NUM_CO_EDITORS 3

union semun {
    int val;	            // value for SETVAL
    struct semid_ds *buf;	// for IPC_STAT, IPC_SET
    unsigned short *array;	// array for GETALL, SETALL
};

class UnBoundedBuffer : public std::queue<std::string> {

protected:
    int _semid;
    int _num;
    struct sembuf sops[1];

public:

    // Constructor
    UnBoundedBuffer(int num):_num(num) {
        // Create a semaphore
        _semid = semget(IPC_PRIVATE, num, IPC_CREAT);
        union semun sem;
        sem.val = 1;
        semctl(_semid, num, IPC_SET, sem);
        sops->sem_num = _num;
        sops->sem_flg = SEM_UNDO;
    };

    ~UnBoundedBuffer() {
        semctl(_semid, _num, IPC_RMID);
    }

    virtual void insert(std::string s) {
        // Lock the semaphore
        sops->sem_op = -1;
        semop(_semid, sops, 1);

        // Push the string to the queue and unlock the semaphore
        queue<std::string>::push(s);
        sops->sem_op = 1;
        semop(_semid, sops, 1);
    }

    virtual std::string remove() {
        // Lock the semaphore
        sops->sem_op = -1;
        semop(_semid, sops, 1);
        std::string string;

        // If the queue is empty return -1
        if (queue<std::string>::empty()) {
            string = "empty";
        } else {
            string = queue<std::string>::front();
            queue<std::string>::pop();
        }

        // Unlock the semaphore and return the string
        sops->sem_op = 1;
        semop(_semid, sops, 1);
        return string;
    }
};

class BoundedBuffer : public UnBoundedBuffer {

private:
    int _size;
    int _current;

public:

    // Constructor
    BoundedBuffer(int size, const int num):UnBoundedBuffer(num), _size(size), _current(0){};

    // Check if the buffer is full
    int isFull() const {
        return _size == _current;
    }

    void insert(std::string s) {
        // Check the buffer is not full and insert the string to the buffer
        if (!isFull()) {
            _current++;
            UnBoundedBuffer::insert(s);
        }
    }

    std::string remove() {
        // Check the queue isn't empty and remove string from the buffer
        if (_current > 0) {
            _current--;
        }
        return UnBoundedBuffer::remove();
    }
};

class Producer {

private:
    int _num;
    int _size;
    int _produced;

public:
    BoundedBuffer *_buffer;

    // Constructor
    Producer(int num, int size, int products, int semaphore):_size(products), _num(num){
        _produced = 0;
        _buffer = new BoundedBuffer(size, semaphore);
    };

    // Copy constructor
    Producer(Producer *producer) {
        _num = producer->_num;
        _size = producer->_size;
        _produced = producer->_produced;
        _buffer = producer->_buffer;
    }

    int createString() {

        // Check there is space in the buffer
        if (_buffer->isFull()) {
            return 1;
        }

        // If it produced all the products, insert "DONE" and return 0
        if (_produced == _size) {
            _buffer->insert(DONE);
            return 0;
        }

        // Generate a random type from the categories
        std::string categories[NUM_CO_EDITORS] = {"SPORTS", "NEWS", "WEATHER"};
        std::string type = categories[rand() % NUM_CO_EDITORS];

        // Produces a new string and inserts it to the buffer.
        _buffer->insert("producer " + std::to_string(_num) + " " + type + " " + std::to_string(++_produced));
        return 1;
    }
};

class Dispatcher {
public:
    std::vector<Producer*> _producers;
    std::vector<UnBoundedBuffer*> _buffers;
    int _numOfProducers;

    // Constructor
    Dispatcher(std::vector<Producer*> &producers, std::vector<UnBoundedBuffer*> &buffers, int numOfProducers):
            _producers(producers), _buffers(buffers), _numOfProducers(numOfProducers){};
};

class CoEditor {
public:
    UnBoundedBuffer* _unBoundedBuffer;
    BoundedBuffer* _boundedBuffer;

    CoEditor(UnBoundedBuffer *unBoundedBuffer, BoundedBuffer *boundedBuffer): _unBoundedBuffer(unBoundedBuffer),
                                                                              _boundedBuffer(boundedBuffer){};
};
#endif //EX3_EX3_H

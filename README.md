# Concurrent Mergesort 

## 1. Description of the Project

[//]: # "What was the purpose of the project?" 
The objective of this project is to implement a multiprocess sorting system. For this purpose, the global sorting method will be divided into several tasks, some of them independent of each other, so that they can be parallelized to reduce the execution time. This kind of architectures can be used to tackle problems with large volumes of problems with large volumes of data in real applications.


[//]: # "What your application does?" 
The main purpose of the application is to sort an array of numbers using concurrence to make the algorithm scalable. The arrays with which to be sorted can be found in the `Data` directory inside the `src` directory.

[//]: # "What problem does it solve" 


[//]: # "What was your motivation?" 
The main motivation of this program was to apply all the concepts learned in the Operating Systems course in order to build a highly scalable sorting algorithm and deepen the knowledge on low level Operating Systems mechanisms which can be used in C.

[//]: # "Why did you build this project?" 


[//]: # "Building Procedure" 
In order to carry out this project, we use an incremental approach. Starting from a monoprocessing ordering system, which has been modified using operating systems mechanims such as shared memory, message queue, signal and semaphores until the multiprocessing version is reached.

### Algorithm Overview
  ![Selection_074](https://user-images.githubusercontent.com/46072805/192671599-48c65b59-7d8a-4886-8d8f-5a12507b647d.png)

The previous picture shows a digram of the components involved in the algorithm. The bubble in the middle shows the data structure called `Sort` (and defined in `sort.h`) which allows the communcition between the different processes. There are three type of processes working in the algorithm:
- Main process: Is the father and main process, and has the following functions:
  - Creates the `Sort` structure and stores it into the shared memory.
  - Initializes the structure with the adequate data.
  - Creates the message queue so that the worker processes can communicate.
  - Creates the necessary pipes for the ilustrator and workers communication.
  - Creates the necessary semaphores used to control concurrency.
  - Creates the workers and ilustrator processes.
  - Regulates the execution of the algorithm in each of its levels.
- Worker processes: Are the process which really sort the array. Each of this process:
  - Establishes and alarm each second to communicate with the ilustrator. When the alarm is receives, it will comunicate the ilustrator which job is it working on through its pipe and it will wait for a response from the ilustrator to continue its job.
  - Reads a message containing its job in the message queue.
  - Modifies the status of this job in the `Sort` structure in the shared memory to make it visible that this job is already being carried out.
  - When the job is finished, the status of the job if updated again in the shared memory.
  - When the process receives the signal SIGTERM, the worker will free all of its resources and kill himself as a process.
- Ilustrator process: Shows the status of the algorithm through the screen, showing the current state of the array and a list where for each worker it explains what it is doing at that moment.


### Project Structure
The structure of the project is the following:
- `Proyecto.pdf`: Report with all the results obtained and an explanation of the project.
- `src`: A directory which contains all the source code for implementing the algorithm:
  - `Data`: Directory which contains the files which contain the arrays to check the correct behavour of the algorithm.
  - `Makefile`: A Makefile for the easy execution of the program. Instructions for its use will be given later.
  - `main.c`: The main program which will be executed. It the sort function defined in `sort.c`.
  - `main_optional_part.c`: An improved version of the program which lets several processes work on different levels concurrently.
  - `gnu_plot_generator.c`: Program which test the execution of the program using different number of workers saving the results into a file in order to later plot the results.
  - `sort.c` and `sort.h`: The header file contains the definition of the data structures used by the algorithm for interprocess comunication, as well as a detailed explanation of the utility of each function in `sort.c`. `sort.c` is the main file of the projects, where the algorithm to sort the array is developed and all the operating systems resources are used to coordinate all the processes involved.
  - `utils.c` and `utils.h`: General use functions are defined in these modules.
  - `global.h`: Status and Bool types are defined here.



## 2. Technologies Used

[//]: # "What technologies were used?" 
The main technologies used were all the low level Operating System mechanisms learned throghout the subject of Operating Systems. These include:
- Semaphores: to control concurrency.
- Shared memory: to share information between different processes.
- Pipes: to communicate between different processes.
- Signals: another way to communicate with processes where no pipe creations is needed. 
- Message queue: For asynchronous communication between processes. 

[//]: # "Why you used the technologies you used?" 


[//]: # "Some of the challenges you faced and features you hope to implement in the future." 





## 3. Learning Outcomes

[//]: # "What did you learn?" 
The main learning outcomes were:
- To deppen my knowledge on how sorting algorithms work.
- How to parallelize algorithms to allow concurrency and scalability.
- Learn how to code multiprocess programs using C.
- Learning how to use all the resources the Operating System provides in order to control concurrency.
- Automation of results.
- Writing technical report skills, which include analysis of the data obtained.
- How parallelization can help improve performance in sorting algorithms.


## 4. How to Install and Run
In order to run the algorith, you can make use of the `Makefile` provided. In order use it, open a terminal inside the `src` directory and type
```
make all
```
This will compile all the needed files. In order to clean all the created files you can use the command
```
make clean
```
In order to run a normal execution you can use one of the following commands, depending on the size of the array you want to sort:
```
make run_small
make run_medium
make run_large
```

Similarly, to run the improved version of the algorithm you can use one of the following commands, depending on the size of the array you want to sort:
```
make runi_small
make runi_medium
make runi_large
```

Finally, if you want a file which contains the result of several execution, you can use the following commands, again depending on the size of the array you want to sort:
```
make gnu_small
make gnu_medium
make gnu_large
```
You can personalise the parameters inside the `Makefile` in order to change the number of workers used, number of levels in the algorithm and the delay. By default the results are saved in the file `results.txt`.


[//]: # "## 5. Extra Information"



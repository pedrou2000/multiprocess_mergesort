# Multithreaded Mergesort 

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


[//]: # "Why you used the technologies you used?" 


[//]: # "Some of the challenges you faced and features you hope to implement in the future." 





## 3. Learning outcomes

[//]: # "What did you learn?" 



## 4. How to Install and Run



## 5. Extra Information



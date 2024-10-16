#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "schedulers.h"
#include "task.h"
#include "list.h"
#include "cpu.h"
#include <stdbool.h>

bool FirstPrint = false;

//Create your nodes here

/**
 * Adds tasks struct to the linked list of tasks
 * @param name The name of the task
 * @param priority The priority of the task
 * @param burst The CPU burst time of the task
 */
void add(char *name, int priority, int burst){
    if (FirstPrint == false)
    {
        FirstPrint = true;
        printf("-------------------------------------------------------------\n");
        printf("Algorithm Description\n");
        printf("-------------------------------------------------------------\n\n");
    }
    
    //dynamically allocate task and then call insert function 
    
}

/**
 * Invokes the scheduler to run the task
 * Implement FCFS scheduling algorithm here
 */
void schedule(){
    
    
}

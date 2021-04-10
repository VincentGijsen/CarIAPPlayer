/*
 * tasks.h
 *
 *  Created on: 10 Apr 2021
 *      Author: vtjgi
 */

#ifndef INC_TASKS_H_
#define INC_TASKS_H_

#include <stdint.h>

typedef enum{
	undefTask=0,
	TaskInitAuthentication,

}TaskType;

typedef void (*funct)();

typedef struct{
	funct f;
	uint32_t scheduledAfter;

}typeDefTask;


typedef struct{
	typeDefTask task;
	uint8_t active;

}typeDefTaskListItem;


/*
 * Prototype
 *
 */
taskRunner(typeDefTask task);
#endif /* INC_TASKS_H_ */

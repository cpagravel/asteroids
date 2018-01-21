#ifndef _Tasks_H
#define _Tasks_H

extern OS_TID t_stage;
extern OS_SEM sem_reset;

extern OS_MUT mut_levelStart;

extern OS_MUT mut_GLCD; 												/* Mutex to control GLCD access     */

extern __task void led (void);
extern __task void keyread(void);
extern __task void adc (void);
extern __task void joystick (void); 

extern void initializeTasks(void);

#endif
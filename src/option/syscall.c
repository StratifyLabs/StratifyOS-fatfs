/*------------------------------------------------------------------------*/
/* Sample code of OS dependent controls for FatFs                         */
/* (C)ChaN, 2012                                                          */
/*------------------------------------------------------------------------*/


#include <pthread.h>
#include <stdlib.h>		/* ANSI memory controls */
#include <malloc.h>		/* ANSI memory controls */

#include "ff.h"


static pthread_mutex_t fatfs_lock_mutex[_VOLUMES];

extern void cortexm_svcall(void (*function)(void*), void * args);
extern void scheduler_root_set_delaymutex(void * args);
extern int pthread_mutex_force_unlock(pthread_mutex_t *mutex);

#if _FS_REENTRANT


int ff_force_unlock(int volume){
	return pthread_mutex_force_unlock(&fatfs_lock_mutex[volume]);
}

/*------------------------------------------------------------------------*/
/* Create a Synchronization Object */
/*------------------------------------------------------------------------*/
/* This function is called by f_mount() function to create a new
/  synchronization object, such as semaphore and mutex. When a 0 is
/  returned, the f_mount() function fails with FR_INT_ERR.
 */

int ff_cre_syncobj (	/* 1:Function succeeded, 0:Could not create due to any error */
		BYTE vol,			/* Corresponding logical drive being processed */
		_SYNC_t* sobj		/* Pointer to return the created sync object */
)
{
	int ret;
	pthread_mutexattr_t mutexattr;

	if( pthread_mutexattr_init(&mutexattr) < 0 ){
		return -1;
	}

	pthread_mutexattr_setpshared(&mutexattr, 1);
	pthread_mutexattr_setprioceiling(&mutexattr, 19);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);

	if( pthread_mutex_init(&fatfs_lock_mutex[vol], &mutexattr) < 0 ){
		return 0;
	}

	*sobj = &fatfs_lock_mutex[vol];

	return 1;

	//*sobj = CreateMutex(NULL, FALSE, NULL);		/* Win32 */
	//ret = (int)(*sobj != INVALID_HANDLE_VALUE);

	//	*sobj = SyncObjects[vol];		/* uITRON (give a static created semaphore) */
	//	ret = 1;

	//	*sobj = OSMutexCreate(0, &err);	/* uC/OS-II */
	//	ret = (int)(err == OS_NO_ERR);

	//  *sobj = xSemaphoreCreateMutex();	/* FreeRTOS */
	//	ret = (int)(*sobj != NULL);

	return ret;
}



/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj() function. When a 0 is
/  returned, the f_mount() function fails with FR_INT_ERR.
 */

int ff_del_syncobj (	/* 1:Function succeeded, 0:Could not delete due to any error */
		_SYNC_t sobj		/* Sync object tied to the logical drive to be deleted */
)
{
	if( pthread_mutex_destroy(sobj) < 0 ){
		return 0;
	}

	return 1;
}



/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a FALSE is returned, the file function fails with FR_TIMEOUT.
 */

int ff_req_grant (	/* TRUE:Got a grant to access the volume, FALSE:Could not get a grant */
		_SYNC_t sobj	/* Sync object to wait */
)
{
	int ret;
	struct timespec abs_time;

	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += 5;

	ret = 1;
	if( pthread_mutex_timedlock(sobj, &abs_time) < 0 ){
		ret = 0;
	} else {
		cortexm_svcall(scheduler_root_set_delaymutex, sobj);
	}

	return ret;
}



/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
 */

void ff_rel_grant (
		_SYNC_t sobj	/* Sync object to be signaled */
)
{

	cortexm_svcall(scheduler_root_set_delaymutex, 0);
	pthread_mutex_unlock(sobj);
}

#endif




#if _USE_LFN == 3	/* LFN with a working buffer on the heap */
/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/
/* If a NULL is returned, the file function fails with FR_NOT_ENOUGH_CORE.
 */

void* ff_memalloc (	/* Returns pointer to the allocated memory block */
		UINT msize		/* Number of bytes to allocate */
)
{
	return malloc(msize);
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
		void* mblock	/* Pointer to the memory block to free */
)
{
	free(mblock);
}

#endif

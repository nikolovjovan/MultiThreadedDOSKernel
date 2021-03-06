/*
 * KSemap.cpp
 * 
 * Created on: May 24, 2018
 *     Author: Jovan Nikolov 2016/0040
 */

#include <mem.h>
#include <stdlib.h>

#include "Macro.h"
#include "KSemap.h"
#include "KThread.h"
#include "System.h"

unsigned KernelSem::capacity = InitialObjectCapacity, KernelSem::count = 0;
KernelSem **KernelSem::objects = 0;

KernelSem::KernelSem (int init)
{
    initialize(0, init);
}

KernelSem::KernelSem (Semaphore *userSem, int init)
{
    initialize(userSem, init);
}

KernelSem::~KernelSem ()
{
    delete mBlocked; // Implicitly unblocks all the threads that were inside!
}

int KernelSem::wait (int toBlock)
{
    #ifndef BCC_BLOCK_IGNORE
    System::lock();
    #endif
    int result = 0;
    if (!toBlock && mValue <= 0) result = -1;
    else if (--mValue < 0)
    {
        block();
        result = 1;
    }
    #ifndef BCC_BLOCK_IGNORE
    System::unlock();
    #endif
    return result;
}

void KernelSem::signal ()
{
    #ifndef BCC_BLOCK_IGNORE
    System::lock();
    #endif
    if (mValue++ < 0) deblock();
    #ifndef BCC_BLOCK_IGNORE
    System::unlock();
    #endif
}

int KernelSem::val () const
{
    return mValue;
}

KernelSem* KernelSem::at (unsigned index)
{
    if (index < count) return objects[index];
    else return 0;
}

void KernelSem::block ()
{
    #ifndef BCC_BLOCK_IGNORE
    asmLock();
    #endif
    System::running->mState = ThreadState::Blocked;
    mBlocked->put((PCB*) System::running);
    System::dispatch();
    #ifndef BCC_BLOCK_IGNORE
    asmUnlock();
    #endif
}

void KernelSem::deblock ()
{
    #ifndef BCC_BLOCK_IGNORE
    asmLock();
    #endif
    PCB *thread = mBlocked->get();
    if (thread)
    {
        thread->mState = ThreadState::Running;
        System::threadPut(thread);
    }
    #ifndef BCC_BLOCK_IGNORE
    asmUnlock();
    #endif
}

void KernelSem::initialize (Semaphore *userSem, int init)
{
    mValue = init;
    mSemaphore = userSem;
    mBlocked = new PCBQueue();
    mID = count++;
    if (count > capacity)
    {
        #ifndef BCC_BLOCK_IGNORE
        asmLock();
        #endif
        KernelSem **newObjects = (KernelSem**) calloc(capacity << 1, sizeof(KernelSem*));
        if (objects)
        {
            memcpy(newObjects, objects, capacity * sizeof(KernelSem*));
            free(objects);
        }
        if (newObjects)
        {
            objects = newObjects;
            capacity <<= 1;
        }
        #ifndef BCC_BLOCK_IGNORE
        asmUnlock();
        #endif
    }
    if (objects)  objects[mID] = this;
}
/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include<sched.h>
#include "math.h"


unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);

void next_epoch()   /* to initialize next epoch for linux based scheduler  */
{
	int i=0;
	struct pentry *p;
	while (i < NPROC) 
	{
		p = &proctab[i];
		if (p->pstate != PRFREE)
		{
			if (p->counter != 0 && p->counter != p->timeQuantum) /*if no unused previous epoch quantum or process did not run at all*/ 
			{
        p->timeQuantum = p->pprio + (p->counter) / 2;
				
			} 
			else 
			{
				p->timeQuantum = p->pprio;
			}
			p->counter = p->timeQuantum;
			p->goodness =  p->pprio + p->counter;
		}
   i++;
   
	}	
}


/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */

int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
  
  
	
  
  if(sched==1)
  {
    // Exponential distribution scheduler 
    int priExp = (int) expdev(0.1);
    int newpid=NULLPROC;
    int nextP = q[rdyhead].qnext;    
    int nextK=q[nextP].qkey;
    int oldP=nextP;
    
    optr = &proctab[currpid];
    if (optr->pstate == PRCURR) 
        {
		      optr->pstate = PRREADY;
		      insert(currpid,rdyhead,optr->pprio);
	      }
    
    //Loop through till nextP points to
   /*for(;priExp < nextK;)
      {
        oldP=nextP;
        nextP = q[nextP].qprev;
        nextK=q[nextP].qkey;        
      }*/
      
      do
      {
      
      nextP=q[nextP].qnext;
    nextK=q[nextP].qkey;
      if(nextK>priExp)
      {break;}
    //  oldP=nextP;
    
    
      }while(nextP!=q[rdytail].qprev );
       
     ////Dequeue only if less than 50. Otherwise NULLPROC/////
     
      if(nextP<NPROC)
          {
            //nextP=q[nextP].qnext;
            //nextP=oldP;
            newpid=dequeue(nextP);
          }
          
     ////Context switch code- Taken from regular scheduler/////     
        if(currpid!=newpid)
        {//If the decided task is not the current task then context switch
          nptr = &proctab[(currpid = newpid)];     
	        nptr->pstate = PRCURR;
          		
	        #ifdef	RTCLOCK
		        preempt = QUANTUM;		/* reset preemption counter 	*/
	        #endif
          
	        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

	        /* The OLD process returns here when resumed. */                  
          
        
      }
      else
      {
      
          #ifdef	RTCLOCK
		        preempt = QUANTUM;		/* reset preemption counter 	*/
	        #endif
          optr->pstate = PRCURR;
          
      }
      
      return(OK);
    }  
    
    
   
    
    
    
    
    else if(sched==0)
    {
      if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
        (lastkey(rdytail)<optr->pprio)) {
        return(OK);
        }
	
    	/* force context switch */

	    if (optr->pstate == PRCURR) {
		      optr->pstate = PRREADY;
		      insert(currpid,rdyhead,optr->pprio);
	      }

	    /* remove highest priority process at end of ready list */

	    nptr = &proctab[ (currpid = getlast(rdytail)) ];
	    nptr->pstate = PRCURR;		/* mark it currently running	*/
      #ifdef	RTCLOCK
	    preempt = QUANTUM;		/* reset preemption counter	*/
      #endif
	
	    ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
    	/* The OLD process returns here when resumed. */
  
		return OK;
   }
   
   
    //Linux scheduler
    
   else if(sched==2)
   {
   	
   int maxG = 0;
   int nextP = q[rdyhead].qnext;
   int oldP = nextP;
   int newpid=NULLPROC;
   int currG=proctab[currpid].goodness;
   int currC=proctab[currpid].counter;
    optr = &proctab[currpid];
	 int flag=0;
		currG += preempt - currC;   /* dynamic priority*/
		currC = preempt;
		
		if (preempt <= 0) // if process used up its time quantum 
		{
       optr->goodness=0;
       optr->counter=0;
			//currC = 0;
			//currG = 0;
		}
   
   
    ////To find max goodness////
		do 
		{
			if (proctab[nextP].goodness > maxG) 
			{
			  oldP = nextP;
				maxG = proctab[nextP].goodness;
			}
			nextP = q[nextP].qnext;
		}while (nextP != q[rdytail].qnext );
   //////////////////////////////
   
   /*if (optr->pstate == PRCURR) //next state is PRCURR so process wants to continue but counter = 0 
		{
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
		}*/
   /*if(nextP<NPROC)
   {
   newpid=dequeue(oldP);
   }*/
   
		if(optr->pstate != PRCURR || currC == 0)
   {
   flag=1;
   }
   
   
   if(maxG==0)
		{
   
    if(flag==1)
		{
			next_epoch();  
			preempt = currC;
			
		if (currC==0) // next state is PRCURR so process wants to continue but counter = 0 
		{
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
		}
		
 	  nptr = &proctab[ (currpid = dequeue(NULLPROC)) ];
    nptr->pstate = PRCURR;
					
     #ifdef RTCLOCK
     preempt = QUANTUM;
     #endif

    ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
    return OK;			
    	
    			
		}
    }	
		else //if(maxG>0)
   {
		/* no preemption of current process as its goodness value is highest */
		if (flag==0 && currG > maxG)
		{
			preempt = currC;
			return(OK);
		}

		else if (flag==1 || currG < maxG)
		{
     //newpid=dequeue(oldP);
			if (optr->pstate == PRCURR)
      {
     	optr->pstate = PRREADY;
      insert(currpid,rdyhead,optr->pprio);
      }

			nptr = &proctab[ (currpid = dequeue(oldP)) ];
			nptr->pstate = PRCURR;

                        
      preempt = nptr->counter;

      ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
      return OK;	 	
		}
   }
   //next_epoch();
   /* next epoch as there is no runnable process having goodness value > 0 and next state of current process is not PRCURR */
     
       
		return OK; 
   }
}

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <ctype.h>
#include <vector>
#include <string.h>
#include <queue>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sentd
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Statistics 
 * Do NOT change the name/declaration of these variables
 * You need to set the value of these variables appropriately within your code.
 * */
int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;

/* Globals 
 * Do NOT change the name/declaration of these variables
 * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 * */
float TIMEOUT = 15.0;
int WINSIZE;         //This is supplied as cmd-line parameter; You will need to read this value but do NOT modify it; 
int SND_BUFSIZE = 1000; //Sender's Buffer size
int RCV_BUFSIZE = 1000; //Receiver's Buffer size
float time_local = 0;

void tolayer3(int AorB,struct pkt packet);
void stoptimer(int AorB);
void starttimer(int AorB,float increment);
void tolayer5(int AorB,char *datasent);

struct sr_pkt {
	pkt packet;
	bool ackd;
	float time_sent;
};

/* send variables */
int send_base;
int nextseqnum;
int longest_outstanding_packet;
std::vector<sr_pkt> A_buffer;


/* receiver variables*/
int rcv_base;
std::vector<sr_pkt> B_buffer;

/* CHECKSUM FUNCTIONS */
void setChecksum(pkt* packet)
{
	int checksum = 0;
	checksum += packet->acknum;
	checksum += packet->seqnum;
	for (int i = 0; i < 20; i ++)
	{
		checksum += packet->payload[i];
	}
	packet->checksum = checksum;
	//printf("checksum is %u \n", checksum);
	//printf("checksum in packet is %u \n",packet->checksum);
}

int getChecksum(pkt* packet)
{
	int checksum = 0;
	checksum += packet->acknum;
	checksum += packet->seqnum;
	for (int i = 0; i < 20; i ++)
	{
		checksum += packet->payload[i];
	}
	//printf("checksum is %u \n", checksum);
	return checksum;
}
int checkChecksum(pkt* packet)
{
	//printf("packet checksum: %u, calculated checksum %u \n",packet->checksum,getChecksum(packet));
	if (packet->checksum != getChecksum(packet))
	{
		//printf("checksum does not match up populated: %u, calculated: %u \n",packet->checksum,getChecksum(packet));
		return -1;
	}
	return 0;
}
/* END OF CHECKSUM FUNCTIONS*/

/* determines longest outstanding packet and sets the global variable appropriately */
void determineLongestOutstandingPacket()
{
	int firstSentPacket = send_base;
	float firstSentPacketTime = 99999999999.9;

	for (int i = send_base; i < WINSIZE + send_base; i ++)
	{
		if (A_buffer[i].ackd == false
				&& A_buffer[i].time_sent != 0
				&& A_buffer[i].time_sent < firstSentPacketTime)
		{
			firstSentPacketTime = A_buffer[i].time_sent;
			firstSentPacket = i;
		}
	}

	/* set the longest outstand packet appropriately and the time left until timeout for the timer*/
	longest_outstanding_packet = firstSentPacket;
	//printf("longest outstanding packet set to %u \n",longest_outstanding_packet);
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) //ram's comment - students can change the return type of the function from struct to pointers if necessary
{
	/* Increment the number of messages received from A's application */
	A_application ++;

	/* create, initialize, and push packet into buffer */
	sr_pkt sr_packet;
	sr_packet.ackd = false;
	sr_packet.time_sent = 0.0;
	sr_packet.packet.acknum = 0;
	sr_packet.packet.checksum = 0;
	sr_packet.packet.seqnum = 0;
	memset(&sr_packet.packet.payload,0,20);
	strcpy((char *)&sr_packet.packet.payload,(char *)&message.data);
	A_buffer.push_back(sr_packet);

	/* send new message if condition holds */
	if (nextseqnum < WINSIZE + send_base)
	{
		/* populate packet */
		A_buffer[nextseqnum].packet.seqnum = nextseqnum;
		setChecksum(&A_buffer[nextseqnum].packet);

		/* send packet to layer below */
		tolayer3(0,A_buffer[nextseqnum].packet);
		//printf("output at A! sequence number %u \n",nextseqnum);
		A_transport++;

		/* set the time sent of the packet */
		A_buffer[nextseqnum].time_sent = time_local;

		/* if the packet has a seqnum of the base then we know it should start the first timer */
		if (send_base == nextseqnum)
		{
			longest_outstanding_packet = send_base;
			starttimer(0,TIMEOUT);
		}

		/* increment the seqnum*/
		nextseqnum = nextseqnum + 1;
	}

}

void B_output(struct msg message)  /* need be completed only for extra credit */
// ram's comment - students can change the return type of this function from struct to pointers if needed  
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	//printf("input at A! \n");
	if (checkChecksum(&packet))
	{
		//printf("checksum failed, packet is corrupt at A \n");
	}
	else
	{
		/* mark the packet ackd with the ack just received*/
		printf("just ACKD seq %u \n",packet.acknum);
		A_buffer[packet.acknum].ackd = true;

		/* move the base forward until we reach lowest unackd packet */
		while (A_buffer[send_base].ackd == true)
		{
			printf("incrementing base... \n");
			send_base++;
		}
		/* send more ready packets*/

		if (send_base == nextseqnum)
		{
			stoptimer(0);
		}
		else
		{
			if (longest_outstanding_packet == packet.acknum)
			{
				stoptimer(0);
				determineLongestOutstandingPacket();
				starttimer(0,A_buffer[longest_outstanding_packet].time_sent + TIMEOUT - time_local);
				printf("timeout value is : %f \n", A_buffer[longest_outstanding_packet].time_sent + TIMEOUT - time_local);
			}
		}
	}

}

/* called when A's timer goes off */
void A_timerinterrupt() //ram's comment - changed the return type to void.
{
	//printf("Timeout! resending sequence %u \n", longest_outstanding_packet);
	/* re send the packet that the timer was corresponding too */
	tolayer3(0,A_buffer[longest_outstanding_packet].packet);
	//printf("resent packet has the following sequence number %u \n", A_buffer[longest_outstanding_packet].packet.seqnum);
	/* set the time sent to now on the re-sent packet*/
	A_buffer[longest_outstanding_packet].time_sent = time_local;
	A_transport++;

	/* determine which packet the timer now belongs to.*/
	/* 1. the packet must be in the window */
	/* 2. the packet will have the smallest time sent */

	determineLongestOutstandingPacket();
	float timeLeft = A_buffer[longest_outstanding_packet].time_sent + TIMEOUT - time_local;
	starttimer(0,timeLeft);

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{
	send_base = 0;
	nextseqnum = 0;
	longest_outstanding_packet = 0;
	A_buffer.resize(SND_BUFSIZE);
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	//printf("input at B! with sequence %u \n",packet.seqnum);
	B_transport++;

	if (checkChecksum(&packet))
	{
		printf("checksum failed, packet is corrupt at A \n");
	}

	/* if packet is rcv_base then send to layer 5 (application)*/
	else
	{
		/* we only ACK this one packet to A even though we may send many to the application */
		pkt sendpacket;
		memset(&sendpacket.payload,0,20);
		sendpacket.acknum = packet.seqnum;
		sendpacket.seqnum = packet.seqnum;
		setChecksum(&sendpacket);
		//printf("sending ACK to A seq %u, \n",sendpacket.ack)
		tolayer3(1,sendpacket);

		if (packet.seqnum == rcv_base)
		{
			B_buffer[rcv_base].packet = packet;
			B_buffer[rcv_base].ackd = true;

			/* while the packet at the base has already been received we keep
			 * incrementing the base and sending payloads to the application at B*/
			while (B_buffer[rcv_base].ackd == true)
			{
				printf("Sending to application at B from sequence %u \n",rcv_base);
				tolayer5(1,B_buffer[rcv_base].packet.payload);
				B_application++;
				rcv_base++;
			}
		}

		/* otherwise, buffer in the data structure and send appropriate ACK */
		else
		{
			B_buffer[packet.seqnum].ackd = true;
			B_buffer[packet.seqnum].packet = packet;
		}
	}
}

/* called when B's timer goes off */
void B_timerinterrupt() //ram's comment - changed the return type to void.
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //ram's comment - changed the return type to void.
{
	rcv_base = 0;
	B_buffer.resize(RCV_BUFSIZE);
}

int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  


/*****************************************************************
***************** NETWORK EMULATION CODE IS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/



/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1


struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */


void insertevent(struct event *p)
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time_local);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}





/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
//    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */

   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time_local + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
}





void init(int seed)                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(seed);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time_local=0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}






//int TRACE = 1;             /* for my debugging */
//int nsim = 0;              /* number of messages from 5 to 4 so far */ 
//int nsimmax = 0;           /* number of msgs to generate, then stop */
//float time = 0.000;
//float lossprob;            /* probability that a packet is dropped  */
//float corruptprob;         /* probability that one bit is packet is flipped */
//float lambda;              /* arrival rate of messages from layer 5 */   
//int   ntolayer3;           /* number sent into layer 3 */
//int   nlost;               /* number lost in media */
//int ncorrupt;              /* number corrupted by media*/

/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 */
int isNumber(char *input)
{
    while (*input){
        if (!isdigit(*input))
            return 0;
        else
            input += 1;
    }

    return 1;
}

int main(int argc, char **argv)
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   int opt;
   int seed;

   //Check for number of arguments
   if(argc != 5){
    fprintf(stderr, "Missing arguments\n");
  printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
    return -1;
   }

   /* 
    * Parse the arguments 
    * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html 
    */
   while((opt = getopt(argc, argv,"s:w:")) != -1){
       switch (opt){
           case 's':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -s\n");
                           return -1;
                       }
                       seed = atoi(optarg);
                       break;

           case 'w':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -w\n");
                           return -1;
                       }
                       WINSIZE = atoi(optarg);
                       break;

           case '?':   break;

           default:    printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
                       return -1;
       }
   }
  
   init(seed);
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time_local = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   //Do NOT change any of the following printfs
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time_local,nsim);

   printf("\n");
   printf("Protocol: ABT\n");
   printf("[PA2]%d packets sent from the Application Layer of Sender A[/PA2]\n", A_application);
   printf("[PA2]%d packets sent from the Transport Layer of Sender A[/PA2]\n", A_transport);
   printf("[PA2]%d packets received at the Transport layer of Receiver B[/PA2]\n", B_transport);
   printf("[PA2]%d packets received at the Application layer of Receiver B[/PA2]\n", B_application);
   printf("[PA2]Total time: %f time units[/PA2]\n", time_local);
   printf("[PA2]Throughput: %f packets/time units[/PA2]\n", B_application/time_local);
   return 0;
}


/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
/*void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  // x is uniform on [0,2*lambda] 
                             // having mean of lambda       
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} */




void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
 //AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time_local);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB,float increment)
// AorB;  /* A or B is trying to stop timer */

{

 struct event *q;
 struct event *evptr;
 ////char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time_local);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time_local + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(int AorB,struct pkt packet)
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 ////char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time_local;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(int AorB,char *datasent)
{
  
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}

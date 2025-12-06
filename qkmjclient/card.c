#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "curses.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>

#include "mjdef.h"
#include "qkmj.h"

int card_num=144;
char all_card[150]={1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,
                    7,7,7,7,8,8,8,8,9,9,9,9,11,11,11,11,12,12,12,12,
                    13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,
                    17,17,17,17,18,18,18,18,19,19,19,19,21,21,21,21,
                    22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,
                    26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,
                    31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,
                    41,41,41,41,42,42,42,42,43,43,43,43,51,52,53,54,
                    55,56,57,58};

int generate_random(int range);

void generate_card()
{
  long seed;
  int i,j,index,rand_int;

  seed=time(NULL);
  srand48(seed);
  for(i=0;i<150;i++)
    mj[i]=0;
  for(i=0;i<card_num;i++)
  {
    index=0;
    rand_int=generate_random(card_num-i);
    for(j=0;j<card_num;j++)
    {
      if(!mj[j])   /* The room is empty */
      {
        if(index==rand_int)
          break;
        index++;
      }
    }
    mj[j]=all_card[i];
  }
}

int generate_random(int range)
{
  double rand_num;
  int rand_int;

  rand_num=drand48();
  rand_int=(int) (range*rand_num);
  return(rand_int);
}
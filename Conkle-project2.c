/*
   File:
   Author: Andrija (Andy) Conkle
   Course: EECS 3540 Operating Systems, Spring 2022
   Date: 3/28/22
   This file contains the main function for for Lab #2 as well as all the functions necessary to complete the lab.
*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

struct thread_parameters
{
   int lo; 
   int hi;   
};


int *arr, SIZE = 0, THRESHOLD = 10, PIECES = 10, MAXTHREADS = 4;  
bool alternate = false, medianB = false, multiBool = true; 
 
int gettimeofday();
int pthread_tryjoin_np(); 
void *hybridQs(void *arrData);
void isSorted(int SIZE); 
void printArray(int SIZE);
void printPieces(int pieces, struct thread_parameters pieceA[]);

int main(int argc, char *argv[])
{
   int seed, k, A, B, C, median; // SEED, 
   struct thread_parameters par;
   struct thread_parameters* piecesArr; // array of structs to hold each "piece" 
   pthread_t tid[MAXTHREADS];
   struct timeval start, end;  

   for (int i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "-n")) // SIZE
      {
         i++;
         SIZE = atoi(argv[i]); // Convert the char to an int
         if (SIZE < 0)
         {
            printf("Enter a positive integer for the array size.\n");
            exit(1);
         } 
      }
      else if (!strcmp(argv[i], "-s")) // THRESHOLD -- Default value for THRESHOLD is 10 unless the user specifies other wise.
      {
         i++;
         THRESHOLD = atoi(argv[i]); // convert char to int
      }
      else if (!strcmp(argv[i], "-a")) // ALTERNATE -- specifies whether Insertion sort will be used over Shell Sort. Default is Shell Sort
      {
         i++; // increment to get the value associated with the command.
         if (!strcmp(argv[i], "I") || !strcmp(argv[i], "i")) alternate = true;
         else if (!strcmp(argv[i], "S") || !strcmp(argv[i], "s")) alternate = false;
         else exit(1);
      }
      else if (!strcmp(argv[i], "-r")) // SEED -- Determines whether or not we will SEED the randomness of the array scramble. 
      {
         i++;                  // increment to get the value associated with the command.
         seed = atoi(argv[i]); // convert
         if (seed == -1) srand(clock());
         else srand(seed);
      }
      else if (!strcmp(argv[i], "-m")) // MULTITHREAD -- Determines if the program will run with multiple threads or not. Default value is YES
      {
         i++; // increment to get the value associated with the command.
         if (!strcmp(argv[i], "N") || !strcmp(argv[i], "n")) multiBool = false;
      }
      else if (!strcmp(argv[i], "-p")) // PIECES -- Determines how many partitions to divide the original list into.
      {
         i++; // increment to get the value associated with the command.
         if(multiBool == false) { PIECES = 10; }
         else { PIECES = atoi(argv[i]); }
         if(PIECES < MAXTHREADS) { MAXTHREADS = PIECES; }
      }
      else if (!strcmp(argv[i], "-t")) // MAXTHREADS -- This specifies the number of threads that will be run at once. Only applies if MULTITHREAD is yes. Default value is 4. 
      {
         i++; // increment to get the value associated with the command.
         if(multiBool == false) { MAXTHREADS = 1;}
         else { MAXTHREADS = atoi(argv[i]); }
         if (PIECES < MAXTHREADS) { MAXTHREADS = PIECES; } // MAXTHREADS CANNOT BE GREATER THAN PIECES
      }
      else if (!strcmp(argv[i], "-m3")) // MEDIAN OF THREE PARTION
      {
         i++; // increment to get the value associated with the command.
         if (!strcmp(argv[i], "Y") || !strcmp(argv[i], "y")) medianB = true;
      }
      else // Not a recognized command. Output an error message and exit. 
      {
         printf("Unrecognized Command: %s\n", argv[i]);
         exit(1);
      }
   }

   par.lo = 0; 
   par.hi = SIZE - 1;
   double begin = clock();                   // start cpu timer for building array
   arr = (int*) malloc(sizeof(int) * SIZE);  // create the array
   printf("Array created in: %5.3f seconds\n", (clock() - begin) / 1000000.0);

   for (int i = 0; i < SIZE; i++) { arr[i] = i; } // initialize array to be from 0 to n - 1 values
   printf("Array initalized in: %5.3f seconds\n", (clock() - begin) / 1000000.0);

   // Scramble the array
   for (int i = 0; i < SIZE; i++)
   {
      int j = i + rand() / (RAND_MAX / (SIZE - i) + 1);
      int temp = arr[j];
      arr[j] = arr[i];
      arr[i] = temp;
   } 
   printf("Array scarmbled in: %5.3f seconds\n", (clock() - begin) / 1000000.0);
   //printArray(SIZE);

   // PIECES: Taake the original scrambled list and split it into a specific number of segments (either 10 or usr defined number)
   piecesArr = malloc(PIECES * sizeof(struct thread_parameters)); // array to hold the indexes of the different pieces.
   piecesArr[0].lo = par.lo;  // put the entire scrambled array into the first index of the array of structs. 
   piecesArr[0].hi = par.hi;  // keep the biggest segment in piecesArr[0]
   int i = par.lo; 
   int j = par.hi; 
   int pivot = arr[par.lo]; 
 
   for(int p = 1; p < PIECES; p++)
   {
      if(medianB == true) 
      {
         k = (piecesArr[0].hi + piecesArr[0].lo) / 2; // middle 
         A = arr[i];   // value at lo index
         B = arr[k];       // value at "middle" index
         C = arr[j];   // value at the hi index
         if((A >= B && A <= C) || (A >= C && A <= B)) // p->lo is the median
            median = i; 
         else if((B >= A && B <= C) || (B >= C && B <= A)) // k is the median 
            median = k; 
         else // p->hi is the median 
            median = j;
         // Swap the values and use p->lo as the pivot like normal. 
         int temp = arr[i];
         arr[i] = arr[median]; 
         arr[median] = temp; 
         pivot = arr[i];  
      }

      // Partition: 
      j = j + 1;    
      do
      { 
         do i++; while(arr[i] < pivot);  
         do j--; while(arr[j] > pivot);  
         if(i < j)
         {
            //swap arr[i] and arr[j]
            int temp = arr[i]; 
            arr[i] = arr[j]; 
            arr[j] = temp; 
            //printf("Swapping(PIECES) %i & %i\n", arr[i], arr[j]); 	
         }
         else { break; }
      }while(true);

      // Swap the pivot and arr[j]  
      arr[par.lo] = arr[j]; 
      arr[j] = pivot;

      struct thread_parameters l, r; 
      l.lo = par.lo;
      l.hi = j - 1; 
      r.lo = j; 
      r.hi = par.hi; 
      int rightSize = r.hi - r.lo + 1; 
      int leftSize = l.hi - l.lo + 1; 

      if(rightSize > leftSize)
      {
         piecesArr[0].lo = j + 1;       // set LO value for biggest piece 
         piecesArr[0].hi = par.hi;      // set HI value for biggest piece
         pivot = arr[j + 1];            // pivot is leftmost value of biggest piece
         piecesArr[p].hi = j - 1;       // Set HI value of small piece
         piecesArr[p].lo = par.lo;      // Set LO value of small piece
         i = piecesArr[0].lo; 
         j = piecesArr[0].hi; 
         //printPieces(p, piecesArr);
      }
      else
      { 
         piecesArr[0].lo = par.lo;       // set LO value for biggest piece
         piecesArr[0].hi = j - 1;        // set HI value for biggest piece
         pivot = arr[piecesArr[0].lo];   // pivot is leftmost value of biggest piece
         piecesArr[p].lo = j + 1;        // save the start and end indices of the small piece to be sorted later ----- j + 1
         piecesArr[p].hi = par.hi;
         i = piecesArr[0].lo;            // starting index of big piece
         j = piecesArr[0].hi;            // ending index of big piece
         //printPieces(p, piecesArr);
      }
      par.hi = j; // set the new rightmost index of the biggest piece
      par.lo = i; // set the left most index of the biggest piece
   }
   // Rearrange the pieces to be in descending order from longest to shortest 
   for(int i = 0; i < PIECES; i++)
   {
      int bigSize = piecesArr[0].hi - piecesArr[0].lo;
      int smallSize = piecesArr[i].hi - piecesArr[i].lo; 
      if(smallSize > bigSize)
      {
         int tempHI = piecesArr[0].hi; 
         int tempLO = piecesArr[0].lo; 
         piecesArr[0].hi = piecesArr[i].hi; 
         piecesArr[0].lo = piecesArr[i].lo;
         piecesArr[i].hi = tempHI;
         piecesArr[i].lo = tempLO;
      } 
   }
 
   gettimeofday(&start, NULL);
   // If multiBool is false, then we will only use one thread to sort the entire array of pieces.   
   if(multiBool == false)
   { 
      for(int i = 0; i < PIECES; i++)
      {
         int rc = pthread_create(&tid[i], NULL, hybridQs, &piecesArr[i]); // there will only be 1 thread
         pthread_join(tid[i], NULL); 
      }
   }
   else 
   { 
      /* Create MAXTHREADS number of threads (MAXTHREADS <= PIECES). When creating the threads we are onlu sending MAXTHREADS number of pieces. So, to send the rest of the pieces we are going to poll
         the threads using pthread_tryjoin_np. When this equals 0 then we know that the thread has completed, therefore we will create another thread and send it the piece located at pieceIndex 
         Every time we make another thread and send it to the sorting function, we will increment the pieceIndex so that it can send the proper piece to the next thread until we have sent 
         PIECES - 1 number of PIECES. 

         MAXTHREADS cannot be more than PIECES! The biggest PIECE will be sent to the first thread and each subsequent thread will be sent the next largest PIECE until all the PIECES have
         been sent. 
     */
      int pieceIndex = 0; // holds the index of the next piece to be sent to the thread after MAXTHREADS number of threads have been sent. 
      for(int i = 0; i < MAXTHREADS && i < PIECES; i++)
      {
         int rc = pthread_create(&tid[i], NULL, hybridQs, &piecesArr[i]); // this only sends MAXTHREADS number of pieces.
         if(rc) { perror("Failed to create thread.\n"); }
         pieceIndex++; 
      }
      // Poll the threads sending it piecesArr[pCount]
      while(pieceIndex < PIECES)
      {
         for(int i = 0; i < MAXTHREADS && i < PIECES; i++)
         {
            int tj = pthread_tryjoin_np(tid[i], NULL);
            if(tj == 0 && pieceIndex < PIECES)
            {
               pthread_create(&tid[i], NULL, hybridQs, &piecesArr[pieceIndex]); 
               pieceIndex++; 
            }
            else if(tj != 0 && pieceIndex < PIECES)
            {
               usleep(50000);
               continue; 
            }
         }
      }

      // Join all of the threads. 
      for(int i = 0; i < MAXTHREADS; i++)
      {
         int rj = pthread_join(tid[i], NULL); 
         if(rj) { perror("Failed to join thread.\n"); }
      }
   } 
   gettimeofday(&end, NULL); 
   printf("Seconds spent sorting %i: Wall Clock: %5.3f / CPU: %5.3f\n", SIZE, (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0), (clock() - begin) / 1000000.0);
   printf("Total Runtime: %f\n", (clock() - begin) / 1000000.0);

   isSorted(SIZE);  // check to see if the array is sorted. 
   free(arr);       // clear memory made for the array
   return 0;
}

/* This hybrid quicksort function will quicksort the elements in the array until either the user defined THRESHOLD or default THRESHOLD value (10) has been reached. The chosen pivot value 
   will be the leftmost element of the piece that the thread is given when called. 
   If Median of Three has been specified, then select the pivot using this method and continue to partitioning the list after. 
   Else, partition the list using the leftmost value as the pivot. 
   If the number of elements left is <= the THRESHOLD value, then use the alternate sorting algorithm to sort the list. The default alternate is shell sort. The user can specify that insertion sort be 
   used by typing the command '-a I' || '-a i' 
*/
void *hybridQs(void *arrData)
{ 
	struct thread_parameters *p = arrData; 
   int LO = p->lo;                  // pointer lowest index 
   int HI = p->hi;              // pointer higher index  
   int x = arr[LO];                 // pivot value for quicksort
   int k, A, B, C, median; 
   int segmentSize = (HI - LO + 1); 
 
   if(segmentSize <= 1) 
      return NULL; // if the length of the segment < 2, return since it is only one element long.  

   // If there are only two items, then no reason using a sorting algorithm. Make sure that we need to swap and if not just exit. 
   if(segmentSize == 2) 
   {
      if(arr[LO] < arr[HI]) 
         return NULL; // we are good, so return
   
      // else, swap the values and return
      //printf("Swapping(segmentSize) %i & %i\n", arr[LO], arr[HI]); 
      int temp = arr[LO]; 
      arr[LO] = arr[HI]; 
      arr[HI] = temp; 
      return NULL; 
   }

   // We have at least 3 items. Depending on the THRESHOLD we will either recursively Quicksort the segment, 
   // or use Shell sort / Insertion Sort (default alternate algorithm is shell sort)
	if(segmentSize <= THRESHOLD)
	{
      if(alternate == true) // insertion sort
		{
         for(int i = LO + 1; i <= HI; i++)
         {
            for(int j = i; j > LO; j--)
            {
               if(arr[j - 1] < arr[j]) 
                  break; 
               else 
               { 
                  //printf("Swapping(IS) %i & %i\n", arr[j-1], arr[j]); 
                  int temp = arr[j - 1]; 
                  arr[j - 1] = arr[j]; 
                  arr[j] = temp;
                  //printArray(size);  
               }
            }
         }
         //printArray(size); 
         return NULL; 
		}
      // Else, Shell Sort is the alternate algorithm. 
  		k = 1;
		while(k <= segmentSize) k *= 2; 
      do
      {
         for (int i = LO; i < LO + (segmentSize - k); i++) // for each comb position
         {
            for (int j = i; j >= 0; j -= k) // Tooth-to-tooth is k
            {
               if (arr[j] <= arr[j + k]) break; // move upstream/exit?
               else
               {
                  //printf("Swapping(SS) %i & %i\n", arr[j], arr[j+k]); 
						int temp = arr[j]; 
						arr[j] = arr[j + k]; 
						arr[j + k] = temp; 
                  //printArray(size);                  
               } 
            }
         }
         k = k >> 1; // or k /= 2;
      } while (k > 0);
      return NULL; 
	}
	
   if(LO < HI)
	{
      // User has said to use the median of three method:
      //    Find the "middle" value by getting the hi and low index and dividing by 2. 
      //    Compare the value at the low, middle, and high index, use the "median" of those three as the pivot. 
      if(medianB == true) 
      {

         k = (HI + LO) / 2; // middle 
         A = arr[LO];   // value at lo index
         B = arr[k];       // value at "middle" index
         C = arr[HI];   // value at the hi index
         if((A >= B && A <= C) || (A >= C && A <= B)) // p->lo is the median
            median = LO; 
         else if((B >= A && B <= C) || (B >= C && B <= A)) // k is the median 
            median = k; 
         else // p->hi is the median 
            median = HI;
         // Swap the values and use p->lo as the pivot like normal. 
         int temp = arr[LO];
         arr[LO] = arr[median]; 
         arr[median] = temp; 
         x = arr[LO];  
      }

      // Partition:
      // Put all the numbers greater than the pivot to the right and all numbers less than to the left. 
      // The default pivot elements is p->lo unless using Median Of Three. 

      HI = p->hi + 1;
		do
	   { 
			do LO++; while (arr[LO] < x); 
			do HI--; while (arr[HI] > x); 
			if(LO < HI) // swap arr[LO] & arr[HI]
			{  
				int temp = arr[LO]; 
				arr[LO] = arr[HI]; 
				arr[HI] = temp; 
            //printf("Swapping(QS) %i & %i\n", arr[LO], arr[HI]); 	
            //printArray(size); 			   
         }
			else { break; } 
      } while(true); 

		// swap pivot value and HI (placing the pivot)
      //printf("Swapping(1) %i & %i\n", arr[p->lo], arr[HI]);
		arr[p->lo] = arr[HI]; 
		arr[HI] = x;  
      //printArray(size); 

      struct thread_parameters left, right;
      left.lo = p->lo; // original left
      left.hi = HI - 1; // pivot is in HI
      right.lo = HI + 1; 
      right.hi = p->hi;    // original right
		hybridQs(&left); //  recursively quicksort the left side  
		hybridQs(&right); // recursively quicksort the right side 
      return NULL; 
   }
   pthread_exit(NULL); 
}

void isSorted(int size)
{
   for (int i = 0; i < size - 1; i++)
   {
      if (arr[i] > arr[i + 1])
      {
         printf("Array is not sorted.\n"); 
         return;  
      }
   }
   printf("Array is sorted.\n");
   return; 
}

void printArray(int size)
{
   printf("{ ");
   for (int i = 0; i < size; i++)
   {
      printf("%i ", arr[i]);
   }
   printf("}\n");
}

void printPieces(int pcount, struct thread_parameters piecesA[])
{
   //printf("[piecesArr[%i].lo = %i |", 0, piecesA[0].lo);
   //printf("[piecesArr[%i].hi = %i\n", 0, piecesA[0].hi);

   for(int i = 0 ; i <= pcount; i++)
   {
      printf("piecesArr[%i]: %5i---%1i\n", i, piecesA[i].lo, piecesA[i].hi);
   }
}
/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:FireFly, 1:Dragon*/
	char *fname =  (char *)malloc(20);
	char *str_protocol = (char *)malloc(8);
 	fname = argv[6];
	if(protocol==0)
		str_protocol = "Firefly";
	else if(protocol==1)
		str_protocol = "Dragon";
	
	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	printf("\n===== 506 SMP Simulator Configuration =====");
	printf("\nL1_SIZE:\t\t%d",cache_size);
	printf("\nL1_ASSOC:\t\t%d",cache_assoc);
	printf("\nL1_BLOCKSIZE:\t\t%d",blk_size);
	printf("\nNUMBER OF PROCESSORS:\t%d",num_processors);
	printf("\nCOHERENCE PROTOCOL:\t%s\n",str_protocol);
	printf("\nTRACE FILE :\t%s\n",fname);
	//****************************************************//

 
	//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//	
	Cache *cache_arr[num_processors]; 
	for(int i=0;i<num_processors;i++)
		cache_arr[i] = new Cache(cache_size,cache_assoc,blk_size,i);



	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	int proc;
	char op;
	typedef unsigned long ulong;
	ulong hexaddr;
	int line_count=0;
/*
	while(!feof(pFile)){
		fscanf(pFile,"%d ",&proc);
		fscanf(pFile,"%c ",&op);
		fscanf(pFile,"%lx",&hexaddr);
		cache_arr[proc]->Access(hexaddr,op,protocol,num_processors,proc,cache_arr);
		line_count++;
		if((line_count==499999)){
			printf("\nProcessor number = %d", proc);
			printf("\nOperation = %c", op);
			printf("\nAddress = %lx\n", hexaddr);
		}
	}
*/while(1){
		fscanf(pFile,"%d %c %lx",&proc,&op,&hexaddr);
		//fscanf(pFile,"%c ",&op);
		//fscanf(pFile,"%lx",&hexaddr);
		if(feof(pFile))
			break;
		cache_arr[proc]->Access(hexaddr,op,protocol,num_processors,proc,cache_arr);
		line_count++;
		if((line_count==500001)){
			printf("\nProcessor number = %d", proc);
			printf("\nOperation = %c", op);
			printf("\nAddress = %lx\n", hexaddr);
		}
	}

	printf("\nLine count = %d", line_count);
//	printf("\nProcessor number = %d", proc);
//	printf("\nOperation = %c", op);
//	printf("\nAddress = %lx\n", hexaddr);
	

	///******************************************************************//
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	for(int j=0;j<num_processors;j++){
		cache_arr[j]->printStats();
	}	
}

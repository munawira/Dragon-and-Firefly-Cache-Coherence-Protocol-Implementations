/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b,int id )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = mmsupply = cctransfer = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   cache_id = id;
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op,int protocol,int num_processors,int id,Cache **cache_arr)//,int proc,int id,Cache **cache_arr)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
	cacheLine *newline;
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(protocol==1){//Dragon
		if(line == NULL)/*miss*/
		{
			if(op == 'w') writeMisses++;
			else readMisses++;

			newline = fillLine(addr);
	   		if(op == 'w'){//write operation
				 BusTrans = BusUpd;
				 if(copies_exist(num_processors,id,addr,cache_arr)){
					newline->setFlags(SHARED_MODIFIED);
					cctransfer++;
				}
				else{
					newline->setFlags(MODIFIED);
					mmsupply++;
					
				}
			}	
			else{//read operation
				BusTrans = BusRd;
				if(copies_exist(num_processors,id,addr,cache_arr)){
					newline->setFlags(SHARED_CLEAN);
					cctransfer++;
				}
				else{
					newline->setFlags(EXCLUSIVE);
					mmsupply++;
				}
			}
		}
		else
		{
			/**since it's a hit, update LRU and update dirty flag**/
			updateLRU(line);
			switch(line->getFlags()){
				case EXCLUSIVE:
				if(op=='w')
					line->setFlags(MODIFIED);
				BusTrans = None;
				break;

				case MODIFIED:
				BusTrans = None;
				break;

				case SHARED_CLEAN:
				if(op=='w'){
					if(copies_exist(num_processors,id,addr,cache_arr)){
						line->setFlags(SHARED_MODIFIED);
						BusTrans = BusUpd;
					}
					else{
						line->setFlags(MODIFIED);
						BusTrans = None;
					}
				}else
				BusTrans = None;
				break;

				case SHARED_MODIFIED:
				if(op=='w'){
					if(copies_exist(num_processors,id,addr,cache_arr)){
						BusTrans = BusUpd;
					}
					else{
					line->setFlags(MODIFIED);
					BusTrans = None;
					}
				}else
				BusTrans = None;
				break;
			}
		}
	}
	else if(protocol==0){//FireFly
//		printf("***********");
		if(line == NULL)/*miss*/
		{
			if(op == 'w') writeMisses++;
			else readMisses++;

			newline = fillLine(addr);
	   		if(op == 'w') {//if write miss
				if(copies_exist(num_processors,id,addr,cache_arr)){
					cctransfer++;//other caches supply the data
					newline->setFlags(SHARED);    
					BusTrans = BusRd;
			//		cctransfer++;//other caches update the data
					mmsupply++;//Shared on a write hit writes through the data
				}
				else{
					mmsupply++;//reads from the memory
					newline->setFlags(MODIFIED);
					BusTrans = None;
				}
			}
			else{//if read miss
				if(copies_exist(num_processors,id,addr,cache_arr)){
					cctransfer++;//other caches supply the data
					newline->setFlags(SHARED);   
					BusTrans = BusRd; 
				}
				else{
					mmsupply++;
					newline->setFlags(VALID);
					BusTrans = None; 
				}
			}
		
		}
		else
		{
			/**since it's a hit, update LRU and update dirty flag**/
			updateLRU(line);
			switch(line->getFlags()){
				case MODIFIED:
					BusTrans = None;
				break;
				case VALID:
					if(op=='w'){
					      line->setFlags(MODIFIED);
					}
					BusTrans = None;
				break;
				case SHARED:
					if(op=='w'){
						mmsupply++;//newly commented
						if(copies_exist(num_processors,id,addr,cache_arr)){
				//		mmsupply++;//writing back to memory newly inserted
					//		cctransfer++;
					//		BusTrans = BusRd;
					//		line->setFlags(SHARED);
						}
						else{
							line->setFlags(MODIFIED);
							BusTrans = None;
						}
						BusTrans = None;
					}
					else
						BusTrans = None;
				break;
			}
		}
	}
	int temp =0;
	if(BusTrans!=None){
		for(int i=0;i<num_processors;i++){
			if(i==id)
				continue;
			temp+=cache_arr[i]->snoop_trans(BusTrans,addr,protocol);
		
		}
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(/*(victim->getFlags() == DIRTY)||*/(victim->getFlags()==SHARED_MODIFIED)||(victim->getFlags()==MODIFIED))
	writeBack(addr);

    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
//	printf("\n===== Simulation results      =====\n");
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
	printf("\n===== Simulation results (Cache_%d)     =====",cache_id);
	printf("\n01. number of reads:\t\t\t\t\t%lu",reads);
	printf("\n02. number of read misses:\t\t\t\t%lu",readMisses);
	printf("\n03. number of writes:\t\t\t\t\t%lu",writes);
	printf("\n04. number of write misses:\t\t\t\t%lu",writeMisses);
	printf("\n05. total miss rate:\t\t\t\t\t%f",(double)(readMisses+writeMisses)/(reads+writes));
	printf("\n06. number of writebacks:\t\t\t\t%lu",writeBacks);
	printf("\n07. number of memory transactions:\t\t\t%lu",mmsupply+writeBacks);
	printf("\n08. number of cache to cache transfers:\t\t\t%lu\n",cctransfer);
}

bool Cache::copies_exist(int proc,int id,ulong hexaddr,Cache **cache_arr){
	int i;
	for(i=0;i<proc;i++){
		if(i==id)
			continue;
		else{
			if((cache_arr[i]->findLine(hexaddr))!=NULL)
			return true;
		}
	}
	return false;
}

int Cache::snoop_trans(int BusTrans,ulong addr,int protocol){
	cacheLine *line = findLine(addr);
	if(line==NULL)
		return 0;
	else if(protocol ==1){//Dragon
		switch(line->getFlags()){
			case EXCLUSIVE:
				if(BusTrans == BusRd){
					line->setFlags(SHARED_CLEAN);
				}
			break;
			case MODIFIED:
				if(BusTrans == BusRd){
					//increment cache to cache transfer counter
			//		cctransfer++;
					line->setFlags(SHARED_MODIFIED);
//					mmsupply++;//block written to memory
				}
				
			break;
			case SHARED_MODIFIED:
				if(BusTrans == BusRd){
					//cache to cache transfer counter updated
			//		cctransfer++;
//					mmsupply++;//block written to memory
				}
				else if(BusTrans == BusUpd){
					line->setFlags(SHARED_CLEAN);
					//mmsupply++;//newly added
				}
			break;
			case SHARED_CLEAN:
			break;
		}
	}
	else if(protocol ==0){//FireFly
		switch(line->getFlags()){
			case SHARED:
			break;
			case VALID:
				if(BusTrans==BusRd)
					line->setFlags(SHARED);
			break;
			case MODIFIED:
				if(BusTrans==BusRd){
					mmsupply++;
					line->setFlags(SHARED);
				}
			break;
		}
	}
	return 0;
}

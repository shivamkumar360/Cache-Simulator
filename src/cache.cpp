#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <cstdint>
#include <emscripten/bind.h>

using namespace std;

struct Block {
    bool valid;
    uint64_t tag;
    int lastused;
    bool dirty;
};

// we are defining helper functions since we are using them again and again 


// hit case function 


void hit_c(vector<vector<Block>>& cache,int assoc,uint64_t index,uint64_t tag,int timer,int* hit,int* hit1,bool* h,int fifo_or_lru,bool writeins)
{
    if(cache.empty())
    {
        return;
    }
    for(int i=0;i<assoc;i++)
        {
            if(cache[index][i].valid==1 && cache[index][i].tag==tag)
            {
                (*hit)++;
                (*hit1)++;
                (*h)=true;
                if(fifo_or_lru!=0)                 // 0 for fifo and 1 for lru
                cache[index][i].lastused= timer;

                if(writeins)
                {
                cache[index][i].dirty= true;
                }

                break;
            }
        }

}

// miss case function


void miss_c(vector<vector<Block>>& cache,int assoc,uint64_t index,uint64_t tag,int timer,bool writeins,int* memory_writes)
{
    if(cache.empty())
    {
        return;
    }
    int space_found=0;
               for(int i=0;i<assoc;i++)
        {
            if(cache[index][i].valid==0)
            {
                space_found=1;
                cache[index][i].valid=1;
                cache[index][i].tag=tag;
                cache[index][i].lastused=timer;
                if(writeins)
                {
                cache[index][i].dirty=true;
                }
                break;
            }
        }

         if(space_found==0)
        {
            int lru=0;
            for(int i=0;i<assoc;i++)
        {
            if(cache[index][i].lastused<cache[index][lru].lastused)
            {
                lru=i;
            }
        }
            if(cache[index][lru].dirty)
            {
                (*memory_writes)++;
            }
            cache[index][lru].tag=tag;
            cache[index][lru].lastused= timer;
            cache[index][lru].dirty = writeins;
        }

}


// now run_cache_sim is the main function



string run_cache_sim(const string& trace_data, int cache_size1,int cache_size2,int cache_size3, int block_size1,int block_size2,int block_size3, int assoc1,int assoc2,int assoc3,int lat1, int lat2, int lat3, int lat_mem, bool fifo_or_lru) {
    if (trace_data.empty()) return "Error: File is empty.";

    
    if (cache_size1 <= 0 || block_size1 <= 0 || assoc1 <= 0) return "Error: Invalid parameters.";
    if (cache_size1 % (block_size1 * assoc1) != 0) {
        return "Invalid config: Cache size must be divisible by (block_size * assoc).";
    }

    if (cache_size2 < 0 || block_size2 < 0 || assoc2 < 0) return "Error: Invalid parameters.";
    if (cache_size2 >0 && cache_size2 % (block_size2 * assoc2) != 0) {
        return "Invalid config: Cache size must be divisible by (block_size * assoc).";
    }

    if (cache_size3 < 0 || block_size3 < 0 || assoc3 < 0) return "Error: Invalid parameters.";
    if (cache_size3 >0 && cache_size3 % (block_size3 * assoc3) != 0) {
        return "Invalid config: Cache size must be divisible by (block_size * assoc).";
    }
    
    

    // we will calcualte number of sets now



    int num_sets1 = (cache_size1 / block_size1) / assoc1;
    int num_sets2=0;
    int num_sets3=0;
    if(cache_size2>0)
    num_sets2 = (cache_size2 / block_size2) / assoc2;
    if(cache_size3>0)
    num_sets3 = (cache_size3 / block_size3) / assoc3;


    // no of sets calculated for all three cache l1,l2 and l3 ....l1 can't be empty 

    // now we are defining 2d vectors for all three cache l1,l2 and l3


    vector<vector<Block>> cache1(num_sets1, vector<Block>(assoc1));
    vector<vector<Block>> cache2(num_sets2, vector<Block>(assoc2));
    vector<vector<Block>> cache3(num_sets3, vector<Block>(assoc3));

    // now we are initialising all 3 2d vectors for l1,l2 and l3

    for(int i=0;i<num_sets1;i++)
    {
        for(int j=0;j<assoc1;j++)
        {
            cache1[i][j].valid=false;
            cache1[i][j].tag=0;
            cache1[i][j].lastused = -1;
            cache1[i][j].dirty = 0;
        }
    }

    for(int i=0;i<num_sets2;i++)
    {
        for(int j=0;j<assoc2;j++)
        {
            cache2[i][j].valid=false;
            cache2[i][j].tag=0;
            cache2[i][j].lastused = -1;
            cache2[i][j].dirty = 0;
        }
    }

    for(int i=0;i<num_sets3;i++)
    {
        for(int j=0;j<assoc3;j++)
        {
            cache3[i][j].valid=false;
            cache3[i][j].tag=0;
            cache3[i][j].lastused = -1;
            cache3[i][j].dirty = 0;
        }
    }

    // all 3 caches has been initialised 

    
    // now we are calculating   no of  offset bits for all three cache 



    int num_offset_bits1 = 0;
while ((1 << num_offset_bits1) < block_size1) 
{
num_offset_bits1++;
}

 int num_offset_bits2 = 0;
while ((1 << num_offset_bits2) < block_size2) 
{
num_offset_bits2++;
}

 int num_offset_bits3 = 0;
while ((1 << num_offset_bits3) < block_size3) 
{
num_offset_bits3++;
}

    
    
    stringstream infile(trace_data);
    string line;
    char type;
    uint64_t addr;
    int total = 0; 
    int hit = 0;           // overall hit 
    int  miss = 0;         // overall miss
    int  timer = 0;        // timer for replacement 
    int hit1 = 0;          // we get hit in cache 1
    int hit2 = 0;          // we get hit in cache 2
    int hit3 = 0;          // we get hit in cache 3
    double amat=0;         // average memory access time
    int memory_writes=0;


    while (getline(infile, line)) 
    {
        if (line.empty())
        {
         continue;
        }

        stringstream ss(line);

        // first there will be  a character i.e. R or W,  then a hexadecimal address

        if (!(ss >> type >> hex >> addr)) 
        {
        continue;
        }

        timer++;    // here we are updating the timer
        total++;    // here we are updating the total no of accesses 
        

        // now we will calculate index and tags for each cache (for l2 and l3 also if they exist)
       

        uint64_t block_addr1 = addr >> num_offset_bits1;
        uint64_t index1 = block_addr1 % num_sets1;
        uint64_t tag1   = block_addr1 / num_sets1;
        

        uint64_t block_addr2 = 0;
        uint64_t index2 = 0;
        uint64_t tag2   = 0;


        if(num_sets2>0)
        {
         block_addr2 = addr >> num_offset_bits2;
         index2 = block_addr2 % num_sets2;
         tag2   = block_addr2 / num_sets2;
        }

        uint64_t block_addr3 = 0;
        uint64_t index3 = 0;
        uint64_t tag3   = 0;


        if(num_sets3>0)
        {
         block_addr3 = addr >> num_offset_bits3;
         index3 = block_addr3 % num_sets3;
         tag3   = block_addr3 / num_sets3;
        }
        
         
        // tags and indexes have been calculated ...now we will have a lookup 


        bool writeins=false;
        if(type=='W')
        {
            writeins=true;
        }
        bool h=false;
        hit_c(cache1,assoc1,index1,tag1,timer,&hit,&hit1,&h,fifo_or_lru,writeins);


        // hit function was called for l1 ...so if there was a hit h would have become true
        // if h is still false we will have a lookup in l2 cache

        if(h==false)
        {

            bool h1=false;
            hit_c(cache2,assoc2,index2,tag2,timer,&hit,&hit2,&h1,fifo_or_lru,writeins);

            // if still there is not a hit we will go to cache l3

        if(h1==false)
        {

               bool h2=false;
               hit_c(cache3,assoc3,index3,tag3,timer,&hit,&hit3,&h2,fifo_or_lru,writeins);

               // if there is not a hit in l3 also ... we will bring it to all three cache
        
           if(h2==false)
           {
               miss++;    
         miss_c(cache1,assoc1,index1,tag1,timer,writeins,&memory_writes);
         miss_c(cache2,assoc2,index2,tag2,timer,writeins,&memory_writes);
         miss_c(cache3,assoc3,index3,tag3,timer,writeins,&memory_writes);
         // after a miss what needs to be done has been done for all three cache 
         // since there was a miss in all three  
           }
           else
           {

            miss_c(cache1,assoc1,index1,tag1,timer,writeins,&memory_writes);
            miss_c(cache2,assoc2,index2,tag2,timer,writeins,&memory_writes);

            // this is the case when we got a hit in l3...so we are calling miss fun for l1 and l2
           }
        }
        else
        {
            miss_c(cache1,assoc1,index1,tag1,timer,writeins,&memory_writes);
            // this is the case whe we got a hit a l2...so we are calling miss fun for l1
        }
        }
    }



    int effective_lat2 =0;
    if (cache_size2 > 0) 
    {
        effective_lat2=lat2;
    }
    int effective_lat3 =0;
    if (cache_size3 > 0) 
    {
        effective_lat3 =lat3;
    }
    

    

    if (total == 0)
    {
     return "Error: No valid memory traces found.";
    }


    double hit_per = ((double)hit / total);
    double hit1_per = ((double)hit1 / total);
    double hit2_per = ((double)hit2 / total);
    double hit3_per = ((double)hit3 / total);

    
    amat = hit1_per * lat1 
         + hit2_per * (lat1 + effective_lat2) 
         + hit3_per * (lat1 + effective_lat2 + effective_lat3) 
         + (1 - hit_per) * (lat1 + effective_lat2 + effective_lat3 + lat_mem);

    stringstream res;
    res << "--- Simulation Results ---\n";
    res << "Sets_L1: " << num_sets1 << " | Assoc_L1: " << assoc1 << "\n";
    res << "Sets_L2: " << num_sets2 << " | Assoc_L2: " << assoc2 << "\n";
    res << "Sets_L3: " << num_sets3 << " | Assoc_L3: " << assoc3 << "\n";
    res << "Total Accesses: " << total << "\n";
    res << "Policy: " << (fifo_or_lru ? "LRU" : "FIFO") << "\n";
    res << "Hits: " << hit << "\n";
    res << "L1_Hits: " << hit1 << "\n";
    res << "L2_Hits: " << hit2 << "\n";
    res << "L3_Hits: " << hit3 << "\n";
    res << "Misses: " << miss << "\n";
    res << "Hit Rate: " << fixed << setprecision(2) << ((double)hit / total) * 100 << "%\n";
    res << "Miss Rate: " << fixed << setprecision(2) << ((double)miss / total) * 100 << "%\n";
    res << "AMAT: " << amat<< "\n";
    res << "Write_back writes: " << memory_writes << "\n";
    return res.str();
}


EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("run_cache_sim", &run_cache_sim);
}
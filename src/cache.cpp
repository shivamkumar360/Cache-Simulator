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
};


void hit_c(vector<vector<Block>>& cache,int assoc,uint64_t index,uint64_t tag,int timer,int* hit,int* hit1,bool* h,int fifo_or_lru)
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
                if(fifo_or_lru!=0) // 0 for fifo and 1 for lru
                cache[index][i].lastused= timer;
                break;
            }
        }

}


void miss_c(vector<vector<Block>>& cache,int assoc,uint64_t index,uint64_t tag,int timer)
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
            cache[index][lru].tag=tag;
            cache[index][lru].lastused= timer;
        }

}




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


    int num_sets1 = (cache_size1 / block_size1) / assoc1;
    int num_sets2=0;
    int num_sets3=0;
    if(cache_size2>0)
    num_sets2 = (cache_size2 / block_size2) / assoc2;
    if(cache_size3>0)
    num_sets3 = (cache_size3 / block_size3) / assoc3;
    vector<vector<Block>> cache1(num_sets1, vector<Block>(assoc1));
    vector<vector<Block>> cache2(num_sets2, vector<Block>(assoc2));
    vector<vector<Block>> cache3(num_sets3, vector<Block>(assoc3));

    for(int i=0;i<num_sets1;i++)
    {
        for(int j=0;j<assoc1;j++)
        {
            cache1[i][j].valid=false;
            cache1[i][j].tag=0;
            cache1[i][j].lastused = -1;
        }
    }

    for(int i=0;i<num_sets2;i++)
    {
        for(int j=0;j<assoc2;j++)
        {
            cache2[i][j].valid=false;
            cache2[i][j].tag=0;
            cache2[i][j].lastused = -1;
        }
    }

    for(int i=0;i<num_sets3;i++)
    {
        for(int j=0;j<assoc3;j++)
        {
            cache3[i][j].valid=false;
            cache3[i][j].tag=0;
            cache3[i][j].lastused = -1;
        }
    }

    

    

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
    int hit = 0;
    int  miss = 0;
    int  timer = 0;
    int hit1 = 0;
    int hit2 = 0;
    int hit3 = 0;
    double amat=0;
    while (getline(infile, line)) 
    {
        if (line.empty()) continue;
        stringstream ss(line);

        // find a char then a hex address

        if (!(ss >> type >> hex >> addr)) continue;

        timer++;
        total++;
        
       

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


        bool h=false;
        hit_c(cache1,assoc1,index1,tag1,timer,&hit,&hit1,&h,fifo_or_lru);
        if(h==false)
        {

            bool h1=false;
            hit_c(cache2,assoc2,index2,tag2,timer,&hit,&hit2,&h1,fifo_or_lru);

        if(h1==false)
        {

               bool h2=false;
               hit_c(cache3,assoc3,index3,tag3,timer,&hit,&hit3,&h2,fifo_or_lru);
        
           if(h2==false)
           {
               miss++;    
         miss_c(cache1,assoc1,index1,tag1,timer);
         miss_c(cache2,assoc2,index2,tag2,timer);
         miss_c(cache3,assoc3,index3,tag3,timer);
           }
           else
           {

            miss_c(cache1,assoc1,index1,tag1,timer);
            miss_c(cache2,assoc2,index2,tag2,timer);
           }
        }
        else
        {
            miss_c(cache1,assoc1,index1,tag1,timer); 
        }
        }
    }



    int effective_lat2 = (cache_size2 > 0) ? lat2 : 0;
    int effective_lat3 = (cache_size3 > 0) ? lat3 : 0;

    

    if (total == 0) return "Error: No valid memory traces found.";


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
    return res.str();
}



EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("run_cache_sim", &run_cache_sim);
}
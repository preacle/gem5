#ifndef __CPU_O3_SSNPDT_HH__
#define __CPU_O3_SSNPDT_HH__
#include <iostream>
#include <vector>
#define PREDSIZE (1024*1024)
using namespace std;


class predItem{
public:
    uint64_t tag;   // tag
    uint64_t value; // SSN差
    uint64_t c;     // 置信度
    uint64_t v;     // 有效位
    uint64_t pc;
    uint64_t pc_c;

public:
        predItem():tag(0),value(0),c(0),v(0),pc(0),pc_c(0){}
//        predItem(uint64_t t, uint64_t val, uint64_t c, uint64_t v):
//          tag(t),value(val),c(c),v(v){}

        bool insert(uint64_t insertTag,uint64_t insertVal,uint64_t insertPC){
            if (v == 1){
               if (tag == insertTag){
                    if (value == insertVal){
                        c = (63 == c)?63:c+1;
                    }
                    else{
                        value = insertVal;
                        c = 1;
                    }

                    if ( pc == insertPC){
                      pc_c = (63 == pc_c)?63:pc_c+1;
                    }else{
                      pc = insertPC;
                      pc_c = 1;
                    }
                }else{
                    tag = insertTag;
                    value = insertVal;
                    c = 1;
                    pc = insertPC;
                    pc_c = 1;

                }
            }else{
                v = 1;
                tag = insertTag;
                value = insertVal;
                c = 1;
                pc = insertPC;
                pc_c = 1;
            }
            return true;
        }
};

class SSNPDT{
    vector<predItem* > table;
    // 0 有效位
    // 1 pc tag
    // 2 ssn
    uint64_t pclink[PREDSIZE][3];
public:
    SSNPDT(){
      int sz = PREDSIZE;
      table = vector<predItem*>(sz);
      for (int i=0;i!=sz;i++) table[i] = new predItem();
    }
    bool insertLoad(uint64_t pc, uint64_t pc_v, uint64_t val){
        uint64_t idx = (pc>>1) % PREDSIZE;
        uint64_t tag = (pc>>1) / PREDSIZE;
        table[idx]->insert(tag,val,pc_v);
        return true;
    }
    bool getSSN(uint64_t pc,uint64_t gssn, uint64_t& val, uint64_t& c){

        uint64_t idx = (pc>>1) % PREDSIZE;
        uint64_t tag = (pc>>1) / PREDSIZE;
        if (table[idx]->tag != tag){
            return false;
        }
        if (c == 63){
          return table[idx]->v;
        }
        uint64_t trueIdx = table[idx]->pc;
        uint64_t pc_tag = trueIdx/PREDSIZE;
        uint64_t pc_idx = trueIdx%PREDSIZE;
        if (table[idx]->pc_c > table[idx]->c
          && pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag){
          val = gssn - pclink[pc_idx][3];
          if (val <= 30){
            c = table[idx]->pc_c;
            return true;
          }
        }
        val = table[idx]->value;
        c = table[idx]->c;
        return table[idx]->v;
    }
    void insertStore(uint64_t trueIdx,uint64_t ssn){
      uint64_t idx = trueIdx%PREDSIZE;
      pclink[idx][0] = 1;
      pclink[idx][1] = trueIdx/PREDSIZE;
      pclink[idx][2] = ssn;
      return;
    }

    void invaild(uint64_t trueIdx){
      uint64_t idx = (trueIdx)%PREDSIZE;
      if ((trueIdx)/PREDSIZE == pclink[idx][1])
        pclink[idx][0] = 0;
    }
};
#endif //__CPU_O3_SSNPDT_HH__

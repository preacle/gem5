#ifndef __CPU_O3_SSNPDT_HH__
#define __CPU_O3_SSNPDT_HH__
#include <iostream>
#include <vector>
#define PREDSIZE (512)
#define LOGPREDSIZE (9)
using namespace std;

#define maxBypassDist 64
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
    vector<predItem* > table0;
    vector<predItem* > table1;
    // 0 有效位
    // 1 pc tag
    // 2 ssn
    uint64_t pclink[PREDSIZE][3];
public:
    SSNPDT(){
      int sz = PREDSIZE;
      table0 = vector<predItem*>(sz);
      table1 = vector<predItem*>(sz);
      for (int i=0;i!=sz;i++) table0[i] = new predItem();
      for (int i=0;i!=sz;i++) table1[i] = new predItem();
    }
    bool insertLoad(uint64_t pc, uint64_t pc_v, uint64_t val,uint64_t history){
        uint64_t idx0 = (pc>>1) % PREDSIZE;
        uint64_t tag0 = (pc>>1) / PREDSIZE;
        table0[idx0]->insert(tag0,val,pc_v);

        uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
        uint64_t tag1 = ((pc>>1)^(pc>>9)^history)/ PREDSIZE;
        table1[idx1]->insert(tag1,val,pc_v);

        return true;
    }
    bool getSSN(
      uint64_t pc,
      uint64_t gssn,
      uint64_t history,
      uint64_t& val,
      uint64_t& c){

        uint64_t idx0 = (pc>>1) % PREDSIZE;
        uint64_t tag0 = (pc>>1) / PREDSIZE;

        uint64_t trueIdx;
        uint64_t pc_tag;
        uint64_t pc_idx;

        uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
        uint64_t tag1 = ((pc>>1)^(pc>>9)^history)/ PREDSIZE;
        if (table0[idx0]->tag != tag0 && table1[idx1]->tag != tag1){
            return false;
        }
        if (table1[idx1]->tag == tag1){
          trueIdx = table1[idx1]->pc;
          pc_tag = trueIdx/PREDSIZE;
          pc_idx = trueIdx%PREDSIZE;
          if (table1[idx1]->pc_c > table1[idx1]->c
            && pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag){
            val = gssn - pclink[pc_idx][2];
            if (val < maxBypassDist){
              c = table1[idx1]->pc_c;
              return true;
            }
          }
          val = table1[idx1]->value;
          c = table1[idx1]->c;
          return table1[idx1]->v;
        }
        else{
          trueIdx = table0[idx0]->pc;
          pc_tag = trueIdx/PREDSIZE;
          pc_idx = trueIdx%PREDSIZE;
          if (table0[idx0]->pc_c > table0[idx0]->c
            && pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag){
            val = gssn - pclink[pc_idx][2];
            if (val < maxBypassDist){
              c = table0[idx0]->pc_c;
              return true;
            }
          }
          val = table0[idx0]->value;
          c = table0[idx0]->c;
          return table0[idx0]->v;
        }
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

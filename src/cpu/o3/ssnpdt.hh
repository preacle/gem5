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
    int v;     // 有效位
    uint64_t pc;
    uint64_t pc_c;
    bool delayUntilCommit;

public:
        predItem():tag(0),value(0),c(0),v(0),pc(0),pc_c(0),delayUntilCommit(false){}
//        predItem(uint64_t t, uint64_t val, uint64_t c, uint64_t v):
//          tag(t),value(val),c(c),v(v){}

        void insert(uint64_t insertTag,uint64_t insertVal,uint64_t insertPC,int pow){
               if (tag == insertTag){
                    delayUntilCommit = false;
                    v = min(3,v+pow);
                    if (value == insertVal && insertPC != 0){
                        // std::cout<<"insert:1"<<std::endl;
                        c = (63 == c)?63:c+1;
                    }
                    else{
                        // std::cout<<"insert:2"<<std::endl;
                        value = insertVal;
                        c = 1;
                    }

                    if ( pc == insertPC){
                        // std::cout<<"insert:3"<<std::endl;
                      pc_c = (3 == pc_c)?3:pc_c+1;
                    }else{
                        // std::cout<<"insert:4"<<std::endl;
                      pc = insertPC;
                      pc_c = 1;
                    }
                }else{
                   if (v >= pow){
                      v = v - pow;
                   }else{
                      delayUntilCommit = false;
                      v = pow;
                      tag = insertTag;
                      value = insertVal;
                      c = 1;
                      pc = insertPC;
                      pc_c = 1;
                    }
                }
        }

        void delay(uint64_t insertTag,uint64_t insertVal,uint64_t insertPC,int pow){
          if (tag == insertTag){
               delayUntilCommit = true;
               v = min(3,v+pow);
               if (value == insertVal && insertPC != 0){
                   // std::cout<<"insert:1"<<std::endl;
                   c = (8 == c)?8:c+1;
               }
               else{
                   // std::cout<<"insert:2"<<std::endl;
                   value = insertVal;
                   c = 1;
               }

               if ( pc == insertPC){
                   // std::cout<<"insert:3"<<std::endl;
                 pc_c = (4 == pc_c)?4:pc_c+1;
               }else{
                   // std::cout<<"insert:4"<<std::endl;
                 pc = insertPC;
                 pc_c = 1;
               }
           }else{
              if (v >= pow){
                 v = v - pow;
              }else{
                delayUntilCommit = true;
                 v = pow;
                 tag = insertTag;
                 value = insertVal;
                 c = 1;
                 pc = insertPC;
                 pc_c = 1;
               }
           }
        }
        void clear(uint64_t clearTag){
          std::cout<<"clear"<<std::endl;
          if (tag == clearTag){
            c = 0;
            value = 9999;
            pc = 0;
            pc_c = 0;
            v = 0;
          }
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
    bool insertLoad(uint64_t pc, uint64_t pc_v, uint64_t val,uint64_t history,int pow){
        if (val >= 30){
          uint64_t idx0 = (pc>>1) % PREDSIZE;
          uint64_t tag0 = (pc>>1) / PREDSIZE;
          table0[idx0]->clear(tag0);

          uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
          uint64_t tag1 = ((pc)^(pc>>11)^history)% PREDSIZE;
          table1[idx1]->clear(tag1);
          return true;
        }

        // std::cout<<"insert: pc"<<pc<<" pc_v"<<pc_v<<" val"<<val<<" history"<<history<<std::endl;
        uint64_t idx0 = (pc>>1) % PREDSIZE;
        uint64_t tag0 = (pc>>1) / PREDSIZE;
        table0[idx0]->insert(tag0,val,pc_v,pow);

        uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
        uint64_t tag1 = ((pc)^(pc>>11)^history)% PREDSIZE;
        table1[idx1]->insert(tag1,val,pc_v,pow);

        return true;
    }

    bool delay(uint64_t pc, uint64_t pc_v, uint64_t val,uint64_t history,int pow){
        if (val >= 20){
          return true;
        }

        // std::cout<<"insert: pc"<<pc<<" pc_v"<<pc_v<<" val"<<val<<" history"<<history<<std::endl;
        uint64_t idx0 = (pc>>1) % PREDSIZE;
        uint64_t tag0 = (pc>>1) / PREDSIZE;
        table0[idx0]->delay(tag0,val,pc_v,pow);

        uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
        uint64_t tag1 = ((pc)^(pc>>11)^history)% PREDSIZE;
        table1[idx1]->delay(tag1,val,pc_v,pow);

        return true;
    }

    bool clearLoad(uint64_t pc,uint64_t history){
      // std::cout<<"clearLoad  pc: "<<pc<<" history"<<history;
      uint64_t idx0 = (pc>>1) % PREDSIZE;
      uint64_t tag0 = (pc>>1) / PREDSIZE;
      table0[idx0]->clear(tag0);

      uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
      uint64_t tag1 = ((pc)^(pc>>11)^history)% PREDSIZE;
      table1[idx1]->clear(tag1);

      return true;
    }
    bool getSSN(
      uint64_t pc,
      uint64_t gssn,
      uint64_t history,
      uint64_t& val,
      uint64_t& c,
      bool& delay,
      bool& delayUntilCommit){
         // std::cout<<"getSSN:pc"<<pc<<" :gssn"<<gssn<<std::endl;
        uint64_t idx0 = (pc>>1) % PREDSIZE;
        uint64_t tag0 = (pc>>1) / PREDSIZE;

        uint64_t trueIdx;
        uint64_t pc_tag;
        uint64_t pc_idx;

        uint64_t idx1 = ((pc>>1)^(pc>>9)^history)% PREDSIZE;
        uint64_t tag1 = ((pc)^(pc>>11)^history)% PREDSIZE;
        if (table0[idx0]->tag != tag0 && table1[idx1]->tag != tag1){
            // std::cout<<"getSSN:pc"<<pc<<"miss"<<std::endl;
            return false;
        }
        if (table1[idx1]->tag == tag1){
          delayUntilCommit = table1[idx1]->delayUntilCommit;
          trueIdx = table1[idx1]->pc;
          if (trueIdx == 0){
              delay = true;
          }
          pc_tag = trueIdx/PREDSIZE;
          pc_idx = trueIdx%PREDSIZE;
          // std::cout<<"getSSN:pc"<<pc<<" pc_c:"<<table1[idx1]->pc_c
          // <<" c:"<<table1[idx1]->c
          // <<" validTag:"<<(pclink[pc_idx][1] == pc_tag)
          // <<" valid:"<<pclink[pc_idx][0]
         // <<" 3:"<<(gssn >= pclink[pc_idx][2])<<std::endl;
          if (table1[idx1]->pc_c > table1[idx1]->c
            && pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag
            &&  gssn >= pclink[pc_idx][2]){
            delay = true;
            val = gssn - pclink[pc_idx][2];
            // std::cout<<"getSSN:pc"
            // <<pc<<" :idx"<<trueIdx<<" validTag"
            // <<(pclink[pc_idx][1] == pc_tag)
            // <<" value:"<<val<<std::endl;
            if (val < maxBypassDist){
              c = table1[idx1]->pc_c;
              return true;
            }
          }
          if (pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag){
              val = table1[idx1]->value;
              c = table1[idx1]->c;
              return true;
          }
          return false;
        }
        else{
          delayUntilCommit = table0[idx0]->delayUntilCommit;
          trueIdx = table0[idx0]->pc;
          if (trueIdx == 0){
              delay = true;
          }
          pc_tag = trueIdx/PREDSIZE;
          pc_idx = trueIdx%PREDSIZE;
          // std::cout<<"getSSN:pc"<<pc<<" pc_c:"<<table0[idx1]->pc_c
          // <<" c:"<<table0[idx1]->c
          // <<" validTag:"<<(pclink[pc_idx][1] == pc_tag)
          // <<" valid:"<<pclink[pc_idx][0]
          // <<" 3:"<<(gssn >= pclink[pc_idx][2])<<std::endl;
          if (table0[idx0]->pc_c > table0[idx0]->c
            && pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag
            &&  gssn >= pclink[pc_idx][2]){
            delay = true;
            // std::cout<<"getSSN:pc"<<pc
            // <<"trueIdx"<<trueIdx
            // <<" validTag:"<<(pclink[pc_idx][1] == pc_tag)
            // <<" value:"<<val<<std::endl;
            val = gssn - pclink[pc_idx][2];
            if (val < maxBypassDist){
              c = table0[idx0]->pc_c;
              return true;
            }
          }
          if (pclink[pc_idx][0] && pclink[pc_idx][1] == pc_tag){
              val = table0[idx0]->value;
              c = table0[idx0]->c;
              return true;
          }
          return false;
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

    void invaildAll(uint64_t squashSSN){
      for (int idx = 0;idx<PREDSIZE;idx++){
        if (pclink[idx][0]&&pclink[idx][2]>squashSSN)
          pclink[idx][0] = 0;
      }
      return ;
    }
};
#endif //__CPU_O3_SSNPDT_HH__

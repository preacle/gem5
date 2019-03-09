#ifndef __CPU_O3_SSNPDT_HH__
#define __CPU_O3_SSNPDT_HH__
#include <iostream>
#include <vector>

using namespace std;

class predItem{
public:
    uint64_t tag;   // tag
    uint64_t value; // SSN差
    uint64_t c;     // 置信度
    uint64_t v;     // 有效位

public:
        predItem():tag(0),value(0),c(0),v(0){}
        predItem(uint64_t t, uint64_t val, uint64_t c, uint64_t v):
          tag(t),value(val),c(c),v(v){}

        bool insert(uint64_t insertTag,uint64_t insertVal){
            if (v == 1){
               if (tag == insertTag){
                    if (value == insertVal){
                        c = (3 == c)?3:c+1;
                    }
                    else{
                        value = insertVal;
                        c = 1;
                    }
                }else{
                    tag = insertTag;
                    value = insertVal;
                    c = 1;
                }
            }else{
                v = 1;
                tag = insertTag;
                value = insertVal;
                c = 1;
            }
            return true;
        }
};

class SSNPDT{
    vector<predItem* > table;
public:
    SSNPDT(){
      int sz = 1024;
      table = vector<predItem*>(sz);
      for (int i=0;i!=sz;i++) table[i] = new predItem();
    }
    bool insert(uint64_t pc,uint64_t val){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        table[idx]->insert(tag,val);
        return true;
    }
    bool getSSN(uint64_t pc, uint64_t& val, uint64_t& c){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        if (table[idx]->tag != tag){
            return false;
        }
        val = table[idx]->value;
        c = table[idx]->c;
        return table[idx]->v;
    }
};
#endif //__CPU_O3_SSNPDT_HH__

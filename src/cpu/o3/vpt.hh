#ifndef __CPU_O3_VPT_HH__
#define __CPU_O3_VPT_HH__
#include <iostream>
#include <list>
#include <vector>

using namespace std;

class GHTItem{
public:
  uint64_t SN;
  uint64_t HIST;
public:
  GHTItem(uint64_t sn,uint64_t hi):SN(sn),HIST(hi){}
};

class GHT{
public:
  std::list<GHTItem *> hist;
  GHT(){
    for (int i=0;i<30;i++) hist.push_front(new GHTItem(0,0));
  }
  uint64_t getHist(){
    return hist.front()->HIST;
  }
  void insert(uint64_t sn,bool taken){
    uint64_t H = getHist();
    H = (H<<1)|(taken?1:0);
    hist.push_front(new GHTItem(sn,H));
    if (hist.size() == 30){
      auto x = hist.back();
      hist.pop_back();
      free(x);
    }

  }
  void squash(uint64_t sn){
    if (hist.size() != 0){
      auto top = hist.front();
      if (top->SN >= sn){
        hist.pop_front();
        free(top);
      }
    }
    return ;
  }
  void squash(uint64_t sn,bool taken){
    squash(sn);
    insert(sn,taken);
  }
};

class VPTItem{
public:
    uint64_t tag;   // tag
    uint64_t value; // 值
    uint64_t c;     // 置信度
    uint64_t v;     // 有效位

public:
        VPTItem():tag(0),value(0),c(0),v(0){}
        VPTItem(uint64_t t, uint64_t val, uint64_t c, uint64_t v):
          tag(t),value(val),c(c),v(v){}

        bool insert(uint64_t insertTag,uint64_t insertVal){
            if (v == 1){
               if (tag == insertTag){
                    if (value == insertVal){
                        c = (127 == c)?127:c+1;
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

class LVP{
    vector<VPTItem* > table;
public:
    LVP(){
      int sz = 1024;
      table = vector<VPTItem*>(sz);
      for (int i=0;i!=sz;i++) table[i] = new VPTItem();
    }
    bool insert(uint64_t pc,uint64_t val){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        table[idx]->insert(tag,val);
        return true;
    }
    bool clear(uint64_t pc){
        uint64_t idx = (pc>>2) % 1024;
        free(table[idx]);
        table[idx] = new VPTItem();
        return true;
    }
    bool getValue(uint64_t pc, uint64_t& val, uint64_t& c){
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



class L0Item{
public:
    uint64_t tag;   // tag
    uint64_t value; // 值
    uint64_t v;     // 有效位
    uint64_t sz;
    uint64_t ssn;

public:
        L0Item():tag(0),value(0),v(0),ssn(0){}

        bool insert(
          uint64_t insertTag,uint64_t insertValue,uint64_t SSN,uint64_t SZ){
            if (v >= 1){
               if (tag == insertTag){
                 value = insertValue;
                 ssn = SSN;
                 sz = SZ;
                 v= (v+1)>=3?3:1;
                }else{
                    v--;
                }
            }else{
                v = 1;
                tag = insertTag;
                value = insertValue;
                ssn = SSN;
                sz = SZ;
            }
            return true;
        }
        void clear(uint64_t insertTag){
          if (tag == insertTag)
            v = 0;
        }
};

class L0CACHE{
  vector<L0Item* > table;
  public:
    L0CACHE(){
      int sz = 1024;
      table = vector<L0Item*>(sz);
      for (int i=0;i!=sz;i++) table[i] = new L0Item();
    }

    bool insert(uint64_t pc,uint64_t insertValue, uint64_t SSN,uint64_t SZ){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        table[idx]->insert(tag,insertValue,SSN,SZ);
        return true;
    }

    bool getValue(uint64_t pc, uint64_t& val,uint64_t& SSN,uint64_t SZ){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        if (table[idx]->tag != tag || table[idx]->sz != SZ){
            return false;
        }
        if (table[idx]->tag == tag
          && table[idx]->sz == SZ
          && table[idx]->v >= 1){
          table[idx]->v++;
          val = table[idx]->value;
          SSN = table[idx]->ssn;
          return true;
        }
        return false;
    }

    bool clear(uint64_t pc){
      uint64_t idx = (pc>>2) % 1024;
      uint64_t tag = (pc>>2) / 1024;
      table[idx]->clear(tag);
      return true;
    }
};

class SAPItem{
public:
    uint64_t tag;   // tag
    uint64_t addr; // 值
    uint64_t c;     // 置信度
    uint64_t v;     // 有效位
    uint64_t stride;

public:
        SAPItem():tag(0),addr(0),c(0),v(0),stride(0){}

        bool insert(uint64_t insertTag,uint64_t insertAddr){
            if (v == 1){
               if (tag == insertTag){
                    if (addr == insertAddr){
                        addr = addr + stride;
                        c = (63 == c)?63:c+1;
                    }
                    else{
                        stride = insertAddr - addr;
                        addr = insertAddr;
                        c = 1;
                    }
                }else{
                  if (insertAddr == 0){
                      v = 0;
                  }
                    tag = insertTag;
                    addr = insertAddr;
                    stride = 0;
                    c = 1;
                }
            }else{
                v = 1;
                tag = insertTag;
                addr = insertAddr;
                stride = 0;
                c = 1;
            }
            return true;
        }
};

class SAP{
    vector<SAPItem* > table;
public:
    SAP(){
      int sz = 1024;
      table = vector<SAPItem*>(sz);
      for (int i=0;i!=sz;i++) table[i] = new SAPItem();
    }
    bool insert(uint64_t pc,uint64_t addr){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        table[idx]->insert(tag,addr);
        return true;
    }

    bool clear(uint64_t pc){
        uint64_t idx = (pc>>2) % 1024;
        free(table[idx]);
        table[idx] = new SAPItem();
        return true;
    }
    bool getValue(uint64_t pc, uint64_t& addr, uint64_t& c){
        uint64_t idx = (pc>>2) % 1024;
        uint64_t tag = (pc>>2) / 1024;
        if (table[idx]->tag != tag){
            return false;
        }
        addr = table[idx]->addr;
        c = table[idx]->c;
        return table[idx]->v;
    }
};
#endif //__CPU_O3_SVW_HH__

#ifndef __CPU_O3_SVW_HH__
#define __CPU_O3_SVW_HH__

#include <iostream>
#include <list>
#include <vector>

using namespace std;
typedef uint64_t SVWTag_t;
typedef uint64_t SVWStoreSeqNum_t;
typedef uint64_t SVWKey_t;

template <class Impl>
class SVW{
  public:
    typedef typename Impl::DynInstPtr DynInstPtr;
  public:
    int depCheckShift = 2;
    class svwItem{
        public:
            bool VAILD;
            SVWTag_t TAG;
            SVWStoreSeqNum_t SSN;
        svwItem(bool valid,SVWTag_t tag,SVWStoreSeqNum_t ssn)
        :VAILD(valid),TAG(tag),SSN(ssn){}
    };
    vector<list<svwItem> > svwItems;
    uint64_t size;
    uint64_t assoc;
public:
    SVW(size_t sz,size_t assoc):size(sz),assoc(assoc){
        svwItems = vector<list<svwItem>>
          (sz,list<svwItem> (assoc,svwItem(0,0,0)));
    }


    void insert(SVWKey_t key, SVWTag_t tag, SVWStoreSeqNum_t ssn){
        svwItems[key].pop_back();
        svwItems[key].push_front(svwItem(true,tag,ssn));
    }

    void insert(DynInstPtr &inst){
      if (inst->effAddr == 0)
        return;
      auto inst_eff_addr1 = inst->effAddr >> depCheckShift;
      auto inst_eff_addr2 =
        (inst->effAddr + inst->effSize - 1) >> depCheckShift;
      for (auto addr = inst_eff_addr1; addr <= inst_eff_addr2; addr++){
        SVWKey_t key = addr % size;
        SVWTag_t tag = addr / size;
        insert(key,tag,inst->SSN);
      }
    }

    SVWStoreSeqNum_t search(SVWKey_t key,SVWTag_t tag){
        SVWStoreSeqNum_t ret = 0;
        for (auto i:svwItems[key]){
            if (i.VAILD && i.TAG == tag){
                return i.SSN;
            }else{
                ret = i.SSN;
            }
        }
        return ret;
    }

    bool violation(DynInstPtr &inst){
      if (inst->effAddr == 0)
        return false;
      auto inst_eff_addr1 = inst->effAddr >> depCheckShift;
      auto inst_eff_addr2 =
        (inst->effAddr + inst->effSize - 1) >> depCheckShift;
      for (auto addr = inst_eff_addr1; addr <= inst_eff_addr2; addr++){
        SVWKey_t key = addr % size;
        SVWTag_t tag = addr / size;
        SVWStoreSeqNum_t ssn = search(key, tag);
        if (ssn > inst->SSN){
          return true;
        }
      }
      return false;
    }


    void printKey(SVWKey_t key){
        for (auto i:svwItems[key]){
            std::cout<<i.valid<<" "<<i.TAG<<" "<<i.SSN<<std::endl;
        }
        std::cout<<"---------------------------"<<std::endl;
    }
};
#endif //__CPU_O3_SVW_HH__

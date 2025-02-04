//Abstract class that represents the distribution network.

#ifndef __DistributionNetworkAbstract__
#define __DistributionNetworkAbstract__

#include <vector>
#include <iostream>

#include "DSNetwork.h"
#include "Connection.h"
#include <map>
#include <iostream>
#include <assert.h>


//This class represents a general case of a distribution network and cannot be instantiated

class DistributionNetwork : public Unit {
private:
  
public:
    //General constructor, just used to heritage with unit
    DistributionNetwork(id_t id, std::string name) : Unit(id, name) {}
    virtual ~DistributionNetwork() {}

    // 下面都是纯虚函数，通过assert(false)强制子类实现

    //This just executes cycle over all the dsnetworks
    // 模拟分发网络在一个时钟周期中的行为 
    virtual void cycle() {assert(false);} 
    //Get last levels connections together. Useful to connect with mswitches later.
    virtual std::map<int, Connection*> getLastLevelConnections() {assert(false);} 
    // Get the top connections (i.e., the ones that connect the SDMemory ports)
    virtual std::vector<Connection*> getTopConnections() {assert(false);} 
    virtual void printStats(std::ofstream& out, unsigned int indent) {assert(false);}
    virtual void printEnergy(std::ofstream& out, unsigned int indent) {assert(false);}

};

#endif
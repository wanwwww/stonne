// Created on 06/11/2019 by Francisco Munoz Martinez

#ifndef __DSNetworkTop__
#define __DSNetworkTop__

#include <vector>
#include <iostream>

#include "DSNetwork.h"
#include "Connection.h"
#include <map>
#include <iostream>
#include "DistributionNetwork.h"


//This class represents the whole DSNetwork that is composed by several DSNetworks trees. Basically, a DSNetwork has as many trees as input ports has the architecture to fetch input data.
// 管理多个DSNetwork子网络的行为 

class DSNetworkTop : public DistributionNetwork {
private:
    unsigned int n_input_ports; // 输入端口数量
    unsigned int ms_size_per_port; //Number of multipliers per each ds tree
    unsigned int port_width; 
    
    std::vector<DSNetwork*> dsnetworks; //one per port  存储每个输入端口对应的DSNetwork子网络的指针
    std::vector<Connection*> connections; //one per port 存储与存储器端口连接的connection对象，用于数据传输 
    
public:
    //The constructor creates n_input_ports dsnetworks with portWidth port_width
    DSNetworkTop(id_t id, std::string name, Config stonne_cfg); 
    ~DSNetworkTop();
    void cycle(); //This just executes cycle over all the dsnetworks
    std::map<int, Connection*> getLastLevelConnections(); //Get last levels connections from all the astrees together. Useful to connect with mswitches later.
    std::vector<Connection*> getTopConnections(); // Get the top connections (i.e., the ones that connect the SDMemory ports with the DS subtrees)
    void printStats(std::ofstream& out, unsigned int indent);
    void printEnergy(std::ofstream& out, unsigned int indent);

};

#endif

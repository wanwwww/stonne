// Created on 06/11/2019 by Francisco Munoz Martinez

#include "DSNetworkTop.h"
#include <iostream>
#include "utility.h"

DSNetworkTop::DSNetworkTop(id_t id, std::string name, Config stonne_cfg) : DistributionNetwork(id,name) {
    this->n_input_ports = stonne_cfg.m_SDMemoryCfg.n_read_ports;  // dn_bw

    // 根据乘法器网络类型，计算每个端口分配的乘法器数量 
    if(stonne_cfg.m_MSNetworkCfg.multiplier_network_type==LINEAR) {
        this->ms_size_per_port = stonne_cfg.m_MSNetworkCfg.ms_size / n_input_ports;
    }
    else if(stonne_cfg.m_MSNetworkCfg.multiplier_network_type==OS_MESH) {
        this->ms_size_per_port = (stonne_cfg.m_MSNetworkCfg.ms_rows + stonne_cfg.m_MSNetworkCfg.ms_cols) / n_input_ports;
        // std::cout<<std::endl;
        // std::cout<<"n_ms : "<<stonne_cfg.m_MSNetworkCfg.ms_rows + stonne_cfg.m_MSNetworkCfg.ms_cols<<std::endl;
        // std::cout<<"n_input_ports : "<<n_input_ports<<std::endl;
        // std::cout<<"DSNetwork ms_size_per_port : "<<this->ms_size_per_port<<std::endl;
        // std::cout<<std::endl;
    }
    this->port_width = stonne_cfg.m_DSwitchCfg.port_width; // 16 

    // 循环为每个输入端口创建connection对象和DSNetwork网络
    // connection对象：表示存储器到子网络的连接
    // DSNework子网络，初始化其连接和参数 
    for(int i=0; i<this->n_input_ports; i++) {
        //std::cout<<"create DSNetwork"<<std::endl;
        //Creating the top connection first
        Connection* conn = new Connection(this->port_width);
        //Creating the tree
        std::string name = "ASNetworkTree "+i; 
        DSNetwork* dsnet = new DSNetwork(i,name, stonne_cfg, this->ms_size_per_port, conn); //Creating the dsnetwork with the connection
        connections.push_back(conn);
        dsnetworks.push_back(dsnet);
       
    }

}

// 返回键值对类型
std::map<int, Connection*> DSNetworkTop::getLastLevelConnections() {
    // connectionsLastLevel 用于存储所有子网络的最后一级连接，即子网络计算结果的输出连接 
    std::map<int, Connection*> connectionsLastLevel; 
    for(int i=0; i<this->n_input_ports; i++)  { //For each tree we add its lastlevelconnections  得到每个端口对应树的最后一级连接 
        std::map<int, Connection*> connectionsPort = this->dsnetworks[i]->getLastLevelConnections(); //Getting the last level conns of the tree i
        unsigned int index_base = i*ms_size_per_port; //Current connection respect to the first connection in the first tree
        for(int j=0; j<this->ms_size_per_port; j++) { //We are sure connectionsPort size is ms_size_per_per_port 
            Connection* current_connection = connectionsPort[j]; //Local index
            connectionsLastLevel[index_base+j]=current_connection; //Adding to the global list
        }
    }
    return connectionsLastLevel;
}

//Return the top connections (i.e., the input connections that connects the DSMemory ports with the subtrees)
// 顶层连接，即存储器端口到子网络的连接
std::vector<Connection*> DSNetworkTop::getTopConnections() {
    return this->connections;
}

// cycle()方法调用所有DSNetwork子网络的cycle()方法，模拟每个子网络在一个时钟周期内的行为 
void DSNetworkTop::cycle() {
    for(int i=0; i<this->n_input_ports; i++) {
        dsnetworks[i]->cycle();
    }
}

DSNetworkTop::~DSNetworkTop() {
    for(int i=0; i<this->n_input_ports; i++) {
        delete dsnetworks[i];
        delete connections[i];
    }
}

void DSNetworkTop::printStats(std::ofstream& out, unsigned int indent) {
    out << ind(indent) << "\"DSNetworkStats\" : {" << std::endl;
    //General statistics if there are

    //For each subtree
    out << ind(indent+IND_SIZE) << "\"DSTreeStats\" : [" << std::endl;
    for(int i=0; i<this->n_input_ports; i++) {
        dsnetworks[i]->printStats(out, indent+IND_SIZE+IND_SIZE);
        if(i==(this->n_input_ports-1)) { //If I am in the last tree I do not have to separate the objects with a comma
            out << std::endl;
        }
        else { //Put a comma between two DStree objects
            out << "," << std::endl;
        }
    }
    out << ind(indent+IND_SIZE) << "]" << std::endl;
    out << ind(indent) << "}";
}

void DSNetworkTop::printEnergy(std::ofstream& out, unsigned int indent) {
   /*
       This component prints:
           - Connections between memory and DSNetworks
           - DSNetworks
   */
   
   //Printing wires between memory and DSNetwork
   for(int i=0; i<this->n_input_ports; i++)  {
       this->connections[i]->printEnergy(out, indent, "DN_WIRE");
   }

   //Printing ASNetworks
   for(int i=0; i<this->n_input_ports; i++)  {
       this->dsnetworks[i]->printEnergy(out, indent);
   }
   
}

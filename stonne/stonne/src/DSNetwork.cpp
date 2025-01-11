//
// Created by Francisco Munoz on 17/06/19.
//
#include "DSNetwork.h"
#include <assert.h>
#include "utility.h"
#include <math.h>

// 构造函数创建 分发网络树，是一个二叉树
//This Constructor creates the distribution tree similar to the one shown in the paper
DSNetwork::DSNetwork(id_t id, std::string name, Config stonne_cfg, unsigned int ms_size, Connection* inputConnection) : Unit(id,name) {
    // Collecting parameters from the configuration file
    this->ms_size = ms_size;  // 在TPU模式下，该值为1
    this->port_width=stonne_cfg.m_DSwitchCfg.port_width;  // 16
    //End collecting parameters from the configuration file 
    assert(ispowerof2(ms_size)); //Ensure the number of multipliers is power of 2.
   
    this->inputConnection = inputConnection; // 整个子网络的输入连接
    int nlevels = log10(ms_size) / log10(2); //All the levels without count the leaves (MSwitches)  // 计算层数 0
    this->nlevels = nlevels;
    // std::cout<<"the num of nlevels : "<<this->nlevels<<std::endl;  // 0 
    int switches_this_level = 1; // 根节点只有一个 
    unsigned int sw_id = 0; //id of each sw

    // 在TPU模式下，this->nlevels = 0，不执行下面的循环
    // 从根节点开始，逐层创建节点和连接 
    for(int i=0; i < this->nlevels; i++) {  //From root to leaves (without the MSs)
        //std::cout<<"1111111111111111111111111111111111111111111111====================="<<std::endl;
        //Calculating the output ports in this level
        for(int j=0; j < switches_this_level; j++) { // From left to right of the structure
            std::string sw_str="DSwitch "+sw_id; 
            // DSwitch::DSwitch(id_t id, std::string name, unsigned int level, unsigned int num_in_level, Config stonne_cfg, unsigned int ms_size) 
            DSwitch* ds = new DSwitch(sw_id, sw_str, i, j, stonne_cfg, this->ms_size); // 
            std::pair<int, int> levelandnum (i,j);  // 节点的层id 和层内id 组成数据对 （0，0）
            dswitchtable[levelandnum] = ds;
            
            //Connecting with source. (i.e., connections from the above level)
            // 设置输入连接
            if(i==0) {  //The first node is connected to the inputConnection. This is an extreme case but it is more clear to do it inside loop
                ds->setInputConnection(inputConnection);
            }

            else {
                std::pair<int, int> sourcepair(i-1, j); // the same number of connections in the above level matches the number of Dswitches in this l
                Connection* correspondingSourceConnection = connectiontable[sourcepair];
                ds->setInputConnection(correspondingSourceConnection);
            }

            // 创建并连接输出连接 
            //Creating and Connecting destination connections
            //For each connection
            for(int c=0; c < CONNECTIONS_PER_SWITCH; c++) {  // CONNECTIONS_PER_SWITCH = 2
                Connection* connection = new Connection(port_width); //Output link so output ports
                int connection_pos_this_level = j*CONNECTIONS_PER_SWITCH+c; //number of switches alreay created + shift this switch
                std::pair<int, int> connectionpair (i, connection_pos_this_level);
                connectiontable[connectionpair] = connection;
                //Connecting switch with its connection
                if(c == LEFT) {  // 0
                    std::cout<<"left"<<std::endl;
                    ds->setLeftConnection(connection);
                }

                else if(c == RIGHT) {  // 1
                     ds->setRightConnection(connection);
                }

            }
            sw_id+=1; //Increasing id number to identify the ds
        
        }
        //In the next level, the input ports is the output ports of this level
        switches_this_level=switches_this_level * 2; 
    }

}

DSNetwork::~DSNetwork() {
    //Delete the switches from dswitchtable
    for(std::map<std::pair<int, int>, DSwitch*>::iterator it=dswitchtable.begin(); it != dswitchtable.end(); ++it) {
        delete it->second; 
    }
    
    //Removing connections from connectiontable
    for(std::map<std::pair<int, int>, Connection*>::iterator it=connectiontable.begin(); it != connectiontable.end(); ++it) {
        delete it->second;
    }
}

// 得到一个端口对应树的最后一级连接 
std::map<int, Connection*> DSNetwork::getLastLevelConnections() {
    int last_level_index = this->nlevels-1; //The levels start from 0, so in the table the last level is nlevels-1  // -1 
    std::map<int, Connection*> connectionsLastLevel; //Map with the connections of the last level of DS
    for(int i=0; i<this->ms_size; i++) {  //Each multiplier must have its own connection if the DSNetwork has been created correctly
        if(last_level_index>=0) { // i.e., num_ms > dn_bw
            std::pair<int,int> current_connection (last_level_index, i);
            connectionsLastLevel[i]=this->connectiontable[current_connection];
        }
        else {
            connectionsLastLevel[i]=this->inputConnection;  // 在TPU情况下，树只有一个节点也就是一级，last_level_index = 0-1 = -1
        }
    }
    return connectionsLastLevel;
}

// 模拟从根节点到叶节点的数据传输
// 从根节点开始，逐层调用每个节点的cycle()方法 
void DSNetwork::cycle() {
    //std::cout<<"111111111111111111111111"<<std::endl;
    // Iterate over each DS from the root to the leaves and from left to right. Each DSwitch will execute its cycle and will receive data from the top
    //and will send it to the lower level by using source routing. 

    //The order from root to leaves is important in terms of the correctness of the network.
    int switches_this_level = 1; //Only one switch in the root
    //Going down to the leaves (no count the MSs)
    for(int i=0; i < this->nlevels; i++) {
        std::cout<<"=================================="<<std::endl;
        for(int j=0; j < switches_this_level; j++) {
            std::pair<int,int> current_switch_pair (i,j);
            DSwitch* ds = dswitchtable[current_switch_pair];
            ds->cycle();  //executing the cycle for the current DS. 
        }
        switches_this_level = switches_this_level*2;    
    }

}

// 计算数据从根节点传递到叶子节点的时间 
unsigned long DSNetwork::get_time_routing() {
    //The order from root to leaves is important in terms of the correctness of the network.
    unsigned long time = 0;
    int switches_this_level = 1; //Only one switch in the root
    //Going down to the leaves (no count the MSs)
    for(int i=0; i < this->nlevels; i++) {
        for(int j=0; j < switches_this_level; j++) {
            std::pair<int,int> current_switch_pair (i,j);
            DSwitch* ds = dswitchtable[current_switch_pair];
            time+=ds->get_time_routing();  //executing the cycle for the current DS. 
        }
        switches_this_level = switches_this_level*2;
    }
    return time;
}

void DSNetwork::printStats(std::ofstream& out, unsigned int indent) {
    out << ind(indent) << "{" << std::endl; //Since it is inside an array we do not name the object
        //out << ind(indent+IND_SIZE) << "\"ms_size\" : " << this->ms_size  << std::endl; DSNetwork global parameters
        out << ind(indent+IND_SIZE) << "\"DSwitchStats\" : [" << std::endl;   //One entry per DSwitch
        int switches_this_level = 1;
        for(int i=0; i < this->nlevels; i++) {  //From root to leaves (without the MSs)
        //Calculating the output ports in this level
            //One array for each level will allow the access to the ASwitch easier
            out << ind(indent+IND_SIZE+IND_SIZE) << "[" << std::endl;
            for(int j=0; j < switches_this_level; j++) { // From left to right of the structure
                std::pair<int,int> current_switch_pair (i,j);
                DSwitch* ds = dswitchtable[current_switch_pair];
                ds->printStats(out, indent+IND_SIZE+IND_SIZE+IND_SIZE);
                if(j==(switches_this_level-1)) {  //If I am in the last switch of the level, the comma to separate the swes is not added
                    out << std::endl; //This is added because the call to ds print do not show it (to be able to put the comma, if neccesary)
                }
                else {
                    out << "," << std::endl; //Comma and line break are added to separate with the next ASwitch in the array of this level
                }

            }
            if(i==(this->nlevels-1)) { //If I am in the last level, the comma to separate the different levels is not added
                out << ind(indent+IND_SIZE+IND_SIZE) << "]" << std::endl;
            }
    
            else { //If I am not in the last level, then the comma is printed to separate with the next level
                out << ind(indent+IND_SIZE+IND_SIZE) << "]," << std::endl;
            }
            
            switches_this_level=switches_this_level*2;
        }

        
        out << ind(indent+IND_SIZE) << "]" << std::endl;
    out << ind(indent) << "}";
}

void DSNetwork::printEnergy(std::ofstream& out, unsigned int indent) {
  /*
       This component prints:
           - wires that connect every DSwitch with its child (include the wires that connect with the MSNetwork)
           - DSwitches
  */

  //Printing the wires
  for(std::map<std::pair<int, int>, Connection*>::iterator it=connectiontable.begin(); it != connectiontable.end(); ++it) {
        Connection* conn = connectiontable[it->first];
        conn->printEnergy(out, indent, "DN_WIRE");
  }

  //Printing the DSwitches
   for(std::map<std::pair<int, int>, DSwitch*>::iterator it=dswitchtable.begin(); it != dswitchtable.end(); ++it) {
        DSwitch* ds = dswitchtable[it->first];
        ds->printEnergy(out, indent);
  }


}

//
// Created by Francisco Munoz on 17/06/19.
//

#ifndef __DSNETWORK__H__
#define __DSNETWORK__H__

#include "MSNetwork.h"
#include "DSwitch.h"
#include "Unit.h"
#include <iostream>
#include <fstream>
#include <map>
#include "Config.h"


#define CONNECTIONS_PER_SWITCH 2
#define LEFT 0
#define RIGHT 1


class DSNetwork : public Unit{
private:
    unsigned int ms_size; //Number of multipliers. i.e., the leaves of the network 
    unsigned int port_width;
    int nlevels; //Number of levels of the DS without taking into account the MS level // 层数，不包含叶节点层
    // 保存网络中各个层的交换节点DSwitch对象，以<层数，节点编号>的键对存储
    std::map<std::pair<int, int>, DSwitch* > dswitchtable; //Map with the switches of the topology. The connection among them will be different depending on the topology used
    // 保存各层的输出连接对象 
    std::map<std::pair<int, int>, Connection*> connectiontable; // Outputs connections of each level. 
    // 整个网络的输入连接
    Connection* inputConnection;  //Given by external
    
    
public:
    DSNetwork(id_t id, std::string name, Config stonne_cfg, unsigned int ms_size, Connection* inputConnection); //ms_size = ms_size of the group that contain this tree
    ~DSNetwork();
    const int getNLevels()  const { return this->nlevels; } // 层数-1
    const int getMsSize()   const { return this->ms_size; }  // 叶子节点个数 
    // 返回最后一级的连接映射，用于与下一组件对接，如MSwitch
    std::map<int, Connection*> getLastLevelConnections();
    // 设置网络的输入连接
    void setInputConnection(Connection* inputConnection)  { this->inputConnection = inputConnection; } //This function set the inputConnection with the Prefetch buffer

    //Useful functions

    //Cycle function
    void cycle();  // 模拟网络中的一次数据传输，从根节点到叶节点逐层进行
    unsigned long get_time_routing();  // 计算数据从输入到输出经过整个网络的时间 
    void printStats(std::ofstream& out, unsigned int indent); //Print the stats of the component
    void printEnergy(std::ofstream& out, unsigned int indent);
    //void setConfiguration(Config cfg);

};

#endif 

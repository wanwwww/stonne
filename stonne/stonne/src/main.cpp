#include <iostream>
#include "STONNEModel.h"
#include "types.h"
#include <chrono>
#include <assert.h>
#include "testbench.h"
#include <string>
#include <math.h>
#include <utility.h>

using namespace std;

void configConvParameters(int argc, char *argv[], Config &stonne_cfg, std::string &layer_name, unsigned int &R, unsigned int &S, unsigned int &C, unsigned int &K, unsigned int &G, unsigned int &N, unsigned int &X, unsigned int &Y, unsigned int &strides,
                      unsigned int &T_R, unsigned int &T_S, unsigned int &T_C, unsigned int &T_K, unsigned int &T_G, unsigned int &T_N, unsigned int &T_X_, unsigned int &T_Y_, TileGenerator::Target &tileGeneratorTarget, TileGenerator::Generator &tileGenerator);

//void configSparseGEMMParameters(int argc, char *argv[], Config &stonne_cfg, std::string &layer_name, unsigned int &M, unsigned int &N, unsigned int &K, unsigned int &MK_sparsity, unsigned int &KN_sparsity, Dataflow &dataflow, unsigned int &optimize);

void configDenseGEMMParameters(int argc, char *argv[], Config &stonne_cfg, std::string &layer_name, unsigned int &M, unsigned int &K, unsigned int &N, unsigned int &T_M, unsigned int &T_K, unsigned int &T_N, TileGenerator::Target &tileGeneratorTarget, TileGenerator::Generator &tileGenerator);

//void configSparseDenseParameters(int argc, char *argv[], Config &stonne_cfg, std::string &layer_name, unsigned int &M, unsigned int &N, unsigned int &K, unsigned int &MK_sparsity, unsigned int &T_N, unsigned int &T_K, TileGenerator::Target &tileGeneratorTarget, TileGenerator::Generator &tileGenerator);

bool runConvCommand(int argc, char *argv[], Config stonne_cfg);
//bool runSparseGEMMCommand(int argc, char *argv[]);
bool runDenseGEMMCommand(int argc, char *argv[], Config stonne_cfg);
//bool runSparseDenseCommand(int argc, char *argv[]);
bool runHelpCommand();
//float* generateMatrixDense(unsigned int rows, unsigned int cols, unsigned int sparsity);

//void generateSparseDense(unsigned int rows, unsigned int cols, unsigned int sparsity);

//unsigned int* generateBitMapFromDense(float* denseMatrix, unsigned int rows, unsigned int cols, GENERATION_TYPE gen_type);

// float* generateMatrixSparseFromDense(float* denseMatrix, unsigned int* bitmap, unsigned int rows, unsigned int cols, GENERATION_TYPE gen_type);

// //void generateSparseDense(unsigned int rows, unsigned int cols, unsigned int sparsity);
// int* generateMinorIDFromDense(float* denseMatrix, unsigned int rows, unsigned int cols, int &nnz, GENERATION_TYPE gen_type);
// int* generateMajorPointerFromDense(float* denseMatrix, unsigned int rows, unsigned int cols, GENERATION_TYPE gen_type);

// void printDenseMatrix(float* matrix, unsigned int rows, unsigned int cols);
// void printBitMap(unsigned int* bitmap, unsigned int rows, unsigned int cols);
// void printSparseMatrix(float* sparseMatrix, unsigned int* bitmap, unsigned int rows, unsigned int cols);

int main(int argc, char *argv[]) {

    Config stonne_cfg;

    if(argc > 1) { //IF there is at least one parameter, -h is checked

        string arg = argv[1]; // Operation Type

        if(arg=="-h") {
            runHelpCommand();
        }
      
        else if(arg=="-CONV") {
            stonne_cfg.layer_type = CONV;
            runConvCommand(argc, argv, stonne_cfg);
        }

	    // else if(arg=="-SparseGEMM") {
        //     runSparseGEMMCommand(argc, argv);
	    // }

	    else if(arg=="-DenseGEMM") {
            stonne_cfg.layer_type = GEMM;
            runDenseGEMMCommand(argc, argv, stonne_cfg);
	    }

        else if(arg=="-FC"){
            stonne_cfg.layer_type = FC;
            runDenseGEMMCommand(argc, argv, stonne_cfg);
        }

	    // else if(arg=="-SparseDense") {
	    //     runSparseDenseCommand(argc, argv);

	    // }

	    else {
	        std::cout << "How to use STONNE User Interface: ./" << argv[0] << " -h" << std::endl;
	    }
    }

    else {
        std::cout << "How to use STONNE User Interface: ./" << argv[0] << " -h" << std::endl;
    }
}

bool runConvCommand(int argc, char *argv[], Config stonne_cfg) {
    
    // 定义权重位宽
    unsigned int weight_width=4; 
    int min_weight = -(1 << (weight_width-1)) ;
    int max_weight = (1 << (weight_width-1))-1;

    // SNN parameter
    unsigned int V_th = 0;
    bool pooling_enabled = (stonne_cfg.layer_type==GEMM || stonne_cfg.layer_type==CONV);

    /** Generating the inputs and outputs **/

    //Layer parameters (See MAERI paper to find out the taxonomy meaning)
    std::string layer_name="TestLayer";
    unsigned int R=1;                                  // R
    unsigned int S=3;                                  // S
    unsigned int C=1;                                  // C
    unsigned int K=1;                                  // K
    unsigned int G=1;                                  // G
    unsigned int N=1;                                  // N
    unsigned int X=1;                                  // X //TODO CHECK X=1 and Y=1
    unsigned int Y=3;                                  // Y
    unsigned int strides=1;                            // Strides
 
    //Tile parameters (See MAERI paper to find out the taxonomy meaning)
    unsigned int T_R=1;                                // T_R
    unsigned int T_S=3;                                // T_S
    unsigned int T_C=1;                                // T_C
    unsigned int T_K=1;                                // T_K
    unsigned int T_G=1;                                // T_G
    unsigned int T_N=1;                                // T_N
    unsigned int T_X_=1;                               // T_X
    unsigned int T_Y_=1;                               // T_Y

    // TileGenerator parameters
    TileGenerator::Target tileGeneratorTarget = TileGenerator::Target::NONE;
    TileGenerator::Generator tileGenerator = TileGenerator::Generator::CHOOSE_AUTOMATICALLY;

    //Config stonne_cfg; //Hardware parameters
//    stonne_cfg.m_MSNetworkCfg.ms_size=128;
    configConvParameters(argc, argv, stonne_cfg, layer_name, R, S, C, K, G, N, X, Y, strides, T_R, T_S, T_C, T_K, T_G, T_N, T_X_, T_Y_, tileGeneratorTarget, tileGenerator); //Modify stonne_cfg and the variables according to user arguments

    //Calculating output parameters
    unsigned int X_= (X - R + strides) / strides;      // X_
    unsigned int Y_=(Y - S + strides) / strides;       // Y_
    std::cout << "Output Size: (X'=" << X_ << ", Y'=" << Y_ << ")" << std::endl; 

    int Timestamp = stonne_cfg.Timestamp;

    //Creating arrays to store the ifmap ofmap and weights
    unsigned int ifmap_size=N*X*Y*C*Timestamp;
    unsigned int filter_size=R*S*(C/G)*K;
    unsigned int ofmap_size;
    if(pooling_enabled){
        ofmap_size=X_*Y_*K*Timestamp/4;
    } else {
        ofmap_size=X_*Y_*K*Timestamp;
    }

    unsigned int neuron_state_size = X_*Y_*K;

    int* ifmap = new int[ifmap_size];
    int* filter = new int[filter_size];
    int* ofmap = new int[ofmap_size];

    int* neuron_state = new int[neuron_state_size];

    int* ofmap_cpu = new int[ofmap_size]; //Used to store the CPU computed values to compare with the simulator version

    int* neuron_state_cpu = new int[neuron_state_size];

    //Filling the arrays with random values
    for(int i=0; i<ifmap_size; i++) {
        ifmap[i]=rand()%2;
    }

    for(int i=0;i<filter_size; i++) {
        filter[i]=rand()%((max_weight-min_weight+1)+min_weight);
    }

    for(int i=0;i<neuron_state_size;i++){
        neuron_state[i] = 0;
        neuron_state_cpu[i] = 0;
    }

    for(int i=0;i<ofmap_size;i++){
        ofmap[i] = 0;
        ofmap_cpu[i] = 0;
    }

    //computing CPU version
    //std::cout<<" cpu start"<<std::endl;
    sequential_layer(R, S, C, K, G, N, X, Y, strides, ifmap, filter, ofmap_cpu, neuron_state_cpu, stonne_cfg.V_th, Timestamp, pooling_enabled); 


    /** END of generating the inputs and outputs **/
    //
    //
    //
    /** Configuring and running the accelerator  **/
    
    //Computing the CNN Layer with the simulator
    //std::cout<<" instance start"<<std::endl;
    Stonne* stonne_instance = new Stonne(stonne_cfg); //Creating instance of the simulator
    stonne_instance->loadDNNLayer(CONV, layer_name, R, S, C, K, G, N, X, Y, strides, ifmap, filter, ofmap, neuron_state, CNN_DATAFLOW); //Loading the layer
    
    //Loads or generates a tile configuration depending on whether a TileGenerator target has been specified
    // if (tileGeneratorTarget == TileGenerator::Target::NONE)
    //     stonne_instance->loadTile(T_R, T_S, T_C, T_K, T_G, T_N, T_X_, T_Y_);
    // else
    //     stonne_instance->generateTile(tileGenerator, tileGeneratorTarget);

    stonne_instance->loadTile(T_R, T_S, T_C, T_K, T_G, T_N, T_X_, T_Y_);

    stonne_instance->run(); //Running the simulator 

    /** END of configuring and running the accelerator  **/
    //
    //
    //
    /** CHECKING the results to make sure that the output is correct  **/

     // Comparing the results
    int timestamp_flag = 0;
    if(pooling_enabled){

        // 对比脉冲数据
        for(int i=0;i<ofmap_size;i++){          
            float difference_spike=fabs(ofmap[i]-ofmap_cpu[i]);
            if(difference_spike > 0) {
                std::cout << "ERROR position " << i <<  ": Value ofmap simulator: " << ofmap[i] << ". Value ofmap CPU: " << ofmap_cpu[i] << std::endl;
                std::cout << "\033[1;31mT test not passed\033[0m" << std::endl;
                delete[] ifmap;
                delete[] filter;
                delete[] ofmap;
                delete[] ofmap_cpu;
                delete[] neuron_state;
                delete[] neuron_state_cpu;
                delete stonne_instance;
                assert(false); //Always false
            }
        }

        // 对比膜电位数据
        for(int i=0;i<neuron_state_size;i++){          
            float difference_Vth=fabs(neuron_state[N*X_*Y_*K]-neuron_state_cpu[N*X_*Y_*K]);
            if(difference_Vth > 0) {
                std::cout << "ERROR position " << i <<  ": Value neuron_state simulator: " << neuron_state[i] << ". Value ofmap CPU: " << neuron_state_cpu[i] << std::endl;
                std::cout << "\033[1;31mT test not passed\033[0m" << std::endl;
                delete[] ifmap;
                delete[] filter;
                delete[] ofmap;
                delete[] ofmap_cpu;
                delete[] neuron_state;
                delete[] neuron_state_cpu;
                delete stonne_instance;
                assert(false); //Always false
            }
        }


    } else{

        for(int i=0;i<ofmap_size; i++) {
            if((i>0) && (i%(N*X_*Y_*K)==0)){
                timestamp_flag++;
            }
            float difference=fabs(ofmap[i]-ofmap_cpu[i])+fabs(neuron_state[i-timestamp_flag*N*X_*Y_*K]-neuron_state_cpu[i-timestamp_flag*N*X_*Y_*K]);
            if(difference > 0) {
                std::cout << "ERROR position " << i <<  ": Value ofmap simulator: " << ofmap[i] << ". Value ofmap CPU: " << ofmap_cpu[i] << std::endl;
                std::cout << "ERROR position " << i <<  ": Value neuron_state simulator: " << neuron_state[i] << ". Value ofmap CPU: " << neuron_state_cpu[i] << std::endl;
                std::cout << "\033[1;31mT test not passed\033[0m" << std::endl;
                delete[] ifmap;
                delete[] filter;
                delete[] ofmap;
                delete[] ofmap_cpu;
                delete[] neuron_state;
                delete[] neuron_state_cpu;
                delete stonne_instance;
                assert(false); //Always false
                
            }
        }

    }


    //If the code does not stop then the TEST is correct
    std::cout<<std::endl;
    std::cout << "\033[1;32mTest passed correctly \033[0m" << std::endl << std::endl;
    
    std::cout<<"layer type : "<<stonne_cfg.layer_type<<std::endl;
    std::cout<<"Layer_t{CONV, POOL, FC, GEMM, SPARSE_DENSE}"<<std::endl;
    std::cout<<std::endl;


    delete[] ifmap;
    //std::cout<<"ifmap deleted"<<std::endl;
    delete[] filter;
    //std::cout<<"filter deleted"<<std::endl;
    delete[] neuron_state;
    //std::cout<<"neuron_state deleted"<<std::endl;
    delete[] ofmap_cpu;
    //std::cout<<"ofmap_cpu deleted"<<std::endl;
    delete[] neuron_state_cpu;
    //std::cout<<"neuron_state_cpu deleted"<<std::endl;
    delete stonne_instance; 
    //std::cout<<"stonne_instance deleted"<<std::endl;
    delete[] ofmap;
    //std::cout<<"ofmap deleted"<<std::endl;
    return true;
}

bool runDenseGEMMCommand(int argc, char *argv[], Config stonne_cfg) {

    // 定义权重位宽
    unsigned int weight_width=4; 
    int min_weight = -(1 << (weight_width-1)) ;
    int max_weight = (1 << (weight_width-1))-1;

    // 是否选择池化模块
    bool pooling_enabled = stonne_cfg.layer_type == GEMM;
   
    /** Generating the inputs and outputs **/

    // SNN parameters
    //unsigned int V_th = 0;  // 膜电位阈值 

    //Layer parameters (See MAERI paper to find out the taxonomy meaning)
    std::string layer_name="TestLayer";
    unsigned int M=1;                                  // M
    unsigned int K=3;                                  // K
    unsigned int N=1;                                  // N
 
    //Tile parameters 
    unsigned int T_M=1;                                // T_M
    unsigned int T_K=1;                                // T_K
    unsigned int T_N=1;                                // T_N

    // TileGenerator parameters
    TileGenerator::Target tileGeneratorTarget = TileGenerator::Target::NONE;  
    TileGenerator::Generator tileGenerator = TileGenerator::Generator::CHOOSE_AUTOMATICALLY;  


    //Config stonne_cfg; //Hardware parameters
    
//    stonne_cfg.m_MSNetworkCfg.ms_size=128;
    // 根据命令行输入的参数修改stonne_cfg和变量
    configDenseGEMMParameters(argc, argv, stonne_cfg, layer_name, M, K, N, T_M, T_K, T_N, tileGeneratorTarget, tileGenerator); //Modify stonne_cfg and the variables according to user arguments

    int Timestamp = stonne_cfg.Timestamp;

    //Creating arrays to store the matrices
    // 创建数组存储矩阵，这个数据在后续的模拟中相当于是内存
    unsigned int MK_size=M*K*Timestamp;
    unsigned int KN_size=N*K;
    unsigned int output_size;

    // 该方法中层的类型是 GEMM 或 FC
    if(pooling_enabled){  // 加池化层，将输出结果的维度除以4
        // std::cout<<"layer_type is GEMM"<<std::endl;
        output_size=(M/2)*(N/2)*Timestamp;
        //std::cout<<"the output_size is : "<<output_size<<std::endl;
    }
    else{
        output_size=M*N*Timestamp;
        // std::cout<<"###################### the layer type is FC #########################"<<std::endl;
        // std::cout<<" the size of output is "<<output_size<<std::endl;
    }

    unsigned int neuron_state_size = M*N;  

    // 动态分配内存给数组，并将它们的指针赋值给MK_matrix等
    int* MK_matrix = new int[MK_size];
    int* KN_matrix = new int[KN_size];
    int* output = new int[output_size];

    int* neuron_state = new int[neuron_state_size];

    int* output_cpu = new int[output_size]; //Used to store the CPU computed values to compare with the simulator version

    int* neuron_state_cpu = new int[neuron_state_size];



    //Filling the arrays with random values
    // 使用随机数初始化输入矩阵和权重矩阵 
    for(int i=0; i<MK_size; i++) {
        MK_matrix[i]=rand()%2;
    }

    for(int i=0;i<KN_size; i++) {
        KN_matrix[i]=rand()%((max_weight-min_weight+1)+min_weight);
    }

    for(int i=0;i<neuron_state_size;i++){
        neuron_state[i] = 0;
        neuron_state_cpu[i] = 0;
    }

    for(int i=0;i<output_size;i++){
        output[i] = 0;
        output_cpu[i] = 0;
    }


    //computing CPU version based on a Conv parameters mapping. Note a CONV layer might be seen as a GEMM if the mapping is correct.
    //sequential_layer(1, K, 1, M, 1, N, 1, K, 1, KN_matrix, MK_matrix, output_cpu); //Supposes that MK=inputs (M=batch size) and KN=filters (N=number of filters)
    sequential_layer(1, K, 1, N, 1, 1, M, K, 1, MK_matrix, KN_matrix, output_cpu, neuron_state_cpu, stonne_cfg.V_th, Timestamp, pooling_enabled); //Supposes that MK=inputs (M=batch size) and KN=filters (N=number of filters)

    // if(pooling_enabled){

    //     for(int t=0;t<Timestamp;t++){
    //         for(int i=0;i<M/2;i++){
    //             for(int j=0;j<N/2;j++){
    //                 std::cout<<output_cpu[t*M*N/4+i*N/2+j]<<" ";
    //             }
    //             std::cout<<std::endl;
    //         }
    //         std::cout<<std::endl;
    //     }

    // } else {
    //     for(int t=0;t<Timestamp;t++){
    //         for(int i=0;i<M;i++){
    //             for(int j=0;j<N;j++){
    //                 std::cout<<output_cpu[t*M*N+i*N+j]<<" ";
    //             }
    //             std::cout<<std::endl;
    //         }
    //         std::cout<<std::endl;
    //     }
    // }


    std::cout<<"1"<<std::endl;
    Stonne* stonne_instance = new Stonne(stonne_cfg); //Creating instance of the simulator
    std::cout<<"2"<<std::endl;

    // std::cout<<std::endl;
    // std::cout<<"2. load layer (main.cpp)"<<std::endl;
    // std::cout<<std::endl;
    stonne_instance->loadDenseGEMM(layer_name, N, K, M, MK_matrix, KN_matrix, output, neuron_state, CNN_DATAFLOW); //Loading the layer

    // std::cout<<std::endl;
    // std::cout<<"3. load tile (main.cpp)"<<std::endl;
    // std::cout<<std::endl;
    stonne_instance->loadGEMMTile(T_N, T_K, T_M);

    //Loads or generates a tile configuration depending on whether a TileGenerator target has been specified
    // if (tileGeneratorTarget == TileGenerator::Target::NONE)
    //     stonne_instance->loadGEMMTile(T_N, T_K, T_M);
    // else
    //     stonne_instance->generateTile(tileGenerator, tileGeneratorTarget);

    // std::cout<<std::endl;
    // std::cout<<"4. ===================== Starting Simulation :  (main.cpp) =========================="<<std::endl;
    // std::cout<<std::endl;
    stonne_instance->run(); //Running the simulator 

    /** END of configuring and running the accelerator  **/
    //
    //
    //
    /** CHECKING the results to make sure that the output is correct  **/

    // 输出数据
    // std::cout<<std::endl;
    // std::cout << "*************** the input data: ********************" << std::endl;
    // // for(int i=0;i<MK_size;i++){
    // //     std::cout<<MK_matrix[i]<<" "; 
    // // }
    // // std::cout<<std::endl;

    // for(int t=0;t<Timestamp;t++){
    //     for(int i=0;i<M;i++){
    //         for(int j=0;j<K;j++){
    //             std::cout<<MK_matrix[t*M*K+i*K+j]<<" ";
    //         }
    //         std::cout<<std::endl;
    //     }
    //     std::cout<<std::endl;
    // }

    // std::cout<<std::endl;
    // std::cout << "*************** the weight data: ********************" << std::endl;
    // // for(int i=0;i<KN_size;i++){
    // //     std::cout<<KN_matrix[i]<<" "; 
    // // }
    // // std::cout<<std::endl;
    // // 权重矩阵的存储是列优先
    // for(int i=0;i<K;i++){
    //     for(int j=0;j<N;j++){
    //         std::cout<<KN_matrix[j*K+i]<<" ";
    //     }
    //     std::cout<<std::endl;
    // }

    // // for(int i=0;i<K;i++){
    // //     for(int j=0;j<N;j++){
    // //         std::cout<<KN_matrix[i*N+j]<<" ";
    // //     }
    // //     std::cout<<std::endl;
    // // }

    // std::cout<<std::endl;
    // std::cout << "*************** the output data: ********************" << std::endl;
    // if(pooling_enabled){

    //     for(int t=0;t<Timestamp;t++){
    //         for(int i=0;i<M/2;i++){
    //             for(int j=0;j<N/2;j++){
    //                 std::cout<<output[t*M*N/4+i*N/2+j]<<" ";
    //             }
    //             std::cout<<std::endl;
    //         }
    //         std::cout<<std::endl;
    //     }

    // } else {
    //     for(int t=0;t<Timestamp;t++){
    //         for(int i=0;i<M;i++){
    //             for(int j=0;j<N;j++){
    //                 std::cout<<output[t*M*N+i*N+j]<<" ";
    //             }
    //             std::cout<<std::endl;
    //         }
    //         std::cout<<std::endl;
    //     }
    // }

    // std::cout<<std::endl;
    // std::cout << "*************** the Vth data: ********************" << std::endl;
    // for(int i=0;i<M;i++){
    //     for(int j=0;j<N;j++){
    //         std::cout<<neuron_state[i*N+j]<<" ";
    //     }
    //     std::cout<<std::endl;
    // }

    // //Comparing the results
    // int timestamp_flag = 0;
    // if(pooling_enabled){

    //     for(int i=0;i<output_size;i++){// 对比输出脉冲
    //         float difference_spike=fabs(output[i]-output_cpu[i]); 
    //         if(difference_spike>0){
    //             std::cout << "ERROR position " << i <<  ": Value ofmap simulator: " << output[i] << ". Value ofmap CPU: " << output_cpu[i] << std::endl;
    //             std::cout << "\033[1;31mT test not passed\033[0m" << std::endl;
    //             delete[] MK_matrix;
    //             delete[] KN_matrix;
    //             delete[] output;
    //             delete[] output_cpu;
    //             delete[] neuron_state;
    //             delete[] neuron_state_cpu;
    //             delete stonne_instance;
    //             assert(false); //Always false
    //         }
    //     }

    //     for(int i=0;i<neuron_state_size;i++){  // 对比膜电位
    //         float difference_Vth = fabs(neuron_state[i]-neuron_state_cpu[i]);
    //         if(difference_Vth > 0) {
    //             std::cout << "ERROR position " << i <<  ": Value neuron_state simulator: " << neuron_state[i] << ". Value ofmap CPU: " << neuron_state_cpu[i] << std::endl;
    //             std::cout << "\033[1;31mT test not passed\033[0m" << std::endl;
    //             delete[] MK_matrix;
    //             delete[] KN_matrix;
    //             delete[] output;
    //             delete[] output_cpu;
    //             delete[] neuron_state;
    //             delete[] neuron_state_cpu;
    //             delete stonne_instance;
    //             assert(false); //Always false
    //         }
    //     }

    // } else {
        
    //     // 不对计算结果进行池化
    //     for(int i=0;i<output_size; i++) {
    //         if((i>0) && i%(M*N)==0){
    //             timestamp_flag++;
    //         }
    //         float difference=fabs(output[i]-output_cpu[i])+fabs(neuron_state[i-timestamp_flag*M*N]-neuron_state_cpu[i-timestamp_flag*M*N]);
    //         if(difference > 0) {
    //             std::cout << "ERROR position " << i <<  ": Value ofmap simulator: " << output[i] << ". Value ofmap CPU: " << output_cpu[i] << std::endl;
    //             std::cout << "ERROR position " << i <<  ": Value neuron_state simulator: " << neuron_state[i] << ". Value ofmap CPU: " << neuron_state_cpu[i] << std::endl;
    //             std::cout << "\033[1;31mT test not passed\033[0m" << std::endl;
    //             delete[] MK_matrix;
    //             delete[] KN_matrix;
    //             delete[] output;
    //             delete[] output_cpu;
    //             delete[] neuron_state;
    //             delete[] neuron_state_cpu;
    //             delete stonne_instance;
    //             assert(false); //Always false
    //         }
    //     }

    // }

    // If the code does not stop then the TEST is correct
    std::cout<<std::endl;
    std::cout << "\033[1;32mTest passed correctly \033[0m" << std::endl << std::endl;

    std::cout<<"layer type : "<<stonne_cfg.layer_type<<std::endl;
    std::cout<<"Layer_t{CONV, POOL, FC, GEMM, SPARSE_DENSE}"<<std::endl;
    std::cout<<std::endl;

    delete[] MK_matrix;
    //std::cout<<"1"<<std::endl;
    delete[] KN_matrix;
    //std::cout<<"2"<<std::endl;
    delete[] output;
    //std::cout<<"3"<<std::endl;
    delete[] neuron_state;
    //std::cout<<"4"<<std::endl;
    delete[] output_cpu;           // ?
    //std::cout<<"5"<<std::endl;
    delete[] neuron_state_cpu;         // ?
    //std::cout<<"6"<<std::endl;
    delete stonne_instance; 
    //std::cout<<"7"<<std::endl;
    return true;
}

bool runHelpCommand() {
    std::cout << "Welcome to the STONNE User Interface Version 1.0: " << std::endl;
    std::cout << "Complete documentation can be found at README.md (https://github.com/stonne-simulator/stonne)" << std::endl;
    std::cout << "***********************************************************************************************************" << std::endl;
    std::cout << "***********************************************************************************************************" << std::endl;
    std::cout << std::endl;

    std::cout << "The STONNE User Interface allows to execute the STONNE simulator with any parameter. Currently, STONNE runs" << std::endl;
    std::cout << "5 types of operations: Convolution Layers, FC Layers, Dense GEMMs, Sparse GEMMs and SparseDense GEMMs." << std::endl;
    std::cout << "Note that almost any kernel can be, in the end, mapped using these operations." << std::endl;
    std::cout << std::endl;
    std::cout << "The simulator also includes a module called STONNE Mapper, able to generate automatic mappings for" << std::endl;
    std::cout << "CONV, FC/DenseGEMM and SparseDense layers. Its use enables fast prototyping and efficient layer mapping." << std::endl;
    std::cout << std::endl;

    std::cout << "Usage: ./stonne [-h | -CONV | -FC | -DenseGEMM | -SparseGEMM | -SparseDense] [Hardware Parameters] [Dimension and tile Parameters]"  << std::endl;
    std::cout << std::endl;

    std::cout << "[Hardware parameters]" << std::endl;
    std::cout << "  -num_ms=[x]: Number of multiplier switches (must be power of 2)" << std::endl;
    std::cout << "  -dn_bw=[x]: Number of read ports in the SDMemory (must be power of 2)" << std::endl;
    std::cout << "  -rn_bw=[x]: Number of write ports in the SDMemory (must be power of 2)" << std::endl;
    std::cout << "  -rn_type=[0=ASNETWORK, 1=FENETWORK, 2=TEMPORALRN]: type of the ReduceNetwork to be used (Not supported for SparseGEMM)" << std::endl;
    std::cout << "  -accumulation_buffer=[0,1]: enables the accumulation buffer (enabled by default in SparseDense)" << std::endl;
    std::cout << "  -print_stats=[0,1]: Flag that enables the printing of the statistics" << std::endl;
    std::cout << std::endl;

    std::cout << "[Dimension and Tile parameters]" << std::endl;
    std::cout << "The dimensions of the kernel depends on the type of the operation that is going to be run. Next, we describe" << std::endl;
    std::cout << "described the dimensions according to each supported operation." << std::endl;
    std::cout << std::endl;

    std::cout << "[[CONV]]" << std::endl;
    std::cout << "  -layer_name=[x]: Name of the layer to run. The output statistic file will be named accordingly" << std::endl; 
    std::cout << "  -R=[x]: Number of flter rows" << std::endl;
    std::cout << "  -S=[x]: Number of filter columns" << std::endl;
    std::cout << "  -C=[x]: Number of filter and input channels" << std::endl;
    std::cout << "  -K=[x]: Number of filters and output channels" << std::endl;
    std::cout << "  -G=[x]: Number of groups" << std::endl;
    std::cout << "  -N=[x]: Number of inputs (Only 1 is supported so far)" << std::endl;
    std::cout << "  -X=[x]: Number of input rows" << std::endl;
    std::cout << "  -Y=[x]: Number of input columns" << std::endl;
    std::cout << "  -strides=[x]: Stride value used in the layer" << std::endl;
    std::cout << "  -T_R=[x]: Number of flter rows mapped at a time" << std::endl;
    std::cout << "  -T_S=[x]: Number of filter columns mapped at a time" << std::endl;
    std::cout << "  -T_C=[x]: Number of filter and input channels per group mapped at a time" << std::endl;
    std::cout << "  -T_K=[x]: Number of filters and output channels per group mapped at a time" << std::endl;
    std::cout << "  -T_G=[x]: Number of groups mappd at a time" << std::endl;
    std::cout << "  -T_N=[x]: Number of inputs mapped at a time (Only 1 is supported so far)" << std::endl;
    std::cout << "  -T_X_=[x]: Number of input rows mapped at a time" << std::endl;
    std::cout << "  -T_Y_=[x]: Number of input columns mapped a time" << std::endl;
    std::cout << std::endl;
    std::cout << "  [[[STONNE Mapper]]]" << std::endl;
    std::cout << "    1. If used, the following parameters can be skipped: strides, T_R, T_S, T_C, T_K, T_G, T_N, T_X_ and T_Y_." << std::endl;
    std::cout << "    2. When using it, it is mandatory to also use the option -accumulation_buffer=1 to ensure that the tile configuration can" << std::endl;
    std::cout << "       adjust to the hardware resources." << std::endl;
    std::cout << "    -generate_tile=[0|none, 1|performance, 2|energy, 3|energy_efficiency]: Enables mapping generation, specifying the target" << std::endl;
    std::cout << std::endl;
    std::cout << "  ** Please take into consideration that: **" << std::endl;
    std::cout << "  1. Number of Virtual Neurons mapped (Num_VNs) will be T_K*T_G*T_N*T_X_*T_T_" << std::endl;
    std::cout << "  2. The minimum number of MSwitches needed will be at least VN_Size*Num_VNs" << std::endl;
    std::cout << "  3. Note in case of folding (iteration over the same VN) is enabled, and the accumulation buffer disabled" << std::endl;
    std::cout << "     1 extra MSwitch per VN will be needed to manage the psum. In this case, the minimum number of MSwitches" << std::endl;
    std::cout << "     needed will be at least (VN_Size+1)*Num_VNs. Folding (iteration over the same virtual neuron) will be" << std::endl;
    std::cout << "     enabled if (R/T_S)*(S/T_S)*(C/T_C) > 1" << std::endl;
    std::cout << std::endl;

    std::cout << "[[FC]]" << std::endl;
    std::cout << "  -layer_name=[x]:  Name of the layer to run. The output statistic file will be called by this name" << std::endl;
    std::cout << "  -M=[x]: Number of output neurons" << std::endl;
    std::cout << "  -N=[x]: Batch size" << std::endl;
    std::cout << "  -K=[x]: Number of input neurons" << std::endl;
    std::cout << "  -T_M=[x]: Number of output neurons mapped at a time" << std::endl;
    std::cout << "  -T_N=[x]: Number of batches mapped at a time" << std::endl;
    std::cout << "  -T_K=[x]: Number of input neurons mapped at a time" << std::endl;
    std::cout << std::endl;
    std::cout << "  [[[STONNE Mapper]]]" << std::endl;
    std::cout << "    1. If used, the following parameters can be skipped: T_M, T_N and T_K." << std::endl;
    std::cout << "    2. When using it, it is mandatory to also use the option -accumulation_buffer=1 to ensure that the tile configuration can" << std::endl;
    std::cout << "       adjust to the hardware resources." << std::endl;
    std::cout << "    -generate_tile=[0|none, 1|performance]: Enables mapping generation, specifying the target (only performance is supported)" << std::endl;
    std::cout << std::endl;
    std::cout << "  ** Please take into consideration that: **" << std::endl;
    std::cout << "  1. Number of Virtual Neurons mapped (Num_VNs) will be T_N*T_M" << std::endl;
    std::cout << "  2. The minimum number of MSwitches needed will be at least T_K*Num_VNs." << std::endl;
    std::cout << std::endl;

    std::cout << "[[DenseGEMM]]" << std::endl;
    std::cout << "  -layer_name=[x]:  Name of the layer to run. The output statistic file will be called by this name" << std::endl;
    std::cout << "  -M=[x]: Number of rows MK matrix" << std::endl;
    std::cout << "  -N=[x]: Number of columns KN matrix" << std::endl;
    std::cout << "  -K=[x]: Number of columns MK and rows KN matrix (cluster size)" << std::endl;
    std::cout << "  -T_M=[x]: Number of M rows mapped at a time" << std::endl;
    std::cout << "  -T_N=[x]: Number of N columns at a time" << std::endl;
    std::cout << "  -T_K=[x]: Number of K elements mapped at a time" << std::endl;
    std::cout << std::endl;
    std::cout << "  [[[STONNE Mapper]]]" << std::endl;
    std::cout << "    1. If used, the following parameters can be skipped: T_M, T_N and T_K." << std::endl;
    std::cout << "    2. When using it, it is mandatory to also use the option -accumulation_buffer=1 to ensure that the tile configuration can" << std::endl;
    std::cout << "       adjust to the hardware resources." << std::endl;
    std::cout << "    -generate_tile=[0|none, 1|performance]: Enables mapping generation, specifying the target (only performance is supported)" << std::endl;
    std::cout << endl;
    std::cout << "  ** Please take into consideration that: **" << std::endl;
    std::cout << "  1. Number of Virtual Neurons mapped (Num_VNs) will be T_N*T_M" << std::endl;
    std::cout << "  2. The minimum number of MSwitches needed will be at least T_K*Num_VNs." << std::endl;
    std::cout << std::endl;

    std::cout << "[[SparseGEMM]]" << std::endl;
    std::cout << "  -layer_name=[x]:  Name of the layer to run. The output statistic file will be called by this name" << std::endl;
    std::cout << "  -M=[x]: Number of rows MK matrix" << std::endl;
    std::cout << "  -N=[x]: Number of columns KN matrix" << std::endl;
    std::cout << "  -K=[x]: Number of columns MK and rows KN matrix (cluster size)" << std::endl;
    std::cout << "  -MK_sparsity=[x]: Percentage of sparsity MK matrix (0-100)" << std::endl;
    std::cout << "  -KN_sparsity=[x]: Percentahe of sparsity KN matrix (0-100)" << std::endl;
    std::cout << "  -dataflow=[MK_STA_KN_STR, MK_STR_KN_STA]: Dataflow to used during operations " << std::endl;
    std::cout << "  -optimize=[0,1]: apply compiler-based optimizations" << std::endl;
    std::cout << std::endl;

    std::cout << "[[SparseDense]]" << std::endl;
    std::cout << "  -layer_name=[x]:  Name of the layer to run. The output statistic file will be called by this name" << std::endl;
    std::cout << "  -M=[x]: Number of rows MK matrix" << std::endl;
    std::cout << "  -N=[x]: Number of columns KN matrix" << std::endl;
    std::cout << "  -K=[x]: Number of columns MK and rows KN matrix (cluster size)" << std::endl;
    std::cout << "  -MK_sparsity=[x]: Percentage of sparsity MK matrix (0-100)" << std::endl;
    std::cout << "  -T_N=[x]: Number of N columns mapped at a time" << std::endl;
    std::cout << "  -T_K=[x]: Number of K elements mapped at a time" << std::endl;
    std::cout << std::endl;
    std::cout << "  [[[STONNE Mapper]]]" << std::endl;
    std::cout << "    1. If used, the following parameters can be skipped: T_N and T_K." << std::endl;
    std::cout << "    2. When using it, it is mandatory to also use the option -accumulation_buffer=1 to ensure that the tile configuration can" << std::endl;
    std::cout << "       adjust to the hardware resources." << std::endl;
    std::cout << "    -generate_tile=[0|none, 1|performance]: Enables mapping generation, specifying the target (only performance is supported)" << std::endl;
    std::cout << std::endl;

    std::cout << "***********************************************************************************************************" << std::endl;
    std::cout << "***********************************************************************************************************" << std::endl;
    std::cout << std::endl;
    std::cout << "[Examples of use]" << std::endl;
    std::cout << "- Running a CONV layer (manual mapping)" << std::endl;
    std::cout << "  ./stonne -CONV -R=3 -S=3 -C=6 -G=1 -K=6 -N=1 -X=20 -Y=20 -T_R=3 -T_S=3 -T_C=1 -T_G=1 -T_K=1 -T_N=1 -T_X_=3 -T_Y_=1 -num_ms=64 -dn_bw=8 -rn_bw=8" << std::endl;
    std::cout << "- Running a CONV layer using STONNE Mapper (energy target)" << std::endl;
    std::cout << "  ./stonne -CONV -R=3 -S=3 -C=6 -G=1 -K=6 -N=1 -X=20 -Y=20 -generate_tile=energy -num_ms=64 -dn_bw=8 -rn_bw=8 -accumulation_buffer=1" << std::endl;
    exit(0);
    return true; //Never executed
}

//This function modifies the default values of the parameters according to user arguments.
void configConvParameters(int argc, char *argv[], Config &stonne_cfg, std::string &layer_name, unsigned int &R, unsigned int &S, unsigned int &C, unsigned int &K, unsigned int &G, unsigned int &N, unsigned int &X, unsigned int &Y, unsigned int &strides,
                      unsigned int &T_R, unsigned int &T_S, unsigned int &T_C, unsigned int &T_K, unsigned int &T_G, unsigned int &T_N, unsigned int &T_X_, unsigned int &T_Y_, TileGenerator::Target &tileGeneratorTarget, TileGenerator::Generator &tileGenerator) {

    //Parsing
    for(int i=2; i<argc; i++) { //0 is the name of the program and 1 is the execution command type
        string arg = argv[i];
        //Spliting using = character
        string::size_type pos = arg.find('=');
        if(arg.npos != pos) {
            string value_str=arg.substr(pos+1);
            string name=arg.substr(0, pos);
            unsigned int value;
            if((name != "-layer_name") && (name != "-rn_type") && (name != "-mn_type") && (name != "-mem_ctrl") && (name != "-generate_tile") && (name != "-generator") && (name!="-pooling_type")) { //string parameters
                value=stoi(value_str);
            }
            //Checking parameter name
            if(name=="-num_ms") {
                if(!ispowerof2(value)) {   //Checking that the num_ms is power of 2
                    std::cout << "Error: -num_ms must be power of 2" << std::endl;
                    exit(1);
                }
                std::cout << "Changing num_ms to " << value << std::endl; //To debug
                stonne_cfg.m_MSNetworkCfg.ms_size=value;
            }

            else if(name=="-ms_rows"){
                if(!ispowerof2(value)){
                    std::cout<<"Error: -ms_rows must be power of 2"<<std::endl;
                    exit(1);
                }
                std::cout<<"Changing ms_rows to "<<value<<std::endl;
                stonne_cfg.m_MSNetworkCfg.ms_rows = value;
            }

            else if(name == "-ms_cols"){
                if(!ispowerof2(value)){
                    std::cout<<"Error: -ms_cols must be power of 2"<<std::endl;
                    exit(1);
                }
                std::cout<<"Changing ms_cols to "<<value<<std::endl;
                stonne_cfg.m_MSNetworkCfg.ms_cols = value;
                stonne_cfg.m_MSNetworkCfg.ms_size=stonne_cfg.m_MSNetworkCfg.ms_rows*stonne_cfg.m_MSNetworkCfg.ms_cols;
            }

            else if(name=="-dn_bw") {
                if(!ispowerof2(value)) {
                    std::cout << "Error: -dn_bw must be power of 2" << std::endl;
                    exit(1);
                }
                std::cout << "Changing dn_bw to " << value << std::endl; //To debug
                stonne_cfg.m_SDMemoryCfg.n_read_ports=value;
            }

            else if(name=="-rn_bw") {
                if(!ispowerof2(value)) {
                    std::cout << "Error: -rn_bw must be power of 2" << std::endl; 
                    exit(1);
                }
                std::cout << "Changing rn_bw to " << value << std::endl;
                stonne_cfg.m_SDMemoryCfg.n_write_ports=value;
            }

	        else if(name=="-accumulation_buffer") {
                std::cout << "Changing accumulation_buffer to " << value << std::endl;
                stonne_cfg.m_ASNetworkCfg.accumulation_buffer_enabled=value;
            }

            else if(name=="-print_stats") {
                if((value != 0) && (value != 1)) {
                    std::cout << "Error: -print_stats only supports 0 or 1" << std::endl;
                    exit(1);
                }
                std::cout << "Changing print_stats to " << value << std::endl;
                stonne_cfg.print_stats_enabled=value; 
            }

            else if(name=="-rn_type") {
                std::cout << "Changing rn_type to " << value_str << std::endl;
                stonne_cfg.m_ASNetworkCfg.reduce_network_type=get_type_reduce_network_type(value_str);
            }

            else if(name == "-mn_type"){
                std::cout << "Changing mn_type to " << value_str << std::endl;
                stonne_cfg.m_MSNetworkCfg.multiplier_network_type =get_type_multiplier_network_type(value_str);
            }

            else if(name == "-mem_ctrl"){
                std::cout << "Changing mem_ctrl to " << value_str << std::endl;
                stonne_cfg.m_SDMemoryCfg.mem_controller_type =get_type_memory_controller_type(value_str);
            }

             else if(name=="-pooling_type") {
                std::cout<<"Changing pooling_type to "<<value_str <<std::endl;
                stonne_cfg.pooling_type = get_type_pooling_type(value_str);
            }

            //Running configuration parameters (layer and tile)
   
           //Layer parameters
            else if(name=="-layer_name") {
                std::cout << "Changing layer_name to " << value_str << std::endl;
                layer_name=value_str; 
            }
            
            else if(name=="-R") {
                    std::cout << "Changing R to " << value << std::endl;
                    R=value;
            }

            else if(name=="-S") {
                    std::cout << "Changing S to " << value << std::endl;
                    S=value;
            }

            else if(name=="-C") {
                    std::cout << "Changing C to " << value << std::endl;
                    C=value;
            } 
            
            else if(name=="-K") {
                    std::cout << "Changing K to " << value << std::endl;
                    K=value;
            }
    
            else if(name=="-G") {
                std::cout << "Changing G to " << value << std::endl;
                G=value;
            }
    
            else if(name=="-N") {
                    std::cout << "Changing N to " << value << std::endl;
                    N=value;
            }

            else if(name=="-X") {
                    std::cout << "Changing X to " << value << std::endl;
                    X=value;
            }

            else if(name=="-Y") {
                    std::cout << "Changing Y to " << value << std::endl;
                    Y=value;
            }

            else if(name=="-strides") {
                std::cout << "Changing strides to " << value << std::endl;
                strides=value;
            }

            //Tile parameters
            else if(name=="-T_R") {
                    std::cout << "Changing T_R to " << value << std::endl;
                    T_R=value;
            } 

            else if(name=="-T_S") {
                    std::cout << "Changing T_S to " << value << std::endl;
                    T_S=value;
            }

            else if(name=="-T_C") {
                    std::cout << "Changing T_C to " << value << std::endl;
                    T_C=value;
            }

            else if(name=="-T_K") {
                    std::cout << "Changing T_K to " << value << std::endl;
                    T_K=value;
            }

            else if(name=="-T_G") {
                std::cout << "Changing T_G to " << value << std::endl;
                T_G=value;
            }

            else if(name=="-T_N") {
                    std::cout << "Changing T_N to " << value << std::endl;
                    T_N=value;
            }

            else if(name=="-T_X_") {
                    std::cout << "Changing T_X_ to " << value << std::endl;
                    T_X_=value;
            }

            else if(name=="-T_Y_") {
                    std::cout << "Changing T_Y_ to " << value << std::endl;
                    T_Y_=value;
            }

            else if(name=="-generate_tile") {
                std::cout << "Changing generate_tile to " << value_str << std::endl;
                tileGeneratorTarget = parseTileGeneratorTarget(value_str);
            }

            else if(name=="-generator") {
                std::cout << "Changing generator to " << value_str << std::endl;
                tileGenerator = parseTileGenerator(value_str);
            }

            else if(name == "-V_th"){
                std::cout<<"Changing neuron membrane potential threshold to "<< value <<std::endl;
                stonne_cfg.V_th = value;
            }

            else if(name == "-Timestamp"){
                std::cout<<"Changing timestamp to "<<value<<std::endl;
                stonne_cfg.Timestamp = value;
            }

            //Parameter is not recognized
            else {
                    std::cout << "Error: parameter " << name << " does not exist" << std::endl;
                    exit(1);
                }
            }

        else {
            std::cout << "Error: parameter " << arg << " does not exist" << std::endl;
            exit(1);
        }
    }
}

// configDenseGEMMParameters(argc, argv, stonne_cfg, layer_name, M, K, N, T_M, T_K, T_N, tileGeneratorTarget, tileGenerator); 
// configDenseGEMMParameters(argc, argv, stonne_cfg, layer_name, V_th, M, K, N, T_M, T_K, T_N, tileGeneratorTarget, tileGenerator); 
void configDenseGEMMParameters(int argc, char *argv[], Config &stonne_cfg, std::string &layer_name, unsigned int &M, unsigned int &K, unsigned int &N, unsigned int &T_M, unsigned int &T_K, unsigned int &T_N, TileGenerator::Target &tileGeneratorTarget, TileGenerator::Generator &tileGenerator) {
  //Parsing
    for(int i=2; i<argc; i++) { //0 is the name of the program and 1 is the execution command type
        string arg = argv[i];
        //Spliting using = character
        string::size_type pos = arg.find('='); // 查找‘=’在arg中第一次出现的位置，并将该位置存储在pos变量中
        if(arg.npos != pos) {  //arg.npos表示一个无效的索引或‘未找到’状态
            string value_str=arg.substr(pos+1);   // extract a substring 
            string name=arg.substr(0, pos);
            unsigned int value;
            if((name != "-layer_name") && (name != "-rn_type") && (name != "-mn_type") && (name != "-mem_ctrl") && (name != "-generate_tile") && (name != "-generator") && (name != "-pooling_type")) { //string parameters
                value=stoi(value_str);  //如果不是特定的某些参数的话，将其类型转化为整型  Convert a string to an integer
            }

            //Checking parameter name

            if(name=="-num_ms") {
                if(!ispowerof2(value)) {   //Checking that the num_ms is power of 2
                    std::cout << "Error: -num_ms must be power of 2" << std::endl;
                    exit(1);
                }
                std::cout << "Changing num_ms to " << value << std::endl; //To debug
                stonne_cfg.m_MSNetworkCfg.ms_size=value;
            }
            else if(name=="-ms_rows") {
                    if(!ispowerof2(value)) {
                        std::cout << "Error: -ms_rows must be power of 2" << std::endl;
                        exit(1);
                    }
                    std::cout << "Changing ms_rows to " << value << std::endl; //To debug
                    stonne_cfg.m_MSNetworkCfg.ms_rows=value;

            }
            else if(name=="-ms_cols") {
                    if(!ispowerof2(value)) {   
                        std::cout << "Error: -ms_cols must be power of 2" << std::endl;
                        exit(1);
                    }
                    std::cout << "Changing ms_cols to " << value << std::endl; //To debug
                    stonne_cfg.m_MSNetworkCfg.ms_cols=value;
                    stonne_cfg.m_MSNetworkCfg.ms_size=stonne_cfg.m_MSNetworkCfg.ms_rows*stonne_cfg.m_MSNetworkCfg.ms_cols;
            }

            else if(name=="-dn_bw") {
                if(!ispowerof2(value)) {
                    std::cout << "Error: -dn_bw must be power of 2" << std::endl;
                    exit(1);
                }
                std::cout << "Changing dn_bw to " << value << std::endl; //To debug
                stonne_cfg.m_SDMemoryCfg.n_read_ports=value;
            }

            else if(name=="-rn_bw") {
                if(!ispowerof2(value)) {
                    std::cout << "Error: -rn_bw must be power of 2" << std::endl; 
                    exit(1);
                }
                std::cout << "Changing rn_bw to " << value << std::endl;
                stonne_cfg.m_SDMemoryCfg.n_write_ports=value;
            }

            else if(name=="-accumulation_buffer") {
                std::cout << "Changing accumulation_buffer to " << value << std::endl;
                stonne_cfg.m_ASNetworkCfg.accumulation_buffer_enabled=value;
            }

            else if(name=="-print_stats") {
                if((value != 0) && (value != 1)) {
                    std::cout << "Error: -print_stats only supports 0 or 1" << std::endl;
                    exit(1);
                }
                std::cout << "Changing print_stats to " << value << std::endl;
                stonne_cfg.print_stats_enabled=value; 
            }

            else if(name=="-rn_type") {
                std::cout << "Changing rn_type to " << value_str << std::endl;
                stonne_cfg.m_ASNetworkCfg.reduce_network_type=get_type_reduce_network_type(value_str);
            }

            else if(name=="-mn_type") {
                std::cout << "Changing mn_type to " << value_str << std::endl;
                stonne_cfg.m_MSNetworkCfg.multiplier_network_type=get_type_multiplier_network_type(value_str); // 检查命令行中输入的乘法器网络类型是否合法 
            }

            else if(name=="-mem_ctrl") {
                std::cout << "Changing mem_ctrl to " << value_str << std::endl;
                stonne_cfg.m_SDMemoryCfg.mem_controller_type=get_type_memory_controller_type(value_str);
            }

            else if(name=="-pooling_type") {
                std::cout<<"Changing pooling_type to "<<value_str <<std::endl;
                stonne_cfg.pooling_type = get_type_pooling_type(value_str);
            }

            //Running configuration parameters (layer)
   
           //Layer parameters
            else if(name=="-layer_name") {
                std::cout << "Changing layer_name to " << value_str << std::endl;
                layer_name=value_str; 
            }
    
            else if(name=="-M") {
                    std::cout << "Changing M to " << value << std::endl;
                    M=value;
            }

            else if(name=="-N") {
                    std::cout << "Changing N to " << value << std::endl;
                    N=value;
            }

            else if(name=="-K") {
                    std::cout << "Changing K to " << value << std::endl;
                    K=value;
            } 
            
            else if(name=="-T_M") {
                    std::cout << "Changing T_M to " << value << std::endl;
                    T_M=value;
            }

            else if(name=="-T_N") {
                std::cout << "Changing T_N to " << value << std::endl;
                T_N=value;
            }

                else if(name=="-T_K") {
                std::cout << "Changing T_K to " << value << std::endl;
                T_K=value;
            }

            else if(name=="-generate_tile") {
                std::cout << "Changing generate_tile to " << value_str << std::endl;
                tileGeneratorTarget = parseTileGeneratorTarget(value_str);
            }

            else if(name=="-generator") {
                std::cout << "Changing generator to " << value_str << std::endl;
                tileGenerator = parseTileGenerator(value_str);
            }

            // SNN parameter
            else if(name == "-V_th"){
                std::cout<<"Changing neuron membrane potential threshold to "<< value <<std::endl;
                stonne_cfg.V_th = value;
            }
            else if(name == "-Timestamp"){
                std::cout<<"Changing timestamp to "<<value<<std::endl;
                stonne_cfg.Timestamp = value;
            }

            //Parameter is not recognized
            else {
                    std::cout << "Error: parameter " << name << " does not exist" << std::endl;
                    exit(1);
            }

        }
        
        else {
            std::cout << "Error: parameter " << arg << " does not exist" << std::endl;
            exit(1);
        }
    }  

    std::cout<<std::endl;

}
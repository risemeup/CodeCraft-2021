#ifndef  GLOBALVARIABLE_H
#define GLOBALVARIABLE_H

// #include <bits/stdc++.h>
#include<iostream>
#include<vector>
#include<unordered_map>
#include<unordered_set>
#include<cstring>
#include<string>
#include<algorithm>
#include<ctime>
#include<cmath>
#include<deque>
using namespace std;


#define DEBUG

#ifdef DEBUG
const std::string filePath2 = "./data/training-2.txt";
const std::string filePath1 = "./data/training-1.txt";
#endif



enum Op{
    UNFIT,
    ADD, DEL,
    PURCHASE,MIGRATE,
    PLACE_A,  PLACE_B, PLACE_AB
};

struct serverType{
    std::string severName;
    int cores, mems,  deviceCost, dailyPowerCost;
};

struct virtualType{
    std::string vmName;
    int cores, mems, binnary;
};

// 继承于virtualType类
struct  virtualMachine:virtualType
{
    int id, node, boss;
    virtualMachine() = default;
    virtualMachine(virtualType vir){
        vmName = vir.vmName;
        cores =  vir.cores;
        mems = vir.mems;
        binnary = vir.binnary;
        id = -1;
        node = -1;
        boss = -1;
    }
    // bool operator< (const virtualMachine& t)const{
    //     return max(cores,mems)>max(t.cores,t.mems);
    // }
};

struct request{
    int op;
    std::string vmName;
    int requestID;
};

struct server{
    serverType servertype;
    int  index; //在服务器集合中的下标(存放在服务器列表的顺序)
    int purchaseId; //实际购买序列中的id (实际的购买顺序) 与上面的顺序在多种服务器采购时候可能并不相同
    double log_rate;
    std::pair<int,int> serverNodeA,serverNodeB;//服务结点
    std::unordered_map<int,virtualMachine> requestIdSet;

    server() = default;
    server(serverType & _servertype, int _index){
        index = _index;
        purchaseId = -1;
        servertype = _servertype;
        serverNodeA = {_servertype.cores / 2, _servertype.mems / 2};
        serverNodeB = {_servertype.cores / 2, _servertype.mems / 2};
        log_rate = log(_servertype.cores)-log(_servertype.mems);
    }

    bool isFIt_A(int cores, int mems){
        return serverNodeA.first >= cores && serverNodeA.second >= mems;
    }

    bool isFIt_B(int cores, int mems){
        return serverNodeB.first >= cores && serverNodeB.second >= mems;
    }

    void addvir(virtualMachine& vir){
        vir.boss = index;
        requestIdSet[vir.id] = vir;
        int core = vir.cores;
        int mem = vir.mems;
        if(vir.node ==  PLACE_AB){
            core /= 2;
            mem /= 2;
            serverNodeA.first -= core;
            serverNodeA.second -= mem;
            serverNodeB.first -= core;
            serverNodeB.second -= mem;
        }
        else if(vir.node == PLACE_A){
            serverNodeA.first -= core;
            serverNodeA.second -= mem;
        }
        else{
            serverNodeB.first -= core;
            serverNodeB.second -= mem;
        }
    }

    void delvir(int virid){
        virtualMachine vir = requestIdSet[virid];
        requestIdSet.erase(virid);
        int core = vir.cores;
        int mem = vir.mems;
        if(vir.node ==  PLACE_AB){
            core /= 2;
            mem /= 2;
            serverNodeA.first += core;
            serverNodeA.second += mem;
            serverNodeB.first += core;
            serverNodeB.second += mem;
        }
        else if(vir.node == PLACE_A){
            serverNodeA.first += core;
            serverNodeA.second += mem;
        }
        else{
            serverNodeB.first += core;
            serverNodeB.second += mem;
        }
    }
};
#endif

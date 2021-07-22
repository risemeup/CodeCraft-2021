#ifndef RWTOOL_H
#define RWTOOL_H

#include "GlobalVariable.h"

struct RWTool{
    RWTool(const std::string& filePath);
    RWTool();
    int readServerInfos();
    int readVirtualMachineInfos();
    void readDay(int& T, int & K);
    int readDay();
    int readRequstInfos();
    void chooseServerDevice(int day);
    void chooseServerDevice(int day, double f1, double f2);
    void makeHashDistance1(int day);
    void makeHashDistance2(int day);
    void makeHashDistance3();
    std::vector<std::string> split(std::string _str, char what);
    void addRes(std::string str);
    std::vector<serverType> serverTypeVec;//服务器种类数组
    std::unordered_map<std::string, serverType> serverInfos;//服务器信息映射
    std::unordered_map<std::string, virtualType> virtualMachineInfos;//虚拟机信息映射
    std::unordered_map <int, std::string>  requestIdMaps;//请求ID的信息映射
    std::vector<std::vector<request>> requestAll;//按照天数储存所有请求
    std::vector<std::string> res;
    std::vector<std::string> serverTypeSort;
    std::unordered_map<int, pair<string, int>> distance;
    int max_core;
    int max_mem;
};
#endif
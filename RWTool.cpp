#include "RWTool.h"

RWTool::RWTool(const std::string& filePath){
    #ifdef DEBUG
    std::freopen(filePath.c_str(),"r",stdin);//将文件重定向到标准输入  之后对标准输入操作就是对文件操作
    #endif
}
RWTool::RWTool(){
    #ifdef DEBUG
    std::freopen(filePath2.c_str(),"r",stdin);//将文件重定向到标准输入  之后对标准输入操作就是对文件操作
    #endif
    max_core = 0;
    max_mem = 0;
}

int RWTool::readServerInfos(){
    int serverTypeNums;
    std::cin>>serverTypeNums;//读入服务器数量
    std::cin.ignore(1024, '\n');//要忽略\n换行符 因为cin本身不读取'\n' 此函数读取且抛弃换行符
    for (int i = 0; i < serverTypeNums; ++i) {
        std::cin.ignore();//忽略左空格
        std::string oneLine;
        getline(std::cin, oneLine);
        oneLine.pop_back();//弹出右空格
        auto fields = split(oneLine,',');//以.分开各部分
        std::string serverName = fields[0];//stoi  string123 变为 int 123
        int cores = std::stoi(fields[1]);
        int mems = std::stoi(fields[2]);
        max_core = max(max_core, cores);
        max_mem = max(max_mem, mems);
        int hardDeviceCost = std::stoi(fields[3]);
        int dailyPowerCost = std::stoi(fields[4]);
        serverTypeVec.push_back({serverName, cores, mems, hardDeviceCost, dailyPowerCost});
        serverInfos[serverName] = serverTypeVec.back();
    }





    return serverTypeNums;
};

int RWTool::readVirtualMachineInfos() {//同上
    int vmNums;
    std::cin>>vmNums;
    std::cin.ignore(1024, '\n');
    //read virtual hosts types
    for (int i = 0; i < vmNums; ++i) {
        std::cin.ignore();
        std::string oneLine;
        getline(std::cin, oneLine);
        oneLine.pop_back();
        auto fields = split(oneLine,',');

        std::string vmName = fields[0];
        int cores = std::stoi(fields[1]);
        int mems = std::stoi(fields[2]);
        int binnary = std::stoi(fields[3]);
        virtualMachineInfos[vmName] = {vmName, cores, mems, binnary};

    }
    return vmNums;
}

int RWTool::readRequstInfos() {//同上
    std::vector<request> requests;
    int requestNums;
    std::cin>>requestNums;
    std::cin.ignore(1024, '\n');
    for (int i = 0; i < requestNums; ++i) {
        std::cin.ignore();//读取左空格
        std::string oneLine;
        getline(std::cin, oneLine);
        oneLine.pop_back();//把右空格弹出
        auto fields = split(oneLine,',');
        int op = (fields[0]=="add")? ADD:DEL;
        std::string vmName;
        int requestID;
        if(op == ADD){
            vmName = fields[1].substr(1);
            requestID = std::stoi(fields[2]);
            requestIdMaps[requestID] = vmName;//请求ID和虚拟机名称映射上
        }else{
            //delete
            requestID = std::stoi(fields[1]);
            vmName = requestIdMaps[requestID];//找到需要删除的虚拟机的名字
        }
        requests.push_back({op,vmName,requestID});

    }
    requestAll.emplace_back(requests);
    return requestNums;
}

 void RWTool::readDay(int& T, int & K){
     std::cin >> T;
     std::cin >> K;
     return;
 }

int RWTool::readDay() {//读取操作天数
    int ret;
    std::cin>>ret;
    return ret;
}

std::vector<std::string>  RWTool::split(std::string _str, char what) {
    auto ret = std::vector<std::string>();
    size_t pos;
    while((pos = _str.find(what))!=std::string::npos){
        ret.emplace_back(_str.substr(0, pos));
        _str = _str.substr(pos + 1);
    }
    ret.emplace_back(_str);
    return ret;
}

void RWTool::addRes(std::string str) {
    res.emplace_back(str);
}

void RWTool ::chooseServerDevice(int day){//按照天选服务器  主要是dailyPowerCost因素
            auto cmp = [&](serverType& serverA, serverType& serverB){
            //double weightA = 0 * serverA.deviceCost + serverA.dailyPowerCost;
            //double weightB = 0 * serverB.deviceCost + serverB.dailyPowerCost;
            
            double weightA = 1.7 * (serverA.deviceCost + serverA.dailyPowerCost*day) / serverA.cores +
                            1.0 * (serverA.deviceCost + serverA.dailyPowerCost*day) / serverA.mems;
            double weightB = 1.7 * (serverB.deviceCost + serverB.dailyPowerCost*day) / serverB.cores +
                            1.0 * (serverB.deviceCost + serverB.dailyPowerCost*day) / serverB.mems;
            
            // double weightA = serverA.cores*800 + serverA.mems*300 + serverA.deviceCost + serverA.dailyPowerCost*day;
            // double weightB = serverB.cores*800  + serverB.mems*300 + serverB.deviceCost + serverB.dailyPowerCost*day;
            // double divide = (double)vmUsedCores / (vmUsedCores +  vmUsedMems);
            //double weightA = (serverA.deviceCost * 10+ day * serverA.dailyPowerCost *0.5 );
            //double weightB = (serverB.deviceCost * 10 + day  * serverB.dailyPowerCost *0.5 );
            //weightA = weightA / serverA.cores * divide + weightA / serverA.mems * (1 - divide);
            //weightB = weightB / serverB.cores * divide + weightB / serverB.mems * (1 - divide);
            return weightA <= weightB;
            } ;


            serverTypeSort.clear();
            std::sort(serverTypeVec.begin(), serverTypeVec.end(), cmp);  //排序服务器

            for(int i = 0; i < serverTypeVec.size(); ++i){
               serverTypeSort.push_back(serverTypeVec[i].severName);//排序结果的服务器名 写入serverTypeSort
           }
}

// 服务器性价比排序，day:剩余天数；f1：cpu核心的比重；f2：mem的比重
void RWTool ::chooseServerDevice(int day, double f1, double f2){
            auto cmp = [&](serverType& serverA, serverType& serverB){
            //double weightA = 0 * serverA.deviceCost + serverA.dailyPowerCost;
            //double weightB = 0 * serverB.deviceCost + serverB.dailyPowerCost;
            
            // w = f1*(购买成本+日常能耗*剩余天数)/cpu核心数量 + f2*(购买成本+日常能耗*剩余天数)/mem
            double weightA = f1 * (serverA.deviceCost + serverA.dailyPowerCost*day) / serverA.cores +
                            f2 * (serverA.deviceCost + serverA.dailyPowerCost*day) / serverA.mems;
            double weightB = f1 * (serverB.deviceCost + serverB.dailyPowerCost*day) / serverB.cores +
                            f2 * (serverB.deviceCost + serverB.dailyPowerCost*day) / serverB.mems;

            
            // double weightA = serverA.cores*800 + serverA.mems*300 + serverA.deviceCost + serverA.dailyPowerCost*day;
            // double weightB = serverB.cores*800  + serverB.mems*300 + serverB.deviceCost + serverB.dailyPowerCost*day;
            // double divide = (double)vmUsedCores / (vmUsedCores +  vmUsedMems);
            //double weightA = (serverA.deviceCost * 10+ day * serverA.dailyPowerCost *0.5 );
            //double weightB = (serverB.deviceCost * 10 + day  * serverB.dailyPowerCost *0.5 );
            //weightA = weightA / serverA.cores * divide + weightA / serverA.mems * (1 - divide);
            //weightB = weightB / serverB.cores * divide + weightB / serverB.mems * (1 - divide);
            return weightA < weightB;
            } ;


            // serverTypeSort.clear();
            std::sort(serverTypeVec.begin(), serverTypeVec.end(), cmp);  //排序服务器

        //     for(int i = 0; i < serverTypeVec.size(); ++i){
        //        serverTypeSort.push_back(serverTypeVec[i].severName);//排序结果的服务器名 写入serverTypeSort
        //    }
}

void RWTool ::makeHashDistance1(int day){
            distance.clear();
            auto cmp = [&](serverType& serverA, serverType& serverB){
            
            double weightA = (serverA.dailyPowerCost*day);
                          
            double weightB =( serverB.dailyPowerCost*day) ;

            return weightA <= weightB;
            } ;


            serverTypeSort.clear();
            std::sort(serverTypeVec.begin(), serverTypeVec.end(), cmp);  //排序服务
            for(int i = 0;i <= max_core/2; i++)
                for(int j = 0; j <= max_mem/2; j++)
                {
                    int key = i*1000 + j;
                    pair<string, int> val = {"", 1e9};
                    for(auto& item_ : serverTypeVec)
                    {
                        auto& item =  serverInfos[item_.severName];
                        auto& ser_type = item;
                        if(ser_type.cores/2 >= i && ser_type.mems/2 >= j)
                        {
                            int diff = item.cores/2 + item.mems/2 - i - j;
                            val.first = item.severName;
                            val.second = diff;
                            break;
                        }
                    }
                    if(val.first != "")
                        distance[key] = val;
                }
        return;
    }
    
void RWTool ::makeHashDistance2(int day){
            distance.clear();
            auto cmp = [&](serverType& serverA, serverType& serverB){
            
            double weightA = serverA.deviceCost + serverA.dailyPowerCost*day ;
            double weightB =  serverB.deviceCost + serverB.dailyPowerCost*day;
            return weightA <= weightB;

        } ;


            serverTypeSort.clear();
            std::sort(serverTypeVec.begin(), serverTypeVec.end(), cmp);  //排序服务
            for(int i = 0;i <= max_core/2; i++)
                for(int j = 0; j <= max_mem/2; j++)
                {
                    int key = i*1000 + j;
                    pair<string, int> val = {"", 1e9};
                    for(auto& item_ : serverTypeVec)
                    {
                        auto& item =  serverInfos[item_.severName];
                        auto& ser_type = item;
                        if(ser_type.cores/2 >= i && ser_type.mems/2 >= j)
                        {
                            int diff = item.cores/2 + item.mems/2 - i - j;
                            val.first = item.severName;
                            val.second = diff;
                            break;
                        }
                    }
                    if(val.first != "")
                        distance[key] = val;
                }
        return;
    }

void RWTool ::makeHashDistance3(){
    
        for(int i = 0;i <= max_core/2; i++)
        for(int j = 0; j <= max_mem/2; j++)
        {
            int key = i*1000 + j;
            pair<string, int> val = {"", 1e9};
            for(auto& item: serverInfos)
            {
                auto& ser_type = item.second;
                if(ser_type.cores/2 >= i && ser_type.mems/2 >= j)
                {
                    int diff = item.second.cores/2 + item.second.mems/2 - i - j;
                    if(diff == val.second && item.second.dailyPowerCost < serverInfos[val.first].dailyPowerCost)
                    {
                        val.first = item.first;
                        val.second = diff;
                    }
                    else if(diff < val.second)
                    {
                        val.first = item.first;
                        val.second = diff;
                    }
                }
            }
            if(val.first != "")
                distance[key] = val;
        }
}
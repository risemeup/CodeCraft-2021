#include "Solution.h"
#include<cmath>


double BFD_fit_cpu = 1.0;   // 放置虚拟机 cpu比例 如果小于0：=ser_sort_cpu_rate + core_mem_rate
double core_mem_rate;       // 当天请求cpu/mem*10
double mig_BFD_cpu = -1;   // 迁移cpu比例  如果小于0：=ser_sort_cpu_rate + core_mem_rate
double no_migrate_rate = 0.07;     // 服务器容量小于rate不迁移
double ser_sort_cpu_rate = 2.7;     // 服务器排序常数
double bigger_server = 1.1;         // 购买稍大一些的服务器


Solution::Solution(RWTool* _rwtool, int _days): rwtool(_rwtool), days(_days){
    serverDeviceCost = 0;
    powerCost = 0;
    all_time = (double)clock(); 
    all_migrate = 0;
}


void Solution::purchase(int vmcores,int vmmems){
    std::string name;

    for(auto& _server: rwtool -> serverTypeVec)
    {
        if(_server.cores / 2 >= vmcores && _server.mems / 2  >=  vmmems ){
            serversNeedBuy[_server.severName]++;
            alreadyBuyServers.emplace_back(_server,alreadyBuyServers.size());//调用构造函数
            break;
        }
    }

    // for(int i = 0; i < rwtool -> serverTypeSort.size(); ++i){//遍历排序好的服务器 找到第一个可以放下虚拟机的服务器
    //     name = rwtool ->serverTypeSort[i];
    //     serverType&  _server = rwtool -> serverInfos[name];
    //     if(_server.cores / 2 >= vmcores && _server.mems / 2  >=  vmmems ){
    //         break;
    //     }
    // }
    // serverType &servertype = rwtool -> serverInfos[name] ;
    // serversNeedBuy[servertype.severName]++;
    // alreadyBuyServers.emplace_back(servertype,alreadyBuyServers.size());//调用构造函数
}


void heap_swap(vector<int>& vir_nums,int a,int b,unordered_map<int,int>& hp,unordered_map<int,int>& ph)
{
    swap(ph[hp[a]],ph[hp[b]]);
    swap(hp[a],hp[b]);
    swap(vir_nums[a],vir_nums[b]);
}

void down(vector<int>& vir_nums,int i,unordered_map<int,int>& hp,unordered_map<int,int>& ph)
{

    int t=i;
    if(i*2<=vir_nums[0]&&vir_nums[i*2]<vir_nums[t]) t=i*2;
    if(i*2+1<=vir_nums[0]&&vir_nums[i*2+1]<vir_nums[t]) t=i*2+1;
    if(i!=t)
    {
        heap_swap(vir_nums,i,t,hp,ph);
        down(vir_nums,t,hp,ph);
    }
}

inline void up(vector<int>& vir_nums,int i,unordered_map<int,int>& hp,unordered_map<int,int>& ph)
{
    if(i>vir_nums[0]) return;
    while(i/2&&vir_nums[i/2]>vir_nums[i])
    {
        heap_swap(vir_nums,i,i/2,hp,ph);
        i/=2;
    }
}

// 最佳适应下降，堆排序（物理服务器数量）
// 整体思路：
// 1. 服务器按所拥有的虚拟机数量从小到大排序
// 2. 从小到大尝试将尽可能多的服务器清空
// 3. 服务器所拥有的虚拟机数量在迁移过程中会变化，用堆的结构维护数量的有序性，每次处理的都是当前最少虚拟机的服务器
vector<string> Solution::migrate_BFD_dui()
{
    int max_migrate=requestMap.size()*30/1000;
    vector<vector<int>> migrate_map; // [源服务器，目的服务器，结点,虚拟机ID]

    // 小根堆：比较绕，无必要跳过
    vector<int> vir_nums={0};
    unordered_map<int,int> hp,ph;   // hp:堆指外  ph:外指堆
    for(auto& ser:alreadyBuyServers)
    {   
        if(ser.requestIdSet.size())
        {
            vir_nums.push_back(ser.requestIdSet.size());
            vir_nums[0]++;
            hp[vir_nums[0]]=ser.index;
            ph[ser.index]=vir_nums[0];
        }
    }
    for(int i=vir_nums[0]/2;i>0;i--)
        down(vir_nums,i,hp,ph);


    double f1,f2,w1,w2;  
    double cpu_rate = mig_BFD_cpu < 0? core_mem_rate + ser_sort_cpu_rate : mig_BFD_cpu;
    
    // vir_nums[0]维护还未处理的服务器数量
    while(vir_nums[0])
    {   
        // 每次取出堆顶元素当作源服务器
        int src_id=hp[1];
        server& src_server=alreadyBuyServers[src_id];
        // 如果当前服务器已空或者，已经很满了就直接跳过，不往外迁移，no_migrate_rate表示当剩余容量小于多少比例时不迁移，若超时，可以增加这个参数
        if(vir_nums[1]==0 || (src_server.serverNodeA.first+src_server.serverNodeA.second+src_server.serverNodeB.first+src_server.serverNodeB.second)
            <(src_server.servertype.cores+src_server.servertype.mems) * no_migrate_rate)
        {
            heap_swap(vir_nums,1,vir_nums[0],hp,ph);
            vir_nums[0]--;
            down(vir_nums,1,hp,ph);
            continue;
        }

        vector<vector<int>> migrate_i;

        vector<virtualMachine> shuang;
        vector<virtualMachine> dan;
        vector<virtualMachine> need_set;
        for(auto& item:src_server.requestIdSet)
        {
            virtualMachine vir=item.second;
            if(vir.binnary)
                shuang.push_back(vir);
            else
                dan.push_back(vir);
        }

        // 对源服务器的虚拟机从大到小排序  先双结点再单节点
        // sort(shuang.begin(),shuang.end(),[](virtualMachine& a,virtualMachine& b){
        // return a.cores+a.mems > b.cores + b.mems; 
        // });
        // sort(dan.begin(),dan.end(),[](virtualMachine& a,virtualMachine& b){
        // return a.cores+a.mems > b.cores+b.mems; 
        // });

        // for(auto& item:shuang)
        //     need_set.push_back(item);
        // for(auto& item:dan)
        //     need_set.push_back(item);


        // for(auto& vir:need_set){
        for(auto& item:src_server.requestIdSet){
            if(migrate_map.size() >= max_migrate) break;

            virtualMachine vir=item.second;

            int core=vir.cores,mem=vir.mems;
            vector<int> pre_migrate={src_server.index,-1,UNFIT};

            // 双结点虚拟机
            if(vir.binnary) 
            {
                core/=2,mem/=2;
                f1 = (src_server.serverNodeA.first + src_server.serverNodeB.first) * cpu_rate + (src_server.serverNodeA.second + src_server.serverNodeB.second);
                f2 = src_server.servertype.cores + src_server.servertype.mems;

                for(auto& dst_server:alreadyBuyServers){
                    if(src_server.index==dst_server.index || dst_server.requestIdSet.empty()) continue;
            
                    if(dst_server.isFIt_A(core, mem) && dst_server.isFIt_B(core, mem))
                    {
                        w1 = (dst_server.serverNodeA.first +dst_server.serverNodeB.first - core * 2) * cpu_rate + dst_server.serverNodeA.second + dst_server.serverNodeB.second - mem*2;
                        w2 = dst_server.servertype.cores + dst_server.servertype.mems;
                        if(w1 * f2 < w2 * f1){    // w1/w2<f1/f2  ---->  w1*f2<w2*f1
                        // if(w1 < f1){
                            f1=w1,f2=w2;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_AB;
                        }
                    }
                }
            }
            else
            {                   
                if(vir.node==PLACE_A) f1 = src_server.serverNodeA.first * cpu_rate + src_server.serverNodeA.second;
                else f1 = src_server.serverNodeB.first * cpu_rate + src_server.serverNodeB.second; 
                f2 = src_server.servertype.cores + src_server.servertype.mems;
                
                for(auto& dst_server:alreadyBuyServers){
                    if(src_server.index == dst_server.index || dst_server.requestIdSet.empty()) continue;

                    if(dst_server.isFIt_A(core, mem))
                    {
                        w1 = (dst_server.serverNodeA.first - core) * cpu_rate + dst_server.serverNodeA.second - mem;
                        w2 = dst_server.servertype.cores + dst_server.servertype.mems;
                        if(w1 * f2 < w2 * f1){
                        // if(w1 < f1){
                            f1=w1,f2=w2;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_A;
                        }
                    }
                    if(dst_server.isFIt_B(core, mem))
                    {
                        w1 = (dst_server.serverNodeB.first - core) * cpu_rate + dst_server.serverNodeB.second - mem;
                        w2 = dst_server.servertype.cores + dst_server.servertype.mems;
                        if(w1*f2<w2*f1){
                        // if(w1 < f1){
                            f1=w1,f2=w2;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_B;
                        }
                    }
                }
            }
            if(pre_migrate[1]==-1)  // 表示源服务器没有任何一台虚拟机能迁移出去
                continue;
            else                    // 更新堆信息
            {
                pre_migrate.push_back(vir.id);
                migrate_i.push_back(pre_migrate);
                vir.node=pre_migrate[2];
                alreadyBuyServers[pre_migrate[1]].addvir(vir);

                vir_nums[ph[pre_migrate[1]]]++;
                if(ph[pre_migrate[1]]<=vir_nums[0])
                {
                    up(vir_nums,ph[pre_migrate[1]],hp,ph);
                    down(vir_nums,ph[pre_migrate[1]],hp,ph);
                }
                
                if(migrate_i.size() + migrate_map.size() >=  max_migrate)
                    break;
            }
        }
        for(auto item:migrate_i){
            int vir_id=item[3];
            alreadyBuyServers[item[0]].delvir(vir_id);
            migrate_map.push_back(item);

            vir_nums[ph[item[0]]]--;
            up(vir_nums,ph[item[0]],hp,ph);
            down(vir_nums,ph[item[0]],hp,ph);
        }
        heap_swap(vir_nums,1,vir_nums[0],hp,ph);
        vir_nums[0]--;
        down(vir_nums,1,hp,ph);
    }

    // 生成标准迁移格式
    vector<string> migrate_info;
    migrate_info.push_back("(migration, "+std::to_string(migrate_map.size())+")");
    for(auto item:migrate_map){
        int vir_id=item[3];
        int dst_id=alreadyBuyServers[item[1]].purchaseId;
        int node=item[2];
        requestMap[vir_id] = item[1];
        if(node==PLACE_AB)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+")");
        else if(node==PLACE_A)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", A)");
        else
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", B)");
    }
    return migrate_info;

}

vector<string> Solution::migrate_BFD_dui_twice()
{
    int max_migrate=requestMap.size()*30/1000;
    vector<vector<int>> migrate_map; // [源服务器，目的服务器，结点,虚拟机ID]

    vector<int> vir_nums={0};
    unordered_map<int,int> hp,ph;   // hp:堆指外  ph:外指堆
    for(auto& ser:alreadyBuyServers)
    {   
        if(ser.requestIdSet.size())
        {
            vir_nums.push_back(ser.requestIdSet.size());
            vir_nums[0]++;
            hp[vir_nums[0]]=ser.index;
            ph[ser.index]=vir_nums[0];
        }
    }

    double f1,f2,w1,w2; 
    for(int try_cnt = 0;try_cnt < 2;try_cnt++)
    {
        vir_nums[0] = vir_nums.size() - 1;
        
        // 重建堆
        for(int i=vir_nums[0]/2;i>0;i--)
            down(vir_nums,i,hp,ph);
    
        while(vir_nums[0])
        {
            int src_id=hp[1];
            server& src_server=alreadyBuyServers[src_id];
            if(vir_nums[1]==0 || (src_server.serverNodeA.first+src_server.serverNodeA.second+src_server.serverNodeB.first+src_server.serverNodeB.second)
            <(src_server.servertype.cores+src_server.servertype.mems) * no_migrate_rate)
            {
                heap_swap(vir_nums,1,vir_nums[0],hp,ph);
                vir_nums[0]--;
                down(vir_nums,1,hp,ph);
                continue;
            }

            vector<vector<int>> migrate_i;

            for(auto& item:src_server.requestIdSet)
            {
                if(migrate_map.size() >= max_migrate) break;

                virtualMachine vir=item.second;
                int core=vir.cores,mem=vir.mems;
                vector<int> pre_migrate={src_server.index,-1,UNFIT};

                if(vir.binnary) 
                {
                    core/=2,mem/=2;

                    if(try_cnt == 1)
                    {
                        f1 = (src_server.serverNodeA.first + src_server.serverNodeB.first) * mig_BFD_cpu + (src_server.serverNodeA.second + src_server.serverNodeB.second);
                        f2 = src_server.servertype.cores + src_server.servertype.mems;
                    }
                    else
                    {
                        f1 = 1e9;
                        f2 = 1;
                    }
                    

                    for(auto& dst_server:alreadyBuyServers){
                        if(src_server.index==dst_server.index || dst_server.requestIdSet.empty()) continue;
                
                        if(dst_server.isFIt_A(core, mem) && dst_server.isFIt_B(core, mem))
                        {
                            w1 = (dst_server.serverNodeA.first +dst_server.serverNodeB.first - core * 2) * mig_BFD_cpu + dst_server.serverNodeA.second + dst_server.serverNodeB.second - mem*2;
                            w2 = dst_server.servertype.cores + dst_server.servertype.mems;
                            if(w1 * f2 < w2 * f1){    // w1/w2<f1/f2  ---->  w1*f2<w2*f1
                                f1=w1,f2=w2;
                                pre_migrate[1]=dst_server.index;
                                pre_migrate[2]=PLACE_AB;
                            }
                        }
                    }
                }
                else
                {            
                    if(try_cnt == 1)
                    {
                        if(vir.node==PLACE_A) f1 = src_server.serverNodeA.first * mig_BFD_cpu + src_server.serverNodeA.second;
                        else f1 = src_server.serverNodeB.first * mig_BFD_cpu + src_server.serverNodeB.second; 
                        f2 = src_server.servertype.cores + src_server.servertype.mems;
                    }       
                    else
                    {
                        f1 = 1e9;
                        f2 = 1;
                    }
                    
                    
                    for(auto& dst_server:alreadyBuyServers){
                        if(src_server.index==dst_server.index || dst_server.requestIdSet.empty()) continue;

                        if(dst_server.isFIt_A(core, mem))
                        {
                            w1 = (dst_server.serverNodeA.first - core) * mig_BFD_cpu + dst_server.serverNodeA.second - mem;
                            w2 = dst_server.servertype.cores + dst_server.servertype.mems;
                            if(w1*f2<w2*f1){
                                f1=w1,f2=w2;
                                pre_migrate[1]=dst_server.index;
                                pre_migrate[2]=PLACE_A;
                            }
                        }
                        if(dst_server.isFIt_B(core, mem))
                        {
                            w1 = (dst_server.serverNodeB.first - core) * mig_BFD_cpu + dst_server.serverNodeB.second - mem;
                            w2 = dst_server.servertype.cores + dst_server.servertype.mems;
                            if(w1 * f2 < w2 * f1){
                                f1=w1,f2=w2;
                                pre_migrate[1]=dst_server.index;
                                pre_migrate[2]=PLACE_B;
                            }
                        }
                    }
                }
                if(pre_migrate[1]==-1) 
                    continue;
                else
                {
                    pre_migrate.push_back(vir.id);
                    migrate_i.push_back(pre_migrate);
                    vir.node=pre_migrate[2];
                    alreadyBuyServers[pre_migrate[1]].addvir(vir);

                    vir_nums[ph[pre_migrate[1]]]++;
                    if(ph[pre_migrate[1]]<=vir_nums[0])
                    {    
                        up(vir_nums,ph[pre_migrate[1]],hp,ph);
                        down(vir_nums,ph[pre_migrate[1]],hp,ph);
                    }
                    
                    if(migrate_i.size() + migrate_map.size() >=  max_migrate)
                        break;
                }
            }
            for(auto item:migrate_i){
                int vir_id=item[3];
                alreadyBuyServers[item[0]].delvir(vir_id);
                alreadyBuyServers[item[0]].delvir(vir_id);
                migrate_map.push_back(item);

                vir_nums[ph[item[0]]]--;
                up(vir_nums,ph[item[0]],hp,ph);
                down(vir_nums,ph[item[0]],hp,ph);
            }
            heap_swap(vir_nums,1,vir_nums[0],hp,ph);
            vir_nums[0]--;
            down(vir_nums,1,hp,ph);
        }


    }
    
    // vector<pair<int, int>> after_migrate;
    // for(int i = 0;i < alreadyBuyServers.size();i++)
    // {   
    //     auto ser = alreadyBuyServers[i];   
    //     if(ser.requestIdSet.size())
    //     {
    //         after_migrate.push_back({ser.requestIdSet.size(), i});
    //     }
    // }
    // sort(after_migrate.begin(), after_migrate.end());

    vector<string> migrate_info;
    migrate_info.push_back("(migration, "+std::to_string(migrate_map.size())+")");
    for(auto item:migrate_map)
    {
        int vir_id=item[3];
        int dst_id=alreadyBuyServers[item[1]].purchaseId;
        int node=item[2];
        requestMap[vir_id] = item[1];
        if(node==PLACE_AB)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+")");
        else if(node==PLACE_A)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", A)");
        else
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", B)");
    }
    return migrate_info;

}

vector<string> Solution::migrate_BFD_dui_capacity()
{
    int max_migrate=requestMap.size()*30/1000;
    vector<vector<int>> migrate_map; // [源服务器，目的服务器，结点,虚拟机ID]

    vector<int> vir_capacity={0};
    unordered_map<int,int> hp,ph;   // hp:堆指外  ph:外指堆
    for(auto& ser:alreadyBuyServers)
    {   
        if(ser.requestIdSet.size())
        {
            int left_capacity = ser.servertype.cores + ser.servertype.mems 
                                - ser.serverNodeA.first - ser.serverNodeA.second
                                - ser.serverNodeB.first - ser.serverNodeB.second;
            // int left_capacity = ser.serverNodeA.first + ser.serverNodeA.second
            //                     +ser.serverNodeB.first + ser.serverNodeB.second;
            vir_capacity.push_back(left_capacity);
            vir_capacity[0]++;
            hp[vir_capacity[0]]=ser.index;
            ph[ser.index]=vir_capacity[0];
        }
    }
    for(int i=vir_capacity[0]/2;i>0;i--)
        down(vir_capacity,i,hp,ph);


    // int n=vir_nums.size();
    long long f1,f2,w1,w2;  
    while(vir_capacity[0])
    {
        int src_id=hp[1];
        server& src_server=alreadyBuyServers[src_id];
        if(vir_capacity[1]==0 || (src_server.serverNodeA.first+src_server.serverNodeA.second+src_server.serverNodeB.first+src_server.serverNodeB.second)
            <(src_server.servertype.cores+src_server.servertype.mems)*0.035)
        {
            heap_swap(vir_capacity,1,vir_capacity[0],hp,ph);
            vir_capacity[0]--;
            down(vir_capacity,1,hp,ph);
            continue;
        }

        vector<vector<int>> migrate_i;

        for(auto& item:src_server.requestIdSet)
        {
            if(migrate_map.size() >= max_migrate) break;

            virtualMachine vir=item.second;
            int core=vir.cores,mem=vir.mems;
            vector<int> pre_migrate={src_server.index, -1, UNFIT};

            if(vir.binnary) 
            {
                core/=2,mem/=2;
                f1=(long long)src_server.serverNodeA.first+src_server.serverNodeA.second+src_server.serverNodeB.first+src_server.serverNodeB.second;
                f2=(long long)src_server.servertype.cores+src_server.servertype.mems;

                for(auto& dst_server:alreadyBuyServers){
                    if(src_server.index==dst_server.index || dst_server.requestIdSet.empty()) continue;
            
                    if(dst_server.isFIt_A(core, mem) && dst_server.isFIt_B(core, mem))
                    {
                        w1=(long long)dst_server.serverNodeA.first+dst_server.serverNodeA.second+dst_server.serverNodeB.first+dst_server.serverNodeB.second - core*2 - mem*2;
                        w2=(long long)dst_server.servertype.cores+dst_server.servertype.mems;
                        if(w1*f2<w2*f1){    // w1/w2<f1/f2  ---->  w1*f2<w2*f1
                            f1=w1,f2=w2;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_AB;
                        }
                    }
                }
            }
            else
            {                   
                if(vir.node==PLACE_A) f1=(long long)src_server.serverNodeA.first+src_server.serverNodeA.second;
                else f1=(long long)src_server.serverNodeB.first+src_server.serverNodeB.second; 
                f2=((long long)src_server.servertype.cores+src_server.servertype.mems);
                
                for(auto& dst_server:alreadyBuyServers){
                    if(src_server.index==dst_server.index || dst_server.requestIdSet.empty()) continue;

                    if(dst_server.isFIt_A(core, mem))
                    {
                        w1=(long long)dst_server.serverNodeA.first+dst_server.serverNodeA.second - core - mem;
                        w2=((long long)dst_server.servertype.cores+dst_server.servertype.mems);
                        if(w1*f2<w2*f1){
                            f1=w1,f2=w2;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_A;
                        }
                    }
                    if(dst_server.isFIt_B(core, mem))
                    {
                        w1=(long long)dst_server.serverNodeB.first+dst_server.serverNodeB.second - core - mem;
                        w2=((long long)dst_server.servertype.cores+dst_server.servertype.mems);
                        if(w1*f2<w2*f1){
                            f1=w1,f2=w2;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_B;
                        }
                    }
                }
            }
            if(pre_migrate[1]==-1) 
                continue;
            else
            {
                pre_migrate.push_back(vir.id);
                migrate_i.push_back(pre_migrate);
                vir.node=pre_migrate[2];
                alreadyBuyServers[pre_migrate[1]].addvir(vir);

                vir_capacity[ph[pre_migrate[1]]] = vir_capacity[ph[pre_migrate[1]]] + core + mem;
                if(ph[pre_migrate[1]]<=vir_capacity[0])
                {
                    up(vir_capacity,ph[pre_migrate[1]],hp,ph);
                    down(vir_capacity,ph[pre_migrate[1]],hp,ph);
                }
                
                if(migrate_i.size() + migrate_map.size() >=  max_migrate)
                    break;
            }
        }
        for(auto item:migrate_i){
            int vir_id=item[3];
            migrate_map.push_back(item);

            int core = alreadyBuyServers[item[0]].requestIdSet[vir_id].cores;
            int mem = alreadyBuyServers[item[0]].requestIdSet[vir_id].mems;
            vir_capacity[ph[item[0]]] = vir_capacity[ph[item[0]]] - core - mem;
            up(vir_capacity,ph[item[0]],hp,ph);
            down(vir_capacity,ph[item[0]],hp,ph);

            alreadyBuyServers[item[0]].delvir(vir_id);
        }
        heap_swap(vir_capacity,1,vir_capacity[0],hp,ph);
        vir_capacity[0]--;
        down(vir_capacity,1,hp,ph);
    }
    
    // vector<pair<int, int>> after_migrate;
    // for(int i = 0;i < alreadyBuyServers.size();i++)
    // {   
    //     auto ser = alreadyBuyServers[i];   
    //     if(ser.requestIdSet.size())
    //     {
    //         after_migrate.push_back({ser.requestIdSet.size(), i});
    //     }
    // }
    // sort(after_migrate.begin(), after_migrate.end());

    vector<string> migrate_info;
    migrate_info.push_back("(migration, "+std::to_string(migrate_map.size())+")");
    for(auto item:migrate_map){
        int vir_id=item[3];
        int dst_id=alreadyBuyServers[item[1]].purchaseId;
        int node=item[2];
        requestMap[vir_id] = item[1];
        if(node==PLACE_AB)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+")");
        else if(node==PLACE_A)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", A)");
        else
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", B)");
    }
    return migrate_info;

}

vector<string> Solution::migrate_BFD() 
{
    int max_migrate=requestMap.size();
    vector<vector<int>> migrate_map; // [源服务器，目的服务器，结点,虚拟机ID]
    max_migrate=max_migrate*30/1000;     //当天可迁移的总数

    vector<pair<int,int>> virnum;
    for(int i=0;i<alreadyBuyServers.size();i++){
        server& _server=alreadyBuyServers[i];
        virnum.push_back({_server.requestIdSet.size(),i});//first: 服务器i已有的虚拟机数量  second：服务器ID
    }

    sort(virnum.begin(),virnum.end());// 按服务器拥有的虚拟机数量，从小到大排序，优先迁移少数的虚拟机

    int n=virnum.size();
    double minv = 1e9, cur = 0; 
    for(int i=0;migrate_map.size()<max_migrate && virnum[i].first<=5 && i<n; i++)
    {
        server& src_server=alreadyBuyServers[virnum[i].second];

        if((src_server.serverNodeA.first+src_server.serverNodeA.second+src_server.serverNodeB.first+src_server.serverNodeB.second)
            <(src_server.servertype.cores+src_server.servertype.mems) * 0.035)
            continue;

        vector<vector<int>> migrate_i;

        vector<virtualMachine> shuang;
        vector<virtualMachine> dan;
        vector<virtualMachine> need_set;
        for(auto& item:src_server.requestIdSet)
        {
            virtualMachine vir=item.second;
            if(vir.binnary)
                shuang.push_back(vir);
            else
                dan.push_back(vir);
        }

        sort(shuang.begin(),shuang.end(),[](virtualMachine& a,virtualMachine& b){
        return a.cores+a.mems > b.cores + b.mems; 
        });
        sort(dan.begin(),dan.end(),[](virtualMachine& a,virtualMachine& b){
        return a.cores+a.mems > b.cores+b.mems; 
        });

        for(auto& item:shuang)
            need_set.push_back(item);
        for(auto& item:dan)
            need_set.push_back(item);

        for(auto& vir:need_set)
        {
            int core=vir.cores,mem=vir.mems;
            vector<int> pre_migrate={src_server.index,-1,UNFIT};

            if(vir.binnary) 
            {
                core/=2,mem/=2;
                minv = ((src_server.serverNodeA.first + src_server.serverNodeB.first) * mig_BFD_cpu + src_server.serverNodeA.second +src_server.serverNodeB.second)
                        / (src_server.servertype.cores+src_server.servertype.mems);
                for(int j=n-1;j>=0;j--){
                    if(j==i || alreadyBuyServers[virnum[j].second].requestIdSet.empty()) continue;

                    server& dst_server=alreadyBuyServers[virnum[j].second];
            
                    if(dst_server.isFIt_A(core, mem) && dst_server.isFIt_B(core, mem))
                    {
                        cur = ((dst_server.serverNodeA.first + dst_server.serverNodeB.first - core*2) * mig_BFD_cpu + dst_server.serverNodeA.second + dst_server.serverNodeB.second - mem*2)
                                / (dst_server.servertype.cores+dst_server.servertype.mems);
                        if(cur < minv){    // w1/w2<f1/f2  ---->  w1*f2<w2*f1
                            minv = cur;
                            pre_migrate[1]=dst_server.index;
                            pre_migrate[2]=PLACE_AB;
                        }
                    }
                }
            }
            else{                   
                    if(vir.node==PLACE_A) minv = (src_server.serverNodeA.first * mig_BFD_cpu + src_server.serverNodeA.second) / (src_server.servertype.cores + src_server.servertype.mems);
                    else minv= (src_server.serverNodeB.first * mig_BFD_cpu + src_server.serverNodeB.second) / (src_server.servertype.cores + src_server.servertype.mems); 
                    
                    for(int j=n-1;j>=0;j--){
                        if(j==i || alreadyBuyServers[virnum[j].second].requestIdSet.empty()) continue;

                        server& dst_server=alreadyBuyServers[virnum[j].second];

                        if(dst_server.isFIt_A(core, mem))
                        {
                            cur = (dst_server.serverNodeA.first - core) * mig_BFD_cpu + dst_server.serverNodeA.second - mem;
                            if(minv < cur){
                                cur = minv;
                                pre_migrate[1]=dst_server.index;
                                pre_migrate[2]=PLACE_A;
                            }
                        }
                        if(dst_server.isFIt_B(core, mem))
                        {
                            cur = (dst_server.serverNodeB.first - core) * mig_BFD_cpu + dst_server.serverNodeB.second - mem;
                            if(minv < cur){
                                cur = minv;
                                pre_migrate[1]=dst_server.index;
                                pre_migrate[2]=PLACE_B;
                            }
                        }
                    }
            }
            if(pre_migrate[1]==-1) 
            {
                continue;
            }
            else{
                pre_migrate.push_back(vir.id);
                migrate_i.push_back(pre_migrate);
                vir.node=pre_migrate[2];
                alreadyBuyServers[pre_migrate[1]].addvir(vir);
                
                if(migrate_i.size() + migrate_map.size() >=  max_migrate){
                    break;
                }
            }
        }
        for(auto item:migrate_i){
            int vir_id=item[3];
            alreadyBuyServers[item[0]].delvir(vir_id);
            migrate_map.push_back(item);
        }
    }

    vector<string> migrate_info;
    migrate_info.push_back("(migration, "+std::to_string(migrate_map.size())+")");
    for(auto item:migrate_map){
        int vir_id=item[3];
        int dst_id=alreadyBuyServers[item[1]].purchaseId;
        int node=item[2];
        requestMap[vir_id] = item[1];
        if(node==PLACE_AB)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+")");
        else if(node==PLACE_A)
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", A)");
        else
            migrate_info.push_back("("+std::to_string(vir_id)+", "+std::to_string(dst_id)+", B)");
    }
    return migrate_info;
}


void Solution::match(int totalDay, int day) {//每天的处理request的操作
    nowday = day;
    left_day = totalDay - day;
    place();
}

// 最佳适应下降法选服务器放置，输入待分配的虚拟机， 其中cpu_rate为全局参数，表示cpu这个维度的权重
pair<int,int> Solution::BFD(virtualMachine& vir){
    int vmcores = vir.cores,  vmmems = vir.mems;
    double minv=1e9;
    double cpu_rate = BFD_fit_cpu < 0? core_mem_rate + ser_sort_cpu_rate : BFD_fit_cpu;
    pair<int,int> res={-1,UNFIT};
    if(vir.binnary) // 双结点虚拟机
    {
        vmcores/=2,vmmems/=2;
        for(server& _server:alreadyBuyServers)  // 依次遍历已购买的服务器，计算放在该服务器中的适应度
        {
            if(_server.isFIt_A(vmcores, vmmems) && _server.isFIt_B(vmcores, vmmems))
            {
                // 计算适应度，越小越好 （剩余总资源-该虚拟机所占的资源）/(服务器总资源)  表示剩余率
                double left = ((_server.serverNodeA.first + _server.serverNodeB.first - vmcores * 2) * cpu_rate 
                                + _server.serverNodeA.second + _server.serverNodeB.second - vmmems *2)
                            /(_server.servertype.cores+_server.servertype.mems);
                if(left < minv){
                    minv = left;
                    res.first=_server.index,res.second=PLACE_AB;
                }
            }
               
        }
    }
    else    // 单节点虚拟机
    {
        for(server& _server:alreadyBuyServers){
            if(_server.isFIt_A(vmcores,vmmems)) // 服务器A结点
            {
                double left = ((_server.serverNodeA.first - vmcores) * cpu_rate + _server.serverNodeA.second - vmmems)
                                /(_server.servertype.cores+_server.servertype.mems);
                if(left < minv){
                    res.first=_server.index,res.second=PLACE_A;
                    minv = left;
                }
            }
            if(_server.isFIt_B(vmcores, vmmems))    // 服务器B结点
            {
                double left = ((_server.serverNodeB.first - vmcores) * cpu_rate + _server.serverNodeB.second - vmmems) 
                                /(_server.servertype.cores+_server.servertype.mems);
                if(left < minv){
                    res.first=_server.index,res.second=PLACE_B;
                    minv = left;
                }
            }
        }
    }
    return res;
}

void Solution::add_by_order(vector<request>& requests)
{
    for(request& req:requests)
    {
        if(req.op==ADD){
            virtualType type = rwtool -> virtualMachineInfos[req.vmName];
            virtualMachine machine(type);
            machine.id = req.requestID;
            pair<int,int> ret = BFD(machine);
            if(ret.first == -1)
            {
                if(machine.binnary)
                {
                    purchase(machine.cores / 2, machine.mems /2);  // 放不下，挑选能放下vir的最便宜的服务器
                    machine.node = PLACE_AB;
                }
                else
                {
                    purchase(machine.cores, machine.mems);  // 放不下，挑选能放下vir的最便宜的服务器
                    machine.node = PLACE_A;
                }
                alreadyBuyServers.back().addvir(machine);
            }
            else
            {
                machine.node = ret.second;  // 标注当前虚拟机应该放置的节点
                alreadyBuyServers[ret.first].addvir(machine);   // 挂载到对应的服务器
            }
            requestMap[machine.id] = machine.boss;
        }
        else{
            int serid = requestMap[req.requestID];   // 查询待删除的虚拟机位于哪台服务器
            alreadyBuyServers[serid].delvir(req.requestID);  // 该方法直接传入，待删除的虚拟机ID
            requestMap.erase(req.requestID);
        }
    }
}

void Solution::add_by_order_D(vector<request>& requests)
{
    vector<request> need_add;
    for(request& req:requests)
    {
        if(req.op==ADD){
            need_add.push_back(req);
        }
        else{
            add_by_sort(need_add);
            need_add.clear();

            int serid = requestMap[req.requestID];   // 查询待删除的虚拟机位于哪台服务器
            alreadyBuyServers[serid].delvir(req.requestID);  // 该方法直接传入，待删除的虚拟机ID
            requestMap.erase(req.requestID);
        }
    }
    add_by_sort(need_add);
}

// 处理顺序：双结点请求 -> 单节点请求 -> 删除结点 
void Solution::add_by_sort(vector<request>& requests)
{
    if(requests.empty()) return;

    // 分单双节点，先处理双节点，再处理单节点
    vector<virtualMachine> shuang;  // 双结点虚拟机请求
    vector<virtualMachine> dan;     // 单节点虚拟机请求
    vector<int> need_del;           // 删除请求

    // 提取信息
    for(request& req:requests){
        if(req.op==ADD){
            virtualType type = rwtool -> virtualMachineInfos[req.vmName];
            virtualMachine machine(type);
            machine.id = req.requestID;
            if(machine.binnary)
                shuang.push_back(machine);
            else
                dan.push_back(machine);
        }
        else{
            need_del.push_back(req.requestID);
        }
    }

    // 根据虚拟机总资源从大到小排序
    sort(shuang.begin(),shuang.end(),[](virtualMachine& a,virtualMachine& b){
      return a.cores + a.mems > b.cores + b.mems; 
    });
    sort(dan.begin(),dan.end(),[](virtualMachine& a,virtualMachine& b){
      return a.cores + a.mems > b.cores + b.mems; 
    });

    // 处理双节点虚拟机
    for(virtualMachine& vir:shuang){
        pair<int,int> ret = BFD(vir);   // 最佳适应下降法选择最优的服务器，返回 first: 服务器id  second：应该放置的节点
        if(ret.first==-1) {
            purchase(vir.cores * 0.5 * bigger_server, vir.mems * 0.5 * bigger_server);  // 放不下，挑选能放下vir的最便宜的服务器
            vir.node = PLACE_AB;    // 直接放在刚买的服务器上
            alreadyBuyServers.back().addvir(vir);

        }
        else{
            vir.node = ret.second;  // 标注当前虚拟机应该放置的节点
            alreadyBuyServers[ret.first].addvir(vir);   // 挂载到对应的服务器
        }
        requestMap[vir.id] = vir.boss;
    }

    for(virtualMachine& vir:dan){
        pair<int,int> ret = BFD(vir);
        if(ret.first==-1) {
            purchase(vir.cores * bigger_server, vir.mems * bigger_server);
            vir.node = PLACE_A;     // 新买的服务器，A,B节点都能方向，直接选A
            alreadyBuyServers.back().addvir(vir);
        }
        else{
            vir.node = ret.second;
            alreadyBuyServers[ret.first].addvir(vir);
        }
        requestMap[vir.id] = vir.boss;
    }

    // 移除虚拟机
    for(int removeid:need_del){
        int serid = requestMap[removeid];   // 查询待删除的虚拟机位于哪台服务器
        alreadyBuyServers[serid].delvir(removeid);  // 该方法直接传入，待删除的虚拟机ID
        requestMap.erase(removeid);
    }

}

pair<int, int> Solution::BFD_group(int start, int binnary, int vmcores, int vmmems)
{
    pair<int,int> res={-1,UNFIT};
    double minv = 1e9;
    if(binnary) 
    {
        vmcores/=2,vmmems/=2;
        for(int i=start; i<alreadyBuyServers.size();i++)
        {
            server& _server = alreadyBuyServers[i];
            if(_server.isFIt_A(vmcores, vmmems) && _server.isFIt_B(vmcores, vmmems))
            {
                // 计算适应度，越小越好 （剩余总资源-该虚拟机所占的资源）/(服务器总资源)  表示剩余率
                double left = (double)(_server.serverNodeA.first + _server.serverNodeA.second 
                                + _server.serverNodeB.first + _server.serverNodeB.second - vmcores*2 - vmmems*2)
                            /(_server.servertype.cores+_server.servertype.mems);
                if(left < minv){
                    minv = left;
                    res.first=_server.index,res.second=PLACE_AB;
                }
            }
               
        }
    }
    else
    {
        for(int i=start; i<alreadyBuyServers.size();i++)
        {
            server& _server = alreadyBuyServers[i];
            if(_server.isFIt_A(vmcores,vmmems))
            {
                double left = (double)(_server.serverNodeA.first + _server.serverNodeA.second - vmcores - vmmems)
                                /(_server.servertype.cores+_server.servertype.mems);
                if(left < minv){
                    res.first=_server.index,res.second=PLACE_A;
                    minv = left;
                }
            }
            if(_server.isFIt_B(vmcores, vmmems))
            {
                double left = (double)(_server.serverNodeB.first + _server.serverNodeB.second - vmcores - vmmems)
                                /(_server.servertype.cores+_server.servertype.mems);
                if(left < minv){
                    res.first=_server.index,res.second=PLACE_B;
                    minv = left;
                }
            }
        }
    }
    return res;
}

void Solution::combination_fit_shuang(vector<virtualMachine>& left, vector<int>& need_fit)
{
    std::string name,best_type;
    double minv = 1e9;
    int res;

    int n = need_fit.size();
    int size = 1<<n;
    int res_core[size];
    int res_mem[size];
    memset(res_core, 0, sizeof res_core);
    memset(res_mem, 0, sizeof res_mem);
    
    for(int i = 0;i < n ;i++)
    {
        int vmcores = left[need_fit[i]].cores / 2;
        int vmmems = left[need_fit[i]].mems / 2;
        for(int j = 0; j < 1<<i ; j++)
        {
            int idx = (1<<i) + j;
            res_core[idx] = vmcores + res_core[j]; 
            res_mem[idx] = vmmems + res_mem[j];

            if(rwtool -> distance.count(res_core[idx]*1000 + res_mem[idx]))
            {
                auto& item = rwtool -> distance[res_core[idx]*1000 + res_mem[idx]];
                auto& ser_type = rwtool -> serverInfos[item.first];
                double rate = (double)(ser_type.dailyPowerCost * left_day + ser_type.deviceCost) / (2.0 * res_core[idx] + res_mem[idx]);
                if(rate < minv)
                {
                    minv = rate;
                    best_type = item.first;
                    res = idx;
                }
            }
        } 
    }

    serverType &servertype = rwtool -> serverInfos[best_type] ;
    serversNeedBuy[servertype.severName]++;
    alreadyBuyServers.emplace_back(servertype,alreadyBuyServers.size());//调用构造函数

    int cnt = 0;
    while(res)
    {
        if(res&1)
        {
            left[need_fit[cnt]].node = PLACE_AB;
        }
        res >>= 1;
        cnt ++;
    }
}

void Solution::combination_fit_dan(vector<virtualMachine>& left, vector<int>& need_fit)
{
    std::string name,best_type;
    double minv = 1e9;
    int res;
    
    int n = need_fit.size();
    int node[n], node_cpy[n];
    memset(node, 0, sizeof node);
    for(int i = 1;i < 1<<n ;i++)
    {
        memset(node_cpy, 0, sizeof node_cpy);
        int cores_A = 0,mems_A = 0,cores_B = 0,mems_B = 0,cnt = n-1, j = i; 
        while(j)
        {
            if(j&1)
            {
                int pre_cores_A = cores_A + left[need_fit[cnt]].cores;
                int pre_mems_A = mems_A + left[need_fit[cnt]].mems;
                // int left_A = max(pre_cores_A, cores_B)*2 - pre_cores_A - cores_B
                //             +max(pre_mems_A, mems_B)*2 - pre_mems_A - mems_B;
                int left_A = abs(pre_cores_A - cores_B) + abs(pre_mems_A - mems_B);

                int pre_cores_B = cores_B + left[need_fit[cnt]].cores;
                int pre_mems_B = mems_B + left[need_fit[cnt]].mems;
                // int left_B = max(cores_A, pre_cores_B)*2 - cores_A - pre_cores_B
                //             +max(mems_A, pre_mems_B)*2 - mems_A - pre_mems_B;
                int left_B = abs(cores_A - pre_cores_B) + abs(mems_A - pre_mems_B);

                if(left_A < left_B)
                {
                    cores_A = pre_cores_A;
                    mems_A = pre_mems_A;
                    node_cpy[cnt] = PLACE_A;
                }
                else
                {
                    cores_B = pre_cores_B;
                    mems_B = pre_mems_B;
                    node_cpy[cnt] = PLACE_B;
                }
            }
            j >>= 1;
            cnt --;
        }

        int vmcores = max(cores_A, cores_B);
        int vmmems = max(mems_A, mems_B);
        if(rwtool -> distance.count(vmcores*1000 + vmmems))
        {
            auto& item = rwtool -> distance[vmcores*1000 + vmmems];
            auto& ser_type = rwtool -> serverInfos[item.first];
            // int real_dis = serverinfo.cores + serverinfo.mems - cores_A -mems_A - cores_B - mems_B;
            double rate = (double)(ser_type.dailyPowerCost * left_day + ser_type.deviceCost) / (cores_A * 2.0 + mems_A + cores_B * 2.0 + mems_B);
            // double rate = 1.5 * (ser_type.dailyPowerCost * left_day + ser_type.deviceCost) / ser_type.cores
            //             + (ser_type.dailyPowerCost * left_day + ser_type.deviceCost) / ser_type.mems;
            if(rate < minv)
            {
                minv = rate;
                best_type = item.first;
                res = i;
                memcpy(node, node_cpy, sizeof node);
            }
        }
    }
    serverType &servertype = rwtool -> serverInfos[best_type] ;
    serversNeedBuy[servertype.severName]++;
    alreadyBuyServers.emplace_back(servertype,alreadyBuyServers.size());//调用构造函数

    int cnt = n-1;
    while(res)
    {
        if(res&1)
        {
            left[need_fit[cnt]].node = node[cnt];
        }
        res >>= 1;
        cnt --;
    }
}

// 请求添加的虚拟机先按cpu/mem的比例排序，再从头尾各取7个进行组合优化
void Solution::add_by_group(vector<request>& requests)
{

    // 分单双节点，先处理双节点，再处理单节点
    vector<virtualMachine> shuang;
    vector<virtualMachine> dan;
    vector<int> need_del;
    vector<virtualMachine> left_shuang;
    vector<virtualMachine> left_dan;
    
    for(request& req:requests){
        if(req.op==ADD){
            virtualType type = rwtool -> virtualMachineInfos[req.vmName];
            virtualMachine machine(type);
            machine.id = req.requestID;
            if(machine.binnary)
                shuang.push_back(machine);
            else
                dan.push_back(machine);
        }
        else{
            need_del.push_back(req.requestID);
        }
    }
    
    // if(shuang.size() + dan.size() > 1000)
    // {
    //     add_by_sort(requests);
    //     return;
    // }
        

    // 根据虚拟机总资源从大到小排序
    sort(shuang.begin(),shuang.end(),[](virtualMachine& a,virtualMachine& b){
      return a.cores + a.mems > b.cores + b.mems; 
    });
    sort(dan.begin(),dan.end(),[](virtualMachine& a,virtualMachine& b){
      return a.cores + a.mems > b.cores + b.mems; 
    });

    // 处理双节点虚拟机
    for(virtualMachine& vir:shuang){
        pair<int,int> ret = BFD(vir);   // 最佳适应下降法选择最优的服务器，返回 first: 服务器id  second：应该放置的节点
        if(ret.first==-1) {
            left_shuang.push_back(vir);
        }
        else{
            vir.node = ret.second;  // 标注当前虚拟机应该放置的节点
            alreadyBuyServers[ret.first].addvir(vir);   // 挂载到对应的服务器
            requestMap[vir.id] = vir.boss;
        }
    }

    for(virtualMachine& vir:dan){
        pair<int,int> ret = BFD(vir);
        if(ret.first==-1) {
            left_dan.push_back(vir);
        }
        else{
            vir.node = ret.second;
            alreadyBuyServers[ret.first].addvir(vir);
            requestMap[vir.id] = vir.boss;
        }
    }

    // 根据虚拟机总资源从大到小排序
    sort(left_shuang.begin(),left_shuang.end(),[](virtualMachine& a,virtualMachine& b){
      return a.cores * b.mems > b.cores * a.mems; 
    });
    sort(left_dan.begin(),left_dan.end(),[](virtualMachine& a,virtualMachine& b){
      return a.cores * b.mems > b.cores * a.mems; 
    });


    int combination_num = 14;
    vector<int> combination;
    int left_num = left_shuang.size();
    while(left_num)
    {
        if(left_num >= combination_num)
        {
            for(int i=0;i < left_shuang.size() && combination.size() < combination_num/2;i++)
            if(left_shuang[i].node == -1)
                combination.push_back(i);

            for(int i = left_shuang.size()-1 ;i >= 0 && combination.size() < combination_num; i--)
                if(left_shuang[i].node == -1) 
                    combination.push_back(i);
        }
        else
        {
            for(int i=0;i < left_shuang.size(); i++)
            {
                if(left_shuang[i].node == -1)
                    combination.push_back(i);
            }
        }
        
        
        combination_fit_shuang(left_shuang, combination);
        for(int i:combination)
        {
            if(left_shuang[i].node != -1)
            {
                virtualMachine& vir = left_shuang[i];
                alreadyBuyServers.back().addvir(vir);
                requestMap[vir.id] = vir.boss;
                left_num --;
            }
        }
        combination.clear(); 

        for(virtualMachine& vir:left_shuang)
        {
            if(vir.node == -1)
            {
                if(alreadyBuyServers.back().isFIt_A(vir.cores/2, vir.mems/2)
                &&alreadyBuyServers.back().isFIt_B(vir.cores/2, vir.mems/2))
                {
                    vir.node = PLACE_AB;
                    alreadyBuyServers.back().addvir(vir);
                    requestMap[vir.id] = vir.boss;
                    left_num --;
                }
            }
        }
    }
    

    left_num = left_dan.size();
    combination_num = 14;
    while(left_num)
    {
        if(left_num >= combination_num)
        {
            for(int i=0;i < left_dan.size() && combination.size() < combination_num/2; i++)
            if(left_dan[i].node == -1)
                combination.push_back(i);

            for(int i = left_dan.size()-1 ;i >= 0 && combination.size() < combination_num; i--)
                if(left_dan[i].node == -1) 
                    combination.push_back(i);
        }
        else
        {
            for(int i=0;i < left_dan.size(); i++)
            {
                if(left_dan[i].node == -1)
                    combination.push_back(i);
            }
        }
        
        auto cmp = [&](int& a, int& b)
        {
            return left_dan[a].cores + left_dan[a].mems < left_dan[b].cores + left_dan[b].mems;
        };
        sort(combination.begin(),combination.end(), cmp);
        combination_fit_dan(left_dan, combination);
        for(int i:combination)
        {
            if(left_dan[i].node != -1)
            {
                virtualMachine& vir = left_dan[i];
                alreadyBuyServers.back().addvir(vir);
                requestMap[vir.id] = vir.boss;
                left_num --;
            }
        }
        combination.clear(); 

        for(virtualMachine& vir:left_dan)
        {
            if(vir.node == -1)
            {
                if(alreadyBuyServers.back().isFIt_A(vir.cores, vir.mems))
                {
                    vir.node = PLACE_A;
                    alreadyBuyServers.back().addvir(vir);
                    requestMap[vir.id] = vir.boss;
                    left_num --;
                }
                else if(alreadyBuyServers.back().isFIt_B(vir.cores, vir.mems))
                {
                    vir.node = PLACE_B;
                    alreadyBuyServers.back().addvir(vir);
                    requestMap[vir.id] = vir.boss;
                    left_num --;
                }
            }
        }
    }

    // 移除虚拟机
    for(int removeid:need_del){
        int serid = requestMap[removeid];   // 查询待删除的虚拟机位于哪台服务器
        alreadyBuyServers[serid].delvir(removeid);  // 该方法直接传入，待删除的虚拟机ID
        requestMap.erase(removeid);
    }

}


void Solution::place() {
    #ifdef DEBUG
    double time_Start = (double)clock();    // 记录处理时间
    #endif

    // [1] 预处理：读取信息，服务器性价比排序

    // 读取当天的所有请求
    std::vector<request> &requests = rwtool -> requestAll[nowday];
    int preServerNums = alreadyBuyServers.size();

    // 当天请求添加的服务器性价比排序
    long long sum_cores = 0;
    long long sum_mems = 0;
    for(request& req:requests)
    {
        if(req.op == ADD)
        {
            virtualType type = rwtool -> virtualMachineInfos[req.vmName];
            sum_cores += type.cores;
            sum_mems += type.mems;
        }
    }
    core_mem_rate = (sum_cores - sum_mems) * 1.0 / (sum_cores + sum_mems);
    core_mem_rate *= 10;

    // 服务器性价比排序
    rwtool -> chooseServerDevice(left_day, core_mem_rate + ser_sort_cpu_rate, 1.0);
    // rwtool -> chooseServerDevice(left_day, 1.0, core_mem_rate + ser_sort_cpu_rate);
    // rwtool -> chooseServerDevice(left_day);


    // [2] 迁移
    vector<string> migrate_info;
    // migrate_info.push_back("(migration, 0)");        // 不迁移
    // migrate_info = migrate_BFD();                    // 最佳适应下降迁移
    migrate_info = migrate_BFD_dui();                   // 最佳适应下降+堆排序（服务器拥有的虚拟机数量）
    // migrate_info = migrate_BFD_dui_twice();          // 最佳适应下降+两次堆优化  超时
    // migrate_info = migrate_BFD_dui_capacity();       // 最佳适应下降+堆排序（服务器剩余的cpu+mem容量）
    
    // 魔改部分，随缘优化
    // if(nowday % 100 == 0){
    //     migrate_info=migrate_FD_GAI(); 
    // }else if(nowday % 4 == 0){
    //     migrate_info=migrate_BFD_GAI(); 
    // }else{
    //     migrate_info = migrate_BFD();
    // }

    // [3] 处理所有新增和删除请求
    add_by_sort(requests);          // 先处理双结点虚拟机再处理单节点虚拟机， 按容量排序
    // add_by_group(requests);      // 组合优化
    // add_by_order(requests);      // 顺序处理请求
    // add_by_order_D(requests);    // 先处理删除,再处理请求, 有bug 


    // 重映射id 以保证服务器是批量购买
    rwtool->addRes("(purchase, "+std::to_string(serversNeedBuy.size())+")");
    int nowServerNums = alreadyBuyServers.size();
    int currentID = preServerNums;
    uint64_t server_cost=0;
    uint64_t day_cost=0;


    //下面的这一段应该是很难看  因为每种服务器的购买数量是一次性购买的  但是实际的服务器索引不一定是一口气全买 
    //比如 5台A 4台B  A的索引不一定是0 - 5  有可能 1,3,5,6,8 所以有一个purchaseId购买序列  
    //服务器有两个索引   一个是在服务器列表的索引 index   另一个是购买顺序索引 purchaseId  输出时候使用后者
    for(int i=preServerNums;i<nowServerNums;i++){
        server & _server = alreadyBuyServers[i];
        if(_server.purchaseId == -1){
            int num = serversNeedBuy[_server.servertype.severName];
            server_cost += _server.servertype.deviceCost *(uint64_t)num;
            rwtool->addRes("("+_server.servertype.severName+", "+std::to_string(num)+")");
            for(int j=i;j<nowServerNums;j++){
                if(_server.servertype.severName == alreadyBuyServers[j].servertype.severName){
                    alreadyBuyServers[j].purchaseId = currentID++;
                }
            }
        }
    }
    serverDeviceCost+=server_cost;
    serversNeedBuy.clear();
    
    // [4] 将购买和迁移方案写成标准格式
    // 添加虚拟机迁移信息
    for(auto info:migrate_info){
        rwtool->addRes(info);
    }

    int add_num=0,del_num=0;
    // 输出add操作对应的分配序列
    for(request& request:requests){
        if(request.op == ADD){
            add_num++;
            server& _server = alreadyBuyServers[requestMap[request.requestID]];
            virtualMachine & vir = _server.requestIdSet[request.requestID];
            std::string str = "("+std::to_string(_server.purchaseId);
            if(vir.node == PLACE_AB){
                rwtool->addRes(str+")");
            }else if(vir.node == PLACE_A){
                rwtool->addRes(str+", A)");
            }else rwtool->addRes(str+", B)");
        }
        else{
            del_num++;
        }
    }

    // [5] 控制台信息打印, debug模式才有
    #ifdef DEBUG
    // compute daily cost
    for(server & _server : alreadyBuyServers){
        if(!_server.requestIdSet.empty()) day_cost+=_server.servertype.dailyPowerCost;
    }
    double time_cost = ((double)clock() - time_Start)/1000; 
    powerCost += day_cost;
    // all_time += time_cost;
    all_migrate += migrate_info.size()-1;
    printf("day=%d\tserver_cost=%-9lldday_cost=%-8lldt=%.2fs\tmigrate_num=%-4lldadd_num=%-5ddel_num=%-5dall_vir=%-5lld\n"
    ,nowday,server_cost,day_cost,time_cost,migrate_info.size()-1,add_num,del_num,requestMap.size());
    #endif
}

void Solution::displayCost() {
    // std::cout<<"\nServerCost: "<< serverDeviceCost <<"\nPowerCost: "<<powerCost<<"\nTotal: "<<serverDeviceCost+powerCost<<std::endl;
    printf("\nServerCost: %lld\nPowerCost: %lld\nTotal: %lld\nall_migrate: %lld\nall_time: %.2fs\n"
            ,serverDeviceCost, powerCost, serverDeviceCost+powerCost, all_migrate, ((double)clock()-all_time)/1000);
}
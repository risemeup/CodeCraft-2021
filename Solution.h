#ifndef SOLUTION_H
#define SOLUTION_H

#include "RWTool.h"

class Solution{
    public:
        Solution(RWTool* rwtool, int days);
        void match(int totalDay,int day);
        void displayCost();
        uint64_t  serverDeviceCost, powerCost,all_migrate;
        std::vector<server> alreadyBuyServers;
    private:
        RWTool* rwtool;
        int days;
        
        double all_time;
        int nowday;
        int left_day;
        
        std::unordered_map<int,int> requestMap;//请求放在哪个服务器的哪个结点 first服务器索引 second放置方式
        std::unordered_map<std::string,int> serversNeedBuy;
        
        pair<int,int> BFD(virtualMachine& vir);
        pair<int,int> BFD_log(virtualMachine& vir);
        pair<int,int> BFD_group(int start, int binnary, int vmcores, int vmmems);
        // void purchase(virtualMachine& req);
        void purchase(int vmcores,int vmmems);
        void place();
        vector<string> migrate_BFD();
        vector<string> migrate_BFD_dui();
        vector<string> migrate_BFD_dui_capacity();
        vector<string> migrate_BFD_dui_twice();
        vector<string> migrate_BFD_combination();
        vector<string> migrate_BFD_er();
        vector<string> migrate_FD();
        vector<string> migrate_FD_GAI();
        vector<string> migrate_BFD_GAI();
        int fitServer(server &  _server,  request&  _request);
        void add_by_order(vector<request>& requests);
        void add_by_order_D(vector<request>& requests);
        void add_by_sort(vector<request>& requests);
        void add_by_group(vector<request>& requests);
        void combination_fit_shuang(vector<virtualMachine>& left, vector<int>& need_fit);
        void combination_fit_dan(vector<virtualMachine>& left, vector<int>& need_fit);
        bool combination_migrate_shuang(vector<virtualMachine>& left, 
        vector<int>& need, unordered_map<string, vector<int>>& ser_info, unordered_map<int, pair<int, int>>& ans, int& left_num);
        inline void find(vector<int>& pre_migrate, vector<int>& core_A, int& start, unordered_map<int, int>& core_A_hp);
};

#endif
#include "Solution.h"
#include "RWTool.h"

int main(){

	// [1] read standard input
    RWTool rwtool;                      
    rwtool.readServerInfos();              // 读取物理服务器信息
    rwtool.readVirtualMachineInfos();      // 读取虚拟机信息

    int T = 0, K = 0;
    rwtool.readDay(T,K);//读取总共天数 T和已知天数K  这个函数被重载了因为现在要读取两个数字

    Solution solution(&rwtool,T);

    // 将标准输出重定向到'output.txt'    
    #ifdef DEBUG
        std::freopen("output.txt","w",stdout);
    #endif
    
    //先读取k天的
    for(int ti = 0; ti < K; ti++){
        rwtool.readRequstInfos();
    }
    
    //以下输出一天 读取一天 而不是最后一口气输出
    for(int ti = 0; ti < T; ti++){
        rwtool.res.clear();                     //读之前要清空结果信息
        rwtool.chooseServerDevice(T - ti);      //选择服务器

        #ifdef DEBUG                            // debug模式可以打印到控制台，正式提交版需要在GlobalVariable.h文件中，取消 DEBUG 宏定义
            std::freopen( "CON", "w", stdout );
            solution.match(T, ti);              //进行第i天的操作 
            std::freopen("output.txt","a",stdout);
        #else
            solution.match(T, ti);//进行第i天的操作
        #endif
        
        // 以下为输出当天信息， 标准输入已重定向到文件，所以不会在控制台显示
        for(auto &s: rwtool.res){
            std::cout<<s<<"\n";
        }

        // 清空输出缓冲区 
        fflush(stdout);

        if(rwtool.requestAll.size() < T)
            rwtool.readRequstInfos();   // 读取下一天的信息
    }


    #ifdef DEBUG
        std::freopen( "CON", "w", stdout );
        solution.displayCost();
    #endif



	return 0;
}


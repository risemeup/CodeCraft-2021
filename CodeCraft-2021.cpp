#include "Solution.h"
#include "RWTool.h"

int main(){

	// [1] read standard input
    RWTool rwtool;                      
    rwtool.readServerInfos();              // ��ȡ�����������Ϣ
    rwtool.readVirtualMachineInfos();      // ��ȡ�������Ϣ

    int T = 0, K = 0;
    rwtool.readDay(T,K);//��ȡ�ܹ����� T����֪����K  �����������������Ϊ����Ҫ��ȡ��������

    Solution solution(&rwtool,T);

    // ����׼����ض���'output.txt'    
    #ifdef DEBUG
        std::freopen("output.txt","w",stdout);
    #endif
    
    //�ȶ�ȡk���
    for(int ti = 0; ti < K; ti++){
        rwtool.readRequstInfos();
    }
    
    //�������һ�� ��ȡһ�� ���������һ�������
    for(int ti = 0; ti < T; ti++){
        rwtool.res.clear();                     //��֮ǰҪ��ս����Ϣ
        rwtool.chooseServerDevice(T - ti);      //ѡ�������

        #ifdef DEBUG                            // debugģʽ���Դ�ӡ������̨����ʽ�ύ����Ҫ��GlobalVariable.h�ļ��У�ȡ�� DEBUG �궨��
            std::freopen( "CON", "w", stdout );
            solution.match(T, ti);              //���е�i��Ĳ��� 
            std::freopen("output.txt","a",stdout);
        #else
            solution.match(T, ti);//���е�i��Ĳ���
        #endif
        
        // ����Ϊ���������Ϣ�� ��׼�������ض����ļ������Բ����ڿ���̨��ʾ
        for(auto &s: rwtool.res){
            std::cout<<s<<"\n";
        }

        // ������������ 
        fflush(stdout);

        if(rwtool.requestAll.size() < T)
            rwtool.readRequstInfos();   // ��ȡ��һ�����Ϣ
    }


    #ifdef DEBUG
        std::freopen( "CON", "w", stdout );
        solution.displayCost();
    #endif



	return 0;
}


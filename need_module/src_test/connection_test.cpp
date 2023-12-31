//
// Created by zhjd on 2021/5/11.
//
//ros头文件
#include "common_include.h"

//内部头文件
// #include "need_satisfied.h"
#include "prior_need.h"
#include "perception_filter.h"

using namespace  std;
// time_t inner_need::time_for_wandor  =  0;
perception_filter *Filter = new perception_filter(20);   //TODO: 确定合适的per过滤时长阈值。

// ros node
ros::Subscriber sub_perception;
ros::Subscriber sub_robot_emotion;
ros::Subscriber sub_robot_status;
ros::Publisher pub;  //need
ros::Publisher query_attitude;
ros::Subscriber sub_idleState;
//全局变量: 
bool single_attitude = false;  //是否实时输出需求模块的状态，默认不输出
int period_cur = 0;   //每个period的周期时长,由 sleep函数 决定。




void single_need_test(ros::NodeHandle*  n_ptr){
            vector<need> need_lists;
            need need_local;
            need_local.IDtype = "Teacher";
            need_local.need_name = "WarmGreet";
            need_local.person_name = "Teacher_Li";
            need_lists.push_back(need_local);

            if( need_lists.size() != 0 )
                for(int j =0 ; j< need_lists.size(); j++){
                    
                    //接受社交态度及相关参数
                    social_msg::attitude_query query;
                    query.IDtype = need_lists[j].IDtype;
                    query.motivation = need_lists[j].need_name;
                    query.person_name = need_lists[j].person_name;
                    query_attitude.publish(query);
                    double t = 1.0;
                    ros::Duration timeout(t);
                    ROS_INFO("Need waiting for social attitude");
                    auto start = std::chrono::high_resolution_clock::now();
                    social_msg::attitude_msg::ConstPtr result = ros::topic::waitForMessage<social_msg::attitude_msg>("attitude", *n_ptr, timeout);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    
                    
                    // 将need发布给行为模块
                    social_msg::need_msg need_output;
                    need_output.person_name =  need_lists[j].person_name;
                    need_output.IDtype = need_lists[j].IDtype;
                    need_output.scene = need_lists[j].scene;
                    need_output.need_name = need_lists[j].need_name;  
                    need_output.target_angle = need_lists[j].target_angle;
                    need_output.target_distance = need_lists[j].target_distance;
                    need_output.rob_emotion = need_lists[j].robot_emotion_str;//TODO: need_lists[i].rob_emotion;
                    need_output.rob_emotion_intensity = need_lists[j].robot_emotion_intensity;
                    need_output.person_emotion = need_lists[j].person_emotion;//need_lists[i].person_emotion
                    need_output.weight = need_lists[j].weight;
                    need_output.speech = need_lists[j].speech;
                    need_output.qt_order = period_cur;
                    need_output.satisfy_value = need_lists[j].satisfy_value;
                    if (result != NULL)
                    {
                        // std::cout << "时间差: " << duration.count() << " 毫秒" << std::endl;
                        ROS_INFO("[Debug] Attitude Time Duration: %d ms", duration.count());
                        
                        if( need_output.person_name ==  result->person_name &&
                            need_output.IDtype      ==  result->IDtype &&
                            need_output.need_name   ==  result->motivation
                         ){
                            ROS_INFO("Received right social attitude");
                            need_output.attitude    = result->attitude;
                            need_output.move_speed  = result->move_speed;
                            need_output.distance   = result->distance;
                            need_output.voice_speed = result->voice_speed;
                        }
                        else{
                            ROS_INFO("Received wrong social attitude");
                            need_output.attitude    = "enthusiastic";   //enthusiastic,respectful,serious,disgust
                            need_output.move_speed  = 1.0;
                            need_output.distance    = 1.0;
                            need_output.voice_speed = 1.0;
                        }
                    }
                    else
                    {
                        ROS_WARN("Timeout: Failed to receive social attitude within %f seconds", t);
                        need_output.attitude    = "enthusiastic";   //enthusiastic,respectful,serious,disgust
                        need_output.move_speed  = 1.0;
                        need_output.distance    = 1.0;
                        need_output.voice_speed = 1.0;
                    }       
                }
        
            period_cur++;     
    // cout<< "End PriorNeed !!\n";
}



// 回调函数
void PerceptionUpdate(const social_msg::perception_msg& msg){
    
    social_msg::perception_msg per;
    per = msg;
    
    // float64 time            # 时间戳
    // string person_name      # 用户的名字
    // string IDtype           # 用户的身份
    // string scene            # 场景名称 (school, hospital, family)
    // float64 target_angle    # 交互对象角度
    // float64 target_distance # 交互对象距离
    // string intention        # 用户的top1意图
    // float64 p               # 用户的top1意图识别概率
    // string intention_2      # 用户的top2意图[可以赋空值]
    // float64 p_2             # 用户的top2意图识别概率[可以赋空值]
    // string intention_3      # 用户的top2意图[可以赋空值]
    // float64 p_3             # 用户的top2意图识别概率[可以赋空值]
    // string person_speech    # 用户问题文本说的话说的话[用户说的话]
    // string speech           # 回答文本[机器人回复的话]
    // string person_emotion   # 用户的心情
    
    
    std::cout << "时间戳: " << per.time << std::endl;
    std::cout << "用户的名字: " << per.person_name << std::endl;
    std::cout << "用户的身份: " << per.IDtype << std::endl;
    std::cout << "场景名称: " << per.scene << std::endl;
    std::cout << "交互对象角度: " << per.target_angle << std::endl;
    std::cout << "交互对象距离: " << per.target_distance << std::endl;
    std::cout << "用户的top1意图: " << per.intention << std::endl;
    std::cout << "用户的top1意图识别概率: " << per.p << std::endl;
    std::cout << "用户的top2意图: " << per.intention_2 << std::endl;
    std::cout << "用户的top2意图识别概率: " << per.p_2 << std::endl;
    // std::cout << "用户的top3意图: " << per.intention_3 << std::endl;
    // std::cout << "用户的top3意图识别概率: " << per.p_3 << std::endl;
    std::cout << "用户问题文本说的话: " << per.person_speech << std::endl;
    std::cout << "回答文本: " << per.speech << std::endl;
    std::cout << "用户的心情: " << per.person_emotion << std::endl;
    std::cout << std::endl;
}



int main(int argc, char** argv){
    // 是否实时输出需求模块的状态，默认不输出
    if(argc == 2){
        single_attitude = argv[1];
    }
        

    // ROS
    ros::init(argc, argv, "need_module");
    ros::NodeHandle n;
    cout<< "Need Module Start to Subscribe（接收ROS信息） !!\n";

    // 接受感知信息
    sub_perception = n.subscribe("perception_msg", 1000, PerceptionUpdate);
    ros::spinOnce();
    
    if(single_attitude)
    {
        // 社交态度查询
        query_attitude = n.advertise<social_msg::attitude_query>("attitude_query", 1);  

        // 控制需求先验模型的运行周期
        ros::Rate loop_rate(0.35);  //5s一次

        // 为需求模型的运行  创建单独的线程 。  
        cout<< "Wait to run PriorNeed !!\n";
        while(ros::ok){
            if( ros::isShuttingDown() )
                break;
            single_need_test(&n);
            ros::spinOnce();
            loop_rate.sleep();
        }
    }
    else{
        ros::Rate loop_rate(0.35);  //5s一次
        while(ros::ok){
            if( ros::isShuttingDown() )
                break;
            ros::spinOnce();
            loop_rate.sleep();
        }
    }
    
    return 0;
}



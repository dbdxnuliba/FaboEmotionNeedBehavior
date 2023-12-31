#include "ros/ros.h"
#include <dynamic_reconfigure/server.h>
#include <interface_sim/robot_emotion_publisherConfig.h>
#include "social_msg/need_msg.h"
#include "social_msg/robot_emotion.h"
#include "social_msg/robot_status.h"
#include "social_msg/perception_msg.h"
#include "social_msg/attitude_msg.h"
#include "social_msg/idleState.h"
ros::Publisher pub_perception;
ros::Publisher pub_attitude;
ros::Publisher pub_body;
ros::Publisher pub_emotion;
ros::Publisher pub_needlist;
ros::Publisher pub_idleState;


void callback(robot_emotion_publisher::robot_emotion_publisherConfig &config, uint32_t level)
{
// ros::Rate loop_rate(1);

// while ( ros::ok() ) {
  if(config.publish == true)     
        {
        //感知信息 
            // 意图
            if(config.O_____per_switch == true)
            {
            social_msg::perception_msg perception;
            ros::Time now = ros::Time::now();
            perception.time = now.sec;
            perception.person_name = config.per_person_name;
            perception.IDtype = config.per_person_IDtype;
            perception.scene = config.per_scene;
            perception.intention = config.per_intention;
            perception.p = config.per_p;
            perception.intention_2 = config.per_intention_2;
            perception.p_2 = config.per_p_2;
            perception.speech = config.per_speech;
            perception.person_emotion = config.per_emotion;
            pub_perception.publish(perception);

            social_msg::attitude_msg attitude;    
            attitude.person_name = config.per_person_name;
            attitude.attitude = config.attitude_type;
            pub_attitude.publish(attitude);
            }

            //社交态度
            if(config.O_____attitude_switch == true)
            {
            social_msg::attitude_msg attitude;    
            attitude.person_name = config.attitude_person_name;
            attitude.IDtype = config.attitude_person_IDtype;
            attitude.motivation = config.attitude_motivation;
            attitude.attitude = config.attitude_type;
            attitude.move_speed = config.attitude_move_speed;
            attitude.distance = config.attitude_distance;
            attitude.voice_speed = config.attitude_voice_speed;
            attitude.attitude = config.attitude_type;
            pub_attitude.publish(attitude);
            }

            //空闲状况
            if(config.O_____idleState_switch == true)
            {
            social_msg::idleState idleState_msg; 
            idleState_msg.idleState = config.idleState;
            idleState_msg.hehavior_name = config.idle_hehavior_name;
            idleState_msg.person_name = config.idle_person_name;
            idleState_msg.IDtype = config.idle_person_IDtype;
            idleState_msg.target_angle  = config.idle_target_angle;
            idleState_msg.target_distance = config.idle_target_distance;
            idleState_msg.person_emotion = config.idle_person_emotion;
            idleState_msg.satisfy_value = config.idle_satisfy_value;
            pub_idleState.publish(idleState_msg);
            } 
    //         //身体状况 
    //         if(config.O_____body_switch == true)
    //         {
    //         social_msg::robot_status robot_status;
    //         robot_status.body1 = config.body_1_energy ; 
    //         robot_status.body2 = config.body_2_gaze ; 
    //         robot_status.body3 = config.body_3_expression ; 
    //         robot_status.body4 = config.body_4_body ; 
    //         robot_status.body5 = config.body_5_arm ; 
    //         robot_status.body6 = config.body_6_leg ; 
    //         robot_status.body7 = config.body_7_blank ; 
    //         robot_status.idleState = config.body_idleState ; 
    //         pub_body.publish(robot_status);
    //         }
    
    // //情感
    //         if(config.O_____emotion_switch == true)
    //         {
    //         social_msg::robot_emotion  robot_emotion ;
    //         robot_emotion.emotion1 = config.emotion_1_Joy;
    //         robot_emotion.emotion2 = config.emotion_2_Trust;
    //         robot_emotion.emotion3 = config.emotion_3_Suprise;
    //         robot_emotion.emotion4 = config.emotion_4_Sadness;
    //         robot_emotion.emotion5 = config.emotion_5_Anger;
    //         robot_emotion.emotion6 = config.emotion_6_Fear;
    //         robot_emotion.emotion7 = config.emotion_7_Disgust;
    //         robot_emotion.emotion8 = config.emotion_8_Boring;
    //         pub_emotion.publish(robot_emotion);
    //         }

    // //需求
    //         if(config.need_switch == true)
    //         {
    //         social_msg::need_msg  need ;
    //         need.need_name = config.need_name;
    //         need.person_name = config.need_person_name;
    //         need.IDtype = config.need_person_IDtype;
    //         need.weight = config.need_weight;
    //         need.person_emotion = config.need_person_emotion;
    //         need.rob_emotion = config.need_rob_emotion;
    //         need.rob_emotion_intensity = config.need_rob_emotion_intensity;   //TODO：
    //         need.speech = config.need_speech;
    //         // need.speech_ = config.need_speech;
    //         need.qt_order = config.qt_order;
    //         need.satisfy_value = config.satisfy_value_2;
    //         pub_needlist.publish(need);
    //         }
    }
}
 
int main(int argc, char **argv)
{
        ros::init(argc, argv, "need_model_dynamic_reconfigure");
        ros::NodeHandle n;
        // 感知信息
        pub_perception  = n.advertise<social_msg::perception_msg>("perception_msg", 10);
        pub_attitude  = n.advertise<social_msg::attitude_msg>("attitude", 10);
        pub_body  = n.advertise<social_msg::robot_status>("robot_status", 10);
        pub_idleState  = n.advertise<social_msg::idleState>("idleState", 10);
        
        // 情感
        pub_emotion  = n.advertise<social_msg::robot_emotion>("robot_emotion", 10);
        // 需求
        pub_needlist  = n.advertise<social_msg::need_msg>("need_lists", 10);
        // 行为
        
        dynamic_reconfigure::Server<robot_emotion_publisher::robot_emotion_publisherConfig> server;
        dynamic_reconfigure::Server<robot_emotion_publisher::robot_emotion_publisherConfig>::CallbackType f;
        sleep(2);
        f = boost::bind(&callback, _1, _2);
        server.setCallback(f);
        ros::spin();
        return 0;
}
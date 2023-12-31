#include "BehaviorManager.h"

bool JudgeStampSame(const std_msgs::Header& header1, const std_msgs::Header& header2) {
    return (header1.stamp.sec == header2.stamp.sec);
}

// Read behavior data from .json file and save in mmbehaviorsLibrary.
void BehaviorManager::ReadInBehaviorLibrary(const string &config_file)
{
    cout << "Reading behavior data..." << endl;
    Json::Value root;
    Json::Reader reader;
    fstream ifs(config_file);

    string strjson;
    if (!ifs.is_open()) {
        cout << "Fail to open: " << config_file << endl;
        return;
    }

    string strline;
    while (getline(ifs, strline)) {
        strjson += strline;
    }
    ifs.close();

    if (!reader.parse(strjson, root)){
		cout << "Fail to analysis json: " << config_file << endl;
		ifs.close();
        return;
    }

    cout << "Loaded!" << endl;

    int behavior_num = root["behavior"].size();

    printInColor("\nBehavior list: ", BLUE);
    cout << "(name, phase)" << endl;

    for(int i=0; i<behavior_num; i++){
        // printf("i = %d", i);
        auto root_i = root["behavior"][i];
        Behavior behavior;
        string behavior_name = root_i["name"].asString();
        msbehaviorsCatalog.insert(behavior_name);
        
        behavior.name = behavior_name;
        behavior.weight = root_i["weight"].asDouble();
        behavior.type = root_i["type"].asString();
        behavior.is_light = root_i["is_light"].asBool();

        // cout << "set is_light" << endl;
        
        auto root_sub = root_i["subBehavior"];
        int sub_behavior_num = root_sub.size();
        vector<SubBehavior> sub_behaviors;

        // cout << "set sub_behaviors" << endl;

        // printf("sub_behavior_num = %d\n", sub_behavior_num);

        for(int j=0; j<sub_behavior_num; j++){
            // readinArmConfig(root_sub[j],sub_behaviors);
            auto root_sub_j = root_sub[j];
            SubBehavior sub_behavior;
            sub_behavior.discription = root_sub_j["discription"].asString();

            vector<string> actuators = {"Gaze", "Emotion", "Voice", "Manipulator", "Mover"};

            for (int i = 0; i < actuators.size(); i++)
            {
                string actuator = actuators[i];
                sub_behavior.mActuators[i]->is_necessary = root_sub_j[actuator]["is_necessary"].asBool();
                sub_behavior.mActuators[i]->weight = root_sub_j[actuator]["weight"].asDouble();
            }

            sub_behaviors.push_back(sub_behavior);

            for (int i = 0; i < 5; i++) {
                behavior.necessary_count[i] += sub_behavior.mActuators[i]->is_necessary?1:0;
            }
        }
        
        behavior.subBehaviorSeries = sub_behaviors;
        behavior.total_phase = behavior.subBehaviorSeries.size();
        mmbehaviorsLibrary.insert(pair<string,Behavior>(behavior_name, behavior));
        cout << "  - " << behavior_name << ", " << sub_behavior_num << endl;
    }
    cout << endl;
    cout << "Finish mmbehaviorsLibrary creation." << endl;
}

Behavior* BehaviorManager::GetBehaviorByName(string name)
{
    auto behavior_index = mmbehaviorsLibrary.find(name);
    if (behavior_index == mmbehaviorsLibrary.end()){
        // cout << " fails." << endl;
        return nullptr;
    }
    // cout << " succeeds." << endl << endl;
    Behavior* new_behavior = new Behavior(behavior_index->second, false);
    // new_behavior->configureByNeedMsg();
    return new_behavior;
}

// Handle need message and generate the behavior instance 
// with mmbehaviorsLibrary data and need message's configuration.
bool BehaviorManager::ReadInNewNeed(const behavior_module::need_msg &msg)
{
    string need_name = msg.need_name;
    printInColor("【Add new behavior】", BLUE);
    cout << need_name;

    // 1. Query the need in mmbehaviorsLibrary.
    Behavior* new_behavior = GetBehaviorByName(need_name);
    // new_behavior->configureByNeedMsg(msg);
    string result = (new_behavior != nullptr)?", succeeds.":", fails.";
    cout << result << endl << endl;

    if (new_behavior == nullptr)
    {
        return false;
    }
    else {
        (*new_behavior).configureByNeedMsg(msg);

        int insert_location = AddNewBehavior(*new_behavior);
        ComputeParallel();

        if (insert_location <= mParallelNum){
            UpdateBehaviorPub();
        }
        else
        {
            // debug: zhjd 以下内容用于插入行为时的“您先稍等下”
            // TODO: 改进方向：（1）添加behavior name的筛选，如果是“问好”之类的简单行为，则不生成“您先稍等下”
            // （2）添加behavior person name的比较，如果是对同一个人，则不生成“您先稍等下”
            //  (3)家庭场景，  不生成“您先稍等下”
            // if (new_behavior->type=="INTERACTION")
            // {
            //     string need_name = "TellToWait";
            //     Behavior* tell_behavior = GetBehaviorByName(need_name);
            //     if (tell_behavior!=nullptr) {
            //         tell_behavior->target = new_behavior->target;
            //         tell_behavior->target_angle = new_behavior->target_angle;
            //         tell_behavior->target_distance = new_behavior->target_distance;
            //         AddNewBehavior(*tell_behavior);
            //         ComputeParallel();
            //         UpdateBehaviorPub();
            //     }
            //     else {
            //         cout << "No behavior called " << need_name;
            //         return false;
            //     }
            // }
        }
        PrintBehaviorseries();
        return true;
    }
}

// Delete light behavior, add the new behavior instance into mvbehaviorsTotal
// and judge whether it is among the parallel behaviors to be execute.
int BehaviorManager::AddNewBehavior(Behavior &new_behavior)
{
    // Judge if mvbehaviorsTotal is empty.
    {
        unique_lock<mutex> lock(mutexCheckNewBeh);
        mbNewBehFlag = true;
    }

    if (mvbehaviorsTotal.empty()) {
        TellIdleState(false, nullptr);
        InsertBehavior(new_behavior);
        return 1;
    }

    // Judge if a light behavior exists.
    if (mvbehaviorsTotal[0].is_light){
        InsertBehavior(new_behavior);
        return 1;
    }
    
    int insert_location = InsertBehavior(new_behavior);
    return insert_location;
}

void BehaviorManager::TellIdleState(bool state, Behavior *completedBehavior)
{
    //TODO: tell EmotionModule the idle state
    behavior_module::idleState msg;
    msg.idleState = state;
    // msg.idleState = true;
    if (completedBehavior == nullptr) {
        msg.hehavior_name = "None";
    }
    else {
        msg.hehavior_name = completedBehavior->name;
        msg.person_name = completedBehavior->target;
        msg.IDtype = completedBehavior->IDtype;
        msg.target_angle = completedBehavior->target_angle;
        msg.target_distance = completedBehavior->target_distance;
        msg.person_emotion = completedBehavior->person_emotion;
        msg.satisfy_value = completedBehavior->satisfy_value;
    }
    publisher_idlestate_.publish(msg);
    printInColor("【Sent IdleState】 ", BLUE);
    cout << state << ", Behavior : " << msg.hehavior_name << endl << endl;
}

void BehaviorManager::WaitToUpdate(float wait_seconds)
{
    {
        unique_lock<mutex> lock(mutexCheckNewBeh);
        mbNewBehFlag = false;
    }
    sleep(wait_seconds);
    {
        unique_lock<mutex> lock(mutexCheckNewBeh);
        if (!mbNewBehFlag) {
            ComputeParallel();
            UpdateBehaviorPub();
        }
    }
}

// Make the necessary judge, update the parallel behaviors to be execute
// and finally pub them.
void BehaviorManager::UpdateBehaviorPub()
{
    cout << "mParallelNum = " << mParallelNum << endl;
    if (mvbehaviorsTotal.empty()){
        return;
    }

    behavior_module::behavior_msg msg;

    if (!mvbehaviorsCurrent.empty()){
        if(!mbPauseFlag)
        {
            msg.name = "Stop_All_Actions_Now";
            publisher_behavior_.publish(msg);
            PrintBehaviorMsgInfo(msg);
            mbPauseFlag = true;
        }
        return;
    }

    vector<behavior_module::behavior_msg> msgs;

    for(int i = 0 ; i < mParallelNum ; i++){
        mvbehaviorsCurrent.push_back(mvbehaviorsTotal[i]);
        msg = GenerateBehaviorMsg(mvbehaviorsTotal[i]);

        for(int j = 0 ; j < 5 ; j ++) {
            msg.occupancy[j] = 0;
        }
        msgs.push_back(msg);
    }
    // cout << "mviOccupancy : {" << mviOccupancy[0];
    // for(int i = 1 ; i < 5 ; i++) {cout << "," << mviOccupancy[i];}
    // cout << "}" << endl;

    for(int i = 0 ; i < 5 ; i++){
        msgs[mviOccupancy[i]-1].occupancy[i] = 1;
    }

    for(auto &one_msg:msgs)
    {
        publisher_behavior_.publish(one_msg);
        PrintBehaviorMsgInfo(one_msg);
    }
}

// Handle feedback from perform_module:
// 1. Update progress of sub-behaviors into mvbehaviorsTotal and parallelBehaviorSeries.
// 2. Delete completed behaviors in mvbehaviorsTotal and parallelBehaviorSeries.
// 3. Start function UpdateBehaviorPub when necessary.
void BehaviorManager::behavior_feedback_callback(const behavior_module::behavior_feedback_msg &msg)
{
    printInColor("【BehaviorFeedback】", GREEN);
    cout << msg.hehavior_name << "," << (int)msg.current_phase << endl;

    // to be tested: varify behavior by stamp
    bool rightBehaviorFlag = false;
    bool completeFlag = false;
    vector<Behavior>::iterator itor = mvbehaviorsTotal.begin();

    for (auto &behavior : mvbehaviorsTotal){
        if (behavior.name == msg.hehavior_name && JudgeStampSame(behavior.header, msg.header)){
            rightBehaviorFlag = true;
            if(msg.current_phase >= behavior.current_phase && 
                            msg.current_phase <= behavior.total_phase)
            {
                if (msg.current_phase == behavior.total_phase || behavior.is_light) {
                    completeFlag = true;
                }
                else {
                    for (int i = behavior.current_phase; i < msg.current_phase; i++)
                        for (int j = 0; j < 5; j++)
                            behavior.necessary_count[j] -= (behavior.subBehaviorSeries[i].mActuators[j]->is_necessary)?1:0;
                }
                behavior.current_phase = msg.current_phase;
                // PrintBehaviorseries();
                break;
            }
            else
            {
                cout << "Right behavior, wrong phase." << endl;
                return;
            }
        }
        itor++;
    }

    if (completeFlag == true){
        bool idleState = (mvbehaviorsTotal.size() == 1);
        TellIdleState(idleState, &(*itor));
        mvbehaviorsTotal.erase(itor);
    }

    if (!rightBehaviorFlag){
        cout << "There is no behavior \"" << msg.hehavior_name << "\" with stamp.sec==" <<  msg.header.stamp.sec << endl;
        PrintBehaviorseries();
        return;
    }

    rightBehaviorFlag = false;
    itor = mvbehaviorsCurrent.begin();

    for (auto &behavior : mvbehaviorsCurrent){
        if(JudgeStampSame(behavior.header, msg.header)){

            rightBehaviorFlag = false;
            
            if (msg.current_phase == behavior.total_phase || mbPauseFlag){
                // finish one behavior
                mvbehaviorsCurrent.erase(itor);
                if(mvbehaviorsCurrent.empty() && !mvbehaviorsTotal.empty()){
                    if(!mbPauseFlag){
                        mParallelNum = 1;
                    }
                    mviOccupancy = {1,1,1,1,1};
                    mbPauseFlag = false;
                    // TODO: SetWaitTimeHere
                    mtWaitToUpdate = new thread(&BehaviorManager::WaitToUpdate, this, 5);
                    mtWaitToUpdate->detach();
                }
            }
            else{
                behavior.current_phase = msg.current_phase;
            }
            PrintBehaviorseries();
            return;
        }
        itor++;
    }
    cout << "   No behavior \"" << msg.hehavior_name << "\" in current behavior series to be paused." << endl << endl;
    printInColor("- BehaviorsCurrent: \n", CYAN);
    PrintBehaviors(mvbehaviorsCurrent);
    cout << endl;
    return;
}

void BehaviorManager::PrintBehaviorLibraryInfo()
{
    cout << "Behavior num : " << msbehaviorsCatalog.size() << "\n\n";
    cout << "Behavior names:\n";
    for(auto &name:msbehaviorsCatalog){
        cout << "   " << name << ",\n";
    }
    cout << "\n    'behavior':[\n";
    cout << "    {\n";
    for(auto &behavior_map:mmbehaviorsLibrary){
        auto behavior = behavior_map.second;
        cout << "        {\n";
        cout << "            'name' : '" << behavior.name << "',\n";
        cout << "            'weight' : '" << behavior.weight << "',\n";
        cout << "            'type' : '" << behavior.type << "',\n";
        cout << "            'subBehavior':[\n";
        for (auto &sub_behavior:behavior.subBehaviorSeries){
            cout << "                {\n";
            cout << "                    'discription'   : '" << sub_behavior.discription << "',\n";
            vector<string> actuators = {"Gaze", "Emotion", "Voice", "Manipulator", "Mover"};
            for (int  i = 0; i < 5; i++) {
                cout << "                    " << actuators[i] << "          : {";
                if (sub_behavior.mActuators[i]->is_necessary) cout << "true";
                else cout << "false";
                cout << "," << sub_behavior.mActuators[i]->weight << "},\n";
            }
            cout << "                },\n";
        }
        cout << "        },\n";
    }
    cout << "    }\n";
}

behavior_module::behavior_msg BehaviorManager::GenerateBehaviorMsg(const Behavior& beh)
{
    behavior_module::behavior_msg msg;
    {
        msg.header.frame_id = beh.header.frame_id;
        msg.header.seq = beh.header.seq;
        msg.header.stamp.sec = (int)(beh.header.stamp.sec);
        msg.header.stamp.nsec = (int)(beh.header.stamp.nsec);
    }
    msg.name = beh.name;
    msg.scene = beh.scene;
    msg.type = beh.type;
    msg.current_phase = beh.current_phase;
    msg.total_phase = beh.total_phase;
    msg.target = beh.target;
    msg.target_angle = beh.target_angle;
    msg.target_distance = beh.target_distance;
    msg.speech = beh.speech;
    msg.rob_emotion = beh.rob_emotion;
    msg.rob_emotion_intensity = beh.rob_emotion_intensity;
    msg.attitude = beh.attitude;
    msg.move_speed = beh.move_speed;
    msg.distance = beh.distance;
    msg.voice_speed = beh.voice_speed;
    return msg;
}

int BehaviorManager::InsertBehavior(Behavior &new_behavior)
{
    int index = 0;
    unique_lock<mutex> lock(mutexBehaviorsTotal);
    int num = mvbehaviorsTotal.size();
    if(!num){
        mvbehaviorsTotal.emplace_back(new_behavior);
        // PrintBehaviorseries();
        return 1;
    }
    double new_weight = new_behavior.weight;
    while(mvbehaviorsTotal[index].weight >= new_weight && index < num ){
        index++;
    }

    if(index==num){
        mvbehaviorsTotal.push_back(new_behavior);
        // PrintBehaviorseries();
        return index + 1;
    }
    else{
        mvbehaviorsTotal.push_back(mvbehaviorsTotal.back());
    }
    for(int i = num - 1 ; i > index ; i--){
        mvbehaviorsTotal[i] = mvbehaviorsTotal[i - 1];
    }
    mvbehaviorsTotal[index] = new_behavior;
    // PrintBehaviorseries();
    return index + 1;
}

int BehaviorManager::ComputeParallel()
{
    int parallel_num = 0;
    vector<int> count = {0,0,0,0,0};
    mviOccupancy = vector<int>{1,1,1,1,1};
    vector<int> local_count;
    for (auto &behavior:mvbehaviorsTotal)
    {
        for(int i = 0 ; i < 5 ; i++){
            if(behavior.necessary_count[i]){
                local_count.push_back(i);
                if(count[i] == 1){
                    // cout << "Compute parallel_num = " << parallel_num << endl;
                    mParallelNum = parallel_num;
                    return parallel_num;
                }
            }
        }
        parallel_num ++;
        for(auto &index:local_count) {
            count[index] ++;
            mviOccupancy[index] = parallel_num;
        }
    }
    cout << "Compute parallel_num = " << parallel_num << endl;
    mParallelNum = parallel_num;
    return parallel_num;
}

void BehaviorManager::PrintBehaviorseries()
{
    printInColor("【printBehaviorList】\n", BLUE);
    if(mvbehaviorsTotal.empty()){
        cout << "   BehaviorSeries is empty now." << endl;
        return;
    }
    printInColor("- BehaviorsTotal: \n", CYAN);
    PrintBehaviors(mvbehaviorsTotal);
    printInColor("- BehaviorsCurrent: \n", CYAN);
    PrintBehaviors(mvbehaviorsCurrent);

    cout << endl;
    cout << " - 行为停止后有待并行的行为数量 :" << mParallelNum << endl;
    cout << " - 当前正在并行的行为数量 : " << mvbehaviorsCurrent.size() << endl;
    cout << " - PauseFlag : " << mbPauseFlag << endl;
    cout << endl;
    return;
}

void BehaviorManager::PrintBehaviorMsgInfo(behavior_module::behavior_msg msg)
{
    printInColor("【Sent behavior_msg】", BLUE);
    cout << "   " <<  msg.name << "\t" << msg.target << "\t" << msg.type << "\t" << (int)msg.current_phase << "/" << (int)msg.total_phase << "\t";
    cout << "{" << (int)msg.occupancy[0];
    for(int i = 1 ; i < 5 ; i++){
        cout << "," << (int)msg.occupancy[i];
    }
    cout << "}" << endl << endl;
}

void BehaviorManager::PrintBehaviors(vector<Behavior> &behaviors)
{
    cout << "   " << "Order\t" << "weight\t" << "phase\t" << "necessary\t" << "name\t" << "target\t" << "stamp.secs\t" << endl;
    int order = 1;
    for(auto &behavior: behaviors){
        cout << "       ";
        cout << order << "\t" 
             << behavior.weight << "\t"
             << behavior.current_phase << "/" << behavior.total_phase << "\t"
             << "{" << behavior.necessary_count[0] ;
        for(int i = 1 ; i < 5 ; i++){
            cout << "," << behavior.necessary_count[i];
        }
        cout << "} \t";
        cout << behavior.name << "\t";
        if (behavior.target=="")
            cout << "None" << "\t";
        else
            cout << behavior.target << "\t";
        cout << behavior.header.stamp.sec << "\t";
        cout << endl;
        order ++;
    }
}
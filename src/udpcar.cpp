//
// Created by zzj on 2021/10/27.
//
#include "udpcar.h"
namespace carnet {
    udp_car::udp_car(const ros::NodeHandle &nh,std::string node_name)//构造函数
            : nh_(nh),node_name_(node_name) {
        memset(this->data_to_send, 0, sizeof(this->data_to_send));
        this->data_to_send[0] = 0b00001000;
        this->data_to_send[1] = 0b00000000;
        this->data_to_send[2] = 0b00000000;
        this->data_to_send[3] = 0X06;
        this->data_to_send[4] = 0X01;
        this->data_to_send[5] = 0x23;
        this->data_to_send[6] = 0x00;
        this->data_to_send[7] = 0x20;
        vL._int = vR._int = 0;
        local_ip = "192.168.1.101";
        local_port = 8001;
        remote_ip = "192.168.1.10";
        remote_port = 4001;
        initSocket();
        //turtle_sub = nh_.subscribe<geometry_msgs::Twist>("/turtle1/cmd_vel", 1, &udp_car::joyCallback, this);
        sub = nh_.subscribe("/joy",100,&udp_car::joyCallback,this);
        loop_timer_ = nh_.createTimer(ros::Duration(0.02), boost::bind(&udp_car::timerCb, this));
    }

    //void udp_car::joyCallback(const geometry_msgs::Twist keyboard) {
      // vL._int = -(keyboard.linear.x + keyboard.angular.z)/2*1000;//vL._int = -(keyboard.linear.x *2 + keyboard.angular.z *0.7)/2/4*1000;
      // vR._int = -(keyboard.linear.x - keyboard.angular.z)/2*1000;//vR._int = -(keyboard.linear.x *2 - keyboard.angular.z *0.7)/2/4*1000;
      // cmd.is_update = true;
      // cmd.last_timestamp = ros::WallTime::now().toSec();
   // }
    
    void udp_car::joyCallback(const sensor_msgs::Joy::ConstPtr& con){//回调函数
    	
    	float vel_linear = con->axes[4];
    	float vel_angular = con->axes[3];
    	double B=1.2;
    	vL._int = (short)((-vel_linear-vel_angular*B/2)*100);
    	vR._int = (short)((-vel_linear+vel_angular*B/2)*100);
    	cmd.is_update = true;
        cmd.last_timestamp = ros::WallTime::now().toSec();
        
    }

    void udp_car::sendmsgs() {
        static int counter = -1;
        counter ++;
        counter = (counter =1) ? counter = 0: counter;
        this->cmd.is_update &= (ros::WallTime::now().toSec() - cmd.last_timestamp) * 1000 < cmd.elapse_time_;
        if (this->cmd.is_update){
            printf(" I am controling.....");
            switch(counter){
                case 0:{
                    this->data_to_send[8] = 0x01;
                    this->data_to_send[9] = vL._char[0];
                    this->data_to_send[10] = vL._char[1];
                    printf("vL=: %d,vL[low8]: %d,vL[higher8]: %d\n",vL._int,vL._char[0],vL._char[1]);
                    sendto(this->CarNetSocket, data_to_send, sizeof(data_to_send), 0, (struct sockaddr *) &addr_remote,
                           addr_remote_len);
                }
                case 1:{
                    this->data_to_send[8] = 0x02;
                    this->data_to_send[9] = vR._char[0];
                    this->data_to_send[10] = vR._char[1];
                    printf("vR=: %d,vR[low8]: %d,vR[higher8]: %d\n",vR._int,vR._char[0],vR._char[1]);
                    sendto(this->CarNetSocket, data_to_send, sizeof(data_to_send), 0, (struct sockaddr *) &addr_remote,
                           addr_remote_len);
                }
                default: {
                    break;
                }
            }
        } else{
            switch(counter){
                case 0:{
                    printf("Not Receive... \n");
                    this->data_to_send[8] = 0x01;
                    //this->data_to_send[9] = 0x00;
                    this->data_to_send[9] = vL._char[0];
                    //this->data_to_send[10] = 0x00;
                    this->data_to_send[10] = vL._char[1];
                    printf("vL[low8]: %d,vL[higher8]: %d\n",this->data_to_send[9],this->data_to_send[10]);
                    sendto(this->CarNetSocket, data_to_send, sizeof(data_to_send), 0, (struct sockaddr *) &addr_remote,
                           addr_remote_len);
                }
                case 1:{
                    this->data_to_send[8] = 0x02;
                    //this->data_to_send[9] = 0x00;
                    this->data_to_send[9] = vR._char[0];
                    //this->data_to_send[10] = 0x00;
                    this->data_to_send[10] = vR._char[1];
                    printf("vR[low8]: %d,vR[higher8]: %d\n",this->data_to_send[9],this->data_to_send[10]);
                    sendto(this->CarNetSocket, data_to_send, sizeof(data_to_send), 0, (struct sockaddr *) &addr_remote,
                           addr_remote_len);
                }
                default: {
                    break;
                }
            }
        }
    }


void udp_car::timerCb() {
        ROS_INFO_ONCE("udp car start");
         sendmsgs();
    }
bool udp_car::initSocket() {

        this->addr_remote_len = sizeof(this->addr_remote);

        //// create a socket
        this->CarNetSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (this->CarNetSocket < 0) {
            perror("create CarNetSocket failed!\n");
            return false;
        } else {
            std::cout << "create CarNetSocket succeed!" << std::endl;
        }
        //// set the local address
        memset((char *) &addr_local, 0, sizeof(addr_local));
        this->addr_local.sin_addr.s_addr = inet_addr(local_ip.c_str());// htonl(INADDR_ANY);
        this->addr_local.sin_family = AF_INET;
        this->addr_local.sin_port = htons(local_port);

        //// bind the socket with local address
        if (bind(CarNetSocket, (sockaddr *) &addr_local, sizeof(sockaddr)) < 0) {
            perror("bind the CarNetSocket failed!");
            return false;
        } else {
            std::cout << "bind the CarNetSocket succeed!" << std::endl;
            std::cout << "Local Port : " << this->local_port << std::endl;
        }
        //// set the remote address
        memset(&addr_remote, 0, sizeof(addr_remote));
        this->addr_remote.sin_addr.s_addr = inet_addr(remote_ip.c_str());
        this->addr_remote.sin_family = AF_INET;
        this->addr_remote.sin_port = htons(remote_port);

        std::cout << "Remote IP  : " << this->remote_ip.c_str() << std::endl;
        std::cout << "Remote Port: " << this->remote_port << std::endl;

        return true;
    }
}
int main(int argc, char **argv) {
    std::string node_name = "udp_car";
    ros::init(argc, argv, node_name);
    ros::NodeHandle nh("");
    carnet::udp_car sender(nh, node_name);//实例化udp_car对象 sender
    ROS_INFO("Initialized sender node.");
    ros::spin();
}
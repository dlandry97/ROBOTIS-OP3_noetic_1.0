/* ROS API Header */
#include <ros/ros.h>
// #include <std_msgs/String.h>

/* ROBOTIS Controller Header */
// #include "robotis_controller/robotis_controller.h"

/* Sensor Module Header */
// #include "open_cr_module/open_cr_module.h"

/* Motion Module Header */
// #include "op3_base_module/base_module.h"
// #include "op3_head_control_module/head_control_module.h"
// #include "op3_action_module/action_module.h"
// #include "op3_walking_module/op3_walking_module.h"
// #include "op3_direct_control_module/direct_control_module.h"
// #include "op3_online_walking_module/online_walking_module.h"
// #include "op3_tuning_module/tuning_module.h"


#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "../../../ROBOTIS-Framework/robotis_controller/include/robotis_controller/robotis_controller.h"
// #include "../../../ROBOTIS-Framework/robotis_controller/robotis_controller.h"
#include "../../../DynamixelSDK/c++/include/dynamixel_sdk/dynamixel_sdk.h"
// #include "../../../dynamixel_sdk.h"                                  // Uses DYNAMIXEL SDK library

// Control table address
#define ADDR_PRO_TORQUE_ENABLE          562                 // Control table address is different in Dynamixel model
#define ADDR_PRO_GOAL_POSITION          596
#define ADDR_PRO_PRESENT_POSITION       611

// Protocol version
#define PROTOCOL_VERSION                2.0                 // See which protocol version is used in the Dynamixel

// Default setting
#define DXL_ID                          1                   // Dynamixel ID: 1
#define BAUDRATE                        1000000
#define DEVICENAME                      "/dev/ttyUSB0"      // Check which port is being used on your controller
                                                            // ex) Windows: "COM1"   Linux: "/dev/ttyUSB0"

#define TORQUE_ENABLE                   1                   // Value for enabling the torque
#define TORQUE_DISABLE                  0                   // Value for disabling the torque
#define DXL_MINIMUM_POSITION_VALUE      -150000             // Dynamixel will rotate between this value
#define DXL_MAXIMUM_POSITION_VALUE      150000              // and this value (note that the Dynamixel would not move when the position value is out of movable range. Check e-manual about the range of the Dynamixel you use.)
#define DXL_MOVING_STATUS_THRESHOLD     20                  // Dynamixel moving status threshold

#define ESC_ASCII_VALUE                 0x1b

using namespace robotis_framework;
using namespace dynamixel;
// using namespace robotis_op;

const int BAUD_RATE = 2000000;

const int SUB_CONTROLLER_ID = 200;
const int DXL_BROADCAST_ID = 254;
const int DEFAULT_DXL_ID = 1;
const std::string SUB_CONTROLLER_DEVICE = "/dev/ttyUSB0";
const int POWER_CTRL_TABLE = 24;
const int RGB_LED_CTRL_TABLE = 26;
const int TORQUE_ON_CTRL_TABLE = 64;

bool g_is_simulation = false;
int g_baudrate;
std::string g_offset_file;
std::string g_robot_file;
std::string g_init_file;
std::string g_device_name;

ros::Publisher g_init_pose_pub;
ros::Publisher g_demo_command_pub;



class Collect
{
  public:

    Collect()
    {
      std::string dev_desc_dir_path = ros::package::getPath("robotis_device") + "/devices";

      // load robot info : port , device
      robot_ = new Robot(robot_file_path, dev_desc_dir_path);
      // timer = nh.createTimer(ros::Duration(0.1 ),timer_cb);
      _current_pub = nh.advertise<std::vector<double>>("/collection/currents", 0,this);

      // nh.param<bool>("gazebo", controller->gazebo_mode_, false);
      // g_is_simulation = controller->gazebo_mode_;

      /* real robot */
      if (g_is_simulation == false)
      {
        // open port
        PortHandler *port_handler = (PortHandler *) PortHandler::getPortHandler(g_device_name.c_str());
        bool set_port_result = port_handler->setBaudRate(BAUD_RATE);
        if (set_port_result == false)
          ROS_ERROR("Error Set port");

        PacketHandler *packet_handler = PacketHandler::getPacketHandler(PROTOCOL_VERSION);

        usleep(100 * 1000);

        // set RGB-LED to GREEN
        int led_full_unit = 0x1F;
        int led_range = 5;
        int led_value = led_full_unit << led_range;
        int _return = packet_handler->write2ByteTxRx(port_handler, SUB_CONTROLLER_ID, RGB_LED_CTRL_TABLE, led_value);

        if(_return != 0)
          ROS_ERROR("Fail to control LED [%s]", packet_handler->getRxPacketError(_return));

        port_handler->closePort();
      }
      
  

      nh.param<std::string>("device_name", g_device_name, SUB_CONTROLLER_DEVICE);
      nh.param<int>("baud_rate", g_baudrate, BAUD_RATE);


      ROS_INFO("collection->init");
      while_func();

    }

    void get_current(){
      PortHandler *port_handler = (PortHandler *) PortHandler::getPortHandler(g_device_name.c_str());
      bool set_port_result = port_handler->setBaudRate(BAUD_RATE);
      if (set_port_result == false)
        ROS_ERROR("Error Set port");

      PacketHandler *packet_handler = PacketHandler::getPacketHandler(PROTOCOL_VERSION);

      std::vector<double> robot_currents;
      robot_currents.resize(12);

      for (auto& it : robot_->dxls_)
      {
        std::string joint_name = it.first;
        Dynamixel *dxl = it.second;
      
          // int dxl_comm_result = packet_handler->read4ByteTxRx(port_handler, DXL_ID, 126, (uint32_t*)&dxl_present_position, &dxl_error);
          int dxl_comm_result = packet_handler->read4ByteTxRx(port_handler, dxl->id_, item->address_, &read_data, error);
          if (dxl_comm_result != COMM_SUCCESS)
          {
              packet_handler->printTxRxResult(dxl_comm_result);
              robot_currents[i] = dxl_comm_result;
          }
          else if (dxl_error != 0)
          {
              ROS_WARN("dxl_error");
              packet_handler->printRxPacketError(dxl_error);
          }
      }
      _current_pub.publish(robot_currents);
      port_handler->closePort();


          // printf("[ID:%03d] GoalPos:%03d  PresPos:%03d\n", DXL_ID, dxl_goal_position[index], dxl_present_position);
    }

    void timer_cb(const ros::TimerEvent&){
      ROS_INFO("Time triggered");
      get_current();
    }

    void while_func(){
      ros::Rate r(rate);
      while(ros::ok()){
        get_current();
        ros::spinOnce();
        r.sleep();
      }
      

    }



  private:
    ros::NodeHandle nh;
    // ros::Timer timer;
    ros::Publisher _current_pub;
    std::vector<double> currents;
    int rate;


};

int main(int argc, char **argv){
  ros::init(argc,argv,"Collect");
  Collect node;
  ros::spin();
  return 0;
}








// int main(int argc, char **argv)
// {


//   ros::Timer timer = nh.createTimer(ros::Duration(0.1 ),timer_cb);
  
//   _current_pub = nh.advertise<std::vector>("/collection/currents", 0);

//   ROS_INFO("current_collection->init");
  

//   /* Load ROS Parameter */

  
  

  
// //   ros::Subscriber dxl_torque_sub = nh.subscribe("/robotis/dxl_torque", 1, dxlTorqueCheckCallback);
// //   g_init_pose_pub = nh.advertise<std_msgs::String>("/robotis/base/ini_pose", 0);
// //   g_demo_command_pub = nh.advertise<std_msgs::String>("/ball_tracker/command", 0);

//   // nh.param<bool>("gazebo", controller->gazebo_mode_, false);
//   // g_is_simulation = controller->gazebo_mode_;

//   // /* real robot */
//   // if (g_is_simulation == false)
//   // {
//   //   // open port
//   //   PortHandler *port_handler = (PortHandler *) PortHandler::getPortHandler(g_device_name.c_str());
//   //   bool set_port_result = port_handler->setBaudRate(BAUD_RATE);
//   //   if (set_port_result == false)
//   //     ROS_ERROR("Error Set port");

//   //   PacketHandler *packet_handler = PacketHandler::getPacketHandler(PROTOCOL_VERSION);

//   //   // // power on dxls
//   //   // int torque_on_count = 0;

//   //   // while (torque_on_count < 5)
//   //   // {
//   //   //   int _return = packet_handler->write1ByteTxRx(port_handler, SUB_CONTROLLER_ID, POWER_CTRL_TABLE, 1);

//   //   //   if(_return != 0)
//   //   //     ROS_ERROR("Torque on DXLs! [%s]", packet_handler->getRxPacketError(_return));
//   //   //   else
//   //   //     ROS_INFO("Torque on DXLs!");

//   //   //   if (_return == 0)
//   //   //     break;
//   //   //   else
//   //   //     torque_on_count++;
//   //   // }

//   //   usleep(100 * 1000);

//   //   // set RGB-LED to GREEN
//   //   int led_full_unit = 0x1F;
//   //   int led_range = 5;
//   //   int led_value = led_full_unit << led_range;
//   //   int _return = packet_handler->write2ByteTxRx(port_handler, SUB_CONTROLLER_ID, RGB_LED_CTRL_TABLE, led_value);

//   //   if(_return != 0)
//   //     ROS_ERROR("Fail to control LED [%s]", packet_handler->getRxPacketError(_return));

//   //   port_handler->closePort();
//   // }
 

//   while (ros::ok())
//   {
//     // usleep(1 * 1000);
//     get_current();

//     ros::spin();
//   }

//   return 0;
// }


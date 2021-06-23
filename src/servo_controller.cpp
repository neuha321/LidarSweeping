/*
 * This script shares data from the Dynamixel XH430-W350-r
 * servo motor through the "current_servo_data" topic, which
 * is then processed by the servo_controller script
 */
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "dynamixel_sdk_examples/GetPosition.h"
#include "dynamixel_sdk_examples/SetPosition.h"
#include "dynamixel_sdk/dynamixel_sdk.h"

#include <sstream>

using namespace dynamixel;

// Dynamixel XH430-W350-r control table address
#define ADDR_TORQUE_ENABLE    64
#define ADDR_GOAL_POSITION    116
#define ADDR_PRESENT_POSITION 132

// Protocol version
#define PROTOCOL_VERSION      2.0

// Default setting
#define DXL_ID               1
#define BAUDRATE              57600
#define DEVICE_NAME           "/dev/ttyUSB0"

PortHandler * portHandler;
PacketHandler * packetHandler;

bool getCurrentPositionCallback( dynamixel_sdk_examples::GetPosition::Request & req,
                                    dynamixel_sdk_examples::GetPosition::Response & res)
{
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    int32_t position = 0;
    
    dxl_comm_result = packetHandler->read4ByteTxRx( portHandler, 
                                                    (uint8_t)req.id, 
                                                    ADDR_PRESENT_POSITION, 
                                                    (uint32_t *)&position, 
                                                    &dxl_error);
    if (dxl_comm_result == COMM_SUCCESS) 
    {
        ROS_INFO("current servo position: %d", position);
        res.position = position;
        return true;
    }
    else    
    {
        ROS_INFO("failed to get position: %d", dxl_comm_result);
        return false;
    }
}

void setGoalPositionCallback(const dynamixel_sdk_examples::SetPosition::ConstPtr & msg)
{
  uint8_t dxl_error = 0;
  int dxl_comm_result = COMM_TX_FAIL;
  uint32_t position = (unsigned int)msg->position;

  dxl_comm_result = packetHandler->write4ByteTxRx( portHandler, 
                                                    (uint8_t)msg->id, 
                                                    ADDR_GOAL_POSITION, 
                                                    position, 
                                                    &dxl_error);
  if (dxl_comm_result == COMM_SUCCESS) 
  {
    ROS_INFO("new goal position: %d", msg->position);
  } 
  else 
  {
    ROS_ERROR("failed to set position: %d", dxl_comm_result);
  }
}

int main(int argc, char **argv)
{
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    int32_t newGoal = 3000;
    int32_t lastPosition = 0;
    uint8_t servo_id = 1;
    ROS_INFO("Running..");
/*
    portHandler = PortHandler::getPortHandler(DEVICE_NAME);
    packetHandler = PacketHandler::getPacketHandler(PROTOCOL_VERSION);

    if (!portHandler->openPort()) 
    {
        ROS_ERROR("failed to open port");
        return -1;
    }

    if (!portHandler->setBaudRate(BAUDRATE)) 
    {
        ROS_ERROR("failed to set baudrate");
        return -1;
    }

    dxl_comm_result = packetHandler->write1ByteTxRx( portHandler, 
                                                        DXL_ID, 
                                                        ADDR_TORQUE_ENABLE, 
                                                        1, 
                                                        &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) 
    {
        ROS_ERROR("failed to enable Dynamixel torque", DXL_ID);
        return -1;
    }
*/
    ros::init(argc, argv, "servo_controller_node");
    ros::NodeHandle nh;
    ros::Rate loop_rate(10);
    ros::ServiceClient client = nh.serviceClient<dynamixel_sdk_examples::GetPosition>("get_position");
    ros::Publisher set_position_pub = nh.advertise<dynamixel_sdk_examples::SetPosition>("/set_position", 100);
    dynamixel_sdk_examples::GetPosition get_srv;
    get_srv.request.id = 1;
    dynamixel_sdk_examples::SetPosition msg;
    msg.id = servo_id;
    msg.position = newGoal;
    set_position_pub.publish(msg);
    ROS_INFO("Published");
    while(ros::ok)
    {
        if (client.call(get_srv))
        {
            if(lastPosition != get_srv.response.position)
                ROS_INFO("current position: %d/4095", get_srv.response.position);
            else if (newGoal != 3000)
            {
                newGoal = 3000;
                msg.position = newGoal;
                set_position_pub.publish(msg);
            }
            else
            {
                newGoal = 1000;
                msg.position = newGoal;
                set_position_pub.publish(msg);
            }
            lastPosition = get_srv.response.position;
        }
        
        ros::spinOnce;
        loop_rate.sleep();
    }

    portHandler->closePort();
    return 0;
}
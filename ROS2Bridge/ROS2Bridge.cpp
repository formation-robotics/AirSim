// ROS2Bridge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <chrono>
#include "rclcpp/rclcpp.hpp"

#include "geometry_msgs/msg/accel.hpp"
#include "geometry_msgs/msg/quaternion.hpp"
#include "geometry_msgs/msg/quaternion_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "geometry_msgs/msg/vector3_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/header.hpp"
#include "std_msgs/msg/string.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"

#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"

using namespace std::chrono_literals;

class ROS2AirSim : public rclcpp::Node
{
public:
	ROS2AirSim() : Node("AirSim"), helloCount_(0), stateCount_(0)
	{
		// Set up the AirSim API
		client.confirmConnection();
		client.enableApiControl(true);
		client.armDisarm(true);

		// Create ROS2 timers, publishers, and subscriptions
		helloTimer_ = this->create_wall_timer(10000ms, std::bind(&ROS2AirSim::hello_callback, this));
		helloPublisher_ = this->create_publisher<std_msgs::msg::String>("/airsim/hello");

		stateTimer_ = this->create_wall_timer(50ms, std::bind(&ROS2AirSim::state_callback, this));
		pingPublisher_ = this->create_publisher<std_msgs::msg::Bool>("/exo/airsim/drone/ping");
		accelPublisher_ = this->create_publisher<geometry_msgs::msg::Accel>("/exo/airsim/drone/accel");
		fixPublisher_ = this->create_publisher<sensor_msgs::msg::NavSatFix>("/exo/airsim/drone/gps");
		odomPublisher_ = this->create_publisher<nav_msgs::msg::Odometry>("/exo/airsim/drone/odometry");
		imuPublisher_ = this->create_publisher<sensor_msgs::msg::Imu>("/exo/airsim/drone/imu");

	}

private:
	msr::airlib::MultirotorRpcLibClient client;

	void hello_callback()
	{
		auto message = std_msgs::msg::String();
		message.data = "Hello, world! " + std::to_string(helloCount_++);
		RCLCPP_INFO(this->get_logger(), "Publishing '%s'", message.data.c_str());
		helloPublisher_->publish(message);
	}

	void state_callback()
	{
		RCLCPP_INFO(this->get_logger(), "Publishing state %s", std::to_string(stateCount_++));

		// Publish a ping from AirSim
		auto pingMessage = std_msgs::msg::Bool();
		pingMessage.data = client.ping();
		pingPublisher_->publish(pingMessage);

		// Publish the state
		auto state = client.getMultirotorState();
		
		auto accelMessage = geometry_msgs::msg::Accel();
		accelMessage.linear.x = state.kinematics_estimated.accelerations.linear.x();
		accelMessage.linear.y = state.kinematics_estimated.accelerations.linear.y();
		accelMessage.linear.z = state.kinematics_estimated.accelerations.linear.z();
		accelMessage.angular.x = state.kinematics_estimated.accelerations.angular.x();
		accelMessage.angular.y = state.kinematics_estimated.accelerations.angular.y();
		accelMessage.angular.z = state.kinematics_estimated.accelerations.angular.z();
		accelPublisher_->publish(accelMessage);
		
		auto fixMessage = sensor_msgs::msg::NavSatFix();
		fixMessage.latitude = state.gps_location.latitude;
		fixMessage.longitude = state.gps_location.longitude;
		fixMessage.altitude = state.gps_location.altitude;
		fixPublisher_->publish(fixMessage);
		
		auto odomMessage = nav_msgs::msg::Odometry();
		odomMessage.pose.pose.position.x = state.kinematics_estimated.pose.position.x();
		odomMessage.pose.pose.position.y = state.kinematics_estimated.pose.position.y();
		odomMessage.pose.pose.position.z = state.kinematics_estimated.pose.position.z();
		odomMessage.pose.pose.orientation.x = state.kinematics_estimated.pose.orientation.x();
		odomMessage.pose.pose.orientation.y = state.kinematics_estimated.pose.orientation.y();
		odomMessage.pose.pose.orientation.z = state.kinematics_estimated.pose.orientation.z();
		odomMessage.pose.pose.orientation.w = state.kinematics_estimated.pose.orientation.w();
		odomMessage.twist.twist.linear.x = state.kinematics_estimated.twist.linear.x();
		odomMessage.twist.twist.linear.y = state.kinematics_estimated.twist.linear.y();
		odomMessage.twist.twist.linear.z = state.kinematics_estimated.twist.linear.z();
		odomMessage.twist.twist.angular.x = state.kinematics_estimated.twist.angular.x();
		odomMessage.twist.twist.angular.y = state.kinematics_estimated.twist.angular.y();
		odomMessage.twist.twist.angular.z = state.kinematics_estimated.twist.angular.z();
		odomPublisher_->publish(odomMessage);

		auto imuMessage = sensor_msgs::msg::Imu();
		imuMessage.linear_acceleration.x = state.kinematics_estimated.accelerations.linear.x();
		imuMessage.linear_acceleration.y = state.kinematics_estimated.accelerations.linear.y();
		imuMessage.linear_acceleration.z = state.kinematics_estimated.accelerations.linear.z();
		imuMessage.angular_velocity.x = state.kinematics_estimated.twist.angular.x();
		imuMessage.angular_velocity.y = state.kinematics_estimated.twist.angular.y();
		imuMessage.angular_velocity.z = state.kinematics_estimated.twist.angular.z();
		imuMessage.orientation.x = state.getOrientation().x();
		imuMessage.orientation.y = state.getOrientation().y();
		imuMessage.orientation.z = state.getOrientation().z();
		imuMessage.orientation.w = state.getOrientation().w();
		imuPublisher_->publish(imuMessage);
		
	}

	size_t helloCount_;
	rclcpp::TimerBase::SharedPtr helloTimer_;
	rclcpp::Publisher<std_msgs::msg::String>::SharedPtr helloPublisher_;

	size_t stateCount_;
	rclcpp::TimerBase::SharedPtr stateTimer_;
	rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pingPublisher_;
	rclcpp::Publisher<geometry_msgs::msg::Accel>::SharedPtr accelPublisher_;
	rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr fixPublisher_;
	rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odomPublisher_;
	rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imuPublisher_;

};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	rclcpp::spin(std::make_shared<ROS2AirSim>());
	rclcpp::shutdown();

	return 0;
}
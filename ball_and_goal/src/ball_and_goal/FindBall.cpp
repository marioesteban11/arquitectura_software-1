#include "ball_and_goal/FindBall.h"


#include "geometry_msgs/Twist.h"
#include "bica/Component.h"
#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <std_msgs/Float32.h>
namespace ball_and_goal
{

FindBall::FindBall(): it_(nh_) , buffer_() , listener_(buffer)
{
    image_sub_ = it_.subscribe("/hsv/image_filtered", 1, &FindBall::imageCb, this);
    vel_pub_ = nh_.advertise<geometry_msgs::Twist>("/mobile_base/commands/velocity", 1);
}

double
FindBall::publish_detection(float x, float y)
{
    double angle;
    float x = 1;
    float y = 0;

    geometry_msgs::TransformStamped odom2bf_msg;
    try{
        odom2bf_msg = buffer_.lookupTransform("odom", "base_footprint", ros::Time(0));
    }   catch (std::exception & e)
    {
        return;
    }

    tf2::Stamped<tf2::Transform> odom2bf;
    tf2::fromMsg(odom2bf_msg, odom2bf);

    tf2::Stamped<tf2::Transform> bf2object;
    bf2object.setOrigin(tf2::Vector3(x, y ,0));
    bf2object.setRotation(tf2::Quaternion(0, 0, 0, 1));

    tf2::Transform odom2object = odom2bf * bf2object;

    geometry_msgs::TransformStamped odom2object_msg;
    odom2object_msg.header.stamp = ros::Time::now();
    odom2object_msg.header.frame_id = "odom";
    odom2object_msg.child_frame_id = "object";

    odom2object_msg.transform = tf2::toMsg(odom2object);

    broadcaster.sendTransform(odom2object_msg);

    geometry_msgs::TransformStamped bf2obj_2_msg;
    try {
        bf2obj_2_msg = buffer_.lookupTransform( "base_footprint", "object", ros::Time(0));
    } catch (std::exception & e)
    {
        return;
    }

    //angulo del robot respecto a la pelota
    angle = atan2(bf2obj_2_msg.transform.translation.y, bf2obj_2_msg.transform.translation.x);
    return angle;

}

void
FindBall::imageCb(const sensor_msgs::Image::ConstPtr& msg)
{
    if(!isActive()){
        return;
    }
    float l = 1;
    float p = 0;

    geometry_msgs::Twist msg2;
    int pos_x,pos_y;

    cv_bridge::CvImagePtr cv_ptr, cv_imageout;
    cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);//imagen que me acaba de llegar

    cv::Mat hsv;
    cv:cvtColor(cv_ptr->image , hsv, CV_RGB2HSV);

    int height = hsv.rows;
    int width = hsv.cols;
    int step = hsv.step;
    int channels = 3;  // RGB

    int x = 0;
    int y = 0;
    int counter = 0;
    for (int i=0; i < height; i++ ){
        for (int j=0; j < width; j++ )
        {
        int posdata = i * step + j * channels;
        
        if((hsv.data[posdata] >= 102) && (hsv.data[posdata] <= 125) && (hsv.data[posdata+1]  >=0) && (hsv.data[posdata+1] <= 255)&& (hsv.data[posdata+2]  >=0) && (hsv.data[posdata+2] <= 255 ))
        {
            x += j;
            y += i;
            counter++; 
        }
        
        }
    }
    if (counter > 0){
        //ROS_INFO("Ball at %d %d", x / counter , y / counter);
        pos_x = x / counter;
        pos_y = y / counter;

       angle = publish_detection(l, p);
       if( angle >= -0.1 && angle <= 0.1) 
       {
           msg2.linear.x = 0.2;
           msg2.angular.z = 0.0;
       }
       else
       {
           msg2.linear.x = 0.0;
           msg2.angular.z = 0.5;
       }
    //    msg2.angular.z = 0.5;
    //    if(pos_x >= 200 && pos_x <= 300)
    //    {
    //        if (pos_y <= 100)
    //        {
    //            msg2.linear.x = 0.0;
    //        }
    //        else 
    //        {
    //            msg2.linear.x = 0.2;
    //        }
    //        msg2.angular.z = 0.0;
    //    }
    //    
    //} else {
    //    ROS_INFO("NO BALL FOUND");

    }

    vel_pub_.publish(msg2);

}
} // ball_and_goal


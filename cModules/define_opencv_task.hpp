#ifndef H_DEFINE_OPENCV_TASK_HPP
#define H_DEFINE_OPENCV_TASK_HPP

#include "core.hpp"
#include "objdetect.hpp"
#include "imgproc.hpp"
using namespace cv;

typedef struct infor_label_st
{
    Mat img;
    int label;
    infor_label_st(Mat _img,int _lb)
    {
        img = _img.clone();
        label = _lb;
    }
}infor_label_st;

typedef struct {
    vector<Mat> database_image;
    vector<int> database_label;
}face_database_st;

#endif
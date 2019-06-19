//
// Created by LiYang on 2019-02-19.
//

#ifndef FUSION_INPUTSTREAM_H
#define FUSION_INPUTSTREAM_H

#include "utilities.h"

#ifdef REALSENSE
#include <librealsense2/rs.hpp>
#endif



class inputStream {
public:
    virtual std::vector<pangolin::TypedImage> getFrames() =0;

    inputStream():currentFrame(0) {}

    virtual int getCurrent() {return currentFrame;}

    CameraParameters cam_params;

protected:
    int currentFrame;


};

class RGBDDataset : public inputStream {
public:
    virtual std::vector<pangolin::TypedImage> getFrames();

    RGBDDataset(std::string base):

    baseDirPath(base){
        // static parameter
        cam_params.image_width = 640;
        cam_params.image_height = 480;
        cam_params.focal_y = 766.315580;//NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS2;
        cam_params.focal_x = 751.706129;//NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS2;
        cam_params.principal_y = cam_params.image_height/2;
        cam_params.principal_x = cam_params.image_width /2;

//        currentFrame = 250;

    }

    std::string baseDirPath;
    std::string filename;
    std::string filename_color;

};

#ifdef REALSENSE
class realSenseStream : public inputStream{

public:
    virtual std::vector<pangolin::TypedImage> getFrames();

    realSenseStream(std::string &filename);

    realSenseStream();

    ~realSenseStream();


private:

    rs2::pipeline pipeline;
    std::shared_ptr<rs2::device> device;
    std::shared_ptr<rs2::playback> playback;


    std::shared_ptr<rs2::decimation_filter> dec_filter;  // Decimation - reduces depth frame density
    std::shared_ptr<rs2::threshold_filter> thr_filter;   // Threshold  - removes values outside recommended range
    std::shared_ptr<rs2::spatial_filter> spat_filter;    // Spatial    - edge-preserving spatial smoothing
    std::shared_ptr<rs2::temporal_filter> temp_filter;   // Temporal   - reduces temporal noise

    std::shared_ptr<rs2::disparity_transform> depth_to_disparity;
    std::shared_ptr<rs2::disparity_transform> disparity_to_depth;

    float depth_scale;


};

#endif

#endif //FUSION_INPUTSTREAM_H

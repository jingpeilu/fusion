//
// Created by LiYang on 2019-02-19.
//

#include "inputStream.h"


std::vector<pangolin::TypedImage> RGBDDataset::getFrames() {
    std::vector<pangolin::TypedImage> results;

    char nf[10];

    //sprintf(nf, "%05d", currentFrame++);
    //filename = baseDirPath + nf + "-depth.png";
    //filename_color = baseDirPath + nf + "-color.png";
    
    sprintf(nf, "%06d", currentFrame++);
    filename = baseDirPath + nf + "-depth.png";
    filename_color = baseDirPath + nf + "-color.png";
    //filename = baseDirPath + "depth_" + nf + ".png";
    //filename_color = baseDirPath +"color_"+ nf + ".png";

    results.push_back(pangolin::LoadImage(filename));
    results.push_back(pangolin::LoadImage(filename_color));

    return std::forward<std::vector<pangolin::TypedImage>>(results);
}

#ifdef REALSENSE

realSenseStream::~realSenseStream() {
    pipeline.stop();
}


realSenseStream::realSenseStream(std::string &filename) {

    rs2::config configuration{};
    configuration.disable_all_streams();
    configuration.enable_device_from_file(filename);

    //configuration.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 15);
    //configuration.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 15);

    pipeline.start(configuration);
    device = std::make_shared<rs2::device>(pipeline.get_active_profile().get_device());
    playback = std::make_shared<rs2::playback>(device->as<rs2::playback>());

    playback->set_real_time(false);

    auto streams = pipeline.get_active_profile().get_streams();

    for (const auto &stream : streams) {
        if (stream.stream_type() == RS2_STREAM_DEPTH) {
            auto intrinsics = stream.as<rs2::video_stream_profile>().get_intrinsics();
            cam_params.focal_x = intrinsics.fx;
            cam_params.focal_y = intrinsics.fy;
            cam_params.image_height = intrinsics.height;
            cam_params.image_width = intrinsics.width;
            cam_params.principal_x = intrinsics.ppx;
            cam_params.principal_y = intrinsics.ppy;
        }
    }

    dec_filter = std::make_shared<rs2::decimation_filter>(1.0f); // Decimation - reduces depth frame density

    spat_filter = std::make_shared<rs2::spatial_filter>();  // Spatial    - edge-preserving spatial smoothing
    temp_filter = std::make_shared<rs2::temporal_filter>(); // Temporal   - reduces temporal noise
    thr_filter = std::make_shared<rs2::threshold_filter>();
    depth_to_disparity = std::make_shared<rs2::disparity_transform>(true);
    disparity_to_depth = std::make_shared<rs2::disparity_transform>(false);

    depth_scale = pipeline.get_active_profile().get_device().first<rs2::depth_sensor>().get_depth_scale();

    //for (int i = 0; i < 100; ++i)
    //{
    //	auto data = pipeline.wait_for_frames();
    //	auto depth = data.get_depth_frame();
    //	auto color = data.get_color_frame();
    //	std::cout << "Skipping " << color.get_frame_number() << std::endl;
    //}
}

realSenseStream::realSenseStream() {


// using yifei's implementation
    // Explicitly enable depth and color stream, with these constraints:
    // Same dimensions and color stream has format BGR 8bit
    rs2::config configuration{};
    configuration.disable_all_streams();

    // Use the first detected device, if any
    rs2::context ctx{};
    auto devices = ctx.query_devices();

    size_t device_no = 0;
    for (auto d = devices.begin(); d != devices.end(); ++d) {
        std::cout << "[" << device_no << "] " << (*d).get_info(RS2_CAMERA_INFO_NAME) << std::endl;
        ++device_no;
    }

    uint32_t device_choice = 0;
    std::cout << "Please choose a device = ";
    //std::cin >> device_choice;
    std::cout << "Choosing device [" << device_choice << "]" << std::endl;

    if (devices.size() == 0 || device_choice > device_no)
        throw std::runtime_error{"No RealSense device detected"};

    {
        auto device = devices[device_choice]; // The device handle defined here is invalid after pipeline.start()

        // Print info about the device
        std::cout << "Using this RealSense device:" << std::endl;
        configuration.enable_device(std::string(device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)));
        for (int info_idx = 0; info_idx < static_cast<int>(RS2_CAMERA_INFO_COUNT); ++info_idx) {
            auto info_type = static_cast<rs2_camera_info>(info_idx);
            std::cout << "  " << std::left << std::setw(20) << info_type << " : ";
            if (device.supports(info_type))
                std::cout << device.get_info(info_type) << std::endl;
            else
                std::cout << "Not supported" << std::endl;
        }
    }
    configuration.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
    configuration.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
    std::cout << configuration.can_resolve(pipeline) << std::endl;

    pipeline.start(configuration);

    // Get depth sensor intrinsics
    auto streams = pipeline.get_active_profile().get_streams();
    for (const auto &stream : streams) {
        if (stream.stream_type() == RS2_STREAM_DEPTH) {
            auto intrinsics = stream.as<rs2::video_stream_profile>().get_intrinsics();
            cam_params.focal_x = intrinsics.fx;
            cam_params.focal_y = intrinsics.fy;
            cam_params.image_height = intrinsics.height;
            cam_params.image_width = intrinsics.width;
            cam_params.principal_x = intrinsics.ppx;
            cam_params.principal_y = intrinsics.ppy;
        }
    }

    dec_filter = std::make_shared<rs2::decimation_filter>(1.0f); // Decimation - reduces depth frame density
    spat_filter = std::make_shared<rs2::spatial_filter>();       // Spatial    - edge-preserving spatial smoothing
    temp_filter = std::make_shared<rs2::temporal_filter>();      // Temporal   - reduces temporal noise
    thr_filter = std::make_shared<rs2::threshold_filter>();

    depth_to_disparity = std::make_shared<rs2::disparity_transform>(true);
    disparity_to_depth = std::make_shared<rs2::disparity_transform>(false);

    // Get depth scale which is used to convert the measurements into millimeters
    depth_scale = pipeline.get_active_profile().get_device().first<rs2::depth_sensor>().get_depth_scale();


}

int num_debug3 = 0;

std::vector<pangolin::TypedImage> realSenseStream::getFrames() {



    // copy from the example in realsense2 project
    std::vector<pangolin::TypedImage> results;

    auto data = pipeline.wait_for_frames();

    auto depth = data.get_depth_frame();
    auto colordata = data.get_color_frame();
    auto filtered = depth;

    filtered = dec_filter->process(filtered);
    filtered = thr_filter->process(filtered);
    filtered = depth_to_disparity->process(filtered);
    filtered = spat_filter->process(filtered);
    filtered = temp_filter->process(filtered);
    filtered = disparity_to_depth->process(filtered);


    // Allocate storage for user image to return
    results.push_back(pangolin::TypedImage(cam_params.image_width,
                                           cam_params.image_height, pangolin::PixelFormatFromString("GRAY16LE")));
    results.push_back(pangolin::TypedImage(cam_params.image_width,
                                           cam_params.image_height, pangolin::PixelFormatFromString("RGBA32")));

//    pangolin::StreamInfo streamD(pangolin::PixelFormatFromString("GRAY16LE"), cam_params.image_width,
//            cam_params.image_height, cam_params.image_width*2, 0);
//    const pangolin::Image<unsigned char> img = streamD.StreamImage((uint8_t*)filtered.get_data());


    uint16_t *ptr = (uint16_t *) results[0].begin();
    uint16_t *src = (uint16_t *) filtered.get_data();

    uint16_t m = depth_scale * 1000.0 * 10;// make it cm measurement
    for (size_t y = 0; y < cam_params.image_height; ++y) {
        for (size_t x = 0; x < cam_params.image_width; ++x) {
            uint16_t tmp = *(src++);
            ptr[y * cam_params.image_width + x] = tmp * m;
        }
        // std::memcpy(image.RowPtr(y), img.RowPtr(y), image.pitch);
    }

    char *ptr1 = (char *) results[1].begin();
    char *src1 = (char *) colordata.get_data();

    //First, create an ImageData object:

    for (size_t y = 0; y < cam_params.image_height * cam_params.image_width; ++y) {
        ptr1[y*4 ] = src1[3*y ];
        ptr1[y*4 +1] = src1[3*y +1];
        ptr1[y*4 +2] = src1[3*y +2];
        ptr1[y*4 +3] = 255;

        // std::memcpy(image.RowPtr(y), img.RowPtr(y), image.pitch);
    }

    if (0) {
        //DEBUG CODE
        std::string prefix = "rawData" + std::to_string(num_debug3++);
        SaveImage(results[0], pangolin::PixelFormatFromString("GRAY16LE"), prefix + ".png", false);
        // END DEBUG CODE
    }

    return std::forward<std::vector<pangolin::TypedImage>>(results);

}


#endif

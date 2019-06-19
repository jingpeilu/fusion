#include <limits>
#include <iostream>
#include <unistd.h>
#include <thread>
#include "opencv2/opencv.hpp"
#include <pangolin/pangolin.h>

#include <glm/glm.hpp>

#include "fusion.h"
#include "renderEngine.h"
#include "inputStream.h"



#include "opencv2/xfeatures2d.hpp"

bool isPause = false;
bool renderAll = false;
int totalFrame = 600;
bool isFollow = true;


moodycamel::ReaderWriterQueue<std::pair<pangolin::TypedImage,pangolin::TypedImage>> queue;



// thread reads the images
void readingImages(RGBDDataset& dataset)
{
  //  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    while (!pangolin::ShouldQuit()) {

        while (dataset.getCurrent() < totalFrame) {
            std::vector<pangolin::TypedImage> &&tr = dataset.getFrames();

            queue.enqueue(std::make_pair(std::move(tr[0]), std::move(tr[1])));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    }
}


void PauseMethod() {
    isPause = isPause ? false : true;

    std::cout << "You typed ctrl-r or pushed reset" << std::endl;
}

void RenderModeMethod() {
    renderAll = renderAll ? false : true;

    std::cout << "You typed ctrl-r or pushed reset" << std::endl;
}

void AddMoreFramesMethod() {
    totalFrame += 50;

    std::cout << "Current total frame: " << totalFrame << std::endl;
}

void CameraModeMethod() {
    isFollow = isFollow ? false : true;

    std::cout << "Current total frame: " << totalFrame << std::endl;
}

int main(/*int argc, char* argv[]*/) {

    // template for opencv
    //declare input/output

// opencv
    cv::Ptr<cv::Feature2D> f2d = cv::xfeatures2d::SURF::create();
    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;

    cv::Mat img_1, img_2, descriptors_1, descriptors_2;

    cv::BFMatcher matcher;
    std::vector< cv::DMatch > matches;


    // for better display on large screen
#if defined __APPLE__ || defined(MACOSX)
#define FACTOR_WINDOW 1
#else
#define FACTOR_WINDOW 2
#endif
    int window_width = 640*FACTOR_WINDOW;
    int window_height = 480*FACTOR_WINDOW;
    // Create OpenGL window in single line
    pangolin::CreateWindowAndBind("Main", window_width, window_height); // Macos resolution has been modified


    // 3D Mouse handler requires depth testing to be enabled
    glEnable(GL_DEPTH_TEST);

    //std::string basePath = "/Users/jingpeilu/Downloads/rgbd-scenes-v2/imgs/scene_06/";
    std::string basePath = "/Users/jingpeilu/Desktop/research/data/depth1/";
    //std::string basePath = "/Users/jingpeilu/Desktop/research/data/depth1/";

    //
    //std::string bagPath = "/Users/liyang/Documents/20190221_144309.bag";

    //realSenseStream dataset(bagPath);

    RGBDDataset dataset(basePath);

    std::thread t1(readingImages, std::ref(dataset));

    //for (int i=0; i< 30; ++i) dataset.getFrame(); // drop first 30 frames


    // make the projection same as the dataset;

    pangolin::OpenGlMatrixSpec projectionMatrix = pangolin::ProjectionMatrix(dataset.cam_params.image_width*FACTOR_WINDOW,
                                                                             dataset.cam_params.image_height*FACTOR_WINDOW,
                                                                             dataset.cam_params.focal_x,
                                                                             dataset.cam_params.focal_y,
                                                                             dataset.cam_params.principal_x*FACTOR_WINDOW,
                                                                             dataset.cam_params.principal_y*FACTOR_WINDOW,
                                                                             CAMERA_MODEL_N,
                                                                             CAMERA_MODEL_F);

    pangolin::OpenGlRenderState s_cam(
            projectionMatrix,
            pangolin::ModelViewLookAt(0, 0, 0, 0, 0, -1, pangolin::AxisY)
    );

    // Aspect ratio allows us to constrain width and height whilst fitting within specified
    // bounds. A positive aspect ratio makes a view 'shrink to fit' (introducing empty bars),
    // whilst a negative ratio makes the view 'grow to fit' (cropping the view).
    pangolin::View &d_cam = pangolin::Display("cam")
            .SetBounds(0, 1.0f, 0, 1.0f, -640 / 480.0)
            .SetHandler(new pangolin::Handler3D(s_cam));

    // This view will take up no more than a third of the windows width or height, and it
    // will have a fixed aspect ratio to match the image that it will display. When fitting
    // within the specified bounds, push to the top-left (as specified by SetLock).
    pangolin::View &d_image = pangolin::Display("image")
            .SetBounds(0.0f, 1.0f/5.0f, 0, 1 / 5.0f, 640.0 / 480)
            .SetLock(pangolin::LockLeft, pangolin::LockBottom);

    pangolin::View &d_image2 = pangolin::Display("depth")
            .SetBounds(0.0f, 1.0f /5.0f, 1 / 5.0f, 2.0f/5.0f, 640.0 / 480)
            .SetLock(pangolin::LockLeft, pangolin::LockBottom);

    std::cout << "Resize the window to experiment with SetBounds, SetLock and SetAspect." << std::endl;
    std::cout << "Notice that the cubes aspect is maintained even though it covers the whole screen." << std::endl;


    std::pair<pangolin::TypedImage,pangolin::TypedImage> tr;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.try_dequeue(tr);
//    std::vector<pangolin::TypedImage> &&tr = dataset.getFrames();

    unsigned width = tr.first.w;
    unsigned height = tr.first.h;

    Fusion fusion;

    fusion.init(tr.first, dataset.cam_params);

    img_1 = cv::Mat(tr.second.h, tr.second.w, CV_8UC3, tr.second.ptr);;

    f2d->detect( img_1, keypoints_1 );
    f2d->compute( img_1, keypoints_1, descriptors_1 );



    std::cout << width << " " << height << std::endl;

    std::cout << sizeof(cVec3) << sizeof(float) << std::endl;

    pangolin::GlTexture colorTex(width, height, GL_RGBA, true, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    colorTex.Load(tr.second);

    pangolin::GlTexture depthTex(width, height, GL_RGBA, true, 0, GL_RGBA, GL_UNSIGNED_BYTE);
    depthTex.Load(tr.first);
    float z = -1;

    //glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    // Default hooks for exiting (Esc) and fullscreen (tab).

    float fps = 0;

    // Demonstration of how we can register a keyboard hook to trigger a method
    pangolin::RegisterKeyPressCallback(' ', PauseMethod);
    // Demonstration of how we can register a keyboard hook to trigger a method
    pangolin::RegisterKeyPressCallback('r', RenderModeMethod);
    // Demonstration of how we can register a keyboard hook to trigger a method
    pangolin::RegisterKeyPressCallback('t', AddMoreFramesMethod);
    // Demonstration of how we can register a keyboard hook to trigger a method
    pangolin::RegisterKeyPressCallback('f', CameraModeMethod);


    CameraParameters p = fusion.getCameraParam();
    Eigen::Matrix3f K = Eigen::Matrix3f::Identity();
    K(0, 0) = p.focal_x;
    K(1, 1) = p.focal_y;
    K(0, 2) = p.principal_x;
    K(1, 2) = p.principal_y;

    pangolin::OpenGlMatrix viewMatrix = pangolin::ModelViewLookAt(0, 0, 0, 0, 0, 1,
                                                                  pangolin::AxisY);// default camera status

    Eigen::Matrix3f Kinv = K.inverse();

    while (!pangolin::ShouldQuit()) {


        glEnable(GL_DEPTH_TEST);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set folloing camera and draw the frustum
        glm::mat4 m = fusion.getModelViewMatrix();
        pangolin::OpenGlMatrix viewMatrix2;
        renderEngine::convert2GL(m, viewMatrix2);

        pangolin::OpenGlMatrix viewMatrix3 =  viewMatrix2;

        Eigen::Matrix4f pose = Eigen::Matrix4f(viewMatrix *viewMatrix2).inverse();

        if (isFollow) {
            s_cam.SetModelViewMatrix(viewMatrix3);
        }


        d_cam.Activate(s_cam);

        fusion.displayGlobalModel(s_cam, renderAll, window_height);

        pangolin::glDrawFrustum(Kinv,
                                p.image_width,
                                p.image_height,
                                pose,
                                3.0f);

        //display the image
        d_image.Activate();
        glColor3f(1.0, 1.0, 1.0);
        colorTex.RenderToViewportFlipY();

        //display the image
        d_image2.Activate();
        glColor3f(1.0, 1.0, 1.0);
        depthTex.RenderToViewportFlipY();

        std::string s = "Instruction: space to pause; 'r' to switch rendering mode; 'f' to switch camera mode; 't' to add frames\n"+fusion.getMessage();

        std::string delimiter = "\n";
        glColor3f(1.0, 1.0, 1.0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        size_t pos = 0;
        std::string token;
        int i = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            pangolin::GlText txt = pangolin::GlFont::I().Text(token);
            txt.DrawWindow(10, window_height * 0.98 - i++*1.2*txt.Height(), 0.5);
            s.erase(0, pos + delimiter.length());
        }

        glDisable(GL_BLEND);


        pangolin::FinishFrame();

        std::pair<pangolin::TypedImage,pangolin::TypedImage> tr ;
        if (!isPause && queue.try_dequeue(tr)) {



            img_2 = cv::Mat(tr.second.h, tr.second.w, CV_8UC3, tr.second.ptr);
            f2d->detect( img_2, keypoints_2 );
            f2d->compute( img_2, keypoints_2, descriptors_2 );

            matcher.match( descriptors_1, descriptors_2, matches );

            //ocl_surf(img2, oclMat(), keypoints2, descriptors2GPU);

            //-- Sort matches and preserve top 10% matches
            std::sort(matches.begin(), matches.end());
            std::vector<cv::DMatch > good_matches;
            double minDist = matches.front().distance,
                    maxDist = matches.back().distance;
            const int LOOP_NUM = 10;
            const int GOOD_PTS_MAX = 30;
            const float GOOD_PORTION = 0.15f;

            const int ptsPairs = std::min(GOOD_PTS_MAX, (int)(matches.size() * GOOD_PORTION));
            for( int i = 0; i < ptsPairs; i++ )
            {
                good_matches.push_back( matches[i] );
            }
            std::cout << "\nMax distance: " << maxDist << std::endl;
            std::cout << "Min distance: " << minDist << std::endl;

            std::cout << "Calculating homography using " << ptsPairs << " point pairs." << std::endl;

            // drawing the results
            cv::Mat img_matches;
//            drawMatches( cpu_img1, keypoints1, cpu_img2, keypoints2,
//                         good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
//                         vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS  );

            //-- Localize the object
            std::vector<cv::Point2f> obj;
            std::vector<cv::Point2f> scene;

            std::vector<cl_float4> matchCL;

            for( size_t i = 0; i < good_matches.size(); i++ )
            {
                //-- Get the keypoints from the good matches
                obj.push_back( keypoints_1[ good_matches[i].queryIdx ].pt );
                scene.push_back( keypoints_2[ good_matches[i].trainIdx ].pt );
                cl_float4 tmp;
                tmp.x = keypoints_1[ good_matches[i].queryIdx ].pt.x;
                tmp.y = keypoints_1[ good_matches[i].queryIdx ].pt.y;
                tmp.z = keypoints_2[ good_matches[i].trainIdx ].pt.x;
                tmp.w = keypoints_2[ good_matches[i].trainIdx ].pt.y;
                matchCL.push_back(tmp);
            }


//            double focal = 1.0;
//            cv::Point2d pp(0.0, 0.0);
//            cv::Mat E, R, R2,t,t2, mask;
//
//            E = cv::findEssentialMat(obj, scene, p.focal_x,cv::Point2d(p.principal_x,p.principal_y), cv::RANSAC,0.999,0.1);
//            cv::decomposeEssentialMat(E,R,R2,t2);
//            cv::recoverPose(E, obj, scene, R, t, p.focal_x, cv::Point2d(p.principal_x,p.principal_y));
//            std::cout<<"CV Translation: "<<t<<t2<<std::endl;
////            E = fusion.getKMatrix() * E * fusion.getKMatrix();
////            E = findFundamentalMat(obj, scene, focal, pp, RANSAC, 0.999, 1.0, mask);
//            cv::namedWindow("matches", 1);
//            drawMatches(img_1, keypoints_1, img_2, keypoints_2, good_matches, img_matches);
//            imshow("matches", img_matches);
//            cv::waitKey(0);

            // switch
            descriptors_1 = descriptors_2;
            keypoints_1 = keypoints_2;
            img_2.copyTo(img_1);



            fusion.update(tr.first,matchCL);//
            depthTex.Load(tr.first);
            colorTex.Load(tr.second);





        } else {
            isFollow = false;
        }


    }

    //  delete[] imageArray;
    fusion.destroy();

    t1.join();

    return 0;
}

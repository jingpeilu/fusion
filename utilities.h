//
// Created by LiYang on 2018-12-04.
//

#ifndef FUSION_UTILITIES_H
#define FUSION_UTILITIES_H

#include "libs.h"

#include "GPUBuffer.h"

#include "Utilities/readerwriterqueue.h"

typedef glm::vec3 cVec3;

typedef glm::quat cQuat;

typedef glm::mat3 cMat;

typedef float cFloat;

#define NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS2         (570.3f)   // Based on 640x480 pixel size.
#define NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS         (285.63f)   // Based on 320x240 pixel size.
#define NUI_CAMERA_DEPTH_NOMINAL_INVERSE_FOCAL_LENGTH_IN_PIXELS (3.501e-3f) // (1/NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS)
#define NUI_CAMERA_DEPTH_NOMINAL_DIAGONAL_FOV                   (70.0f)
#define NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV                 (58.5f)
#define NUI_CAMERA_DEPTH_NOMINAL_VERTICAL_FOV                   (45.6f)

#define PI 3.14159

#define CAMERA_MODEL_N (0.1f)  //near plane
#define CAMERA_MODEL_F (1000.0f)  //far plane


#define EPSILON 0.00000001


struct cPoints{
    std::vector<cVec3>  mPos;
    std::vector<cVec3>  mNorm;
    std::vector<cVec3>  mAttri; // mRadius,mConfident,index
    //std::vector<cFloat> mRadius;
    //std::vector<cFloat> mConfident;
    std::vector<int> mTimeStamp;
    // internal status

    void clear()
    {
        mPos.clear();
        mNorm.clear();
        mAttri.clear();
        mTimeStamp.clear();
    }

};

struct gpuPoints{
    GPUBuffer  mPos;
    GPUBuffer  mNorm;
    GPUBuffer  mAttri; // mRadius,mConfident,index
    //std::vector<cFloat> mRadius;
    //std::vector<cFloat> mConfident;
    // internal status
    int numPoints;

    void setNumPoints(int num){
        numPoints = num;
        mPos.inUsedSize = num * 4* sizeof(float);
        mNorm.inUsedSize = num * 4* sizeof(float);
        mAttri.inUsedSize = num * 4* sizeof(float);
    }

};

struct CameraParameters
{
    int image_width, image_height;
    float focal_x, focal_y;
    float principal_x, principal_y;

    /**
         * Returns camera parameters for a specified pyramid level; each level corresponds to a scaling of pow(.5, level)
         * @param level The pyramid level to get the parameters for with 0 being the non-scaled version,
         * higher levels correspond to smaller spatial size
         * @return A CameraParameters structure containing the scaled values
         */
    CameraParameters level(const size_t level) const
    {
        if (level == 0)
            return *this;

        const float scale_factor = powf(0.5f, static_cast<float>(level));
        return CameraParameters{image_width >> level, image_height >> level,
                                focal_x * scale_factor, focal_y * scale_factor,
                                (principal_x + 0.5f) * scale_factor - 0.5f,
                                (principal_y + 0.5f) * scale_factor - 0.5f};
    }
};


// Helper function to return precision delta time for 3 counters since last call based upon host high performance counter
// *********************************************************************
static double shrDeltaT(int iCounterID = 0)
{
    // local var for computation of microseconds since last call
    double DeltaT;

#ifdef _WIN32 // Windows version of precision host timer

    // Variables that need to retain state between calls
        static LARGE_INTEGER liOldCount[3] = { {0, 0}, {0, 0}, {0, 0} };

        // locals for new count, new freq and new time delta
	    LARGE_INTEGER liNewCount, liFreq;
	    if (QueryPerformanceFrequency(&liFreq))
	    {
		    // Get new counter reading
		    QueryPerformanceCounter(&liNewCount);

		    if (iCounterID >= 0 && iCounterID <= 2)
		    {
			    // Calculate time difference for timer 0.  (zero when called the first time)
			    DeltaT = liOldCount[iCounterID].LowPart ? (((double)liNewCount.QuadPart - (double)liOldCount[iCounterID].QuadPart) / (double)liFreq.QuadPart) : 0.0;
			    // Reset old count to new
			    liOldCount[iCounterID] = liNewCount;
			}
			else
			{
		        // Requested counter ID out of range
		        DeltaT = -9999.0;
			}

		    // Returns time difference in seconds sunce the last call
		    return DeltaT;
	    }
	    else
	    {
		    // No high resolution performance counter
		    return -9999.0;
	    }

#elif defined (__APPLE__) || defined (MACOSX)
    static time_t _NewTime;
        static time_t _OldTime[3];

        _NewTime  = clock();

		if (iCounterID >= 0 && iCounterID <= 2)
		{
		    // Calculate time difference for timer (iCounterID).  (zero when called the first time)
		    DeltaT = double(_NewTime-_OldTime[iCounterID])/CLOCKS_PER_SEC;

		    // Reset old time (iCounterID) to the new one
//		    _OldTime[iCounterID].tv_sec  = _NewTime.tv_sec;
//		    _OldTime[iCounterID].tv_usec = _NewTime.tv_usec;
            _OldTime[iCounterID] = _NewTime;
		}
		else
		{
	        // Requested counter ID out of range
	        DeltaT = -9999.0;
		}
        return DeltaT;
#else
//#elif defined(UNIX) // Linux version of precision host timer. See http://www.informit.com/articles/article.aspx?p=23618&seqNum=8
    static struct timeval _NewTime;  // new wall clock time (struct representation in seconds and microseconds)
    static struct timeval _OldTime[3]; // old wall clock timers 0, 1, 2 (struct representation in seconds and microseconds)

    // Get new counter reading
    gettimeofday(&_NewTime, NULL);

    if (iCounterID >= 0 && iCounterID <= 2)
    {
        // Calculate time difference for timer (iCounterID).  (zero when called the first time)
        DeltaT =  ((double)_NewTime.tv_sec + 1.0e-6 * (double)_NewTime.tv_usec) - ((double)_OldTime[iCounterID].tv_sec + 1.0e-6 * (double)_OldTime[iCounterID].tv_usec);
        // Reset old timer (iCounterID) to new timer
        _OldTime[iCounterID].tv_sec  = _NewTime.tv_sec;
        _OldTime[iCounterID].tv_usec = _NewTime.tv_usec;
    }
    else
    {
        // Requested counterID is out of rangewith respect to available counters
        DeltaT = -9999.0;
    }

    // Returns time difference in seconds sunce the last call
    return DeltaT;
#endif
}

template<typename T, int m, int n>
inline glm::mat<m, n, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, n>& em)
{
    glm::mat<m, n, float, glm::precision::highp> mat;
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            mat[j][i] = em(i, j);
        }
    }
    return mat;
}

template<typename T, int m>
inline glm::vec<m, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, 1>& em)
{
    glm::vec<m, float, glm::precision::highp> v;
    for (int i = 0; i < m; ++i)
    {
        v[i] = em(i);
    }
    return v;
}



//#include <queue>
//#include <mutex>
//#include <condition_variable>
//
//// A threadsafe-queue.
//template <class T>
//class SafeQueue
//{
//public:
//    SafeQueue(void)
//            : q()
//            , m()
//            , c()
//    {}
//
//    ~SafeQueue(void)
//    {}
//
//    // Add an element to the queue.
//    void enqueue(T&& t)
//    {
//        std::lock_guard<std::mutex> lock(m);
//        q.push(std::move(t));
//        c.notify_one();
//    }
//
//    // Get the "front"-element.
//    // If the queue is empty, wait till a element is avaiable.
//    bool dequeue(T& val, std::chrono::milliseconds timeout=std::chrono::milliseconds(10) )
//    {
//        std::unique_lock<std::mutex> lock(m);
////        while(q.empty())
////        {
////            // release lock as long as the wait and reaquire it afterwards.
////            c.wait(lock);
////        }
//
//        if(!c.wait_for(lock, timeout, [this] { return !q.empty(); }))
//            return false;
//
//        val = std::move(q.front());
//        q.pop();
//        return true;
//    }
//
//
//private:
//    std::queue<T> q;
//    mutable std::mutex m;
//    std::condition_variable c;
//};



#endif //FUSION_UTILITIES_H

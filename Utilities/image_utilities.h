//
// Created by LiYang on 2018-12-05.
//

#ifndef FUSION_IMAGE_UTILITIES_H
#define FUSION_IMAGE_UTILITIES_H

#include "../utilities.h"

typedef cFloat IMAGETYPE;

// a simle wrapper for std::vector<uchar>
class cImage{
public:
    cImage():mWidth(0),mHeight(0),mdata(NULL){};
    cImage(cImage& a){mdata = a.mdata;mWidth = a.mWidth;mHeight = a.mHeight;};
    cImage(unsigned width, unsigned height): mWidth(width),mHeight(height){ mdata = new IMAGETYPE[width*height];};

    void loadPNG(std::vector<unsigned char>& data, unsigned width, unsigned height) {
        mdata = new IMAGETYPE[width*height];
        mWidth = width;
        mHeight = height;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                mdata[y*width + x] = (IMAGETYPE)data[y*width*4 + x*4]; // png has rgba for depth which is not cool
            }
            //std::cout<<mdata[y*width+width/2]<<std::endl;
        }
    }

    ~cImage(){if (mdata != NULL) delete[] mdata;};

    IMAGETYPE & operator() (unsigned x, unsigned y){
        if (x >= mWidth || y >= mHeight)
            throw ("const Matrix subscript out of bounds");
        return mdata[y*mWidth + x]; // row first data structure
    };        // Subscript operators often come in pairs

    IMAGETYPE operator() (unsigned x, unsigned y) const{
        if (x >= mWidth || y >= mHeight)
            throw ("const Matrix subscript out of bounds");
        return mdata[y*mWidth + x];
    };  // Subscript operators often come in pairs

    IMAGETYPE* data(){return mdata;};
    unsigned getW(){return mWidth;};
    unsigned getH(){return mHeight;};
private:

    IMAGETYPE* mdata;
    unsigned mWidth;
    unsigned mHeight;


};

// helpful functions for image

void bilateralFilter(cImage& source, cImage& filteredImage, int diameter, double sigmaI, double sigmaS);



#endif //FUSION_IMAGE_UTILITIES_H

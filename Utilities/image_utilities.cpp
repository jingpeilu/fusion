//
// Created by LiYang on 2018-12-05.
//

#include <math.h>
#include <vector>
#include "../utilities.h"

#include "image_utilities.h"

float distance(int x, int y, int i, int j) {
    return float(sqrt(pow(x - i, 2) + pow(y - j, 2)));
}

double gaussian(float x, double sigma) {
    return exp(-(pow(x, 2))/(2 * pow(sigma, 2))) / (2 * PI * pow(sigma, 2));

}

double applyBilateralFilter(cImage& source,
        int x, int y, int diameter, double sigmaI, double sigmaS) {
    double iFiltered = 0;
    double wP = 0;
    int neighbor_x = 0;
    int neighbor_y = 0;
    int half = diameter / 2;

    for(int i = 0; i < diameter; i++) {
        for(int j = 0; j < diameter; j++) {
            neighbor_x = x - (half - i);
            neighbor_y = y - (half - j);
            double gi = gaussian(source(neighbor_x, neighbor_y) - source(x, y), sigmaI);
            double gs = gaussian(distance(x, y, neighbor_x, neighbor_y), sigmaS);
            double w = gi * gs;
            iFiltered = iFiltered + source(neighbor_x, neighbor_y) * w;
            wP = wP + w;
        }
    }
    iFiltered = iFiltered / wP;
    return iFiltered;


}

void bilateralFilter(cImage& source, cImage& filteredImage,int diameter, double sigmaI, double sigmaS) {
    int half = diameter / 2;
    for(int i = half; i < source.getH() - half; i++) {
        for(int j = half; j < source.getW() - half; j++) {
            filteredImage(j,i) =  applyBilateralFilter(source, j, i, diameter, sigmaI, sigmaS);
        }
        std::cout<<filteredImage(source.getH()/2,i)<<std::endl;
    }
}

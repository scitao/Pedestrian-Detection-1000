#pragma once
#include <cv.h>
#include <vector>
#include "Cell.h"

class Grid
{
public:
    Grid( cv::Mat src, cv::Size cellDims );
    ~Grid( void );

    Cell& cell( int x, int y );

    std::vector< cv::Mat > createDescriptorVectors( int blockWidth );

    cv::Mat createHogImage( int scale );

    int dimX(void) const;

    int dimY(void) const;

private:
    void populateCells( int numBins );

    std::vector< Cell > mCell;
    cv::Size mCellDims;
    cv::Size mGridDims;
    cv::Mat mSource;
};

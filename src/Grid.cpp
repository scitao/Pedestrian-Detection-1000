#include <cv.h>
#include <highgui.h>
#include <cstdio>
#include "Grid.h"
#include "util.h"
using namespace cv;
using namespace std;

Mat Grid::createHogImage( const int scale )
{
    // create HOG
    Size scaledCellDims( mCellDims.width * scale, mCellDims.height * scale );
    Mat hog( mGridDims.height * scaledCellDims.height, mGridDims.width * scaledCellDims.width, CV_32FC1 );
    for( int gridY = 0; gridY < dimY(); gridY++ )
    {
        for( int gridX = 0; gridX < dimX(); gridX++ )
        {
            // construct cell's ROI
            Rect roi( gridX * scaledCellDims.width, gridY * scaledCellDims.height, scaledCellDims.width, scaledCellDims.height );
            Mat cellOutputRegion = hog( roi );

            // output cell's hog to ROI
            Mat cellHog = cell( gridX, gridY ).drawHOG( scale );
            cellHog.copyTo( cellOutputRegion );
        }
    }

    return hog;
}

Grid::Grid( Mat src, const Size cellDims, const int numBins, const bool shouldIgnoreSign )
        : mCellDims( cellDims ), mNumBins( numBins ), mShouldIgnoreSign( shouldIgnoreSign )
{
    // convert to greyscale
    Mat bwSrc;
    cvtColor( src, bwSrc, CV_RGB2GRAY );

    // convert to floating point
    bwSrc.convertTo( bwSrc, CV_32F, 1.0 / 255 );
    mSource = bwSrc;

    // populate cells
    populateCells();

    // accumulate descriptor vectors
    const int blockWidth = 4;
    vector< Mat > descriptorVectors = createDescriptorVectors( blockWidth );

    // normalize descriptorVectors
    normalizeDescriptorVectors( descriptorVectors );

    // save descriptor vectors
    mDescriptorVector = descriptorVectors;
}

Grid::~Grid( void )
{
}

Cell& Grid::cell( int x, int y )
{
    return const_cast< Cell& >( static_cast< const Grid* >( this )->cell( x, y ) );
}

const Cell& Grid::cell( int x, int y ) const
{
    assert( x >= 0 && x < dimX() );
    assert( y >= 0 && y < dimY() );

    return mCell[ x + y * dimX() ];
}

vector< Mat > Grid::createDescriptorVectors( const int blockWidth ) const
{
    // compute range of blocks (from top-left corner)
    const int blockRadius = blockWidth / 2;
    const Range gridRangeX( blockRadius, dimX() - blockRadius );
    const Range gridRangeY( blockRadius, dimY() - blockRadius );

    // allocate memory for descriptor vectors
    const int numCellsPerBlock = blockWidth * blockWidth;
    vector< Mat > descriptorVectors( gridRangeX.size() * gridRangeY.size() );

    Mat descriptorVector( 1, numCellsPerBlock * mNumBins, CV_32FC1 );

    // visit each block and populate its descriptor vector
    for( int gridY = gridRangeY.start; gridY < gridRangeY.end; gridY++ )
    {
        for( int gridX = gridRangeX.start; gridX < gridRangeX.end; gridX++ )
        {
            // compute the range of cells in the block
            const Range cellRangeX( gridX - blockRadius, gridX - blockRadius + blockWidth );
            const Range cellRangeY( gridY - blockRadius, gridY - blockRadius + blockWidth );

            // create the descriptor vector for this block
            // visit each cell in the block and copy the histogram values into
            // the descriptor vector
            for( int descriptorIndex = 0, cellX = cellRangeX.start; cellX < cellRangeX.end; cellX++ )
            {
                for( int cellY = cellRangeY.start; cellY < cellRangeY.end; cellY++ )
                {
                    for( int i = 0; i < mNumBins; i++, descriptorIndex++ )
                    {
                        const float descriptorValue = cell( cellX, cellY ).bin( i );
                        descriptorVector.at< float >( 0, descriptorIndex ) = descriptorValue;
                    }
                }
            }
            const int descriptorVectorIndex = ( gridX - gridRangeX.start ) + ( gridY - gridRangeY.start ) * gridRangeX.size();
            descriptorVectors[ descriptorVectorIndex ] = descriptorVector.clone();
        }
    }

    return descriptorVectors;
}

int Grid::dimX( void ) const
{
    return mGridDims.width;
}

int Grid::dimY( void ) const
{
    return mGridDims.height;
}

const std::vector< cv::Mat >& Grid::getDescriptorVectors( void ) const
{
    return mDescriptorVector;
}

void Grid::normalizeDescriptorVectors( vector< Mat >& descriptorVectors )
{
    // normalize locally to each descriptor vector
    for( size_t descriptorVectorIndex = 0; descriptorVectorIndex < descriptorVectors.size(); descriptorVectorIndex++ )
    {
        Mat& descriptors = descriptorVectors[ descriptorVectorIndex ];

        // compute L2 norm for descriptor vector
        const double epsilon = 0.1;
        double sumOfSquares = 0;
        for( int descriptorIndex = 0; descriptorIndex < descriptors.rows; descriptorIndex++ )
        {
            const float descriptorValue = descriptors.at< float >( 0, descriptorIndex );
            sumOfSquares += descriptorValue * descriptorValue;
        }
        const float l2Norm = std::sqrt( sumOfSquares + epsilon );
//        printf( "L2Norm: %f\n", l2Norm );

        // normalize
        for( int descriptorIndex = 0; descriptorIndex < descriptors.rows; descriptorIndex++ )
        {
            float& descriptorValue = descriptors.at< float >( 0, descriptorIndex );
//            printf( "descriptor[%d]: before: %f", descriptorIndex, descriptorValue );
            descriptorValue /= l2Norm;
//            printf( "after: %f\n", descriptorValue );
        }
    }
}

void Grid::populateCells( void )
{
    // allocate cells
    mGridDims.width = mSource.cols / mCellDims.width;
    mGridDims.height = mSource.rows / mCellDims.height;
    mCell.resize( mGridDims.width * mGridDims.height, Cell( mNumBins, mShouldIgnoreSign ) );

    // populate cells
//    printf( "(gridWidth, gridHeight) = (%d,%d)\n", dimX(), dimY() );
    for( int gridY = 0; gridY < dimY(); gridY++ )
    {
        for( int gridX = 0; gridX < dimX(); gridX++ )
        {
            // construct cell's ROI
            Rect roi( gridX * mCellDims.width, gridY * mCellDims.height, mCellDims.width, mCellDims.height );
            Mat cellSrc = mSource( roi );
            cell( gridX, gridY ).addImage( cellSrc );
        }
    }
}

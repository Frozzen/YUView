#include "frameobject.h"

#include "yuvfile.h"
#include <QPainter>

QCache<CacheIdx, QByteArray> FrameObject::frameCache;

FrameObject::FrameObject(const QString& srcFileName, QObject* parent) : DisplayObject(parent)
{
    QFileInfo fi(srcFileName);
    QString ext = fi.suffix();
    ext = ext.toLower();
    if( ext == "yuv" )
    {
        p_srcFile = new YUVFile(srcFileName);
    }
    else
    {
        exit(1);
    }

    // preset internal values
    p_pixelFormat = YUVC_UnknownPixelFormat;
    p_interpolationMode = 0;
    p_colorConversionMode = 0;

    // try to extract format information
    p_srcFile->extractFormat(&p_width, &p_height, &p_pixelFormat, &p_numFrames, &p_frameRate);

    // check returned values
    if(p_width < 0)
        p_width = 640;
    if(p_height < 0)
        p_height = 480;
    if(p_pixelFormat == YUVC_UnknownPixelFormat)
        p_pixelFormat = YUVC_420YpCbCr8PlanarPixelFormat;
    if(p_numFrames < 0)
        p_numFrames = 1;
    if(p_frameRate < 0)
        p_frameRate = 30.0;

    // set our name (remove file extension)
    int lastPoint = p_srcFile->fileName().lastIndexOf(".");
    p_name = p_srcFile->fileName().left(lastPoint);
}

FrameObject::~FrameObject()
{
    delete p_srcFile;
}

void FrameObject::loadImage(unsigned int frameIdx)
{
    if( p_srcFile == NULL )
        return;

    // check if we have this frame index in our cache already
    CacheIdx cIdx(p_srcFile->fileName(), frameIdx);
    QByteArray* cachedFrame = frameCache.object(cIdx);
    if( cachedFrame == NULL )    // load the corresponding frame from yuv file into the frame buffer
    {
        // add new QByteArray to cache and use its data buffer
        cachedFrame = new QByteArray();

        p_srcFile->getOneFrame(cachedFrame, frameIdx, p_width, p_height, p_pixelFormat);

        // add this frame into our cache, use MBytes as cost
        int sizeInMB = cachedFrame->size() >> 20;
        frameCache.insert(cIdx, cachedFrame, sizeInMB);
    }

    p_lastIdx = frameIdx;

    // TODO: do we need to check this here?
    if( cachedFrame->size() == 0 )
        return;

    // update our QImage with frame buffer
    p_displayImage = QImage((uchar*)cachedFrame->data(), p_width, p_height, QImage::Format_RGB888);
}

// this slot is called when some parameters of the frame change
void FrameObject::refreshDisplayImage()
{
    clearCache();
    loadImage(p_lastIdx);
}

int FrameObject::getPixelValue(int x, int y) {
    if ( (p_srcFile == NULL) || (x < 0) || (y < 0) || (x >= p_width) || (y >= p_height) )
        return 0;

    // check if we have this frame index in our cache already
    CacheIdx cIdx(p_srcFile->fileName(), p_lastIdx);
    QByteArray* cachedFrame = frameCache.object(cIdx);
    if( cachedFrame == NULL )    // load the corresponding frame from yuv file into the frame buffer
    {
        // add new QByteArray to cache and use its data buffer
        cachedFrame = new QByteArray();

        p_srcFile->getOneFrame(cachedFrame, p_lastIdx, p_width, p_height, p_pixelFormat);

        // add this frame into our cache, use MBytes as cost
        int sizeInMB = cachedFrame->size() >> 20;
        frameCache.insert(cIdx, cachedFrame, sizeInMB);
    }

    char* srcYUV = cachedFrame->data();
    int ret=0;
    unsigned char *components = reinterpret_cast<unsigned char*>(&ret);
    components[3] = srcYUV[3*(y*p_width + x)+0];
    components[2] = srcYUV[3*(y*p_width + x)+1];
    components[1] = srcYUV[3*(y*p_width + x)+2];
    return ret;
}


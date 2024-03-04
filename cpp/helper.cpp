#include "helper.h"

// Qt lib import
#include <QtConcurrent>

Helper::Helper()
{
    threadPool_.setMaxThreadCount( 1 );
    QtConcurrent::run( &threadPool_, std::bind( &Helper::displayLoop, this ) );
}

Helper::~Helper()
{
    this->stop();
}

void Helper::setImageItem(JQImageItem *imageItem)
{
    imageItem_ = imageItem;
}

void Helper::setImageItem2(JQImageItem2 *imageItem2)
{
    imageItem2_ = imageItem2;
}

void Helper::stop()
{
    continueRun_ = false;
    threadPool_.waitForDone();
}

void Helper::displayLoop()
{
    const auto        rawImage = QImage( ":/images/3.png" );
    QVector< QImage > imageList;

    // 根据原始输入图片进行旋转，生成测试数据
    const int frames = 300; // 总帧数

    for ( int i = 0; i < frames; ++i )
    {
        // 使用正弦函数生成缩放效果
        const float phase = ( static_cast< float >( i ) / frames ) * 2 * M_PI; // 计算当前帧的相位
        const float scale = 1 + 0.25f * ( qSin( phase ) + 1 );                 // 计算当前缩放比例，范围从1到1.5

        QTransform transform;
        transform.scale( scale, scale ); // 应用缩放变换
        QImage scaledImage = rawImage.transformed( transform, Qt::SmoothTransformation );

        // 计算裁剪区域
        QRect cropRect(
            0,
            0,
            rawImage.width(),
            rawImage.height() );

        // 裁剪放大的图片以保持原始尺寸
        const auto croppedImage = scaledImage.copy( cropRect );
        imageList.push_back( croppedImage );
    }

    // 图片设置接口是多线程安全的，所以可以在任意线程中调用
    for ( int imageIndex = 0; continueRun_; ++imageIndex )
    {
        QThread::msleep( 16 );

        if ( imageItem_ )
        {
            imageItem_->setImage( imageList[ imageIndex % imageList.size() ] );
        }

        if ( imageItem2_ )
        {
            imageItem2_->setImage( imageList[ imageIndex % imageList.size() ] );
        }
    }

    QThread::msleep( 200 );
}

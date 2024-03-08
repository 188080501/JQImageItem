#include "helper.h"

// Qt lib import
#include <QDebug>
#include <QtMath>
#ifndef Q_OS_WASM
#   include <QtConcurrent>
#endif

Helper::Helper()
{
    const auto rawImage = QImage( ":/images/1.png" );

    // 根据原始输入图片进行缩放，生成测试数据
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
        imageList_.push_back( croppedImage );
    }

#ifndef Q_OS_WASM
    auto future = QtConcurrent::run(
        [ = ]()
        {
            while ( isContinue_ )
            {
                QThread::msleep( 16 );
                this->setNextImage();
            }
        } );
    Q_UNUSED( future );
#else
    connect( &timer_, &QTimer::timeout, this, &Helper::setNextImage );
    timer_.start( 16 );
#endif
}

void Helper::stop()
{
#ifndef Q_OS_WASM
    isContinue_ = false;
    QThreadPool::globalInstance()->waitForDone();
#else
    timer_.stop();
#endif
}

void Helper::setImageItem(JQImageItem *imageItem)
{
    if ( imageItem )
    {
        imageItem->setImage( { } );
    }
    imageItem_ = imageItem;
}

void Helper::setImageItem2(JQImageItem2 *imageItem2)
{
    if ( imageItem2 )
    {
        imageItem2->setImage( { } );
    }
    imageItem2_ = imageItem2;
}

void Helper::setNextImage()
{
    ++imageIndex_;

    if ( imageItem_ )
    {
        imageItem_->setImage( imageList_[ imageIndex_ % imageList_.size() ] );
    }

    if ( imageItem2_ )
    {
        imageItem2_->setImage( imageList_[ imageIndex_ % imageList_.size() ] );
    }
}

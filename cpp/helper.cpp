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
    continueRun_ = false;
    threadPool_.waitForDone();
}

void Helper::setImageItem(JQImageItem *imageItem)
{
    imageItem_ = imageItem;
}

void Helper::setImageItem2(JQImageItem2 *imageItem2)
{
    imageItem2_ = imageItem2;
}

void Helper::displayLoop()
{
    const auto image1 = QImage( ":/images/3.png" );

    while ( continueRun_ )
    {
        QThread::msleep( 16 );

        if ( imageItem_ )
        {
            imageItem_->setImage( image1 );
        }

        if ( imageItem2_ )
        {
            imageItem2_->setImage( image1 );
        }
    }

    QThread::msleep( 200 );
}

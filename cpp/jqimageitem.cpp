// .h include
#include "jqimageitem.h"

// Qt lib import
#include <QDebug>
#include <QMutex>
#include <QPainter>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObjectFormat>

static QMutex mutex_;

// JQImageItemRenderer
class JQImageItemRenderer: public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
    friend JQImageItem;

public:
    JQImageItemRenderer() = default;

    ~JQImageItemRenderer()
    {
        QMutexLocker locker( &mutex_ );

        if ( texture > 0 )
        {
            glDeleteTextures( 1, &texture );
            texture = 0;
        }
    }

public:
    void render() override
    {
        if ( !buffer_.isNull() )
        {
            QMutexLocker locker( &mutex_ );

            if ( buffer_.format() != QImage::Format_RGB888 )
            {
                qDebug() << "JQImageItemRenderer::render: unsupported image format:" << buffer_.format();
                return;
            }

            if ( ( texture > 0 ) && ( textureSize == buffer_.size() ) )
            {
                // 有纹理并且大小匹配时直接更新纹理，避免创建的开销
                glBindTexture( GL_TEXTURE_2D, texture );
                glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, buffer_.width(), buffer_.height(), GL_RGB, GL_UNSIGNED_BYTE, buffer_.constBits() );
            }
            else
            {
                if ( texture > 0 )
                {
                    glDeleteTextures( 1, &texture );
                    texture = 0;
                }

                // 创建纹理
                glGenTextures( 1, &texture );
                glBindTexture( GL_TEXTURE_2D, texture );

                // 设置纹理参数
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

                // 上传纹理数据
                glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, buffer_.width(), buffer_.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, buffer_.constBits() );
                textureSize = buffer_.size();
            }

            buffer_ = { };
        }

        if ( texture == 0 ) { return; }

        glBindTexture( GL_TEXTURE_2D, texture );

        // 将纹理附加到FBO上
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0 );

        // 完成后解绑FBO和纹理
        glBindTexture( GL_TEXTURE_2D, 0 );
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );
        format.setSamples( 1 );
        format.setMipmap( false );
        return new QOpenGLFramebufferObject( size, format );
    }

private:
    GLuint texture = 0;
    QSize  textureSize;
    QImage buffer_;
};

// JQImageItem
JQImageItem::JQImageItem()
{
    this->setMirrorVertically( false );

    renderer_ = new JQImageItemRenderer;
}

JQImageItem::~JQImageItem()
{
    QMutexLocker locker( &mutex_ );
}

void JQImageItem::setImage(const QImage &image)
{
    QMutexLocker locker( &mutex_ );

    if ( !image.isNull() && ( image.size() != this->size().toSize() ) )
    {
        renderer_->buffer_ = image.scaled( this->size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ).convertToFormat( QImage::Format_RGB888 );
    }
    else
    {
        renderer_->buffer_ = image;
    }

    QMetaObject::invokeMethod( this, "update", Qt::QueuedConnection );
}

QQuickFramebufferObject::Renderer *JQImageItem::createRenderer() const
{
    renderer_->initializeOpenGLFunctions();

    return renderer_;
}

// JQImageItem2
JQImageItem2::JQImageItem2()
{
    this->setRenderTarget( QQuickPaintedItem::FramebufferObject );
}

void JQImageItem2::setImage(const QImage &image)
{
    QMutexLocker locker( &mutex_ );

    if ( !image.isNull() && ( image.size() != this->size().toSize() ) )
    {
        buffer_ = image.scaled( this->size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    }
    else
    {
        buffer_ = image;
    }

    QMetaObject::invokeMethod( this, std::bind( &JQImageItem2::update, this, QRect() ) );
}

void JQImageItem2::paint(QPainter *painter)
{
    QMutexLocker locker( &mutex_ );

    if ( buffer_.isNull() )
    {
        painter->setBrush( QColor( 0, 0, 0 ) );
        painter->drawRect( QRect( 0, 0, this->width(), this->height() ) );
    }
    else
    {
        painter->drawImage( 0, 0, buffer_ );
    }
}

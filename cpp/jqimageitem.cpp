// .h include
#include "jqimageitem.h"

// Qt lib import
#include <QDebug>
#include <QMutex>
#include <QPainter>
#include <QThread>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

#if ( defined Q_CC_MSVC ) || ( defined Q_CC_MINGW ) || ( ( defined Q_OS_MAC ) && !( defined Q_OS_IOS ) ) || ( ( defined Q_OS_LINUX ) && !( defined Q_OS_ANDROID ) )
#   define IS_DESKTOP
#endif

static const char vertexShader[] = R"(
    attribute vec4 rawVertex;
    attribute vec2 rawTexture;
    varying   vec2 currentTexture;
    void main()
    {
        currentTexture = rawTexture;
        gl_Position    = rawVertex;
    }
)";

static const char fragmentShader[] = R"(
    varying vec2      currentTexture;
    uniform sampler2D colorTexture;
    uniform bool      enabledPremultiply;
    uniform bool      enabledSwapRedBlue;
    void main()
    {
        vec4 textureColor = texture2D( colorTexture, currentTexture );
        if ( textureColor.w < 0.001 ) { discard; }
        if ( enabledPremultiply )
        {
            textureColor.x *= textureColor.w;
            textureColor.y *= textureColor.w;
            textureColor.z *= textureColor.w;
        }
        if ( enabledSwapRedBlue )
        {
            float buf      = textureColor.x;
            textureColor.x = textureColor.z;
            textureColor.z = buf;
        }
        gl_FragColor = textureColor;
    }
)";

// JQImageItemRenderer
class JQImageItemRenderer: public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
    friend JQImageItem;

public:
    JQImageItemRenderer() = default;

    ~JQImageItemRenderer() = default;

private:
    bool init()
    {
        this->initializeOpenGLFunctions();

        QSharedPointer< QOpenGLShaderProgram > program( new QOpenGLShaderProgram );

        auto vertexData = getGlslData( vertexShader );
        if ( vertexData.isEmpty() )
        {
            qDebug() << "JQImageItemRenderer: read glsl file error";
            return false;
        }

        auto fragmentData = getGlslData( fragmentShader );
        if ( fragmentData.isEmpty() )
        {
            qDebug() << "JQImageItemRenderer: read glsl file error";
            return false;
        }

        if ( !program->addShaderFromSourceCode( QOpenGLShader::Vertex, vertexData ) )
        {
            qDebug() << "JQImageItemRenderer: add vertex shader error";
            return false;
        }

        if ( !program->addShaderFromSourceCode( QOpenGLShader::Fragment, fragmentData ) )
        {
            qDebug() << "JQImageItemRenderer: add fragment shader error";
            return false;
        }

        program->bindAttributeLocation( "rawVertex", 0 );
        program->bindAttributeLocation( "rawTexturePos", 1 );

        if ( !program->link() )
        {
            qDebug() << "JQImageItemRenderer: add fragment shader error";
            return false;
        }

        enabledPremultiplyLocation_ = program->uniformLocation( "enabledPremultiply" );
        enabledSwapRedBlueLocation_ = program->uniformLocation( "enabledSwapRedBlue" );

        program_ = program;
        program_->setUniformValue( program_->uniformLocation( "colorTexture" ), 0 );

        imageVAO_ = generateVAOData();

        return true;
    }

    QByteArray getGlslData(const char *rawData)
    {
        QByteArray result;
#ifndef IS_DESKTOP
        result.append( QByteArrayLiteral( "precision highp float;\n" ) );
#endif
        result.append( rawData );
        return result;
    }

    void render() override
    {
        if ( !buffer_.isNull() )
        {
            mutex_.lock();
            auto buffer = buffer_;
            buffer_ = { };
            mutex_.unlock();

            if ( imageTexture_ && ( QSize( imageTexture_->width(), imageTexture_->height() ) == buffer.size() ) )
            {
                // 根据图片类型选择合适的setData
                if ( buffer.format() == QImage::Format_Grayscale8 )
                {
                    includesTransparentData_   = false;
                    includesPremultipliedData_ = false;
                    includesBGRData_           = false;
                    imageTexture_->setData( 0, QOpenGLTexture::Luminance, QOpenGLTexture::UInt8, buffer.constBits() );
                }
                else if ( buffer.format() == QImage::Format_RGB888 )
                {
                    includesTransparentData_   = false;
                    includesPremultipliedData_ = false;
                    includesBGRData_           = false;
                    imageTexture_->setData( 0, QOpenGLTexture::RGB, QOpenGLTexture::UInt8, buffer.constBits() );
                }
                else if ( buffer.format() == QImage::Format_RGBA8888 )
                {
                    includesTransparentData_   = true;
                    includesPremultipliedData_ = false;
                    includesBGRData_           = false;
                    imageTexture_->setData( 0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, buffer.constBits() );
                }
                else if ( buffer.format() == QImage::Format_RGB32 )
                {
                    includesTransparentData_   = false;
                    includesPremultipliedData_ = false;
                    includesBGRData_           = true;
                    imageTexture_->setData( 0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, buffer.constBits() );
                }
                else if ( buffer.format() == QImage::Format_ARGB32 )
                {
                    includesTransparentData_   = true;
                    includesPremultipliedData_ = false;
                    includesBGRData_           = true;
                    imageTexture_->setData( 0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, buffer.constBits() );
                }
                else if ( buffer.format() == QImage::Format_ARGB32_Premultiplied )
                {
                    includesTransparentData_   = true;
                    includesPremultipliedData_ = true;
                    includesBGRData_           = true;
                    imageTexture_->setData( 0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, buffer.constBits() );
                }
                else
                {
                    qDebug() << "JQImageItemRenderer::render: unsupported image format:" << buffer;
                    return;
                }
            }
            else
            {
                includesTransparentData_   = buffer.hasAlphaChannel();
                includesPremultipliedData_ = false;
                includesBGRData_           = false;

                imageTexture_.reset( new QOpenGLTexture( buffer ) );
                imageTexture_->setWrapMode( QOpenGLTexture::ClampToEdge );
                imageTexture_->setAutoMipMapGenerationEnabled( false );
                imageTexture_->setMinificationFilter( QOpenGLTexture::Linear );
                imageTexture_->setMagnificationFilter( QOpenGLTexture::Linear );
            }
        }

        if ( !program_ || !imageTexture_ ) { return; }

        program_->bind();
        imageVAO_->bind();
        imageTexture_->bind();

        GLfloat clearColor[ 4 ];

        // 带透明数据时先清空老的数据，并且开启混合
        if ( includesTransparentData_ )
        {
            this->glGetFloatv( GL_COLOR_CLEAR_VALUE, clearColor );
            this->glClearColor( 0.0, 0.0, 0.0, 0.0 );
            this->glClear( GL_COLOR_BUFFER_BIT );
            this->glEnable( GL_BLEND );
            this->glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

            program_->setUniformValue( enabledPremultiplyLocation_, !includesPremultipliedData_ );
        }
        else
        {
            program_->setUniformValue( enabledPremultiplyLocation_, false );
        }

        program_->setUniformValue( enabledSwapRedBlueLocation_, includesBGRData_ );

        this->glDrawArrays( GL_TRIANGLES, 0, 6 );

        if ( includesTransparentData_ )
        {
            this->glDisable( GL_BLEND );
            this->glClearColor( clearColor[ 0 ], clearColor[ 1 ], clearColor[ 2 ], clearColor[ 3 ] );
            this->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }

        imageTexture_->release();
        imageVAO_->release();
        program_->release();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        return new QOpenGLFramebufferObject( size, QOpenGLFramebufferObject::NoAttachment );
    }

    QSharedPointer< QOpenGLVertexArrayObject > generateVAOData()
    {
        struct VertexTextureVBO
        {
            float vertexX;
            float vertexY;
            float vertexZ;

            float textureX;
            float textureY;
        };

        QByteArray vboBuffer;
        vboBuffer.resize( 6 * static_cast< int >( sizeof( VertexTextureVBO ) ) );

        auto current = reinterpret_cast< VertexTextureVBO * >( vboBuffer.data() );

        // 绘制一个图片需要2个三角面片，这里生成对应的顶点和纹理坐标
        // 因为OpenGL的坐标系是左下角为原点, 所以这里纹理是颠倒映射的
        current->vertexX = -1.0f; current->vertexY = -1.0f; current->vertexZ = 0; current->textureX = 0; current->textureY = 0; ++current;
        current->vertexX = -1.0f; current->vertexY =  1.0f; current->vertexZ = 0; current->textureX = 0; current->textureY = 1; ++current;
        current->vertexX =  1.0f; current->vertexY =  1.0f; current->vertexZ = 0; current->textureX = 1; current->textureY = 1; ++current;
        current->vertexX =  1.0f; current->vertexY =  1.0f; current->vertexZ = 0; current->textureX = 1; current->textureY = 1; ++current;
        current->vertexX =  1.0f; current->vertexY = -1.0f; current->vertexZ = 0; current->textureX = 1; current->textureY = 0; ++current;
        current->vertexX = -1.0f; current->vertexY = -1.0f; current->vertexZ = 0; current->textureX = 0; current->textureY = 0; ++current;

        // 创建VBO和VAO
        auto vbo = new QOpenGLBuffer( QOpenGLBuffer::Type::VertexBuffer );

        QSharedPointer< QOpenGLVertexArrayObject > vao;
        vao.reset(
            new QOpenGLVertexArrayObject,
            [ vbo ](QOpenGLVertexArrayObject *vao)
            { if ( !qApp ) { return; } vao->destroy(); delete vao; vbo->destroy(); delete vbo; } );

        vao->create();
        vao->bind();

        vbo->create();
        vbo->bind();

        vbo->allocate( vboBuffer.size() );
        vbo->write( 0, vboBuffer.constData(), vboBuffer.size() );

        this->glEnableVertexAttribArray( 0 );
        this->glEnableVertexAttribArray( 1 );

        this->glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( VertexTextureVBO ), nullptr );
        this->glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( VertexTextureVBO ), reinterpret_cast< void * >( 12 ) );

        vbo->release();
        vao->release();

        return vao;
    }

private:
    QMutex mutex_;
    QImage buffer_;

    bool includesTransparentData_    = false;
    bool includesPremultipliedData_  = false;
    bool includesBGRData_            = false;

    int enabledPremultiplyLocation_ = -1;
    int enabledSwapRedBlueLocation_ = -1;

    QSharedPointer< QOpenGLShaderProgram >     program_;
    QSharedPointer< QOpenGLTexture >           imageTexture_;
    QSharedPointer< QOpenGLVertexArrayObject > imageVAO_;
};

// JQImageItem
JQImageItem::~JQImageItem()
{
    if ( renderer_ )
    {
        renderer_->mutex_.lock();
        renderer_->mutex_.unlock();
    }
}

void JQImageItem::setImage(const QImage &image)
{
    if ( !image.isNull() &&
         ( image.format() != QImage::Format_Grayscale8 ) &&
         ( image.format() != QImage::Format_RGB888 ) &&
         ( image.format() != QImage::Format_RGBA8888 ) &&
         ( image.format() != QImage::Format_RGB32 ) &&
         ( image.format() != QImage::Format_ARGB32 ) &&
         ( image.format() != QImage::Format_ARGB32_Premultiplied ) )
    {
        qDebug() << "JQImageItem::setImage: unsupported image format: " << image.format();
        return;
    }

    if ( !renderer_ ) { return; }

    if ( !image.isNull() )
    {
        if ( ( image.width() < this->width() ) && ( image.height() < this->height() ) )
        {
            // 输入图片分辨率小于控件分辨率时, 保持图片分辨率，交由OpenGL完成缩放，这样传输性能更优，并且绘制性能也更好

            QMutexLocker locker( &renderer_->mutex_ );
            renderer_->buffer_ = image;
        }
        else
        {
            // 输入图片分辨率大于控件分辨率时, 为了减少传输开销，在CPU端完成缩放，但是缩放时会占用一定的CPU资源
            auto newImage = image.scaled( this->size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

            QMutexLocker locker( &renderer_->mutex_ );
            renderer_->buffer_ = newImage;
        }
    }
    else
    {
        QMutexLocker locker( &renderer_->mutex_ );
        renderer_->buffer_ = image;
    }

    QMetaObject::invokeMethod( this, "update", Qt::QueuedConnection );
}

QQuickFramebufferObject::Renderer *JQImageItem::createRenderer() const
{
    renderer_ = new JQImageItemRenderer;
    renderer_->init();

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

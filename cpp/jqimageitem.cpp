﻿// .h include
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

static QMutex mutex_;

#if ( defined Q_CC_MSVC ) || ( ( defined Q_OS_MAC ) && !( defined Q_OS_IOS ) ) || ( ( defined Q_OS_LINUX ) && !( defined Q_OS_ANDROID ) )
#   define IS_DESKTOP
#endif

static const char vertexShader[] =
    "attribute vec4 rawVertex;"
    "attribute vec2 rawTexture;"
    "varying vec2 currentTexture;"
    "void main()"
    "{"
    "    currentTexture = rawTexture;"
    "    gl_Position    = rawVertex;"
    "}";

static const char fragmentShader[] =
    "varying vec2 currentTexture;"
    "uniform sampler2D colorTexture;"
    "void main()"
    "{"
    "    vec4 textureColor = texture2D( colorTexture, currentTexture );"
    "    if ( textureColor.w < 0.001 ) { discard; }"
    "    gl_FragColor = textureColor;"
    "}";

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

        program_.reset( new QOpenGLShaderProgram );

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

        if ( !program_->addShaderFromSourceCode( QOpenGLShader::Vertex, vertexData ) )
        {
            qDebug() << "JQImageItemRenderer: add vertex shader error";
            return false;
        }

        if ( !program_->addShaderFromSourceCode( QOpenGLShader::Fragment, fragmentData ) )
        {
            qDebug() << "JQImageItemRenderer: add fragment shader error";
            return false;
        }

        program_->bindAttributeLocation( "rawVertex", 0 );
        program_->bindAttributeLocation( "rawTexturePos", 1 );

        if ( !program_->link() )
        {
            qDebug() << "JQImageItemRenderer: add fragment shader error";
            return false;
        }

        program_->setUniformValue( program_->uniformLocation( "colorTexture" ), 0 );

        backgroundVAO_ = vertexTextureToVAO( this );

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
            QMutexLocker locker( &mutex_ );

            if ( backgroundTexture_ && ( QSize( backgroundTexture_->width(), backgroundTexture_->height() ) == buffer_.size() ) )
            {
                if ( buffer_.format() == QImage::Format_RGB32 )
                {
                    backgroundTexture_->setData( 0, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, buffer_.constBits() );
                }
                else if ( buffer_.format() == QImage::Format_ARGB32 )
                {
                    backgroundTexture_->setData( 0, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, buffer_.constBits() );
                }
                else if ( buffer_.format() == QImage::Format_RGB888 )
                {
                    backgroundTexture_->setData( 0, QOpenGLTexture::RGB, QOpenGLTexture::UInt8, buffer_.constBits() );
                }
                else
                {
                    qDebug() << "JQImageItemRenderer::render: unsupported image format:" << buffer_.format();
                    return;
                }
            }
            else
            {
                backgroundTexture_.reset( new QOpenGLTexture( buffer_ ) );
            }

            buffer_ = { };
        }

        if ( !program_ || !backgroundTexture_ ) { return; }

        this->glClear( GL_COLOR_BUFFER_BIT );

        program_->bind();
        backgroundVAO_->bind();
        backgroundTexture_->bind();

        this->glDrawArrays( GL_TRIANGLES, 0, 6 );

        backgroundTexture_->release();
        backgroundVAO_->release();
        program_->release();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        return new QOpenGLFramebufferObject( size, QOpenGLFramebufferObject::NoAttachment );
    }

    QSharedPointer< QOpenGLVertexArrayObject > createVAOFromByteArray(
        const QByteArray &            data,
        const std::function< void() > glBindCallback )
    {
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

        vbo->allocate( data.size() );
        vbo->write( 0, data.constData(), data.size() );

        glBindCallback();

        vbo->release();
        vao->release();

        return vao;
    }

    QSharedPointer< QOpenGLVertexArrayObject > vertexTextureToVAO(
        QOpenGLFunctions *gl )
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

        current->vertexX = -1.00001f; current->vertexY = -1.00001f; current->vertexZ = 0; current->textureX = 0; current->textureY = 0; ++current;
        current->vertexX = -1.00001f; current->vertexY =  1.00001f; current->vertexZ = 0; current->textureX = 0; current->textureY = 1; ++current;
        current->vertexX =  1.00001f; current->vertexY =  1.00001f; current->vertexZ = 0; current->textureX = 1; current->textureY = 1; ++current;
        current->vertexX =  1.00001f; current->vertexY =  1.00001f; current->vertexZ = 0; current->textureX = 1; current->textureY = 1; ++current;
        current->vertexX =  1.00001f; current->vertexY = -1.00001f; current->vertexZ = 0; current->textureX = 1; current->textureY = 0; ++current;
        current->vertexX = -1.00001f; current->vertexY = -1.00001f; current->vertexZ = 0; current->textureX = 0; current->textureY = 0; ++current;

        return createVAOFromByteArray(
            vboBuffer,
            [ = ]()
            {
                gl->glEnableVertexAttribArray( 0 );
                gl->glEnableVertexAttribArray( 1 );

                gl->glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( VertexTextureVBO ), nullptr );
                gl->glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( VertexTextureVBO ), reinterpret_cast< void * >( 12 ) );
            } );
    }

private:
    QImage buffer_;

    QSharedPointer< QOpenGLShaderProgram >     program_;
    QSharedPointer< QOpenGLTexture >           backgroundTexture_;
    QSharedPointer< QOpenGLVertexArrayObject > backgroundVAO_;
};

// JQImageItem
JQImageItem::JQImageItem()
{
    renderer_ = new JQImageItemRenderer;
}

JQImageItem::~JQImageItem()
{
    QMutexLocker locker( &mutex_ );
}

void JQImageItem::setImage(const QImage &image)
{
    QMutexLocker locker( &mutex_ );

    if ( !image.isNull() )
    {
        if ( ( image.width() < this->width() ) && ( image.height() < this->height() ) )
        {
            renderer_->buffer_ = image;
        }
        else
        {
            renderer_->buffer_ = image.scaled( this->size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        }
    }
    else
    {
        renderer_->buffer_ = image;
    }

    if ( renderer_->buffer_.format() == QImage::Format_ARGB32_Premultiplied )
    {
        renderer_->buffer_ = renderer_->buffer_.convertToFormat( QImage::Format_ARGB32 );
    }

    QMetaObject::invokeMethod( this, "update", Qt::QueuedConnection );
}

QQuickFramebufferObject::Renderer *JQImageItem::createRenderer() const
{
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
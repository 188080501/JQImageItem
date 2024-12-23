#ifndef JQLIBRARY_JQIMAGEITEM_H_
#define JQLIBRARY_JQIMAGEITEM_H_

// Qt lib import
#include <QObject>
#include <QMutex>
#include <QImage>
#include <QQuickPaintedItem>
#include <QQuickFramebufferObject>

#define JQIMAGEITEM_REGISTERTYPE \
    qmlRegisterType< JQImageItem >( "JQImageItem", 1, 0, "JQImageItem" ); \
    qmlRegisterType< JQImageItem2 >( "JQImageItem", 1, 0, "JQImageItem2" );

class JQImageItemRenderer;

class JQImageItem: public QQuickFramebufferObject
{
    Q_OBJECT
    Q_DISABLE_COPY( JQImageItem )

    Q_PROPERTY( bool smoothScale READ smoothScale WRITE setSmoothScale NOTIFY smoothScaleChanged )

public:
    JQImageItem() = default;

    ~JQImageItem() override;

public:
    void setImage(const QImage &image, const Qt::TransformationMode scaleMode);

    void setImage(const QImage &image);

public slots:
    void clean();

private:
    Renderer *createRenderer() const override;

private:
    mutable JQImageItemRenderer *renderer_ = nullptr;

    // Property statement code start
private: bool smoothScale_ = true;
public: inline bool smoothScale() const;
public: inline void setSmoothScale(const bool &newValue);
    Q_SIGNAL void smoothScaleChanged(const bool smoothScale);
private:
    // Property statement code end
};

class JQImageItem2: public QQuickPaintedItem
{
    Q_OBJECT
    Q_DISABLE_COPY( JQImageItem2 )

public:
    JQImageItem2();

    virtual ~JQImageItem2() override = default;

public:
    void setImage(const QImage &image);

private:
    void paint(QPainter *painter) override;

private:
    QMutex mutex_;
    QImage buffer_;
};

// Property accomplish code start
inline bool JQImageItem::smoothScale() const
{ return smoothScale_; }
inline void JQImageItem::setSmoothScale(const bool &newValue)
{ if ( newValue == smoothScale_ ) { return; } smoothScale_ = newValue; emit smoothScaleChanged( smoothScale_ ); }
// Property accomplish code end

#endif//JQLIBRARY_JQIMAGEITEM_H_

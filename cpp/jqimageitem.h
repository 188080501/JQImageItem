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

public:
    JQImageItem() = default;

    ~JQImageItem() override;

public:
    void setImage(const QImage &image);

private:
    Renderer *createRenderer() const override;

private:
    mutable JQImageItemRenderer *renderer_ = nullptr;
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

#endif//JQLIBRARY_JQIMAGEITEM_H_

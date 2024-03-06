#ifndef HELPER_H_
#define HELPER_H_

// Qt lib import
#include <QDebug>
#include <QTimer>

// JQLibrary lib import
#include <JQImageItem>

class Helper: public QObject
{
    Q_OBJECT

public:
    Helper();

    virtual ~Helper() override = default;

public slots:
    void setImageItem(JQImageItem *imageItem);

    void setImageItem2(JQImageItem2 *imageItem2);

private:
    void setNextImage();

private:
    QTimer timer_;

    QVector< QImage > imageList_;
    int               imageIndex_ = 0;

    JQImageItem * imageItem_  = nullptr;
    JQImageItem2 *imageItem2_ = nullptr;
};

#endif//HELPER_H_

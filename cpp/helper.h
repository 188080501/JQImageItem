#ifndef HELPER_H_
#define HELPER_H_

// Qt lib import
#include <QDebug>
#include <QThreadPool>

// JQLibrary lib import
#include <JQImageItem>

class Helper: public QObject
{
    Q_OBJECT

public:
    Helper();

    virtual ~Helper() override;

public slots:
    void setImageItem(JQImageItem *imageItem);

    void setImageItem2(JQImageItem2 *imageItem2);

    void stop();

private:
    void displayLoop();

private:
    QThreadPool threadPool_;
    bool        continueRun_ = true;

    JQImageItem * imageItem_ = nullptr;
    JQImageItem2 *imageItem2_ = nullptr;
};

#endif//HELPER_H_

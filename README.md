# 简介

本仓库实现了一个在QML框架中，显示QImage数据的QML控件，取名为JQImageItem

本控件针对的场合是需要显示并且频繁修改QImage的场景，例如视频显示。

提供了2个实现版本，一个是基于QQuickFramebufferObject（FBO）的JQImageItem，一个是基于QQuickPaintedItem的JQImageItem2。

在绝大多数场合下，FBO的实现性能要明显好于其他版本，因此如果你需要使用本仓库的控件，请使用这个版本。

代码基于纯Qt+OpenGL实现，开发时测试了2个Qt版本，分别是5.15.2和6.5.2，理论上其他Qt版本也能正常使用。

# 如何使用

在QML中，直接实例化控件，然后需要把控件的指针传回到C++，然后在C++端进行数据更新。实例化代码如下：

```
JQImageItem {
    id: imageItem
    anchors.fill: parent
}
```

如果要调整图片显示的大小，位置等属性，那么直接调整这个Item即可。

在C++端拿到指针后，调用setImage接口，可以把QImage数据设置到控件内，然后控件会显示这个QImage图片数据，代码如下：

```
imageItem->setImage( QImage( "C:/your/image/xxx.png" ) );
```

这个setImage接口是线程安全的，因此你可以在任何线程调用这个接口

# 为什么快？

要理解为什么本仓库的实现块，我们需要先理解3个常见的QImage显示方法

一般来说，在QML中显示一个QImage，有3个方法实现，他们分别如下：

* QQuickImageProvider

    这是目前网上最多的实现方法，大致原理是把QImage图片数据通过图片显示的逻辑，传给QML。

    这个方法理论上是最慢的，因为这个逻辑下，需要把一个图片，完整的经过Qt的图片资源处理逻辑，才能显示到QML中。

    在Qt的设计初衷上，这个类根本不是给这个场景用的，因此大多数人的实现，都需要在QML端给url赋值随机数，或者一个自增变量，来避免URL相同而引起的图片不刷新问题。

    并且，这个逻辑下，会严重破坏QML的图片缓存逻辑（如果你不设置cache为false的话）。

    因此在频繁修改图片的需求下，这个实现方式，我是完全不推荐使用的。

* QQuickPaintedItem

    这个类的实现，更加的贴近我们的使用场景，也不存在url这些对实现需求完全无用的中间参数变量等。

    这个类大致的流程是在C++端，通过QPainter绘制一个图片到QImage中，然后Qt会把这个QImage图片显示到QML上

    这个类的速度其实已经很快了，但是他还是存在一个额外的开销，就是QPainter的步骤。即使在分辨率，格式完全相同的情况下，QPainter绘制一个图片到QQuickPaintedItem内部的缓冲区，会存在至少一次的拷贝开销。

    不过这个类本身也有FBO的实现方式，可以说如果你的需求是QPainter绘制图表，或者其他内容的话，这就是最合适的实现。

* QQuickFramebufferObject（FBO）

    分析我们的需求，目前已经有了一个QImage，QImage有完整封装好的图片数据，我们希望他可以直接传给OpenGL实现，没有其他的开销。那么此时，直接把这个图片数据，上传到OpenGL的纹理对象（QOpenGLTexture），然后直接绘制，那么这就是最快的方法。

    基于FBO显示图片的话，会比较复杂，大致流程是：

    * 创建自己的shader，只保留图片渲染需要的部分

    * 创建和图片渲染相关的VBO，VAO

    * 把图片根据需求，更新到纹理中

    * 在OpenGL的绘制回调中，调用所有的相关对象，完成图片绘制

    也就是说基于FBO的实现，虽然逻辑更复杂，代码更多，但是更精准的控制了相关资源的使用、创建和释放逻辑。因此获得了一定的性能优势。并且在分辨率和格式相同的情况下，纹理不需要重新分配显存空间，可以复用。

    换句话说，就是特化了QImage显示的场景，降低了控件的通用性以换取性能。

    另外这里还有一个重要的性能优势，就是图片分辨率小于控件分辨率时，不在C++端进行缩放，而是把小图片直接上传到纹理，然后由QpenGL在显示的时候完成缩放逻辑。

    基于FBO也有图片附件的显示方法，代码量更少。但是实测下来性能开销也很大，因此我自己重写了shader，没有使用图片附件的方法。

> 注1：图片附件的代码如下，有兴趣的小伙伴可以自行搜索下文档：```glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0 );```

> 注2：我们暂时不讨论AnimatedImage和VideoOutput，因为这2个控件有其他更精准的使用场景

# 关于测试代码

测试代码分为2部分，一个是QML部分，分别实例化了JQImageItem和JQImageItem2，鼠标点击切换，开发者们可以观察切换后，CPU和GPU占用情况的变化。

C++的Helper类，会起一个线程，以大约60FPS的速度在设置图片给JQImageItem，以模拟图片变化的场合。

测试数据会有一个缩放的效果，这个涉及到的图片序列已经在初始化的时候生成完毕，并且统一了分辨率。这样在测试代码运行过程中的开销，基本就只有渲染开销。

# 心得体会

就如同之前所说，特化代码，换取了性能优势。因此这个方法需要更多的代码量，以及OpenGL相关的知识，大部分开发者不会做到这一步。

对我而言，很久以前就有这个QImage的显示需求，我都是拿QQuickPaintedItem实现，即使知道了FBO可以更快，但是无奈没有相关的知识储备，一直没有做实现，一直到现在才把这个实现做了出来。

实现的代码量其实不大，前后加起来就是300行不到。但是我觉得这个功能真的很重要，因此直接开源，方便其他开发者们取用。

# JQImageItem

用于在QML框架下，显示一个QImage到qml，主要针对的场合是需要频繁修改图片的场景，例如视频显示。

提供了2个实现版本，一个是基于QQuickFramebufferObject，一个是基于QQuickPaintedItem。在绝大多数场合下，QQuickFramebufferObject的实现性能要明显好于QQuickPaintedItem版本。

QQuickFramebufferObject更快的原因是更精准的控制了相关资源的使用、创建和释放逻辑。

设置图片的```setImage```接口是多线程安全的，方便开发者使用。

开发时测试了2个Qt版本，分别是5.15.2和6.5.2，理论上其他Qt版本也能正常使用。

#include "bongocat.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QUrl>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>

BongoCat::BongoCat(QWidget *parent)
    : QLabel(parent)
{
    setFocusPolicy(Qt::StrongFocus);    // 设置焦点策略：同时支持鼠标点击和键盘 Tab 切换两种方式获取焦点
    setAttribute(Qt::WA_TranslucentBackground);

    // 加载图片资源
    m_background = QPixmap(":/resources/background.png");
    m_cat = QPixmap(":/resources/cat.png");

    // 按键效果资源
    m_keyShift = QPixmap(":/resources/left-keys/Shift.png");
    m_keyCtrl = QPixmap(":/resources/left-keys/Control.png");
    m_keyR = QPixmap(":/resources/left-keys/KeyR.png");
    m_keyUp = QPixmap(":/resources/right-keys/UpArrow.png");
    m_keyDown = QPixmap(":/resources/right-keys/DownArrow.png");
    m_keyLeft = QPixmap(":/resources/right-keys/LeftArrow.png");
    m_keyRight = QPixmap(":/resources/right-keys/RightArrow.png");

    // 让控件大小跟背景图一致（简单做法）
    if (!m_background.isNull())
    {
        setFixedSize(m_background.size());
    }

    m_catSound = new QSoundEffect(this);
    m_catSound->setSource(QUrl("qrc:/resources/sounds_meow.wav"));
    m_catSound->setVolume(0.8f);
}

// 小猫左手按键：Shift / Ctrl / R
bool BongoCat::isLeftKey(int key) const
{
    return key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_R;
}

// 小猫右手按键：方向键或 WASD
bool BongoCat::isRightKey(int key) const
{
    return key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right
           || key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D;
}

const QPixmap* BongoCat::leftKeyPixmap(int key) const
{
    if (key == Qt::Key_Shift)
    {
        return &m_keyShift;
    }
    if (key == Qt::Key_Control)
    {
        return &m_keyCtrl;
    }
    if (key == Qt::Key_R)
    {
        return &m_keyR;
    }

    return nullptr;
}

const QPixmap* BongoCat::rightKeyPixmap(int key) const
{
    if (key == Qt::Key_Up || key == Qt::Key_W)
    {
        return &m_keyUp;
    }
    if (key == Qt::Key_Down || key == Qt::Key_S)
    {
        return &m_keyDown;
    }
    if (key == Qt::Key_Left || key == Qt::Key_A)
    {
        return &m_keyLeft;
    }
    if (key == Qt::Key_Right || key == Qt::Key_D)
    {
        return &m_keyRight;
    }

    return nullptr;
}


// 处理鼠标按下
void BongoCat::handleKeyPress(int key)
{
    if(!isLeftKey(key) && !isRightKey(key))
    {
        return; // 仅处理小猫有的按键，其他的按键直接忽略不管
    }

    // 左右手敲击状态
    if (isLeftKey(key))
    {
        m_leftKey = key;
    }
    else if (isRightKey(key))
    {
        m_rightKey = key;
    }

    // update() 是 QWidget 的方法。
    // 作用是：告诉 Qt 这个控件需要重绘，然后 Qt 会在合适的时机调用 paintEvent()。
    // 所以在按键变化时调用 update()，它就会触发 paintEvent，把新画面显示出来。
    update(); // 触发重绘
}

// 处理鼠标抬起
void BongoCat::handleKeyRelease(int key)
{
    if(!isLeftKey(key) && !isRightKey(key))
    {
        return; // 仅处理小猫有的按键，其他的按键直接忽略不管
    }

    if (isLeftKey(key) && m_leftKey == key)
    {
        m_leftKey = 0;
    }
    else if (isRightKey(key) && m_rightKey == key)
    {
        m_rightKey = 0;
    }

    // update() 是 QWidget 的方法。
    // 作用是：告诉 Qt 这个控件需要重绘，然后 Qt 会在合适的时机调用 paintEvent()。
    // 所以在按键变化时调用 update()，它就会触发 paintEvent，把新画面显示出来。
    update(); // 触发重绘
}


// 绘制小猫和按键提示图
void BongoCat::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);    // qt风格的别报警告
    // (void)event;

    QPainter painter(this);     // 创建画笔
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);    // 开启平滑缩放（防止图片缩放时锯齿明显）

    // 先画背景：作为最底层
    if (!m_background.isNull())
    {
        // drawPixmap(x, y, pixmap)
        // x,y 是绘制的位置，(0,0) 表示控件左上角
        painter.drawPixmap(0, 0, m_background);
    }

    // 再画小猫主体：覆盖在背景上
    if (!m_cat.isNull())
    {
        painter.drawPixmap(0, 0, m_cat);
    }

    // 当按住 Shift/Ctrl/R 时叠加对应图层
    if (m_leftKey != 0)
    {
        const QPixmap* left = leftKeyPixmap(m_leftKey);
        if (left && !left->isNull())
        {
            painter.drawPixmap(0, 0, *left);
        }
    }

    // 按住方向键或 WASD 时叠加对应图层
    if (m_rightKey != 0)
    {
        const QPixmap* right = rightKeyPixmap(m_rightKey);
        if (right && !right->isNull())
        {
            painter.drawPixmap(0, 0, *right);
        }
    }
}

// 双击小猫会叫哦
void BongoCat::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(m_catSound)
    {
        m_catSound->play();
    }

    QLabel::mouseDoubleClickEvent(event);
}

// 鼠标按下事件：开始拖动窗口
void BongoCat::mousePressEvent(QMouseEvent* event)
{
    // 只处理鼠标左键按下
    if (event->button() == Qt::LeftButton)
    {
        // 获取当前控件所属的顶级窗口（主窗口）
        QWidget* w = window();
        if (w)  // 确保窗口指针有效
        {
            m_dragging = true;      // 标记进入拖动状态

            // 计算鼠标点击位置相对于窗口左上角的偏移量
            // event->globalPos()      → 鼠标在整个屏幕上的全局坐标
            // w->frameGeometry().topLeft() → 窗口框架（含标题栏）在屏幕上的左上角坐标
            // 偏移量 = 鼠标点击点 - 窗口左上角 → 用于保持鼠标与窗口的相对位置不变
            m_dragOffset = event->globalPos() - w->frameGeometry().topLeft();
        }
    }

    QLabel::mousePressEvent(event); // 调用父类的默认处理
}

// 鼠标移动事件：实现窗口跟随鼠标拖动
void BongoCat::mouseMoveEvent(QMouseEvent* event)
{
    // 只有处于拖动状态 且 左键仍按下时才移动窗口
    if (m_dragging && (event->buttons() & Qt::LeftButton))
    {
        QWidget* w = window();
        if (w)
        {
            // 计算窗口新的左上角位置 = 当前鼠标全局位置 - 开始拖动时的偏移量
            // 这样可以保持鼠标点击点与窗口的相对位置不变
            w->move(event->globalPos() - m_dragOffset);
        }
    }

    QLabel::mouseMoveEvent(event);  // 调用父类默认处理
}

// 鼠标释放事件：结束拖动状态
void BongoCat::mouseReleaseEvent(QMouseEvent* event)
{
    // 只处理左键释放
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = false;     // 清除拖动标志，结束拖动
    }

    QLabel::mouseReleaseEvent(event);   // 调用父类默认处理
}

// 右键菜单：退出程序
void BongoCat::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    QAction* quitAction = menu.addAction(QStringLiteral("退出")); // 用QStringLiteral避免运行时字符串转换

    // 在鼠标右键点击的全局屏幕位置显示菜单，并返回用户选择的动作
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == quitAction)
    {
        qApp->quit();
    }
}

// 鼠标按下事件触发
void BongoCat::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        handleKeyPress(event->key());
    }

    QLabel::keyPressEvent(event);
}

// 鼠标抬起事件触发
void BongoCat::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        handleKeyRelease(event->key());
    }

    QLabel::keyReleaseEvent(event);
}

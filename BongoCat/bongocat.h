#ifndef BONGOCAT_H
#define BONGOCAT_H

#include <QLabel>
#include <QMainWindow>
#include <QWidget>
#include <QKeyEvent>
#include <QSoundEffect>

class BongoCat : public QLabel
{
public:
    BongoCat(QWidget* parent = nullptr);

    // 处理事件
    void handleKeyPress(int key);       // 处理鼠标按下
    void handleKeyRelease(int key);     // 处理鼠标抬起


    // 鼠标双击事件：猫叫
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    // 键盘事件
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

protected:
    // 绘制小猫与按键效果
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // 图片资源
    QPixmap m_background;   // 背景图
    QPixmap m_cat;          // 喵咪图

    // 按键提示图
    QPixmap m_keyShift;
    QPixmap m_keyCtrl;
    QPixmap m_keyR;
    QPixmap m_keyUp;
    QPixmap m_keyDown;
    QPixmap m_keyLeft;
    QPixmap m_keyRight;

    // 声音
    QSoundEffect* m_catSound = nullptr;    // 猫叫

    // 当前按下的左右手按键（0 表示未按下）
    int m_leftKey = 0;
    int m_rightKey = 0;

    // 拖动窗口用
    bool m_dragging = false;    // 看看当前是否在拖动窗口
    QPoint m_dragOffset;        // 鼠标按下时鼠标到窗口左上角的偏移量

    // 判断小猫的左手/右手按键
    bool isLeftKey(int key) const;
    bool isRightKey(int key) const;

    // 根据按键获取对应的提示图
    const QPixmap* leftKeyPixmap(int key) const;
    const QPixmap* rightKeyPixmap(int key) const;

};

#endif // BONGOCAT_H

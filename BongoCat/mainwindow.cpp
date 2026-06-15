#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "bongocat.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 先设无边框，再开透明背景（顺序不强制，缺一不可）
    // Tool标志（避免任务栏图标、Alt+Tab独立窗口等），WindowStaysOnTopHint能保证显示在其他程序上面
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 使用 BongoCat 作为中央控件
    m_bongoCat = new BongoCat(this);
    setCentralWidget(m_bongoCat);

    m_bongoCat->setFocus(); // 让 BongoCat 获得焦点（键盘/鼠标事件优先）
    // 如果 BongoCat 已指定有效尺寸，则固定主窗口大小与之匹配
    if (m_bongoCat->size().isValid())
    {
        setFixedSize(m_bongoCat->size());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

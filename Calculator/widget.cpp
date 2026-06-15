#include "widget.h"
#include "./ui_widget.h"

#include <QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("计算器");

    // 输入框右对齐：视觉上从右侧开始显示输入
    ui->lineEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 程序启动后，先把所有按钮连接好
    connectAllButtons();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::connectAllButtons()
{
    // 找到当前窗口下的所有 QPushButton
    const auto buttons = findChildren<QPushButton *>();

    // 每一个按钮都连接到同一个槽函数 onAnyButtonClicked
    for (QPushButton *button : buttons)
    {
        connect(button, &QPushButton::clicked, this, &Widget::onAnyButtonClicked);
    }
}

void Widget::onAnyButtonClicked()
{
    // sender() 就是“刚刚被点击的那个按钮”
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button || !ui || !ui->lineEdit)
    {
        return;
    }

    // text 就是按钮上显示的文字，trimmed() 用于清理首尾空白
    QString key = button->text().trimmed();

    // 1. 命令键
    if (key == "DEL")
    {
        // 删除“光标前面的一个字符”，不是固定尾删
        int cursorPos = ui->lineEdit->cursorPosition();
        if (cursorPos > 0)
        {
            QString text = ui->lineEdit->text();
            text.remove(cursorPos - 1, 1);
            ui->lineEdit->setText(text);
            ui->lineEdit->setCursorPosition(cursorPos - 1);
        }
        return;
    }
    if (key == "C")
    {
        // TODO: 清空全部
        ui->lineEdit->setText("");
        return;
    }
    if (key == "CE")
    {
        // TODO: 清除当前输入
        ui->lineEdit->setText("");
        return;
    }
    if (key == "=")
    {
        // 计算表达式并显示结果
        QString expr = ui->lineEdit->text().trimmed();

        // 将界面上的特殊除号统一转换为计算器可识别的 '/'
        expr.replace("∕", "/");
        expr.replace("÷", "/");
        expr.replace("×", "*");

        const QString ret = calculator.evaluate(expr);
        if (calculator.hasError())
        {
            ui->lineEdit->setText("您输入的表达式不合法，请重新输入！");
        }
        else
        {
            ui->lineEdit->setText(ret);
        }
        return;
    }


    // 2. 运算符键
    if (key == "+" || key == "-" || key == "*" || key == "/" || key == "%" ||
        key == "×" || key == "÷" || key == "∕")
    {
        // TODO: 这里可以加“防止连续输入运算符”等规则
        ui->lineEdit->insert(key);
        return;
    }


    // 3. 其他按键（数字和小数点）
    ui->lineEdit->insert(key);
}

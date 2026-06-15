#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "mathexpressionevaluator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    // 把界面上所有按钮都连到同一个槽函数
    void connectAllButtons();

private slots:
    // 任意按钮点击后都会进这个槽函数
    void onAnyButtonClicked();

private:
    Ui::Widget *ui;
    MathExpressionEvaluator calculator;
};

#endif // WIDGET_H

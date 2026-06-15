#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QProcess>
#include <QColor>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE


enum class LogLevel { Info, Debug, Warn, Error, Success };

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget* parent = nullptr);
    ~Widget();

private slots:

    void onSubmitClicked();             // 提交/开始下载按钮的按下逻辑
    void handleProcessStdout();         // 子进程有新的标准输出时触发（进度日志、成功提示等）
    void handleProcessStderr();         // 子进程有新的标准错误时触发（网络错误、提取失败、参数错误等）
    // 处理QProcess级别错误（不是Python业务错误）：解释器不存在、进程启动失败、进程崩溃、读写管道异常等
    void handleProcessError(QProcess::ProcessError error);
    // 子进程结束后的统一收尾逻辑：exitCode==0 视为成功；非 0 视为失败，并在 UI 中给出提示
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::Widget* ui;

    void setUi();                       // 设置/创建页面的结构
    void setUiBusy(bool busy);          // busy=true时禁用输入和按钮，防止重复提交。busy=false时恢复控件可用状态。
    QString py_backend_exe_path() const; // 解析打包后的后端可执行文件路径
    QString py_venv_path() const;       // 解析python解释器路径
    QString py_scr_path() const;        // 解析python脚本代码路径
    void append_logText(const QString log_text, LogLevel level = LogLevel::Debug);    // 追加日志信息
    QColor logColor(LogLevel level) const;  // 日志等级的颜色配置

    QProcess* pyProcess;
    QLineEdit* urlLineEdit;
    QPushButton* submit_button;
    QLabel* state_label;
    QTextEdit* log_textEdit;
};
#endif // WIDGET_H

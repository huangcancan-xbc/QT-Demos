#include "widget.h"
#include "./ui_widget.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QVBoxLayout>

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , pyProcess(new QProcess(this))
    , urlLineEdit(nullptr)
    , submit_button(nullptr)
    , state_label(nullptr)
    , log_textEdit(nullptr)
{
    ui->setupUi(this);

    setUi();    // 创建页面

    // 子进程通道模式：SeparateChannels = stdout/stderr分离，便于在ui中单独标识错误输出
    pyProcess->setProcessChannelMode(QProcess::SeparateChannels);


    // 按钮点击 -> 提交下载流程入口。
    connect(submit_button, &QPushButton::clicked, this, &Widget::onSubmitClicked);

    // 子进程标准输出：读取并追加到日志框
    connect(pyProcess, &QProcess::readyReadStandardOutput, this, &Widget::handleProcessStdout);

    // 子进程标准错误：读取并追加到日志框（含错误前缀）
    connect(pyProcess, &QProcess::readyReadStandardError, this, &Widget::handleProcessStderr);

    // 子进程Process级别的错误处理：自身错误（比如启动失败）回调
    connect(pyProcess, &QProcess::errorOccurred, this, &Widget::handleProcessError);

    // 子进程结束统一处理方式
    connect(pyProcess, &QProcess::finished, this, &Widget::handleProcessFinished);

    append_logText("[info] 应用已经启动！准备好开始下载您的视频了吗……");
    append_logText(QString("[info] 当前视频输出目录：%1").arg(QDir::toNativeSeparators(QDir::currentPath())));
}

Widget::~Widget()
{
    if (pyProcess->state() != QProcess::NotRunning)
    {
        // 先尝试“优雅结束”子进程（相当于请求对方自行退出）。
        pyProcess->terminate();

        // 最多等待 1.5s，避免界面关闭时长时间阻塞，如果超时仍未退出，说明子进程可能卡住，进入强制兜底
        // waitForFinished(毫秒)相当于usleep，但属于QProcess的阻塞等待，返回值是bool
        if (!pyProcess->waitForFinished(1500))
        {
            pyProcess->kill();      // 直接杀死进程，防止它留在后台继续占用资源
            pyProcess->waitForFinished(1000);
        }
    }

    delete ui;
}

void Widget::setUi()
{
    setWindowTitle("Qt + Python 视频下载器");    // 设置窗口标题

    auto* layout = new QVBoxLayout(this);       // 创建垂直布局管理器
    layout->setContentsMargins(12, 12, 12, 12); // 设置内边距，参数顺序：左、上、右、下
    layout->setSpacing(12);                     // 设置同一布局中，各个控件/子布局之间的间距

    QLabel* tip_title = new QLabel("粘贴视频链接后点击“开始下载”，程序会调用 Python 脚本进行下载。", this);   // 提示文字标题
    tip_title->setWordWrap(true);               // 设置自动换行

    urlLineEdit = new QLineEdit(this);          // 输入视频链接的输入框
    urlLineEdit->setPlaceholderText("https://www.youtube.com/watch?v=...");  // 输入框中的提示性文字（占位提示文本）
    urlLineEdit->setClearButtonEnabled(true);   // 显示清空按钮（×），可以一键清空内容

    submit_button = new QPushButton("开始下载", this);                  // 提交按钮
    state_label = new QLabel("状态：空闲/可用", this);                   // 状态信息（每次只能下载一个）
    state_label->setStyleSheet("color: green;");

    log_textEdit = new QTextEdit(this);
    log_textEdit->setReadOnly(true);            // 只读
    log_textEdit->setPlaceholderText("Python 后端日志会显示在这里。");   // 提示性文字（占位提示文本）

    // 设置布局和垂直方向的拉伸系数生效
    urlLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    submit_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(tip_title, 1);
    layout->addWidget(urlLineEdit, 1);
    layout->addWidget(submit_button, 1);
    layout->addWidget(state_label, 1);
    layout->addWidget(log_textEdit, 6);
}

void Widget::setUiBusy(bool busy)
{
    // 任务运行中禁用输入，避免“连续点击”触发重复任务。
    state_label->setEnabled(!busy);
    urlLineEdit->setEnabled(!busy);
    submit_button->setEnabled(!busy);

    if(busy)
    {
        state_label->setText("状态：运行中");
        state_label->setStyleSheet("color:#ba7234;");
    }
    else if(state_label->text() == "状态：运行中")
    {
        // 只有仍处于“运行中”时才回落到“空闲”，避免覆盖“成功/失败”。
        state_label->setText("状态：空闲/可用");
        state_label->setStyleSheet("color:#23d06b;");
    }
}

void Widget::onSubmitClicked()
{
    // 一次只允许一个下载任务，避免并发覆盖日志和状态
    // state()用于获取进程实时状态，QProcess::NotRunning是进程未运行的标识
    // QProcess::NotRunning (0)：进程未运行（初始状态或已结束/崩溃）
    // QProcess::Starting (1)：正在启动中（程序还未真正运行）
    // QProcess::Running (2)：进程正在运行，可读写
    if (pyProcess->state() != QProcess::NotRunning)
    {
        QMessageBox::information(this, "任务进行中", "已有下载任务正在运行。");
        return;
    }

    // trimmed()去除首尾空白，避免用户输入异常
    const QString videoUrl = urlLineEdit->text().trimmed();
    if (videoUrl.isEmpty())
    {
        QMessageBox::warning(this, "缺少链接", "请先输入视频链接！");
        return;
    }

    // 正则校验（仅允许 http/https，且后面至少有非空内容）
    static const QRegularExpression urlRegex(R"(^https?://\S+$)");
    if (!urlRegex.match(videoUrl).hasMatch())
    {
        QMessageBox::warning(this, "链接格式错误", "请输入合法的 http/https 链接！");
        append_logText(QString("错误：链接格式非法：%1").arg(videoUrl));
        return;
    }

    // 当前输出路径
    const QString outputPath = QDir::currentPath();
    // 优先使用发布目录中的后端可执行文件；找不到时回退到“python + 脚本”模式（便于本地开发）
    const QString backendExePath = py_backend_exe_path();
    const bool useBackendExe = !backendExePath.isEmpty();

    QString program;                    // 最终要启动的可执行程序
    QStringList Compilation_options;    // 子进程参数

    if (useBackendExe)
    {
        program = backendExePath;
        Compilation_options << "--url" << videoUrl << "--output-dir" << outputPath;
    }
    else
    {
        const QString pyVenvPath = py_venv_path();      // 解释器路径
        const QString pySrcPath = py_scr_path();        // python脚本代码路径
        if (pyVenvPath.isEmpty())
        {
            QMessageBox::critical(this, "未找到 Python 虚拟环境", "未找到 .venv 中的 Python 解释器。\n");
            append_logText("错误：未在项目目录或上级目录找到 Python 解释器（.venv）。");
            return;
        }
        if (pySrcPath.isEmpty())
        {
            QMessageBox::critical(this, "未找到后端脚本", "未找到 python/download_video.py。");
            append_logText("错误：未找到后端脚本 python/download_video.py。");
            return;
        }

        program = pyVenvPath;
        Compilation_options << "-u" << pySrcPath << "--url" << videoUrl << "--output-dir" << outputPath;
    }

    // QDir::toNativeSeparators()用于把路径中的斜杠转为当前系统原生分隔符
    // Windows 输出：C:\Videos\Downloads
    // Linux/macOS 输出：C:/Videos/Downloads
    append_logText("--------------------------------------------------------------------------------------------------------------------------------------------------------");
    append_logText(QString("[debug] 开始时间：%1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    if (useBackendExe)
    {
        append_logText(QString("[debug] 后端可执行程序：%1").arg(QDir::toNativeSeparators(backendExePath)));
    }
    else
    {
        append_logText(QString("[debug] Python 解释器：%1").arg(QDir::toNativeSeparators(program)));
        append_logText(QString("[debug] Python 后端脚本：%1").arg(QDir::toNativeSeparators(py_scr_path())));
    }
    append_logText(QString("[debug] 下载的视频链接：%1").arg(videoUrl));
    append_logText(QString("[debug] 视频输出目录：%1").arg(QDir::toNativeSeparators(outputPath)));
    append_logText("--------------------------------------------------------------------------------------------------------------------------------------------------------");

    // 标记繁忙——运行中
    setUiBusy(true);

    // 设置子进程的工作目录
    pyProcess->setWorkingDirectory(outputPath);
    // 启动外部进程，等价于命令行执行：pythonExe -u scriptPath --url xxx --output-dir xxx 作用：
    // 第一个参数：QString program：要执行的程序路径（如 "python.exe" 或 "/usr/bin/python3"）
    // 第二个参数：QStringList Compilation_options：传给程序的命令行参数列表（这里包含 -u、脚本路径、--url 等）
    // 异步启动 Python 脚本
    // 通过进程边界调用（不需在 C++ 链接 Python）
    // 完成后可通过 readyReadStandardOutput / finished 信号获取结果
    pyProcess->start(program, Compilation_options);
}

QString Widget::py_backend_exe_path() const
{
    // 发布包优先：Video_Downloader.exe 同级 runtime/vd_backend.exe
    const QStringList paths = {
        QDir(QCoreApplication::applicationDirPath()).filePath("runtime/vd_backend.exe"),
        QDir(QCoreApplication::applicationDirPath()).filePath("vd_backend.exe"),
        QDir::current().filePath("runtime/vd_backend.exe"),
        QDir::current().filePath("vd_backend.exe")
    };

    for (const QString &path : paths)
    {
        QFileInfo info(path);
        if (info.exists() && info.isFile())
        {
            return info.absoluteFilePath();
        }
    }

    return QString();
}

// 处理子进程的标准输出
void Widget::handleProcessStdout()
{
    // fromLocal8Bit：按本地编码解码字节流，减少中文日志乱码概率
    append_logText(QString::fromLocal8Bit(pyProcess->readAllStandardOutput()));
}

// 处理子进程的标准错误
void Widget::handleProcessStderr()
{
    // stderr通常是错误/警告信息，这里统一加前缀便于肉眼筛查。
    const QString stderrText = QString::fromLocal8Bit(pyProcess->readAllStandardError());
    append_logText(QString("[标准错误] %1").arg(stderrText));
}

// 处理QProcess级别的错误
void Widget::handleProcessError(QProcess::ProcessError error)
{
    // 把QProcess枚举转成可读文本，便于排查
    QString errorName;
    switch (error)
    {
        case QProcess::FailedToStart:
            errorName = "启动失败";
            break;
        case QProcess::Crashed:
            errorName = "进程崩溃";
            break;
        case QProcess::Timedout:
            errorName = "超时";
            break;
        case QProcess::ReadError:
            errorName = "读取错误";
            break;
        case QProcess::WriteError:
            errorName = "写入错误";
            break;
        case QProcess::UnknownError:
        default:
            errorName = "未知错误";
            break;
    }

    append_logText(QString("错误：QProcess 级别的错误! QProcess 错误类型：%1").arg(errorName));
    append_logText(QString("错误详情：%1").arg(pyProcess->errorString()));

    setUiBusy(false);
    state_label->setText("状态：失败！");
    state_label->setStyleSheet("color:#d62728; font-weight:600;");
}

// 统一处理进程结束后的逻辑
void Widget::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    setUiBusy(false);       // 无论成功失败，任务都结束了，先恢复可交互状态

    if (exitStatus == QProcess::CrashExit)
    {
        append_logText("错误：Python 子进程异常崩溃。");
        state_label->setText("状态：失败！");
        state_label->setStyleSheet("color:#d62728; font-weight:600;");
        QMessageBox::critical(this, "下载失败", "Python 子进程异常崩溃！");
        return;
    }

    if (exitCode == 0)
    {
        append_logText("成功：下载任务已完成！");
        state_label->setText("状态：成功！");
        state_label->setStyleSheet("color:#2ca02c; font-weight:600;");
        QMessageBox::information(this, "完成", "视频下载完成！");
        return;
    }

    append_logText(QString("错误：Python 子进程退出码为 %1").arg(exitCode));
    state_label->setText("状态：失败！");
    state_label->setStyleSheet("color:#d62728; font-weight:600;");
    QMessageBox::warning(this, "下载失败", QString("Python 脚本以退出码 %1 结束。\n请查看日志定位问题。").arg(exitCode));
}

QString Widget::py_venv_path() const
{
    const QString curPath = QString::fromUtf8("."); // 当前路径
    // 优先使用项目虚拟环境
    const QStringList paths = {
       QDir(curPath).filePath("../../.venv/Scripts/python.exe"),
       QDir(curPath).filePath("../../.venv/bin/python"),
       QDir::current().filePath("../../.venv/Scripts/python.exe"),
       QDir::current().filePath("../../.venv/bin/python")
    };

    for (const QString& path : paths)
    {
        // QFileInfo 用于跨平台判断“是否存在且是普通文件”。
        // info.exists()          文件/目录是否存在
        // info.isFile()          是普通文件
        // info.isDir()           是目录
        // info.isExecutable()    是否可执行
        // info.absoluteFilePath()  返回规范化的绝对路径
        // info.fileName()        只取文件名
        // info.size()            文件大小（字节）
        QFileInfo info(path);
        if (info.exists() && info.isFile())
        {
            return info.absoluteFilePath();
        }
    }

    return QString();
}

QString Widget::py_scr_path() const
{
    const QString curPath = QString::fromUtf8(".");
    // 先找源码目录，再找当前目录和可执行目录，兼容不同启动方式。
    const QStringList paths = {
        QDir(curPath).filePath("../../python/download_video.py"),
        QDir::current().filePath("../../python/download_video.py"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../python/download_video.py")
    };

    for (const QString& path : paths)
    {
        QFileInfo info(path);
        if (info.exists() && info.isFile())
        {
            return info.absoluteFilePath();
        }
    }

    return QString();
}

QColor Widget::logColor(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::Info:    return QColor("green");
        case LogLevel::Debug:   return QColor("#000000");
        case LogLevel::Warn:    return QColor("#be9e34");
        case LogLevel::Error:   return QColor("#ff1900");
        case LogLevel::Success: return QColor("#12994a");
    }

    return QColor("grey");
}

void Widget::append_logText(const QString log_text, LogLevel level)
{
    if (log_text.isEmpty())
    {
        return;
    }

    // 按前缀自动识别日志等级，仅给前缀 [xxx] 上色；正文使用默认颜色。
    auto levelFromPrefix = [&](const QString &prefix) -> LogLevel
    {
        if (prefix == "[info]") return LogLevel::Info;
        if (prefix == "[debug]") return LogLevel::Debug;
        if (prefix == "[warn]") return LogLevel::Warn;
        if (prefix == "[error]") return LogLevel::Error;
        if (prefix == "[success]") return LogLevel::Success;
        if (prefix == "[标准错误]") return LogLevel::Error;
        return level;
    };

    // 不管什么系统，都统一换行符为\n，replace用于替换，
    QString normalized = log_text;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');

    // 获取光标，定位到末尾开始插入
    QTextCursor cursor = log_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);

    // 只匹配行首的 [xxx] 前缀准备上色
    const QRegularExpression prefixRegex(R"(^(\[[^\]]+\])(\s*))");
    const QStringList lines = normalized.split('\n');   // 按行处理（避免大段文本一次性插入卡顿）
    for (int i = 0; i < lines.size(); ++i)
    {
        const QString &line = lines.at(i);
        // 最后一行、这行是空行、以换行为结尾
        if (i == lines.size() - 1 && line.isEmpty() && normalized.endsWith('\n'))
        {
            break;      // 就跳过（防止多余空行）
        }

        // 默认正文的颜色
        QTextCharFormat normalFmt;
        normalFmt.setForeground(QColor("grey"));

        // match：执行正则匹配
        const QRegularExpressionMatch match = prefixRegex.match(line);
        if (match.hasMatch())   // hasMatch：正则是否匹配成功（返回bool）
        {
            // 提取 [前缀] 和后面的空格
            // capturedLength：返回某个捕获组匹配到的文本长度，不传参数时返回整个匹配的长度
            const QString prefix = match.captured(1);
            const QString spaces = match.captured(2);
            // mid从指定位置开始截取子字符串mid(开始位置, 长度) 或 mid(开始位置)
            // 参数：第一个：起始索引，第二个（可选）：要取的字符数（不写 = 取到末尾）
            const QString rest = line.mid(match.capturedLength(0));

            // 根据前缀决定颜色
            QTextCharFormat prefixFmt;
            prefixFmt.setForeground(logColor(levelFromPrefix(prefix)));

            // 先插入带颜色的前缀
            // setCharFormat用于设置接下来插入的文字的格式（颜色、粗体等），只影响后续的 insertText()，不改现有的内容
            cursor.setCharFormat(prefixFmt);
            cursor.insertText(prefix);

            // 恢复正常颜色，插入空格 + 正文
            cursor.setCharFormat(normalFmt);
            cursor.insertText(spaces);
            cursor.insertText(rest);
        }
        else
        {
            // 没有前缀，整行用默认颜色
            cursor.setCharFormat(normalFmt);
            cursor.insertText(line);
        }

        cursor.insertText("\n");    // 每行结束后加换行
    }

    // 更新光标位置并滚动到最新内容
    log_textEdit->setTextCursor(cursor);
    log_textEdit->ensureCursorVisible();
}

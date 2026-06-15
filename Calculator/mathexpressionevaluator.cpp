#include "mathexpressionevaluator.h"
#include <QDebug>
#include <QLocale>
#include <limits>

// 使用高精度浮点数类型
// #ifdef Q_OS_WIN
// typedef long double long double;
// #else
// typedef long double long double;
// #endif

MathExpressionEvaluator::MathExpressionEvaluator()
    : m_pos(0)
    , m_hasError(false)
{
    // 初始化运算符优先级
    m_precedence['+'] = 1;
    m_precedence['-'] = 1;
    m_precedence['*'] = 2;
    m_precedence['/'] = 2;
    m_precedence['%'] = 2;
    m_precedence['^'] = 3;  // 幂运算优先级最高
}

QString MathExpressionEvaluator::evaluate(const QString& expression)
{
    // 重置状态
    m_hasError = false;
    m_errorMessage.clear();
    m_pos = 0;

    // 检查表达式是否为空
    if (expression.isEmpty())
    {
        m_hasError = true;
        m_errorMessage = "表达式为空";
        return m_errorMessage;
    }

    // 移除空格
    m_expression = removeSpaces(expression);

    // 验证表达式语法
    if (!validateExpression())
    {
        m_hasError = true;
        return "错误: " + m_errorMessage;
    }

    try
    {
        // 转换为后缀表达式
        QVector<QString> postfix = infixToPostfix();
        if (m_hasError)
        {
            return "错误: " + m_errorMessage;
        }

        // 计算后缀表达式
        long double result = evaluatePostfix(postfix);
        if (m_hasError)
        {
            return "错误: " + m_errorMessage;
        }

        // 格式化并返回结果
        return formatNumber(result);

    }
    catch (const std::exception& e)
    {
        m_hasError = true;
        m_errorMessage = QString("计算异常: %1").arg(e.what());
        return "错误: " + m_errorMessage;
    }
}

QString MathExpressionEvaluator::lastError() const
{
    return m_errorMessage;
}

bool MathExpressionEvaluator::hasError() const
{
    return m_hasError;
}

void MathExpressionEvaluator::clearError()
{
    m_hasError = false;
    m_errorMessage.clear();
}

QString MathExpressionEvaluator::removeSpaces(const QString& str)
{
    QString result;
    for (int i = 0; i < str.length(); ++i)
    {
        if (!str.at(i).isSpace())
        {
            result.append(str.at(i));
        }
    }

    return result;
}

bool MathExpressionEvaluator::validateExpression()
{
    int bracketCount = 0;
    bool lastWasOperator = true;  // 开始时允许负号
    bool lastWasDot = false;
    bool lastWasE = false;

    for (int i = 0; i < m_expression.length(); ++i)
    {
        QChar ch = m_expression.at(i);

        // 检查非法字符
        if (!isDigitOrDot(ch) && !isValidOperator(ch) && ch != 'e' && ch != 'E')
        {
            m_errorMessage = QString("非法字符: '%1'").arg(ch);
            return false;
        }

        // 处理数字和小数点
        if (isDigitOrDot(ch))
        {
            if (ch == '.')
            {
                // 检查连续小数点
                if (lastWasDot)
                {
                    m_errorMessage = "小数点重复出现";
                    return false;
                }

                lastWasDot = true;

                // 检查小数点位置是否合法
                if ((i == 0 || !m_expression.at(i-1).isDigit()) &&
                    (i == m_expression.length()-1 || !m_expression.at(i+1).isDigit()))
                {
                    m_errorMessage = "小数点使用错误";
                    return false;
                }
            }
            else
            {
                lastWasDot = false;
            }
            lastWasOperator = false;
            lastWasE = false;
        }
        // 处理科学计数法中的e/E
        else if (ch == 'e' || ch == 'E')
        {
            if (lastWasE)
            {
                m_errorMessage = "科学计数法格式错误";
                return false;
            }

            if (i == 0 || i == m_expression.length()-1)
            {
                m_errorMessage = "科学计数法格式错误";
                return false;
            }

            // 检查e/E前面是否为数字或右括号
            QChar prev = m_expression.at(i-1);
            if (!prev.isDigit() && prev != ')')
            {
                m_errorMessage = "科学计数法格式错误";
                return false;
            }

            // 检查e/E后面是否为数字或+/-号
            if (i + 1 < m_expression.length())
            {
                QChar next = m_expression.at(i+1);
                if (!next.isDigit() && next != '+' && next != '-')
                {
                    m_errorMessage = "科学计数法格式错误";
                    return false;
                }
            }

            lastWasE = true;
            lastWasOperator = true;
            lastWasDot = false;
        }
        // 处理运算符
        else if (isValidOperator(ch))
        {
            if (ch == '(')
            {
                bracketCount++;
                lastWasOperator = true;  // 左括号后可以接负号
            }
            else if (ch == ')')
            {
                bracketCount--;
                if (bracketCount < 0)
                {
                    m_errorMessage = "括号不匹配";
                    return false;
                }
                lastWasOperator = false;
            }
            else
            {
                // 检查运算符连续出现的情况（负号除外）
                if (lastWasOperator && ch != '-')
                {
                    m_errorMessage = "运算符连续出现";
                    return false;
                }

                lastWasOperator = true;
            }

            lastWasDot = false;
            lastWasE = false;
        }
    }

    // 检查括号是否匹配
    if (bracketCount != 0)
    {
        m_errorMessage = "括号不匹配";
        return false;
    }

    // 检查表达式是否以运算符结尾（除了右括号）
    if (!m_expression.isEmpty())
    {
        QChar lastChar = m_expression.at(m_expression.length() - 1);
        if (isValidOperator(lastChar) && lastChar != ')') {
            m_errorMessage = "表达式以运算符结尾";
            return false;
        }
    }

    return true;
}

bool MathExpressionEvaluator::isValidOperator(QChar ch) const
{
    return ch == '+' || ch == '-' || ch == '*' ||
           ch == '/' || ch == '%' || ch == '^' ||
           ch == '(' || ch == ')';
}

bool MathExpressionEvaluator::isDigitOrDot(QChar ch) const
{
    return ch.isDigit() || ch == '.';
}

QString MathExpressionEvaluator::getNextToken()
{
    // 跳过空白字符
    while (m_pos < m_expression.length() && m_expression.at(m_pos).isSpace())
    {
        m_pos++;
    }

    if (m_pos >= m_expression.length())
    {
        return QString();
    }

    QChar ch = m_expression.at(m_pos);

    // 如果是运算符
    if (isValidOperator(ch))
    {
        m_pos++;
        return QString(ch);
    }

    // 处理一元负号（内部用 '~' 表示），例如 "~12.5"
    if (ch == '~')
    {
        QString token;
        token.append(ch);
        m_pos++;

        bool hasDot = false;
        bool hasE = false;
        bool hasDigit = false;

        while (m_pos < m_expression.length())
        {
            QChar current = m_expression.at(m_pos);

            if (current.isDigit())
            {
                hasDigit = true;
                token.append(current);
                m_pos++;
            }
            else if (current == '.')
            {
                if (hasDot)
                {
                    m_errorMessage = "数字中有多个小数点";
                    m_hasError = true;
                    return QString();
                }
                hasDot = true;
                token.append(current);
                m_pos++;
            }
            else if (current == 'e' || current == 'E')
            {
                if (hasE)
                {
                    m_errorMessage = "数字中有多个e/E";
                    m_hasError = true;
                    return QString();
                }
                hasE = true;
                token.append(current);
                m_pos++;

                if (m_pos < m_expression.length())
                {
                    QChar next = m_expression.at(m_pos);
                    if (next == '+' || next == '-')
                    {
                        token.append(next);
                        m_pos++;
                    }
                }
            }
            else
            {
                break;
            }
        }

        if (!hasDigit)
        {
            m_errorMessage = "负号后没有数字";
            m_hasError = true;
            return QString();
        }

        return token;
    }

    // 如果是数字或科学计数法
    QString token;
    bool hasDot = false;
    bool hasE = false;

    while (m_pos < m_expression.length())
    {
        QChar current = m_expression.at(m_pos);

        // 检查是否为数字或小数点
        if (current.isDigit() || current == '.')
        {
            if (current == '.')
            {
                if (hasDot)
                {
                    m_errorMessage = "数字中有多个小数点";
                    m_hasError = true;
                    return QString();
                }

                hasDot = true;
            }

            token.append(current);
            m_pos++;
        }
        // 检查是否为科学计数法中的e/E
        else if (current == 'e' || current == 'E')
        {
            if (hasE)
            {
                m_errorMessage = "数字中有多个e/E";
                m_hasError = true;
                return QString();
            }

            hasE = true;
            token.append(current);
            m_pos++;

            // 处理e/E后面的符号
            if (m_pos < m_expression.length())
            {
                QChar next = m_expression.at(m_pos);
                if (next == '+' || next == '-')
                {
                    token.append(next);
                    m_pos++;
                }
            }
        }
        // 检查是否为e/E后的符号（已经处理过）
        else if ((current == '+' || current == '-') &&
                 m_pos > 0 &&
                 (m_expression.at(m_pos-1) == 'e' || m_expression.at(m_pos-1) == 'E'))
        {
            // 这种情况已经在上面处理过了
            break;
        }
        // 其他情况结束数字解析
        else
        {
            break;
        }
    }

    return token;
}

QVector<QString> MathExpressionEvaluator::infixToPostfix()
{
    QVector<QString> output;
    QStack<QChar> operators;

    m_pos = 0;
    m_hasError = false;

    // 处理一元负号：将一元负号替换为特殊符号 '~'
    QString processedExpr;
    for (int i = 0; i < m_expression.length(); ++i)
    {
        QChar ch = m_expression.at(i);
        if (ch == '-')
        {
            // 检查是否是负号（一元运算符）
            if (i == 0 || m_expression.at(i-1) == '(' ||
                (isValidOperator(m_expression.at(i-1)) && m_expression.at(i-1) != ')'))
            {
                processedExpr.append('~');  // 用 '~' 表示一元负号
            }
            else
            {
                processedExpr.append(ch);
            }
        }
        else
        {
            processedExpr.append(ch);
        }
    }

    m_expression = processedExpr;
    m_precedence['~'] = 4;  // 一元负号优先级最高

    m_pos = 0;

    while (m_pos < m_expression.length())
    {
        QString token = getNextToken();

        if (token.isEmpty())
        {
            continue;
        }

        if (m_hasError)
        {
            return QVector<QString>();
        }

        QChar firstChar = token.at(0);

        // 如果是数字或一元负号
        if (firstChar.isDigit() || firstChar == '.' || firstChar == '~')
        {
            output.append(token);
        }
        // 如果是左括号
        else if (firstChar == '(')
        {
            operators.push('(');
        }
        // 如果是右括号
        else if (firstChar == ')')
        {
            while (!operators.isEmpty() && operators.top() != '(')
            {
                output.append(QString(operators.top()));
                operators.pop();
            }
            if (operators.isEmpty())
            {
                m_errorMessage = "括号不匹配";
                m_hasError = true;
                return QVector<QString>();
            }

            operators.pop();  // 弹出 '('
        }
        // 如果是运算符
        else
        {
            while (!operators.isEmpty() && operators.top() != '(' &&
                   m_precedence.value(operators.top(), 0) >= m_precedence.value(firstChar, 0))
            {
                output.append(QString(operators.top()));
                operators.pop();
            }

            operators.push(firstChar);
        }
    }

    // 将剩余的运算符弹出
    while (!operators.isEmpty())
    {
        if (operators.top() == '(')
        {
            m_errorMessage = "括号不匹配";
            m_hasError = true;
            return QVector<QString>();
        }

        output.append(QString(operators.top()));
        operators.pop();
    }

    return output;
}

long double MathExpressionEvaluator::evaluatePostfix(const QVector<QString>& postfix)
{
    QStack<long double> values;

    for (const QString& token : postfix)
    {
        QChar firstChar = token.at(0);

        // 如果是数字
        if (firstChar.isDigit() || firstChar == '.' || firstChar == '~')
        {
            bool ok = false;
            long double num = 0;

            if (firstChar == '~')
            {
                // 处理一元负号
                QString numStr = token.mid(1);
                if (numStr.isEmpty())
                {
                    m_errorMessage = "负号后没有数字";
                    m_hasError = true;
                    return 0;
                }
                num = parseScientificNotation(numStr, &ok);
                if (!ok)
                {
                    m_errorMessage = "无效数字: " + token;
                    m_hasError = true;
                    return 0;
                }
                num = -num;
            }
            else
            {
                num = parseScientificNotation(token, &ok);
                if (!ok)
                {
                    m_errorMessage = "无效数字: " + token;
                    m_hasError = true;
                    return 0;
                }
            }

            values.push(num);
        }
        // 如果是运算符
        else
        {
            if (values.size() < 2)
            {
                m_errorMessage = "运算符缺少操作数";
                m_hasError = true;
                return 0;
            }

            long double b = values.pop();
            long double a = values.pop();
            long double result = 0;

            switch (firstChar.toLatin1())
            {
            case '+':
                result = a + b;
                break;
            case '-':
                result = a - b;
                break;
            case '*':
                result = a * b;
                break;
            case '/':
                if (b == 0)
                {
                    m_errorMessage = "除零错误";
                    m_hasError = true;
                    return 0;
                }
                result = a / b;
                break;
            case '%':
                if (b == 0)
                {
                    m_errorMessage = "取模运算除零错误";
                    m_hasError = true;
                    return 0;
                }
                result = std::fmod(a, b);
                break;
            case '^':
                result = std::pow(a, b);
                if (std::isinf(result) || std::isnan(result))
                {
                    m_errorMessage = "幂运算结果超出范围";
                    m_hasError = true;
                    return 0;
                }
                break;
            default:
                m_errorMessage = QString("未知运算符: %1").arg(firstChar);
                m_hasError = true;
                return 0;
            }

            // 检查溢出
            if (std::isinf(result) || std::isnan(result))
            {
                m_errorMessage = "计算结果超出范围";
                m_hasError = true;
                return 0;
            }

            values.push(result);
        }
    }

    if (values.size() != 1)
    {
        m_errorMessage = "表达式不完整";
        m_hasError = true;
        return 0;
    }

    return values.top();
}

bool MathExpressionEvaluator::shouldUseScientificNotation(long double value) const
{
    if (value == 0)
    {
        return false;
    }

    // 按“数字位数”判断：只有当数字位数超过 20 位才使用科学计数法
    QString str = QString::number(std::abs(value), 'f', 30);

    // 移除小数末尾无意义的 0 和末尾小数点
    while (str.endsWith('0'))
    {
        str.chop(1);
    }
    if (str.endsWith('.'))
    {
        str.chop(1);
    }

    // 统计数字字符个数（忽略小数点）
    int digitCount = 0;
    for (int i = 0; i < str.length(); ++i)
    {
        if (str.at(i).isDigit())
        {
            digitCount++;
            if (digitCount > 20)
            {
                return true;
            }
        }
    }

    return false;
}

QString MathExpressionEvaluator::formatNumber(long double value) const
{
    // 检查是否为特殊值
    if (std::isnan(value))
    {
        return "NaN";
    }
    if (std::isinf(value))
    {
        return value > 0 ? "Infinity" : "-Infinity";
    }

    // 判断是否使用科学计数法
    if (shouldUseScientificNotation(value))
    {
        // 使用科学计数法，保留10位有效数字
        return QString::number(value, 'E', 10);
    }
    else
    {
        // 使用常规表示法
        QString str = QString::number(value, 'f', 15);

        // 移除末尾不必要的零
        if (str.contains('.'))
        {
            while (str.endsWith('0'))
            {
                str.chop(1);
            }
            if (str.endsWith('.'))
            {
                str.chop(1);
            }
        }

        return str;
    }
}

long double MathExpressionEvaluator::parseScientificNotation(const QString& str, bool* ok) const
{
    if (ok) *ok = true;

    // 尝试直接转换
    bool conversionOk = false;
    long double result = str.toDouble(&conversionOk);

    if (conversionOk)
    {
        return result;
    }

    // 如果直接转换失败，尝试手动解析科学计数法
    int eIndex = str.indexOf('e', 0, Qt::CaseInsensitive);
    if (eIndex == -1)
    {
        if (ok) *ok = false;
        return 0;
    }

    QString mantissaStr = str.left(eIndex);
    QString exponentStr = str.mid(eIndex + 1);

    bool mantissaOk = false;
    bool exponentOk = false;
    long double mantissa = mantissaStr.toDouble(&mantissaOk);
    int exponent = exponentStr.toInt(&exponentOk);

    if (!mantissaOk || !exponentOk)
    {
        if (ok) *ok = false;
        return 0;
    }

    return mantissa * std::pow(10.0, exponent);
}

bool MathExpressionEvaluator::isValidNumber(const QString& str) const
{
    bool ok = false;
    parseScientificNotation(str, &ok);
    return ok;
}

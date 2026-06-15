#ifndef MATHEXPRESSIONEVALUATOR_H
#define MATHEXPRESSIONEVALUATOR_H

#include <QString>
#include <QStringList>
#include <QStack>
#include <QMap>
#include <QVector>
#include <cmath>

/**
 * @brief 数学表达式计算器类
 *
 * 支持加减乘除、取模、幂运算，支持负数和小数，支持括号
 * 自动处理运算符优先级，提供完整的错误检测
 */
class MathExpressionEvaluator
{
public:
    MathExpressionEvaluator();

    /**
     * @brief 计算数学表达式的值
     * @param expression 数学表达式字符串
     * @return 计算结果的字符串表示，如果出错则返回错误信息
     */
    QString evaluate(const QString& expression);

    /**
     * @brief 获取最后一条错误信息
     * @return 错误信息字符串
     */
    QString lastError() const;

    /**
     * @brief 检查是否有错误发生
     * @return true表示有错误，false表示无错误
     */
    bool hasError() const;

    /**
     * @brief 清除错误状态
     */
    void clearError();

private:
    // 运算符优先级
    QMap<QChar, int> m_precedence;

    // 表达式字符串
    QString m_expression;

    // 当前解析位置
    int m_pos;

    // 错误标志
    bool m_hasError;

    // 错误信息
    QString m_errorMessage;

    /**
     * @brief 移除字符串中的所有空白字符
     * @param str 输入字符串
     * @return 去除空白后的字符串
     */
    QString removeSpaces(const QString& str);

    /**
     * @brief 验证表达式语法是否正确
     * @return true表示语法正确，false表示有错误
     */
    bool validateExpression();

    /**
     * @brief 检查字符是否为有效运算符
     * @param ch 待检查的字符
     * @return true表示是有效运算符
     */
    bool isValidOperator(QChar ch) const;

    /**
     * @brief 检查字符是否为数字或小数点
     * @param ch 待检查的字符
     * @return true表示是数字或小数点
     */
    bool isDigitOrDot(QChar ch) const;

    /**
     * @brief 获取下一个标记（数字或运算符）
     * @return 标记字符串
     */
    QString getNextToken();

    /**
     * @brief 将中缀表达式转换为后缀表达式（逆波兰表示法）
     * @return 后缀表达式列表
     */
    QVector<QString> infixToPostfix();

    /**
     * @brief 计算后缀表达式的值
     * @param postfix 后缀表达式列表
     * @return 计算结果
     */
    long double evaluatePostfix(const QVector<QString>& postfix);

    /**
     * @brief 检查数字是否应该使用科学计数法表示
     * @param value 数值
     * @return true表示应该使用科学计数法
     */
    bool shouldUseScientificNotation(long double value) const;

    /**
     * @brief 格式化数值为字符串
     * @param value 数值
     * @return 格式化后的字符串
     */
    QString formatNumber(long double value) const;

    /**
     * @brief 解析科学计数法字符串为数值
     * @param str 科学计数法字符串
     * @param ok 是否解析成功
     * @return 解析后的数值
     */
    long double parseScientificNotation(const QString& str, bool* ok = nullptr) const;

    /**
     * @brief 检查字符串是否为有效数字
     * @param str 待检查的字符串
     * @return true表示是有效数字
     */
    bool isValidNumber(const QString& str) const;
};

#endif // MATHEXPRESSIONEVALUATOR_H

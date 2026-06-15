#ifndef BUTTON_H
#define BUTTON_H

#include <QToolButton>

class Button : public QToolButton
{
    Q_OBJECT
public:
    Button(const QString &text, QWidget *parent = nullptr);
};

#endif // BUTTON_H

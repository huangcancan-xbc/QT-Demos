#include "button.h"

#include <QSizePolicy>

Button::Button(const QString &text, QWidget *parent)
    : QToolButton(parent)
{
    setSizePolicy(QSizePolicy:: Expanding, QSizePolicy:: Preferred);
    setText(text);
}

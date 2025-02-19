#include "board.h"
#include <QDebug>
#include <QString>

Board::Board(QObject* parent)
    : QObject{parent}
{
}

Board::Board(const QString &title, QObject *parent)
    : QObject(parent), _title(title)
{
}

void Board::setTitle(QString &title)
{
    _title = title;
}

void Board::isPressed()
{
    qDebug() << __FUNCTION__ << __LINE__;
}

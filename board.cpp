#include "board.h"
#include <QDebug>

Board::Board(QObject *parent)
    : QObject{parent}
{}

void Board::isPressed()
{
    qDebug() << __FUNCTION__ << __LINE__;
}

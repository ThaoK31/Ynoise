#include "board.h"
#include <QDebug>

Board::Board(QObject *parent)
    : QObject{parent}
{}
Board::~Board() {
}

void Board::isPressed()
{
    qDebug() << __FUNCTION__ << __LINE__;
}

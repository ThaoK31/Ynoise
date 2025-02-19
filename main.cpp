#include "soundpad.h"
#include "board.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Soundpad soundpad;
    Board board;

    QObject::connect(&soundpad, &Soundpad::play, &board, &Board::isPressed);

    soundpad.setIsPlaying(true);

    return a.exec();
}

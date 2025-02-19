#include "board.h"
#include <QDebug>
#include <QString>

Board::Board(QObject* parent)
    : QObject{parent}
{
}

Board::Board(const QString& title, QObject* parent)
    : QObject(parent), _title(title)
{
}

void Board::setTitle(const QString& title)
{
    _title = title != _title ? title : _title;
    emit titleUpdated();
}

void Board::addSoundpad(Soundpad* soundpad)
{
    if (soundpad != nullptr)
    {
        _soundpads.append(soundpad);
        emit soundpadsUpdated();
    }
}

void Board::removeSoundpad(Soundpad* soundpad)
{
    if (soundpad != nullptr)
    {
        int index = _soundpads.indexOf(soundpad);
        // Check if the index is found.
        if (index != -1)
        {
            _soundpads.removeAt(index);
            emit soundpadUpdated();
        }
    }
}

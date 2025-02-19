#include "board.h"

Board::Board(QObject *parent)
    : QObject{parent}
{}

void Board::importSound(const QString &filePath, const QString &title)
{
    Soundpad newSound(filePath, title, this);  // Crée un nouvel objet Soundpad
    m_soundpads.append(newSound);  // Ajoute le Soundpad à la liste
    emit importSound();  // Émet le signal indiquant qu'un son a été importé
    qDebug() << "Son importé: " << title << " à partir de " << filePath << __LINE__;
}

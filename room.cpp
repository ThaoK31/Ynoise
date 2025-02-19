#include "room.h"

Room::Room(QObject *parent)
    : QObject{parent}, title(""), serverIP(""), port(0) {}

Room::Room(const QString &title, const QString &serverIP, const int port, QObject *parent)
    : QObject{parent}, title(title), serverIP(serverIP), port(port) {}

Room::~Room() {
}

void Room::addBoard(Board *board) {
    boards.append(board);
}

void Room::joinRoom() {
//A voir si il faut pas le mettre au niveau du USER plutot
}



void Room::deleteSoundBoardFromRoom(Board *board) {
    boards.removeOne(board);
    
}


// Getters
QString Room::getTitle() const { return title; }
QString Room::getServerIP() const { return serverIP; }
int Room::getPort() const { return port; }

// Setters
void Room::setTitle(const QString &title) { this->title = title; }
void Room::setServerIP(const QString &ip) { this->serverIP = ip; }
void Room::setPort(int port) { this->port = port; }




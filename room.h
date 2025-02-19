#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QString>
#include <QList>
#include "Board.h"

class Room : public QObject
{
    Q_OBJECT
public:
    explicit Room(QObject *parent = nullptr);
    Room(const QString &title, const QString &setServerIP, const int port, QObject *parent = nullptr);
    ~Room();

    void addBoard(Board *board);
    void joinRoom();
    void deleteSoundBoardFromRoom(Board *board);

    // Getters
    QString getTitle() const;
    QString getServerIP() const;
    int getPort() const;

    // Setters
    void setTitle(const QString &title);
    void setServerIP(const QString &ip);
    void setPort(int port);

private:
    QString title;
    QString serverIP;
    int port;
    QList<Board*> boards;

signals:
};

#endif // ROOM_H

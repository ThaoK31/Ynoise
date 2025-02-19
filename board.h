#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QString>
#include <QList>
#include "soundpad.h"

// Class for contain many soundpads.
class Board : public QObject
{
    Q_OBJECT

private:
    QString _title;
    QList<Soundpad*> _soundpads;

public:
    explicit Board(QObject* parent = nullptr);

    Board(const QString& title, QObject* parent = nullptr);

    void setTitle(const QString& title);

    void addSoundpad(Soundpad* soundpad);
    void removeSoundpad(Soundpad* soundpad);

signals:
    void titleUpdated();
    void soundpadsUpdated();
};

#endif // BOARD_H

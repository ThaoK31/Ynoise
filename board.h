#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QString>

// Class for contain a list of Sounpad and for display to the UI.
class Board : public QObject
{
    Q_OBJECT
private:
    QString _title;
public:
    explicit Board(QObject *parent = nullptr);
slots:
    void isPressed();
};

#endif // BOARD_H

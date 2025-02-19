#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QString>

// Class for playing song when the Soundpad press a shortcut.
class Board : public QObject
{
    Q_OBJECT

private:
    QString _title;

public:
    explicit Board(QObject* parent = nullptr);

    Board(const QString& title, QObject* parent = nullptr);

    void setTitle(QString& title);

public slots:
    void isPressed();
};

#endif // BOARD_H

#ifndef BOARD_H
#define BOARD_H

#include <optional>
#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>
#include "Soundpad.h"

// Class for contain a list of Sounpad and for display to the UI.
class Board : public QObject
{
    Q_OBJECT
private:
    QString title;
    QList<Soundpad> m_soundpads;
public:
    explicit Board(QObject *parent = nullptr);
public slots:
    void onCreateSoundpad(const QString &title,
                          const QString &soundfilePath,
                          const QVariant &imageFilePath,
                          const std::optional<bool> &canDuplicateKey,
                          const std::optional<bool> &isPlaying,
                          const QVariant &shortcut
                          );
};

#endif // BOARD_H

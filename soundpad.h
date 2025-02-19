#ifndef SOUNDPAD_H
#define SOUNDPAD_H

#include <QObject>
#include <QString>



class Soundpad : public QObject
{
    Q_OBJECT
public:
    explicit Soundpad(QObject *parent = nullptr);

    Soundpad(const QString &title, const QString &soundFilePath,
             const QString &imageFilePath, bool canDuplicatePlay,
             const QString &shortcut, QObject *parent = nullptr);


    void dragAndDrop();
    void importSound();
    void stop();
    
    // Getters
    QString getTitle() const;
    QString getSoundFilePath() const;
    QString getImageFilePath() const;
    bool getCanDuplicatePlay() const;
    bool getIsPlaying() const;
    bool getIsPressed() const;
    QString getShortcut() const;

    // Setters
    void setTitle(const QString &title);
    void setSoundFilePath(const QString &soundFilePath);
    void setImageFilePath(const QString &imageFilePath);
    void setCanDuplicatePlay(bool canDuplicatePlay);
    void setIsPlaying(bool isPlaying);
    void setIsPressed(bool isPressed);
    void setShortcut(const QString &shortcut);

private:
    QString title;
    QString soundFilePath;
    QString imageFilePath;

    bool canDuplicatePlay;
    bool isPlaying;
    bool isPressed;

    QString shortcut;

signals: 
    void titleChanged();
    void soundFilePathChanged();
    void imageFilePathChanged();
    void canDuplicatePlayChanged();
    void isPressedChanged();
    void shortcutChanged();
    void play();
};

#endif // SOUNDPAD_H





#include "soundpad.h"


// Constructeur par défaut (toutes valeurs à zéro)
Soundpad::Soundpad(QObject *parent)
    : QObject{parent},
      title(""),
      soundFilePath(""),
      imageFilePath(""),
      canDuplicatePlay(false),
      isPlaying(false),
      isPressed(false),
      shortcut("")
{}

// Constructeur paramétré (permet d'initialiser les valeurs dès la création)
Soundpad::Soundpad(const QString &title, const QString &soundFilePath, 
    const QString &imageFilePath, bool canDuplicatePlay, 
    const QString &shortcut, QObject *parent)
: QObject(parent), title(title), soundFilePath(soundFilePath), 
imageFilePath(imageFilePath), canDuplicatePlay(canDuplicatePlay), 
isPlaying(false), isPressed(false), shortcut(shortcut) {}


// Getters
QString Soundpad::getTitle() const { return title; }
QString Soundpad::getSoundFilePath() const { return soundFilePath; }
QString Soundpad::getImageFilePath() const { return imageFilePath; }
bool Soundpad::getCanDuplicatePlay() const { return canDuplicatePlay; }
bool Soundpad::getIsPlaying() const { return isPlaying; }
bool Soundpad::getIsPressed() const { return isPressed; }
QString Soundpad::getShortcut() const { return shortcut; }

// Setters
void Soundpad::setTitle(const QString &title) { this->title = title; }
void Soundpad::setSoundFilePath(const QString &soundFilePath) { this->soundFilePath = soundFilePath; }
void Soundpad::setImageFilePath(const QString &imageFilePath) { this->imageFilePath = imageFilePath; }
void Soundpad::setCanDuplicatePlay(bool canDuplicatePlay) { this->canDuplicatePlay = canDuplicatePlay; }

void Soundpad::setIsPlaying(bool isPlaying) { 
    this->isPlaying = isPlaying; 
    emit play(); 
}

void Soundpad::setIsPressed(bool isPressed) { this->isPressed = isPressed; }
void Soundpad::setShortcut(const QString &shortcut) { this->shortcut = shortcut; }
#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QByteArray>
#include "soundpad.h"
#include "server.h"

// Class for contain a list of Sounpad and for display to the UI.
class Board : public QObject
{
    Q_OBJECT
private:
    QString _title;
    QList<Soundpad*> _soundpads;
    Server* _server;
    QHash<QString, QByteArray> _soundCache; // Cache des sons (soundId -> données)
    
public:
    explicit Board(QObject *parent = nullptr);
    ~Board();
    
    // Méthodes réseau
    void startNetworking(bool asServer = true, const QString &serverAddress = "127.0.0.1");
    void sendSoundCommand(const QString &command, const QString &soundId);
    void playSoundFromId(const QString &soundId);
    
    // Gestion des soundpads
    void addSoundpad(Soundpad* pad);
    void removeSoundpad(Soundpad* pad);
    Soundpad* getSoundpadById(const QString &id);
    
    // Getters/Setters
    QString title() const { return _title; }
    void setTitle(const QString &title) { _title = title; }
    QList<Soundpad*> soundpads() const { return _soundpads; }
    
public slots:
    void isPressed();
    void onSoundpadActivated(const QString &soundId);
    
    // Slots réseau
    void onSoundCommandReceived(const QString &command, const QString &soundId);
    void onSoundDataReceived(const QByteArray &soundData, const QString &soundId);
    void onClientConnected(const QString &clientAddress);
    void onClientDisconnected(const QString &clientAddress);
    
signals:
    void soundPlayRequested(const QString &soundId);
    void soundpadAdded(Soundpad* pad);
    void soundpadRemoved(Soundpad* pad);
    void clientConnectedSignal(const QString &clientAddress);
    void clientDisconnectedSignal(const QString &clientAddress);
};

#endif // BOARD_H

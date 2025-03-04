#include "board.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>

Board::Board(QObject *parent)
    : QObject{parent}, _title("Soundboard")
{
    _server = new Server(this);
    
    // Connexion des signaux du serveur
    connect(_server, &Server::soundCommandReceived, this, &Board::onSoundCommandReceived);
    connect(_server, &Server::soundDataReceived, this, &Board::onSoundDataReceived);
    connect(_server, &Server::clientConnected, this, &Board::onClientConnected);
    connect(_server, &Server::clientDisconnected, this, &Board::onClientDisconnected);
}

Board::~Board()
{
    // Nettoyage des soundpads
    qDeleteAll(_soundpads);
    _soundpads.clear();
}

void Board::isPressed()
{
    qDebug() << __FUNCTION__ << __LINE__;
}

void Board::startNetworking(bool asServer, const QString &serverAddress)
{
    if (asServer) {
        // Mode serveur
        _server->startSoundboardServer(45454, 45455);
        qDebug() << "Démarrage en mode serveur";
    } else {
        // Mode client - à implémenter dans une version future
        qDebug() << "Mode client non implémenté, connexion à" << serverAddress;
    }
}

void Board::sendSoundCommand(const QString &command, const QString &soundId)
{
    _server->sendSoundCommand(command, soundId);
}

void Board::playSoundFromId(const QString &soundId)
{
    if (_soundCache.contains(soundId)) {
        // Ici, vous utiliseriez normalement QAudioOutput pour jouer le son
        qDebug() << "Lecture du son" << soundId << "depuis le cache";
        
        // Envoyer une commande pour que les autres clients jouent aussi le son
        sendSoundCommand("play", soundId);
    } else {
        qDebug() << "Son" << soundId << "non trouvé dans le cache";
    }
}

void Board::addSoundpad(Soundpad* pad)
{
    if (!pad) return;
    
    _soundpads.append(pad);
    
    // Connexion du signal de jeu
    connect(pad, &Soundpad::play, [this, pad]() {
        onSoundpadActivated(pad->getTitle());
    });
    
    emit soundpadAdded(pad);
    qDebug() << "Soundpad ajouté:" << pad->getTitle();
}

void Board::removeSoundpad(Soundpad* pad)
{
    if (!pad) return;
    
    int index = _soundpads.indexOf(pad);
    if (index >= 0) {
        _soundpads.removeAt(index);
        emit soundpadRemoved(pad);
        qDebug() << "Soundpad supprimé:" << pad->getTitle();
    }
}

Soundpad* Board::getSoundpadById(const QString &id)
{
    for (Soundpad* pad : _soundpads) {
        if (pad->getTitle() == id) {
            return pad;
        }
    }
    return nullptr;
}

void Board::onSoundpadActivated(const QString &soundId)
{
    Soundpad* pad = getSoundpadById(soundId);
    if (!pad) {
        qDebug() << "Soundpad" << soundId << "non trouvé";
        return;
    }
    
    QString soundFilePath = pad->getSoundFilePath();
    if (soundFilePath.isEmpty()) {
        qDebug() << "Pas de fichier son défini pour" << soundId;
        return;
    }
    
    // Lire le son localement et l'envoyer sur le réseau
    _server->playSoundLocally(soundFilePath);
    
    // Charger le son et l'envoyer aux clients via UDP
    QFile soundFile(soundFilePath);
    if (soundFile.open(QIODevice::ReadOnly)) {
        QByteArray soundData = soundFile.readAll();
        soundFile.close();
        
        // Envoyer les données audio via UDP
        _server->sendSoundData(soundData, soundId);
    }
    
    emit soundPlayRequested(soundId);
}

void Board::onSoundCommandReceived(const QString &command, const QString &soundId)
{
    qDebug() << "Commande reçue:" << command << "pour son" << soundId;
    
    if (command == "play") {
        playSoundFromId(soundId);
    }
}

void Board::onSoundDataReceived(const QByteArray &soundData, const QString &soundId)
{
    // Stocker dans le cache
    _soundCache[soundId] = soundData;
    qDebug() << "Données audio reçues pour" << soundId << ", taille:" << soundData.size() << "octets";
    
    // Si c'est un nouveau son, créer un Soundpad
    if (!getSoundpadById(soundId)) {
        Soundpad* newPad = new Soundpad(soundId, "", "", true, "", this);
        addSoundpad(newPad);
    }
}

void Board::onClientConnected(const QString &clientAddress)
{
    qDebug() << "Client connecté:" << clientAddress;
    emit clientConnectedSignal(clientAddress);
}

void Board::onClientDisconnected(const QString &clientAddress)
{
    qDebug() << "Client déconnecté:" << clientAddress;
    emit clientDisconnectedSignal(clientAddress);
}

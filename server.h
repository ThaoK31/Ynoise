#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QObject>
#include <QByteArray>
#include <QBuffer>
#include <QAudioFormat>
#include <QNetworkDatagram>
#include <QHash>
#include <QList>
#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>

// Structure pour représenter une room
struct Room {
    QUuid uuid;
    QString name;
    QList<QTcpSocket*> members;
    QHash<QString, QByteArray> soundpadData; // Données des soundpads de la room
};

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

    void start(); // Méthode pour démarrer le serveur
    void sendUDP(); // Méthode pour envoyer via UDP
    void sendTCP(); // Méthode pour envoyer via TCP
    void send(); // Méthode générale pour envoyer des données
    
    // Nouveaux membres pour la soundboard
    void startSoundboardServer(int tcpPort = 45454, int udpPort = 45455);
    void sendSoundCommand(const QString &command, const QString &soundId); // Envoie une commande via TCP
    void sendSoundData(const QByteArray &soundData, const QString &soundId); // Envoie les données audio via UDP
    void playSoundLocally(const QString &soundFilePath); // Joue un son localement
    
    // Gestion des rooms
    QUuid createRoom(const QString &roomName);
    bool joinRoom(const QUuid &roomUuid, QTcpSocket *clientSocket);
    void leaveRoom(QTcpSocket *clientSocket);
    void broadcastToRoom(const QUuid &roomUuid, const QByteArray &data, QTcpSocket *excludeClient = nullptr);
    void sendSoundCommandToRoom(const QUuid &roomUuid, const QString &command, const QString &soundId); 
    void sendSoundDataToRoom(const QUuid &roomUuid, const QByteArray &soundData, const QString &soundId);
    void syncSoundboardData(QTcpSocket *clientSocket);
    Room* findRoomByUuid(const QUuid &uuid);
    Room* findRoomByClient(QTcpSocket *clientSocket);

signals:
    void soundCommandReceived(const QString &command, const QString &soundId);
    void soundDataReceived(const QByteArray &soundData, const QString &soundId);
    void clientConnected(const QString &clientAddress);
    void clientDisconnected(const QString &clientAddress);
    void roomCreated(const QUuid &roomUuid, const QString &roomName);
    void clientJoinedRoom(const QUuid &roomUuid, const QString &clientAddress);
    void clientLeftRoom(const QUuid &roomUuid, const QString &clientAddress);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onUdpReadyRead();
    void processSoundCommand(const QByteArray &data);
    void processRoomCommand(QTcpSocket *clientSocket, const QJsonObject &jsonObj);

private:
    QTcpServer *tcpServer;     // Serveur TCP pour accepter les connexions
    QList<QTcpSocket*> tcpClients; // Liste des clients TCP connectés
    QUdpSocket *udpSocket;     // Socket UDP pour la communication audio
    QHash<QString, QByteArray> soundCache; // Cache pour les données sonores (soundId -> données)
    QList<Room> rooms;         // Liste des rooms disponibles
    QHash<QTcpSocket*, QHostAddress> clientAddresses; // Adresses des clients pour UDP
    QHash<QTcpSocket*, quint16> clientPorts;         // Ports des clients pour UDP
    int tcpPort;
    int udpPort;
    bool isServerRunning;
};

#endif // SERVER_H

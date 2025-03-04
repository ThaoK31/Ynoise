#include "server.h"
#include <QDebug>
#include <QObject>
#include <QHostAddress>
#include <QFile>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

Server::Server(QObject *parent) : QObject(parent), 
    tcpPort(45454), 
    udpPort(45455), 
    isServerRunning(false)
{
    tcpServer = new QTcpServer(this);
    udpSocket = new QUdpSocket(this);
    tcpClients.clear();
    soundCache.clear();
    rooms.clear();
}

Server::~Server() {
    // Les objets QObject seront automatiquement détruits grâce au système de parente de Qt
    
    // Fermer les connexions
    for (QTcpSocket* socket : tcpClients) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
        }
    }
    
    if (udpSocket->state() == QAbstractSocket::BoundState) {
        udpSocket->close();
    }
    
    if (tcpServer->isListening()) {
        tcpServer->close();
    }
}

void Server::start() {
    startSoundboardServer(tcpPort, udpPort);
}

void Server::startSoundboardServer(int tcpPort, int udpPort) {
    this->tcpPort = tcpPort;
    this->udpPort = udpPort;
    
    // Démarrer le serveur TCP
    if (!tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qDebug() << "Erreur lors du démarrage du serveur TCP:" << tcpServer->errorString();
        return;
    }
    qDebug() << "Serveur TCP démarré sur le port" << tcpPort;
    
    // Configurer le socket UDP
    if (!udpSocket->bind(QHostAddress::Any, udpPort)) {
        qDebug() << "Erreur lors de la liaison du socket UDP:" << udpSocket->errorString();
        tcpServer->close(); // Fermer le serveur TCP si UDP ne fonctionne pas
        return;
    }
    qDebug() << "Socket UDP lié sur le port" << udpPort;
    
    // Connecter les signaux
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);
    connect(udpSocket, &QUdpSocket::readyRead, this, &Server::onUdpReadyRead);
    
    isServerRunning = true;
    qDebug() << "Serveur Soundboard démarré : TCP=" << tcpPort << ", UDP=" << udpPort;
}

void Server::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    if (!clientSocket) return;
    
    tcpClients.append(clientSocket);
    
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, [this, clientSocket]() {
        tcpClients.removeOne(clientSocket);
        emit clientDisconnected(clientSocket->peerAddress().toString());
        clientSocket->deleteLater();
    });
    
    qDebug() << "Nouvelle connexion TCP de" << clientSocket->peerAddress().toString();
    emit clientConnected(clientSocket->peerAddress().toString());
}

void Server::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;
    
    QByteArray data = clientSocket->readAll();
    
    if (data.isEmpty()) return;
    
    // Traiter les données comme une commande de son
    processSoundCommand(data);
    
    // Traiter les données comme une commande de room
    QString jsonString = QString::fromUtf8(data);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.contains("roomCommand")) {
            processRoomCommand(clientSocket, jsonObj);
        }
    }
}

void Server::onUdpReadyRead() {
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        
        if (datagram.isValid()) {
            QByteArray data = datagram.data();
            QString soundId;
            QByteArray soundData;
            
            // Format attendu: [soundId:taille]\n[données audio]
            int headerEndPos = data.indexOf("\n");
            if (headerEndPos > 0) {
                QString header = QString::fromUtf8(data.left(headerEndPos));
                if (header.startsWith("[") && header.contains(":]")) {
                    soundId = header.mid(1, header.indexOf(":") - 1);
                    soundData = data.mid(headerEndPos + 1);
                    
                    // Mettre en cache le son et émettre le signal
                    soundCache[soundId] = soundData;
                    emit soundDataReceived(soundData, soundId);
                    
                    qDebug() << "Données audio reçues pour" << soundId 
                             << ", taille:" << soundData.size() << "octets";
                }
            }
        }
    }
}

void Server::processSoundCommand(const QByteArray &data) {
    QString jsonString = QString::fromUtf8(data);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    
    if (!jsonDoc.isObject()) {
        qDebug() << "Format JSON invalide dans la commande";
        return;
    }
    
    QJsonObject jsonObj = jsonDoc.object();
    
    if (jsonObj.contains("command") && jsonObj.contains("soundId")) {
        QString command = jsonObj["command"].toString();
        QString soundId = jsonObj["soundId"].toString();
        
        emit soundCommandReceived(command, soundId);
        
        qDebug() << "Commande reçue:" << command << "pour son" << soundId;
        
        // Traiter la commande
        if (command == "play" && soundCache.contains(soundId)) {
            // En condition réelle, vous utiliseriez un lecteur audio pour jouer le son depuis soundCache
            qDebug() << "Lecture du son" << soundId;
        }
    }
}

void Server::sendSoundCommand(const QString &command, const QString &soundId) {
    if (tcpClients.isEmpty()) {
        qDebug() << "Aucun client connecté pour envoyer la commande";
        return;
    }
    
    QJsonObject jsonObj;
    jsonObj["command"] = command;
    jsonObj["soundId"] = soundId;
    
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson();
    
    // Envoyer à tous les clients
    for (QTcpSocket* clientSocket : tcpClients) {
        if (clientSocket->state() == QAbstractSocket::ConnectedState) {
            clientSocket->write(jsonData);
        }
    }
    
    qDebug() << "Commande" << command << "envoyée pour le son" << soundId;
}

void Server::sendSoundData(const QByteArray &soundData, const QString &soundId) {
    if (!isServerRunning) {
        qDebug() << "Le serveur n'est pas en cours d'exécution";
        return;
    }
    
    // Préparer les données avec en-tête
    QByteArray header = QString("[%1:%2]\n").arg(soundId).arg(soundData.size()).toUtf8();
    QByteArray packetData = header + soundData;
    
    // Envoyer à tous les clients connus
    for (QTcpSocket* clientSocket : tcpClients) {
        QHostAddress clientAddress = clientSocket->peerAddress();
        udpSocket->writeDatagram(packetData, clientAddress, udpPort);
    }
    
    // Mettre en cache les données sonores
    soundCache[soundId] = soundData;
    
    qDebug() << "Données audio envoyées pour" << soundId << ", taille:" << soundData.size() << "octets";
}

void Server::playSoundLocally(const QString &soundFilePath) {
    QFile soundFile(soundFilePath);
    if (!soundFile.exists()) {
        qDebug() << "Fichier son introuvable:" << soundFilePath;
        return;
    }
    
    if (!soundFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Impossible d'ouvrir le fichier son:" << soundFilePath;
        return;
    }
    
    QByteArray soundData = soundFile.readAll();
    soundFile.close();
    
    QString soundId = QFileInfo(soundFilePath).baseName();
    
    // Stocker dans le cache
    soundCache[soundId] = soundData;
    
    // Simuler la lecture (dans un vrai cas, vous utiliseriez QAudioOutput)
    qDebug() << "Lecture locale du son" << soundId << "à partir de" << soundFilePath;
    
    // En situation réelle, le code pour jouer le son serait ici
}

void Server::send() {
    // Méthode de test simplifiée
    sendUDP();
}

void Server::sendUDP() {
    const QHostAddress ipAddress("127.0.0.1");

    qDebug() << "Utilisation IP:" << ipAddress.toString();

    if (!udpSocket->bind(ipAddress, 45454)) {
        qDebug() << "Erreur de socket :" << udpSocket->errorString();
        return;
    }
    qDebug() << "Socket lié sur" << ipAddress.toString() << ":45454";

    QObject::connect(udpSocket, &QUdpSocket::readyRead, [this]() {
        while (udpSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(int(udpSocket->pendingDatagramSize()));
            QHostAddress sender;
            quint16 senderPort;
            udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            qDebug() << "Message reçu de" << sender.toString() << ":" << senderPort << "->" << datagram;
        }
    });

    // Envoyer un message UDP
    QByteArray message = "Log: Test 1";
    qint64 bytesSent = udpSocket->writeDatagram(message, ipAddress, 45454);
    if (bytesSent == -1) {
        qDebug() << "Erreur lors de l'envoi du message:" << udpSocket->errorString();
    } else {
        qDebug() << "Message envoyé avec succès:" << message;
    }
    return;
}

void Server::sendTCP() {
    if (tcpClients.isEmpty()) {
        qDebug() << "Aucun client TCP connecté, impossible d'envoyer des données";
        return;
    }

    QByteArray message = "Log: Test TCP 1";
    
    for (QTcpSocket* clientSocket : tcpClients) {
        if (clientSocket->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesSent = clientSocket->write(message);
            if (bytesSent == -1) {
                qDebug() << "Erreur lors de l'envoi du message TCP:" << clientSocket->errorString();
            } else {
                qDebug() << "Message TCP envoyé avec succès:" << message;
                clientSocket->flush();
            }
        }
    }
}

QUuid Server::createRoom(const QString &roomName)
{
    Room newRoom;
    newRoom.uuid = QUuid::createUuid();
    newRoom.name = roomName;
    rooms.append(newRoom);
    qDebug() << "Room created:" << newRoom.uuid.toString() << "Name:" << roomName;
    emit roomCreated(newRoom.uuid, roomName);
    return newRoom.uuid;
}

bool Server::joinRoom(const QUuid &roomUuid, QTcpSocket *clientSocket)
{
    Room* room = findRoomByUuid(roomUuid);
    if (!room) {
        qDebug() << "Room not found:" << roomUuid.toString();
        return false;
    }
    if (!room->members.contains(clientSocket)) {
        room->members.append(clientSocket);
        qDebug() << "Client" << clientSocket->peerAddress().toString() << "joined room" << roomUuid.toString();
        emit clientJoinedRoom(roomUuid, clientSocket->peerAddress().toString());
    }
    // Synchroniser les données de soundboard vers le client
    syncSoundboardData(clientSocket);
    return true;
}

void Server::leaveRoom(QTcpSocket *clientSocket)
{
    Room* room = findRoomByClient(clientSocket);
    if (room) {
        room->members.removeAll(clientSocket);
        qDebug() << "Client" << clientSocket->peerAddress().toString() << "left room" << room->uuid.toString();
        emit clientLeftRoom(room->uuid, clientSocket->peerAddress().toString());
    }
}

void Server::broadcastToRoom(const QUuid &roomUuid, const QByteArray &data, QTcpSocket *excludeClient)
{
    Room* room = findRoomByUuid(roomUuid);
    if (!room) return;
    for (QTcpSocket* client : room->members) {
        if (client != excludeClient && client->state() == QAbstractSocket::ConnectedState) {
            client->write(data);
        }
    }
}

void Server::sendSoundCommandToRoom(const QUuid &roomUuid, const QString &command, const QString &soundId)
{
    QJsonObject jsonObj;
    jsonObj["command"] = command;
    jsonObj["soundId"] = soundId;
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson();
    broadcastToRoom(roomUuid, jsonData);
    qDebug() << "Sound command sent to room" << roomUuid.toString() << ":" << command << soundId;
}

void Server::sendSoundDataToRoom(const QUuid &roomUuid, const QByteArray &soundData, const QString &soundId)
{
    QByteArray header = QString("[%1:%2]\n").arg(soundId).arg(soundData.size()).toUtf8();
    QByteArray packetData = header + soundData;
    Room* room = findRoomByUuid(roomUuid);
    if (!room) return;
    // Pour chaque membre de la room, envoyer via UDP si on a son adresse et port
    for (QTcpSocket* client : room->members) {
        if (clientAddresses.contains(client) && clientPorts.contains(client)) {
            QHostAddress addr = clientAddresses[client];
            quint16 port = clientPorts[client];
            udpSocket->writeDatagram(packetData, addr, port);
        }
    }
    qDebug() << "Sound data sent to room" << roomUuid.toString() << "for sound" << soundId;
}

void Server::syncSoundboardData(QTcpSocket *clientSocket)
{
    Room* room = findRoomByClient(clientSocket);
    if (!room) return;
    QJsonObject roomData;
    roomData["type"] = "soundboardSync";
    QJsonObject dataObj;
    for (auto it = room->soundpadData.begin(); it != room->soundpadData.end(); ++it) {
        dataObj[it.key()] = QString::fromUtf8(it.value().toBase64());
    }
    roomData["data"] = dataObj;
    QJsonDocument doc(roomData);
    QByteArray message = doc.toJson();
    clientSocket->write(message);
}

Room* Server::findRoomByUuid(const QUuid &uuid)
{
    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i].uuid == uuid)
            return &rooms[i];
    }
    return nullptr;
}

Room* Server::findRoomByClient(QTcpSocket *clientSocket)
{
    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i].members.contains(clientSocket))
            return &rooms[i];
    }
    return nullptr;
}

void Server::processRoomCommand(QTcpSocket *clientSocket, const QJsonObject &jsonObj)
{
    QString action = jsonObj.value("roomCommand").toString();
    if (action == "createRoom") {
        QString roomName = jsonObj.value("roomName").toString();
        QUuid newRoom = createRoom(roomName);
        // Optionnel: ajouter automatiquement le client dans la room
        joinRoom(newRoom, clientSocket);
    } else if (action == "joinRoom") {
        QString roomUuidStr = jsonObj.value("roomUuid").toString();
        QUuid roomUuid(roomUuidStr);
        bool success = joinRoom(roomUuid, clientSocket);
        if (!success) {
            qDebug() << "Failed to join room:" << roomUuidStr;
        }
    } else if (action == "leaveRoom") {
        leaveRoom(clientSocket);
    }
}
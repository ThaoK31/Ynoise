#include "room.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QThread>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QRandomGenerator>
#include <QStringList>
#include <QDebug>

Room::Room(const QString &name, bool isHost, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_invitationCode("")
    , m_hostUsername("")
    , m_server(nullptr)
    , m_clientSocket(nullptr)
    , m_isHost(isHost)
    , m_board(nullptr)
    , m_port(45678 + QRandomGenerator::global()->bounded(1000))
{
    qDebug() << "Création d'une nouvelle Room:" << name << "(Hôte:" << (isHost ? "Oui" : "Non") << ")";
    qDebug() << "Port par défaut assigné:" << m_port;
    
    // Création du board principal
    m_board = new Board("Tableau principal");
    m_board->setObjectName("1");
    
    // Connexion des signaux du board avec adaptateurs lambda pour gérer les arguments différents
    connect(m_board, &Board::soundPadAdded, this, [this](SoundPad* pad) {
        notifySoundPadAdded(m_board, pad);
    });
    
    connect(m_board, &Board::soundPadRemoved, this, [this](SoundPad* pad) {
        notifySoundPadRemoved(m_board, pad);
    });
    
    if (isHost) {
        // Création du serveur
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &Room::handleNewConnection);
        
        // Générer un code d'invitation initial
        generateInvitationCode();
        
        qDebug() << "Serveur créé, en attente de démarrage";
    } else {
        qDebug() << "Room créée en mode client, en attente de connexion";
    }
}

Room::~Room()
{
    // Arrêt du serveur ou déconnexion du client
    if (m_isHost) {
        stopServer();
    } else {
        disconnect();
    }
    
    // Suppression du board
    delete m_board;
}

void Room::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
        
        // Informer les autres utilisateurs si nous sommes l'hôte
        if (m_isHost) {
            QJsonObject data;
            data["name"] = m_name;
            broadcastMessage("room_renamed", data);
        }
    }
}

QString Room::generateInvitationCode()
{
    qDebug() << "Génération du code d'invitation...";
    
    // Obtenir l'adresse IP locale en utilisant la méthode dédiée
    QString localAddress = getLocalIpAddress();
    
    // Générer le code d'invitation (format: adresse_ip:port)
    m_invitationCode = QString("%1:%2").arg(localAddress).arg(m_port);
    qDebug() << "Code d'invitation généré:" << m_invitationCode;
    
    return m_invitationCode;
}

bool Room::joinWithCode(const QString &inviteCode, const QString &username)
{
    if (m_isHost || inviteCode.isEmpty() || username.isEmpty())
        return false;
    
    QStringList parts = inviteCode.split(":");
    
    if (parts.size() != 2) {
        qDebug() << "Format de code d'invitation invalide:" << inviteCode << "(doit être au format adresse_ip:port)";
        return false;
    }
    
    QString address = parts.at(0);
    bool ok;
    int port = parts.at(1).toInt(&ok);
    
    if (!ok || port <= 0) {
        qDebug() << "Port invalide dans le code d'invitation:" << parts.at(1);
        return false;
    }
    
    qDebug() << "Tentative de connexion à" << address << ":" << port << "avec le nom d'utilisateur" << username;
    
    return connectToRoom(address, port, username);
}

bool Room::startServer()
{
    qDebug() << "Tentative de démarrage du serveur...";
    
    if (!m_isHost) {
        qDebug() << "ERREUR: Tentative de démarrer un serveur sur un client";
        return false;
    }
    
    if (m_server) { // Test si le serveur tourne deja
        qDebug() << "ferme le serveur avant de le relancer";
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    
    qDebug() << "créer un serveur";
    m_server = new QTcpServer(this);
    
    connect(m_server, &QTcpServer::newConnection, this, &Room::handleNewConnection);
    
    bool serverStartSuccess = false;
    
    serverStartSuccess = m_server->listen(QHostAddress::Any, m_port);
    
    if (!serverStartSuccess) {
        qDebug() << "ERREUR fatal erreur dans le lancecment du serveur :" << m_server->errorString();
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }
    
    m_port = m_server->serverPort();
    qDebug() << "Serveur démarré avec succès sur le port" << m_port;
    
    QString localIp = getLocalIpAddress();
    if (localIp.isEmpty()) {
        qDebug() << "Avertissement: Impossible de déterminer l'adresse IP locale, utilisation de localhost";
        localIp = QHostAddress(QHostAddress::LocalHost).toString();
    }
    
    m_invitationCode = QString("%1:%2").arg(localIp).arg(m_port);
    qDebug() << "Code d'invitation genere :" << m_invitationCode;
    
    if (m_hostUsername.isEmpty()) {
        m_hostUsername = "Hôte";
        qDebug() << "ATTENTION: utilisation du nom par defaut:" << m_hostUsername;
    } else {
        qDebug() << "Nom d'hote defini avec " << m_hostUsername;
    }
    
    emit serverStartedSignal(localIp, m_port);
    
    return true;
}

bool Room::connectToRoom(const QString &address, int port, const QString &username)
{
    if (m_isHost || address.isEmpty() || port <= 0 || username.isEmpty()) {
        qDebug() << "Paramètres invalides pour la connexion:" 
                 << "isHost=" << m_isHost 
                 << "address=" << address 
                 << "port=" << port 
                 << "username=" << username;
        return false;
    }
    
    if (!m_clientSocket) {
        m_clientSocket = new QTcpSocket(this);
        
        QObject::connect(m_clientSocket, &QTcpSocket::readyRead,
                         this, &Room::handleDataReceived);
        QObject::connect(m_clientSocket, &QTcpSocket::disconnected,
                         this, &Room::handleClientDisconnected);
        QObject::connect(m_clientSocket, &QTcpSocket::errorOccurred,
                         this, [this](QAbstractSocket::SocketError socketError) {
            qDebug() << "Erreur de socket:" << m_clientSocket->errorString() 
                     << "Code d'erreur:" << socketError;
        });
    }
    
    if (m_clientSocket->state() == QTcpSocket::UnconnectedState) {
        qDebug() << "Tentative de connexion à" << address << ":" << port;
        
        m_clientSocket->connectToHost(address, port);
        
        if (m_clientSocket->waitForConnected(3000)) {
            qDebug() << "Connexion établie avec succès, envoi des informations utilisateur";
            
            QJsonObject data;
            data["username"] = username;
            sendMessage(m_clientSocket, "join", data);
            
            return true;
        } else {
            qDebug() << "Échec de la connexion:" << m_clientSocket->errorString();
        }
    } else {
        qDebug() << "Socket déjà dans l'état:" << m_clientSocket->state();
    }
    
    return false;
}

void Room::disconnect()
{
    if (m_clientSocket) {
        if (m_clientSocket->state() == QTcpSocket::ConnectedState) {
            QJsonObject data;
            data["reason"] = "user_disconnect";
            sendMessage(m_clientSocket, "disconnect", data);
            
            m_clientSocket->disconnectFromHost();
        }
    }
}

void Room::handleNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        
        if (socket) {
            QObject::connect(socket, &QTcpSocket::readyRead, this, &Room::handleDataReceived);
            QObject::connect(socket, &QTcpSocket::disconnected, this, &Room::handleClientDisconnected);
            
            m_users[socket] = ConnectedUser();
        }
    }
}

void Room::handleDataReceived()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    
    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject())
        return;
    
    QJsonObject message = doc.object();
    QString messageType = message["type"].toString();
    QJsonObject messageData = message["data"].toObject();
    
    processMessage(socket, messageType, messageData);
}

void Room::broadcastMessage(const QString &type, const QJsonObject &data)
{
    if (!m_isHost)
        return;
    
    QJsonObject message;
    message["type"] = type;
    message["data"] = data;
    
    QJsonDocument doc(message);
    QByteArray byteArray = doc.toJson(QJsonDocument::Compact);
    
    for (auto it = m_users.begin(); it != m_users.end(); ++it) {
        if (it.key() && it.key()->state() == QTcpSocket::ConnectedState) {
            it.key()->write(byteArray);
        }
    }
}

void Room::sendMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data)
{
    if (!socket || socket->state() != QTcpSocket::ConnectedState)
        return;
    
    QJsonObject message;
    message["type"] = type;
    message["data"] = data;
    
    QJsonDocument doc(message);
    QByteArray byteArray = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "Envoi du message" << type << "au client" << socket->peerAddress().toString();
    socket->write(byteArray);
}

QList<QByteArray> Room::splitDataIntoChunks(const QByteArray &data, int chunkSize) {
    QList<QByteArray> chunks;
    for (int i = 0; i < data.size(); i += chunkSize) {
        chunks.append(data.mid(i, chunkSize));
    }
    return chunks;
}

// Reconstitue les données originales à partir d'un QJsonArray de chunks (utile côté réception)
QByteArray reassembleData(const QJsonArray &chunks) {
    QByteArray fullData;
    for (int i = 0; i < chunks.size(); ++i) {
        QString chunkStr = chunks.at(i).toString();
        fullData.append(QByteArray::fromBase64(chunkStr.toUtf8()));
    }
    return fullData;
}

void Room::handleClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    
    if (m_users.contains(socket)) {
        QString username = m_users[socket].username;
        
        if (!username.isEmpty()) {
            emit userDisconnected(username);
            
            // Informer les autres utilisateurs
            if (m_isHost) {
                QJsonObject data;
                data["username"] = username;
                broadcastMessage("user_disconnect", data);
            }
        }
        
        m_users.remove(socket);
    }
    
    socket->deleteLater();
}

void Room::handleSoundpadRemoved(const QJsonObject &data)
{
    QString boardId = data["board_id"].toString();
    QString padId = data["pad_id"].toString();

    qDebug() << "Message 'soundpad_removed' reçu pour le board" << boardId << "et le pad" << padId;

    Board *targetBoard = findBoard(boardId);
    if (targetBoard) {
        SoundPad *padToRemove = targetBoard->getSoundPadById(padId);
        if (padToRemove) {
            targetBoard->removeSoundPad(padToRemove);
            qDebug() << "SoundPad" << padId << "supprimé du board" << boardId;
        } else {
            qDebug() << "SoundPad" << padId << "non trouvé dans le board" << boardId;
        }
    } else {
        qDebug() << "Impossible de trouver le board" << boardId << "pour supprimer le SoundPad";
    }
}

void Room::handleUserJoin(QTcpSocket *socket, const QJsonObject &data)
{
    QString username = data["username"].toString();
    
    if (!username.isEmpty()) {
        ConnectedUser user;
        user.username = username;
        m_users[socket] = user;
        
        QJsonObject joinData;
        joinData["username"] = username;
        
        // Notify other clients
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.key() != socket) {
                sendMessage(it.key(), "user_joined", joinData);
            }
        }
        
        if (m_isHost) {
            sendUserList(socket);
            sendBoardDetails(socket);
        }
        
        emit userConnected(username);
    }
}

void Room::sendUserList(QTcpSocket *socket)
{
    qDebug() << "Envoi de la liste des utilisateurs au nouveau client, incluant l'hôte:" << m_hostUsername;

    QJsonArray usersArray;
    
    if (!m_hostUsername.isEmpty()) {
        usersArray.append(m_hostUsername);
        qDebug() << "Ajout de l'hôte à la liste:" << m_hostUsername;
    } else {
        qDebug() << "ATTENTION: Le nom de l'hôte est vide!";
    }
    
    for (auto it = m_users.begin(); it != m_users.end(); ++it) {
        if (it.key() != socket && !it.value().username.isEmpty()) {
            usersArray.append(it.value().username);
            qDebug() << "Ajout d'un utilisateur à la liste:" << it.value().username;
        }
    }
    
    QJsonObject usersData;
    usersData["users"] = usersArray;
    sendMessage(socket, "users_list", usersData);
}

void Room::sendBoardDetails(QTcpSocket *socket)
{
    if (m_board) {
        if (m_board->objectName().isEmpty()) {
            m_board->setObjectName("1");
            qDebug() << "ID fixe attribué au board principal: 1";
        }
        
        QJsonObject boardData;
        boardData["board_id"] = m_board->objectName();
        boardData["board_name"] = m_board->getTitle();
        
        qDebug() << "Envoi du board principal" << m_board->objectName() << "au client";
        sendMessage(socket, "board_added", boardData);
        
        QTimer::singleShot(300, this, [this, socket] {
            sendSoundpads(socket);
        });
    }
}

void Room::sendSoundpads(QTcpSocket *socket)
{
    qDebug() << "Envoi des" << m_board->getSoundPads().size() << "pads du board principal";
    
    for (SoundPad* pad : m_board->getSoundPads()) {
        if (pad->objectName().isEmpty()) {
            QString padId = QString("pad_%1").arg(QDateTime::currentMSecsSinceEpoch());
            pad->setObjectName(padId);
            qDebug() << "ID généré pour un pad sans identifiant:" << padId;
        }
        
        QJsonObject padData;
        padData["board_id"] = m_board->objectName();
        padData["pad_id"] = pad->objectName();
        padData["title"] = pad->getTitle();
        padData["file_path"] = pad->getFilePath();
        padData["image_path"] = pad->getImagePath();
        padData["can_duplicate_play"] = pad->getCanDuplicatePlay();
        padData["shortcut"] = pad->getShortcut().toString();
        
        qDebug() << "Envoi du pad" << pad->objectName() << "au client";
        sendMessage(socket, "soundpad_added", padData);
        
        QThread::msleep(50);
    }
}

void Room::handleSoundpadAdded(QTcpSocket *socket, const QJsonObject &data)
{
    // Cette fonction traite l'ajout d'un SoundPad
    // La logique de traitement de SoundPad a été extraite en fonctions dédiées.
    QString boardId = data["board_id"].toString();
    QString padId = data["pad_id"].toString();
    
    qDebug() << "Message 'soundpad_added' reçu pour le board" << boardId << "et le pad" << padId;

    Board *targetBoard = findBoard(boardId);
    if (targetBoard) {
        SoundPad *newPad = createSoundPadFromData(data, targetBoard);
        
        if (newPad) {
            bool success = targetBoard->addSoundPadFromRemote(newPad);
            if (!success) {
                delete newPad;
                qDebug() << "Erreur lors de l'ajout du SoundPad";
            } else {
                broadcastNewPadToOthers(socket, data);
            }
        }
    } else {
        qDebug() << "Impossible de trouver le board" << boardId << "pour ajouter le SoundPad";
    }
}

SoundPad* Room::createSoundPadFromData(const QJsonObject &data, Board *targetBoard)
{
    QString padId = data["pad_id"].toString();
    QString title = data["title"].toString();
    bool canDuplicatePlay = data["can_duplicate_play"].toBool();
    QString shortcutString = data["shortcut"].toString();
    QKeySequence shortcut = QKeySequence(shortcutString);

    SoundPad *newPad = new SoundPad(title, "", "", canDuplicatePlay, shortcut, targetBoard);
    newPad->setObjectName(padId);
    
    return newPad;
}

void Room::broadcastNewPadToOthers(QTcpSocket *socket, const QJsonObject &data)
{
    if (m_isHost) {
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.key() != socket) {
                sendMessage(it.key(), "soundpad_added", data);
            }
        }
    }
}

Board* Room::findBoard(const QString &boardId)
{
    if (m_board && m_board->objectName() == boardId) {
        return m_board;
    }
    return nullptr;
}

void Room::handleBoardAdded(const QJsonObject &data)
{
    QString boardId = data["board_id"].toString();
    QString boardName = data["board_name"].toString();
    
    qDebug() << "Message 'board_added' reçu avec ID:" << boardId << "et nom:" << boardName;
    
    if (!m_isHost && m_board) {
        m_board->setObjectName("1");
        if (m_board->getTitle() != boardName) {
            m_board->setTitle(boardName);
        }
        
        qDebug() << "Board principal défini avec l'ID fixe: 1";
    }
}

void Room::processMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data)
{
    if (type == "soundpad_removed") {
        handleSoundpadRemoved(data);
    }
    else if (type == "join") {
        handleUserJoin(socket, data);
    }
    else if (type == "soundpad_added") {
        handleSoundpadAdded(socket, data);
    }
    else if (type == "board_added") {
        handleBoardAdded(data);
    }
    else if (type == "boards_loaded_confirm") {
        confirmBoardsLoaded(data["board_ids"].toArray(), socket);
    }
}

void Room::confirmBoardsLoaded(const QJsonArray &boardIds, QTcpSocket *socket)
{
    if (!socket || socket->state() != QTcpSocket::ConnectedState) {
        qDebug() << "Impossible d'envoyer la confirmation des boards : socket non valide";
        return;
    }
    
    bool allBoardsFound = true;
    QJsonArray missingBoards;
    
    // Vérifier chaque board demandé
    for (int i = 0; i < boardIds.size(); ++i) {
        QString boardId = boardIds[i].toString();
        bool boardFound = false;
        
        // Vérifier le board principal
        if (m_board && m_board->objectName() == boardId) {
            boardFound = true;
        }
        
        if (!boardFound) {
            allBoardsFound = false;
            missingBoards.append(boardId);
        }
    }
    
    // Envoyer la confirmation à l'hôte
    QJsonObject confirmData;
    confirmData["success"] = allBoardsFound;
    
    if (!allBoardsFound) {
        confirmData["missing_boards"] = missingBoards;
        qDebug() << "Certains boards demandés n'ont pas été trouvés :" << missingBoards;
    } else {
        qDebug() << "Tous les boards demandés ont été correctement chargés";
    }
    
    sendMessage(socket, "boards_loaded_confirm", confirmData);
}

void Room::notifySoundPadAdded(Board *board, SoundPad *pad)
{
    qDebug() << "-------------------------------------------------------------";
    qDebug() << "[DEBUG] Entrée dans notifySoundPadAdded pour board:" << (board ? board->objectName() : "null") 
             << "pad:" << (pad ? pad->objectName() : "null");
    
    if (!board || !pad) {
        qDebug() << "[ERREUR] notifySoundPadAdded appelé avec des pointeurs invalides";
        return;
    }
    
    // Vérification des chemins des médias
    QString soundPath = pad->getFilePath();  // Utiliser getFilePath pour le son
    QString imagePath = pad->getImagePath(); // Utiliser getImagePath pour l'image
    
    qDebug() << "[INFO] Chemin audio :" << (soundPath.isEmpty() ? "VIDE" : soundPath);
    qDebug() << "[INFO] Chemin image :" << (imagePath.isEmpty() ? "VIDE" : imagePath);
    
    if (!soundPath.isEmpty()) {
        QFileInfo soundInfo(soundPath);
        if (soundInfo.exists() && soundInfo.isFile()) {
            qDebug() << "[INFO] Sauvegarde du fichier audio :" << soundPath;
            saveMediaToPad(soundPath);
        } else {
            qDebug() << "[ERREUR] Fichier audio introuvable :" << soundPath;
        }
    }
    
    if (!imagePath.isEmpty()) {
        QFileInfo imageInfo(imagePath);
        if (imageInfo.exists() && imageInfo.isFile()) {
            qDebug() << "[INFO] Sauvegarde du fichier image :" << imagePath;
            saveMediaToPad(imagePath);
        } else {
            qDebug() << "[ERREUR] Fichier image introuvable :" << imagePath;
        }
    }
    
    // Préparation pour l'envoi au réseau
    if (board->objectName().isEmpty()) {
        board->setObjectName("1");
        qDebug() << "ID fixe attribué au board: 1";
    }
    
    if (pad->objectName().isEmpty()) {
        QString padId = QString("pad_%1").arg(QDateTime::currentMSecsSinceEpoch());
        pad->setObjectName(padId);
        qDebug() << "ID généré pour un pad sans identifiant:" << padId;
    }
    
    qDebug() << "Notification d'ajout du SoundPad:" << pad->objectName() << "au Board:" << board->objectName();
    qDebug() << "Détails du SoundPad à envoyer:";
    qDebug() << "  - Titre:" << pad->getTitle();
    qDebug() << "  - Chemin audio:" << pad->getFilePath();
    qDebug() << "  - Chemin image:" << pad->getImagePath();
    qDebug() << "  - Lecture multiple:" << (pad->getCanDuplicatePlay() ? "Oui" : "Non");
    qDebug() << "  - Raccourci:" << pad->getShortcut().toString();
    
    QJsonObject padData;
    padData["board_id"] = board->objectName();
    padData["pad_id"] = pad->objectName();
    padData["title"] = pad->getTitle();
    padData["file_path"] = pad->getFilePath();
    padData["image_path"] = pad->getImagePath();
    padData["can_duplicate_play"] = pad->getCanDuplicatePlay();
    padData["shortcut"] = pad->getShortcut().toString();
    
    QByteArray audioData = pad->getAudioData();
    QByteArray imageData = pad->getImageData();
    
    qDebug() << "Préparation des données binaires pour l'envoi:";
    
    // Taille du chunk à utiliser (par exemple 10000 octets, à ajuster selon vos besoins)
    const int chunkSize = 10000;

    if (!audioData.isEmpty()) {
        QList<QByteArray> audioChunks = splitDataIntoChunks(audioData, chunkSize);
        QJsonArray audioJsonChunks;
        for (const QByteArray &chunk : audioChunks) {
            audioJsonChunks.append(QString(chunk.toBase64()));
        }
        padData["audio_data_chunks"] = audioJsonChunks;
    }

    if (!imageData.isEmpty()) {
        QList<QByteArray> imageChunks = splitDataIntoChunks(imageData, chunkSize);
        QJsonArray imageJsonChunks;
        for (const QByteArray &chunk : imageChunks) {
            imageJsonChunks.append(QString(chunk.toBase64()));
        }
        padData["image_data_chunks"] = imageJsonChunks;
    }
    
    qDebug() << "Vérification finale des données avant envoi:";
    if (padData.contains("audio_data_chunks")) {
        QJsonArray audioChunks = padData["audio_data_chunks"].toArray();
        qDebug() << "  - Nombre de chunks audio dans JSON:" << audioChunks.size();
    } else {
        qDebug() << "  - Données audio dans JSON: ABSENTES!";
    }

    if (padData.contains("image_data_chunks")) {
        QJsonArray imageChunks = padData["image_data_chunks"].toArray();
        qDebug() << "  - Nombre de chunks image dans JSON:" << imageChunks.size();
    } else {
        qDebug() << "  - Données image dans JSON: ABSENTES!";
    }

    // Envoi des données : diffusion ou envoi direct selon le mode (hôte ou client)
    if (m_isHost) {
        qDebug() << "Diffusion du message 'soundpad_added' à tous les clients";
        broadcastMessage("soundpad_added", padData);
    } else if (m_clientSocket && m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Envoi du message 'soundpad_added' à l'hôte : "
                 << padData << " l'hôte : " << m_clientSocket;
        sendMessage(m_clientSocket, "soundpad_added", padData);
    }

    emit soundpadAdded(board, pad);
}

void Room::notifySoundPadRemoved(Board *board, SoundPad *pad)
{
    qDebug() << "-------------------------------------------------------------";
    qDebug() << "[DEBUG] Entrée dans notifySoundPadRemoved pour board:"
             << (board ? board->objectName() : "null")
             << "pad:" << (pad ? pad->objectName() : "null");

    if (!board || !pad) {
        qDebug() << "[ERREUR] notifySoundPadRemoved appelé avec des pointeurs invalides";
        return;
    }

    if (board->objectName().isEmpty()) {
        board->setObjectName("1");
        qDebug() << "ID fixe attribué au board dans notifySoundPadRemoved: 1";
    }

    if (pad->objectName().isEmpty()) {
        QString padId = QString("pad_%1").arg(QDateTime::currentMSecsSinceEpoch());
        pad->setObjectName(padId);
        qDebug() << "ID généré pour un pad sans identifiant dans notifySoundPadRemoved:" << padId;
    }

    qDebug() << "Notification de suppression du SoundPad:" << pad->objectName() << "du Board:" << board->objectName();
    qDebug() << "Détails du SoundPad à envoyer:";
    qDebug() << "  - Titre:" << pad->getTitle();
    qDebug() << "  - Chemin audio:" << pad->getFilePath();
    qDebug() << "  - Chemin image:" << pad->getImagePath();
    qDebug() << "  - Lecture multiple:" << (pad->getCanDuplicatePlay() ? "Oui" : "Non");
    qDebug() << "  - Raccourci:" << pad->getShortcut().toString();

    QJsonObject padData;
    padData["board_id"] = board->objectName();
    padData["pad_id"] = pad->objectName();
    padData["title"] = pad->getTitle();
    padData["file_path"] = pad->getFilePath();
    padData["image_path"] = pad->getImagePath();
    padData["can_duplicate_play"] = pad->getCanDuplicatePlay();
    padData["shortcut"] = pad->getShortcut().toString();

    QByteArray audioData = pad->getAudioData();
    QByteArray imageData = pad->getImageData();

    qDebug() << "Préparation des données binaires pour l'envoi:";

    // Taille du chunk à utiliser (par exemple 10000 octets, à ajuster selon vos besoins)
    const int chunkSize = 10000;

    if (!audioData.isEmpty()) {
        QList<QByteArray> audioChunks = splitDataIntoChunks(audioData, chunkSize);
        QJsonArray audioJsonChunks;
        for (const QByteArray &chunk : audioChunks) {
            audioJsonChunks.append(QString(chunk.toBase64()));
        }
        padData["audio_data_chunks"] = audioJsonChunks;
    }

    if (!imageData.isEmpty()) {
        QList<QByteArray> imageChunks = splitDataIntoChunks(imageData, chunkSize);
        QJsonArray imageJsonChunks;
        for (const QByteArray &chunk : imageChunks) {
            imageJsonChunks.append(QString(chunk.toBase64()));
        }
        padData["image_data_chunks"] = imageJsonChunks;
    }

    qDebug() << "Vérification finale des données avant envoi:";
    if (padData.contains("audio_data_chunks")) {
        QJsonArray audioChunks = padData["audio_data_chunks"].toArray();
        qDebug() << "  - Nombre de chunks audio dans JSON:" << audioChunks.size();
    } else {
        qDebug() << "  - Données audio dans JSON: ABSENTES!";
    }

    if (padData.contains("image_data_chunks")) {
        QJsonArray imageChunks = padData["image_data_chunks"].toArray();
        qDebug() << "  - Nombre de chunks image dans JSON:" << imageChunks.size();
    } else {
        qDebug() << "  - Données image dans JSON: ABSENTES!";
    }

    // Envoi des données : diffusion ou envoi direct selon le mode (hôte ou client)
    if (m_isHost) {
        qDebug() << "Diffusion du message 'soundpad_removed' à tous les clients";
        broadcastMessage("soundpad_removed", padData);
    } else if (m_clientSocket && m_clientSocket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Envoi du message 'soundpad_removed' à l'hôte : "
                 << padData << " l'hôte : " << m_clientSocket;
        sendMessage(m_clientSocket, "soundpad_removed", padData);
    }

    emit soundpadRemoved(board, pad);
}

void Room::stopServer()
{
    if (m_server && m_server->isListening()) {
        // Fermer toutes les connexions
        for (QTcpSocket *socket : m_users.keys()) {
            socket->close();
            socket->deleteLater();
        }
        
        // Vider la liste des utilisateurs
        m_users.clear();
        
        // Arrêter le serveur
        m_server->close();
        
        // Émettre le signal d'arrêt du serveur
        emit serverStopped();
    }
}

/**
 * @brief Obtenir l'adresse IP locale
 * @return Adresse IP locale
 */
QString Room::getLocalIpAddress() const
{
    // Parcourir les interfaces réseau disponibles
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        // Exclure les adresses de loopback (127.0.0.1)
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    
    // Utiliser localhost si aucune autre adresse n'est disponible
    return QHostAddress(QHostAddress::LocalHost).toString();
}

QStringList Room::connectedUsers() const
{
    QStringList userList;
    
    // Ajouter l'hôte s'il est défini
    if (!m_hostUsername.isEmpty()) {
        userList.append(m_hostUsername);
    }
    
    // Ajouter les autres utilisateurs connectés
    for (auto it = m_users.constBegin(); it != m_users.constEnd(); ++it) {
        if (!it.value().username.isEmpty()) {
            userList.append(it.value().username);
        }
    }
    
    return userList;
}

void Room::installFileLocally(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    
    // Conservation de l'arborescence complète depuis le dossier Documents
    QString basePath = QDir::homePath() + "/Documents/";
    QString relativePath = fileInfo.absoluteFilePath().remove(basePath);
    QString destination = QDir::homePath() + "/InstalledFiles/" + relativePath;
    
    QDir().mkpath(QFileInfo(destination).absolutePath());
    
    if (QFile::copy(filePath, destination)) {
        qDebug() << "[SUCCÈS] Fichier installé dans :" << QDir::toNativeSeparators(destination);
        emit fileInstalled(destination); // Émission d'un signal si nécessaire
    } else {
        qDebug() << "[ERREUR] Échec de l'installation vers :" << QDir::toNativeSeparators(destination);
        qDebug() << "         Vérifiez les permissions d'écriture et l'espace disque";
    }
}

void Room::saveMediaToPad(const QString &mediaPath) {
    // Vérification que le chemin source existe
    QFileInfo fileInfo(mediaPath);
    
    if (mediaPath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
        qDebug() << "[ERREUR] Chemin du média invalide ou fichier inexistant :" << mediaPath;
        return;
    }
    
    // Création du répertoire de destination
    QString mediaDir = QDir::currentPath() + "/media";
    QDir dir(mediaDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "[ERREUR] Impossible de créer le répertoire :" << mediaDir;
            return;
        }
        qDebug() << "[INFO] Répertoire media créé :" << mediaDir;
    }
    
    // Construction du chemin de destination
    QString destination = mediaDir + "/" + fileInfo.fileName();
    
    // Vérification si le fichier existe déjà
    if (QFile::exists(destination)) {
        QFile::remove(destination);
        qDebug() << "[INFO] Fichier existant supprimé :" << destination;
    }
    
    // Copie du fichier
    if (QFile::copy(mediaPath, destination)) {
        qDebug() << "[SUCCÈS] Média sauvegardé dans :" << QDir::toNativeSeparators(destination);
    } else {
        qDebug() << "[ERREUR] Échec de la sauvegarde du média :" << QDir::toNativeSeparators(destination);
        qDebug() << "         Source :" << mediaPath;
        qDebug() << "         Vérifiez les permissions et l'espace disque";
    }
}

Board* Room::getBoard() const
{
    qDebug() << "Récupération du board principal";
    return m_board;
}

void Room::setHostUsername(const QString &username)
{
    qDebug() << "Définition du nom d'utilisateur de l'hôte :" << username;
    m_hostUsername = username;
    
    // Si nous sommes l'hôte, annoncer le changement de nom
    if (m_isHost && m_server && m_server->isListening()) {
        QJsonObject data;
        data["username"] = username;
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate); // Ajout d'un timestamp
        broadcastMessage("host_username_changed", data);
    }
}

QString Room::invitationCode() const
{
    qDebug() << "Récupération du code d'invitation :" << m_invitationCode;
    return m_invitationCode;
}

QString Room::hostUsername() const
{
    qDebug() << "Récupération du nom d'utilisateur de l'hôte :" << m_hostUsername;
    return m_hostUsername;
}

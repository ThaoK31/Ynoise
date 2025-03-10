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
    , m_port(0)
{
    qDebug() << "Création d'une nouvelle Room:" << name << "(Hôte:" << (isHost ? "Oui" : "Non") << ")";
    
    // Définir un port par défaut
    m_port = 45678 + QRandomGenerator::global()->bounded(1000);
    qDebug() << "Port par défaut assigné:" << m_port;
    
    // Générer un code d'invitation par défaut pour faciliter le débogage
    generateInvitationCode();
    
    // Créer un board par défaut
    m_board = new Board("Board principal");
    
    // Définir un identifiant fixe pour le board (toujours "1")
    m_board->setObjectName("1");
    qDebug() << "ID fixe attribué au board principal: 1";
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
    // Empêcher l'hôte d'utiliser cette méthode
    if (m_isHost || inviteCode.isEmpty() || username.isEmpty())
        return false;
    
    // Format du code d'invitation: "adresse_ip:port"
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
    
    // Vérifier si l'utilisateur est configuré comme hôte
    if (!m_isHost) {
        qDebug() << "ERREUR: Tentative de démarrer un serveur sur un client";
        return false;
    }
    
    // Fermer le serveur existant si nécessaire
    if (m_server) {
        qDebug() << "Serveur existant détecté, fermeture avant redémarrage";
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    
    // Créer le serveur
    qDebug() << "Création d'un nouveau serveur";
    m_server = new QTcpServer(this);
    
    // Connecter le signal de nouvelle connexion
    connect(m_server, &QTcpServer::newConnection, this, &Room::handleNewConnection);
    
    // Tenter le démarrage sur différentes adresses si nécessaire
    bool serverStartSuccess = false;
    
    // D'abord essayer Any (toutes les interfaces)
    qDebug() << "Tentative de démarrage sur QHostAddress::Any";
    serverStartSuccess = m_server->listen(QHostAddress::Any, m_port);
    
    // Si ça ne fonctionne pas, essayer sur localhost
    if (!serverStartSuccess) {
        qDebug() << "Échec sur Any, tentative sur localhost";
        serverStartSuccess = m_server->listen(QHostAddress::LocalHost, m_port);
    }
    
    // Vérifier si le serveur a démarré
    if (!serverStartSuccess) {
        qDebug() << "ERREUR critique lors du démarrage du serveur:" << m_server->errorString();
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }
    
    // Récupérer le port sur lequel le serveur écoute (au cas où le port demandé n'était pas disponible)
    m_port = m_server->serverPort();
    qDebug() << "Serveur démarré avec succès sur le port" << m_port;
    
    // Obtenir l'adresse IP locale pour générer le code d'invitation
    QString localIp = getLocalIpAddress();
    if (localIp.isEmpty()) {
        qDebug() << "Avertissement: Impossible de déterminer l'adresse IP locale, utilisation de localhost";
        localIp = QHostAddress(QHostAddress::LocalHost).toString();
    }
    
    // Générer le code d'invitation (format: adresse_ip:port)
    m_invitationCode = QString("%1:%2").arg(localIp).arg(m_port);
    qDebug() << "Code d'invitation généré:" << m_invitationCode;
    
    // Si le nom d'hôte n'a pas été défini, utiliser un nom par défaut
    if (m_hostUsername.isEmpty()) {
        m_hostUsername = "Hôte";
        qDebug() << "ATTENTION: Nom d'hôte non défini, utilisation du nom par défaut:" << m_hostUsername;
    } else {
        qDebug() << "Nom d'hôte défini:" << m_hostUsername;
    }
    
    // Émettre le signal de démarrage du serveur
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
    
    // Créer un nouveau socket client si nécessaire
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
    
    // Se connecter au serveur si le socket n'est pas déjà connecté
    if (m_clientSocket->state() == QTcpSocket::UnconnectedState) {
        qDebug() << "Tentative de connexion à" << address << ":" << port;
        
        m_clientSocket->connectToHost(address, port);
        
        if (m_clientSocket->waitForConnected(3000)) {
            qDebug() << "Connexion établie avec succès, envoi des informations utilisateur";
            
            // Envoyer les informations utilisateur
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
            // Informer le serveur de la déconnexion
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
            
            // L'utilisateur sera correctement identifié quand il enverra ses infos
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
    
    qDebug() << "Envoi du message" << type << "au client" << socket->peerAddress().toString() << "et comme message" << QJsonDocument(data).toJson();

    socket->write(byteArray);
}

void Room::processMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data)
{
    if (type == "soundpad_removed") {
        // Récupérer les informations du SoundPad supprimé
        QString boardId = data["board_id"].toString();
        QString padId = data["pad_id"].toString();

        qDebug() << "Message 'soundpad_removed' reçu pour le board" << boardId << "et le pad" << padId;

        // Vérifier que nous avons un board correspondant
        Board *targetBoard = nullptr;
        if (m_board && m_board->objectName() == boardId) {
            targetBoard = m_board;
        }

        if (targetBoard) {
            // Supprimer le SoundPad du board
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
    if (type == "join") {
        // Un utilisateur vient de rejoindre
        QString username = data["username"].toString();
        
        if (!username.isEmpty()) {
            // Enregistrer l'utilisateur
            ConnectedUser user;
            user.username = username;
            m_users[socket] = user;
            
            // Notifier les autres utilisateurs
            QJsonObject joinData;
            joinData["username"] = username;
            
            // Envoyer aux autres clients sauf celui qui vient de se connecter
            for (auto it = m_users.begin(); it != m_users.end(); ++it) {
                if (it.key() != socket) {
                    sendMessage(it.key(), "user_joined", joinData);
                }
            }
            
            // Si nous sommes l'hôte, envoyer notre nom d'utilisateur au client qui vient de se connecter
            if (m_isHost) {
                qDebug() << "Envoi de la liste des utilisateurs au nouveau client, incluant l'hôte:" << m_hostUsername;
                
                // Envoyer la liste de tous les utilisateurs déjà connectés (y compris l'hôte)
                QJsonArray usersArray;
                
                // Ajouter l'hôte (nous-mêmes) à la liste
                if (!m_hostUsername.isEmpty()) {
                    usersArray.append(m_hostUsername);
                    qDebug() << "Ajout de l'hôte à la liste:" << m_hostUsername;
                } else {
                    qDebug() << "ATTENTION: Le nom de l'hôte est vide!";
                }
                
                // Ajouter tous les autres utilisateurs connectés
                for (auto it = m_users.begin(); it != m_users.end(); ++it) {
                    if (it.key() != socket && !it.value().username.isEmpty()) {
                        usersArray.append(it.value().username);
                        qDebug() << "Ajout d'un utilisateur à la liste:" << it.value().username;
                    }
                }
                
                // Envoyer la liste complète au nouveau client
                QJsonObject usersData;
                usersData["users"] = usersArray;
                sendMessage(socket, "users_list", usersData);
                
                // Si nous avons un board, l'envoyer au client
                if (m_board) {
                    // Utiliser l'ID fixe (toujours "1") pour le board principal
                    if (m_board->objectName().isEmpty()) {
                        m_board->setObjectName("1");
                        qDebug() << "ID fixe attribué au board principal: 1";
                    }
                    
                    // Envoyer les informations du board
                    QJsonObject boardData;
                    boardData["board_id"] = m_board->objectName();
                    boardData["board_name"] = m_board->getTitle();
                    
                    qDebug() << "Envoi du board principal" << m_board->objectName() << "au client";
                    sendMessage(socket, "board_added", boardData);
                    
                    // Envoyer tous les SoundPads du board après un court délai
                    QTimer::singleShot(300, this, [this, socket] {
                        // Envoyer les SoundPads du board principal
                        qDebug() << "Envoi des" << m_board->getSoundPads().size() << "pads du board principal";
                        
                        for (SoundPad* pad : m_board->getSoundPads()) {
                            // Générer un ID pour le pad s'il n'en a pas
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
                            
                            // Petit délai pour éviter de surcharger le client
                            QThread::msleep(50);
                        }
                    });
                }
            }
            
            // Émettre le signal pour informer l'interface
            emit userConnected(username);
        }
    }
    else if (type == "soundpad_added") {
        QString audioDataStr = data["audio_data"].toString();
        qDebug() << "  - Vérification encodage audio: " << (audioDataStr.isEmpty() ? "VIDE!" : "OK")
                 << " (premiers 20 caractères: " << (audioDataStr.length() > 20 ? audioDataStr.left(20) + "..." : audioDataStr) << ")";


        // Récupérer les informations du SoundPad
        QString boardId = data["board_id"].toString();
        QString padId = data["pad_id"].toString();
        QString title = data["title"].toString();
        QString filePath = data["file_path"].toString();
        QString imagePath = data["image_path"].toString();
        bool canDuplicatePlay = data["can_duplicate_play"].toBool();
        QString shortcutString = data["shortcut"].toString();
        QKeySequence shortcut = QKeySequence(shortcutString);
        
        qDebug() << "Réception d'un SoundPad:";
        qDebug() << "  - ID Board:" << boardId;
        qDebug() << "  - ID Pad:" << padId;
        qDebug() << "  - Titre:" << title;
        qDebug() << "  - Chemin audio:" << filePath;
        qDebug() << "  - Chemin image:" << imagePath;
        qDebug() << "  - Lecture multiple:" << (canDuplicatePlay ? "Oui" : "Non");
        qDebug() << "  - Raccourci:" << shortcutString;
        
        // Récupérer les données binaires encodées en base64
        QByteArray audioData;
        QByteArray imageData;



        qDebug() << "Vérification des données binaires reçues:";
        
        bool audioDataReceived = false;
        if (data.contains("audio_data")) {
            QString encodedAudio = data["audio_data"].toString();
            qDebug() << "  - Données audio encodées reçues:" << encodedAudio.length() << "caractères";
            
            if (!encodedAudio.isEmpty()) {
                audioData = QByteArray::fromBase64(encodedAudio.toLatin1());
                qDebug() << "  - Données audio décodées:" << audioData.size() << "octets";
                
                if (audioData.isEmpty()) {
                    qDebug() << "  - ERREUR: Échec du décodage des données audio";
                } else {
                    audioDataReceived = true;
                    qDebug() << "  - Décodage des données audio réussi";
                }
            }
        } else {
            qDebug() << "  - Pas de données audio reçues";
        }
        
        bool imageDataReceived = false;
        if (data.contains("image_data")) {
            QString encodedImage = data["image_data"].toString();
            qDebug() << "  - Données image encodées reçues:" << encodedImage.length() << "caractères";
            
            if (!encodedImage.isEmpty()) {
                imageData = QByteArray::fromBase64(encodedImage.toLatin1());
                qDebug() << "  - Données image décodées:" << imageData.size() << "octets";
                
                if (imageData.isEmpty()) {
                    qDebug() << "  - ERREUR: Échec du décodage des données image";
                } else {
                    imageDataReceived = true;
                    qDebug() << "  - Décodage des données image réussi";
                }
            }
        } else {
            qDebug() << "  - Pas de données image reçues";
        }
        
        qDebug() << "Message 'soundpad_added' reçu pour le board" << boardId << "et le pad" << padId;
        
        // Vérifier que nous avons un board correspondant
        Board *targetBoard = nullptr;
        if (m_board && m_board->objectName() == boardId) {
            targetBoard = m_board;
        } else {
            qDebug() << "[ERREUR] Board cible non trouvé:" << boardId;
        }
        
        if (targetBoard) {
            qDebug() << "[INFO] Création d'un nouveau SoundPad avec ID:" << padId;
            
            // Créer le SoundPad sans charger immédiatement les médias
            SoundPad *newPad = new SoundPad(title, "", "", canDuplicatePlay, shortcut, targetBoard);
            newPad->setObjectName(padId);
            
            // Définir explicitement le titre
            if (!title.isEmpty()) {
                newPad->blockSignals(true);
                newPad->setTitle(title);
                newPad->blockSignals(false);
                qDebug() << "[INFO] Titre défini:" << title;
            }
            
            // Flag pour suivre si les médias ont été traités
            bool audioProcessed = false;
            bool imageProcessed = false;
            
            // Priorité aux données reçues en base64
            if (audioDataReceived) {
                qDebug() << "[INFO] Définition des données audio depuis les données reçues";
                newPad->setAudioData(audioData);
                
                // Vérifier si les données ont été correctement définies
                if (!newPad->getAudioData().isEmpty()) {
                    audioProcessed = true;
                    qDebug() << "[INFO] Données audio définies sur le nouveau SoundPad:" << audioData.size() << "octets";
                    
                    // Si le fichier n'existe pas localement, le sauvegarder
                    if (!filePath.isEmpty() && !QFile::exists(filePath)) {
                        // Créer un chemin temporaire pour sauvegarder le fichier audio
                        QFileInfo fileInfo(filePath);
                        QString tempDir = QDir::tempPath() + "/Ynoise/";
                        QDir().mkpath(tempDir);
                        QString tempFilePath = tempDir + fileInfo.fileName();
                        
                        qDebug() << "[INFO] Sauvegarde temporaire du fichier audio reçu vers:" << tempFilePath;
                        
                        if (newPad->saveAudioToFile(tempFilePath)) {
                            qDebug() << "[INFO] Fichier audio temporaire sauvegardé avec succès";
                            
                            // Installer le fichier en conservant la structure de répertoire
                            installFileLocally(tempFilePath);
                            
                            // Utiliser le nouveau chemin installé
                            QString basePath = QDir::homePath() + "/Documents/";
                            QString relativePath = fileInfo.absoluteFilePath().remove(basePath);
                            QString installedPath = QDir::homePath() + "/InstalledFiles/" + relativePath;
                            
                            // Mettre à jour le chemin du fichier dans le pad SANS recharger le fichier
                            newPad->blockSignals(true);
                            newPad->setFilePath(installedPath);
                            newPad->blockSignals(false);
                            qDebug() << "[INFO] Chemin audio mis à jour vers:" << installedPath;
                        } else {
                            qDebug() << "[ERREUR] Échec de la sauvegarde du fichier audio temporaire";
                        }
                    } else if (QFile::exists(filePath)) {
                        // Si le fichier existe, on met simplement à jour le chemin sans recharger
                        newPad->blockSignals(true);
                        newPad->setFilePath(filePath);
                        newPad->blockSignals(false);
                        qDebug() << "[INFO] Chemin audio défini (sans chargement) vers le fichier existant:" << filePath;
                    }
                } else {
                    qDebug() << "[ERREUR] Les données audio n'ont pas été définies correctement";
                }
            } else if (!filePath.isEmpty() && QFile::exists(filePath)) {
                // Essayer de charger depuis le fichier local si disponible ET si pas déjà traité
                if (!audioProcessed) {
                    qDebug() << "[INFO] Définition du chemin audio avec fichier local:" << filePath;
                    newPad->setFilePath(filePath);
                    audioProcessed = true;
                }
            }
            
            // Traitement similaire pour l'image
            if (imageDataReceived) {
                qDebug() << "[INFO] Définition des données image depuis les données reçues";
                newPad->setImageData(imageData);
                
                if (!newPad->getImageData().isEmpty()) {
                    imageProcessed = true;
                    qDebug() << "[INFO] Données image définies sur le nouveau SoundPad:" << imageData.size() << "octets";
                    
                    // Si le fichier n'existe pas localement, le sauvegarder
                    if (!imagePath.isEmpty() && !QFile::exists(imagePath)) {
                        // Créer un chemin temporaire pour sauvegarder le fichier image
                        QFileInfo fileInfo(imagePath);
                        QString tempDir = QDir::tempPath() + "/Ynoise/";
                        QDir().mkpath(tempDir);
                        QString tempFilePath = tempDir + fileInfo.fileName();
                        
                        qDebug() << "[INFO] Sauvegarde temporaire du fichier image reçu vers:" << tempFilePath;
                        
                        if (newPad->saveImageToFile(tempFilePath)) {
                            qDebug() << "[INFO] Fichier image temporaire sauvegardé avec succès";
                            
                            // Installer le fichier en conservant la structure de répertoire
                            installFileLocally(tempFilePath);
                            
                            // Utiliser le nouveau chemin installé
                            QString basePath = QDir::homePath() + "/Documents/";
                            QString relativePath = fileInfo.absoluteFilePath().remove(basePath);
                            QString installedPath = QDir::homePath() + "/InstalledFiles/" + relativePath;
                            
                            // Mettre à jour le chemin du fichier dans le pad SANS recharger le fichier
                            newPad->blockSignals(true);
                            newPad->setImagePath(installedPath);
                            newPad->blockSignals(false);
                            qDebug() << "[INFO] Chemin image mis à jour vers:" << installedPath;
                        } else {
                            qDebug() << "[ERREUR] Échec de la sauvegarde du fichier image temporaire";
                        }
                    } else if (QFile::exists(imagePath)) {
                        // Si le fichier existe, on met simplement à jour le chemin sans recharger
                        newPad->blockSignals(true);
                        newPad->setImagePath(imagePath);
                        newPad->blockSignals(false);
                        qDebug() << "[INFO] Chemin image défini (sans chargement) vers le fichier existant:" << imagePath;
                    }
                } else {
                    qDebug() << "[ERREUR] Les données image n'ont pas été définies correctement";
                }
            } else if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
                // Essayer de charger depuis le fichier local si disponible ET si pas déjà traité
                if (!imageProcessed) {
                    qDebug() << "[INFO] Définition du chemin image avec fichier local:" << imagePath;
                    newPad->setImagePath(imagePath);
                    imageProcessed = true;
                }
            }
            
            // Vérifier si les données ont été chargées correctement
            if (!audioProcessed && !filePath.isEmpty()) {
                qDebug() << "[AVERTISSEMENT] Impossible de charger les données audio pour le pad" << padId;
            }
            
            if (!imageProcessed && !imagePath.isEmpty()) {
                qDebug() << "[AVERTISSEMENT] Impossible de charger les données image pour le pad" << padId;
            }
            
            qDebug() << "Tentative d'ajout d'un nouveau SoundPad:" << padId;
            
            // Ajouter le pad au board et vérifier le résultat
            bool ajoutReussi = targetBoard->addSoundPadFromRemote(newPad);
            
            if (!ajoutReussi) {
                // Si l'ajout a échoué (pad en double), supprimer le pad créé
                delete newPad;
                qDebug() << "Suppression du pad en double, l'ajout a échoué";
                return;
            }
            
            // Si nous sommes l'hôte, retransmettre aux autres clients seulement si l'ajout a réussi
            if (m_isHost && socket) {
                qDebug() << "Retransmission du SoundPad aux autres clients";
                for (auto it = m_users.begin(); it != m_users.end(); ++it) {
                    if (it.key() != socket) {
                        sendMessage(it.key(), "soundpad_added", data);
                    }
                }
            }
        } else {
            qDebug() << "Impossible de trouver le board" << boardId << "pour ajouter le SoundPad";
        }
    }
    else if (type == "board_added") {
        // Récupérer les informations du board
        QString boardId = data["board_id"].toString();
        QString boardName = data["board_name"].toString();
        
        qDebug() << "Message 'board_added' reçu avec ID:" << boardId << "et nom:" << boardName;
        
        // Si nous ne sommes pas l'hôte et que nous avons un board principal
        if (!m_isHost && m_board) {
            // Toujours utiliser l'ID fixe "1" pour le board
            m_board->setObjectName("1");
            if (m_board->getTitle() != boardName) {
                m_board->setTitle(boardName);
            }
            
            qDebug() << "Board principal défini avec l'ID fixe: 1";
        }
    }
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
    QStringList usernames;
    
    // Ajouter tous les noms d'utilisateurs connectés
    for (const ConnectedUser &user : m_users.values()) {
        if (!user.username.isEmpty()) {
            usernames.append(user.username);
        }
    }
    
    return usernames;
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

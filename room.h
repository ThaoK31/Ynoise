#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QNetworkInterface>
#include "board.h"
#include "soundpad.h"

class User;

/**
 * @brief Classe représentant une salle de collaboration
 * 
 * Cette classe gère les connexions client, le serveur, le tableau et les utilisateurs connectés.
 */
class Room : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief Structure définissant un utilisateur connecté
     */
    struct ConnectedUser {
        QString username;       // Nom d'utilisateur
        QTcpSocket *socket;     // Socket de connexion
        
        ConnectedUser(const QString &name = "", QTcpSocket *sock = nullptr)
            : username(name), socket(sock) {}
    };
    
    /**
     * @brief Constructeur
     * @param name Nom de la room
     * @param isHost Indique si l'utilisateur est l'hôte
     * @param parent Objet parent
     */
    explicit Room(const QString &name, bool isHost = true, QObject *parent = nullptr);
    
    /**
     * @brief Destructeur
     */
    ~Room();
    
    /**
     * @brief Obtient le nom de la room
     * @return Nom de la room
     */
    QString name() const { return m_name; }
    
    /**
     * @brief Définit le nom de la room
     * @param name Nouveau nom
     */
    void setName(const QString &name);
    
    /**
     * @brief Démarre le serveur de la room
     * @details Cette méthode initialise et configure le serveur pour accepter les connexions entrantes.
     *          Elle définit aussi le code d'invitation pour permettre aux clients de se connecter.
     * @return true si le serveur a démarré avec succès, false sinon
     */
    bool startServer();
    
    /**
     * @brief Arrête le serveur de la room
     * @details Ferme toutes les connexions client et arrête le serveur
     */
    void stopServer();
    
    /**
     * @brief Établit une connexion client
     * @param address Adresse du serveur
     * @param port Port du serveur
     * @param username Nom d'utilisateur
     * @return true si la connexion a réussi
     */
    bool connectToRoom(const QString &address, int port, const QString &username);
    
    /**
     * @brief Déconnecte l'utilisateur
     */
    void disconnect();
    
    /**
     * @brief Rejoindre la room via un code d'invitation
     * @param inviteCode Code d'invitation
     * @param username Nom d'utilisateur
     * @return true si la connexion a réussi
     */
    bool joinWithCode(const QString &inviteCode, const QString &username);

    /**
     * @brief Obtient le code d'invitation pour cette room
     * @return Code d'invitation
     */
    QString invitationCode() const { return m_invitationCode; }
    
    /**
     * @brief Génère un nouveau code d'invitation pour cette room
     * @details Le code d'invitation est de la forme "adresse_ip:port" et permet aux clients
     *          de se connecter à cette room
     * @return Nouveau code d'invitation
     */
    QString generateInvitationCode();
    
    /**
     * @brief Obtient la liste des utilisateurs connectés
     * @return Liste des noms d'utilisateurs
     */
    QStringList connectedUsers() const;

    /**
     * @brief Obtient le board principal
     * @return Pointeur vers le board principal
     */
    Board* getBoard() const { return m_board; }
    
    /**
     * @brief Indique si l'utilisateur local est l'hôte
     * @return true si l'utilisateur est l'hôte
     */
    bool isHost() const { return m_isHost; }

    /**
     * @brief Définit le nom d'utilisateur de l'hôte
     * @param username Nom d'utilisateur
     */
    void setHostUsername(const QString &username) {
        m_hostUsername = username;
    }

    /**
     * @brief Renvoie le nom d'utilisateur de l'hôte de la room
     * @return Le nom d'utilisateur de l'hôte
     */
    QString hostUsername() const { return m_hostUsername; }

    /**
     * @brief Obtient l'adresse IP locale
     * @return Adresse IP locale
     */
    QString getLocalIpAddress() const;

public slots:
    /**
     * @brief Notifie les autres utilisateurs de l'ajout d'un SoundPad
     * @param board Board contenant le SoundPad
     * @param pad SoundPad ajouté
     */
    void notifySoundPadAdded(Board *board, SoundPad *pad);
    
    /**
     * @brief Notifie les autres utilisateurs de la suppression d'un SoundPad
     * @param board Board contenant le SoundPad
     * @param pad SoundPad supprimé
     */
    void notifySoundPadRemoved(Board *board, SoundPad *pad);

    /**
     * @brief Notifie les autres utilisateurs de la modification d'un SoundPad
     * @param board Board contenant le SoundPad
     * @param pad SoundPad modifié
     */
    void notifySoundPadModified(Board *board, SoundPad *pad);

signals:
    /**
     * @brief Signal émis lorsqu'un utilisateur se connecte
     * @param username Nom de l'utilisateur
     */
    void userConnected(const QString &username);
    
    /**
     * @brief Signal émis lorsqu'un utilisateur se déconnecte
     * @param username Nom de l'utilisateur
     */
    void userDisconnected(const QString &username);
    
    /**
     * @brief Signal émis lorsqu'un SoundPad est ajouté
     * @param board Tableau contenant le SoundPad
     * @param pad SoundPad ajouté
     */
    void soundpadAdded(Board *board, SoundPad *pad);
    
    /**
     * @brief Signal émis lorsqu'un SoundPad est supprimé
     * @param board Tableau contenant le SoundPad
     * @param pad SoundPad supprimé
     */
    void soundpadRemoved(Board *board, SoundPad *pad);

    /**
     * @brief Signal émis lorsqu'un SoundPad est modifié
     * @param board Tableau contenant le SoundPad
     * @param pad SoundPad modifié
     */
    void soundpadModified(Board *board, SoundPad *pad);
    
    /**
     * @brief Signal émis lorsque le serveur démarre
     * @param address Adresse IP du serveur
     * @param port Port d'écoute du serveur
     */
    void serverStartedSignal(const QString &address, int port);
    
    /**
     * @brief Signal émis lorsque le serveur s'arrête
     */
    void serverStopped();
    
    /**
     * @brief Signal émis lorsque le nom de la room change
     * @param name Nouveau nom
     */
    void nameChanged(const QString &name);

private slots:
    /**
     * @brief Gère une nouvelle connexion entrante
     */
    void handleNewConnection();
    
    /**
     * @brief Gère la réception de données
     */
    void handleDataReceived();
    
    /**
     * @brief Gère la déconnexion d'un client
     */
    void handleClientDisconnected();
    
private:
    QString m_name;                       // Nom de la room
    QString m_invitationCode;             // Code d'invitation
    QString m_hostUsername;               // Nom d'utilisateur de l'hôte
    Board *m_board;                       // Board principal
    QMap<QTcpSocket*, ConnectedUser> m_users; // Utilisateurs connectés
    QTcpServer *m_server;               // Serveur TCP
    QTcpSocket *m_clientSocket;         // Socket client
    bool m_isHost;                      // Indique si l'utilisateur est l'hôte
    int m_port;                         // Port d'écoute
    
    /**
     * @brief Envoie un message à tous les clients
     * @param type Type de message
     * @param data Données à envoyer
     * @param excludeSocket Socket à exclure de la diffusion (optionnel)
     */
    void broadcastMessage(const QString &type, const QJsonObject &data, QTcpSocket *excludeSocket = nullptr);
    
    /**
     * @brief Envoie un message à un client spécifique
     * @param socket Socket du client
     * @param type Type de message
     * @param data Données à envoyer
     */
    void sendMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data);
    
    /**
     * @brief Traite un message reçu
     * @param socket Socket qui a envoyé le message
     * @param type Type de message
     * @param data Données du message
     */
    void processMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data);
    
    /**
     * @brief Vérifie si les boards sont correctement chargés et envoie une confirmation à l'hôte
     * @param boardIds Liste des identifiants de boards à vérifier
     * @param socket Socket à utiliser pour envoyer la confirmation
     */
    void confirmBoardsLoaded(const QJsonArray &boardIds, QTcpSocket *socket);
};

#endif // ROOM_H

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

class Room : public QObject
{
    Q_OBJECT
    
public:
    struct ConnectedUser {
        QString username;
        QTcpSocket *socket;    

        ConnectedUser(const QString &name = "", QTcpSocket *sock = nullptr)
            : username(name), socket(sock) {}
    };
    
    //--------------------------------------------------------------------------
    // Construction et destruction
    //--------------------------------------------------------------------------

    explicit Room(const QString &name, bool isHost = true, QObject *parent = nullptr);
    ~Room();

    //--------------------------------------------------------------------------
    // Gestion de la room
    //--------------------------------------------------------------------------

    QString name() const;
    void setName(const QString &name);
    bool isHost() const;
    void setHostUsername(const QString &username);
    QString hostUsername() const;

    //--------------------------------------------------------------------------
    // Gestion du serveur et des connexions
    //--------------------------------------------------------------------------

    bool startServer();
    void stopServer();
    bool connectToRoom(const QString &address, int port, const QString &username);
    void disconnect();
    bool joinWithCode(const QString &inviteCode, const QString &username);
    QString invitationCode() const;
    QString generateInvitationCode();
    QString getLocalIpAddress() const;
    QStringList connectedUsers() const;

    //--------------------------------------------------------------------------
    // Gestion des boards et SoundPads
    //--------------------------------------------------------------------------

    Board* getBoard() const;
    void installFileLocally(const QString &filePath);
    void saveMediaToPad(const QString &mediaPath);
    QList<QByteArray> splitDataIntoChunks(const QByteArray &data, int chunkSize);
    QByteArray reassembleData(const QJsonArray &chunks);
    
public slots:
    void notifySoundPadAdded(Board *board, SoundPad *pad);
    void notifySoundPadRemoved(Board *board, SoundPad *pad);

signals:
    void userConnected(const QString &username);
    void userDisconnected(const QString &username);
    void soundpadAdded(Board *board, SoundPad *pad);
    void soundpadRemoved(Board *board, SoundPad *pad);
    void serverStartedSignal(const QString &address, int port);
    void serverStopped();
    void nameChanged(const QString &name);
    void fileInstalled(const QString &path);

private slots:
    void handleNewConnection();
    void handleDataReceived();
    void handleClientDisconnected();

private:
    //--------------------------------------------------------------------------
    // Attributs privés
    //--------------------------------------------------------------------------

    QString m_name;
    QString m_invitationCode;
    QString m_hostUsername;
    bool m_isHost;
    QTcpServer *m_server;
    QTcpSocket *m_clientSocket;
    QMap<QTcpSocket*, ConnectedUser> m_users;
    Board *m_board;
    int m_port;                      // Port d'écoute du serveur

    //--------------------------------------------------------------------------
    // Méthodes privées - Communication
    //--------------------------------------------------------------------------

    void broadcastMessage(const QString &type, const QJsonObject &data);
    void sendMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data);
    void processMessage(QTcpSocket *socket, const QString &type, const QJsonObject &data);

    //--------------------------------------------------------------------------
    // Méthodes privées - Gestion de la logique du message
    //--------------------------------------------------------------------------

    void handleSoundpadRemoved(const QJsonObject &data);
    void handleUserJoin(QTcpSocket *socket, const QJsonObject &data);
    void sendUserList(QTcpSocket *socket);
    void sendBoardDetails(QTcpSocket *socket);
    void sendSoundpads(QTcpSocket *socket);
    void handleSoundpadAdded(QTcpSocket *socket, const QJsonObject &data);
    SoundPad* createSoundPadFromData(const QJsonObject &data, Board *targetBoard);
    void broadcastNewPadToOthers(QTcpSocket *socket, const QJsonObject &data);
    Board* findBoard(const QString &boardId);
    void handleBoardAdded(const QJsonObject &data);
    void confirmBoardsLoaded(const QJsonArray &boardIds, QTcpSocket *socket);
};

#endif // ROOM_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVector>

#include "user.h"
#include "room.h"
#include "board.h"
#include "soundpad.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief Fenêtre principale de l'application SoundPad
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Widget parent
     */
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief Crée une nouvelle Room
     */
    void createRoom();
    
    /**
     * @brief Rejoint une Room existante
     */
    void joinRoom();
    
    /**
     * @brief Génère et affiche un code d'invitation
     */
    void showInviteCode();
    
    /**
     * @brief Ajoute un nouveau tableau de SoundPads
     */
    void addNewBoard();
    
    /**
     * @brief Supprime le tableau de SoundPads actif
     */
    void removeCurrentBoard();
    
    /**
     * @brief Met à jour la liste des utilisateurs connectés
     */
    void updateUsersList();
    
    /**
     * @brief Gère la connexion d'un utilisateur
     * @param username Nom de l'utilisateur
     */
    void handleUserConnected(const QString &username);
    
    /**
     * @brief Gère la déconnexion d'un utilisateur
     * @param username Nom de l'utilisateur
     */
    void handleUserDisconnected(const QString &username);
    
    /**
     * @brief Configure les options de l'utilisateur
     */
    void configureUser();

private:
    Ui::MainWindow *ui;
    
    User *m_user;                       // Utilisateur courant
    Room *m_currentRoom;                // Room active
    QVector<Room*> m_rooms;             // Liste des rooms
    QTabWidget *m_tabWidget;            // Widget d'onglets pour les tableaux
    QDockWidget *m_usersDock;           // Dock pour la liste des utilisateurs
    QListWidget *m_usersListWidget;     // Liste des utilisateurs connectés
    QPushButton *m_inviteButton;        // Bouton d'invitation
    
    /**
     * @brief Configure l'interface utilisateur
     */
    void setupUi();
    
    /**
     * @brief Configure les menus et barres d'outils
     */
    void setupMenus();
    
    /**
     * @brief Initialise l'utilisateur
     */
    void initUser();
    
    /**
     * @brief Connecte les signaux d'un Board avec la Room pour synchroniser les SoundPads
     * @param board Board à connecter
     * @param room Room à laquelle connecter le board
     */
    void connectBoardSignals(Board *board, Room *room);

};
#endif // MAINWINDOW_H

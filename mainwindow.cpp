#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "roomdialog.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_user(nullptr)
    , m_currentRoom(nullptr)
    , m_tabWidget(nullptr)
    , m_usersDock(nullptr)
    , m_usersListWidget(nullptr)
    , m_inviteButton(nullptr)
{
    ui->setupUi(this);
    
    // Configuration de l'interface
    setupUi();
    
    // Configuration des menus
    setupMenus();
    
    // Initialisation de l'utilisateur
    initUser();
    
    // Mise à jour du titre
    setWindowTitle(tr("SoundPad App - %1").arg(m_user->getName()));
    
    // Connexion du signal de changement de nom
    connect(m_user, &User::nameChanged, this, [this](const QString &newName) {
        setWindowTitle(tr("SoundPad App - %1").arg(newName));
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_user;
    
    // Suppression des rooms
    qDeleteAll(m_rooms);
}

void MainWindow::createRoom()
{
    // Affichage de la boîte de dialogue de création
    RoomDialog dialog(RoomDialog::CreateMode, QString(), this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString roomName = dialog.getRoomName();
        
        if (!roomName.isEmpty()) {
            // Création de la room
            Room *room = new Room(roomName, true, this);
            m_rooms.append(room);
            
            // Ajout à l'utilisateur
            m_user->addRoom(room);
            
            // Définir comme room courante
            m_currentRoom = room;
            
            // Mise à jour de l'interface
            updateUsersList();
            
            // Utiliser le board par défaut déjà créé dans le constructeur de Room
            Board *board = room->getBoard();
            
            // Connecter les signaux du board pour synchroniser les SoundPads
            connectBoardSignals(board, room);
            
            // Ajout au widget d'onglets
            m_tabWidget->clear();
            m_tabWidget->addTab(board, board->getTitle());
            
            // Activer le bouton d'invitation
            m_inviteButton->setEnabled(true);
            
            // Connecter les signaux de la room
            connect(room, &Room::userConnected, this, &MainWindow::handleUserConnected);
            connect(room, &Room::userDisconnected, this, &MainWindow::handleUserDisconnected);
            connect(room, &Room::soundpadAdded, this, [this](Board *board, SoundPad *pad) {
                // Le pad a déjà été ajouté au board, mais on peut mettre à jour l'interface si besoin
                qDebug() << "SoundPad ajouté au board:" << pad->objectName();
            });
            connect(room, &Room::soundpadRemoved, this, [this](Board *board, SoundPad *pad) {
                qDebug() << "SoundPad supprimé du board:" << pad->objectName();
            });
            
            // Démarrer le serveur
            if (room->startServer()) {
                // Définir le nom d'utilisateur de l'hôte
                room->setHostUsername(m_user->getName());
                
                QMessageBox::information(this, tr("Room créée"), 
                    tr("La room '%1' a été créée avec succès.\nVotre code d'invitation est : %2")
                    .arg(roomName)
                    .arg(room->invitationCode()));
            } else {
                QMessageBox::critical(this, tr("Erreur"), 
                    tr("Impossible de démarrer le serveur pour la room."));
                
                // Supprimer la room en cas d'échec
                m_user->removeRoom(room);
                m_rooms.removeOne(room);
                delete room;
            }
        }
    }
}

void MainWindow::joinRoom()
{
    // Affichage de la boîte de dialogue de connexion
    RoomDialog dialog(RoomDialog::JoinMode, QString(), this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString inviteCode = dialog.getInviteCode();
        
        if (!inviteCode.isEmpty()) {
            // Création de la room (pour l'instant, on simule une connexion locale)
            Room *room = new Room(tr("Room rejointe"), false, this);
            
            // Tentative de connexion
            if (room->joinWithCode(inviteCode, m_user->getName())) {
                // Ajout à la liste
                m_rooms.append(room);
                
                // Ajout à l'utilisateur
                m_user->addRoom(room);
                
                // Définir comme room courante
                m_currentRoom = room;
                
                // Mise à jour de l'interface
                updateUsersList();
                
                // Utiliser le board par défaut déjà créé dans le constructeur de Room
                Board *board = room->getBoard();
                
                // Connecter les signaux du board pour synchroniser les SoundPads
                connectBoardSignals(board, room);
                
                // Ajout au widget d'onglets
                m_tabWidget->clear();
                m_tabWidget->addTab(board, board->getTitle());
                
                // Activer le bouton d'invitation
                m_inviteButton->setEnabled(true);
                
                // Connecter les signaux de la room
                connect(room, &Room::userConnected, this, &MainWindow::handleUserConnected);
                connect(room, &Room::userDisconnected, this, &MainWindow::handleUserDisconnected);
                connect(room, &Room::soundpadAdded, this, [this](Board *board, SoundPad *pad) {
                    // Le pad a déjà été ajouté au board, mais on peut mettre à jour l'interface si besoin
                    qDebug() << "SoundPad ajouté au board:" << pad->objectName();
                });
                connect(room, &Room::soundpadRemoved, this, [this](Board *board, SoundPad *pad) {
                    qDebug() << "SoundPad supprimé du board:" << pad->objectName();
                });
            } else {
                QMessageBox::critical(this, tr("Erreur"), 
                    tr("Impossible de rejoindre la room avec ce code d'invitation."));
                
                // Supprimer la room en cas d'échec
                delete room;
            }
        }
    }
}

void MainWindow::showInviteCode()
{
    if (!m_currentRoom) {
        QMessageBox::warning(this, tr("Avertissement"), 
            tr("Aucune room active."));
        return;
    }
    
    // Affichage du code d'invitation
    RoomDialog dialog(RoomDialog::InviteMode, m_currentRoom->invitationCode(), this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Dans ce cas, nous n'avons pas besoin de récupérer le code puisque
        // l'utilisateur a la possibilité de le copier directement depuis la boîte de dialogue
        QMessageBox::information(this, tr("Code d'invitation"), 
            tr("Le code d'invitation a été montré avec succès."));
    }
}

void MainWindow::addNewBoard()
{
    QMessageBox::information(this, tr("Information"), 
        tr("L'application ne supporte actuellement qu'un seul tableau par room."));
}

void MainWindow::removeCurrentBoard()
{
    QMessageBox::information(this, tr("Information"), 
        tr("Impossible de supprimer le tableau principal."));
}

void MainWindow::updateUsersList()
{
    // Effacer la liste actuelle
    m_usersListWidget->clear();
    
    if (m_currentRoom) {
        // Récupérer la liste des utilisateurs
        QStringList users = m_currentRoom->connectedUsers();
        
        // Ajouter le nom de l'hôte s'il est défini et n'est pas déjà dans la liste
        QString hostName = m_currentRoom->hostUsername();
        if (!hostName.isEmpty() && !users.contains(hostName)) {
            users.prepend(hostName + tr(" (hôte)"));
            qDebug() << "Ajout de l'hôte à la liste d'utilisateurs:" << hostName;
        }
        
        // Ajouter l'utilisateur local s'il n'est pas déjà dans la liste
        if (!users.contains(m_user->getName())) {
            users.append(m_user->getName() + tr(" (vous)"));
        }
        
        // Mise à jour de l'interface
        for (const QString &username : users) {
            m_usersListWidget->addItem(username);
        }
    }
}

void MainWindow::handleUserConnected(const QString &username)
{
    // Ajout de l'utilisateur à la liste
    m_usersListWidget->addItem(username);
    
    // Notification
    statusBar()->showMessage(tr("Utilisateur connecté: %1").arg(username), 3000);
}

void MainWindow::handleUserDisconnected(const QString &username)
{
    // Recherche et suppression de l'utilisateur dans la liste
    QList<QListWidgetItem*> items = m_usersListWidget->findItems(username, Qt::MatchExactly);
    for (QListWidgetItem *item : items) {
        delete m_usersListWidget->takeItem(m_usersListWidget->row(item));
    }
    
    // Notification
    statusBar()->showMessage(tr("Utilisateur déconnecté: %1").arg(username), 3000);
}

void MainWindow::configureUser()
{
    // Au lieu de demander un nom, afficher un message d'information
    QMessageBox::information(this, tr("Configuration utilisateur"),
        tr("Votre nom d'utilisateur est attribué aléatoirement à partir d'une liste de personnages d'anime.\n\n"
           "Votre nom actuel est : %1").arg(m_user->getName()));
}

void MainWindow::setupUi()
{
    // Configuration de la taille de la fenêtre
    resize(1024, 768);
    
    // Création du widget central avec layout vertical
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);
    
    // Création du widget d'onglets pour les tableaux
    m_tabWidget = new QTabWidget(centralWidget);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    mainLayout->addWidget(m_tabWidget);
    
    // Connexion du signal de fermeture d'onglet
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (m_currentRoom && index >= 0 && index < m_tabWidget->count()) {
            Board *board = qobject_cast<Board*>(m_tabWidget->widget(index));
            if (board) {
                // Dans cette version de l'application, on ne peut pas supprimer le tableau principal
                QMessageBox::information(this, tr("Information"), 
                    tr("Impossible de supprimer le tableau principal."));
            }
        }
    });
    
    // Création de la barre d'outils pour les boards
    QToolBar *boardToolBar = new QToolBar(tr("Boards"), this);
    boardToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    
    QAction *addBoardAction = boardToolBar->addAction(tr("+ Nouveau tableau"));
    connect(addBoardAction, &QAction::triggered, this, &MainWindow::addNewBoard);
    
    QAction *removeBoardAction = boardToolBar->addAction(tr("- Supprimer tableau"));
    connect(removeBoardAction, &QAction::triggered, this, &MainWindow::removeCurrentBoard);
    
    mainLayout->addWidget(boardToolBar);
    
    // Création du dock pour la liste des utilisateurs
    m_usersDock = new QDockWidget(tr("Utilisateurs connectés"), this);
    m_usersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_usersListWidget = new QListWidget(m_usersDock);
    m_usersDock->setWidget(m_usersListWidget);
    
    addDockWidget(Qt::RightDockWidgetArea, m_usersDock);
    
    // Bouton d'invitation
    m_inviteButton = new QPushButton(tr("Inviter"), m_usersDock);
    m_inviteButton->setEnabled(false);
    connect(m_inviteButton, &QPushButton::clicked, this, &MainWindow::showInviteCode);
    
    // Ajout du bouton au dock
    QWidget *dockContents = new QWidget(m_usersDock);
    QVBoxLayout *dockLayout = new QVBoxLayout(dockContents);
    dockLayout->addWidget(m_usersListWidget);
    dockLayout->addWidget(m_inviteButton);
    m_usersDock->setWidget(dockContents);
    
    // Menu contextuel des onglets
    m_tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabWidget, &QTabWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Menu"), this);
        
        QAction *renameAction = new QAction(tr("Renommer ce tableau"), this);
        
        connect(renameAction, &QAction::triggered, this, [this]() {
            int currentIndex = m_tabWidget->currentIndex();
            if (currentIndex >= 0) {
                Board *board = qobject_cast<Board*>(m_tabWidget->widget(currentIndex));
                if (board) {
                    // Dialogue pour renommer le tableau
                    bool ok;
                    QString newTitle = QInputDialog::getText(this, tr("Renommer"), 
                        tr("Nouveau nom du tableau :"), QLineEdit::Normal, 
                        board->getTitle(), &ok);
                    
                    if (ok && !newTitle.isEmpty()) {
                        board->setTitle(newTitle);
                        m_tabWidget->setTabText(currentIndex, newTitle);
                    }
                }
            }
        });
        
        // Ajout des actions au menu contextuel
        contextMenu.addAction(renameAction);
        
        // Affichage du menu contextuel
        contextMenu.exec(m_tabWidget->mapToGlobal(pos));
    });
}

void MainWindow::setupMenus()
{
    // Menu Fichier
    QMenu *fileMenu = menuBar()->addMenu(tr("&Fichier"));
    
    QAction *newRoomAction = fileMenu->addAction(tr("&Créer une Room"));
    connect(newRoomAction, &QAction::triggered, this, &MainWindow::createRoom);
    
    QAction *joinRoomAction = fileMenu->addAction(tr("&Rejoindre une Room"));
    connect(joinRoomAction, &QAction::triggered, this, &MainWindow::joinRoom);
    
    fileMenu->addSeparator();
    
    QAction *inviteAction = fileMenu->addAction(tr("&Inviter"));
    connect(inviteAction, &QAction::triggered, this, &MainWindow::showInviteCode);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction(tr("&Quitter"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Menu Édition
    QMenu *editMenu = menuBar()->addMenu(tr("&Édition"));
    
    // Désactiver l'ajout et la suppression de tableaux dans cette version
    QAction *addBoardAction = editMenu->addAction(tr("&Ajouter un tableau"));
    addBoardAction->setEnabled(false);
    connect(addBoardAction, &QAction::triggered, this, &MainWindow::addNewBoard);
    
    QAction *removeBoardAction = editMenu->addAction(tr("&Supprimer le tableau actif"));
    removeBoardAction->setEnabled(false);
    connect(removeBoardAction, &QAction::triggered, this, &MainWindow::removeCurrentBoard);
    
    // Ajouter une action pour renommer le tableau
    QAction *renameBoardAction = editMenu->addAction(tr("&Renommer le tableau actif"));
    connect(renameBoardAction, &QAction::triggered, this, [this]() {
        int currentIndex = m_tabWidget->currentIndex();
        if (currentIndex >= 0) {
            Board *board = qobject_cast<Board*>(m_tabWidget->widget(currentIndex));
            if (board) {
                // Dialogue pour renommer le tableau
                bool ok;
                QString newTitle = QInputDialog::getText(this, tr("Renommer"), 
                    tr("Nouveau nom du tableau :"), QLineEdit::Normal, 
                    board->getTitle(), &ok);
                
                if (ok && !newTitle.isEmpty()) {
                    board->setTitle(newTitle);
                    m_tabWidget->setTabText(currentIndex, newTitle);
                }
            }
        }
    });
    
    // Menu Paramètres
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Paramètres"));
    
    QAction *userSettingsAction = settingsMenu->addAction(tr("&Configuration utilisateur"));
    connect(userSettingsAction, &QAction::triggered, this, &MainWindow::configureUser);
}

void MainWindow::initUser()
{
    // Création de l'utilisateur
    // Le constructeur attribue maintenant automatiquement un nom aléatoire
    m_user = new User();
    
    // Aucune demande de nom n'est nécessaire car le constructeur User gère cela
    // et attribue un nom aléatoire de personnage d'anime
    
    // Afficher le nom attribué dans la barre de statut
    statusBar()->showMessage(tr("Bienvenue, %1 !").arg(m_user->getName()), 5000);
}

void MainWindow::connectBoardSignals(Board *board, Room *room)
{
    if (!board || !room) {
        qDebug() << "ERREUR: board ou room invalide dans connectBoardSignals";
        return;
    }
    
    qDebug() << "Connexion des signaux du board" << board->objectName() << "à la room";
    
    // IMPORTANT: Utiliser toujours l'ID fixe "1" pour le board principal
    // pour assurer la compatibilité avec les messages réseau
    if (board->objectName().isEmpty()) {
        board->setObjectName("1");
        qDebug() << "ID fixe attribué au board: 1";
    } else if (board->objectName() != "1") {
        qDebug() << "AVERTISSEMENT: Remplacement de l'ID du board" << board->objectName() << "par l'ID fixe '1'";
        board->setObjectName("1");
    }
    
    // Connecter les signaux du board pour la synchronisation des SoundPads
    connect(board, &Board::soundPadAdded, this, [this, room, board](SoundPad *pad) {
        qDebug() << "Signal soundPadAdded émis par le board" << board->objectName() << ", notification envoyée à la room";
        
        // Générer un ID pour ce pad s'il n'en a pas déjà un
        if (pad->objectName().isEmpty()) {
            QString padId = QString("pad_%1").arg(QDateTime::currentMSecsSinceEpoch());
            pad->setObjectName(padId);
            qDebug() << "ID généré pour le pad:" << padId;
        }
        
        // Notifier la room de l'ajout du SoundPad pour synchronisation
        room->notifySoundPadAdded(board, pad);
    });
    
    connect(board, &Board::soundPadRemoved, this, [this, room, board](SoundPad *pad) {
        qDebug() << "Signal soundPadRemoved émis par le board" << board->objectName() << ", notification envoyée à la room";
        
        if (!pad) {
            qDebug() << "ERREUR: pad invalide dans le signal soundPadRemoved";
            return;
        }
        
        // Vérifier que le pad a un ID avant la notification
        if (pad->objectName().isEmpty()) {
            qDebug() << "ERREUR: pad sans ID dans le signal soundPadRemoved";
            QString padId = QString("pad_%1").arg(QDateTime::currentMSecsSinceEpoch());
            pad->setObjectName(padId);
            qDebug() << "ID généré pour le pad avant suppression:" << padId;
        }
        
        // Copier les informations essentielles du pad avant sa suppression
        QString padId = pad->objectName();
        
        // Notifier la room de la suppression du SoundPad pour synchronisation
        room->notifySoundPadRemoved(board, pad);
    });
    
    // Connexion du signal de modification du SoundPad
    connect(board, &Board::soundPadModified, this, [this, room, board](SoundPad *pad) {
        qDebug() << "Signal soundPadModified émis par le board" << board->objectName() << ", notification envoyée à la room";
        
        if (!pad) {
            qDebug() << "ERREUR: pad invalide dans le signal soundPadModified";
            return;
        }
        
        // Vérifier que le pad a un ID avant la notification
        if (pad->objectName().isEmpty()) {
            QString padId = QString("pad_%1").arg(QDateTime::currentMSecsSinceEpoch());
            pad->setObjectName(padId);
            qDebug() << "ID généré pour le pad avant modification:" << padId;
        }
        
        // Notifier la room de la modification du SoundPad pour synchronisation
        room->notifySoundPadModified(board, pad);
    });
    
    // Connecter les signaux de la room pour mettre à jour l'interface utilisateur
    connect(room, &Room::soundpadAdded, this, [this](Board *board, SoundPad *pad) {
        // Mettre à jour l'interface utilisateur si nécessaire
        qDebug() << "Signal soundpadAdded reçu de la room pour le board" << board->objectName();
        
        // Actualiser l'affichage du board si nécessaire
        board->update();
    });
    
    connect(room, &Room::soundpadRemoved, this, [this](Board *board, SoundPad *pad) {
        // Mettre à jour l'interface utilisateur si nécessaire
        qDebug() << "Signal soundpadRemoved reçu de la room pour le board" << board->objectName();
        
        // Actualiser l'affichage du board si nécessaire
        board->update();
    });
    
    connect(room, &Room::soundpadModified, this, [this](Board *board, SoundPad *pad) {
        // Mettre à jour l'interface utilisateur si nécessaire
        qDebug() << "Signal soundpadModified reçu de la room pour le board" << board->objectName();
        
        // Actualiser l'affichage du board si nécessaire
        board->update();
    });
}

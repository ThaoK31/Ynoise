# Ynoise

Application de partage de sons en réseau développée avec Qt.

## Description

Ynoise est une application desktop qui permet de créer, organiser et partager des plateaux de sons (soundpads) en réseau. Elle offre la possibilité de jouer des sons localement ou de les diffuser à tous les participants d'une session collaborative.

## Fonctionnalités principales

- **Gestion de soundpads**: Créer, modifier et organiser des pads sonores
- **Plateaux multiples**: Organiser les soundpads en différents tableaux (boards)
- **Collaboration en temps réel**: Créer ou rejoindre des sessions collaboratives
- **Partage de sons**: Diffuser des sons à tous les participants d'une session
- **Raccourcis clavier**: Associer des raccourcis pour déclencher rapidement les sons
- **Personnalisation**: Modifier les métadonnées et images des soundpads

## Architecture

### Diagramme de classes

```
+------------------+         +------------------+         +---------------------+
|    MainWindow    |         |       User       |         |        Room         |
+------------------+         +------------------+         +---------------------+
| - m_user: User*  |<------->| - m_name: QString|<------->| - m_name: QString   |
| - m_rooms: QList |         | - m_settings: QJS|         | - m_isHost: bool    |
| - m_board: Board*|         +------------------+         | - m_server: QTcpServ|
| - m_tabWidget    |         | + getName()      |         | - m_users: QMap     |
+------------------+         | + setName()      |         | - m_clientSocket    |
| + createRoom()   |         | + getSetting()   |         | - m_board: Board*   |
| + joinRoom()     |         | + setSetting()   |         +---------------------+
| + showInviteCode()|        | + saveSettings() |         | + startServer()     |
| + addNewBoard()  |         | + loadSettings() |         | + connectToRoom()   |
| + configureUser()|         +------------------+         | + joinWithCode()    |
+------------------+                                      | + generateInvitation()|
        |                                                 | + notifySoundPadPlay()|
        |                                                 | + broadcastMessage() |
        v                                                 +---------------------+
+------------------+         +------------------+
|      Board       |<------->|    SoundPad     |
+------------------+         +------------------+
| - m_title: QString|        | - m_title: QString|
| - m_soundPads    |         | - m_filePath     |
| - m_gridLayout   |         | - m_imagePath    |
| - m_contentWidget|         | - m_image: QPixmap|
+------------------+         | - m_shortcut     |
| + getTitle()     |         | - m_mediaPlayer  |
| + setTitle()     |         +------------------+
| + getSoundPads() |         | + play()         |
| + addSoundPad()  |         | + playSync()     |
| + removeSoundPad()|        | + getTitle()     |
| + reorganizeGrid()|        | + setTitle()     |
+------------------+         | + importSound()  |
                             | + editMetadata() |
                             +------------------+

+------------------+         +------------------+         +------------------+
|   RoomDialog     |         |   ConnectedUser  |         |   QMediaPlayer   |
+------------------+         +------------------+         +------------------+
| - m_mode: Mode   |         | - username       |         |                  |
| - m_inviteCode   |         | - socket         |         |                  |
+------------------+         +------------------+         +------------------+
| + getRoomName()  |                                      |                  |
| + getInviteCode()|                                      |                  |
| + generateCode() |                                      |                  |
+------------------+                                      +------------------+
```

### Diagrammes de communication réseau

L'application utilise une architecture en étoile (star topology) où un utilisateur joue le rôle d'hôte central auquel tous les autres clients se connectent. Toutes les communications entre les clients transitent par l'hôte.

#### Connexion client-serveur

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        | Démarre le serveur TCP                       |
        | sur port aléatoire                           |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |                  Code d'invitation           |
        |<-------------------------------------------->|
        |                                              |
        |                                              | Décode l'invitation
        |                                              | (IP:PORT)
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
        |                   Connexion TCP              |
        |<---------------------------------------------|
        |                                              |
        | Accepte la connexion                         |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"join_request",              |
        |          "data":{"username":"..."}}          |
        |<---------------------------------------------|
        |                                              |
        | Enregistre l'utilisateur                     |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"user_joined",               |
        |          "data":{"username":"host"}}         |
        |--------------------------------------------->|
        |                                              |
        |         {"type":"user_joined",               |
        |          "data":{"username":"..."}}          |
        |--------------------------------------------->|
        |                                              |
        |         {"type":"board_init",                |
        |          "data":{"boards":[...]}}            |
        |--------------------------------------------->|
        |                                              |
        |         {"type":"soundpads_init",            |
        |          "data":{"board_id":"...",           |
        |                  "pads":[...]}}              |
        |--------------------------------------------->|
        |                                              |
```

#### Lecture de SoundPad

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        | Lecture d'un SoundPad                        |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"soundpad_played",           |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"..."}}            |
        |--------------------------------------------->|
        |                                              |
        |                                              | Recherche du SoundPad
        |                                              | par ID
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
        |                                              | Lecture du son
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
```

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        |                                              | Lecture d'un SoundPad
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
        |         {"type":"soundpad_played",           |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"..."}}            |
        |<---------------------------------------------|
        |                                              |
        | Recherche du SoundPad                        |
        | par ID                                       |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        | Lecture du son                               |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"soundpad_played",           |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"..."}}            |
        |--------------------------------------------->|
        |                                              |
        |                                              | Lecture du son
        |                                              | (déjà joué localement donc pas reréalisé)
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
```

#### Ajout d'un SoundPad

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        | Création nouveau                             |
        | SoundPad                                     |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"soundpad_added",            |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"...",             |
        |                  "title":"...",              |
        |                  "filepath":"...",           |
        |                  ...}}                       |
        |--------------------------------------------->|
        |                                              |
        |                                              | Création nouveau
        |                                              | SoundPad avec les
        |                                              | données reçues
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
```

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        |                                              | Création nouveau 
        |                                              | SoundPad
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
        |         {"type":"soundpad_added",            |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"...",             |
        |                  "title":"...",              |
        |                  "filepath":"...",           |
        |                  ...}}                       |
        |<---------------------------------------------|
        |                                              |
        | Création nouveau                             |
        | SoundPad avec les                            |
        | données reçues                               |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"soundpad_added",            |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"...",             |
        |                  "title":"...",              |
        |                  "filepath":"...",           |
        |                  ...}}                       |
        |--------------------------------------------->|
        |                                              |
```

#### Suppression d'un SoundPad

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        | Suppression d'un                             |
        | SoundPad                                     |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"soundpad_removed",          |
        |          "data":{"board_id":"...",           |
        |                  "pad_id":"..."}}            |
        |--------------------------------------------->|
        |                                              |
        |                                              | Recherche et suppression
        |                                              | du SoundPad par ID
        |                                              |-------.
        |                                              |       |
        |                                              |<------'
        |                                              |
```

#### Déconnexion d'un client

```
+-----------------+                           +-----------------+
|      HÔTE       |                           |     CLIENT      |
+-----------------+                           +-----------------+
        |                                              |
        |                                              | Fermeture de la
        |                                              | connexion TCP
        |                   Déconnexion                |
        |<---------------------------------------------|
        |                                              |
        | Détecte déconnexion                          |
        |-------.                                      |
        |       |                                      |
        |<------'                                      |
        |                                              |
        |         {"type":"user_left",                 |
        |          "data":{"username":"..."}}          |
        |--------------------------------------------->|
        |                                              |
        |                                              |
```

### Classes principales

- **User**: Gère les informations utilisateur et les préférences
- **Room**: Gère la communication réseau entre les participants
- **Board**: Représente un tableau contenant des soundpads
- **SoundPad**: Représente un élément déclencheur de son
- **MainWindow**: Interface principale de l'application

## Technologies utilisées

- **Qt 6**: Framework d'interface utilisateur
- **C++17**: Langage principal
- **QTcpServer/QTcpSocket**: Communication réseau
- **QMediaPlayer**: Lecture des fichiers audio
- **QJsonDocument**: Sérialisation/désérialisation des données

## Installation

### Prérequis

- Qt 6.4.0 ou supérieur
- Compilateur C++ compatible C++17
- CMake 3.16 ou supérieur

### Compilation

```bash
mkdir build
cd build
cmake ..
make
```

## Utilisation

### Configuration utilisateur

1. Au premier lancement, vous avez un pseudo généré.
2. Les paramètres sont automatiquement sauvegardés

### Création d'un tableau de sons

1. Utilisez l'interface principale pour créer un nouveau tableau
2. Ajoutez des sons en cliquant sur le bouton "+"
3. Importez des fichiers audio (formats supportés: MP3, WAV, OGG)
4. Personnalisez chaque soundpad (titre, image, raccourci)

### Collaboration en réseau

#### Créer une session

1. Menu "Session" > "Créer une session"
2. Donnez un nom à votre session
3. Partagez le code d'invitation généré avec d'autres utilisateurs

#### Rejoindre une session

1. Menu "Session" > "Rejoindre une session"
2. Entrez le code d'invitation fourni par l'hôte
3. Vous rejoindrez automatiquement la session

#### Partage de sons

- Tous les sons déclenchés sont automatiquement diffusés à tous les participants
- Les tableaux sont synchronisés en temps réel
- Les modifications sont propagées à tous les participants

## Protocole réseau

La communication entre clients utilise un protocole JSON simple:

```json
{
  "type": "message_type",
  "data": {
    // Données spécifiques au type de message
  }
}
```

Types de messages principaux:
- `user_joined`: Nouvel utilisateur rejoint la session
- `user_left`: Utilisateur quitte la session
- `soundpad_added`: Ajout d'un nouveau soundpad
- `soundpad_removed`: Suppression d'un soundpad
- `soundpad_modified`: Modification d'un soundpad
- `soundpad_played`: Déclenchement d'un soundpad

## Structure du projet

```
Ynoise/
├── main.cpp                  # Point d'entrée
├── mainwindow.h/cpp          # Fenêtre principale
├── user.h/cpp                # Gestion utilisateur
├── room.h/cpp                # Gestion sessions réseau
├── board.h/cpp               # Gestion tableaux de sons
├── soundpad.h/cpp            # Gestion pads sonores
├── roomdialog.h/cpp          # Création/rejoindre session
├── invitedialog.h/cpp        # Génération code invitation
└── ...
└── resources/                # Ressources (icônes, etc.)
```

## Evolution prévues
superposer des sons
stopper un son en cours lorsque on le rejoue 
envoyer les fichiers entre les clients et serveur

## Problèmes connus
croix de supression de soundpad persistante dans l'UI 
les membres ne s'affichent pas chez le client 

## Évolutions prévues
### Superposition de sons (4 à 8 heures de développement)
Permettre la lecture simultanée de plusieurs sons, en optimisant le mixage pour éviter les saturations.

### Arrêt automatique (2 à 4 heures de développement)
Arrêter automatiquement un son en cours dès qu'il est relancé, afin d'éviter les chevauchements indésirables.

### Transfert de fichiers (8 à 16 heures de développement)
Intégrer un nouveau type de message (par exemple, « file_transfer ») dans le protocole JSON, permettant la lecture, l'envoi (éventuellement découpé en segments pour les fichiers volumineux), ainsi que la réception et le réassemblage des segments pour sauvegarder le fichier localement.

## Licence

Ce projet est distribué sous licence MIT.

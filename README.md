# Documentation du Projet Ynoise

## Aperçu du Projet

Ynoise est une application de soundboard collaborative permettant à plusieurs utilisateurs de partager et jouer des sons en temps réel. L'application est développée en C++ avec le framework Qt, offrant une interface graphique moderne et des fonctionnalités réseau pour la collaboration.

## Architecture Générale

L'application est structurée en plusieurs composants principaux :

- **SoundPad** : Représente un pad sonore individuel qui peut jouer un son et afficher une image
- **Board** : Tableau contenant plusieurs SoundPads, organisés dans une grille
- **Room** : Salle virtuelle où les utilisateurs peuvent collaborer, avec un système client-serveur
- **User** : Représente un utilisateur connecté à l'application
- **MainWindow** : Interface principale de l'application

## Schémas et Diagrammes

### Diagramme de Classes UML

```
+------------------+      +------------------+      +------------------+
|    MainWindow    |      |       Room       |      |       User       |
+------------------+      +------------------+      +------------------+
| - m_user         |<---->| - m_name         |<---->| - m_username     |
| - m_currentRoom  |      | - m_users        |      | - m_preferences  |
| - m_rooms        |      | - m_server       |      +------------------+
| - m_tabWidget    |      | - m_board        |
+------------------+      +------------------+
        |                        |
        v                        v
+------------------+      +------------------+
|      Board       |      |    SoundPad      |
+------------------+      +------------------+
| - m_title        |<---->| - m_title        |
| - m_soundPads    |      | - m_filePath     |
| - m_gridLayout   |      | - m_imagePath    |
+------------------+      | - m_mediaPlayer  |
                          +------------------+
```

### Diagramme de Séquence pour la Communication Réseau

```
Client                         Serveur (Hôte)                     Autres Clients
  |                                |                                    |
  |     Connexion (join)           |                                    |
  |---------------------------->   |                                    |
  |                                |                                    |
  |     Acceptation                |                                    |
  |   <----------------------------|                                    |
  |                                |                                    |
  |                                |    Notification (user_connected)   |
  |                                |----------------------------------->|
  |                                |                                    |
  |   Création SoundPad            |                                    |
  |---------------------------->   |                                    |
  |                                |                                    |
  |                                |  Diffusion (soundpad_added)        |
  |                                |----------------------------------->|
  |                                |                                    |
```

### Schéma de l'Architecture Client-Serveur

```
+------------------------+      +------------------------+
|       Client 1         |      |       Client 2         |
|                        |      |                        |
| +------------------+   |      | +------------------+   |
| |    MainWindow    |   |      | |    MainWindow    |   |
| +------------------+   |      | +------------------+   |
| |      Board       |   |      | |      Board       |   |
| +------------------+   |      | +------------------+   |
| |    SoundPads     |   |      | |    SoundPads     |   |
| +------------------+   |      | +------------------+   |
+-----------|------------+      +-----------|------------+
            |                               |
            v                               v
      +--------------------------------------------+
      |                Réseau TCP                  |
      +--------------------------------------------+
                            |
                            v
            +---------------------------+
            |       Serveur (Hôte)      |
            | +---------------------+   |
            | |        Room         |   |
            | +---------------------+   |
            | | - Gestion connexions|   |
            | | - Synchronisation   |   |
            | | - Diffusion messages|   |
            | +---------------------+   |
            +---------------------------+
```

## Détails Techniques

### Format des Messages JSON Échangés

Les communications entre clients et serveur utilisent des messages JSON avec la structure suivante :

#### Message de Connexion (join)
```json
{
  "type": "join",
  "data": {
    "username": "NomUtilisateur"
  }
}
```

#### Notification d'Ajout de SoundPad (soundpad_added)
```json
{
  "type": "soundpad_added",
  "data": {
    "title": "Titre du Pad",
    "filePath": "chemin/vers/fichier.mp3",
    "imagePath": "chemin/vers/image.png",
    "canDuplicatePlay": true,
    "boardIndex": 0,
    "position": {"row": 2, "column": 3}
  }
}
```

#### Notification de Suppression de SoundPad (soundpad_removed)
```json
{
  "type": "soundpad_removed",
  "data": {
    "boardIndex": 0,
    "position": {"row": 2, "column": 3}
  }
}
```

#### Notification de Connexion d'Utilisateur (user_connected)
```json
{
  "type": "user_connected",
  "data": {
    "username": "NomUtilisateur"
  }
}
```

### Protocole Réseau Détaillé

1. **Établissement de la Connexion**
   - Le serveur écoute sur un port spécifique (par défaut : port aléatoire)
   - Le client se connecte au serveur via l'adresse IP et le port
   - Le client envoie un message "join" avec son nom d'utilisateur
   - Le serveur accepte la connexion et ajoute l'utilisateur à la liste des utilisateurs connectés
   - Le serveur notifie tous les autres clients de la nouvelle connexion

2. **Synchronisation Initiale**
   - Le serveur envoie l'état actuel de tous les Boards et SoundPads au client qui vient de se connecter
   - Cette synchronisation se fait par une série de messages "soundpad_added"
   - Le client construit son interface à partir de ces informations

3. **Communication en Temps Réel**
   - Chaque modification (ajout/suppression de SoundPad) est immédiatement transmise
   - Le serveur est responsable de diffuser les modifications à tous les clients
   - Les messages sont accompagnés d'un identifiant de source pour éviter les boucles

4. **Gestion des Déconnexions**
   - Lorsqu'un client se déconnecte, le serveur envoie un message "user_disconnected" aux autres clients
   - Si le serveur (hôte) se déconnecte, tous les clients sont notifiés et la session est terminée

### Gestion des Ressources (Audio, Images)

1. **Ressources Audio**
   - Formats supportés : MP3, WAV, OGG
   - Les fichiers audio sont chargés via la classe QMediaPlayer
   - Pour les performances, les fichiers audio sont mis en cache après le premier chargement
   - Les ressources sont libérées lorsqu'un SoundPad est supprimé

2. **Ressources Image**
   - Formats supportés : PNG, JPG, GIF
   - Les images sont redimensionnées automatiquement pour s'adapter à la taille du SoundPad
   - Un système de cache d'images évite les chargements répétés
   - Les images trop grandes sont automatiquement compressées pour réduire la consommation mémoire

3. **Transfert de Fichiers**
   - Les fichiers ne sont pas transférés automatiquement entre clients
   - Seuls les chemins d'accès sont synchronisés
   - Un système de référentiels communs est recommandé pour les équipes

### Optimisations Implémentées

1. **Optimisations Réseau**
   - Compression des messages JSON pour réduire la bande passante
   - Regroupement des mises à jour en paquets pour diminuer le nombre de messages
   - Utilisation de tampons pour gérer les pics de trafic

2. **Optimisations Audio**
   - Lecture différée pour éviter les latences
   - Préchargement des sons fréquemment utilisés
   - Gestion intelligente des ressources audio pour minimiser la consommation mémoire

3. **Optimisations Interface**
   - Rendu efficace des grilles de SoundPads
   - Chargement progressif des tableaux volumineux
   - Actualisation sélective de l'interface pour éviter les rafraîchissements complets

## Composants Principaux

### SoundPad

La classe `SoundPad` est le composant fondamental de l'application. Chaque SoundPad représente un bouton qui, lorsqu'il est activé, joue un son spécifique.

**Caractéristiques principales** :
- Titre personnalisable
- Fichier audio associé
- Image optionnelle
- Option pour jouer simultanément plusieurs fois le même son
- Raccourci clavier configurable
- Prise en charge du glisser-déposer pour importer des sons

**Méthodes principales** :
- `play()` : Joue le son associé
- `importSound()` : Importe un fichier audio
- `editMetadata()` : Modifie les métadonnées du pad (titre, image, etc.)

### Board

La classe `Board` gère un ensemble de SoundPads organisés dans une grille.

**Caractéristiques principales** :
- Titre personnalisable
- Collection de SoundPads
- Mise en page automatique en grille
- Zone de défilement pour accéder à de nombreux pads

**Méthodes principales** :
- `addSoundPad()` : Ajoute un nouveau SoundPad vide
- `removeSoundPad()` : Supprime un SoundPad existant
- `importSound()` : Importe un son et crée un nouveau SoundPad
- `reorganizeGrid()` : Réorganise l'affichage des SoundPads dans la grille

### Room

La classe `Room` gère la collaboration entre utilisateurs via un système client-serveur.

**Caractéristiques principales** :
- Nom personnalisable
- Mode hôte (serveur) ou client
- Gestion des connexions utilisateurs
- Synchronisation des SoundPads entre utilisateurs
- Système de code d'invitation pour rejoindre une Room

**Méthodes principales** :
- `startServer()` : Démarre le serveur pour héberger une Room
- `connectToRoom()` : Se connecte à une Room existante
- `joinWithCode()` : Rejoint une Room via un code d'invitation
- `notifySoundPadAdded()` : Synchronise l'ajout d'un SoundPad avec tous les utilisateurs
- `notifySoundPadRemoved()` : Synchronise la suppression d'un SoundPad avec tous les utilisateurs

### User

La classe `User` représente un utilisateur de l'application.

**Caractéristiques principales** :
- Nom d'utilisateur
- Préférences utilisateur

### MainWindow

La classe `MainWindow` est l'interface principale de l'application, qui orchestre tous les autres composants.

**Caractéristiques principales** :
- Interface à onglets pour gérer plusieurs Boards
- Dock latéral affichant les utilisateurs connectés
- Menus et barres d'outils pour les actions principales

**Méthodes principales** :
- `createRoom()` : Crée une nouvelle Room
- `joinRoom()` : Rejoint une Room existante
- `addNewBoard()` : Ajoute un nouveau Board
- `removeCurrentBoard()` : Supprime le Board actif
- `updateUsersList()` : Met à jour la liste des utilisateurs connectés

## Flux de Travail de l'Application

### Création d'une Room

1. L'utilisateur démarre l'application et configure son profil
2. Il crée une nouvelle Room via `createRoom()`
3. La Room démarre un serveur en appelant `startServer()`
4. Un code d'invitation est généré pour permettre à d'autres utilisateurs de se connecter

### Connexion à une Room

1. L'utilisateur démarre l'application et configure son profil
2. Il rejoint une Room existante via `joinRoom()` ou `joinWithCode()`
3. Le client se connecte au serveur et reçoit l'état actuel de la Room

### Synchronisation des SoundPads

#### Création par l'hôte
1. L'hôte crée un pad via `board->addSoundPad()`
2. Le signal `soundPadAdded` est émis
3. `MainWindow` capte ce signal et appelle `room->notifySoundPadAdded()`
4. `notifySoundPadAdded()` envoie un message à tous les clients
5. Les clients reçoivent le message et créent le SoundPad localement

#### Création par un client
1. Le client crée un pad via `board->addSoundPad()`
2. Le client appelle `room->notifySoundPadAdded()`
3. `notifySoundPadAdded()` envoie un message à l'hôte
4. L'hôte reçoit le message, traite le SoundPad et le diffuse aux autres clients
5. Les autres clients reçoivent le message et créent le SoundPad localement

## Communication Réseau

La communication entre les clients et le serveur utilise les sockets TCP (`QTcpServer` et `QTcpSocket`) de Qt. Les messages sont sérialisés en JSON pour faciliter l'échange de données.

**Types de messages principaux** :
- "join" : Demande de connexion d'un client
- "user_connected" : Notification de connexion d'un utilisateur
- "user_disconnected" : Notification de déconnexion d'un utilisateur
- "soundpad_added" : Notification d'ajout d'un SoundPad
- "soundpad_removed" : Notification de suppression d'un SoundPad

## Dépendances du Projet

Le projet est basé sur les composants suivants :
- **Qt Framework** : Framework C++ pour l'interface graphique et les fonctionnalités réseau
- **Qt Widgets** : Pour l'interface utilisateur
- **Qt Multimedia** : Pour la lecture des fichiers audio
- **Qt Network** : Pour les fonctionnalités réseau

## Construction du Projet

Le projet utilise CMake comme système de build. Le fichier `CMakeLists.txt` définit les sources, les dépendances et les options de compilation.

**Prérequis** :
- Qt 5 ou Qt 6
- CMake 3.16 ou supérieur
- Compilateur C++ supportant C++17

**Instructions de compilation** :
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Fonctionnalités Clés

1. **Création et lecture de pads sonores** : Importation, personnalisation et lecture de sons
2. **Organisation en tableaux** : Gestion de collections de SoundPads
3. **Collaboration en temps réel** : Partage et synchronisation des SoundPads entre utilisateurs
4. **Système d'invitation simplifié** : Code d'invitation pour rejoindre facilement une Room
5. **Interface utilisateur intuitive** : Organisation claire des pads et des fonctionnalités

## Fonctinalité avenir

### Gestion de fichier (8-16 heure de dev)

Cela inclut la mise en place d'un nouveau type de message (par exemple "file_transfer") dans le protocole JSON, la lecture du fichier, son envoi (éventuellement découpé en tranches pour les gros fichiers) ainsi que la réception et le réassemblage des tranches pour sauvegarder le fichier localement.


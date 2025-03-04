#include "soundpad.h"
#include "board.h"
#include <QApplication>
#include "server.h"
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Parse les options de ligne de commande
    QCommandLineParser parser;
    parser.setApplicationDescription("Ynoise - Une application de soundboard réseau");
    parser.addHelpOption();
    
    QCommandLineOption serverOption(QStringList() << "s" << "server", "Démarrer en mode serveur");
    parser.addOption(serverOption);
    
    QCommandLineOption clientOption(QStringList() << "c" << "client", "Démarrer en mode client et se connecter au serveur", "adresseServeur", "127.0.0.1");
    parser.addOption(clientOption);
    
    parser.process(a);
    
    bool asServer = parser.isSet(serverOption) || !parser.isSet(clientOption);
    QString serverAddress = parser.value(clientOption);
    
    // Créer les objets principaux
    Soundpad soundpad("Son Test", "C:/chemin/vers/son.wav", "", true, "Ctrl+S");
    Board board;
    
    // Ajouter la soundpad à la board
    board.addSoundpad(&soundpad);
    
    // Connecter le signal de jeu
    QObject::connect(&soundpad, &Soundpad::play, &board, &Board::isPressed);
    
    // Activer le mode serveur par défaut
    board.startNetworking(asServer, serverAddress);
    
    // Démarrer la lecture des sons
    soundpad.setIsPlaying(true);
    
    qDebug() << "Application démarrée en mode" << (asServer ? "serveur" : "client");
    
    return a.exec();
}

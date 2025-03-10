#ifndef BOARD_H
#define BOARD_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include "soundpad.h"

/**
 * @brief Classe représentant un tableau de SoundPads
 */
class Board : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de Board
     * @param title Titre du tableau
     * @param parent Widget parent
     */
    explicit Board(const QString &title = "", QWidget *parent = nullptr);
    ~Board();

    /**
     * @brief Obtient le titre du tableau
     * @return Titre du tableau
     */
    QString getTitle() const { return m_title; }
    
    /**
     * @brief Définit le titre du tableau
     * @param title Nouveau titre
     */
    void setTitle(const QString &title);
    
    /**
     * @brief Obtient la liste des SoundPads
     * @return Liste des SoundPads
     */
    QVector<SoundPad*> getSoundPads() const { return m_soundPads; }
    
    /**
     * @brief Met à jour l'affichage des SoundPads
     * Cette méthode publique permet de réorganiser l'affichage des SoundPads
     */
    void updateDisplay() { reorganizeGrid(); }

public slots:
    /**
     * @brief Importe un son et crée un nouveau SoundPad
     */
    void importSound();
    
    /**
     * @brief Ajoute un nouveau SoundPad vide
     * @return Pointeur vers le SoundPad créé
     */
    SoundPad* addSoundPad();
    
    /**
     * @brief Ajoute un SoundPad provenant d'un autre utilisateur via le réseau
     * @param pad SoundPad déjà créé à ajouter
     * @return true si le pad a été ajouté avec succès
     */
    bool addSoundPadFromRemote(SoundPad* pad);
    
    /**
     * @brief Supprime un SoundPad
     * @param pad SoundPad à supprimer
     */
    void removeSoundPad(SoundPad* pad);

signals:
    /**
     * @brief Signal émis lorsque le titre du tableau change
     * @param title Nouveau titre
     */
    void titleChanged(const QString &title);
    
    /**
     * @brief Signal émis lorsqu'un SoundPad est ajouté
     * @param pad SoundPad ajouté
     */
    void soundPadAdded(SoundPad *pad);
    
    /**
     * @brief Signal émis lorsqu'un SoundPad est supprimé
     * @param pad SoundPad supprimé
     */
    void soundPadRemoved(SoundPad *pad);

private:
    QString m_title;                  // Titre du tableau
    QVector<SoundPad*> m_soundPads;   // Liste des SoundPads
    QGridLayout *m_gridLayout;        // Layout pour organiser les SoundPads
    QWidget *m_contentWidget;         // Widget contenant la grille
    QScrollArea *m_scrollArea;        // Zone de défilement
    QPushButton *m_addButton;         // Bouton pour ajouter un SoundPad

    /**
     * @brief Configure l'interface utilisateur
     */
    void setupUi();
    
    /**
     * @brief Réorganise les SoundPads dans la grille
     */
    void reorganizeGrid();
};

#endif // BOARD_H

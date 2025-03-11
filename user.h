#ifndef USER_H
#define USER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVector>
#include "room.h"

/**
 * @brief Classe représentant un utilisateur de l'application
 */
class User : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de User
     * @param name Nom de l'utilisateur
     * @param parent QObject parent
     */
    explicit User(const QString &name = "", QObject *parent = nullptr);
    ~User();

    /**
     * @brief Obtient le nom de l'utilisateur
     * @return Nom de l'utilisateur
     */
    QString getName() const { return m_name; }
    
    /**
     * @brief Définit le nom de l'utilisateur
     * @param name Nouveau nom
     */
    void setName(const QString &name);
    
    /**
     * @brief Obtient les paramètres de l'utilisateur
     * @return Paramètres sous forme d'objet JSON
     */
    QJsonObject getSettings() const { return m_settings; }
    
    /**
     * @brief Définit les paramètres de l'utilisateur
     * @param settings Nouveaux paramètres
     */
    void setSettings(const QJsonObject &settings);
    
    /**
     * @brief Obtient un paramètre spécifique
     * @param key Clé du paramètre
     * @param defaultValue Valeur par défaut
     * @return Valeur du paramètre
     */
    QVariant getSetting(const QString &key, const QVariant &defaultValue = QVariant()) const;
    
    /**
     * @brief Définit un paramètre spécifique
     * @param key Clé du paramètre
     * @param value Valeur du paramètre
     */
    void setSetting(const QString &key, const QVariant &value);
    
    /**
     * @brief Obtient les rooms associées à l'utilisateur
     * @return Liste des rooms
     */
    QVector<Room*> getRooms() const { return m_rooms; }
    
    /**
     * @brief Ajoute une room à l'utilisateur
     * @param room Room à ajouter
     */
    void addRoom(Room* room);
    
    /**
     * @brief Supprime une room de l'utilisateur
     * @param room Room à supprimer
     */
    void removeRoom(Room* room);

public slots:
    /**
     * @brief Sauvegarde les paramètres dans un fichier
     * @return true si la sauvegarde a réussi
     */
    bool saveSettings();
    
    /**
     * @brief Charge les paramètres depuis un fichier
     * @return true si le chargement a réussi
     */
    bool loadSettings();

signals:
    /**
     * @brief Signal émis lorsque le nom est modifié
     * @param newName Nouveau nom
     */
    void nameChanged(const QString &newName);
    
    /**
     * @brief Signal émis lorsque les paramètres sont modifiés
     */
    void settingsChanged();
    
    /**
     * @brief Signal émis lorsqu'une room est ajoutée
     * @param room Room ajoutée
     */
    void roomAdded(Room* room);
    
    /**
     * @brief Signal émis lorsqu'une room est supprimée
     * @param room Room supprimée
     */
    void roomRemoved(Room* room);

private:
    QString m_name;             // Nom de l'utilisateur
    QJsonObject m_settings;     // Paramètres de l'utilisateur
    QVector<Room*> m_rooms;     // Rooms associées à l'utilisateur
    
    /**
     * @brief Obtient le chemin du fichier de paramètres
     * @return Chemin du fichier
     */
    QString settingsFilePath() const;
};

#endif // USER_H

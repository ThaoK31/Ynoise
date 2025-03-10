#include "user.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>  // Ajout de l'inclusion manquante
#include <QRandomGenerator>
#include <QStringList>
#include <QDebug>

User::User(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name("")
{
    // Charger les paramètres existants
    loadSettings();
    
    // Liste de personnages d'anime pour attribution aléatoire
    QStringList animeCharacters = {
        "Naruto Uzumaki", "Sasuke Uchiha", "Sakura Haruno", "Kakashi Hatake", "Hinata Hyuga",
        "Shikamaru Nara", "Choji Akimichi", "Ino Yamanaka", "Rock Lee", "Neji Hyuga",
        "Tenten", "Gaara", "Kankuro", "Temari", "Jiraiya",
        "Tsunade", "Orochimaru", "Itachi Uchiha", "Kisame Hoshigaki", "Deidara",
        "Sasori", "Hidan", "Kakuzu", "Konan", "Pain",
        "Nagato", "Obito Uchiha", "Madara Uchiha", "Hashirama Senju", "Tobirama Senju",
        "Hiruzen Sarutobi", "Might Guy", "Iruka Umino", "Shino Aburame", "Kiba Inuzuka",
        "Akamaru", "Asuma Sarutobi", "Kurenai Yuhi", "Anko Mitarashi", "Yamato",
        "Sai", "Sumire Kakei", "Mitsuki", "Boruto Uzumaki", "Himawari Uzumaki",
        "Sarada Uchiha", "Shikadai Nara", "Inojin Yamanaka", "Killer Bee", "Minato Namikaze",
        "Kushina Uzumaki", "Konohamaru Sarutobi", "Hanabi Hyuga", "Inari", "Genma Shiranui",
        "Mizuki", "Karin", "Suigetsu Hozuki", "Jugo", "Zabuza Momochi",
        "Haku", "Mifune", "Fuu", "Mirai Sarutobi", "Udon",
        "Metal Lee", "Goku", "Vegeta", "Gohan", "Trunks",
        "Piccolo", "Frieza", "Cell", "Majin Buu", "Krillin",
        "Yamcha", "Android 18", "Android 17", "Bulma", "Master Roshi",
        "Chi-Chi", "Nappa", "Goten", "Beerus", "Whis",
        "Jiren", "Hit", "Goku Black", "Zamasu", "Yusuke Urameshi",
        "Kazuma Kuwabara", "Hiei", "Kurama", "Genkai", "Meliodas",
        "Ban", "King", "Diane", "Escanor", "Elizabeth Liones"
    };
    
    // Attribuer un nom aléatoire
    int randomIndex = QRandomGenerator::global()->bounded(animeCharacters.size());
    m_name = animeCharacters[randomIndex];
    
    // Enregistrer ce nom dans les paramètres
    setSetting("name", m_name);
    saveSettings();
    
    qDebug() << "Nom d'utilisateur attribué aléatoirement:" << m_name;
}

User::~User()
{
    // Sauvegarde des paramètres
    saveSettings();
}

void User::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        setSetting("name", m_name);
        emit nameChanged(m_name);
    }
}

void User::setSettings(const QJsonObject &settings)
{
    m_settings = settings;
    emit settingsChanged();
}

QVariant User::getSetting(const QString &key, const QVariant &defaultValue) const
{
    if (m_settings.contains(key)) {
        QJsonValue value = m_settings.value(key);
        
        switch (value.type()) {
            case QJsonValue::Bool:
                return value.toBool();
            case QJsonValue::Double:
                return value.toDouble();
            case QJsonValue::String:
                return value.toString();
            case QJsonValue::Array:
                return QVariant::fromValue(value.toArray());
            case QJsonValue::Object:
                return QVariant::fromValue(value.toObject());
            default:
                return defaultValue;
        }
    }
    
    return defaultValue;
}

void User::setSetting(const QString &key, const QVariant &value)
{
    QJsonValue jsonValue;
    
    // Utilisation de QMetaType au lieu de QVariant::type()
    QMetaType::Type valueType = static_cast<QMetaType::Type>(value.typeId());
    
    switch (valueType) {
        case QMetaType::Bool:
            jsonValue = QJsonValue(value.toBool());
            break;
        case QMetaType::Int:
        case QMetaType::Double:
            jsonValue = QJsonValue(value.toDouble());
            break;
        case QMetaType::QString:
            jsonValue = QJsonValue(value.toString());
            break;
        default:
            // Types complexes non pris en charge ici
            return;
    }
    
    m_settings[key] = jsonValue;
    emit settingsChanged();
}

void User::addRoom(Room* room)
{
    if (room && !m_rooms.contains(room)) {
        m_rooms.append(room);
        emit roomAdded(room);
    }
}

void User::removeRoom(Room* room)
{
    if (room && m_rooms.contains(room)) {
        m_rooms.removeOne(room);
        emit roomRemoved(room);
    }
}

bool User::saveSettings()
{
    // Créer le dossier de paramètres si nécessaire
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Ouvrir le fichier en écriture
    QFile file(settingsFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        // Convertir les paramètres en JSON
        QJsonDocument doc(m_settings);
        
        // Écrire dans le fichier
        file.write(doc.toJson());
        file.close();
        
        return true;
    }
    
    return false;
}

bool User::loadSettings()
{
    // Vérifier si le fichier existe
    QFile file(settingsFilePath());
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        // Lire le contenu du fichier
        QByteArray data = file.readAll();
        file.close();
        
        // Parser le JSON
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isNull() && doc.isObject()) {
            // Mettre à jour les paramètres
            m_settings = doc.object();
            emit settingsChanged();
            
            return true;
        }
    }
    return false;
}

QString User::settingsFilePath() const
{
    // Chemin du fichier de paramètres
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.json";
}

#include "soundpad.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QKeySequenceEdit>
#include <QMenu>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDataStream>
#include <QBuffer>

void SoundPad::log(const QString &message)
{
    qDebug() << "[SoundPad]" << message;
    emit logMessage(message);
}

// Version constante de log pour les méthodes constantes
void SoundPad::logConst(const QString &message) const
{
    qDebug() << "[SoundPad]" << message;
    // Ne peut pas émettre de signal dans une méthode const
}

QString SoundPad::getAudioInfo() const {
    if (m_audioData.isEmpty()) {
        return tr("Pas de données audio");
    }
    return tr("Données audio: %1 bytes").arg(m_audioData.size());
}

QString SoundPad::getImageInfo() const {
    if (m_imageData.isEmpty()) {
        return tr("Pas de données image");
    }
    return tr("Données image: %1 bytes").arg(m_imageData.size());
}

QByteArray SoundPad::serialize() const {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    
    // Version du format de sérialisation
    stream << (qint32)1;
    
    // Sérialisation des données
    stream << m_title;
    stream << m_canDuplicatePlay;
    stream << m_shortcut;
    stream << m_audioData;
    stream << m_imageData;
    
    logConst(tr("Sérialisation: %1 bytes de données créées").arg(data.size()));
    return data;
}

bool SoundPad::deserialize(const QByteArray &data) {
    QDataStream stream(data);
    
    // Vérifier la version
    qint32 version;
    stream >> version;
    if (version != 1) {
        log(tr("Erreur de désérialisation: version incompatible %1").arg(version));
        return false;
    }
    
    // Désérialisation des données
    QString title;
    bool canDuplicatePlay;
    QKeySequence shortcut;
    QByteArray audioData, imageData;
    
    stream >> title;
    stream >> canDuplicatePlay;
    stream >> shortcut;
    stream >> audioData;
    stream >> imageData;
    
    if (stream.status() != QDataStream::Ok) {
        log(tr("Erreur lors de la désérialisation des données"));
        return false;
    }
    
    // Appliquer les données
    setTitle(title);
    setCanDuplicatePlay(canDuplicatePlay);
    setShortcut(shortcut);
    setAudioData(audioData);
    setImageData(imageData);
    
    log(tr("Désérialisation réussie: %1 bytes de données audio, %2 bytes de données image")
        .arg(audioData.size())
        .arg(imageData.size()));
    return true;
}

SoundPad::SoundPad(const QString &title, 
                   const QString &filePath,
                   const QString &imagePath,
                   bool canDuplicatePlay,
                   const QKeySequence &shortcut,
                   QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_filePath(filePath)
    , m_imagePath(imagePath)
    , m_canDuplicatePlay(canDuplicatePlay)
    , m_isPlaying(false)
    , m_shortcut(shortcut)
    , m_audioBuffer(nullptr)
    , m_button(nullptr)
    , m_imageLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_layout(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
{
    log(tr("Création d'un nouveau SoundPad: %1").arg(title));
    
    // Configuration de l'apparence et des comportements
    setupUi();
    
    // Chargement de l'image si un chemin est spécifié
    if (!m_imagePath.isEmpty()) {
        log(tr("Chargement de l'image: %1").arg(m_imagePath));
        loadImageFromFile(m_imagePath);
        updateUI();
    }
    
    // Configuration du lecteur média
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    
    if (!m_filePath.isEmpty()) {
        log(tr("Chargement du son: %1").arg(m_filePath));
        loadAudioFromFile(m_filePath);
    }
    
    // Connexion du signal de fin de lecture
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState) {
            m_isPlaying = false;
            m_button->setStyleSheet(""); // Réinitialiser le style
            log(tr("Fin de la lecture"));
        }
    });
    
    // Accepter le glisser-déposer
    setAcceptDrops(true);
}

bool SoundPad::loadAudioFromFile(const QString &filePath)
{
    log(tr("Tentative de chargement du fichier audio: %1").arg(filePath));
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        log(tr("Erreur: Impossible d'ouvrir le fichier audio"));
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier audio."));
        return false;
    }
    
    m_filePath = filePath;
    m_audioData = file.readAll();
    file.close();
    
    log(tr("Fichier audio chargé avec succès: %1 bytes").arg(m_audioData.size()));
    
    updateMediaPlayer();
    return true;
}

bool SoundPad::loadImageFromFile(const QString &imagePath)
{
    log(tr("Tentative de chargement du fichier image: %1").arg(imagePath));
    
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        log(tr("Erreur: Impossible d'ouvrir le fichier image"));
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier image."));
        return false;
    }
    
    m_imagePath = imagePath;
    m_imageData = file.readAll();
    file.close();
    
    if (!m_image.loadFromData(m_imageData)) {
        log(tr("Erreur: Format d'image non supporté"));
        QMessageBox::warning(this, tr("Erreur"), tr("Format d'image non supporté."));
        m_imageData.clear();
        return false;
    }
    
    log(tr("Image chargée avec succès: %1 bytes").arg(m_imageData.size()));
    return true;
}

void SoundPad::setAudioData(const QByteArray &audioData)
{
    log(tr("Définition des données audio: %1 bytes").arg(audioData.size()));
    m_audioData = audioData;
    updateMediaPlayer();
}

void SoundPad::setImageData(const QByteArray &imageData)
{
    log(tr("Définition des données image: %1 bytes").arg(imageData.size()));
    m_imageData = imageData;
    if (!m_imageData.isEmpty()) {
        if (!m_image.loadFromData(m_imageData)) {
            log(tr("Erreur: Format d'image non supporté"));
            m_imageData.clear();
        } else {
            log(tr("Image chargée avec succès"));
            updateUI();
        }
    }
}

void SoundPad::updateMediaPlayer()
{
    log(tr("Mise à jour du lecteur média"));
    
    // Si un ancien buffer existe, le supprimer
    if (m_audioBuffer) {
        delete m_audioBuffer;
        m_audioBuffer = nullptr;
    }
    
    // Créer un nouveau buffer si on a des données audio
    if (!m_audioData.isEmpty()) {
        m_audioBuffer = new QBuffer(this);
        m_audioBuffer->setData(m_audioData);
        m_audioBuffer->open(QIODevice::ReadOnly);
        
        // Utiliser le buffer comme source pour le lecteur média
        m_mediaPlayer->setSourceDevice(m_audioBuffer, QUrl("mem://audio"));
        log(tr("Lecteur média configuré avec %1 bytes de données audio").arg(m_audioData.size()));
    } else {
        log(tr("Pas de données audio à configurer"));
    }
}

void SoundPad::play()
{
    if (m_audioData.isEmpty()) {
        log(tr("Tentative de lecture sans données audio"));
        QMessageBox::warning(this, tr("Avertissement"), tr("Aucun fichier audio chargé."));
        return;
    }
    
    if (m_canDuplicatePlay || !m_isPlaying) {
        log(tr("Lecture du son (%1 bytes)").arg(m_audioData.size()));
        m_mediaPlayer->play();
        m_isPlaying = true;
        m_button->setStyleSheet("background-color: rgba(0, 255, 0, 100);");
    } else {
        log(tr("Lecture ignorée: son déjà en cours de lecture"));
    }
}

void SoundPad::editMetadata()
{
    // Création d'une boîte de dialogue pour l'édition des métadonnées
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Configurer le SoundPad"));
    
    QFormLayout formLayout(&dialog);
    
    // Champs de saisie
    QLineEdit titleEdit(m_title, &dialog);
    QLineEdit filePathEdit(m_filePath, &dialog);
    QLineEdit imagePathEdit(m_imagePath, &dialog);
    QCheckBox duplicateCheckBox(&dialog);
    duplicateCheckBox.setChecked(m_canDuplicatePlay);
    QKeySequenceEdit shortcutEdit(&dialog);
    shortcutEdit.setKeySequence(m_shortcut);
    
    // Boutons pour importer son et image
    QPushButton importSoundButton(tr("Choisir un son..."), &dialog);
    QPushButton importImageButton(tr("Choisir une image..."), &dialog);
    
    // Ajout des champs au formulaire
    formLayout.addRow(tr("Titre:"), &titleEdit);
    formLayout.addRow(tr("Fichier audio:"), &filePathEdit);
    formLayout.addRow(QString(), &importSoundButton);
    formLayout.addRow(tr("Image:"), &imagePathEdit);
    formLayout.addRow(QString(), &importImageButton);
    formLayout.addRow(tr("Lecture multiple:"), &duplicateCheckBox);
    formLayout.addRow(tr("Raccourci clavier:"), &shortcutEdit);
    
    // Boutons d'action
    QHBoxLayout buttonsLayout;
    QPushButton saveButton(tr("Enregistrer"), &dialog);
    QPushButton cancelButton(tr("Annuler"), &dialog);
    buttonsLayout.addWidget(&saveButton);
    buttonsLayout.addWidget(&cancelButton);
    formLayout.addRow(&buttonsLayout);
    
    // Connexion des signaux
    connect(&importSoundButton, &QPushButton::clicked, this, [&filePathEdit, this]() {
        QString filePath = QFileDialog::getOpenFileName(this, 
            tr("Importer un son"), 
            QString(), 
            tr("Fichiers audio (*.mp3 *.wav *.ogg)"));
        
        if (!filePath.isEmpty()) {
            filePathEdit.setText(filePath);
        }
    });
    
    connect(&importImageButton, &QPushButton::clicked, this, [&imagePathEdit, this]() {
        QString imagePath = QFileDialog::getOpenFileName(this, 
            tr("Importer une image"), 
            QString(), 
            tr("Images (*.png *.jpg *.jpeg *.bmp)"));
        
        if (!imagePath.isEmpty()) {
            imagePathEdit.setText(imagePath);
        }
    });
    
    connect(&saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Exécution de la boîte de dialogue
    if (dialog.exec() == QDialog::Accepted) {
        // Récupération des valeurs
        setTitle(titleEdit.text());
        
        // Si le chemin audio a changé, charger le nouveau fichier
        if (filePathEdit.text() != m_filePath) {
            loadAudioFromFile(filePathEdit.text());
        }
        
        // Si le chemin image a changé, charger la nouvelle image
        if (imagePathEdit.text() != m_imagePath) {
            loadImageFromFile(imagePathEdit.text());
        }
        
        setCanDuplicatePlay(duplicateCheckBox.isChecked());
        setShortcut(shortcutEdit.keySequence());
        
        // Mise à jour de l'interface
        updateUI();
        
        // Émission du signal de modification
        emit metadataChanged();
    }
}

bool SoundPad::dragAndDrop()
{
    // Méthode à implémenter selon les besoins
    return false;
}

void SoundPad::keyPressEvent(QKeyEvent *event)
{
    // Si un raccourci est défini et qu'il correspond à la touche pressée
    if (!m_shortcut.isEmpty() && m_shortcut.matches(QKeySequence(event->key() | event->modifiers())) == QKeySequence::ExactMatch) {
        play();
        event->accept();
        return;
    }
    
    QWidget::keyPressEvent(event);
}

void SoundPad::dragEnterEvent(QDragEnterEvent *event)
{
    // Accepter les glisser-déposer de fichiers audio et images
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void SoundPad::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        
        // Ne prendre que le premier fichier
        if (!urlList.isEmpty()) {
            QString filePath = urlList.first().toLocalFile();
            QFileInfo fileInfo(filePath);
            QString extension = fileInfo.suffix().toLower();
            
            // Vérifier le type de fichier
            if (extension == "mp3" || extension == "wav" || extension == "ogg") {
                // Fichier audio
                loadAudioFromFile(filePath);
                event->acceptProposedAction();
            } else if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp") {
                // Fichier image
                loadImageFromFile(filePath);
                updateUI();
                event->acceptProposedAction();
            } else {
                // Type de fichier non supporté
                QMessageBox::warning(this, tr("Type de fichier non supporté"), 
                    tr("Seuls les fichiers audio (mp3, wav, ogg) et images (png, jpg, jpeg, bmp) sont acceptés."));
                event->ignore();
            }
        }
    } else {
        event->ignore();
    }
}

void SoundPad::setupUi()
{
    // Création des widgets
    m_button = new QPushButton(this);
    m_imageLabel = new QLabel(this);
    m_titleLabel = new QLabel(this);
    
    // Configuration du layout
    m_layout = new QVBoxLayout(this);
    m_layout->addWidget(m_imageLabel, 1);
    m_layout->addWidget(m_titleLabel, 0);
    m_layout->addWidget(m_button, 0);
    
    // Style de base
    m_button->setMinimumSize(50, 50);
    m_button->setMaximumSize(150, 150);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(50, 50);
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_imageLabel->setScaledContents(true);
    
    // Connexion du bouton
    connect(m_button, &QPushButton::clicked, this, &SoundPad::play);
    
    // Menu contextuel
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        QAction *editAction = menu.addAction(tr("Modifier"));
        QAction *playAction = menu.addAction(tr("Jouer"));
        
        connect(editAction, &QAction::triggered, this, &SoundPad::editMetadata);
        connect(playAction, &QAction::triggered, this, &SoundPad::play);
        
        menu.exec(mapToGlobal(pos));
    });
    
    // Initialisation de l'apparence
    updateUI();
}

void SoundPad::updateUI()
{
    // Mise à jour du titre
    if (m_title.isEmpty()) {
        m_titleLabel->setText(tr("(Sans titre)"));
    } else {
        m_titleLabel->setText(m_title);
    }
    
    // Mise à jour du texte du bouton
    if (m_audioData.isEmpty()) {
        m_button->setText(tr("Choisir un son"));
        
        // Déconnecter les anciens signaux
        m_button->disconnect();
        
        // Reconnecter pour l'importation
        connect(m_button, &QPushButton::clicked, this, &SoundPad::importSound, Qt::UniqueConnection);
    } else {
        m_button->setText(tr("Jouer"));
        
        // Déconnecter les anciens signaux
        m_button->disconnect();
        
        // Reconnecter pour la lecture
        connect(m_button, &QPushButton::clicked, this, &SoundPad::play, Qt::UniqueConnection);
    }
    
    // Mise à jour de l'image
    if (m_image.isNull()) {
        m_imageLabel->setText(tr("(Pas d'image)"));
        m_imageLabel->setPixmap(QPixmap());
    } else {
        m_imageLabel->setText("");
        m_imageLabel->setPixmap(m_image);
    }
}

SoundPad::~SoundPad()
{
    log(tr("Destruction du SoundPad: %1").arg(m_title));
    delete m_mediaPlayer;
    delete m_audioOutput;
    if (m_audioBuffer) {
        delete m_audioBuffer;
    }
}

void SoundPad::setTitle(const QString &title)
{
    log(tr("Modification du titre: %1").arg(title));
    m_title = title;
    updateUI();
}

void SoundPad::setFilePath(const QString &filePath)
{
    log(tr("Modification du chemin audio: %1").arg(filePath));
    m_filePath = filePath;
    if (!m_filePath.isEmpty()) {
        loadAudioFromFile(m_filePath);
    }
}

void SoundPad::setImagePath(const QString &imagePath)
{
    log(tr("Modification du chemin image: %1").arg(imagePath));
    m_imagePath = imagePath;
    if (!m_imagePath.isEmpty()) {
        loadImageFromFile(m_imagePath);
        updateUI();
    }
}

void SoundPad::setImage(const QPixmap &image)
{
    log(tr("Définition d'une nouvelle image"));
    m_image = image;
    
    // Sauvegarder également les données de l'image
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    m_image.save(&buffer, "PNG");
    m_imageData = data;
    
    log(tr("Image convertie en données: %1 bytes").arg(m_imageData.size()));
    updateUI();
}

void SoundPad::setCanDuplicatePlay(bool canDuplicatePlay)
{
    log(tr("Modification de la lecture multiple: %1").arg(canDuplicatePlay ? "activée" : "désactivée"));
    m_canDuplicatePlay = canDuplicatePlay;
}

bool SoundPad::isPlaying() const
{
    return m_isPlaying;
}

void SoundPad::setShortcut(const QKeySequence &shortcut)
{
    log(tr("Modification du raccourci: %1").arg(shortcut.toString()));
    m_shortcut = shortcut;
}

bool SoundPad::saveAudioToFile(const QString &filePath) const
{
    logConst(tr("Tentative de sauvegarde audio vers: %1").arg(filePath));
    
    if (m_audioData.isEmpty()) {
        logConst(tr("Erreur: Pas de données audio à sauvegarder"));
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Aucune donnée audio à sauvegarder."));
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        logConst(tr("Erreur: Impossible de créer le fichier audio"));
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Impossible de créer le fichier audio."));
        return false;
    }
    
    file.write(m_audioData);
    file.close();
    
    logConst(tr("Audio sauvegardé avec succès: %1 bytes").arg(m_audioData.size()));
    return true;
}

bool SoundPad::saveImageToFile(const QString &filePath) const
{
    logConst(tr("Tentative de sauvegarde image vers: %1").arg(filePath));
    
    if (m_imageData.isEmpty()) {
        logConst(tr("Erreur: Pas de données image à sauvegarder"));
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Aucune donnée image à sauvegarder."));
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        logConst(tr("Erreur: Impossible de créer le fichier image"));
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Impossible de créer le fichier image."));
        return false;
    }
    
    file.write(m_imageData);
    file.close();
    
    logConst(tr("Image sauvegardée avec succès: %1 bytes").arg(m_imageData.size()));
    return true;
}

bool SoundPad::importSound()
{
    log(tr("Ouverture de la boîte de dialogue d'importation audio"));
    QString filePath = QFileDialog::getOpenFileName(this, 
        tr("Importer un son"), 
        QString(), 
        tr("Fichiers audio (*.mp3 *.wav *.ogg)"));
    
    if (!filePath.isEmpty()) {
        log(tr("Fichier audio sélectionné: %1").arg(filePath));
        return loadAudioFromFile(filePath);
    }
    
    log(tr("Importation audio annulée"));
    return false;
}

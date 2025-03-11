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
    , m_button(nullptr)
    , m_imageLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_layout(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
{
    // Configuration de l'apparence et des comportements
    setupUi();
    
    // Chargement de l'image si un chemin est spécifié
    if (!m_imagePath.isEmpty()) {
        m_image = QPixmap(m_imagePath);
        updateUI();
    }
    
    // Configuration du lecteur média
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    
    if (!m_filePath.isEmpty()) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(m_filePath));
    }
    
    // Connexion du signal de fin de lecture
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState) {
            m_isPlaying = false;
            m_button->setStyleSheet(""); // Réinitialiser le style
        }
    });
    
    // Accepter le glisser-déposer
    setAcceptDrops(true);
}

SoundPad::~SoundPad()
{
    delete m_mediaPlayer;
    delete m_audioOutput;
}

void SoundPad::setTitle(const QString &title)
{
    m_title = title;
    updateUI();
}

void SoundPad::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
    if (m_mediaPlayer) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(m_filePath));
    }
}

void SoundPad::setImagePath(const QString &imagePath)
{
    m_imagePath = imagePath;
    if (!m_imagePath.isEmpty()) {
        m_image = QPixmap(m_imagePath);
        updateUI();
    }
}

void SoundPad::setImage(const QPixmap &image)
{
    m_image = image;
    updateUI();
}

void SoundPad::setCanDuplicatePlay(bool canDuplicatePlay)
{
    m_canDuplicatePlay = canDuplicatePlay;
}

bool SoundPad::isPlaying() const
{
    return m_isPlaying;
}

void SoundPad::setShortcut(const QKeySequence &shortcut)
{
    m_shortcut = shortcut;
}

bool SoundPad::importSound()
{
    QString filePath = QFileDialog::getOpenFileName(this, 
        tr("Importer un son"), 
        QString(), 
        tr("Fichiers audio (*.mp3 *.wav *.ogg)"));
    
    if (!filePath.isEmpty()) {
        setFilePath(filePath);
        return true;
    }
    return false;
}

void SoundPad::play()
{
    if (m_filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Avertissement"), tr("Aucun fichier audio sélectionné."));
        return;
    }
    
    if (m_canDuplicatePlay || !m_isPlaying) {
        // Si on peut dupliquer la lecture ou si le son n'est pas déjà en cours de lecture
        m_mediaPlayer->play();
        m_isPlaying = true;
        
        // Indication visuelle que le pad est actif
        m_button->setStyleSheet("background-color: rgba(0, 255, 0, 100);");
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
        bool hasChanges = false;
        
        // Mise à jour des métadonnées
        if (m_title != titleEdit.text()) {
            m_title = titleEdit.text();
            hasChanges = true;
        }
        if (m_filePath != filePathEdit.text()) {
            m_filePath = filePathEdit.text();
            hasChanges = true;
        }
        if (m_imagePath != imagePathEdit.text()) {
            m_imagePath = imagePathEdit.text();
            hasChanges = true;
        }
        if (m_canDuplicatePlay != duplicateCheckBox.isChecked()) {
            m_canDuplicatePlay = duplicateCheckBox.isChecked();
            hasChanges = true;
        }
        if (m_shortcut != shortcutEdit.keySequence()) {
            m_shortcut = shortcutEdit.keySequence();
            hasChanges = true;
        }
        
        // Chargement de l'image et du son
        if (!m_imagePath.isEmpty()) {
            m_image = QPixmap(m_imagePath);
        }
        
        if (!m_filePath.isEmpty()) {
            m_mediaPlayer->setSource(QUrl::fromLocalFile(m_filePath));
        }
        
        // Mise à jour de l'interface
        updateUI();
        
        // Émission des signaux de modification
        if (hasChanges) {
            emit metadataChanged();
            emit soundPadModified(this);
        }
    }
}

bool SoundPad::dragAndDrop()
{
    // Cette fonction n'est pas directement utilisée, mais fait partie de l'interface
    // L'implémentation est réalisée via les événements dragEnterEvent et dropEvent
    return true;
}

void SoundPad::keyPressEvent(QKeyEvent *event)
{
    // Vérifier si la touche appuyée correspond au raccourci
    if (!m_shortcut.isEmpty() && event->keyCombination().toCombined() == m_shortcut[0].toCombined()) {
        play();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void SoundPad::dragEnterEvent(QDragEnterEvent *event)
{
    // Vérification que les données contiennent des URLs
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void SoundPad::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        
        // On ne traite que le premier fichier déposé
        if (!urlList.isEmpty()) {
            QString filePath = urlList.first().toLocalFile();
            
            // Vérifier si c'est un fichier audio ou une image
            QFileInfo fileInfo(filePath);
            QString suffix = fileInfo.suffix().toLower();
            
            if (suffix == "mp3" || suffix == "wav" || suffix == "ogg") {
                setFilePath(filePath);
                QMessageBox::information(this, tr("Fichier importé"), 
                    tr("Le fichier audio a été importé avec succès."));
            } else if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "bmp") {
                setImagePath(filePath);
                QMessageBox::information(this, tr("Image importée"), 
                    tr("L'image a été importée avec succès."));
            } else {
                QMessageBox::warning(this, tr("Format non supporté"), 
                    tr("Le fichier déposé n'est ni un fichier audio ni une image supportée."));
            }
        }
    }
    
    event->acceptProposedAction();
}

void SoundPad::setupUi()
{
    // Configuration du layout principal
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(2);
    
    // Création des éléments UI
    m_button = new QPushButton(this);
    m_button->setMinimumSize(100, 100);
    m_button->setMaximumSize(150, 150);
    m_button->setCursor(Qt::PointingHandCursor);
    
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_titleLabel = new QLabel(m_title, this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    // Organisation du layout
    m_layout->addWidget(m_imageLabel);
    m_layout->addWidget(m_titleLabel);
    
    // Configuration du bouton
    m_button->setLayout(m_layout);
    
    // Connexion du signal de clic
    connect(m_button, &QPushButton::clicked, this, &SoundPad::play);
    
    // Configuration du menu contextuel
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu *contextMenu = new QMenu(this);
        
        QAction *playAction = new QAction(tr("Jouer"), this);
        QAction *editAction = new QAction(tr("Configurer"), this);
        
        connect(playAction, &QAction::triggered, this, &SoundPad::play);
        connect(editAction, &QAction::triggered, this, &SoundPad::editMetadata);
        
        contextMenu->addAction(playAction);
        contextMenu->addAction(editAction);
        
        contextMenu->exec(mapToGlobal(pos));
        delete contextMenu; // Libérer la mémoire après utilisation
    });
    
    // Mise à jour initiale de l'UI
    updateUI();
}

void SoundPad::updateUI()
{
    // Mise à jour de l'image
    if (!m_image.isNull()) {
        QPixmap scaledImage = m_image.scaled(m_imageLabel->size(), 
                                           Qt::KeepAspectRatio, 
                                           Qt::SmoothTransformation);
        m_imageLabel->setPixmap(scaledImage);
    } else {
        // Image par défaut si aucune n'est spécifiée
        m_imageLabel->setText(tr("Aucune image"));
    }
    
    // Mise à jour du titre
    m_titleLabel->setText(m_title.isEmpty() ? tr("Sans titre") : m_title);
    
    // Tooltip avec les informations du pad
    setToolTip(QString("%1\nFichier: %2\nRaccourci: %3")
               .arg(m_title)
               .arg(m_filePath.isEmpty() ? tr("Non défini") : m_filePath)
               .arg(m_shortcut.isEmpty() ? tr("Non défini") : m_shortcut.toString()));
}

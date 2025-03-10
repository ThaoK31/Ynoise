#include "board.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QDateTime>

Board::Board(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_gridLayout(nullptr)
    , m_contentWidget(nullptr)
    , m_scrollArea(nullptr)
    , m_addButton(nullptr)
{
    setupUi();
}

Board::~Board()
{
    // Suppression des SoundPads
    qDeleteAll(m_soundPads);
    m_soundPads.clear();
}

void Board::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged(m_title);
    }
}

void Board::importSound()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, 
        tr("Importer des sons"), 
        QString(), 
        tr("Fichiers audio (*.mp3 *.wav *.ogg)"));
    
    for (const QString &filePath : filePaths) {
        if (!filePath.isEmpty()) {
            SoundPad *pad = addSoundPad();
            pad->setFilePath(filePath);
            
            // Définir le titre en fonction du nom du fichier
            QFileInfo fileInfo(filePath);
            pad->setTitle(fileInfo.baseName());
            
            emit soundPadAdded(pad);
        }
    }
}

SoundPad* Board::addSoundPad()
{
    // Création d'un nouveau SoundPad
    SoundPad *pad = new SoundPad(tr("Nouveau pad"), "", "", false, QKeySequence(), this);
    
    // Ajout à la liste
    m_soundPads.append(pad);
    
    // Réorganisation de la grille
    reorganizeGrid();
    
    // Connecter le signal de modification
    connect(pad, &SoundPad::soundPadModified, this, [this, pad]() {
        emit soundPadModified(pad);
    });
    
    // Configurer immédiatement le pad
    pad->editMetadata();
    
    // Signal d'ajout
    emit soundPadAdded(pad);
    
    return pad;
}

SoundPad* Board::getSoundPadById(const QString &padId)
{
    // Parcourir la liste des SoundPads pour trouver celui avec l'identifiant correspondant
    for (SoundPad* pad : m_soundPads) {
        if (pad->objectName() == padId) {
            return pad;
        }
    }
    // Retourner nullptr si aucun SoundPad n'est trouvé avec cet identifiant
    return nullptr;
}


bool Board::addSoundPadFromRemote(SoundPad* pad)
{
    if (!pad) {
        qDebug() << "ERREUR: Tentative d'ajout d'un SoundPad null depuis le réseau";
        return false;
    }
    
    // Vérifier que le pad a un identifiant unique
    if (pad->objectName().isEmpty()) {
        qDebug() << "ERREUR: Le SoundPad reçu n'a pas d'identifiant";
        return false;
    }
    
    // Vérifier que le pad n'existe pas déjà (vérification supplémentaire)
    for (SoundPad *existingPad : m_soundPads) {
        if (existingPad->objectName() == pad->objectName()) {
            qDebug() << "ERREUR: Un SoundPad avec le même identifiant existe déjà:" << pad->objectName();
            return false;
        }
    }
    
    // Définir le parent du pad comme étant ce board
    pad->setParent(this);
    
    // Connecter le signal de modification
    connect(pad, &SoundPad::soundPadModified, this, [this, pad]() {
        emit soundPadModified(pad);
    });
    
    // Ajout à la liste
    m_soundPads.append(pad);
    
    // Réorganisation de la grille
    reorganizeGrid();
    
    qDebug() << "SoundPad ajouté avec succès depuis le réseau:" << pad->objectName();
    
    // Signal d'ajout (évite la boucle infinie car le signal est déjà traité côté réception)
    emit soundPadAdded(pad);
    
    return true;
}

void Board::removeSoundPad(SoundPad* pad)
{
    if (pad && m_soundPads.contains(pad)) {
        // Signal de suppression
        emit soundPadRemoved(pad);
        
        // Retrait de la liste
        m_soundPads.removeOne(pad);
        
        // Supprimer l'objet
        pad->deleteLater();
        
        // Réorganisation de la grille
        reorganizeGrid();
    }
}

void Board::setupUi()
{
    // Configuration du layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Zone de défilement pour les SoundPads
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Widget contenant la grille de SoundPads
    m_contentWidget = new QWidget(m_scrollArea);
    m_gridLayout = new QGridLayout(m_contentWidget);
    m_gridLayout->setContentsMargins(10, 10, 10, 10);
    m_gridLayout->setSpacing(10);
    
    m_scrollArea->setWidget(m_contentWidget);
    
    // Bouton d'ajout de SoundPad
    m_addButton = new QPushButton(tr("+ Ajouter un pad"), this);
    connect(m_addButton, &QPushButton::clicked, this, &Board::addSoundPad);
    
    // Organisation des éléments
    mainLayout->addWidget(m_scrollArea);
    mainLayout->addWidget(m_addButton);
    
    // Configuration du menu contextuel
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Menu"), this);
        
        QAction addPadAction(tr("Ajouter un pad"), this);
        QAction importSoundAction(tr("Importer des sons"), this);
        QAction editTitleAction(tr("Modifier le titre"), this);
        
        connect(&addPadAction, &QAction::triggered, this, &Board::addSoundPad);
        connect(&importSoundAction, &QAction::triggered, this, &Board::importSound);
        connect(&editTitleAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newTitle = QInputDialog::getText(this, 
                tr("Modifier le titre"), 
                tr("Nouveau titre:"), 
                QLineEdit::Normal, 
                m_title, 
                &ok);
            
            if (ok && !newTitle.isEmpty()) {
                setTitle(newTitle);
            }
        });
        
        contextMenu.addAction(&addPadAction);
        contextMenu.addAction(&importSoundAction);
        contextMenu.addAction(&editTitleAction);
        
        contextMenu.exec(mapToGlobal(pos));
    });
}

void Board::reorganizeGrid()
{
    // Effacer le layout existant
    QLayoutItem *child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        delete child;
    }
    
    // Nombre de colonnes (ajuster selon vos préférences)
    const int columns = 4;
    
    // Ajouter les SoundPads à la grille
    for (int i = 0; i < m_soundPads.size(); ++i) {
        int row = i / columns;
        int col = i % columns;
        
        // Création d'un widget pour contenir le SoundPad et son bouton de suppression
        QWidget *container = new QWidget(m_contentWidget);
        QVBoxLayout *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(2, 2, 2, 2);
        containerLayout->setSpacing(2);
        
        // Bouton de suppression
        QPushButton *removeButton = new QPushButton(tr("×"), container);
        removeButton->setMaximumSize(20, 20);
        removeButton->setToolTip(tr("Supprimer ce pad"));
        connect(removeButton, &QPushButton::clicked, this, [this, i]() {
            if (i < m_soundPads.size()) {
                removeSoundPad(m_soundPads[i]);
            }
        });
        
        // Ajout des éléments au container
        containerLayout->addWidget(removeButton, 0, Qt::AlignRight);
        containerLayout->addWidget(m_soundPads[i]);
        
        // Ajout du container à la grille
        m_gridLayout->addWidget(container, row, col);
    }
}

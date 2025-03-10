#include "roomdialog.h"
#include <QFormLayout>
#include <QClipboard>
#include <QApplication>
#include <QUuid>
#include <QMessageBox>

RoomDialog::RoomDialog(Mode mode, const QString &inviteCode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_inviteCode(inviteCode)
{
    setupUI();
}

void RoomDialog::setupUI()
{
    // Configuration commune
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    m_inputEdit = new QLineEdit(this);
    
    QLabel *titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    
    // Configuration spécifique selon le mode
    switch (m_mode) {
        case CreateMode:
        {
            setWindowTitle(tr("Créer une Room"));
            titleLabel->setText(tr("Créer une nouvelle room pour partager vos SoundPads"));
            m_inputEdit->setPlaceholderText(tr("Nom de la room"));
            
            m_primaryButton = new QPushButton(tr("Créer"), this);
            m_secondaryButton = new QPushButton(tr("Annuler"), this);
            m_closeButton = nullptr;
            
            // Validation de la saisie
            m_primaryButton->setEnabled(false);
            connect(m_inputEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
                m_primaryButton->setEnabled(!text.trimmed().isEmpty());
            });
            
            // Connexion des signaux
            connect(m_primaryButton, &QPushButton::clicked, this, &QDialog::accept);
            connect(m_secondaryButton, &QPushButton::clicked, this, &QDialog::reject);
            
            // Layout spécifique
            mainLayout->addWidget(titleLabel);
            
            QFormLayout *formLayout = new QFormLayout();
            formLayout->addRow(tr("Room's name:"), m_inputEdit);
            mainLayout->addLayout(formLayout);
            
            QHBoxLayout *buttonsLayout = new QHBoxLayout();
            buttonsLayout->addWidget(m_secondaryButton);
            buttonsLayout->addWidget(m_primaryButton);
            mainLayout->addLayout(buttonsLayout);
            break;
        }
            
        case JoinMode:
        {
            setWindowTitle(tr("Rejoindre une Room"));
            titleLabel->setText(tr("Entrez le code d'invitation pour rejoindre une room"));
            m_inputEdit->setPlaceholderText(tr("Code d'invitation"));
            
            m_primaryButton = new QPushButton(tr("Rejoindre"), this);
            m_secondaryButton = new QPushButton(tr("Annuler"), this);
            m_closeButton = nullptr;
            
            // Validation de la saisie
            m_primaryButton->setEnabled(false);
            connect(m_inputEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
                m_primaryButton->setEnabled(!text.trimmed().isEmpty());
            });
            
            // Connexion des signaux
            connect(m_primaryButton, &QPushButton::clicked, this, &QDialog::accept);
            connect(m_secondaryButton, &QPushButton::clicked, this, &QDialog::reject);
            
            // Layout spécifique
            mainLayout->addWidget(titleLabel);
            
            QFormLayout *joinFormLayout = new QFormLayout();
            joinFormLayout->addRow(tr("Enter code:"), m_inputEdit);
            mainLayout->addLayout(joinFormLayout);
            
            QHBoxLayout *joinButtonsLayout = new QHBoxLayout();
            joinButtonsLayout->addWidget(m_secondaryButton);
            joinButtonsLayout->addWidget(m_primaryButton);
            mainLayout->addLayout(joinButtonsLayout);
            break;
        }
            
        case InviteMode:
        {
            setWindowTitle(tr("Code d'invitation"));
            titleLabel->setText(tr("Partagez ce code d'invitation pour permettre à d'autres utilisateurs de rejoindre votre room"));
            
            m_inputEdit->setReadOnly(true);
            m_inputEdit->setAlignment(Qt::AlignCenter);
            
            // Si un code existant est fourni, l'utiliser
            if (!m_inviteCode.isEmpty()) {
                m_inputEdit->setText(m_inviteCode);
            }
            
            m_primaryButton = new QPushButton(tr("Générer un nouveau code"), this);
            m_secondaryButton = new QPushButton(tr("Copier"), this);
            m_closeButton = new QPushButton(tr("Fermer"), this);
            
            // Connexion des signaux
            connect(m_primaryButton, &QPushButton::clicked, this, &RoomDialog::generateCode);
            connect(m_secondaryButton, &QPushButton::clicked, this, &RoomDialog::copyToClipboard);
            connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
            
            // Layout spécifique
            mainLayout->addWidget(titleLabel);
            mainLayout->addWidget(m_inputEdit);
            
            QHBoxLayout *inviteButtonsLayout = new QHBoxLayout();
            inviteButtonsLayout->addWidget(m_primaryButton);
            inviteButtonsLayout->addWidget(m_secondaryButton);
            mainLayout->addLayout(inviteButtonsLayout);
            
            mainLayout->addWidget(m_closeButton);
            
            // Génération d'un code initial si nécessaire
            if (m_inviteCode.isEmpty()) {
                generateCode();
            }
            break;
        }
    }
    
    // Configuration commune finale
    setMinimumWidth(350);
}

QString RoomDialog::getRoomName() const
{
    if (m_mode == CreateMode) {
        return m_inputEdit->text().trimmed();
    }
    return QString();
}

QString RoomDialog::getInviteCode() const
{
    if (m_mode == JoinMode) {
        return m_inputEdit->text().trimmed();
    } else if (m_mode == InviteMode) {
        return m_inviteCode;
    }
    return QString();
}

QString RoomDialog::generateCode()
{
    if (m_mode != InviteMode) {
        return QString();
    }
    
    // Génération d'un code d'invitation basé sur un UUID
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Prendre seulement les 8 premiers caractères pour simplifier
    m_inviteCode = uuid.left(8);
    
    // Mise à jour du champ de texte
    m_inputEdit->setText(m_inviteCode);
    
    // Émission du signal
    emit codeGenerated(m_inviteCode);
    
    return m_inviteCode;
}

void RoomDialog::copyToClipboard()
{
    if (m_mode != InviteMode || m_inviteCode.isEmpty()) {
        return;
    }
    
    // Copier le code dans le presse-papier
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_inviteCode);
    
    // Afficher un message de confirmation
    QMessageBox::information(this, tr("Copié"), tr("Le code d'invitation a été copié dans le presse-papier."));
}

#include "invitedialog.h"
#include <QClipboard>
#include <QApplication>
#include <QUuid>
#include <QMessageBox>

InviteDialog::InviteDialog(const QString &inviteCode, QWidget *parent)
    : QDialog(parent)
    , m_inviteCode(inviteCode)
{
    // Configuration de la boîte de dialogue
    setWindowTitle(tr("Code d'invitation"));
    setMinimumWidth(350);
    
    // Création des widgets
    QLabel *titleLabel = new QLabel(tr("Partagez ce code d'invitation pour permettre à d'autres utilisateurs de rejoindre votre room"), this);
    titleLabel->setWordWrap(true);
    
    m_codeEdit = new QLineEdit(this);
    m_codeEdit->setReadOnly(true);
    m_codeEdit->setAlignment(Qt::AlignCenter);
    
    // Si un code existant est fourni, l'utiliser
    if (!m_inviteCode.isEmpty()) {
        m_codeEdit->setText(m_inviteCode);
    }
    
    m_generateButton = new QPushButton(tr("Générer un nouveau code"), this);
    m_copyButton = new QPushButton(tr("Copier"), this);
    m_closeButton = new QPushButton(tr("Fermer"), this);
    
    // Configuration du layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_codeEdit);
    
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_generateButton);
    buttonsLayout->addWidget(m_copyButton);
    mainLayout->addLayout(buttonsLayout);
    
    mainLayout->addWidget(m_closeButton);
    
    // Connexion des signaux
    connect(m_generateButton, &QPushButton::clicked, this, &InviteDialog::generateCode);
    connect(m_copyButton, &QPushButton::clicked, this, &InviteDialog::copyToClipboard);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    // Génération d'un code initial si nécessaire
    if (m_inviteCode.isEmpty()) {
        generateCode();
    }
}

QString InviteDialog::generateCode()
{
    // Génération d'un code d'invitation basé sur un UUID
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Prendre seulement les 8 premiers caractères pour simplifier
    m_inviteCode = uuid.left(8);
    
    // Mise à jour du champ de texte
    m_codeEdit->setText(m_inviteCode);
    
    // Émission du signal
    emit codeGenerated(m_inviteCode);
    
    return m_inviteCode;
}

void InviteDialog::copyToClipboard()
{
    if (!m_inviteCode.isEmpty()) {
        // Copier le code dans le presse-papier
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(m_inviteCode);
        
        // Afficher un message de confirmation
        QMessageBox::information(this, tr("Copié"), tr("Le code d'invitation a été copié dans le presse-papier."));
    }
}

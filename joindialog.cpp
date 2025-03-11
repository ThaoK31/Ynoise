#include "joindialog.h"
#include <QFormLayout>

JoinDialog::JoinDialog(QWidget *parent)
    : QDialog(parent)
{
    // Configuration de la boîte de dialogue
    setWindowTitle(tr("Rejoindre une Room"));
    setMinimumWidth(300);
    
    // Création des widgets
    QLabel *titleLabel = new QLabel(tr("Entrez le code d'invitation pour rejoindre une room"), this);
    titleLabel->setWordWrap(true);
    
    m_codeEdit = new QLineEdit(this);
    m_codeEdit->setPlaceholderText(tr("Code d'invitation"));
    
    m_joinButton = new QPushButton(tr("Rejoindre"), this);
    m_cancelButton = new QPushButton(tr("Annuler"), this);
    
    // Configuration du layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(tr("Enter code:"), m_codeEdit);
    mainLayout->addLayout(formLayout);
    
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_joinButton);
    mainLayout->addLayout(buttonsLayout);
    
    // Connexion des signaux
    connect(m_joinButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Validation de la saisie
    m_joinButton->setEnabled(false);
    connect(m_codeEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_joinButton->setEnabled(!text.trimmed().isEmpty());
    });
}

QString JoinDialog::getInviteCode() const
{
    return m_codeEdit->text().trimmed();
}

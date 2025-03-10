#include "createdialog.h"
#include <QFormLayout>

CreateDialog::CreateDialog(QWidget *parent)
    : QDialog(parent)
{
    // Configuration de la boîte de dialogue
    setWindowTitle(tr("Créer une Room"));
    setMinimumWidth(300);
    
    // Création des widgets
    QLabel *titleLabel = new QLabel(tr("Créer une nouvelle room pour partager vos SoundPads"), this);
    titleLabel->setWordWrap(true);
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Nom de la room"));
    
    m_createButton = new QPushButton(tr("Créer"), this);
    m_cancelButton = new QPushButton(tr("Annuler"), this);
    
    // Configuration du layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(tr("Room's name:"), m_nameEdit);
    mainLayout->addLayout(formLayout);
    
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_createButton);
    mainLayout->addLayout(buttonsLayout);
    
    // Connexion des signaux
    connect(m_createButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Validation de la saisie
    m_createButton->setEnabled(false);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_createButton->setEnabled(!text.trimmed().isEmpty());
    });
}

QString CreateDialog::getRoomName() const
{
    return m_nameEdit->text().trimmed();
}

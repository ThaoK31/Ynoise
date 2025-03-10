#ifndef INVITEDIALOG_H
#define INVITEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief Dialogue pour générer et partager un code d'invitation
 */
class InviteDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param inviteCode Code d'invitation existant (optionnel)
     * @param parent Widget parent
     */
    explicit InviteDialog(const QString &inviteCode = QString(), QWidget *parent = nullptr);

public slots:
    /**
     * @brief Génère un nouveau code d'invitation
     * @return Nouveau code d'invitation
     */
    QString generateCode();
    
    /**
     * @brief Copie le code d'invitation dans le presse-papier
     */
    void copyToClipboard();

signals:
    /**
     * @brief Signal émis lorsqu'un nouveau code est généré
     * @param code Nouveau code
     */
    void codeGenerated(const QString &code);

private:
    QString m_inviteCode;           // Code d'invitation actuel
    QLineEdit *m_codeEdit;          // Champ pour afficher le code
    QPushButton *m_generateButton;  // Bouton pour générer un nouveau code
    QPushButton *m_copyButton;      // Bouton pour copier le code
    QPushButton *m_closeButton;     // Bouton pour fermer la boîte de dialogue
};

#endif // INVITEDIALOG_H

#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief Dialogue unifié pour les opérations liées aux Rooms
 * 
 * Cette classe remplace les anciennes classes:
 * - CreateDialog
 * - JoinDialog
 * - InviteDialog
 */
class RoomDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Mode d'opération du dialogue
     */
    enum Mode {
        CreateMode,  ///< Mode création d'une room
        JoinMode,    ///< Mode pour rejoindre une room
        InviteMode   ///< Mode pour générer un code d'invitation
    };

    /**
     * @brief Constructeur
     * @param mode Mode d'opération du dialogue
     * @param inviteCode Code d'invitation existant (pour le mode Invite)
     * @param parent Widget parent
     */
    explicit RoomDialog(Mode mode, const QString &inviteCode = QString(), QWidget *parent = nullptr);
    
    /**
     * @brief Récupère le nom de la Room (pour CreateMode)
     * @return Nom de la Room
     */
    QString getRoomName() const;
    
    /**
     * @brief Récupère le code d'invitation (pour JoinMode)
     * @return Code d'invitation
     */
    QString getInviteCode() const;

public slots:
    /**
     * @brief Génère un nouveau code d'invitation (pour InviteMode)
     * @return Nouveau code d'invitation
     */
    QString generateCode();
    
    /**
     * @brief Copie le code d'invitation dans le presse-papier (pour InviteMode)
     */
    void copyToClipboard();

signals:
    /**
     * @brief Signal émis lorsqu'un nouveau code est généré (pour InviteMode)
     * @param code Nouveau code
     */
    void codeGenerated(const QString &code);

private:
    Mode m_mode;                    ///< Mode d'opération du dialogue
    QString m_inviteCode;           ///< Code d'invitation actuel (pour InviteMode)
    
    QLineEdit *m_inputEdit;         ///< Champ d'entrée (nom ou code selon le mode)
    
    // Boutons communs
    QPushButton *m_primaryButton;   ///< Bouton principal (Créer/Rejoindre/Générer)
    QPushButton *m_secondaryButton; ///< Bouton secondaire (Annuler/Copier)
    QPushButton *m_closeButton;     ///< Bouton de fermeture (pour InviteMode uniquement)
    
    /**
     * @brief Configure l'interface utilisateur selon le mode
     */
    void setupUI();
};

#endif // ROOMDIALOG_H

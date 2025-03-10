#ifndef JOINDIALOG_H
#define JOINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief Dialogue pour rejoindre une Room
 */
class JoinDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Widget parent
     */
    explicit JoinDialog(QWidget *parent = nullptr);
    
    /**
     * @brief Récupère le code d'invitation
     * @return Code d'invitation
     */
    QString getInviteCode() const;

private:
    QLineEdit *m_codeEdit;      // Champ pour le code d'invitation
    QPushButton *m_joinButton;  // Bouton pour rejoindre
    QPushButton *m_cancelButton; // Bouton d'annulation
};

#endif // JOINDIALOG_H

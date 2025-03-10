#ifndef CREATEDIALOG_H
#define CREATEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief Dialogue de création d'une Room
 */
class CreateDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Widget parent
     */
    explicit CreateDialog(QWidget *parent = nullptr);
    
    /**
     * @brief Récupère le nom de la Room
     * @return Nom de la Room
     */
    QString getRoomName() const;

private:
    QLineEdit *m_nameEdit;   // Champ pour le nom de la Room
    QPushButton *m_createButton;  // Bouton de création
    QPushButton *m_cancelButton;  // Bouton d'annulation
};

#endif // CREATEDIALOG_H

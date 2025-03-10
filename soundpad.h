#ifndef SOUNDPAD_H
#define SOUNDPAD_H

#include <QString>
#include <QPixmap>
#include <QKeySequence>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QAudioOutput>
#include <QMediaPlayer>

/**
 * @brief Classe représentant un pad sonore pouvant jouer un son avec une image associée
 */
class SoundPad : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de SoundPad
     * @param title Titre du pad
     * @param filePath Chemin vers le fichier audio
     * @param imagePath Chemin vers l'image (optionnel)
     * @param canDuplicatePlay Si true, peut jouer plusieurs fois simultanément
     * @param shortcut Raccourci clavier (optionnel)
     * @param parent Widget parent
     */
    explicit SoundPad(const QString &title = "", 
                      const QString &filePath = "",
                      const QString &imagePath = "",
                      bool canDuplicatePlay = false,
                      const QKeySequence &shortcut = QKeySequence(),
                      QWidget *parent = nullptr);
    
    ~SoundPad();

    // Getters et setters
    QString getTitle() const { return m_title; }
    void setTitle(const QString &title);
    
    QString getFilePath() const { return m_filePath; }
    void setFilePath(const QString &filePath);
    
    QString getImagePath() const { return m_imagePath; }
    void setImagePath(const QString &imagePath);
    
    QPixmap getImage() const { return m_image; }
    void setImage(const QPixmap &image);
    
    bool getCanDuplicatePlay() const { return m_canDuplicatePlay; }
    void setCanDuplicatePlay(bool canDuplicatePlay);
    
    bool isPlaying() const;
    
    QKeySequence getShortcut() const { return m_shortcut; }
    void setShortcut(const QKeySequence &shortcut);

public slots:
    /**
     * @brief Importe un fichier audio
     * @return true si l'import a réussi
     */
    bool importSound();
    
    /**
     * @brief Joue le son associé au pad
     */
    void play();
    
    /**
     * @brief Ouvre une fenêtre pour éditer les métadonnées
     */
    void editMetadata();

signals:
    /**
     * @brief Signal émis lorsque les métadonnées sont modifiées
     */
    void metadataChanged();
    
    /**
     * @brief Signal émis lorsque les propriétés du SoundPad sont modifiées
     * @param pad Le SoundPad modifié
     */
    void soundPadModified(SoundPad* pad);

protected:
    /**
     * @brief Gère les événements de glisser-déposer
     */
    bool dragAndDrop();
    
    /**
     * @brief Capte les événements clavier pour les raccourcis
     */
    void keyPressEvent(QKeyEvent *event) override;
    
    /**
     * @brief Traite les événements de glisser-déposer
     */
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString m_title;          // Titre du pad
    QString m_filePath;       // Chemin vers le fichier audio
    QString m_imagePath;      // Chemin vers l'image
    QPixmap m_image;          // Image associée
    bool m_canDuplicatePlay;  // Si true, peut jouer plusieurs fois simultanément 
    bool m_isPlaying;         // Indique si le son est en cours de lecture
    QKeySequence m_shortcut;  // Raccourci clavier associé

    // Éléments UI
    QPushButton *m_button;    // Bouton principal du pad
    QLabel *m_imageLabel;     // Label pour afficher l'image
    QLabel *m_titleLabel;     // Label pour afficher le titre
    QVBoxLayout *m_layout;    // Layout principal

    // Lecteur audio
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;

    /**
     * @brief Configure l'apparence du SoundPad
     */
    void setupUi();
    
    /**
     * @brief Met à jour l'apparence en fonction des propriétés
     */
    void updateUI();
};

#endif // SOUNDPAD_H

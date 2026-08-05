#ifndef CUSTOMACTIONSETTINGSWIDGET_H
#define CUSTOMACTIONSETTINGSWIDGET_H

#include <QVector>
#include <QWidget>

class QSettings;
class QVBoxLayout;
class QScrollArea;
class QFrame;

struct CustomActionEntry {
    QString fileType;
    QString name;
    QString bundledIconName;
    QString iconPath;
    QString command;
    QString submenu;
    bool monitorOutput = false;
};

class CustomActionSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit CustomActionSettingsWidget(QWidget *parent = nullptr);

    void loadFromSettings(QSettings *settings);
    void saveToSettings(QSettings *settings);
    const QVector<CustomActionEntry> &entries() const { return actionEntries; }

    void setDefaults(const QVector<QStringList> &rows);
    void clearAll();

signals:
    void entriesChanged();

public slots:
    void addActionModule();
    void showUsageInfo();

private:
    void rebuildModules();
    void regroupBySubmenu();
    QFrame *buildModuleFrame(int index);

    QScrollArea *scrollArea;
    QVBoxLayout *modulesLayout;
    QVector<CustomActionEntry> actionEntries;
};

#endif

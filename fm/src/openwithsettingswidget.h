#ifndef OPENWITHSETTINGSWIDGET_H
#define OPENWITHSETTINGSWIDGET_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QWidget>

class QFormLayout;
class QVBoxLayout;
class QScrollArea;
struct OpenWithEntry;

class OpenWithSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit OpenWithSettingsWidget(QWidget *parent = nullptr);

    void loadFromConfig();
    void saveToConfig();

private slots:
    void addSuffixModule();

private:
    QWidget *buildSuffixSection();
    QWidget *buildCategorySection(const QString &categoryId);
    void fillEntryForm(QFormLayout *form, OpenWithEntry *entry, const QStringList &suffixesHint);

    QScrollArea *scrollArea = nullptr;
    QWidget *contentWidget = nullptr;
    QVBoxLayout *suffixModulesLayout = nullptr;
    QHash<QString, QVBoxLayout *> categoryLayouts;
};

#endif

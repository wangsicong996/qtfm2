#ifndef UICOLORS_H
#define UICOLORS_H

#include <QColor>
#include <QHash>
#include <QString>

class QSettings;

/** Customizable UI region colors (empty/invalid = theme default). */
enum class UiColorId {
    FilePaneInactive = 0,
    FilePaneActive,
    BookmarksList,
    BookmarkGroupBar,
    BookmarkGroupButton,
    SidebarTabSelected,
    SidebarTabUnselected,
    TopChrome,
    TopChromeButton,
    Count
};

struct UiColorSet {
    QHash<int, QColor> colors; // key = int(UiColorId), invalid = default

    QColor get(UiColorId id) const
    {
        return colors.value(int(id), QColor());
    }
    void set(UiColorId id, const QColor &c)
    {
        if (!c.isValid()) {
            colors.remove(int(id));
        } else {
            colors.insert(int(id), c);
        }
    }
    bool isDefault(UiColorId id) const
    {
        const QColor c = get(id);
        return !c.isValid();
    }
};

class UiColors
{
public:
    static QString settingsKey(UiColorId id, bool dark);
    static QString jsonKey(UiColorId id);
    static UiColorId idFromJsonKey(const QString &key);

    static void load(QSettings *settings, UiColorSet *light, UiColorSet *dark);
    static void save(QSettings *settings, const UiColorSet &light, const UiColorSet &dark);

    static QColor resolve(const UiColorSet &set, UiColorId id, const QColor &fallback);

    static bool exportToJson(const QString &filePath,
                             const UiColorSet &light,
                             const UiColorSet &dark,
                             QString *errorMessage = nullptr);
    static bool importFromJson(const QString &filePath,
                               UiColorSet *light,
                               UiColorSet *dark,
                               QString *errorMessage = nullptr);

    /** English id used in JSON; UI labels stay in the settings dialog via tr(). */
    static const char *jsonKeyCStr(UiColorId id);
};

#endif // UICOLORS_H

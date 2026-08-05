#include "uicolors.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace {

const char *kJsonKeys[int(UiColorId::Count)] = {
    "filePaneInactive",
    "filePaneActive",
    "bookmarksList",
    "bookmarkGroupBar",
    "bookmarkGroupButton",
    "sidebarTabSelected",
    "sidebarTabUnselected",
    "topChrome",
    "topChromeButton",
};

QString colorToSetting(const QColor &c)
{
    return c.isValid() ? c.name(QColor::HexRgb) : QString();
}

QColor colorFromSetting(const QString &raw)
{
    if (raw.isEmpty()) {
        return QColor();
    }
    const QColor c(raw);
    return c.isValid() ? c : QColor();
}

void loadSet(QSettings *settings, UiColorSet *set, bool dark)
{
    if (!settings || !set) {
        return;
    }
    set->colors.clear();
    for (int i = 0; i < int(UiColorId::Count); ++i) {
        const auto id = UiColorId(i);
        const QString raw = settings->value(UiColors::settingsKey(id, dark)).toString();
        const QColor c = colorFromSetting(raw);
        if (c.isValid()) {
            set->set(id, c);
        }
    }
}

void saveSet(QSettings *settings, const UiColorSet &set, bool dark)
{
    if (!settings) {
        return;
    }
    for (int i = 0; i < int(UiColorId::Count); ++i) {
        const auto id = UiColorId(i);
        settings->setValue(UiColors::settingsKey(id, dark), colorToSetting(set.get(id)));
    }
}

QJsonObject setToJson(const UiColorSet &set)
{
    QJsonObject obj;
    for (int i = 0; i < int(UiColorId::Count); ++i) {
        const auto id = UiColorId(i);
        const QColor c = set.get(id);
        obj.insert(QLatin1String(kJsonKeys[i]), c.isValid() ? c.name(QColor::HexRgb) : QString());
    }
    return obj;
}

void setFromJson(const QJsonObject &obj, UiColorSet *set)
{
    if (!set) {
        return;
    }
    set->colors.clear();
    for (int i = 0; i < int(UiColorId::Count); ++i) {
        const auto id = UiColorId(i);
        const QString raw = obj.value(QLatin1String(kJsonKeys[i])).toString();
        const QColor c = colorFromSetting(raw);
        if (c.isValid()) {
            set->set(id, c);
        }
    }
}

} // namespace

QString UiColors::settingsKey(UiColorId id, bool dark)
{
    const int i = int(id);
    if (i < 0 || i >= int(UiColorId::Count)) {
        return QString();
    }
    return QStringLiteral("uiColor.%1.%2")
        .arg(QLatin1String(kJsonKeys[i]), dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

QString UiColors::jsonKey(UiColorId id)
{
    const int i = int(id);
    if (i < 0 || i >= int(UiColorId::Count)) {
        return QString();
    }
    return QLatin1String(kJsonKeys[i]);
}

const char *UiColors::jsonKeyCStr(UiColorId id)
{
    const int i = int(id);
    if (i < 0 || i >= int(UiColorId::Count)) {
        return "";
    }
    return kJsonKeys[i];
}

UiColorId UiColors::idFromJsonKey(const QString &key)
{
    for (int i = 0; i < int(UiColorId::Count); ++i) {
        if (key == QLatin1String(kJsonKeys[i])) {
            return UiColorId(i);
        }
    }
    return UiColorId::Count;
}

void UiColors::load(QSettings *settings, UiColorSet *light, UiColorSet *dark)
{
    loadSet(settings, light, false);
    loadSet(settings, dark, true);

    // Migrate legacy dual-pane keys (theme-agnostic) into both sets if new keys empty.
    if (settings) {
        const QString legacyInactive = settings->value(QStringLiteral("dualPaneInactiveColor")).toString();
        const QString legacyActive = settings->value(QStringLiteral("dualPaneActiveColor")).toString();
        const QColor inact = colorFromSetting(legacyInactive);
        const QColor act = colorFromSetting(legacyActive);
        if (light && light->isDefault(UiColorId::FilePaneInactive) && inact.isValid()) {
            light->set(UiColorId::FilePaneInactive, inact);
        }
        if (light && light->isDefault(UiColorId::FilePaneActive) && act.isValid()) {
            light->set(UiColorId::FilePaneActive, act);
        }
        if (dark && dark->isDefault(UiColorId::FilePaneInactive) && inact.isValid()) {
            dark->set(UiColorId::FilePaneInactive, inact);
        }
        if (dark && dark->isDefault(UiColorId::FilePaneActive) && act.isValid()) {
            dark->set(UiColorId::FilePaneActive, act);
        }
    }
}

void UiColors::save(QSettings *settings, const UiColorSet &light, const UiColorSet &dark)
{
    saveSet(settings, light, false);
    saveSet(settings, dark, true);
    // Keep legacy keys in sync with the currently preferred pair (light), for older builds.
    if (settings) {
        settings->setValue(QStringLiteral("dualPaneInactiveColor"),
                           colorToSetting(light.get(UiColorId::FilePaneInactive)));
        settings->setValue(QStringLiteral("dualPaneActiveColor"),
                           colorToSetting(light.get(UiColorId::FilePaneActive)));
    }
}

QColor UiColors::resolve(const UiColorSet &set, UiColorId id, const QColor &fallback)
{
    const QColor c = set.get(id);
    return c.isValid() ? c : fallback;
}

bool UiColors::exportToJson(const QString &filePath,
                            const UiColorSet &light,
                            const UiColorSet &dark,
                            QString *errorMessage)
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("light"), setToJson(light));
    root.insert(QStringLiteral("dark"), setToJson(dark));

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

bool UiColors::importFromJson(const QString &filePath,
                              UiColorSet *light,
                              UiColorSet *dark,
                              QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = err.errorString();
        }
        return false;
    }
    const QJsonObject root = doc.object();
    setFromJson(root.value(QStringLiteral("light")).toObject(), light);
    setFromJson(root.value(QStringLiteral("dark")).toObject(), dark);
    return true;
}

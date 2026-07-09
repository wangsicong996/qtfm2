#include "macdisks.h"

#if defined(__APPLE__) && !defined(__IOS__)

#import <Foundation/Foundation.h>

#include <QStringList>

namespace MacDisks {

static QString gLastError;

static QString fromNSString(NSString *s)
{
    if (!s || s.length == 0) {
        return QString();
    }
    return QString::fromNSString(s);
}

static bool runDiskutil(const QStringList &args, QString *stdOut = nullptr)
{
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/usr/sbin/diskutil";
    NSMutableArray<NSString *> *nsArgs = [NSMutableArray array];
    for (const QString &a : args) {
        [nsArgs addObject:a.toNSString()];
    }
    task.arguments = nsArgs;

    NSPipe *outPipe = [NSPipe pipe];
    task.standardOutput = outPipe;
    task.standardError = outPipe;

    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *ex) {
        gLastError = QString::fromNSString(ex.reason);
        return false;
    }

    NSData *data = [[outPipe fileHandleForReading] readDataToEndOfFile];
    const QString output = QString::fromUtf8(
        reinterpret_cast<const char *>(data.bytes),
        static_cast<int>(data.length));

    if (stdOut) {
        *stdOut = output;
    }

    if (task.terminationStatus != 0) {
        gLastError = output.trimmed();
        if (gLastError.isEmpty()) {
            gLastError = QStringLiteral("diskutil failed (exit %1)").arg(task.terminationStatus);
        }
        return false;
    }
    return true;
}

static bool contentIsSkippable(const QString &content)
{
    if (content.isEmpty()) {
        return false;
    }
    static const char *skip[] = {
        "EFI",
        "Apple_Boot",
        "Apple_Recovery",
        "Apple_APFS_Recovery",
        "Apple_APFS_ISC",
        "Apple_Kernel_Recovery",
        "GUID_partition_scheme",
        "FDisk_partition_scheme",
        "Apple_partition_scheme",
        "Container",
    };
    for (const char *s : skip) {
        if (content.contains(QLatin1String(s))) {
            return true;
        }
    }
    return false;
}

static QString wholeDiskFromIdentifier(const QString &ident)
{
    const int s = ident.indexOf(QLatin1Char('s'));
    if (s > 0) {
        return ident.left(s);
    }
    return ident;
}

static void appendVolume(const NSDictionary *dict,
                         const QString &parentMediaName,
                         bool parentRemovable,
                         const QString &physicalDiskGroup,
                         qint64 physicalDiskSizeBytes,
                         QVector<MacDiskVolume> *out)
{
    if (!dict || !out) {
        return;
    }

    const QString ident = fromNSString(dict[@"DeviceIdentifier"]);
    if (ident.isEmpty()) {
        return;
    }

    const QString content = fromNSString(dict[@"Content"]);
    const QString mount = fromNSString(dict[@"MountPoint"]);
    const QString volName = fromNSString(dict[@"VolumeName"]);
    const QString mediaName = fromNSString(dict[@"MediaName"]);
    const bool removable = [dict[@"RemovableMedia"] boolValue]
                           || [dict[@"Ejectable"] boolValue]
                           || parentRemovable;
    const bool internal = [dict[@"Internal"] boolValue];
    const bool optical = [dict[@"OpticalDiskMedia"] boolValue]
                         || content.contains(QStringLiteral("CD_DA"))
                         || content.contains(QStringLiteral("DVD"));

    const bool hasUserLabel = !volName.isEmpty() || !mediaName.isEmpty();
    const bool mountableLeaf = !contentIsSkippable(content)
                               && (hasUserLabel || !mount.isEmpty() || removable);

    if (mountableLeaf) {
        QString subtitle = volName;
        if (subtitle.isEmpty()) {
            subtitle = mediaName;
        }
        if (subtitle.isEmpty()) {
            subtitle = parentMediaName;
        }
        if (subtitle.isEmpty()) {
            subtitle = QStringLiteral("Storage");
        }

        MacDiskVolume row;
        row.deviceIdentifier = ident;
        row.displayTitle = QStringLiteral("%1 — %2").arg(ident, subtitle);
        row.mountPoint = mount;
        row.isOptical = optical;
        row.wholeDiskIdentifier = wholeDiskFromIdentifier(ident);
        row.physicalDiskGroup = physicalDiskGroup.isEmpty()
                                    ? wholeDiskFromIdentifier(ident)
                                    : physicalDiskGroup;
        row.physicalDiskSizeBytes = physicalDiskSizeBytes;

        bool duplicate = false;
        for (const MacDiskVolume &existing : *out) {
            if (existing.deviceIdentifier == row.deviceIdentifier) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            // Hide sealed internal system snapshots without a mount point.
            if (internal && !removable && mount.isEmpty() && volName.isEmpty()) {
                return;
            }
            out->append(row);
        }
    }
}

static void walkNode(const NSDictionary *dict,
                     const QString &parentMediaName,
                     bool parentRemovable,
                     const QString &physicalDiskGroup,
                     qint64 physicalDiskSizeBytes,
                     QVector<MacDiskVolume> *out)
{
    if (!dict || !out) {
        return;
    }

    const QString mediaName = fromNSString(dict[@"MediaName"]);
    const QString effectiveMedia = mediaName.isEmpty() ? parentMediaName : mediaName;
    const bool removable = [dict[@"RemovableMedia"] boolValue]
                           || [dict[@"Ejectable"] boolValue]
                           || parentRemovable;

    if (NSArray *parts = dict[@"Partitions"]) {
        for (NSDictionary *child in parts) {
            if ([child isKindOfClass:[NSDictionary class]]) {
                walkNode(child, effectiveMedia, removable, physicalDiskGroup,
                         physicalDiskSizeBytes, out);
            }
        }
    }
    if (NSArray *apfs = dict[@"APFSVolumes"]) {
        for (NSDictionary *child in apfs) {
            if ([child isKindOfClass:[NSDictionary class]]) {
                walkNode(child, effectiveMedia, removable, physicalDiskGroup,
                         physicalDiskSizeBytes, out);
            }
        }
    }

    appendVolume(dict, effectiveMedia, removable, physicalDiskGroup,
                 physicalDiskSizeBytes, out);
}

QVector<MacDiskVolume> listVolumes()
{
    gLastError.clear();
    QVector<MacDiskVolume> result;

    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/usr/sbin/diskutil";
    task.arguments = @[ @"list", @"-plist" ];
    NSPipe *pipe = [NSPipe pipe];
    task.standardOutput = pipe;
    task.standardError = pipe;

    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *ex) {
        gLastError = QString::fromNSString(ex.reason);
        return result;
    }

    if (task.terminationStatus != 0) {
        NSData *errData = [[pipe fileHandleForReading] readDataToEndOfFile];
        gLastError = QString::fromUtf8(
            reinterpret_cast<const char *>(errData.bytes),
            static_cast<int>(errData.length)).trimmed();
        return result;
    }

    NSData *data = [[pipe fileHandleForReading] readDataToEndOfFile];
    NSError *error = nil;
    id plist = [NSPropertyListSerialization propertyListWithData:data
                                                           options:NSPropertyListImmutable
                                                            format:nullptr
                                                             error:&error];
    if (!plist || ![plist isKindOfClass:[NSDictionary class]]) {
        gLastError = error ? QString::fromNSString(error.localizedDescription)
                           : QStringLiteral("diskutil plist parse failed");
        return result;
    }

    NSDictionary *root = (NSDictionary *)plist;
    NSArray *all = root[@"AllDisksAndPartitions"];
    if (![all isKindOfClass:[NSArray class]]) {
        return result;
    }

    for (NSDictionary *disk in all) {
        if ([disk isKindOfClass:[NSDictionary class]]) {
            const QString topDisk = fromNSString(disk[@"DeviceIdentifier"]);
            const qint64 diskBytes = static_cast<qint64>([disk[@"Size"] unsignedLongLongValue]);
            walkNode(disk, QString(), false, topDisk, diskBytes, &result);
        }
    }

    return result;
}

bool mountVolume(const QString &deviceIdentifier)
{
    gLastError.clear();
    if (deviceIdentifier.isEmpty()) {
        gLastError = QStringLiteral("empty device id");
        return false;
    }
    return runDiskutil(QStringList{QStringLiteral("mount"), deviceIdentifier});
}

bool unmountVolume(const QString &deviceIdentifier)
{
    gLastError.clear();
    if (deviceIdentifier.isEmpty()) {
        return false;
    }
    return runDiskutil(QStringList{QStringLiteral("unmount"), deviceIdentifier});
}

bool ejectWholeDisk(const QString &wholeDiskIdentifier)
{
    gLastError.clear();
    if (wholeDiskIdentifier.isEmpty()) {
        return false;
    }
    return runDiskutil(QStringList{QStringLiteral("eject"), wholeDiskIdentifier});
}

QString lastErrorMessage()
{
    return gLastError;
}

bool volumeSpaceBytes(const QString &deviceIdentifier,
                      qint64 *usedBytes,
                      qint64 *totalBytes,
                      const QString &mountPoint)
{
    if (!usedBytes || !totalBytes || deviceIdentifier.isEmpty()) {
        return false;
    }
    *usedBytes = 0;
    *totalBytes = 0;

    if (!mountPoint.isEmpty()) {
        NSURL *url = [NSURL fileURLWithPath:mountPoint.toNSString() isDirectory:YES];
        NSNumber *totalNum = nil;
        NSNumber *availNum = nil;
        if ([url getResourceValue:&totalNum forKey:NSURLVolumeTotalCapacityKey error:nil]
            && [url getResourceValue:&availNum
                              forKey:NSURLVolumeAvailableCapacityForImportantUsageKey
                               error:nil]
            && totalNum && availNum) {
            const qint64 total = static_cast<qint64>(totalNum.longLongValue);
            const qint64 avail = static_cast<qint64>(availNum.longLongValue);
            if (total > 0) {
                *totalBytes = total;
                *usedBytes = qMax<qint64>(0, total - avail);
                return true;
            }
        }
        if ([url getResourceValue:&totalNum forKey:NSURLVolumeTotalCapacityKey error:nil]
            && [url getResourceValue:&availNum forKey:NSURLVolumeAvailableCapacityKey error:nil]
            && totalNum && availNum) {
            const qint64 total = static_cast<qint64>(totalNum.longLongValue);
            const qint64 avail = static_cast<qint64>(availNum.longLongValue);
            if (total > 0) {
                *totalBytes = total;
                *usedBytes = qMax<qint64>(0, total - avail);
                return true;
            }
        }
    }

    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/usr/sbin/diskutil";
    task.arguments = @[ @"info", @"-plist", deviceIdentifier.toNSString() ];
    NSPipe *pipe = [NSPipe pipe];
    task.standardOutput = pipe;
    task.standardError = pipe;

    @try {
        [task launch];
        [task waitUntilExit];
    } @catch (NSException *ex) {
        gLastError = QString::fromNSString(ex.reason);
        return false;
    }

    if (task.terminationStatus != 0) {
        NSData *errData = [[pipe fileHandleForReading] readDataToEndOfFile];
        gLastError = QString::fromUtf8(
            reinterpret_cast<const char *>(errData.bytes),
            static_cast<int>(errData.length)).trimmed();
        return false;
    }

    NSData *data = [[pipe fileHandleForReading] readDataToEndOfFile];
    NSError *error = nil;
    id plist = [NSPropertyListSerialization propertyListWithData:data
                                                           options:NSPropertyListImmutable
                                                            format:nullptr
                                                             error:&error];
    if (!plist || ![plist isKindOfClass:[NSDictionary class]]) {
        gLastError = error ? QString::fromNSString(error.localizedDescription)
                           : QStringLiteral("diskutil info plist parse failed");
        return false;
    }

    NSDictionary *dict = (NSDictionary *)plist;
    auto readLong = [&dict](NSString *key) -> qint64 {
        id value = dict[key];
        if ([value respondsToSelector:@selector(longLongValue)]) {
            return static_cast<qint64>([value longLongValue]);
        }
        return 0;
    };

    qint64 total = readLong(@"TotalSize");
    if (total <= 0) {
        total = readLong(@"Size");
    }
    if (total <= 0) {
        total = readLong(@"APFSSize");
    }
    qint64 free = readLong(@"VolumeFreeSpace");
    if (free <= 0) {
        free = readLong(@"FreeSpace");
    }
    qint64 used = readLong(@"VolumeUsedSpace");
    if (used <= 0) {
        used = readLong(@"UsedSpace");
    }
    if (total <= 0) {
        return false;
    }
    *totalBytes = total;
    if (used > 0) {
        *usedBytes = qMin(used, total);
    } else if (free > 0) {
        *usedBytes = qMax<qint64>(0, total - free);
    } else {
        *usedBytes = 0;
    }
    return true;
}

} // namespace MacDisks

#endif
